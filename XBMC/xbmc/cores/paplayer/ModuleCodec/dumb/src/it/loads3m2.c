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
 * loads3m2.c - Function to read a ScreamTracker 3    / / \  \
 *              file, opening and closing it for     | <  /   \_
 *              you, and do an initial run-through.  |  \/ /\   /
 *                                                    \_  /  > /
 * Split off from loads3m.c by entheh.                  | \ / /
 *                                                      |  ' /
 *                                                       \__/
 */

#include "dumb.h"



DUH *dumb_load_s3m(const char *filename)
{
	DUH *duh = dumb_load_s3m_quick(filename);
	dumb_it_do_initial_runthrough(duh);
	return duh;
}
