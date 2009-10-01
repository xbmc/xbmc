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
 * unload.c - Code to free a DUH from memory.         / / \  \
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



static void destroy_signal(DUH_SIGNAL *signal)
{
	if (signal) {
		if (signal->desc)
			if (signal->desc->unload_sigdata)
				if (signal->sigdata)
					(*signal->desc->unload_sigdata)(signal->sigdata);

		free(signal);
	}
}



/* unload_duh(): destroys a DUH struct. You must call this for every DUH
 * struct created, when you've finished with it.
 */
void unload_duh(DUH *duh)
{
	int i;

	if (duh) {
		if (duh->signal) {
			for (i = 0; i < duh->n_signals; i++)
				destroy_signal(duh->signal[i]);

			free(duh->signal);
		}

		if (duh->tag) {
			if (duh->tag[0][0])
				free(duh->tag[0][0]);
			free(duh->tag);
		}

		free(duh);
	}
}
