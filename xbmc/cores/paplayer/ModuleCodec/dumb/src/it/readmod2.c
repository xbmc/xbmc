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
 * readmod2.c - Function to read a good old-          / / \  \
 *              fashioned Amiga module from an       | <  /   \_
 *              open file and do an initial          |  \/ /\   /
 *              run-through.                          \_  /  > /
 *                                                      | \ / /
 * Split off from readmod.c by entheh.                  |  ' /
 *                                                       \__/
 */

#include "dumb.h"



DUH *dumb_read_mod(DUMBFILE *f)
{
	DUH *duh = dumb_read_mod_quick(f);
	dumb_it_do_initial_runthrough(duh);
	return duh;
}
