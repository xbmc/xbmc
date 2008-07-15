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
 * loadxm2.c - Function to read a Fast Tracker II     / / \  \
 *             file, opening and closing it for      | <  /   \_
 *             you, and do an initial run-through.   |  \/ /\   /
 *                                                    \_  /  > /
 * Split off from loadxm.c by entheh.                   | \ / /
 *                                                      |  ' /
 *                                                       \__/
 */

#include "dumb.h"



DUH *dumb_load_xm(const char *filename)
{
	DUH *duh = dumb_load_xm_quick(filename);
	dumb_it_do_initial_runthrough(duh);
	return duh;
}
