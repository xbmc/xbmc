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
 * duhtag.c - Function to return the tags stored      / / \  \
 *            in a DUH struct (typically author      | <  /   \_
 *            information).                          |  \/ /\   /
 *                                                    \_  /  > /
 * By entheh.                                           | \ / /
 *                                                      |  ' /
 *                                                       \__/
 */

#include <string.h>

#include "dumb.h"
#include "internal/dumb.h"



const char *duh_get_tag(DUH *duh, const char *key)
{
	int i;
	ASSERT(key);
	if (!duh || !duh->tag) return NULL;

	for (i = 0; i < duh->n_tags; i++)
		if (strcmp(key, duh->tag[i][0]) == 0)
			return duh->tag[i][1];

	return NULL;
}
