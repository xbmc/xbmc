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
 * itload.c - Code to read an Impulse Tracker         / / \  \
 *            file, opening and closing it for       | <  /   \_
 *            you.                                   |  \/ /\   /
 *                                                    \_  /  > /
 * By entheh. Don't worry Bob, you're credited          | \ / /
 * in itread.c!                                         |  ' /
 *                                                       \__/
 */

#include "dumb.h"
#include "internal/it.h"



/* dumb_load_it_quick(): loads an IT file into a DUH struct, returning a
 * pointer to the DUH struct. When you have finished with it, you must pass
 * the pointer to unload_duh() so that the memory can be freed.
 */
DUH *dumb_load_it_quick(const char *filename)
{
	DUH *duh;
	DUMBFILE *f = dumbfile_open(filename);

	if (!f)
		return NULL;

	duh = dumb_read_it_quick(f);

	dumbfile_close(f);

	return duh;
}
