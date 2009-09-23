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
 * loadmod.c - Code to read a good old-fashioned      / / \  \
 *             Amiga module file, opening and        | <  /   \_
 *             closing it for you.                   |  \/ /\   /
 *                                                    \_  /  > /
 * By entheh.                                           | \ / /
 *                                                      |  ' /
 *                                                       \__/
 */

#include "dumb.h"
#include "internal/it.h"



/* dumb_load_mod_quick(): loads a MOD file into a DUH struct, returning a
 * pointer to the DUH struct. When you have finished with it, you must
 * pass the pointer to unload_duh() so that the memory can be freed.
 */
DUH *dumb_load_mod_quick(const char *filename)
{
	DUH *duh;
	DUMBFILE *f = dumbfile_open(filename);

	if (!f)
		return NULL;

	duh = dumb_read_mod_quick(f);

	dumbfile_close(f);

	return duh;
}
