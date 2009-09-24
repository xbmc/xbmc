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
 * atexit.c - Library Clean-up Management.            / / \  \
 *                                                   | <  /   \_
 * By entheh.                                        |  \/ /\   /
 *                                                    \_  /  > /
 *                                                      | \ / /
 *                                                      |  ' /
 *                                                       \__/
 */

#include <stdlib.h>

#include "dumb.h"
#include "internal/dumb.h"



typedef struct DUMB_ATEXIT_PROC
{
	struct DUMB_ATEXIT_PROC *next;
	void (*proc)(void);
}
DUMB_ATEXIT_PROC;



static DUMB_ATEXIT_PROC *dumb_atexit_proc = NULL;



int dumb_atexit(void (*proc)(void))
{
	DUMB_ATEXIT_PROC *dap = dumb_atexit_proc;

	while (dap) {
		if (dap->proc == proc) return 0;
		dap = dap->next;
	}

	dap = malloc(sizeof(*dap));

	if (!dap)
		return -1;

	dap->next = dumb_atexit_proc;
	dap->proc = proc;
	dumb_atexit_proc = dap;

	return 0;
}



void dumb_exit(void)
{
	while (dumb_atexit_proc) {
		DUMB_ATEXIT_PROC *next = dumb_atexit_proc->next;
		(*dumb_atexit_proc->proc)();
		free(dumb_atexit_proc);
		dumb_atexit_proc = next;
	}
}
