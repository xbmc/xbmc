///
///	@file 	host.cpp
/// @brief 	Host class for all HTTP hosts
/// @overview The Host class is used for the default HTTP server and for 
///		all virtual hosts (including SSL hosts). Many objects are 
///		controlled at the host level. Eg. URL handlers.
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
//
////////////////////////////////// Includes ////////////////////////////////////

#include	"http.h"

//////////////////////////////////// Code //////////////////////////////////////

MaHost::MaHost(MaServer *sp, char *ipSpec)
{
	memset((void*) &stats, 0, sizeof(stats));

#if BLD_FEATURE_LOG
	tMod = new MprLogModule("httpHost");
#endif

	authEnabled = 1;
	documentRoot = 0;
	errorDocuments = 0;
	flags = MPR_HTTP_USE_CHUNKING | MPR_HTTP_NO_TRACE;
	httpVersion = MPR_HTTP_1_1;
	limits = sp->http->getLimits();
	mimeFile = 0;
	timeout = MPR_HTTP_SERVER_TIMEOUT;

	if (ipSpec) {
		this->ipSpec = mprStrdup(ipSpec);
		name = mprStrdup(ipSpec);
	} else {
		this->ipSpec = 0;
		name = 0;
	}
	
#if BLD_FEATURE_SESSION
	sessions = new MprHashTable(MA_HTTP_HASH_SIZE);
	sessionTimeout = MPR_HTTP_SESSION_TIMEOUT;
	sessionAutoCreate = false;
#endif

#if BLD_FEATURE_KEEP_ALIVE
	keepAliveTimeout = MPR_HTTP_KEEP_TIMEOUT;
	maxKeepAlive = MPR_HTTP_MAX_KEEP_ALIVE;
	keepAlive = 1;
#endif

#if BLD_FEATURE_ACCESS_LOG
	logHost = 0;
	logPath = 0;
	logFormat = 0;
	logFd = -1;
#endif

	secret = 0;
	server = sp;
	mimeTypes = 0;

#if BLD_FEATURE_SSL_MODULE
	secure = 0;
#endif
#if BLD_FEATURE_DLL
	moduleDirs = 0;
#endif

#if BLD_FEATURE_MULTITHREAD
	mutex = new MprMutex();
#endif
}

////////////////////////////////////////////////////////////////////////////////

MaHost::~MaHost()
{
	MaRequest		*rq, *nextRq;
	MaDir			*dp, *nextDp;
	MaLocation		*lp, *nextLp;
	MaAlias			*ap, *nextAp;
#if BLD_FEATURE_SESSION
	MaSession		*sp, *nextSp;

	sp = (MaSession*) sessions->getFirst();
	while (sp) {
		nextSp = (MaSession*) sessions->getNext(sp);
		sessions->remove(sp);
		delete sp;
		sp = nextSp;
	}
#endif

	rq = (MaRequest*) requests.getFirst();
	while (rq) {
		nextRq = (MaRequest*) requests.getNext(rq);
		rq->cancelRequest();
		rq->lock();
		delete rq;
		rq = nextRq;
	}

	dp = (MaDir*) dirs.getFirst();
	while (dp) {
		nextDp = (MaDir*) dirs.getNext(dp);
		dirs.remove(dp);
		delete dp;
		dp = nextDp;
	}

	lp = (MaLocation*) locations.getFirst();
	while (lp) {
		nextLp = (MaLocation*) locations.getNext(lp);
		locations.remove(lp);
		delete lp;
		lp = nextLp;
	}

	ap = (MaAlias*) aliases.getFirst();
	while (ap) {
		nextAp = (MaAlias*) aliases.getNext(ap);
		delete ap;
		ap = nextAp;
	}

	if (mimeTypes && !(flags & MPR_HTTP_HOST_REUSE_MIME)) {
		delete mimeTypes;
	}
	if (errorDocuments) {
		delete errorDocuments;
	}

#if BLD_FEATURE_ACCESS_LOG
	if (logPath) {
		mprFree(logPath);
	}
	if (logFormat) {
		mprFree(logFormat);
	}
#endif
#if BLD_FEATURE_SESSION
	delete sessions;
#endif

	mprFree(ipSpec);
	mprFree(mimeFile);
	mprFree(documentRoot);
	mprFree(name);
	mprFree(secret);

#if BLD_FEATURE_DLL
	mprFree(moduleDirs);
#endif
#if BLD_FEATURE_LOG
	delete tMod;
#endif
#if BLD_FEATURE_MULTITHREAD
	mutex->lock();
	delete mutex;
#endif
}

////////////////////////////////////////////////////////////////////////////////

