///
///	@file 	dirHandler.cpp
/// @brief 	Dirorization handler for basic and digest direntication.
//
/////////////////////////////////// Copyright //////////////////////////////////
//
//	@copy	default
//	
//	Copyright (c) Mbedthis Software LLC, 2003-2007. All Rights Reserved.
//	
//	This software is distributed under commercial and open source licenses.
//	You may use the GPL open source license described below or you may acquire 
//	a commercial license from Mbedthis Software. You agree to be fully bound 
//	by the terms of either license. Consult the LICENSE.TXT distributed with 
//	this software for full details.
//	
//	This software is open source; you can redistribute it and/or modify it 
//	under the terms of the GNU General Public License as published by the 
//	Free Software Foundation; either version 2 of the License, or (at your 
//	option) any later version. See the GNU General Public License for more 
//	details at: http://www.mbedthis.com/downloads/gplLicense.html
//	
//	This program is distributed WITHOUT ANY WARRANTY; without even the 
//	implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
//	
//	This GPL license does NOT permit incorporating this software into 
//	proprietary programs. If you are unable to comply with the GPL, you must
//	acquire a commercial license to use this software. Commercial licenses 
//	for this software and support services are available from Mbedthis 
//	Software at http://www.mbedthis.com 
//	
//	@end
////////////////////////////////// Includes ////////////////////////////////////

#include	"dirHandler.h"
#include	"http.h"

////////////////////////////////////////////////////////////////////////////////
#if BLD_FEATURE_DIR_MODULE

#define DIR_INCR	512

static int getDirList(MaRequest *rq, MaDirEntry **list, int *count, 
	const char *path, bool enumDirs);
static int match(char *pattern, char *file);

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////// DirModule //////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

