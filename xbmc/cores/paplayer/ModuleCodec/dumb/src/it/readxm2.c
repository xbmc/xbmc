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
 * readxm2.c - Function to read a Fast Tracker II     / / \  \
 *             module from an open file and do an    | <  /   \_
 *             initial run-through.                  |  \/ /\   /
 *                                                    \_  /  > /
 * Split off from readxm.c by entheh.                   | \ / /
 *                                                      |  ' /
 *                                                       \__/
 */

#include "dumb.h"



DUH *dumb_read_xm(DUMBFILE *f)
{
	DUH *duh = dumb_read_xm_quick(f);
	dumb_it_do_initial_runthrough(duh);
	return duh;
}
