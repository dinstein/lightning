#ifndef LIGHTNING_BTCNETWORK_OUTSOURCING_C_INTERFACE_H
#define LIGHTNING_BTCNETWORK_OUTSOURCING_C_INTERFACE_H
#include "config.h"
#include "bitcoin/shadouble.h"
#include "bitcoin/signature.h"
#include <ccan/short_types/short_types.h>

struct bitcoin_tx;
struct outsourcing;
struct LNchannel;

enum outsourcing_result {
	OUTSOURCING_OK,
    OUTSOURCING_DENY = -1,    //server unavailiable
    OUTSOURCING_INVALID = -2, //invalid transaction
    OUTSOURCING_FAIL = -3,    //valid transaction but stale
};

/* outsourcing an output for htlc from committx*/
struct txowatch {
    struct sha256_double *commitid;
    u8 *outscript;
};

struct txdeliver {
    struct bitcoin_tx *deliver_tx;

    /* data to build witness */
    u8 *wscript;

    /* lock-time must be clear to apply this signature*/
    ecdsa_signature sig_nolocked;

    /* sign with lock-time, no needed for "their" HTLC */
    ecdsa_signature* sig;
};

struct lnwatch_htlc_task {
    struct sha256 rhash;

    struct txdeliver* txdeliver;    

    /* additional trigger */
    struct txowatch*  txowatch;    
};

struct lnwatch_task {

    /* txid which may be broadcasted from other side*/
    struct sha256_double *commitid;

    /* if NULL, can be expried by later call*/
    struct sha256* preimage;

    /* if delivered by us, required additional tx to redeem the delayed part*/
    /* if delivered by theirs and is up-to-date, no action is needed except the htlc part */
    struct txdeliver *redeem_tx;

    /* htlcs we sent and should try to redeem if it is expired*/
    struct lnwatch_htlc_task* htlctxs;
};

/* a verify-only task, finished if corresponding txid reach expected depth*/
struct lnwatch_verifytask {
    struct sha256_double *txid;
    unsigned int depth;
};

/* create or sync outsourcing task */
void outsourcing_initchannel(struct outsourcing* svr, const struct LNchannel* lnchn);

/* clear corresponding task */
void outsourcing_clear(struct outsourcing* svr, const struct LNchannel* lnchn);

/* 
   task with same commitid will be updated (e.g. switch a commit from up-to-date to expired) 
   any member is not NULL will be replaced while NULL members is just omited (not deleted)
*/
void outsourcing_tasks(struct outsourcing* svr, const struct LNchannel* lnchn, 
    const struct lnwatch_task *tasks, //array of tasks
    void(*notify)(const struct LNchannel* lnchn, enum outsourcing_result, void *cbdata),
    void *cbdata
    );

void outsourcing_verifytask(struct outsourcing* svr, const struct LNchannel* lnchn, 
    const struct lnwatch_verifytask *tasks, 
    void(*notify)(const struct LNchannel* lnchn, enum outsourcing_result, void *cbdata),
    void *cbdata
    );

//TODO: a batch update for all outsourcing task should be added 
//TODO: the old fashion watch can be added 


#endif /* LIGHTNING_BTCNETWORK_OUTSOURCING_C_INTERFACE_H */
