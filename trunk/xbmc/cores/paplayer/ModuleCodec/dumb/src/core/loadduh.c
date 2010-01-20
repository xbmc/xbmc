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
 * loadduh.c - Code to read a DUH from a file,        / / \  \
 *             opening and closing the file for      | <  /   \_
 *             you.                                  |  \/ /\   /
 *                                                    \_  /  > /
 * By entheh.                                           | \ / /
 *                                                      |  ' /
 *                                                       \__/
 */

#include "dumb.h"
#include "internal/dumb.h"



/* load_duh(): loads a .duh file, returning a pointer to a DUH struct.
 * When you have finished with it, you must pass the pointer to unload_duh()
 * so that the memory can be freed.
 */
DUH *load_duh(const char *filename)
{
	DUH *duh;
	DUMBFILE *f = dumbfile_open(filename);

	if (!f)
		return NULL;

	duh = read_duh(f);

	dumbfile_close(f);

	return duh;
}
