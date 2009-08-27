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
 * duhlen.c - Functions to set and return the         / / \  \
 *            length of a DUH.                       | <  /   \_
 *                                                   |  \/ /\   /
 * By entheh.                                         \_  /  > /
 *                                                      | \ / /
 * Note that the length of a DUH is a constant          |  ' /
 * stored in the DUH struct and in the DUH disk          \__/
 * format. It will be calculated on loading for
 * other formats in which the length is not explicitly stored. Also note that
 * it does not necessarily correspond to the length of time for which the DUH
 * will generate samples. Rather it represents a suitable point for a player
 * such as Winamp to stop, and in any good DUH it will allow for any final
 * flourish to fade out and be appreciated.
 */

#include "dumb.h"
#include "internal/dumb.h"



long duh_get_length(DUH *duh)
{
	return duh ? duh->length : 0;
}



void duh_set_length(DUH *duh, long length)
{
	if (duh)
		duh->length = length;
}
