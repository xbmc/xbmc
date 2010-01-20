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
 * reads3m2.c - Function to read a ScreamTracker 3    / / \  \
 *              module from an open file and do an   | <  /   \_
 *              initial run-through.                 |  \/ /\   /
 *                                                    \_  /  > /
 * Split off from reads3m.c by entheh.                  | \ / /
 *                                                      |  ' /
 *                                                       \__/
 */

#include "dumb.h"



DUH *dumb_read_s3m(DUMBFILE *f)
{
	DUH *duh = dumb_read_s3m_quick(f);
	dumb_it_do_initial_runthrough(duh);
	return duh;
}