int MaHost::start()
{
	char	*hex = "0123456789abcdef";
	uchar	bytes[MPR_HTTP_MAX_SECRET];
	char	ascii[MPR_HTTP_MAX_SECRET * 2 + 1], *ap;
	int		i;

	//
	//	Create a random secret for use in authentication. Don't block.
	//	FUTURE -- conditional on Auth
	//
	mprLog(7, "Get random bytes\n");
	if (mprGetRandomBytes(bytes, sizeof(bytes), 0) < 0) {
		mprError(MPR_L, MPR_LOG, "Can't generate local secret");
		return MPR_ERR_CANT_INITIALIZE;
	}
	ap = ascii;
	for (i = 0; i < (int) sizeof(bytes); i++) {
		*ap++ = hex[bytes[i] >> 4];
		*ap++ = hex[bytes[i] & 0xf];
	}
	*ap = '\0';
	secret = mprStrdup(ascii);
	mprLog(7, "Got %d random bytes\n", sizeof(bytes));

#if BLD_FEATURE_ACCESS_LOG && !BLD_FEATURE_ROMFS
	if (logPath) {
		logFd = open(logPath, O_CREAT | O_APPEND | O_WRONLY | O_TEXT, 0664);
		if (logFd < 0) {
			mprError(MPR_L, MPR_LOG, "Can't open log file %s", logPath);
		}
#if FUTURE
		rotateLog();
#endif
	}
#endif
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

int MaHost::stop()
{
	MaHandler	*hp, *nextHp;

	hp = (MaHandler*) handlers.getFirst();
	while (hp) {
		nextHp = (MaHandler*) handlers.getNext(hp);
		handlers.remove(hp);
		delete hp;
		hp = nextHp;
	}

#if BLD_FEATURE_ACCESS_LOG
	if (logFd >= 0) {
		close(logFd);
		logFd = -1;
	}
#endif
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Add the designated handler
//

int MaHost::addHandler(char *name, char *extensions)
{
	MaHandlerService	*hs;

	hs = server->http->lookupHandlerService(name);
	if (hs == 0) {
		mprError(MPR_L, MPR_LOG, "Can't find handler %s", name); 
		return MPR_ERR_NOT_FOUND;
	}
	if (extensions == 0 || *extensions == '\0') {
		mprLog(MPR_CONFIG, "Add %s\n", name);
		insertHandler(hs->newHandler(server, this, 0));

	} else {
		mprLog(MPR_CONFIG, "Add %s for \"%s\"\n", name, extensions);
		insertHandler(hs->newHandler(server, this, extensions));
	}
	flags |= MPR_HTTP_ADD_HANDLER;
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
#if BLD_FEATURE_ACCESS_LOG

void MaHost::setLog(char *path, char *format)
{
	char	*src, *dest;

	mprAssert(path && *path);
	mprAssert(format && *format);

	logPath = mprStrdup(path);
	logFormat = mprStrdup(format);

	for (src = dest = logFormat; *src; src++) {
		if (*src == '\\' && src[1] != '\\') {
			continue;
		}
		*dest++ = *src;
	}
	*dest = '\0';
}

////////////////////////////////////////////////////////////////////////////////

void MaHost::setLogHost(MaHost *host)
{
	logHost = host;
}

////////////////////////////////////////////////////////////////////////////////

void MaHost::writeLog(char *buf, int len)
{
	static int once = 0;
	if (write(logFd, buf, len) != len && once++ == 0) {
		mprError(MPR_L, MPR_LOG, "Can't write to access log %s", logPath);
	}
}

////////////////////////////////////////////////////////////////////////////////
#if FUTURE
//
//	Called to rotate the access log
//
void MaHost::rotateLog()
{
	struct stat	sbuf;
	char		fileBuf[MPR_MAX_FNAME];
	struct tm	tm;
	time_t		when;

	//
	//	Rotate logs when full
	//
	if (fstat(logFd, &sbuf) == 0 && sbuf.st_mode & S_IFREG && 
			(unsigned) sbuf.st_size > maxSize) {

		char bak[MPR_MAX_FNAME];

		time(&when);
		mprGmtime(&when, &tm);

		mprSprintf(bak, sizeof(bak), "%s-%02d-%02d-%02d-%02d:%02d:%02d", 
			logPath, 
			tm->tm_mon, tm->tm_mday, tm->tm_year, tm->tm_hour, tm->tm_min, 
			tm->tm_sec);

		close(logFd);
		rename(logPath, bak);
		unlink(logPath);

		logFd = open(logPath, O_CREAT | O_TRUNC | O_WRONLY | O_TEXT, 0664);
		logConfig();
	}
}

#endif // FUTURE
#endif
////////////////////////////////////////////////////////////////////////////////

void MaHost::setDocumentRoot(char *dir) 
{
	MaAlias		*ap;
	char		pathBuf[MPR_MAX_FNAME];
	int			len;

	makePath(pathBuf, sizeof(pathBuf) - 1, dir, 1);

	documentRoot = mprStrdup(pathBuf);

	//
	//	Create a catch-all alias. We want this directory to have a trailing
	//	"/" just like all aliases should.
	//
	len = strlen(pathBuf);
	if (pathBuf[len - 1] != '/') {
		pathBuf[len] = '/';
		pathBuf[len + 1] = '\0';
	}
	ap = new MaAlias("", pathBuf);
	insertAlias(ap);
}

////////////////////////////////////////////////////////////////////////////////

int MaHost::openMimeTypes(char *path)
{
	MprFile		*file;
	char		buf[80], *tok, *ext, *type;
	int			line;

	mimeFile = mprStrdup(path);
	mprAssert(mimeTypes == 0);
	file = server->fileSystem->newFile();
	
	if (mimeTypes == 0) {
		mimeTypes = new MprHashTable(157);
	}

	if (file->open(path, O_RDONLY | O_TEXT, 0) < 0) {
		mprError(MPR_L, MPR_LOG, "Can't open mime file %s", path);
		delete file;
		return MPR_ERR_CANT_OPEN;
	}
	line = 0;
	while (file->gets(buf, sizeof(buf)) != 0) {
		line++;
		if (buf[0] == '#' || isspace(buf[0])) {
			continue;
		}
		type = mprStrTok(buf, " \t\n\r", &tok);
		ext = mprStrTok(0, " \t\n\r", &tok);
		if (type == 0 || ext == 0) {
			mprError(MPR_L, MPR_LOG, "Bad mime spec in %s at line %d", 
				path, line);
			continue;
		}
		while (ext) {
			addMimeType(ext, type);
			ext = mprStrTok(0, " \t\n\r", &tok);
		}
	}
	file->close();
	delete file;
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Add a mime type to the mime lookup table. Action Programs are added 
//	separately.
//

void MaHost::addMimeType(char *ext, char *mimeType)
{
	if (*ext == '.') {
		ext++;
	}
	if (mimeTypes == 0) {
		mprError(MPR_L, MPR_LOG, 
			"Mime types file is not yet defined.\nIgnoring mime type %s", 
			mimeType);
		return;
	}
	mimeTypes->insert(new MaMimeHashEntry(ext, mimeType));
}

////////////////////////////////////////////////////////////////////////////////

void MaHost::setMimeActionProgram(char *mimeType, char *actionProgram)
{
	MaMimeHashEntry		*mt;

	mt = (MaMimeHashEntry*) mimeTypes->getFirst();
	while (mt) {
		if (strcmp(mt->getMimeType(), mimeType) == 0) {
			mt->setActionProgram(actionProgram);
			return;
		}
		mt = (MaMimeHashEntry*) mimeTypes->getNext(mt);
	}
	mprError(MPR_L, MPR_LOG, "Can't find mime type %s", mimeType);
}

////////////////////////////////////////////////////////////////////////////////

char *MaHost::getMimeActionProgram(char *mimeType)
{
	MaMimeHashEntry		*mt;

	mt = (MaMimeHashEntry*) mimeTypes->getFirst();
	while (mt) {
		if (strcmp(mt->getMimeType(), mimeType) == 0) {
			return mt->getActionProgram();
		}
		mt = (MaMimeHashEntry*) mimeTypes->getNext(mt);
	}
	mprError(MPR_L, MPR_LOG, "Can't find mime type %s", mimeType);
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

void MaHost::setMimeTypes(MprHashTable *table)
{
	lock();
	mimeTypes = table;
	flags |= MPR_HTTP_HOST_REUSE_MIME;
	unlock();
}

////////////////////////////////////////////////////////////////////////////////

MprHashTable *MaHost::getMimeTypes()
{
	return mimeTypes;
}

////////////////////////////////////////////////////////////////////////////////

void MaHost::insertRequest(MaRequest *rq)
{
	lock();
	requests.insert(rq);
	mprAssert(stats.activeRequests >= 0);
	stats.requests++;
	stats.activeRequests++;
	if (stats.activeRequests > stats.maxActiveRequests) {
		stats.maxActiveRequests = stats.activeRequests;
	}
	unlock();
}

////////////////////////////////////////////////////////////////////////////////
//
//	Remove request and aggregate stats
//

void MaHost::removeRequest(MaRequest *rq)
{
	lock();
	requests.remove(rq);
	mprAssert(stats.activeRequests > 0);
	stats.activeRequests--;
	//
	//	Aggregate the request stats
	//
	stats.errors += rq->stats.errors;
	stats.keptAlive += rq->stats.keptAlive;
	stats.redirects += rq->stats.redirects;
	stats.timeouts += rq->stats.timeouts;
	stats.copyDown += rq->stats.copyDown;
	unlock();
}

////////////////////////////////////////////////////////////////////////////////

char *MaHost::lookupMimeType(char *ext)
{
	MprStringHashEntry	*hp;

	hp = (MprStringHashEntry*) mimeTypes->lookup(ext);
	if (hp == 0) {
		return 0;
	}
	return hp->getValue();
}

////////////////////////////////////////////////////////////////////////////////

void MaHost::setName(char *str)
{
	lock();
	if (name) {
		mprFree(name);
	}
	name = mprStrdup(str);
	unlock();
}

////////////////////////////////////////////////////////////////////////////////

void MaHost::setIpSpec(char *str)
{
	char	buf[MPR_MAX_IP_ADDR_PORT];

	mprAssert(str);

	if (*str == ':') {
		str++;
	}
	if (isdigit(*str) && strchr(str, '.') == 0) {
		mprSprintf(buf, sizeof(buf), "127.0.0.1:%s", str);
		str = buf;
	}
	lock();
	if (ipSpec) {
		mprFree(ipSpec);
	}
	ipSpec = mprStrdup(str);
	unlock();
}

////////////////////////////////////////////////////////////////////////////////

bool MaHost::isVhost()
{
	return (bool) ((flags & MPR_HTTP_VHOST) != 0);
}

////////////////////////////////////////////////////////////////////////////////

bool MaHost::isNamedVhost()
{
	return (bool) ((flags & MPR_HTTP_NAMED_VHOST) != 0);
}

////////////////////////////////////////////////////////////////////////////////

void MaHost::setVhost()
{
	flags |= MPR_HTTP_VHOST;
}

////////////////////////////////////////////////////////////////////////////////

void MaHost::setNamedVhost()
{
	flags |= MPR_HTTP_NAMED_VHOST;
}

////////////////////////////////////////////////////////////////////////////////

void MaHost::setTraceMethod(bool on)
{
	if (on) {
		flags &= ~MPR_HTTP_NO_TRACE;
	} else {
		flags |= MPR_HTTP_NO_TRACE;
	}
}

////////////////////////////////////////////////////////////////////////////////
#if BLD_FEATURE_DLL

void MaHost::setModuleDirs(char *path)
{
	mprLog(MPR_CONFIG, "Module search path:\n\t\t\t%s\n", path);
	mprFree(moduleDirs);
	moduleDirs = mprStrdup(path);

	mprSetModuleSearchPath(path);
}

#endif // BLD_FEATURE_DLL
////////////////////////////////////////////////////////////////////////////////

void MaHost::insertHandler(MaHandler *item)
{
	lock();
	handlers.insert(item);
	unlock();
}

////////////////////////////////////////////////////////////////////////////////

MaHandler *MaHost::lookupHandler(char *name)
{
	MaHandler		*hp;

	hp = (MaHandler*) handlers.getFirst();
	while (hp) {
		if (strcmp(hp->getName(), name) == 0) {
			return hp;
		}
		hp = (MaHandler*) handlers.getNext(hp);
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

void MaHost::deleteHandlers()
{
	MaHandler		*hp, *nextHp;

	hp = (MaHandler*) handlers.getFirst();
	while (hp) {
		nextHp = (MaHandler*) handlers.getNext(hp);
		handlers.remove(hp);
		delete hp;
		hp = nextHp;
	}
	flags |= MPR_HTTP_RESET_HANDLERS;
}

////////////////////////////////////////////////////////////////////////////////

int MaHost::insertLocation(MaLocation *item)
{
	MaLocation	*lp;
	int			rc;

	//
	//	Sort in reverse collating sequence. Must make sure that /abc/def sorts
	//	before /abc
	//
	lock();
	lp = (MaLocation*) locations.getFirst();
	while (lp) {
		rc = strcmp(item->getPrefix(), lp->getPrefix());
		if (rc == 0) {
			lp->insertPrior(item);
			locations.remove(lp);
			delete lp;
			unlock();
			return 0;
		}
		if (strcmp(item->getPrefix(), lp->getPrefix()) > 0) {
			lp->insertPrior(item);
			unlock();
			return 0;
		}
		lp = (MaLocation*) locations.getNext(lp);
	}
	locations.insert(item);
	unlock();
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Find a location object
//

MaLocation *MaHost::findLocation(char *prefix)
{
	MaLocation	*lp;

	lock();
	lp = (MaLocation*) locations.getFirst();
	while (lp) {
		if (strcmp(prefix, lp->getPrefix()) == 0) {
			unlock();
			return lp;
		}
		lp = (MaLocation*) locations.getNext(lp);
	}
	unlock();
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Inherit the aliases, directories, handlers and locations from another host
//

void MaHost::inheritHost(MaHost *host)
{
	MaHandler			*hp;
	MaHandlerService	*hs;
	MaLocation			*lp;
	MaDir				*dp;
	MaAlias				*ap;

	//
	//	Inherit key settings
	//
	httpVersion = host->httpVersion;
#if BLD_FEATURE_KEEP_ALIVE
	keepAlive = host->keepAlive;
	keepAliveTimeout = host->keepAliveTimeout;
	maxKeepAlive = host->maxKeepAlive;
#endif
	timeout = host->timeout;
#if BLD_FEATURE_SESSION
	sessionTimeout = host->sessionTimeout;
#endif
#if BLD_FEATURE_ACCESS_LOG
	logHost = host->logHost;
#endif
	setMimeTypes(host->mimeTypes);

	//
	//	Inherit Aliases
	//
	ap = (MaAlias*) host->aliases.getFirst();
	while (ap) {
		insertAlias(new MaAlias(ap));
		ap = (MaAlias*) host->aliases.getNext(ap);
	}

	//
	//	Inherit Directories
	//
	dp = (MaDir*) host->dirs.getFirst();
	while (dp) {
		insertDir(new MaDir(this, dp));
		dp = (MaDir*) host->dirs.getNext(dp);
	}

	//
	//	Inherit Handlers
	//
	hp = (MaHandler*) host->handlers.getFirst();
	while (hp) {
		hs = server->http->lookupHandlerService(hp->getName());
		insertHandler(hs->newHandler(host->getServer(), this, 
			hp->getExtensions()));
		hp = (MaHandler*) host->handlers.getNext(hp);
	}

	//
	//	Inherit Locations
	//
	lp = (MaLocation*) host->locations.getFirst();
	while (lp) {
		insertLocation(new MaLocation(lp));
		lp = (MaLocation*) host->locations.getNext(lp);
	}
}

////////////////////////////////////////////////////////////////////////////////
//
//	Return the terminal handler that will actually process the request
//

MaHandler *MaHost::matchHandlers(MaRequest *rq)
{
	MaHandler	*hp, *cloneHp;
	MaLocation	*lp;
	char		*uri;
	int			uriLen, rc;

	mprAssert(rq);

rematch:
	//
	//	Insert all handlers that are specified as match always. This includes
	//	typically the Auth handler
	// 
	hp = (MaHandler*) handlers.getFirst();
	while (hp) {
		if (hp->flags & MPR_HANDLER_ALWAYS) {
			cloneHp = hp->cloneHandler();
			rq->insertHandler(cloneHp);
			if (hp->getFlags() & MPR_HANDLER_TERMINAL) {
				rq->setExtraPath(0, -1);
				return cloneHp;
			}
		}
		hp = (MaHandler*) handlers.getNext(hp);
	}

	//
	//	Then match by URI prefix. A URI may have an extra path segment after
	//	the URI (E.g. /cgi-bin/scriptName/some/Extra/Path. We need to know where
	//	the URI ends. NOTE: ScriptAlias directives are handled here.
	//
	lp = (MaLocation*) locations.getFirst();
	while (lp) {
		uri = rq->getUri();
		if (lp->getHandlerName() != 0) {

			rc = strncmp(lp->getPrefix(), uri, lp->getPrefixLen());
			if (rc == 0) {
					
				hp = lookupHandler(lp->getHandlerName());
				if (hp == 0) {
					mprError(MPR_L, MPR_LOG, "Handler %s has not been added", 
						lp->getHandlerName());
					lp = (MaLocation*) locations.getNext(lp);
					continue;
				}
				/* FUTURE -- match all method types */
				if (rq->getFlags() & MPR_HTTP_PUT_REQUEST && 
						!(hp->getFlags() & MPR_HANDLER_PUT)) {
					continue;
				}
				if (lp->getFlags() & MPR_HTTP_LOC_EXTRA_PATH ||
						(hp->flags & MPR_HANDLER_TERMINAL &&
						 hp->flags & MPR_HANDLER_EXTRA_PATH)) {
					if (rq->setExtraPath(lp->getPrefix(), lp->getPrefixLen()) 
							< 0) {
						rq->requestError(400, "Extra path does not validate");
						rq->finishRequest();
						return 0;
					}
				}
				rq->setLocation(lp);
				cloneHp = hp->cloneHandler();
				rq->insertHandler(cloneHp);
				if (hp->getFlags() & MPR_HANDLER_TERMINAL) {
					return cloneHp;
				}
				break;
			}
		}
		lp = (MaLocation*) locations.getNext(lp);
	}
	rq->setExtraPath(0, -1);

	//
	//	Now match by extension or by any custom handler matching technique
	//	FUTURE - optimize match by extension.
	//
	hp = (MaHandler*) handlers.getFirst();
	while (hp) {
		uri = rq->getUri();
		uriLen = strlen(uri);
		rc = hp->matchRequest(rq, uri, uriLen);
		if (rc < 0) {
			return 0;
		} else if (rc == 1) {
			if (! (hp->getFlags() & MPR_HANDLER_ALWAYS)) {
				cloneHp = hp->cloneHandler();
				rq->insertHandler(cloneHp);
				if (hp->getFlags() & MPR_HANDLER_TERMINAL) {
					return cloneHp;
				}
			}
		}
		if (uri != rq->getUri()) {
			rq->deleteHandlers();
			goto rematch;
		}
		hp = (MaHandler*) handlers.getNext(hp);
	}
	rq->requestError(404, "No handler for URL: %s", rq->getUri());
	rq->finishRequest();
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Handle Aliases and determine how a URI maps on to physical storage.
//	May redirect if the alias is a redirection. Also the URI 
//

int MaHost::mapToStorage(MaRequest *rq, char *path, int pathLen, char *uri,
	int flags)
{
	MaAlias		*ap;
	MaDir		*dir;
	char		urlBuf[MPR_HTTP_MAX_URL];
	char		*base, *aliasName, *prefix;
	int			len, redirectCode, prefixLen;

	mprAssert(path);
	mprAssert(pathLen > 0);

	*path = '\0';

	lock();

	//
	//	FUTURE - Could optimize by having a hash lookup for alias prefixes.
	//
	ap = (MaAlias*) aliases.getFirst();
	while (ap) {
		aliasName = ap->getName();
		prefix = ap->getPrefix();
		prefixLen = ap->getPrefixLen();

		//
		//	Note that for Aliases with redirects, the aliasName is a URL and
		//	not a document path.
		//
		if (strncmp(prefix, uri, prefixLen) == 0) {
			redirectCode = ap->getRedirectCode();
			if (redirectCode) {
				if (flags & MPR_HTTP_REDIRECT) {
					rq->redirect(redirectCode, aliasName);
					rq->flushOutput(MPR_HTTP_FOREGROUND_FLUSH, 
						MPR_HTTP_FINISH_REQUEST);
				} else {
					mprStrcpy(path, pathLen, aliasName);
				}
				unlock();
				return 0;
			}

			base = &uri[prefixLen];
			while (*base == '/') {
				base++;
			}
			if (aliasName[strlen(aliasName) - 1] == '/') {
				len = mprSprintf(path, pathLen, "%s%s", aliasName, base);
			} else if (*base) {
				len = mprSprintf(path, pathLen, "%s/%s", aliasName, base);
			} else {
				len = mprStrcpy(path, pathLen, aliasName);
			}

			dir = 0;

			if (*base != '\0') {
				//
				//	There was some extra URI past the matching alias prefix 
				//	portion
				//
#if WIN
				//
				//	Windows will ignore trailing "." and " ". We must reject
				//	here as the URL probably won't match due to the trailing
				//	character and the copyHandler will return the unprocessed
				//	content to the user. Bad.
				//
				int lastc = base[strlen(base) - 1];
				if (lastc == '.' || lastc == ' ') {
					unlock();
					return MPR_ERR_CANT_ACCESS;
				}
#endif		
				unlock();
				return 0;

			} else {
				if (*prefix == '\0') {
					prefix = "/";
				}
				//
				//	The URL matched the alias exactly so we change the URI
				//	to match the prefix as we may have changed the extension
				//
				mprStrcpy(urlBuf, sizeof(urlBuf), prefix);

				MaUrl *url = rq->getUrl();

				if (strcmp(urlBuf, url->uri) != 0) {
					if (url->query && *url->query) {
						mprStrcat(urlBuf, sizeof(urlBuf), "?", url->query, 
							(void*) 0);
					}
					if (flags & MPR_HTTP_REMATCH) {
						rq->setUri(urlBuf);
						rq->parseUri();
					}
				}
			}

			if (flags & MPR_HTTP_REMATCH) {
				rq->deleteHandlers();
			}
			unlock();
			return 0;
		}
		ap = (MaAlias*) aliases.getNext(ap);
	}
	unlock();
	return MPR_ERR_NOT_FOUND;
}

////////////////////////////////////////////////////////////////////////////////

int MaHost::insertAlias(MaAlias *item)
{
	MaAlias		*ap;
	int			rc;

	//
	//	Sort in reverse collating sequence. Must make sure that /abc/def sorts
	//	before /abc. But we sort redirects with status codes first.
	//
	lock();
	ap = (MaAlias*) aliases.getFirst();
	while (ap) {
		rc = strcmp(item->getPrefix(), ap->getPrefix()); 
		if (rc == 0) {
			ap->insertPrior(item);
			aliases.remove(ap);
			delete ap;
			unlock();
			return 0;
		}
		if (rc > 0) {
			if (item->getRedirectCode() >= ap->getRedirectCode()) {
				ap->insertPrior(item);
				unlock();
				return 0;
			}
		}
		ap = (MaAlias*) aliases.getNext(ap);
	}
	aliases.insert(item);
	unlock();
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

void MaHost::insertDir(MaDir *item)
{
	MaDir		*dp;
	int			rc;

	//
	//	Sort in reverse collating sequence. Must make sure that /abc/def sorts
	//	before /abc
	//
	lock();
	if (item->getPath()) {
		dp = (MaDir*) dirs.getFirst();
		while (dp) {
			if (dp->getPath()) {
				rc = strcmp(item->getPath(), dp->getPath());
				if (rc == 0) {
					dp->insertPrior(item);
					dirs.remove(dp);
					delete dp;
					unlock();
					return;
				} else if (rc > 0) {
					dp->insertPrior(item);
					unlock();
					return;
				} 
			}
			dp = (MaDir*) dirs.getNext(dp);
		}
	}
	dirs.insert(item);
	unlock();
}

////////////////////////////////////////////////////////////////////////////////
//
//	Find an exact match. May be called with raw file names. ie. D:\myDir
//

MaDir *MaHost::findDir(char *path)
{
	MaDir		*dp;
	char		buf[MPR_MAX_FNAME];
	char		*dirPath;
	int			len;

	mprGetFullPathName(buf, sizeof(buf) - 1, path);
	len = strlen(buf);
	if (buf[len - 1] != '/') {
		buf[len] = '/';
		buf[++len] = '\0';
	}
#if WIN
	//
	//	Windows is case insensitive for file names. Always map to lower case.
	//
	mprStrLower(buf);
#endif

	dp = (MaDir*) dirs.getFirst();
	while (dp) {
		dirPath = dp->getPath();
		if (dirPath != 0 && strcmp(dirPath, buf) == 0) {
			return dp;
		}
		dp = (MaDir*) dirs.getNext(dp);
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Find the directory entry that this file (path) resides in. path is a 
//	physical file path. We find the most specific (longest) directory that 
//	matches. The directory must match or be a parent of path. Not called with 
//	raw files names. They will be lower case and only have forward slashes. 
//	For windows, the will be in cannonical format with drive specifiers.
//

MaDir *MaHost::findBestDir(char *path)
{
	MaDir	*dp;
	int		len, dlen;

	len = strlen(path);
	dp = (MaDir*) dirs.getFirst();
	while (dp) {
		dlen = dp->getPathLen();
		//
		//	Special case where the Directory has a trailing "/" and we match
		//	all the letter before that.
		//
		if (len == (dlen - 1) && dp->getPath()[len] == '/') {
			dlen--;
		}
#if WIN
		if (mprStrCmpAnyCaseCount(dp->getPath(), path, dlen) == 0)
#else
		if (strncmp(dp->getPath(), path, dlen) == 0)
#endif
		{
			if (dlen >= 0) {
				return dp;
			}
		}
		dp = (MaDir*) dirs.getNext(dp);
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

MprList *MaHost::getAliases()
{
	return &aliases;
}

////////////////////////////////////////////////////////////////////////////////

MprList *MaHost::getDirs()
{
	return &dirs;
}

////////////////////////////////////////////////////////////////////////////////

MprList *MaHost::getLocations()
{
	return &locations;
}

////////////////////////////////////////////////////////////////////////////////

MprList *MaHost::getHandlers()
{
	return &handlers;
}

////////////////////////////////////////////////////////////////////////////////

char *MaHost::makePath(char *buf, int buflen, char *file, bool validate)
{
	char	tmp[MPR_MAX_FNAME];

	mprAssert(file);

	if (replaceReferences(buf, buflen, file) == 0) {
		//  Overflow
		return 0;
	}

	if (*buf == '\0' || strcmp(buf, ".") == 0) {
		mprStrcpy(tmp, sizeof(tmp), server->getServerRoot());

#if WIN
	} else if (*buf != '/' && buf[1] != ':' && buf[2] != '/') {
		mprSprintf(tmp, buflen, "%s/%s", server->getServerRoot(), buf);
#elif VXWORKS
	} else if (strchr((buf), ':') == 0 && *buf != '/') {
		mprSprintf(tmp, buflen, "%s/%s", server->getServerRoot(), buf);
#else
	} else if (*buf != '/') {
		mprSprintf(tmp, buflen, "%s/%s", server->getServerRoot(), buf);
#endif

	} else {
		mprStrcpy(tmp, sizeof(tmp), buf);
	}

	//
	//	Convert to an fully qualified name (on windows).
	//
	mprGetFullPathName(buf, buflen, tmp);

	//
	//	Valided removes "." and ".." from the path and map '\\' to '/'
	//	Restore "." if the path is now empty.
	//
	if (validate) {
		maValidateUri(buf);
		if (*buf == '\0') {
			mprStrcpy(buf, buflen, ".");
		}
	}
	return buf;
}

////////////////////////////////////////////////////////////////////////////////
//
//  Return 0 on overflow. FUTURE -- should replace this with allocated buffers
//
char *MaHost::replaceReferences(char *buf, int buflen, char *str)
{
	char	*src, *dest, *key, *root;
	int		len;

	dest = buf;
	buflen--;
	for (src = str; *src && buflen > 0; ) {
		if (*src == '$') {
			*dest = '\0';
			len = 0;
			key = "DOCUMENT_ROOT";
			if (strncmp(++src, key, strlen(key)) == 0 ) {
				root = getDocumentRoot();
				if (root) {
					len = mprStrcpy(dest, buflen, root);
				}
			} else {
				key = "SERVER_ROOT";
				if (strncmp(src, key, strlen(key)) == 0) {
					len = mprStrcpy(dest, buflen, server->getServerRoot());
				} else {
					key = "PRODUCT";
					if (strncmp(src, key, strlen(key)) == 0) {
						len = mprStrcpy(dest, buflen, BLD_PRODUCT);
					}
				}
			}
			if (*dest) {
				if (len > 0 && dest[len - 1] == '/') {
					dest[len - 1] = '\0';
				}
				len = strlen(dest);
				dest += len;
				buflen -= len;
				src += strlen(key);
				continue;
			}
		}
		*dest++ = *src++;
		buflen--;
	}

	if (buflen <= 0) {
		return 0;
	}

	*dest = '\0';

	return buf;
}

////////////////////////////////////////////////////////////////////////////////
#if BLD_FEATURE_SSL_MODULE
//	MOB -- locking

void MaHost::setSecure(bool on)
{
	secure = on;
}

#endif
////////////////////////////////////////////////////////////////////////////////

void MaHost::setHttpVersion(int v) 
{ 
	httpVersion = v; 

	if (httpVersion == MPR_HTTP_1_1) {
		flags |= MPR_HTTP_USE_CHUNKING;
	} else {
		flags &= ~MPR_HTTP_USE_CHUNKING;
	}
}

////////////////////////////////////////////////////////////////////////////////
#if BLD_FEATURE_SESSION
//
//	Create a new session including the session ID.
//

MaSession *MaHost::createSession(int timeout)
{
	MaSession	*sp;
	char		idBuf[64], *id;
	static int	idCount = 0;

	//
	//	Create a new session ID
	//
	mprSprintf(idBuf, sizeof(idBuf), "%08x%08x%08x", this, time(0), idCount++);
	id = maMD5(idBuf);

	if (timeout <= 0) {
		timeout = sessionTimeout;
	}
	sp = new MaSession(this, id, timeout);
	mprFree(id);

	//
	//	Keep a hash lookup table of all sessions. The key is the session ID.
	//
	lock();
	sessions->insert(sp);
	unlock();
	return sp;
}

////////////////////////////////////////////////////////////////////////////////

void MaHost::destroySession(MaSession *session)
{
	lock();
	sessions->remove(session);
	delete session;
	unlock();
}

////////////////////////////////////////////////////////////////////////////////

MaSession *MaHost::lookupSession(char *sessionId)
{
	MaSession	*sp;

	mprAssert(sessionId && *sessionId);

	lock();
	sp = (MaSession*) sessions->lookup(sessionId);
	if (sp) {
		sp->setLastActivity(mprGetTime(0));
	}
	unlock();
	return sp;
}

#endif /* BLD_FEATURE_SESSION */
////////////////////////////////////////////////////////////////////////////////

void MaHost::addErrorDocument(char *code, char *url)
{
	if (errorDocuments == 0) {
		errorDocuments = new MprHashTable(17);
	}
	errorDocuments->insert(new MprStringHashEntry(code, url));
}

////////////////////////////////////////////////////////////////////////////////

char *MaHost::lookupErrorDocument(int code)
{
	MprStringHashEntry	*sp;
	char				numBuf[16];

	if (errorDocuments == 0) {
		return 0;
	}
	mprItoa(code, numBuf, sizeof(numBuf));
	sp = (MprStringHashEntry*) errorDocuments->lookup(numBuf);
	if (sp == 0) {
		return 0;
	}
	return sp->getValue();
}

////////////////////////////////////////////////////////////////////////////////

void MaHost::setChunking(bool on)
{
	if (on) {
		flags |= MPR_HTTP_USE_CHUNKING;
	} else {
		flags &= ~MPR_HTTP_USE_CHUNKING;
	}
}

////////////////////////////////////////////////////////////////////////////////

bool MaHost::useChunking()
{
	return (flags & MPR_HTTP_USE_CHUNKING) ? 1 : 0;
}

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////// MaSession /////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
#if BLD_FEATURE_SESSION

static void sessionTimeoutProc(void *data, MprTimer *tp)
{
	MaSession 	*sp = (MaSession*) data;
	MaHost		*host;

	sp->lock();
	mprLog(6, "Session %s\nTimeout %d: time since last activity %d secs\n", 
		sp->getId(), sp->getTimeout() / 1000, 
		(mprGetTime(0) - sp->getLastActivity()) / 1000);
	if ((sp->getLastActivity() + sp->getTimeout()) > mprGetTime(0)) {
		tp->reschedule();
		sp->unlock();
		return;
	}

	mprLog(3, "Session Timeout: deleting session %s\n", sp->getId());
	host = sp->getHost();
	host->getSessions()->remove(sp);

	sp->setExpiryTimer(0);
	tp->dispose();

	sp->unlock();
	delete sp;
}

////////////////////////////////////////////////////////////////////////////////

MaSession::MaSession(MaHost *host, char *sessionId, int timeout) : 
	MprHashEntry(sessionId)
{
	int		period;

	this->host = host;
	this->timeout = timeout * 1000;
	lastActivity = mprGetTime(0);

	expiryCallback = 0;
	expiryArg = 0;

	sessionData = mprCreateObjVar("session", MA_HTTP_HASH_SIZE);
	mprSetVarDeleteProtect(&sessionData, 1);

	//
	//	Check for timeout expiry every 1/10 of the session period. This ensures 
	//	we do timeout at most 110% of the timeout period after the last
	//	access. Set a max of 30 secs so we don't do this too often for small
	//	session periods.
	//
	period = max(this->timeout / 10, 15 * 1000);
	expiryTimer = new MprTimer(period, sessionTimeoutProc, this);
#if BLD_FEATURE_MULTITHREAD
	mutex = new MprMutex();
#endif
	mprLog(3, "MaSession: Create session %s\n", getId());
}

////////////////////////////////////////////////////////////////////////////////
//
//	Note that a session may be deleted while others are still using it.
//	The sessionData variable object will have a positive refCount until the
//	last user requests finishes, then it will be deleted.
//

MaSession::~MaSession()
{
	if (expiryCallback) {
		(expiryCallback)(expiryArg);
	}

	mprLog(3, "MaSession: Destroy session %s\n", getId());
	if (expiryTimer) {
		expiryTimer->stop(MPR_TIMEOUT_STOP_TASK);
		expiryTimer->dispose();
	}
	//
	//	Will only delete if no other users are accessing.
	//
	mprDestroyVar(&sessionData);
#if BLD_FEATURE_MULTITHREAD
	mutex->lock();
	delete mutex;
#endif
}

#endif // BLD_FEATURE_SESSION
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////// MaMimeHashEntry /////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
//	Construct a new string hash entry. Always duplicate the value.
//

MaMimeHashEntry::MaMimeHashEntry(char *ext, char *mimeType) : 
	MprHashEntry(ext)
{
	this->mimeType = mprStrdup(mimeType);
	actionProgram = 0;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Virtual destructor.
//

MaMimeHashEntry::~MaMimeHashEntry()
{
	mprFree(mimeType);
	mprFree(actionProgram);
}

////////////////////////////////////////////////////////////////////////////////

void MaMimeHashEntry::setActionProgram(char *actionProgram)
{
	mprFree(this->actionProgram);
	this->actionProgram = mprStrdup(actionProgram);
}

////////////////////////////////////////////////////////////////////////////////

//
// Local variables:
// tab-width: 4
// c-basic-offset: 4
// End:
// vim: sw=4 ts=4 
//
