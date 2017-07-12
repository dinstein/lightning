#include "names.h"
#include <ccan/str/str.h>
/* Indented for 'check-source' because it has to be included after names.h */

struct {
	enum state v;
	const char *name;
} enum_state_names[] = {
	{ STATE_INIT, "STATE_INIT" },
	{ STATE_OPEN_WAIT_FOR_OPENPKT, "STATE_OPEN_WAIT_FOR_OPENPKT" },
	{ STATE_OPEN_WAIT_FOR_ANCHORPKT, "STATE_OPEN_WAIT_FOR_ANCHORPKT" },
	{ STATE_OPEN_WAIT_FOR_COMMIT_SIGPKT, "STATE_OPEN_WAIT_FOR_COMMIT_SIGPKT" },
	{ STATE_OPEN_WAIT_ANCHORDEPTH_AND_THEIRCOMPLETE, "STATE_OPEN_WAIT_ANCHORDEPTH_AND_THEIRCOMPLETE" },
	{ STATE_OPEN_WAIT_ANCHORDEPTH, "STATE_OPEN_WAIT_ANCHORDEPTH" },
	{ STATE_OPEN_WAIT_THEIRCOMPLETE, "STATE_OPEN_WAIT_THEIRCOMPLETE" },
	{ STATE_NORMAL, "STATE_NORMAL" },
	{ STATE_NORMAL_COMMITTING, "STATE_NORMAL_COMMITTING" },
	{ STATE_SHUTDOWN, "STATE_SHUTDOWN" },
	{ STATE_SHUTDOWN_COMMITTING, "STATE_SHUTDOWN_COMMITTING" },
	{ STATE_MUTUAL_CLOSING, "STATE_MUTUAL_CLOSING" },
	{ STATE_CLOSE_ONCHAIN_CHEATED, "STATE_CLOSE_ONCHAIN_CHEATED" },
	{ STATE_CLOSE_ONCHAIN_THEIR_UNILATERAL, "STATE_CLOSE_ONCHAIN_THEIR_UNILATERAL" },
	{ STATE_CLOSE_ONCHAIN_OUR_UNILATERAL, "STATE_CLOSE_ONCHAIN_OUR_UNILATERAL" },
	{ STATE_CLOSE_ONCHAIN_MUTUAL, "STATE_CLOSE_ONCHAIN_MUTUAL" },
	{ STATE_CLOSED, "STATE_CLOSED" },
	{ STATE_ERR_BREAKDOWN, "STATE_ERR_BREAKDOWN" },
	{ STATE_ERR_ANCHOR_TIMEOUT, "STATE_ERR_ANCHOR_TIMEOUT" },
	{ STATE_ERR_INFORMATION_LEAK, "STATE_ERR_INFORMATION_LEAK" },
	{ STATE_ERR_INTERNAL, "STATE_ERR_INTERNAL" },
	{ STATE_MAX, "STATE_MAX" },
	{ 0, NULL } };


const char *state_name(enum state s)
{
	size_t i;

	for (i = 0; enum_state_names[i].name; i++)
		if (enum_state_names[i].v == s)
			return enum_state_names[i].name;
	return "unknown";
}

enum state name_to_state(const char *name)
{
	size_t i;

	for (i = 0; enum_state_names[i].name; i++)
		if (streq(name, enum_state_names[i].name))
			return enum_state_names[i].v;

	return STATE_MAX;
}

