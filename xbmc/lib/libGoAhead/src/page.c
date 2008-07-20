/*
 * Page.c -- Support for page retrieval.
 *
 * Copyright (c) GoAhead Software Inc., 1995-2000. All Rights Reserved.
 *
 * See the file "license.txt" for usage and redistribution license requirements
 *
 * $Id: page.c,v 1.3 2002/10/24 14:44:50 bporter Exp $
 */

/******************************** Description *********************************/

/*
 *	This module provides page retrieval handling. It provides support for
 *	reading web pages from file systems and has expansion for ROMed web
 *	pages.
 */

/********************************* Includes ***********************************/

#include	"wsIntrn.h"

/*********************************** Code *************************************/
/*
 *	Open a web page. lpath is the local filename. path is the URL path name.
 */

int websPageOpen(webs_t wp, char_t *lpath, char_t *path, int mode, int perm)
{
	a_assert(websValid(wp));

#ifdef WEBS_PAGE_ROM
	return websRomPageOpen(wp, path, mode, perm);
#else
	return (wp->docfd = gopen(lpath, mode, perm));
#endif /* WEBS_PAGE_ROM */
}

/******************************************************************************/
/*
 *	Close a web page
 */

void websPageClose(webs_t wp)
{
	a_assert(websValid(wp));

#ifdef WEBS_PAGE_ROM
	websRomPageClose(wp->docfd);
#else
	if (wp->docfd >= 0) {
		close(wp->docfd);
		wp->docfd = -1;
	}
#endif
}

/******************************************************************************/
/*
 *	Stat a web page lpath is the local filename. path is the URL path name.
 */

int websPageStat(webs_t wp, char_t *lpath, char_t *path, websStatType* sbuf)
{
#ifdef WEBS_PAGE_ROM
	return websRomPageStat(path, sbuf);
#else
	gstat_t	s;

	if (gstat(lpath, &s) < 0) {
		return -1;
	}
	sbuf->size = s.st_size;
	sbuf->mtime = s.st_mtime;
	sbuf->isDir = s.st_mode & S_IFDIR;
	return 0;
#endif
}

/******************************************************************************/
/*
 *	Is this file a directory?
 */

int websPageIsDirectory(char_t *lpath)
{
#ifdef WEBS_PAGE_ROM
	websStatType	sbuf;

	if (websRomPageStat(lpath, &sbuf) >= 0) {
		return(sbuf.isDir);
	} else {
		return 0;
	}
#else
	gstat_t sbuf;

	if (gstat(lpath, &sbuf) >= 0) {
		return(sbuf.st_mode & S_IFDIR);
	} else {
		return 0;
	}
#endif
}


/******************************************************************************/
/*
 *	Read a web page. Returns the number of _bytes_ read.
 *	len is the size of buf, in bytes.
 */

int websPageReadData(webs_t wp, char *buf, int nBytes)
{

#ifdef WEBS_PAGE_ROM
	a_assert(websValid(wp));
	return websRomPageReadData(wp, buf, nBytes);
#else
	a_assert(websValid(wp));
	return read(wp->docfd, buf, nBytes);
#endif
}

/******************************************************************************/
/*
 *	Move file pointer offset bytes.
 */

void websPageSeek(webs_t wp, long offset)
{
	a_assert(websValid(wp));

#ifdef WEBS_PAGE_ROM
	websRomPageSeek(wp, offset, SEEK_CUR);
#else
	lseek(wp->docfd, offset, SEEK_CUR);
#endif
}

/******************************************************************************/


