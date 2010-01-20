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
 * itorder.c - Code to fix invalid patterns in        / / \  \
 *             the pattern table.                    | <  /   \_
 *                                                   |  \/ /\   /
 * By Julien Cugniere.                                \_  /  > /
 *                                                      | \ / /
 *                                                      |  ' /
 *                                                       \__/
 */



#include <stdlib.h>

#include "dumb.h"
#include "internal/it.h"



/* This function ensures that any pattern mentioned in the order table but
 * not present in the pattern table is treated as an empty 64 rows pattern.
 * This is done by adding such a dummy pattern at the end of the pattern
 * table, and redirect invalid orders to it.
 * Patterns 254 and 255 are left untouched, unless the signal is an XM.
 */
int _dumb_it_fix_invalid_orders(DUMB_IT_SIGDATA *sigdata)
{
	int i;
	int found_some = 0;

	int first_invalid = sigdata->n_patterns;
	int last_invalid = (sigdata->flags & IT_WAS_AN_XM) ? 255 : 253;

	for (i = 0; i < sigdata->n_orders; i++) {
		if (sigdata->order[i] >= first_invalid && sigdata->order[i] <= last_invalid) {
			sigdata->order[i] = sigdata->n_patterns;
			found_some = 1;
		}
	}

	if (found_some) {
		IT_PATTERN *new_pattern = realloc(sigdata->pattern, sizeof(*sigdata->pattern) * (sigdata->n_patterns + 1));
		if (!new_pattern)
			return -1;
		
		new_pattern[sigdata->n_patterns].n_rows = 64;
		new_pattern[sigdata->n_patterns].n_entries = 0;
		new_pattern[sigdata->n_patterns].entry = NULL;
		sigdata->pattern = new_pattern;
		sigdata->n_patterns++;
	}

	return 0;
}
