#include "db.h"
#include "log.h"
#include "lnchannel.h"
#include "lnchannel_internal.h"
#include "permute_tx.h"
#include "close_tx.h"
#include "commit_tx.h"
#include "output_to_htlc.h"
#include "pseudorand.h"
#include "remove_dust.h"
#include "secrets.h"
#include "btcnetwork/c/chaintopology.h"
#include "btcnetwork/c/watch.h"
#include "utils/utils.h"
#include "utils/sodium/randombytes.h"
#include <bitcoin/base58.h>
#include <bitcoin/address.h>
#include <bitcoin/script.h>
#include <bitcoin/tx.h>
#include <ccan/array_size/array_size.h>
#include <ccan/cast/cast.h>
#include <ccan/list/list.h>
#include <ccan/mem/mem.h>
#include <ccan/noerr/noerr.h>
#include <ccan/ptrint/ptrint.h>
#include <ccan/str/hex/hex.h>
#include <ccan/structeq/structeq.h>
#include <ccan/tal/str/str.h>
#include <ccan/tal/tal.h>
#include <errno.h>
#include <inttypes.h>
#include <stdlib.h>
#include <sys/types.h>


bool lnchn_add_htlc(struct LNchannel *chn, u64 msatoshi,
    unsigned int expiry,
    const struct sha256 *rhash,
    const u8 route,
    enum fail_error *error_code)
{
    //h = internal_new_htlc(lnchn, t_beg->action.add.mstatoshi,
    //    t_beg->rhash, t_beg->action.add.expiry, 0, RCVD_ADD_HTLC);


    return false;
}


static void do_htlc_update(struct LNchannel *lnchn, 
    struct htlc *h, const struct htlc *yah) {

    assert(htlc_route_is_chain(h));

    if (htlc_is_fixed(h) && htlc_is_dead(yah)) {
        if (yah->r) {
            internal_htlc_fullfill(lnchn, yah->r, h);
        }
        else{
            assert(yah->fail);
            internal_htlc_fail(lnchn, yah->fail, tal_count(yah->fail), h);
        }

    }
    //htlc which is not added
    else if (htlc_is_fixed(yah) && !h->src_expiry) {
        h->src_expiry = tal(h, struct abs_locktime);
        *h->src_expiry = yah->expiry;
        h->deadline = abs_locktime_to_blocks(h->src_expiry)
            - lnchn->dstate->config.deadline_blocks;
    }
    else {
        log_broken_struct(lnchn->log, "htlc [%s] is not need to update",
            struct htlc, h);
    }

}

static void set_htlc_rval(struct LNchannel *lnchn,
    struct htlc *htlc, const struct preimage *rval)
{
    assert(!htlc->r);
    assert(!htlc->fail);
    htlc->r = tal_dup(htlc, struct preimage, rval);
    db_htlc_fulfilled(lnchn, htlc);
}

static void set_htlc_fail(struct LNchannel *lnchn,
    struct htlc *htlc, const void *fail, size_t len)
{
    assert(!htlc->r);
    assert(!htlc->fail);
    htlc->fail = tal_dup_arr(htlc, u8, fail, len, 0);
    db_htlc_failed(lnchn, htlc);
}


void internal_htlc_fail(struct LNchannel *chn, const u8 *fail, size_t len, struct htlc *h)
{
    //for chained htlc, resolved is better
    if (h->r && htlc_route_is_chain(h))return;

    //clear previous fail or resolution
    if (h->r) {
        tal_free(h->r);
        h->r = NULL;
    }
    else if (h->fail) {
        tal_free(h->fail);
        h->fail = NULL;
    }

    set_htlc_fail(chn, h, fail, len);
}

void internal_htlc_fullfill(struct LNchannel *chn, const struct preimage *r, struct htlc *h)
{
    struct sha256 sha;

    if (h->r)return; //no need to resolve twice                     
    else if (h->fail && htlc_route_is_chain(h)) {
        //for chained htlc, resolved is better
        tal_free(h->fail);
        h->fail = NULL;
    }

    sha256(&sha, r, sizeof(*r));

    if (!structeq(&sha, &h->rhash)) {
        log_broken(chn->log,
            "We receive unexpected preimage [%s] which should not resolve the hash %s",
            tal_hexstr(chn, r, sizeof(*r)),
            tal_hexstr(chn, &h->rhash, sizeof(h->rhash)));
        return;
    }

    set_htlc_rval(chn, h, r);
}

void internal_htlc_update(struct LNchannel *lnchn, struct htlc *h) {

    const struct htlc *yah = lite_query_htlc_direct(lnchn->dstate->channels,
        &h->rhash, htlc_route_has_source(h));

    if (!yah) {
        if (h->state == RCVD_REMOVE_COMMIT) {
             htlc_changestate(h, h->state, RCVD_DOWNSTREAM_DEAD);
        }
        else {
            log_broken_struct(lnchn->log, "htlc [%s] has no corresponding source",
                struct htlc, h);
        }
        return;
    }

    do_htlc_update(lnchn, h, yah);

}

void internal_htlc_update_chain(struct LNchannel *lnchn, struct htlc *h) {
    struct LNchannelComm* chainedchn;
    if (!htlc_route_is_chain(h))return;

    chainedchn = lite_comm_channel_from_htlc(lnchn->dstate->channels,
        &h->rhash, htlc_route_has_source(h));

    if (chainedchn) {
        lite_notify_chn_htlc_update(chainedchn, &h->rhash);
        lite_release_comm(lnchn->dstate->channels, chainedchn);
    }
}

bool lnchn_update_htlc(struct LNchannel *lnchn, const struct sha256 *rhash) {
    struct htlc *h;
    const tal_t *tmpctx = tal_tmpctx(lnchn);

    h = htlc_map_get(&lnchn->htlcs, rhash);

    if (!h) {
        log_broken(lnchn->log, "try not update htlc with unexist hash %s",
            tal_hexstr(tmpctx, rhash, sizeof(*rhash)));
        tal_free(tmpctx);
        return false;
    }
    else if (!htlc_route_is_chain(h)) {
        log_broken_struct(lnchn->log, "htlc %s is not in chain",
            struct htlc, h);
        tal_free(tmpctx);
        return false;
    }

    internal_htlc_update(lnchn, h);
    tal_free(tmpctx);
    return true;
}

