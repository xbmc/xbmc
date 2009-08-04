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
 * loadmod2.c - Function to read a good old-          / / \  \
 *              fashioned Amiga module file,         | <  /   \_
 *              opening and closing it for you,      |  \/ /\   /
 *              and do an initial run-through.        \_  /  > /
 *                                                      | \ / /
 * Split off from loadmod.c by entheh.                  |  ' /
 *                                                       \__/
 */

#include "dumb.h"



DUH *dumb_load_mod(const char *filename)
{
	DUH *duh = dumb_load_mod_quick(filename);
	dumb_it_do_initial_runthrough(duh);
	return duh;
}