int mprDirInit(void *handle)
{
	if (maGetHttp() == 0) {
		return MPR_ERR_NOT_INITIALIZED;
	}
	(void) new MaDirModule(handle);
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

MaDirModule::MaDirModule(void *handle) : MaModule("dir", handle)
{
	dirHandlerService = new MaDirHandlerService();
}

////////////////////////////////////////////////////////////////////////////////

MaDirModule::~MaDirModule()
{
	delete dirHandlerService;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////// MaDirHandlerService ////////////////////////////
////////////////////////////////////////////////////////////////////////////////

MaDirHandlerService::MaDirHandlerService() : MaHandlerService("dirHandler")
{
#if BLD_FEATURE_LOG
	log = new MprLogModule("dir");
#endif
}

////////////////////////////////////////////////////////////////////////////////

MaDirHandlerService::~MaDirHandlerService()
{
#if BLD_FEATURE_LOG
	delete log;
#endif
}

////////////////////////////////////////////////////////////////////////////////

MaHandler *MaDirHandlerService::newHandler(MaServer *server, MaHost *host, 
	char *ext)
{
	return new MaDirHandler();
}

////////////////////////////////////////////////////////////////////////////////
///////////////////////////////// MaDirHandler ////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

MaDirHandler::MaDirHandler() : MaHandler("dirHandler", 0, 
	MPR_HANDLER_GET | MPR_HANDLER_HEAD | MPR_HANDLER_TERMINAL |
	MPR_HANDLER_MAP_VIRTUAL | MPR_HANDLER_NEED_FILENAME)
{
	defaultIcon = 0;
	doIndexes = 0;
	fancyIndexing = 0;
	foldersFirst = 0;
	pattern = 0;
	sortField = 0;
	sortOrder = 1;
}

////////////////////////////////////////////////////////////////////////////////

MaDirHandler::~MaDirHandler()
{
	mprFree(sortField);
	mprFree(pattern);
}

////////////////////////////////////////////////////////////////////////////////

MaHandler *MaDirHandler::cloneHandler()
{
	MaDirHandler	*hp;

	hp = new MaDirHandler();
	hp->defaultIcon = defaultIcon;
	hp->sortField = mprStrdup(sortField);
	hp->sortOrder = sortOrder;
	hp->fancyIndexing = fancyIndexing;
	hp->foldersFirst = foldersFirst;
	hp->doIndexes = doIndexes;

	return hp;
}

////////////////////////////////////////////////////////////////////////////////
#if BLD_FEATURE_CONFIG_PARSE

int MaDirHandler::parseConfig(char *key, char *value, MaServer *server, 
	MaHost *host, MaAuth *auth, MaDir *dir, MaLocation *location)
{
	char	*name, *extensions, *option, *nextTok, *junk;

	if (mprStrCmpAnyCase(key, "AddIcon") == 0) {
		//	AddIcon file ext ext ext
		//	Not yet supported
		name = mprStrTok(value, " \t", &extensions);
		extList.parse(extensions);
		return 1;

	} else if (mprStrCmpAnyCase(key, "DefaultIcon") == 0) {
		//	DefaultIcon file
		//	Not yet supported
		defaultIcon = mprStrTok(value, " \t", &junk);
		return 1;

	} else if (mprStrCmpAnyCase(key, "IndexOrder") == 0) {
		//	IndexOrder ascending|descending name|date|size
		mprFree(sortField);
		sortField = 0;
		option = mprStrTok(value, " \t", &sortField);
		if (mprStrCmpAnyCase(option, "ascending") == 0) {
			sortOrder = 1;
		} else {
			sortOrder = -1;
		}
		if (sortField) {
			sortField = mprStrdup(sortField);
		}
		return 1;

	} else if (mprStrCmpAnyCase(key, "IndexIgnore") == 0) {
		//	IndexIgnore pat ...
		// 	Not yet supported
		ignoreList.parse(value);
		return 1;

	} else if (mprStrCmpAnyCase(key, "IndexOptions") == 0) {
		//	IndexOptions FancyIndexing|FoldersFirst ... (set of options)
		option = mprStrTok(value, " \t", &nextTok);
		while (option) {
			if (mprStrCmpAnyCase(option, "FancyIndexing") == 0) {
				fancyIndexing = 1;
			} else if (mprStrCmpAnyCase(option, "HTMLTable") == 0) {
				fancyIndexing = 2;
			} else if (mprStrCmpAnyCase(option, "FoldersFirst") == 0) {
				foldersFirst = 1;
			}
			option = mprStrTok(nextTok, " \t", &nextTok);
		}
		return 1;

	} else if (mprStrCmpAnyCase(key, "Options") == 0) {
		//	Options Indexes
		option = mprStrTok(value, " \t", &nextTok);
		while (option) {
			if (mprStrCmpAnyCase(option, "Indexes") == 0) {
				doIndexes = 1;
			}
			option = mprStrTok(nextTok, " \t", &nextTok);
		}
		return 1;
	}
	return 0;
}

#endif
////////////////////////////////////////////////////////////////////////////////
//
//	Override default matchRequest
//

int MaDirHandler::matchRequest(MaRequest *rq, char *uri, int uriLen)
{
	MaDir		*dir;
	char		path[MPR_MAX_FNAME], urlBuf[MPR_MAX_FNAME];
	const char	*delim;
	char		*index;

	if (rq->host->mapToStorage(rq, path, sizeof(path), uri, 0) < 0) {
		return 0;
	}
	if (path == 0) {
		return 0;
	}

	if (!rq->isDir(path)) {
		return 0;
	}

	/*
 	 *	We must force a redirect on URIs that don't end in "/" as we put
	 *	relative URLs in the directory listing. The URL in the browser must
	 *	end with "/" so that catenation with the links works.
	 */
	if (uri[strlen(uri) - 1] != '/') {

		//
		//	Redirect the user's browser as the requested resource is
		//	a directory.
		//
		mprSprintf(urlBuf, sizeof(urlBuf), "%s/", uri);
		rq->redirect(301, urlBuf);
		rq->flushOutput(MPR_HTTP_FOREGROUND_FLUSH, MPR_HTTP_FINISH_REQUEST);
		return -1;
	}

	/*
	 *	URI may have changed in mapToStorage
	 */
	uri = rq->getUri();
	dir = rq->host->findBestDir(path);
	rq->setDir(dir);

	index = dir->getIndex();
	mprAssert(index);

	delim = (path[strlen(path) - 1] == '/') ? "" : "/";

	if (mprStrcat(path, sizeof(path), delim, index, (void*) 0) < 0) {
		mprError(MPR_L, MPR_LOG, "File too big %s", path);
		/* Too big */
		return 0;
	}

	if (access(path, R_OK) == 0) {
		delim = (uri[strlen(uri) - 1] == '/') ? "" : "/";
		mprSprintf(urlBuf, sizeof(urlBuf), "%s%s%s", uri, delim, index);
		rq->setUri(urlBuf);
		rq->parseUri();
		return 0;
	}

	if (doIndexes == 0) {
		return 0;
	}

	return 1;
}

////////////////////////////////////////////////////////////////////////////////

int MaDirHandler::run(MaRequest *rq)
{
	MaDataStream	*dynBuf;
	char			*uri, urlBuf[MPR_MAX_FNAME];
	int				requestFlags;

	requestFlags = rq->getFlags();
	dynBuf = rq->getDynBuf();

	/*
 	 *	We may get here without matchRequest being called above if we are
	 *	matching by location block. We must force a redirect on URIs that
	 *	don't end in "/" as we put relative URLs in the directory listing. The
	 *	URL in the browser must end with "/" so that catenation with the links
	 *	works.
	 */
	uri = rq->getUri();
	if (uri[strlen(uri) - 1] != '/') {

		//
		//	Redirect the user's browser as the requested resource is
		//	a directory.
		//
		mprSprintf(urlBuf, sizeof(urlBuf), "%s/", uri);
		rq->redirect(301, urlBuf);
		rq->flushOutput(MPR_HTTP_FOREGROUND_FLUSH, MPR_HTTP_FINISH_REQUEST);
		return -1;
	}

	if (requestFlags & (MPR_HTTP_GET_REQUEST | MPR_HTTP_HEAD_REQUEST | 
			MPR_HTTP_POST_REQUEST)) {
		rq->insertDataStream(dynBuf);
		generateListing(rq);
	}

	rq->flushOutput(MPR_HTTP_FOREGROUND_FLUSH, MPR_HTTP_FINISH_REQUEST);
	return MPR_HTTP_HANDLER_FINISHED_PROCESSING;
}

////////////////////////////////////////////////////////////////////////////////

void MaDirHandler::generateListing(MaRequest *rq)
{
	MaDirEntry		*list;
	const char		*dir;
	uint			nameSize;
	int				count, i;

	list = 0;
	count = 0;

	/*
 	 *	Output header
	 */
	dir = rq->getFileName();
	mprAssert(dir);

	rq->setResponseCode(200);
	rq->setHeaderFlags(MPR_HTTP_DONT_CACHE, 0);
	rq->setResponseMimeType("text/html");

	parseQuery(rq);

	if (getDirList(rq, &list, &count, dir, 1) < 0) {
		rq->write("<h2>Can't get file list</h2>\r\n");
		outputFooter(rq);
		return;
	}

	if (pattern) {
		count = filterDirList(rq, list, count);
		mprAssert(count >= 0);
	}

	sortList(list, count);

	/*
	 *	Get max filename
	 */
	nameSize = 0;
	for (i = 0; i < count; i++) {
		nameSize = max(strlen(list[i].name), nameSize);
	}
	nameSize = max(nameSize, 22);

	outputHeader(rq, rq->getUri(), nameSize);
	for (i = 0; i < count; i++) {
		outputLine(rq, &list[i], dir, nameSize);
	}

	outputFooter(rq);

	for (i = 0; i < count; i++) {
		mprFree(list[i].name);
	}
	mprFree(list);
}
 
////////////////////////////////////////////////////////////////////////////////

void MaDirHandler::parseQuery(MaRequest *rq)
{
	char	*value, *query, *next, *tok;

	query = mprStrdup(rq->getUrl()->query);
	if (query == 0) {
		return;
	}

	tok = mprStrTok(query, ";&", &next);
	while (tok) {
		if ((value = strchr(tok, '=')) != 0) {
			*value++ = '\0';
			if (*tok == 'C') {					/* Sort column */
				mprFree(sortField);
				if (*value == 'N') {
					sortField = "Name";
				} else if (*value == 'M') {
					sortField = "Date";
				} else if (*value == 'S') {
					sortField = "Size";
				}
				sortField = mprStrdup(sortField);

			} else if (*tok == 'O') {			/* Sort order */
				if (*value == 'A') {
					sortOrder = 1;
				} else if (*value == 'D') {
					sortOrder = -1;
				}

			} else if (*tok == 'F') {			/* Format */ 
				if (*value == '0') {
					fancyIndexing = 0;
				} else if (*value == '1') {
					fancyIndexing = 1;
				} else if (*value == '2') {
					fancyIndexing = 2;
				}

			} else if (*tok == 'P') {			/* Pattern */ 
				pattern = mprStrdup(value);
			}
		}
		tok = mprStrTok(next, ";&", &next);
	}
	
	mprFree(query);
}

////////////////////////////////////////////////////////////////////////////////

void MaDirHandler::sortList(MaDirEntry *list, int count)
{
	MaDirEntry	tmp;
	int			i, j, rc;

	if (sortField == 0) {
		return;
	}
	if (mprStrCmpAnyCase(sortField, "Name") == 0) {
		for (i = 1; i < count; i++) {
			for (j = 0; j < i; j++) {
				rc = strcmp(list[i].name, list[j].name);
				if (foldersFirst) {
					if (list[i].isDir && !list[j].isDir) {
						rc = -sortOrder;
					} else if (list[j].isDir && !list[i].isDir) {
						rc = sortOrder;
					} 
				}
				rc *= sortOrder;
				if (rc < 0) {
					tmp = list[i];
					list[i] = list[j];
					list[j] = tmp;
				}
			}
		}

	} else if (mprStrCmpAnyCase(sortField, "Size") == 0) {
		for (i = 1; i < count; i++) {
			for (j = 0; j < i; j++) {
				rc = (list[i].size < list[j].size) ? -1 : 1;
				if (foldersFirst) {
					if (list[i].isDir && !list[j].isDir) {
						rc = -sortOrder;
					} else if (list[j].isDir && !list[i].isDir) {
						rc = sortOrder;
					}
				}
				rc *= sortOrder;
				if (rc < 0) {
					tmp = list[i];
					list[i] = list[j];
					list[j] = tmp;
				}
			}
		}

	} else if (mprStrCmpAnyCase(sortField, "Date") == 0) {
		for (i = 1; i < count; i++) {
			for (j = 0; j < i; j++) {
				rc = (list[i].lastModified < list[j].lastModified) ?
					-1: 1;
				if (foldersFirst) {
					if (list[i].isDir && !list[j].isDir) {
						rc = -sortOrder;
					} else if (list[j].isDir && !list[i].isDir) {
						rc = sortOrder;
					}
				}
				rc *= sortOrder;
				if (rc < 0) {
					tmp = list[i];
					list[i] = list[j];
					list[j] = tmp;
				}
			}
		}
	}
}

////////////////////////////////////////////////////////////////////////////////

void MaDirHandler::outputHeader(MaRequest *rq, const char *dir, int nameSize)
{
	char	parent[MPR_MAX_FNAME], *parentSuffix;
	int		order, reverseOrder, fancy, isRootDir;

	fancy = 1;

	rq->write("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 3.2 Final//EN\">\r\n");
	rq->writeFmt("<html>\r\n <head>\r\n  <title>Index of %s</title>\r\n", dir);
	rq->write(" </head>\r\n");
	rq->write("<body>\r\n");

	rq->writeFmt("<h1>Index of %s</h1>\r\n", dir);

	if (sortOrder > 0) {
		order = 'A';
		reverseOrder = 'D';
	} else {
		order = 'D';
		reverseOrder = 'A';
	}

	if (fancyIndexing == 0) {
		fancy = '0';
	} else if (fancyIndexing == 1) {
		fancy = '1';
	} else if (fancyIndexing == 2) {
		fancy = '2';
	}

	mprGetDirName(parent, sizeof(parent), (char*) dir);
	if (parent[strlen(parent) - 1] != '/') {
		parentSuffix = "/";
	} else {
		parentSuffix = "";
	}
	isRootDir = (strcmp(dir, "/") == 0);

	if (fancyIndexing == 2) {
		rq->writeFmt("<table><tr><th><img src=\"/icons/blank.gif\""
			" alt=\"[ICO]\" /></th>");

		rq->writeFmt("<th><a href=\"?C=N;O=%c;F=%c\">Name</a></th>",
			reverseOrder, fancy);
		rq->writeFmt("<th><a href=\"?C=M;O=%c;F=%c\">Last modified</a></th>",
			reverseOrder, fancy);
		rq->writeFmt("<th><a href=\"?C=S;O=%c;F=%c\">Size</a></th>",
			reverseOrder, fancy);
		rq->writeFmt("<th><a href=\"?C=D;O=%c;F=%c\">Description</a></th>\r\n",
			reverseOrder, fancy);

		rq->writeFmt("</tr><tr><th colspan=\"5\"><hr /></th></tr>\r\n");

		if (! isRootDir) {
			rq->writeFmt("<tr><td valign=\"top\"><img src=\"/icons/back.gif\"");
			rq->writeFmt("alt=\"[DIR]\" /></td><td><a href=\"%s%s\">", parent,
				parentSuffix);
			rq->writeFmt("Parent Directory</a></td>");
			rq->writeFmt("<td align=\"right\">  - </td></tr>\r\n");
		}

	} else if (fancyIndexing == 1) {
		rq->writeFmt("<pre><img src=\"/icons/space.gif\" alt=\"Icon\" /> ");

		rq->writeFmt("<a href=\"?C=N;O=%c;F=%c\">Name</a>%*s",
			reverseOrder, fancy, nameSize - 3, " ");
		rq->writeFmt("<a href=\"?C=M;O=%c;F=%c\">Last modified</a>       ",
			reverseOrder, fancy);
		rq->writeFmt("<a href=\"?C=S;O=%c;F=%c\">Size</a>               ",
			reverseOrder, fancy);
		rq->writeFmt("<a href=\"?C=D;O=%c;F=%c\">Description</a>\r\n",
			reverseOrder, fancy);

		rq->write("<hr />");

		if (! isRootDir) {
			rq->writeFmt("<img src=\"/icons/parent.gif\" alt=\"[DIR]\" />");
			rq->writeFmt(" <a href=\"%s%s\">Parent Directory</a>\r\n", parent,
				parentSuffix);
		}

	} else {
		rq->write("<ul>\n");
		if (! isRootDir) {
			rq->writeFmt("<li><a href=\"%s%s\"> Parent Directory</a></li>\r\n", 
				parent, parentSuffix);
		}
	}
}

////////////////////////////////////////////////////////////////////////////////

static void fmtNum(char *buf, int bufsize, int num, int divisor, char *suffix)
{
	int 	whole, point;

	whole = num / divisor;
	point = (num % divisor) / (divisor / 10);

	if (point == 0) {
		mprSprintf(buf, bufsize, "%6d%s", whole, suffix);
	} else {
		mprSprintf(buf, bufsize, "%4d.%d%s", whole, point, suffix);
	}
}

////////////////////////////////////////////////////////////////////////////////

void MaDirHandler::outputLine(MaRequest *rq, MaDirEntry *ep, const char *dir,
	int nameSize)
{
	MprFileInfo	fileInfo;
	time_t		when;
	char		path[MPR_MAX_FNAME], sizeBuf[16], timeBuf[48], *icon;
	struct tm	tm;
	bool		isDir;
	int			len;
	char		*dirSuffix;
	char		*months[] = { 
					"Jan", "Feb", "Mar", "Apr", "May", "Jun",
					"Jul", "Aug", "Sep", "Oct", "Nov", "Dec" 
				};

	if (ep->size >= (1024*1024*1024)) {
		fmtNum(sizeBuf, sizeof(sizeBuf), (int) ep->size, 1024 * 1024 * 1024,
			"G");

	} else if (ep->size >= (1024*1024)) {
		fmtNum(sizeBuf, sizeof(sizeBuf), (int) ep->size, 1024 * 1024, "M");

	} else if (ep->size >= 1024) {
		fmtNum(sizeBuf, sizeof(sizeBuf), (int) ep->size, 1024, "K");

	} else {
		mprSprintf(sizeBuf, sizeof(sizeBuf), "%6d", (int) ep->size);
	}

	mprSprintf(path, sizeof(path), "%s/%s", dir, ep->name);

	if (rq->host->server->getFileSystem()->stat(path, &fileInfo) < 0) {
		when = time(0);
		isDir = 0;

	} else {
		isDir = fileInfo.isDir ? 1 : 0;
	}

	if (isDir) {
		icon = "folder";
		dirSuffix = "/";
	} else {
		icon = "compressed";
		dirSuffix = "";
	}

	when = (time_t) fileInfo.mtime;
	mprLocaltime(&when, &tm);

	mprSprintf(timeBuf, sizeof(timeBuf), "%02d-%3s-%4d %02d:%02d",
		tm.tm_mday, months[tm.tm_mon], tm.tm_year + 1900, tm.tm_hour, 
		tm.tm_min);

	len = strlen(ep->name) + strlen(dirSuffix);

	if (fancyIndexing == 2) {

		rq->writeFmt("<tr><td valign=\"top\">");
		rq->writeFmt("<img src=\"/icons/%s.gif\" alt=\"[   ]\", /></td>", icon);

		rq->writeFmt("<td><a href=\"%s%s\">%s%s</a></td>",
			ep->name, dirSuffix, ep->name, dirSuffix);
		rq->writeFmt("<td>%s</td><td>%s</td></tr>\r\n", timeBuf, sizeBuf);


	} else if (fancyIndexing == 1) {

		rq->writeFmt("<img src=\"/icons/%s.gif\" alt=\"[   ]\", /> ", icon);

		rq->writeFmt("<a href=\"%s%s\">%s%s</a>%-*s %17s %4s\r\n", 
			ep->name, dirSuffix, ep->name, dirSuffix, nameSize - len, "", 
			timeBuf, sizeBuf);

	} else {

		rq->writeFmt("<li><a href=\"%s%s\"> %s%s</a></li>\r\n", 
			ep->name, dirSuffix, ep->name, dirSuffix);
	}
}

////////////////////////////////////////////////////////////////////////////////

void MaDirHandler::outputFooter(MaRequest *rq)
{
	if (fancyIndexing == 2) {
		rq->write("<tr><th colspan=\"5\"><hr /></th></tr>\r\n</table>\r\n");
	} else if (fancyIndexing == 1) {
		rq->write("<hr /></pre>\r\n");
	} else {
		rq->write("</ul>\r\n");
	}
	rq->writeFmt("<address>%s %s at %s Port %d</address>\r\n", 
		BLD_NAME, BLD_VERSION, rq->getIpAddr(), rq->getPort());
	rq->write("</body></html>\r\n");
}

////////////////////////////////////////////////////////////////////////////////

int MaDirHandler::filterDirList(MaRequest *rq, MaDirEntry *list, int count)
{
	int		i, j;

	/*
	 *	Do pattern matching. Entries that don't match, free the name to mark
	 */
	for (i = 0; i < count; i++) {
		if (! match(pattern, list[i].name)) {
			mprFree(list[i].name);
			list[i].name = 0;
		}
	}

	/*
	 *	Compact the list after filtering
	 */
	for (i = 0, j = 0; j < count; ) {
		if (list[j].name) {
			list[i] = list[j];
			i++;
		}
		j++;
	}
	return i;
}

////////////////////////////////////////////////////////////////////////////////
/*
 *	Return true if the file matches the pattern. Supports '?' and '*'
 */

static int match(char *pattern, char *file)
{
	char	*pp, *fp;

	if (pattern == 0 || *pattern == '\0') {
		return 1;
	}
	if (file == 0 || *file == '\0') {
		return 0;
	}

	for (pp = pattern, fp = file; *pp; ) {
		if (*fp == '\0') {
			if (*pp == '*' && pp[1] == '\0') {
				/* Trailing wild card */
				return 1;
			}
			return 0;
		}

		if (*pp == '*') {
			if (match(&pp[1], &fp[0])) {
				return 1;
			}
			fp++;
			continue;

		} else if (*pp == '?' || *pp == *fp) {
			fp++;

		} else {
			return 0;
		}
		pp++;
	}
	if (*fp == '\0') {
		/* Match */
		return 1;
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
#if WIN

static int getDirList(MaRequest *rq, MaDirEntry **list, int *count, 
	const char *dir, bool enumDirs)
{
	WIN32_FIND_DATA	findData;
	HANDLE			h;
	MaDirEntry		*dp;
	MprFileInfo		fileInfo;
	char			path[MPR_MAX_FNAME];
	int				numSlots;

	numSlots = 0;
	*count = 0;
	*list = 0;
	dp = 0;

	if (mprSprintf(path, sizeof(path), "%s/*.*", dir) < 0) {
		return MPR_ERR_WONT_FIT;
	}

	h = FindFirstFile(path, &findData);
	if (h == INVALID_HANDLE_VALUE) {
		return MPR_ERR_CANT_OPEN;
	}

	do {
		if (findData.cFileName[0] == '.') {
			continue;
		}
		if (enumDirs || 
				!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
			if (*count >= numSlots) {
				numSlots += *count + DIR_INCR;
				dp = (MaDirEntry*) mprRealloc(*list, 
					numSlots * sizeof(MaDirEntry));
				if (dp == 0) {
					mprError(MPR_L, MPR_LOG, "Can't allocate memory");
					return MPR_ERR_CANT_ALLOCATE;
				}
				*list = dp;
			}
			dp = &((*list)[*count]);
			(*count)++;

			dp->name = mprStrdup(findData.cFileName);
			if (dp->name == 0) {
				mprError(MPR_L, MPR_LOG, "Can't allocate memory");
				return MPR_ERR_CANT_ALLOCATE;
			}

			// dp->lastModified = (uint) findData.ftLastWriteTime.dwLowDateTime;

			if (mprSprintf(path, sizeof(path), "%s/%s", dir, dp->name) < 0) {
				dp->lastModified = 0;
			} else {
				rq->host->server->getFileSystem()->stat(path, &fileInfo);
				dp->lastModified = fileInfo.mtime;
			}

			dp->isDir = (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				? 1 : 0;

#if FUTURE_64_BIT
			if (findData.nFileSizeLow < 0) {
				dp->size =
					(((uint64) findData.nFileSizeHigh) * INT64(4294967296)) +
					(4294967296L - (uint64) findData.nFileSizeLow);
			} else {
				dp->size =
					(((uint64) findData.nFileSizeHigh * INT64(4294967296))) + 
					(uint64) findData.nFileSizeLow;
			}
#else
			dp->size = (uint) findData.nFileSizeLow;
#endif
		}
	} while (FindNextFile(h, &findData) != 0);

	FindClose(h);
	return 0;
}

#endif
////////////////////////////////////////////////////////////////////////////////

#if BLD_HOST_UNIX || VXWORKS || CYGWIN
static int getDirList(MaRequest *rq, MaDirEntry **list, int *count, 
	const char *path, bool enumDirs)
{
	DIR				*dir;
	MprFileInfo		fileInfo;
	struct dirent	*dirent;
	MaDirEntry		*dp;
	char			fileName[MPR_MAX_FNAME];
	int				numSlots, statRc;

	numSlots = 0;
	*list = 0;
	*count = 0;
	dp = 0;

	dir = opendir((char*) path);
	if (dir == 0) {
		return MPR_ERR_CANT_OPEN;
	}

	while ((dirent = readdir(dir)) != 0) {
		if (dirent->d_name[0] == '.') {
			continue;
		}

		mprSprintf(fileName, sizeof(fileName), "%s/%s", path, dirent->d_name);
		statRc = rq->host->server->getFileSystem()->stat((char*) fileName, 
			&fileInfo);

		if (enumDirs || (statRc == 0 && fileInfo.isDir)) { 
			if (*count >= numSlots) {
				numSlots += *count + DIR_INCR;
				dp = (MaDirEntry*) mprRealloc(*list, 
					numSlots * sizeof(MaDirEntry));
				if (dp == 0) {
					mprError(MPR_L, MPR_LOG, "Can't allocate memory");
					return MPR_ERR_CANT_ALLOCATE;
				}
				*list = dp;
			}
			dp = &((*list)[*count]);
			(*count)++;

			dp->name = mprStrdup(dirent->d_name);
			if (dp->name == 0) {
				mprError(MPR_L, MPR_LOG, "Can't allocate memory");
				return MPR_ERR_CANT_ALLOCATE;
			}

			if (statRc == 0) {
				dp->lastModified = fileInfo.mtime;
				dp->size = fileInfo.size;
				dp->isDir = fileInfo.isDir;
			} else {
				dp->lastModified = 0;
				dp->size = 0;
				dp->isDir = 0;
			}
		}
	}
	closedir(dir);
	return 0;
}

#endif
////////////////////////////////////////////////////////////////////////////////

void mprDirHandlerDummy() {}

#endif // BLD_FEATURE_DIR

//
// Local variables:
// tab-width: 4
// c-basic-offset: 4
// End:
// vim: sw=4 ts=4 
//
