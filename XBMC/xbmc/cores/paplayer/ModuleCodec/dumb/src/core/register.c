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
 * register.c - Signal type registration.             / / \  \
 *                                                   | <  /   \_
 * By entheh.                                        |  \/ /\   /
 *                                                    \_  /  > /
 *                                                      | \ / /
 *                                                      |  ' /
 *                                                       \__/
 */

#include <stdlib.h>

#include "dumb.h"
#include "internal/dumb.h"



static DUH_SIGTYPE_DESC_LINK *sigtype_desc = NULL;
static DUH_SIGTYPE_DESC_LINK **sigtype_desc_tail = &sigtype_desc;



/* destroy_sigtypes(): frees all memory allocated while registering signal
 * types. This function is set up to be called by dumb_exit().
 */
static void destroy_sigtypes(void)
{
	DUH_SIGTYPE_DESC_LINK *desc_link = sigtype_desc, *next;
	sigtype_desc = NULL;
	sigtype_desc_tail = &sigtype_desc;

	while (desc_link) {
		next = desc_link->next;
		free(desc_link);
		desc_link = next;
	}
}



/* dumb_register_sigtype(): registers a new signal type with DUMB. The signal
 * type is identified by a four-character string (e.g. "WAVE"), which you can
 * encode using the the DUMB_ID() macro (e.g. DUMB_ID('W','A','V','E')). The
 * signal's behaviour is defined by four functions, whose pointers you pass
 * here. See the documentation for details.
 *
 * If a DUH tries to use a signal that has not been registered using this
 * function, then the library will fail to load the DUH.
 */
void dumb_register_sigtype(DUH_SIGTYPE_DESC *desc)
{
	DUH_SIGTYPE_DESC_LINK *desc_link = sigtype_desc;

	ASSERT((desc->load_sigdata && desc->unload_sigdata) || (!desc->load_sigdata && !desc->unload_sigdata));
	ASSERT((desc->start_sigrenderer && desc->end_sigrenderer) || (!desc->start_sigrenderer && !desc->end_sigrenderer));
	ASSERT(desc->sigrenderer_generate_samples && desc->sigrenderer_get_current_sample);

	if (desc_link) {
		do {
			if (desc_link->desc->type == desc->type) {
				desc_link->desc = desc;
				return;
			}
			desc_link = desc_link->next;
		} while (desc_link);
	} else
		dumb_atexit(&destroy_sigtypes);

	desc_link = *sigtype_desc_tail = malloc(sizeof(DUH_SIGTYPE_DESC_LINK));

	if (!desc_link)
		return;

	desc_link->next = NULL;
	sigtype_desc_tail = &desc_link->next;

	desc_link->desc = desc;
}



/* _dumb_get_sigtype_desc(): searches the registered functions for a signal
 * type matching the parameter. If such a sigtype is found, it returns a
 * pointer to a sigtype descriptor containing the necessary functions to
 * manage the signal. If none is found, it returns NULL.
 */
DUH_SIGTYPE_DESC *_dumb_get_sigtype_desc(long type)
{
	DUH_SIGTYPE_DESC_LINK *desc_link = sigtype_desc;

	while (desc_link && desc_link->desc->type != type)
		desc_link = desc_link->next;

	return desc_link ? desc_link->desc : NULL;
}
