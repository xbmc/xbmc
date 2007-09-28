/*
 * rom.c -- Support for ROMed page retrieval.
 *
 * Copyright (c) GoAhead Software Inc., 1995-2000. All Rights Reserved.
 *
 * See the file "license.txt" for usage and redistribution license requirements
 *
 * $Id: rom.c,v 1.3 2002/10/24 14:44:50 bporter Exp $
 */

/******************************** Description *********************************/

/*
 *	This module provides web page retrieval from compiled web pages. Use the
 *	webcomp program to compile web pages and link into the GoAhead WebServer.
 *	This module uses a hashed symbol table for fast page lookup.
 *
 *	Usage: webcomp -f webPageFileList -p Prefix >webrom.c
 */

/********************************* Includes ***********************************/

#include	<stdlib.h>

#include	"wsIntrn.h"

/******************************** Local Data **********************************/

#ifdef WEBS_PAGE_ROM

sym_fd_t	romTab;						/* Symbol table for web pages */

/*********************************** Code *************************************/
/*
 *	Open the ROM module
 */

int websRomOpen()
{
	websRomPageIndexType	*wip;
	int						nchars;
	char_t					name[SYM_MAX];

	romTab = symOpen(WEBS_SYM_INIT);

	for (wip = websRomPageIndex; wip->path; wip++) {
		gstrncpy(name, wip->path, SYM_MAX);
		nchars = gstrlen(name) - 1;
		if (nchars > 0 &&
			(name[nchars] == '/' || name[nchars] == '\\')) {
			name[nchars] = '\0';
		}
		symEnter(romTab, name, valueInteger((int) wip), 0);
	}
	return 0;
}

/******************************************************************************/
/*
 *	Close the ROM module
 */

void websRomClose()
{
	symClose(romTab);
}

/******************************************************************************/
/*
 *	Open a web page
 */

int websRomPageOpen(webs_t wp, char_t *path, int mode, int perm)
{
	websRomPageIndexType	*wip;
	sym_t					*sp;

	a_assert(websValid(wp));
	a_assert(path && *path);

	if ((sp = symLookup(romTab, path)) == NULL) {
		return -1;
	}
	wip = (websRomPageIndexType*) sp->content.value.integer;
	wip->pos = 0;
	return (wp->docfd = wip - websRomPageIndex);
}

/******************************************************************************/
/*
 *	Close a web page
 */

void websRomPageClose(int fd)
{
}

/******************************************************************************/
/*
 *	Stat a web page
 */

int websRomPageStat(char_t *path, websStatType *sbuf)
{
	websRomPageIndexType	*wip;
	sym_t					*sp;

	a_assert(path && *path);

	if ((sp = symLookup(romTab, path)) == NULL) {
		return -1;
	}
	wip = (websRomPageIndexType*) sp->content.value.integer;

	memset(sbuf, 0, sizeof(websStatType));
	sbuf->size = wip->size;
	if (wip->page == NULL) {
		sbuf->isDir = 1;
	}
	return 0;
}

/******************************************************************************/
/*
 *	Read a web page
 */

int websRomPageReadData(webs_t wp, char *buf, int nBytes)
{
	websRomPageIndexType	*wip;
	int						len;

	a_assert(websValid(wp));
	a_assert(buf);
	a_assert(wp->docfd >= 0);

	wip = &websRomPageIndex[wp->docfd];

	len = min(wip->size - wip->pos, nBytes);
	memcpy(buf, &wip->page[wip->pos], len);
	wip->pos += len;
	return len;
}

/******************************************************************************/
/*
 *	Position a web page
 */

long websRomPageSeek(webs_t wp, long offset, int origin)
{
	websRomPageIndexType	*wip;
	long pos;

	a_assert(websValid(wp));
	a_assert(origin == SEEK_SET || origin == SEEK_CUR || origin == SEEK_END);
	a_assert(wp->docfd >= 0);

	wip = &websRomPageIndex[wp->docfd];

	if (origin != SEEK_SET && origin != SEEK_CUR && origin != SEEK_END) {
		errno = EINVAL;
		return -1;
	}

	if (wp->docfd < 0) {
		errno = EBADF;
		return -1;
	}

	pos = offset;
	switch (origin) {
	case SEEK_CUR:
		pos = wip->pos + offset;
		break;
	case SEEK_END:
		pos = wip->size + offset;
		break;
	default:
		break;
	}

	if (pos < 0) {
		errno = EBADF;
		return -1;
	}

	return (wip->pos = pos);
}

#endif /* WEBS_PAGE_ROM */

/******************************************************************************/

