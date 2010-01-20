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
 * rawsig.c - Function to retrieve raw signal         / / \  \
 *            data from a DUH provided you know      | <  /   \_
 *            what type of signal it is.             |  \/ /\   /
 *                                                    \_  /  > /
 * By entheh.                                           | \ / /
 *                                                      |  ' /
 *                                                       \__/
 */

#include <stdlib.h>

#include "dumb.h"
#include "internal/dumb.h"



/* You have to specify the type of sigdata, proving you know what to do with
 * the pointer. If you get it wrong, you can expect NULL back.
 */
sigdata_t *duh_get_raw_sigdata(DUH *duh, int sig, long type)
{
	DUH_SIGNAL *signal;

	if (!duh) return NULL;

	if ((unsigned int)sig >= (unsigned int)duh->n_signals) return NULL;

	signal = duh->signal[sig];

	if (signal && signal->desc->type == type)
		return signal->sigdata;

	return NULL;
}
