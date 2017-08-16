#ifndef LIGHTNING_LITE_C_INTERFACE_MANAGER_H
#define LIGHTNING_LITE_C_INTERFACE_MANAGER_H

struct LNchannels;
struct LNchannel;
struct htlc;
struct pubkey;

void    lite_reg_channel(struct LNchannels *mgr, const struct LNchannel *lnchn);
void    lite_reg_htlc(struct LNchannels *mgr, const struct LNchannel *lnchn, const struct htlc *htlc);
void    lite_update_htlc_state(struct LNchannels *mgr, const struct htlc *htlc);
/* return a copy of updated htlc, but some field (e.g. upstream_watch) is always NULL*/
const struct htlc *lite_query_htlc_state(struct LNchannels *mgr, const struct sha256* hash, int upstream);

struct LNchannel* lite_query_channel(struct LNchannels *mgr, struct pubkey *id);
/* the fast entry to query channel including the sourcing htlc, need to call release */
struct LNchannel* lite_query_htlc_src(struct LNchannels *mgr, const struct sha256* hash);
void    lite_release_query_chn(struct LNchannels *mgr, const struct LNchannel* chn);


#endif
