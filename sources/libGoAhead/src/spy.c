/*
 * spy.c -- Spyce Support
 */

/******************************** Description *********************************/

/*
 *	The SPY module processes SPY pages and executes embedded scripts.
 */

/********************************* Includes ***********************************/

#include "wsIntrn.h"

/********************************** Locals ************************************/

int 					(*websSpyOpenCallback)() = NULL;
void 					(*websSpyCloseCallback)() = NULL;
int 					(*websSpyRequestCallback)(webs_t wp, char_t *lpath) = NULL;
/************************************* Code ***********************************/

void setSpyOpenCallback(int (*callback)())
{
	websSpyOpenCallback = callback;
}

void setSpyCloseCallback(void (*callback)())
{
	websSpyCloseCallback = callback;
}

void setSpyRequestCallback(int (*callback)(webs_t wp, char_t *lpath))
{
	websSpyRequestCallback = callback;
}

int websSpyOpen()
{
	if (websSpyOpenCallback == NULL) return 0;
	return websSpyOpenCallback();
}

/************************************* Code ***********************************/
/*
 *	Close Asp symbol table.
 */

void websSpyClose()
{
	if (websSpyCloseCallback != NULL)
		websSpyCloseCallback();
}

/******************************************************************************/
/*
 *	Process SPY requests and expand all scripting commands. We read the
 *	entire SPY page into memory and then process.
 */

int websSpyRequest(webs_t wp, char_t *lpath)
{
	if (websSpyRequestCallback == NULL) return 0;
	return websSpyRequestCallback(wp, lpath);
}

/******************************************************************************/