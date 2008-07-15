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
 * itload2.c - Function to read an Impulse Tracker    / / \  \
 *             file, opening and closing it for      | <  /   \_
 *             you, and do an initial run-through.   |  \/ /\   /
 *                                                    \_  /  > /
 * Split off from itload.c by entheh.                   | \ / /
 *                                                      |  ' /
 *                                                       \__/
 */

#include "dumb.h"



DUH *dumb_load_it(const char *filename)
{
	DUH *duh = dumb_load_it_quick(filename);
	dumb_it_do_initial_runthrough(duh);
	return duh;
}
