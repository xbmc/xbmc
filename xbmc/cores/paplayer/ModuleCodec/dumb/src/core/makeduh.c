/*  _______         ____    __         ___    ___
 * \    _  \       \    /  \  /       \   \  /   /       '   '  '
 *  |  | \  \       |  |    ||         |   \/   |         .      .
 *  |  |  |  |      |  |    ||         ||\  /|  |
 *  |  |  |  |      |  |    ||         || \/ |  |         '  '  '
 *  |  |  |  |      |  |    ||         ||    |  |         .      .
 *  |  |_/  /        \  \__//          ||    |  |
 * /_______/ynamic    \____/niversal  /__\  /____\usic   /|  .  . ibliotheque
 *                                                      /  \
 *                                                     / .  \
 * makeduh.c - Function to construct a DUH from       / / \  \
 *             its components.                       | <  /   \_
 *                                                   |  \/ /\   /
 * By entheh.                                         \_  /  > /
 *                                                      | \ / /
 *                                                      |  ' /
 *                                                       \__/
 */

#include <stdlib.h>
#include <string.h>

#include "dumb.h"
#include "internal/dumb.h"



static DUH_SIGNAL *make_signal(DUH_SIGTYPE_DESC *desc, sigdata_t *sigdata)
{
	DUH_SIGNAL *signal;

	ASSERT((desc->start_sigrenderer && desc->end_sigrenderer) || (!desc->start_sigrenderer && !desc->end_sigrenderer));
	ASSERT(desc->sigrenderer_generate_samples && desc->sigrenderer_get_current_sample);

	signal = malloc(sizeof(*signal));

	if (!signal) {
		if (desc->unload_sigdata)
			if (sigdata)
				(*desc->unload_sigdata)(sigdata);
		return NULL;
	}

	signal->desc = desc;
	signal->sigdata = sigdata;

	return signal;
}



DUH *make_duh(
	long length,
	int n_tags,
	const char *const tags[][2],
	int n_signals,
	DUH_SIGTYPE_DESC *desc[],
	sigdata_t *sigdata[]
)
{
	DUH *duh = malloc(sizeof(*duh));
	int i;
	int fail;

	if (duh) {
		duh->n_signals = n_signals;

		duh->signal = malloc(n_signals * sizeof(*duh->signal));

		if (!duh->signal) {
			free(duh);
			duh = NULL;
		}
	}

	if (!duh) {
		for (i = 0; i < n_signals; i++)
			if (desc[i]->unload_sigdata)
				if (sigdata[i])
					(*desc[i]->unload_sigdata)(sigdata[i]);
		return NULL;
	}

	duh->n_tags = 0;
	duh->tag = NULL;

	fail = 0;

	for (i = 0; i < n_signals; i++) {
		duh->signal[i] = make_signal(desc[i], sigdata[i]);
		if (!duh->signal[i])
			fail = 1;
	}

	if (fail) {
		unload_duh(duh);
		return NULL;
	}

	duh->length = length;

	{
		int mem = n_tags * 2; /* account for NUL terminators here */
		char *ptr;

		for (i = 0; i < n_tags; i++)
			mem += strlen(tags[i][0]) + strlen(tags[i][1]);

		if (mem <= 0) return duh;

		duh->tag = malloc(n_tags * sizeof(*duh->tag));
		if (!duh->tag) return duh;
		duh->tag[0][0] = malloc(mem);
		if (!duh->tag[0][0]) {
			free(duh->tag);
			duh->tag = NULL;
			return duh;
		}
		duh->n_tags = n_tags;
		ptr = duh->tag[0][0];
		for (i = 0; i < n_tags; i++) {
			duh->tag[i][0] = ptr;
			strcpy(ptr, tags[i][0]);
			ptr += strlen(tags[i][0]) + 1;
			duh->tag[i][1] = ptr;
			strcpy(ptr, tags[i][1]);
			ptr += strlen(tags[i][1]) + 1;
		}
	}

	return duh;
}
