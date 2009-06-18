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
 * internal/dumb.h - DUMB's internal declarations.    / / \  \
 *                                                   | <  /   \_
 * This header file provides access to the           |  \/ /\   /
 * internal structure of DUMB, and is liable          \_  /  > /
 * to change, mutate or cease to exist at any           | \ / /
 * moment. Include it at your own peril.                |  ' /
 *                                                       \__/
 * ...
 *
 * Seriously. You don't need access to anything in this file. All right, you
 * probably do actually. But if you use it, you will be relying on a specific
 * version of DUMB, so please check DUMB_VERSION defined in dumb.h. Please
 * contact the authors so that we can provide a public API for what you need.
 */

#ifndef INTERNAL_DUMB_H
#define INTERNAL_DUMB_H


typedef struct DUH_SIGTYPE_DESC_LINK
{
	struct DUH_SIGTYPE_DESC_LINK *next;
	DUH_SIGTYPE_DESC *desc;
}
DUH_SIGTYPE_DESC_LINK;


typedef struct DUH_SIGNAL
{
	sigdata_t *sigdata;
	DUH_SIGTYPE_DESC *desc;
}
DUH_SIGNAL;


struct DUH
{
	long length;

	int n_tags;
	char *(*tag)[2];

	int n_signals;
	DUH_SIGNAL **signal;
};


DUH_SIGTYPE_DESC *_dumb_get_sigtype_desc(long type);


#endif /* INTERNAL_DUMB_H */
