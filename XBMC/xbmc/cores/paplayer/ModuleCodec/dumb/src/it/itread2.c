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
 * itread2.c - Function to read an Impulse Tracker    / / \  \
 *             module from an open file and do an    | <  /   \_
 *             initial run-through.                  |  \/ /\   /
 *                                                    \_  /  > /
 * Split off from itread.c by entheh.                   | \ / /
 *                                                      |  ' /
 *                                                       \__/
 */

#include "dumb.h"



DUH *dumb_read_it(DUMBFILE *f)
{
	DUH *duh = dumb_read_it_quick(f);
	dumb_it_do_initial_runthrough(duh);
	return duh;
}
