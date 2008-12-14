//
///	@file 	request.cpp
/// @brief 	Request class to handle individual HTTP requests.
///	@overview The Request class is the real work-horse in managing 
///		HTTP requests. An instance is created per HTTP request. 
///		During keep-alive it is preserved to process further requests.
///	@remarks Requests run in a single thread and do not need multi-thread 
///		locking except for the timeout code which may run on another thread.
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

////////////////////////////// Forward Declarations ////////////////////////////

static void	socketEventWrapper(void *data, MprSocket *sock, int mask, int isPool);
static void	timeoutWrapper(void *arg, MprTimer *tp);
static int	refillDoc(MprBuf *bp, void *arg);

//////////////////////////////////// Code //////////////////////////////////////
//
//	Requests are only ever instantiated from acceptWrapper in server.cpp which
//	is only ever called by select/task. So these are serialized by select.
//

MaRequest::MaRequest(MaHostAddress *ap, MaHost *hp)
{
	int		i;

	memset((void*) &stats, 0, sizeof(stats));
	memset((void*) &fileInfo, 0, sizeof(fileInfo));

	address = ap;
	host = hp;

#if BLD_FEATURE_LOG
	tMod = new MprLogModule("request");
	mprLog(6, tMod, "New Request, this %x\n", this);
#endif

	bytesWritten = 0;
	currentHandler = 0;
	contentLength = -1;
	contentLengthStr[0] = '\0';
	decodedQuery = 0;
	dir = 0;
	etag = 0;
	extraPath = 0;
	file = 0;
	fileName = 0;
	fileSystem = host->server->getFileSystem();
	flags = 0;
	group = 0;
	inUse = 0;
	limits = host->getLimits();
	listenSock = 0;
	localPort[0] = '\0';
	location = 0;
	methodFlags = 0;
	outputCurrentPos = 0;
	requestMimeType = 0;
	responseLength = 0;
	responseMimeType = 0;
	responseHeaders = new MprStringList();
	password = 0;
	remainingChunk = 0;
	remainingContent = -1;
	remoteIpAddr = 0;
	remotePort = -1;
	responseCode = 200;
	scriptName = 0;
	sock = 0;
	socketEventMask = 0;
	state = MPR_HTTP_START;
	terminalHandler = 0;
	timer = 0;
	timeout = INT_MAX;
	timestamp = 0;
	uri = 0;
	user = 0;

#if BLD_FEATURE_RANGES
	range = 0;
	rangeTok = 0;
	sumOfRanges = -1;
	rangeBoundary = 0;
	nextRange = 0;
	outputStart = -1;
	outputEnd = -1;
	inputTotalSize = inputStart = inputEnd = -1;
#endif

	//
	//	We always need the header object just incase the request needs
	//	environment variables. The other environment objects are created 
 	//	only if needed.
	//
	for (i = 0; i < MA_HTTP_OBJ_MAX; i++) {
		variables[i] = mprCreateUndefinedVar();
	}
	variables[MA_HEADERS_OBJ] = mprCreateObjVar("header", MA_HTTP_HASH_SIZE);

#if BLD_FEATURE_SESSION
	session = 0;
	sessionId = 0;
#endif

	//
	//	Input Buffer (for headers and post data). NOTE: We rely on the fact 
	//	that we will never wrap the buffer pointers (it is normally a ring).
	//
	inBuf = new MprBuf(MPR_HTTP_IN_BUFSIZE, MPR_HTTP_IN_BUFSIZE);

	//
	//	Output data streams
	//
	hdrBuf = new MaDataStream("hdr", MPR_HTTP_BUFSIZE, limits->maxHeader);
	dynBuf = new MaDataStream("dyn", MPR_HTTP_BUFSIZE, limits->maxResponseBody);
	docBuf = new MaDataStream("doc", MPR_HTTP_DOC_BUFSIZE,MPR_HTTP_DOC_BUFSIZE);
	writeBuf = dynBuf;

	docBuf->buf.setRefillProc(refillDoc, this);
	outputStreams.insert(hdrBuf);

	hdrBuf->setSize(-1);
	dynBuf->setSize(-1);
	docBuf->setSize(0);

#if BLD_FEATURE_MULTITHREAD
	mutex = new MprMutex();
#endif

#if BLD_FEATURE_KEEP_ALIVE
	if (host->getKeepAlive()) {
		flags |= MPR_HTTP_KEEP_ALIVE;
	}
#endif
}

////////////////////////////////////////////////////////////////////////////////
//
//	Called from the socket callback. Called locked.
//

MaRequest::~MaRequest()
{
	MaDataStream	*dp, *nextDp;

	mprLog(6, tMod, "~Request\n");

	resetEnvObj();

	dp = (MaDataStream*) outputStreams.getFirst();
	while (dp) {
		nextDp = (MaDataStream*) outputStreams.getNext(dp);
		outputStreams.remove(dp);
		dp = nextDp;
	}

	if (file) {
		delete file;
	}

	delete inBuf;
	delete hdrBuf;
	delete dynBuf;
	delete docBuf;

	mprFree(decodedQuery);
	mprFree(etag);
	mprFree(extraPath);
	mprFree(fileName);
	mprFree(group);
	mprFree(password);
	mprFree(remoteIpAddr);
	mprFree(responseMimeType);
	mprFree(scriptName);
	mprFree(uri);
	mprFree(user);

#if BLD_FEATURE_RANGES
	mprFree(range);
	mprFree(rangeBoundary);
	mprFree(rangeTok);
#endif

	if (timer) {
		timer->stop(MPR_TIMEOUT_STOP);
		timer->dispose();
		timer = 0;
	}
	if (responseHeaders) {
		delete responseHeaders;
	}
	if (sock) {
		sock->dispose();
	}

#if BLD_FEATURE_SESSION
	if (sessionId) {
		mprFree(sessionId);
	}
#endif
#if BLD_FEATURE_LOG
	delete tMod;
#endif
#if BLD_FEATURE_MULTITHREAD
	delete mutex;
#endif
}

////////////////////////////////////////////////////////////////////////////////
//
//	For keep-alive we need to be able to service many requests on a single
//	request object
//

void MaRequest::reset()
{
	MaHandler		*hp, *nextHp;
	MaDataStream	*dp, *nextDp;

	mprLog(8, tMod, "reset\n");

	memset((void*) &fileInfo, 0, sizeof(fileInfo));

	if (timer) {
		timer->stop(MPR_TIMEOUT_STOP);
		timer->dispose();
		timer = 0;
	}
	if (etag) {
		mprFree(etag);
		etag = 0;
	}
	if (uri) {
		mprFree(uri);
		uri = 0;
	}
	if (fileName) {
		mprFree(fileName);
		fileName = 0;
	}
	if (decodedQuery) {
		mprFree(decodedQuery);
		decodedQuery = 0;
	}
	if (password) {
		mprFree(password);
		password = 0;
	}
	if (group) {
		mprFree(group);
		group = 0;
	}
	if (user) {
		mprFree(user);
		user = 0;
	}
	if (file) {
		delete file;
		file = 0;
	}
	if (scriptName) {
		mprFree(scriptName);
		scriptName = 0;
	}
	if (extraPath) {
		mprFree(extraPath);
		extraPath = 0;
	}

#if BLD_FEATURE_RANGES
	if (range) {
		mprFree(range);
		range = 0;
	}
	if (rangeTok) {
		mprFree(rangeTok);
		rangeTok = 0;
	}
	if (rangeBoundary) {
		mprFree(rangeBoundary);
		rangeBoundary = 0;
	}

	nextRange = 0;
	sumOfRanges = 0;
	outputStart = -1;
	outputEnd = -1;
	inputTotalSize = inputStart = inputEnd = -1;
#endif

	remainingChunk = 0;
	contentLength = -1;
	remainingContent = -1;

#if BLD_FEATURE_SESSION
	session = 0;
	mprFree(sessionId);
	sessionId = 0;
#endif

	//
	//	NOTE: requestMimeType is not malloced
	//
	requestMimeType = 0;

	if (responseMimeType) {
		mprFree(responseMimeType);
		responseMimeType = 0;
	}

	flags &= (MPR_HTTP_KEEP_ALIVE | MPR_HTTP_SOCKET_EVENT);

	methodFlags = 0;
	state = MPR_HTTP_START;
	responseCode = 200;
	responseLength = 0;
	bytesWritten = 0;
	outputCurrentPos = 0;
	dir = 0;
	location = 0;
	terminalHandler = 0;

	if (responseHeaders) {
		delete responseHeaders;
	}
	responseHeaders = new MprStringList();

	hdrBuf->buf.flush();
	hdrBuf->setSize(-1);
	dynBuf->buf.flush();
	dynBuf->setSize(-1);

	docBuf->buf.flush();
	docBuf->setSize(0);
	docBuf->buf.setRefillProc(refillDoc, this);

	header.reset();

	dp = (MaDataStream*) outputStreams.getFirst();
	while (dp) {
		nextDp = (MaDataStream*) outputStreams.getNext(dp);
		outputStreams.remove(dp);
		dp = nextDp;
	}
	outputStreams.insert(hdrBuf);

	hp = (MaHandler*) handlers.getFirst();
	while (hp) {
		nextHp = (MaHandler*) handlers.getNext(hp);
		handlers.remove(hp);
		hp = nextHp;
	}

	variables[MA_HEADERS_OBJ] = mprCreateObjVar("header", MA_HTTP_HASH_SIZE);
}

////////////////////////////////////////////////////////////////////////////////
//
//	Destroy all variables in the environment  
//

void MaRequest::resetEnvObj()
{
	int		i;

	for (i = 0; i < MA_HTTP_OBJ_MAX; i++) {
		if (i == MA_LOCAL_OBJ || i == MA_GLOBAL_OBJ) {
			continue;
		}
		mprDestroyVar(&variables[i]);
	}

	//
	//	Forcibly destroy the global object. This will destroy all variables
	//	regardless of the number of outstanding references. NOTE: objects that
	//	have deleteProtect asserted will be preserved.
	//
	if (terminalHandler) {
		if (! (flags & MPR_HTTP_OWN_GLOBAL)) {
			mprDestroyAllVars(&variables[MA_GLOBAL_OBJ]);
		}
	}

	for (i = 0; i < MA_HTTP_OBJ_MAX; i++) {
		variables[i] = mprCreateUndefinedVar();
	}
}

////////////////////////////////////////////////////////////////////////////////
//
//	Serialized. Called from select. Return 0 if event is a success
//

int MaRequest::acceptEvent(void *data, MprSocket *s, char *ip, int portNum, 
		MprSocket *lp, int isPoolThread)
{
	int		timeout;

	mprAssert(s);
	mprAssert(lp);
	mprAssert(ip);
	mprAssert(portNum >= 0);

	remotePort = portNum;
	remoteIpAddr = mprStrdup(ip);
	listenSock = lp;
	sock = s;
	flags &= ~MPR_HTTP_REUSE;

#if SECURITY_FLAW
	//
	//	WARNING -- IP addresses can be spoofed!!! Enable this code at your own
	//	risk. There is no secure way to identify the source of a user based 
	//	solely on IP address. A better approach is to create a virtual host
	//	that accepts traffic from the loop-back port (127.0.0.1) and then to
	//	also require digest authentication for that virtual host. 
	//
	if (strcmp(ip, "127.0.0.1") == 0 || strcmp(ip, lp->getIpAddr()) == 0) {
		flags |= MPR_HTTP_LOCAL_REQUEST;
	}
#endif
#if BLD_FEATURE_KEEP_ALIVE
	remainingKeepAlive = host->getMaxKeepAlive();
#endif
#if BLD_FEATURE_LICENSE
	if (host->server->isExpired()) {
		return -1;
	}
#endif

	if (limits->sendBufferSize > 0) {
		sock->setBufSize(limits->sendBufferSize, -1);
	}
	//
	//	If using a named virtual host, we will be running on the default hosts
	//	timeouts
	//
	timeout = host->getTimeout();
	if (timeout > 0) {
		if (!mprGetDebugMode()) {
			mprAssert(timer == 0);
			timeout = host->getTimeout();
			timer = new MprTimer(MPR_HTTP_TIMER_PERIOD, timeoutWrapper, 
				(void*) this);
		}
	}

#if BLD_FEATURE_MULTITHREAD
	if (isPoolThread) {
		//
		//	Go into blocking mode and generate a psudo read event
		//
#if FUTURE
		//
		//	This has DOS issues as we are not yet doing timed-reads
		//
		sock->setBlockingMode(1);
		flags |= MPR_HTTP_BLOCKING;
		return socketEventWrapper((void*)this, sock, MPR_READABLE, 
			isPoolThread);
#endif
	}
#endif

	enableReadEvents(1);

	return 0;
}

////////////////////////////////////////////////////////////////////////////////

static void socketEventWrapper(void *data, MprSocket *sock, int mask, int isPool)
{
	MaRequest	*rq;
	int			written, bytes, loopCount;

	rq = (MaRequest*) data;

	mprLog(7, "%d: socketEvent enter with mask %x\n", sock->getFd(), mask);

	rq->lock();

	if (rq->getFlags() & MPR_HTTP_INCOMPLETE) {
		mprLog(4, "%d: socketEvent: timeout. Finish request.\n", rq->getFd());
		rq->finishRequest(MPR_HTTP_CLOSE);

	} else {

		rq->setFlags(MPR_HTTP_SOCKET_EVENT, ~0);
		if (mask & MPR_WRITEABLE) {
			loopCount = 25;
			do {
				written = rq->writeEvent(MPR_HTTP_CLOSE);
			} while (written > 0 && (isPool || loopCount-- > 0));
		}

		if (mask & MPR_READABLE) {
			loopCount = 25;
			do {
				bytes = rq->readEvent();
			} while (bytes > 0 && (rq->getState() != MPR_HTTP_RUNNING || rq->getRemainingContent() > 0) && 
				(isPool || loopCount-- > 0));
		}
		rq->setFlags(0, ~MPR_HTTP_SOCKET_EVENT);
	}

	//
	//	This will unlock and if instructed above, may actually delete the 
	//	request.
	//
	rq->unlock();
}

////////////////////////////////////////////////////////////////////////////////
//
//	Return TRUE if there is more to be done on this socket and to cause
//	this function to be recalled to process more data.
//

int MaRequest::readEvent()
{
	int		nbytes, len;

	if (remainingContent > 0 && state >= MPR_HTTP_RUN_HANDLERS) {
		len = inBuf->getLinearSpace();
		len = (remainingContent > len) ? len : remainingContent;

	} else {
		if (inBuf->getStart() > inBuf->getBuf()) {
			inBuf->copyDown();
			stats.copyDown++;
		}
		len = inBuf->getLinearSpace();
	}

	if (sock == 0) {
		return -1;
	}

	//	
	//	Len must be non-zero because if our last read filled the buffer, then
	//	one of the actions below must have either depleted the buffer or 
	//	completed the request.
	//
	mprAssert(len > 0);

	//
	//	Read as much as we can
	//
	nbytes = sock->read(inBuf->getEnd(), len);

	mprLog(6, tMod, "%d: readEvent: nbytes %d\n", getFd(), nbytes);

	if (nbytes < 0) {						// Disconnect
		if (state > MPR_HTTP_START && state < MPR_HTTP_DONE) {
			flags |= MPR_HTTP_INCOMPLETE;
			responseCode = MPR_HTTP_COMMS_ERROR;
		} else {
			closeSocket();
		}
		return -1;
		
	} else if (nbytes == 0) {
		if (sock->getEof()) {
			mprLog(6, tMod, "%d: readEvent: EOF\n", getFd());
			if (flags & MPR_HTTP_CONTENT_DATA && remainingContent > 0) {
				if (state & MPR_HTTP_RUNNING) {
					/*
					 *	Do this for post and put
					 */
					if (terminalHandler) {
						terminalHandler->postData(this, 0, -1);
					}
					if (state != MPR_HTTP_DONE) {
						finishRequest(MPR_HTTP_CLOSE);
					}
				}

			} else {
				if (state > MPR_HTTP_START && state < MPR_HTTP_DONE) {
					flags |= MPR_HTTP_INCOMPLETE;
					responseCode = MPR_HTTP_COMMS_ERROR;
					finishRequest(MPR_HTTP_CLOSE);
				} else {
					closeSocket();
				}
			}
		} else {
			;								// No data available currently
		}
		return 0;

	} else {								// Good data
		inBuf->adjustEnd(nbytes);
		inBuf->addNull();

		processRequest();
		return (flags & MPR_HTTP_CONN_CLOSED) ? 0 : 1;
	}
}

////////////////////////////////////////////////////////////////////////////////

void MaRequest::processRequest()
{
	char	*line, *cp, *end;
	int		nbytes;

	mprLog(6, tMod, "%d: processRequest, state %d, inBuf len %d\n", 
		getFd(), state, inBuf->getLength());

	setTimeMark();

	while (state < MPR_HTTP_DONE && inBuf->getLength() > 0) {

		//
		//	Don't process data if we are running handlers and there is no
		//	content data. Otherwise we will eat the next request.
		//
		if (contentLength == 0 && state >= MPR_HTTP_RUN_HANDLERS) {
			break;
		}

		line = inBuf->getStart();

		if (flags & MPR_HTTP_CONTENT_DATA) {
			mprAssert(remainingContent > 0);
			nbytes = min(remainingContent, inBuf->getLength());
			mprAssert(nbytes > 0);

			if (flags & MPR_HTTP_PULL_POST) {
				enableReadEvents(0);

			} else {
				mprLog(5, tMod, 
					"%d: processRequest: contentData %d bytes, remaining %d\n", 
					getFd(), nbytes, remainingContent - nbytes);

				mprAssert(terminalHandler);
				if (mprStrCmpAnyCase(header.contentMimeType, 
						"application/x-www-form-urlencoded") == 0 &&
						contentLength < 10000) {
					mprLog(3, tMod, "postData:\n%s\n", line);
				}

				inBuf->adjustStart(nbytes);
				remainingContent -= nbytes;
				if (remainingContent <= 0) {
					remainingContent = 0;
					enableReadEvents(0);
				}

				terminalHandler->postData(this, line, nbytes);
				inBuf->resetIfEmpty();
			}

			return;

		} else {
			end = inBuf->getEnd();
			for (cp = line; cp != end && *cp != '\n'; ) {
				cp++;
			}
			if (*cp == '\0') {
				if (inBuf->getSpace() <= 0) {
					requestError(400, "Header line too long");
					finishRequest();
				}
				return;
			}
			*cp = '\0';
			if (cp[-1] == '\r') {
				nbytes = cp - line;
				*--cp = '\0';
			} else {
				nbytes = cp - line;
			}
			inBuf->adjustStart(nbytes + 1);
			if (inBuf->getLength() >= (limits->maxHeader - 1)) {
				requestError(400, "Bad MPR_HTTP request");
				finishRequest();
				return;
			}
		}
		inBuf->resetIfEmpty();

		switch(state) {
		case MPR_HTTP_START:
			if (nbytes == 0 || *line == '\0') {
				break;
			}
			timeout = host->getTimeout();
			if (parseFirstLine(line) < 0) {
				return;
			}
			state = MPR_HTTP_HEADER;
			break;
		
		case MPR_HTTP_HEADER:
			if (nbytes > 1) {				// Always trailing newlines
				if (parseHeader(line) < 0) {
					mprLog(3, tMod, 
						"%d: processMaRequest: can't parse header\n", getFd());
					return;
				}

			} else {
				//
				//	Blank line means end of headers 
				//
#if BLD_DEBUG
				mprLog(3, MPR_RAW, tMod, "\r\n");
#endif
				if (setupHandlers() != 0) {
					break;
				}
				if (flags & (MPR_HTTP_POST_REQUEST | MPR_HTTP_PUT_REQUEST)) {
					if (contentLength < 0) {
						requestError(400, "Missing content length");
						finishRequest();
						break;
					}
					//
					//	Keep accepting read events
					// 
					flags |= MPR_HTTP_CONTENT_DATA;

				} else {
					enableReadEvents(0);
				}
				state = MPR_HTTP_RUN_HANDLERS;
				runHandlers();
			}
			break;

		default:
			mprLog(3, tMod, "%d: processMaRequest: bad state\n", getFd());
			requestError(404, "Bad state");
			finishRequest();
			return;
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
//
//	Parse the first line of a http request
//

int MaRequest::parseFirstLine(char *line)
{
	char		*tok;
	int			len;

	mprAssert(line && *line);

	header.buf = mprStrdup(line);
	header.firstLine = mprStrdup(line);

	mprLog(3, tMod, "%d: Request from %s:%d to %s:%d\n", getFd(), 
		remoteIpAddr, remotePort,
		listenSock->getIpAddr(), listenSock->getPort());

	mprLog(3, tMod, "%d: parseFirstLine: \n<<<<<<<<<<<<<<\n%s\n", 
		getFd(), header.buf);

	header.method = mprStrTok(header.buf, " \t", &tok);
	if (header.method == 0 || *header.method == '\0') {
		requestError(400, "Bad MPR_HTTP request");
		finishRequest();
		return MPR_ERR_BAD_STATE;
	}

	if (strcmp(header.method, "GET") == 0) {
		flags |= MPR_HTTP_GET_REQUEST;
		methodFlags |= MPR_HANDLER_GET;
		
	} else if (strcmp(header.method, "POST") == 0) {
		flags |= MPR_HTTP_POST_REQUEST;
		methodFlags |= MPR_HANDLER_POST;

	} else if (strcmp(header.method, "HEAD") == 0) {
		flags |= MPR_HTTP_HEAD_REQUEST;
		methodFlags |= MPR_HANDLER_HEAD;

	} else if (strcmp(header.method, "OPTIONS") == 0) {
		flags |= MPR_HTTP_OPTIONS_REQUEST;
		methodFlags |= MPR_HANDLER_OPTIONS;

	} else if (strcmp(header.method, "PUT") == 0) {
		flags |= MPR_HTTP_PUT_REQUEST;
		methodFlags |= MPR_HANDLER_PUT;

	} else if (strcmp(header.method, "DELETE") == 0) {
		flags |= MPR_HTTP_DELETE_REQUEST;
		methodFlags |= MPR_HANDLER_DELETE;

	} else if (strcmp(header.method, "TRACE") == 0) {
		if (host->getFlags() & MPR_HTTP_NO_TRACE) {
			requestError(400, "TRACE request is disabled");
		} else {
			flags |= MPR_HTTP_TRACE_REQUEST;
			methodFlags |= MPR_HANDLER_TRACE;
		}

	} else {
		header.method = "UNKNOWN_METHOD";
		requestError(400, "Bad HTTP request");
		finishRequest();
		return MPR_ERR_BAD_STATE;
	}

	header.uri = mprStrTok(0, " \t\n", &tok);
	if (header.uri == 0 || *header.uri == '\0') {
		requestError(400, "Bad MPR_HTTP request");
		finishRequest();
		return MPR_ERR_BAD_STATE;
	}
	if (strlen(header.uri) >= (MPR_HTTP_MAX_URL - 1)) {
		requestError(400, "Bad MPR_HTTP request");
		finishRequest();
		return MPR_ERR_BAD_STATE;
	}

	//
	//	We parse (tokenize) the request uri first. Then we decode and lastly
	//	we validate the URI path portion. This allows URLs to have '?' in 
	//	the URL. We descape and validate insitu.
	//
	if (url.parse(header.uri) < 0) {
		requestError(400, "Bad URL format");
		finishRequest();
		return MPR_ERR_BAD_STATE;
	}

	uri = mprStrdup(url.uri);
	len = strlen(uri);

	if (maUrlDecode(uri, len + 1, uri, 1, 0) == 0) {
		requestError(400, "Bad URL escape");
		finishRequest();
		return MPR_ERR_BAD_STATE;
	}

#if WIN
	//
	//	URLs are case insensitive. Map to lower case internally. Must do this 
	//	after the decode above but before the validation below.
	//
	//	FUTURE -- problematic as we've now lost case. header.uri preserves
	//	the original uri, but it would be better if we made a fileUri and 
	//	used that for mapping to storage.
	mprStrLower(uri);
#endif

	if (maValidateUri(uri) == 0) {
		requestError(400, "URL does not validate");
		finishRequest();
		return MPR_ERR_BAD_STATE;
	}

	//	FUTURE - this should be moved to after alias matching
	if (url.ext == 0 || 
			(requestMimeType = host->lookupMimeType(url.ext)) == 0) {
		requestMimeType = "text/plain";
	}
	responseMimeType = mprStrdup(requestMimeType);

	header.proto = mprStrTok(0, " \t\n", &tok);
	if (header.proto == 0 || 
			(strcmp(header.proto, "HTTP/1.0") != 0 && 
			 strcmp(header.proto, "HTTP/1.1") != 0)) {
		requestError(400, "Unsupported protocol");
		finishRequest();
		return MPR_ERR_BAD_STATE;
	}
	if (strcmp(header.proto, "HTTP/1.0") == 0) {
		flags &= ~MPR_HTTP_CHUNKED;

		/*
		 * 	Turn off keep-alive unless explicitly turned on by the headers
		 */
		flags &= ~MPR_HTTP_KEEP_ALIVE;
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Return non-zero if the request is already handled (error or redirect)
//

int MaRequest::matchHandlers()
{
	MaHandler	*hp;
	char		path[MPR_MAX_FNAME];

	//
	//	FUTURE -- should we process aliases first.
	//
	//	matchHandlers may set location, extraPath and scriptName as 
	//	a side-effect.
	//
	terminalHandler = host->matchHandlers(this);
	if (terminalHandler == 0) {
		return 1;
	}
	if (terminalHandler->getFlags() & MPR_HANDLER_OWN_GLOBAL) {
		flags |= MPR_HTTP_OWN_GLOBAL;
	}


	hp = (MaHandler*) handlers.getFirst();
	while (hp) {
		if (hp->getFlags() & MPR_HANDLER_NEED_ENV) {
			flags |= MPR_HTTP_CREATE_ENV;
			break;
		}
		hp = (MaHandler*) handlers.getNext(hp);
	}

	//
	//	Now map the URI to an actual file name. This may set dir, fileName and
	//	may finish the request by error or redirect. mapToStorage may delete
	//	the handlers or may call finishRequest. We need to detect this and
	//	exit or rematch accordingly if required.
	//
	if (host->mapToStorage(this, path, sizeof(path), uri,
			MPR_HTTP_REDIRECT | MPR_HTTP_REMATCH) < 0) {
		requestError(404, "Can't map URL to storage");
		finishRequest();
		return 1;
	}
	
	if (state == MPR_HTTP_START || state == MPR_HTTP_DONE) {
		//
		//	Looks like we've done a redirect. State will be START if using 
		//	keep-alive, otherwise DONE if the socket has been closed.
		//
		return 1;
	}

	if (handlers.getFirst() == 0) {
		//
		//	If a alias has called deleteHandlers, we need to rematch
		//
		terminalHandler = host->matchHandlers(this);
		if (terminalHandler == 0) {
			return 1;
		}
		hp = (MaHandler*) handlers.getFirst();
		while (hp) {
			if (hp->getFlags() & MPR_HANDLER_NEED_ENV) {
				flags |= MPR_HTTP_CREATE_ENV;
				break;
			}
			hp = (MaHandler*) handlers.getNext(hp);
		}
	}

	if ( !(terminalHandler->getFlags() & MPR_HANDLER_MAP_VIRTUAL) ||
		  (terminalHandler->getFlags() & MPR_HANDLER_NEED_FILENAME)) {
		if (setFileName(path) < 0) {
			//
			//	setFileName will return an error to the user
			//
			return 1;
		}
	}

	if (flags & MPR_HTTP_CREATE_ENV) {
		createEnvironment();
	}

	//
	//	Not standard, but lots of servers define this. CGI/PHP needs it.
	//
	setVar(MA_REQUEST_OBJ, "SCRIPT_FILENAME", path);

	//
	//	We will always get a dir match as a Directory object is created for
	//	the document root
	//
	if (dir == 0) {
		dir = host->findBestDir(path);
		mprAssert(dir);
		if (dir == 0) {
			requestError(404, "Can't map URL to directory");
			finishRequest();
			return 1;
		}
	}

	//
	//	Must not set PATH_TRANSLATED to empty string. CGI/PHP will try to 
	//	open it.
	//
	if (extraPath && host->mapToStorage(this, path, 
			sizeof(path), extraPath, 0) == 0) {
		setVar(MA_REQUEST_OBJ, "PATH_TRANSLATED", path);
	} else {
		setVar(MA_REQUEST_OBJ, "PATH_TRANSLATED", 0);
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

int MaRequest::parseHeader(char *line)
{
	MaHost	*hp;
	char	*browser, *key, *value, *tok, *cp;

#if BLD_FEATURE_LOG
	mprLog(3, MPR_RAW, tMod, "%s\r\n", line);
#endif

	browser = 0;

	if ((key = mprStrTok(line, ": \t\n", &tok)) == 0) {
		requestError(400, "Bad header format");
		finishRequest();
		return MPR_ERR_BAD_ARGS;
	}

	if ((value = mprStrTok(0, "\n", &tok)) == 0) {
		value = "";
	}
	while (isspace(*value)) {
		value++;
	}
	mprStrUpper(key);
	for (cp = key; *cp; cp++) {
		if (*cp == '-') {
			*cp = '_';
		}
	}

	if (strspn(key, "%<>/\\") > 0) {
		requestError(400, "Bad header key value");
		finishRequest();
		return MPR_ERR_BAD_ARGS;
	}

	//
	//	We don't yet know if the request will require environment variables
	//	(flags & MPR_HTTP_CREATE_ENV), so we must preserve these headers just 
	//	in case.
	//	FUTURE: we could match handler first based on URI and just save the 
	//	header and then parse it second.
	//
	mprSetPropertyValue(&variables[MA_HEADERS_OBJ], key, 
		mprCreateStringVar(value, 0));

	//
	//	FUTURE OPT -- switch on first char.
	//
	if (strcmp(key, "USER_AGENT") == 0) {
		mprFree(header.userAgent);
		header.userAgent = mprStrdup(value);

	} else if (strcmp(key, "AUTHORIZATION") == 0) {
		mprFree(header.authType);
		header.authType = mprStrdup(mprStrTok(value, " \t", &tok));
		header.authDetails = mprStrdup(tok);

	} else if (strcmp(key, "CONTENT_LENGTH") == 0) {
		contentLength = atoi(value);
		if (contentLength < 0) {
			requestError(400, "Bad content length");
			flags &= ~MPR_HTTP_KEEP_ALIVE;
			finishRequest();
			return MPR_ERR_BAD_ARGS;
		}
 		if (contentLength >= limits->maxBody) {
			requestError(413, "Request content body is too big");
			flags &= ~MPR_HTTP_KEEP_ALIVE;
			finishRequest();
			return MPR_ERR_BAD_ARGS;
		}
		if (contentLength > 0) {
			flags |= MPR_HTTP_LENGTH;
		} else {
			contentLength = 0;
		}
		remainingContent = contentLength;

	} else if (strcmp(key, "CONTENT_TYPE") == 0) {
		header.contentMimeType = mprStrdup(value);
	
#if BLD_FEATURE_COOKIE || BLD_FEATURE_SESSION
	} else if (strcmp(key, "COOKIE") == 0) {
		flags |= MPR_HTTP_COOKIE;
		mprFree(header.cookie);
		header.cookie = mprStrdup(value);
		mprSetPropertyValue(&variables[MA_HEADERS_OBJ], "HTTP_COOKIE", 
			mprCreateStringVar(value, 0));
#if BLD_FEATURE_SESSION
		{
			char 	*sessionCookie;
			if ((sessionCookie = strstr(value, MA_HTTP_SESSION_PREFIX)) != 0) {
				mprFree(sessionId);
				getCrackedCookie(sessionCookie, 0, &sessionId, 0);
				session = host->lookupSession(sessionId);
				if (session == 0) {
					mprFree(sessionId);
					sessionId = 0;
				}
			}
		}
#endif
#endif

#if BLD_FEATURE_KEEP_ALIVE
	} else if (strcmp(key, "CONNECTION") == 0) {
		mprStrUpper(value);
		if (strcmp(value, "KEEP-ALIVE") == 0) {
			flags |= MPR_HTTP_KEEP_ALIVE;
		} else if (strcmp(value, "CLOSE") == 0) {
			flags &= ~MPR_HTTP_KEEP_ALIVE;
		}
#endif
		if (host->getHttpVersion() == MPR_HTTP_1_0) {
			flags &= ~MPR_HTTP_KEEP_ALIVE;
		}
#if BLD_FEATURE_KEEP_ALIVE
		if (!host->getKeepAlive()) {
			flags &= ~MPR_HTTP_KEEP_ALIVE;
		}
#endif

	} else if (strcmp(key, "HOST") == 0) {
		mprFree(header.host);
		header.host = mprStrdup(value);
		if (address->isNamedVhost()) {
			hp = address->findHost(value);
			if (hp == 0) {
				requestError(404, "No host to serve request. Searching for %s", value);
				finishRequest();
				return MPR_ERR_BAD_ARGS;
			}
			//
			//	Reassign this request to a new host
			//
			host->removeRequest(this);
			host = hp;
			host->insertRequest(this);
		}

#if BLD_FEATURE_IF_MODIFIED
	} else if ((strcmp(key, "IF_MODIFIED_SINCE") == 0)
			   || (strcmp(key, "IF_UNMODIFIED_SINCE") == 0)) {
 		char	*cmd, *cp;
		uint	newDate = 0;
		bool	ifModified = (key[3] == 'M');

 		if ((cp = strchr(value, ';')) != 0) {
 			*cp = '\0';
 		}
 
		cmd = mprStrdup(value);
		newDate = maDateParse(cmd);
		mprFree(cmd);

		if (newDate) {
			requestModified.setDate(newDate, ifModified);
			flags |= MPR_HTTP_IF_MODIFIED;
 		}

	} else if ((strcmp(key, "IF_MATCH") == 0)
			   || (strcmp(key, "IF_NONE_MATCH") == 0)) {
		char	*word, *tok;
		bool	ifMatch = key[3] == 'M';

		if ((tok = strchr(value, ';')) != 0) {
			*tok = '\0';
		}

		requestMatch.setMatch(ifMatch);
		flags |= MPR_HTTP_IF_MODIFIED;

		value = mprStrdup(value);
		word = mprStrTok(value, " ,", &tok);
		while (word) {
			requestMatch.addEtag(word);
			word = mprStrTok(0, " ,", &tok);
		}
		mprFree(value);
#endif
#if BLD_FEATURE_RANGES
#if BLD_FEATURE_IF_MODIFIED
	} else if (strcmp(key, "IF_RANGE") == 0) {
		char	*word, *tok;

		if ((tok = strchr(value, ';')) != 0) {
			*tok = '\0';
		}

		requestMatch.setMatch(1);
		flags |= MPR_HTTP_IF_MODIFIED;

		value = mprStrdup(value);
		word = mprStrTok(value, " ,", &tok);
		while (word) {
			requestMatch.addEtag(word);
			word = mprStrTok(0, " ,", &tok);
		}
		mprFree(value);
#endif

	} else if (strcmp(key, "RANGE") == 0) {
		char	*sp;
		int		rc;
		/*
		 *	Format is:  Range: bytes=n1-n2,n3-n4,...
		 *	Where n1 is first byte pos and n2 is last byte pos
		 */
		if ((sp = strchr(value, '=')) != 0) {

			range = mprStrdup(++sp);

			/*
			 *	Compute the sum of ranges now. Also validate ranges somewhat.
			 */
			nextRange = rangeTok = mprStrdup(range);
			while ((rc = getNextRange(&nextRange)) > 0) {
				if (outputStart >= 0 && outputEnd >= 0) {
					sumOfRanges += (outputEnd - outputStart) + 1;
				}
			}
			mprFree(rangeTok);
			if (rc < 0) {
				requestError(MPR_HTTP_RANGE_NOT_SATISFIABLE, "Bad range");
				finishRequest();
				return MPR_ERR_BAD_ARGS;
			}

			/*
			 *	OPT.
			 *	Re-parse the range header to get the first range. This sets
			 *	outputStart.  Set MULTI if there are multiple ranges.
			 */
			nextRange = rangeTok = mprStrdup(range);
			if (getNextRange(&nextRange) > 0 && nextRange) {
				flags |= MPR_HTTP_OUTPUT_MULTI;
			}
		} else {
			//	FUTURE: could warn about bad range.
		}

	} else if (strcmp(key, "CONTENT_RANGE") == 0) {
		/*
		 *	Format is:  Content-Range: bytes n1-n2/length
		 *	Where n1 is first byte pos and n2 is last byte pos
		 */
		char *sp = value;

		while (*sp && !isdigit(*sp)) {
			sp++;
		}
		if (*sp) {
			inputStart = mprAtoi(sp, 10);

			if ((sp = strchr(sp, '-')) != 0) {
				inputEnd = mprAtoi(++sp, 10);
			}
			if ((sp = strchr(sp, '/')) != 0) {
				/*
				 *	Note this is not the content length transmitted, but the
				 *	original size of the input of which the client is
				 *	transmitting only a portion.
			 	 */
				inputTotalSize = mprAtoi(++sp, 10);
			}
		}
		if (inputStart < 0 || inputEnd < 0 || inputTotalSize < 0 ||
				inputEnd <= inputStart) {
			requestError(MPR_HTTP_RANGE_NOT_SATISFIABLE, 
				"Bad content range");
			finishRequest();
			return MPR_ERR_BAD_ARGS;
		}
		flags |= MPR_HTTP_INPUT_RANGE;

#endif
	} else if (strcmp(key, "X_APPWEB_CHUNK_TRANSFER") == 0) {
		/*
		 *	Control use of chunking
		 */
		mprStrUpper(value);
		if (strcmp(value, "OFF") == 0) {
			flags |= MPR_HTTP_NO_CHUNKING;
		} else {
			flags &= ~MPR_HTTP_NO_CHUNKING;
		}
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
// 	Write data to the client.
//
//	Called locally and also by the socket handler. Return the number of bytes
//	writte if successful. Otherwise return -1. Return 0 if the socket is full.
//	If "completeRequired" is specified, "finishRequest" will be called when 
//	there is no more write data.
//

int MaRequest::writeEvent(bool completeRequired)
{
	MaDataStream	*dp;
	MprBuf			*buf;
	int				totalBytes, bytes, len;

	mprLog(7, tMod, "%d: writeEvent completeRequired %d\n", getFd(), 
		completeRequired);
	setTimeMark();

	len = 0;
	totalBytes = 0;

	dp = (MaDataStream*) outputStreams.getFirst();

	//
	//	Loop over all output data stream buffers, outputting as much as the
	//	socket will accept
	//
	while (dp) {
		buf = &dp->buf;
		len = buf->getLength();

		if (len == 0) {
			if (buf->getRefillProc()) {
				buf->resetIfEmpty();
				buf->refill();
				len = buf->getLength();
				if (len == 0) {
					dp->setSize(0);
				}
			}
			if (len == 0) {
				dp = (MaDataStream*) outputStreams.getNext(dp);
				continue;
			}
		}
		bytes = outputBuffer(dp, buf->getLinearData());
		if (bytes < 0) {
			if (completeRequired) {
				finishRequest();
			}
			return bytes;

		} else if (bytes == 0) {
			break;
		}
		totalBytes += bytes;
	}

	if (dp == 0) {
		mprLog(8, tMod, "%d: writeEvent: end of data streams\n", getFd());
		enableWriteEvents(0);
		if (completeRequired) {
			finishRequest();
		}
		return 0;
	}
	return totalBytes;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Write the contents of an output data stream buffer to the socket
//	Return < 0 on errors. Return 0 if the socket is full.
//
//	FUTURE. Rename outputXXX, writeXXX to be more uniform.

int MaRequest::outputBuffer(MaDataStream *dp, int toWrite)
{
	MprBuf		*buf;
	char		line[MPR_MAX_STRING];
	int			thisWrite, bytes, totalBytes;

	buf = &dp->buf;
	totalBytes = 0;

	mprLog(7, tMod, "%d: outputBuffer: using stream %s len %d\n", getFd(), dp->getName(), toWrite);

	if (dp == hdrBuf) {
		mprLog(3, tMod, "%d: response: >>>>>>>>>>>>\n%s", getFd(), buf->getStart());
	}

	if (sock == 0) {
		return totalBytes;
	}

	while (toWrite > 0) {
		/*
		 *	If doing chunked output, see if we need to output a new chunk header
		 */
		if (flags & MPR_HTTP_CHUNKED && dp != hdrBuf) {
			if (remainingChunk <= 0) {
				remainingChunk = toWrite;

				/*
				 *	Begin with a \r\n. Note: for chunked transfers,
				 *	writeHeaders will not append a trailing \r\n
				 */
				bytes = mprSprintf(line, sizeof(line), "\r\n%x; chunk length %d\r\n", remainingChunk, remainingChunk);
				if (blockingWrite(line, bytes) != bytes) {
					flags |= MPR_HTTP_INCOMPLETE;
					responseCode = MPR_HTTP_COMMS_ERROR;
					return -1;
				}
#if BLD_DEBUG
				mprLog(4, MPR_RAW, tMod, "%s", line); 
#endif
			}
			toWrite = remainingChunk;
		}

		thisWrite = toWrite;

#if BLD_FEATURE_RANGES
		/*
		 *	Output only the required ranges of bytes
		 */
		if (dp != hdrBuf && range) {
			int		restOfRange, bytes, discardBytes;

			discardBytes = 0;

			/*
			 *	If outputStart is negative, it means the range is: -N, ie. 
			 *	last N bytes. We need to reverse out the +1 that is applied
			 *	in the non-negative case when parsing ranges.
			 */
			if (outputStart < 0 && dp->getSize() > 0) {
				outputStart = dp->getSize() - outputEnd + 1;
				if (outputStart < 0) {
					outputStart = 0;
				}
				outputEnd = dp->getSize();
			}

			/*
			 *	If outputEnd is negative, it means the range is: N-, ie. 
			 *	skip first N bytes. 
			 */
			if (outputEnd < 0 && dp->getSize() > 0) {
				outputEnd = dp->getSize();
			}

			if (outputStart < 0 || outputEnd < 0) {
				//	Beyond the end of the range
				discardBytes = thisWrite;

			} else if (outputCurrentPos < outputStart) {
				/*
				 *	Not yet reached the start of the range
				 */
				if ((outputCurrentPos + thisWrite) < outputStart) {
					discardBytes = thisWrite;
				} else {
					discardBytes = outputStart - outputCurrentPos;
				}

			} else if (outputCurrentPos < outputEnd) {
				/*
				 *	Output a range header if this is the start of a new chunk
				 */
				if (flags & MPR_HTTP_OUTPUT_MULTI &&
						outputCurrentPos == outputStart) {
					bytes = mprSprintf(line, sizeof(line), 
						"\r\n--%s\r\n"
						"Content-type: %s\r\n"
						"Content-range: bytes %d-%d/%d\r\n\r\n", 
						rangeBoundary,
						(responseMimeType) ? responseMimeType : "text/html",
						outputStart, outputEnd - 1, responseLength);

					if (blockingWrite(line, bytes) != bytes) {
						flags |= MPR_HTTP_INCOMPLETE;
						responseCode = MPR_HTTP_COMMS_ERROR;
						return -1;
					}
				}

				/*
				 *	Now in range. Get the max len to output in this range.
			 	 */
				restOfRange = outputEnd - outputCurrentPos;
				thisWrite = min(thisWrite, restOfRange);

			} else if (outputCurrentPos >= outputEnd) {
				
				if (getNextRange(&nextRange) <= 0) {
	
					/*
					 *	No more ranges. Write the trailing boundary if doing
					 *	multipart ranges then empty the doc buffer of residual
					 *	data.
					 */
					if (flags & MPR_HTTP_OUTPUT_MULTI) {
						bytes = mprSprintf(line, sizeof(line), 
							"\r\n--%s--\r\n", rangeBoundary);
						if (blockingWrite(line, bytes) != bytes) {
							flags |= MPR_HTTP_INCOMPLETE;
							responseCode = MPR_HTTP_COMMS_ERROR;
							return -1;
						}
					}

					buf->flush();
					buf->setRefillProc(0, 0);
					return totalBytes;
				}
				continue;
			}

			if (discardBytes > 0) {
				buf->adjustStart(discardBytes);
				outputCurrentPos += discardBytes;
				toWrite -= discardBytes;
				//	Pretend we actually wrote them as we want to return 
				//	non-zero.
				totalBytes += discardBytes;
				continue;
			}
		}

		if (thisWrite == 0) {
			return totalBytes;
		}
#endif

		mprAssert(thisWrite > 0);

		bytes = sock->write(buf->getStart(), thisWrite);

		if (bytes < 0) {
			flags |= MPR_HTTP_INCOMPLETE;
			responseCode = MPR_HTTP_COMMS_ERROR;
			return bytes;

		} else if (bytes == 0) {
			//	Socket can't accept more data
			return totalBytes;

#if BLD_DEBUG
		} else if (dp != hdrBuf) {
			//
			//	Trace data actually written
			//
			mprLog(4, MPR_RAW, tMod, "   Write data %d bytes, written %d\n",
				thisWrite, bytes);
#if UNUSED
			if (strcmp(responseMimeType, "text/html") == 0) {
				mprLog(4, MPR_RAW, tMod, "DATA =>\n%s", 
				buf->getStart());
			}
#endif
#endif// BLD_DEBUG
		}

		buf->adjustStart(bytes);
		bytesWritten += bytes;
		totalBytes += bytes;
		toWrite -= bytes;

#if BLD_FEATURE_RANGES
		if (dp != hdrBuf && range) {
			outputCurrentPos += bytes;
		}
#endif

		if (remainingChunk > 0) {
			remainingChunk -= bytes;
		}
	}

	return totalBytes;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Flush output the the client.
//
//	Return 0 if the flush is finished, < 0 on errors and > 0 flushing is 
//	continuing in the background.
//

int MaRequest::flushOutput(bool background, bool completeRequired)
{
	char	allowBuf[80];
	int		rc, handlerFlags;

	mprAssert(state != MPR_HTTP_DONE);
	if (state == MPR_HTTP_DONE) {
		return MPR_ERR_BAD_STATE;
	}

	if (flags & (MPR_HTTP_TRACE_REQUEST | MPR_HTTP_OPTIONS_REQUEST)) {

		docBuf->buf.flush();
		docBuf->setSize(0);
		dynBuf->buf.flush();
		dynBuf->setSize(-1);

		if (flags & MPR_HTTP_TRACE_REQUEST) {
			insertDataStream(dynBuf);
			dynBuf->buf.put(header.firstLine);
			dynBuf->buf.put("\r\n");

		} else if (flags & MPR_HTTP_OPTIONS_REQUEST) {
			if (terminalHandler == 0) {
				mprSprintf(allowBuf, sizeof(allowBuf), "Allow: OPTIONS,TRACE");
			} else {
				handlerFlags = terminalHandler->getFlags();
				mprSprintf(allowBuf, sizeof(allowBuf), 
					"Allow: OPTIONS,TRACE%s%s%s", 
					(handlerFlags & MPR_HANDLER_GET) ? ",GET" : "",
					(handlerFlags & MPR_HANDLER_HEAD) ? ",HEAD" : "",
					(handlerFlags & MPR_HANDLER_POST) ? ",POST" : "",
					(handlerFlags & MPR_HANDLER_PUT) ? ",PUT" : "",
					(handlerFlags & MPR_HANDLER_DELETE) ? ",DELETE" : "");
			}
			setHeader(allowBuf);
		}
	}

	mprLog(5, tMod, "%d: flushOutput: background %d\n", getFd(), background);

	if (!(flags & MPR_HTTP_HEADER_WRITTEN)) {
		writeHeaders();
	}

	if (flags & (MPR_HTTP_HEAD_REQUEST)) {
		//
		//	Now that we've written the headers, we must not write any 
		//	content body.
		//
		cancelOutput();
	}

	if (hdrBuf->buf.getLength() > 0 || getOutputStreamLength(1) > 0) {
		if (background) {
			rc = backgroundFlush();
		} else {
			rc = foregroundFlush();
		}
		if (completeRequired && rc <= 0) {
			mprAssert(state != MPR_HTTP_DONE);
			mprAssert(sock != 0);
			finishRequest();
		}
		return rc;

	} else {
		if (completeRequired) {
			mprAssert(state != MPR_HTTP_DONE);
			mprAssert(sock != 0);
			finishRequest();
		}
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Return 0 if the flush is finished, < 0 on errors and > 0 flushing is 
//	continuing in the background.

int MaRequest::backgroundFlush()
{
	int		written;

	mprLog(5, tMod, "%d: backgroundFlush\n", getFd());

	if ((written = writeEvent(0)) < 0) {
		mprLog(6, tMod, "%d: backgroundFlush -- writeEvent error\n", getFd());
		return MPR_ERR_CANT_WRITE;
	}

	//
	//	Initiate a background flush if not already done and more data to go
	//
	if (written >= 0) {
		mprLog(5, tMod, 
			"%d: flushOutput: start background flush for %d bytes\n", 
			getFd(), getOutputStreamLength(1));
		enableWriteEvents(1);
		return 1;
	}

	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Return 0 if the flush is finished, < 0 on errors.
//

int MaRequest::foregroundFlush()
{
	bool	oldMode;
	int		alreadySetMode;

	mprLog(6, tMod, "%d: foregroundFlush\n", getFd());
	
	if (sock == 0) {
		return 0;
	}

	//
	//	Foreground (blocking) flush
	//
	oldMode = sock->getBlockingMode();
	alreadySetMode = 0;

	while (1) {
		if (writeEvent(0) < 0) {
			return MPR_ERR_CANT_WRITE;
		}
		if (getOutputStreamLength(1) == 0) {
			break;
		}
		if (!alreadySetMode) {
			sock->setBlockingMode(1);
			alreadySetMode++;
		}
	}

	sock->setBlockingMode(oldMode);
	enableWriteEvents(0);
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
//	For handlers that need to read their own post data on demand. This routine
//	will attempt to read and buffer ahead of the callers demands up to the
//	remaining content length. This routine will block and so should only really
//	be used in a multi-threaded server.
//
//	Returns the number of bytes read or < 0 if an error occurs while reading.
//

int MaRequest::readPostData(char *buf, int bufsize)
{
	bool	oldMode;
	int		sofar, thisRead, nbytes;

	if (state != MPR_HTTP_RUNNING) {
		return MPR_ERR_BAD_STATE;
	}
	mprAssert(sock != 0);

	if (! (flags & MPR_HTTP_PULL_POST)) {
		mprAssert(flags & MPR_HTTP_PULL_POST);
		return MPR_ERR_BAD_STATE;
	}
	if (sock == 0) {
		return MPR_ERR_BAD_STATE;
	}

	for (sofar = 0; remainingContent > 0 && sofar < bufsize; ) {

		if (inBuf->getLength() == 0) {
			inBuf->resetIfEmpty();
			thisRead = min(inBuf->getLinearSpace(), remainingContent);

			//
			//	Do a blocking read. Don't ever read more than the 
			//	remaining content length. FUTURE -- need a timed read to 
			//	enable this for single-threaded servers.
			//	NOTE: under windows, this call may fail if we are in an I/O
			//	callback using AsyncSelect. So we may be non-blocking and we
			//	must be careful when nbytes == 0.
			//
			oldMode = sock->getBlockingMode();
			sock->setBlockingMode(1);
			nbytes = sock->read(inBuf->getEnd(), thisRead);
			sock->setBlockingMode(oldMode);

			if (nbytes < 0) {
				return nbytes;

			} else if (nbytes == 0) {
				if (sock->getEof()) {
					return 0;
				}
#if WIN
				//	See note above about AsyncSelect
				mprSleep(1);
#endif
				continue;

			} else if (nbytes > 0) {
				inBuf->adjustEnd(nbytes);
				inBuf->addNull();
			}
		}

		nbytes = min(remainingContent, inBuf->getLength());
		nbytes = min(nbytes, (bufsize - sofar));

		memcpy(&buf[sofar], inBuf->getStart(), nbytes);
		inBuf->adjustStart(nbytes);
		remainingContent -= nbytes;
		sofar += nbytes;
	}

	//
	//	NULL terminate just to make debugging easier
	//
	if (remainingContent == 0 && sofar < bufsize) {
		buf[sofar] = 0;
		if (mprStrCmpAnyCase(header.contentMimeType, 
				"application/x-www-form-urlencoded") == 0) {
			mprLog(3, tMod, 
				"%d: readPostData: ask %d bytes, got %d, remaining %d\n%s\n", 
				getFd(), bufsize, sofar, remainingContent, buf);
		}
	}
	return sofar;
}

////////////////////////////////////////////////////////////////////////////////

void MaRequest::setPullPost()
{
	flags |= MPR_HTTP_PULL_POST;
}

////////////////////////////////////////////////////////////////////////////////

int MaRequest::writeBlk(MaDataStream *dp, char *buf, int len)
{
	int		 rc, toWrite;
	
	rc = 0;
	toWrite = len;

	while (toWrite > 0) {
		dp->buf.resetIfEmpty();
		rc = dp->buf.put((uchar*) buf, toWrite);
		dp->buf.addNull();
		if (rc < 0) {
			return rc;
		}

		if (rc != toWrite) {
			//
			//	The output buffer can't accept all the data so we won't be
			//	able to calculate the content length. Try chunked output if
			//	it is enabled (HTTP/1.1) and we are not doing a ranged
			//	transfer. Otherwise, set a flag to indicate that we have 
			//	flushed the output.
			//
			if (host->useChunking() && !(flags & MPR_HTTP_NO_CHUNKING)) {
#if BLD_FEATURE_RANGES
				if (range == 0)
#endif
					flags |= MPR_HTTP_CHUNKED;
			}
			if (! (flags & MPR_HTTP_CHUNKED)) {
				flags |= MPR_HTTP_FLUSHED;
			}
			if (flushOutput(MPR_HTTP_FOREGROUND_FLUSH, 0) < 0) {
				return MPR_ERR_CANT_WRITE;
			}

			//
			//	flushOutput will remove a stream when it is empty. Must re-add it now.
			//
			if (dp->head == 0) {
				insertDataStream(dp);
			}
		}
		buf += rc;
		toWrite -= rc;
	}
	return len;
}

////////////////////////////////////////////////////////////////////////////////

int MaRequest::write(char *buf, int size)
{
	return writeBlk(writeBuf, buf, size);
}

////////////////////////////////////////////////////////////////////////////////

int MaRequest::write(char *s)
{
	return write(s, strlen(s));
}

////////////////////////////////////////////////////////////////////////////////

int MaRequest::writeFmt(char *fmt, ...)
{
	va_list		vargs;
	char		buf[MPR_HTTP_BUFSIZE];
	int			len;
	
	va_start(vargs, fmt);

	len = mprVsprintf(buf, MPR_HTTP_BUFSIZE, fmt, vargs);
	if (len >= MPR_HTTP_BUFSIZE) {
		mprLog(MPR_VERBOSE, tMod, "%d: put buffer overflow\n", getFd());
		va_end(vargs);
		return 0;
	}
	va_end(vargs);
	return write(buf, len);
}

////////////////////////////////////////////////////////////////////////////////

void MaRequest::enableWriteEvents(bool on)
{
	int		oldMask = socketEventMask;

	mprLog(8, tMod, "%d: enableWriteEvents: %d\n", getFd(), on);

	if (flags & MPR_HTTP_BLOCKING) {
		return;
	}

	socketEventMask &= ~MPR_WRITEABLE;
	socketEventMask |= (on) ? MPR_WRITEABLE: 0;

	if (sock && socketEventMask != oldMask) {
		sock->setCallback(socketEventWrapper, this, (void*) 0,
			socketEventMask, MPR_NORMAL_PRIORITY);
	}
}

////////////////////////////////////////////////////////////////////////////////

void MaRequest::enableReadEvents(bool on)
{
	int		oldMask = socketEventMask;

	mprLog(8, tMod, "enableReadEvents %d\n", on);

	if (flags & MPR_HTTP_BLOCKING) {
		return;
	}
	socketEventMask &= ~MPR_READABLE;
	socketEventMask |= (on) ? MPR_READABLE: 0;

	if (sock && socketEventMask != oldMask) {
		sock->setCallback(socketEventWrapper, this, (void*) 0,
			socketEventMask, MPR_NORMAL_PRIORITY);
	}
}

////////////////////////////////////////////////////////////////////////////////

static void timeoutWrapper(void *arg, MprTimer *tp)
{
	MaRequest	*rq;
	int			delay;

	rq = (MaRequest*) arg;

	delay = rq->timeoutCheck();
	if (delay > 0) {
		tp->reschedule(delay);
	}
}

////////////////////////////////////////////////////////////////////////////////
//
//	WARNING: not called from select
//

int MaRequest::timeoutCheck()
{
	int		elapsed;

	elapsed = getTimeSinceLastActivity();

	if (elapsed >= timeout) {
		//
		//	MUST lock here to synchronize with any thread that may be
		//	executing the request. All socketEvents lock the request thread
		//	for the duration of the request
		//
#if BLD_FEATURE_MULTITHREAD
		if (mutex->tryLock() < 0) {
			//	Return a very short nap and we will retry very soon.
			return 250;
		}
#endif
		mprLog(5, tMod, "%d: timeoutCheck: timed out\n", getFd());

		/*
		 * 	The socket event handler will delete the request for us
		 */
		stats.timeouts++;
		if (timer) {
			timer->dispose();
			timer = 0;
		}
		responseCode = 408;
		flags |= MPR_HTTP_INCOMPLETE;
		enableReadEvents(1);
		enableWriteEvents(1);
#if BLD_FEATURE_MULTITHREAD
		mutex->unlock();
#endif

		return 0;

	} else {
		mprLog(6, tMod, "%d: timeoutCheck: elapsed %d, timeout %d diff %d\n", 
			getFd(), elapsed, timeout, timeout - elapsed);
	}
	return timeout;
}

////////////////////////////////////////////////////////////////////////////////

void MaRequest::requestError(int code, char *fmt, ...)
{
	va_list		args;
	char		*logMsg, *buf, *fileName, *url;

	mprAssert(fmt);

	stats.errors++;

	url = host->lookupErrorDocument(code);
	if (url && *url) {
		redirect(302, url);
		flushOutput(MPR_HTTP_FOREGROUND_FLUSH, 0);
		return;
	}

	fileName = getFileName();
	if (fileName == 0) {
		fileName = "";
	}

	//
	//	Error codes above 700 are used by the unit test suite
	//
	if (code < 700 && code != 301 && code != 302) {
		logMsg = 0;
		va_start(args, fmt);
		mprAllocVsprintf(&logMsg, MPR_HTTP_BUFSIZE, fmt, args);
		va_end(args);
		mprError(MPR_L, MPR_LOG, "%d \"%s\" for \"%s\", file \"%s\": %s", 
			code, getErrorMsg(code), uri ? uri : "", fileName, logMsg);
		mprFree(logMsg);
	}

	buf = 0;
	mprAllocSprintf(&buf, MPR_HTTP_BUFSIZE, 
		"<HTML><HEAD><TITLE>Document Error: %s</TITLE></HEAD>\r\n"
		"<BODY><H2>Access Error: %d -- %s</H2>\r\n"
		"</BODY></HTML>\r\n",
		getErrorMsg(code), code, getErrorMsg(code));
	formatAltResponse(code, buf, MPR_HTTP_DONT_ESCAPE);
	mprFree(buf);

	flushOutput(MPR_HTTP_FOREGROUND_FLUSH, 0);
}

////////////////////////////////////////////////////////////////////////////////
//
//	Redirect the user to another web page
// 

void MaRequest::redirect(int code, char *targetUrl)
{
	char	urlBuf[MPR_HTTP_MAX_URL], headerBuf[MPR_HTTP_MAX_URL];
	char	*uriDir, *cp, *hostName, *proto;

	mprAssert(targetUrl);
	stats.redirects++;

	mprLog(3, tMod, "%d: redirect %d %s\n", getFd(), code, targetUrl);

	if (code < 300 || code > 399) {
		code = 302;
	}

	if (strncmp(targetUrl, url.proto, strlen(url.proto)) != 0) {

		if (strchr(targetUrl, ':') == 0) {

			//
			//	Use the host name that came in the request by preference
			//	otherwise resort to the defined ServerName directive
			//
			if (header.host && *header.host) {
				hostName = header.host;
			} else {
				hostName = host->getName();
			}
#if BLD_FEATURE_SSL_MODULE
			if (host->isSecure()) {
				proto = "https";
			} else {
				proto = url.proto;
			}
#else
			proto = url.proto;
#endif

			if (*targetUrl == '/') {
				mprSprintf(urlBuf, sizeof(urlBuf), "%s://%s/%s", proto, 
					hostName, &targetUrl[1]);

			} else {
				uriDir = mprStrdup(uri);
				if ((cp = strrchr(uriDir, '/')) != 0) {
					*cp = '\0';
				}
				mprSprintf(urlBuf, sizeof(urlBuf), "%s://%s%s/%s", proto, 
					hostName, uriDir, targetUrl);
				mprFree(uriDir);
			}
			targetUrl = urlBuf;
		}
	}

	mprSprintf(headerBuf, sizeof(headerBuf), "Location: %s", targetUrl);
	setHeader(headerBuf, 0);
	setResponseCode(code);
}

////////////////////////////////////////////////////////////////////////////////

void MaRequest::formatAltResponse(int code, char *msg, int callFlags)
{
	MaDataStream	*saveBuf;
	char			buf[MPR_HTTP_MAX_ERR_BODY];
	char			*date;

	responseCode = code;
	saveBuf = writeBuf;
	writeBuf = hdrBuf;

#if BLD_FEATURE_KEEP_ALIVE
	if (flags & MPR_HTTP_INCOMPLETE) {
		flags &= ~MPR_HTTP_KEEP_ALIVE;
	}
#endif

	writeFmt("%s %d %s\r\n", header.proto ? header.proto : "UnknownMethod", 
		responseCode, getErrorMsg(responseCode));
	outputHeader("Server: %s", MPR_HTTP_SERVER_NAME);
 
	if ((date = maGetDateString(0)) != 0) {
		outputHeader("Date: %s", date);
		mprFree(date);
	}
#if BLD_FEATURE_KEEP_ALIVE
	if (flags & MPR_HTTP_KEEP_ALIVE) {
		outputHeader("Connection: keep-alive");
		outputHeader("Keep-Alive: timeout=%d, max=%d", 
			host->getKeepAliveTimeout() / 1000, remainingKeepAlive);
	} else 
#endif
	{
		outputHeader("Connection: close");
	}
	outputHeader("Content-Type: text/html");
	flags |= MPR_HTTP_HEADER_WRITTEN;

	//
	//	Output any remaining custom headers
	//
	if (flags & MPR_HTTP_CUSTOM_HEADERS) {
		MprStringData	*sd, *nextSd;
		sd = (MprStringData*) responseHeaders->getFirst();
		while (sd) {
			nextSd = (MprStringData*) responseHeaders->getNext(sd);
			write(sd->getValue());
			write("\r\n");
			responseHeaders->remove(sd);
			delete sd;
			sd = nextSd;
		}
	}

	if ((flags & MPR_HTTP_HEAD_REQUEST) == 0 && msg && *msg) {
		outputHeader("Content-length: %d", strlen(msg) + 2);
		write("\r\n");
		if (callFlags & MPR_HTTP_DONT_ESCAPE) {
			writeFmt("%s\r\n", msg);
		} else {
			maEscapeHtml(buf, sizeof(buf), msg);
			writeFmt("%s\r\n", buf);
		}
	}
	writeBuf = saveBuf;
}

////////////////////////////////////////////////////////////////////////////////

void MaRequest::setHeaderFlags(int setFlags, int clearFlags)
{
	setFlags &= 
		(MPR_HTTP_DONT_CACHE | MPR_HTTP_HEADER_WRITTEN | MPR_HTTP_FLUSHED);
	clearFlags &= 
		(MPR_HTTP_DONT_CACHE | MPR_HTTP_HEADER_WRITTEN | MPR_HTTP_FLUSHED);
	flags |= setFlags;
	flags &= ~clearFlags;
}

////////////////////////////////////////////////////////////////////////////////

void MaRequest::setHeader(char *value, bool allowMultiple)
{
	char			*cp;
	MprStringData	*sd, *nextSd;
	int				len;
	
	if (! allowMultiple) {
		if ((cp = strchr(value, ':')) != 0) {
			len = cp - value;
		} else {
			len = strlen(value);
		}
		sd = (MprStringData*) responseHeaders->getFirst();
		while (sd) {
			nextSd = (MprStringData*) responseHeaders->getNext(sd);
			if (mprStrCmpAnyCaseCount(sd->getValue(), value, len) == 0) {
				responseHeaders->remove(sd);
				delete sd;
				break;
			}
			sd = nextSd;
		}
	}
	responseHeaders->insert(value);
	flags |= MPR_HTTP_CUSTOM_HEADERS;
}

////////////////////////////////////////////////////////////////////////////////
//
//	For internal use only to output standard headers
//

void MaRequest::outputHeader(char *fmt, ...)
{
	MprStringData		*sd, *nextSd;
	va_list				vargs;
	char				*cp, buf[MPR_HTTP_BUFSIZE];
	int					len;
	
	va_start(vargs, fmt);
	mprVsprintf(buf, MPR_HTTP_MAX_HEADER, fmt, vargs);

	if (flags & MPR_HTTP_CUSTOM_HEADERS) {
		if ((cp = strchr(buf, ':')) != 0) {
			len = cp - buf;
		} else {
			len = strlen(buf);
		}
		sd = (MprStringData*) responseHeaders->getFirst();
		while (sd) {
			nextSd = (MprStringData*) responseHeaders->getNext(sd);
			if (mprStrCmpAnyCaseCount(sd->getValue(), buf, len) == 0) {
				write(sd->getValue());
				write("\r\n");
				responseHeaders->remove(sd);
				delete sd;
				return;
			}
			sd = nextSd;
		}
	}
	write(buf);
	write("\r\n");
}

////////////////////////////////////////////////////////////////////////////////

void MaRequest::writeHeaders()
{
	MprStringData	*sd, *nextSd;
	MaDataStream	*saveBuf;
	char			*date;

	/*
	 * 	Set flag first so that outputBuffer won't go recursive calling writeHeaders again.
	 */
	flags |= MPR_HTTP_HEADER_WRITTEN;

	saveBuf = writeBuf;
	writeBuf = hdrBuf;

#if BLD_FEATURE_RANGES
	if (responseCode == MPR_HTTP_OK && range) {
		responseCode = MPR_HTTP_PARTIAL_CONTENT;
	}
#endif

	writeFmt("%s %d %s\r\n", header.proto, responseCode, getErrorMsg(responseCode));

	date = maGetDateString(0);
	outputHeader("Date: %s", date);
	mprFree(date);

	//
	//	Calculate the total content length of the document
	//
	responseLength = getOutputStreamLength(0);

	outputHeader("Server: %s", MPR_HTTP_SERVER_NAME);

	if (flags & MPR_HTTP_DONT_CACHE) {
		//
		//	OLD HTTP/1.0 
		//		Pragma: no-cache
		//
		outputHeader("Cache-Control: no-cache");
	}

	if (responseCode != 302) {
		outputHeader("Content-type: %s", 
			(responseMimeType) ? responseMimeType : "text/html");
	}

	if (docBuf->head) {
		date = maGetDateString(&fileInfo);
		outputHeader("Last-modified: %s", date);
		mprFree(date);
	}
	if (etag) {
		outputHeader("ETag: \"%s\"", etag);
	}

	if (flags & MPR_HTTP_CHUNKED) {
		outputHeader("Transfer-Encoding: chunked");

	} else if (flags & MPR_HTTP_FLUSHED) {
		//
		//	Can't do keep-alive as the output buffer overflowed and we 
		//	have already flushed some data. i.e. we don't know the total 
		//	length of the content.
		//
		flags &= ~MPR_HTTP_KEEP_ALIVE;

	} else {

#if BLD_FEATURE_RANGES
		if (range) {
			outputHeader("AcceptRanges: bytes");
			if (flags & MPR_HTTP_OUTPUT_MULTI) {
				/*
				 *	FUTURE: would be nice to be able to calculate the content
				 *	length, but need a different output buffering architecture
				 *	where headers are calculated at the very end.
				 */
				flags &= ~MPR_HTTP_KEEP_ALIVE;

			} else {
				/*
				 *	Only come here for simple (single) ranges
				 */
				int		start, end, len;

				/*
				 *	If range is beyond the end of the content, then fix the
				 *	range to be just what is available.
				 */
				end = outputEnd;
				if (end > responseLength) {
					end = responseLength;
				}

				/*
				 *	If outputStart is negative, then we are outputting bytes
				 *	from the end of the file.
				 */
				if (outputStart < 0) {
					mprAssert(end > 0);
					start = responseLength - end + 1;
					end = responseLength;

				} else {
					/*
					 *	We have a start offset. If end is negative, then
					 *	we output through to the end of the buffer.
					 */
					start = outputStart;
					if (end < 0) {
						end = responseLength;
					} else {
						end = outputEnd;
					}
				}
				len = end - start;

				mprAssert(start >= 0);
				mprAssert(end >= 0);
				mprAssert(responseLength > 0);
				mprAssert(len > 0);
				mprAssert(start < end);
				mprAssert(len < responseLength);

				/*
				 *	The third %d is the original content length. NOT the length
				 *	of content actually transferred which may be a range of that
				 *	content.
				 */
				outputHeader("Content-Range: %d-%d/%d", start, end - 1,
					responseLength);

				outputHeader("Content-length: %d", len);
			}

		} else
#endif
			outputHeader("Content-length: %d", responseLength);
	}

#if BLD_FEATURE_KEEP_ALIVE
	//
	//	Unread post data will pollute the channel. We could read it, but 
	//	since something has gone wrong -- better to close the connection.
	//
	if (flags & MPR_HTTP_CONTENT_DATA && remainingContent > 0) {
		flags &= ~MPR_HTTP_KEEP_ALIVE;
	}
	if (flags & MPR_HTTP_KEEP_ALIVE) {
		outputHeader("Connection: keep-alive");
		outputHeader("Keep-Alive: timeout=%d, max=%d", 
			host->getKeepAliveTimeout() / 1000, remainingKeepAlive);
	} else 
#endif
	{
		outputHeader("Connection: close");
	}

	//
	//	Output any remaining custom headers
	//
	if (flags & MPR_HTTP_CUSTOM_HEADERS) {
		sd = (MprStringData*) responseHeaders->getFirst();
		while (sd) {
			nextSd = (MprStringData*) responseHeaders->getNext(sd);
			write(sd->getValue());
			write("\r\n");
#if BLD_DEBUG
			mprLog(5, tMod, "%d: custom header %s\r\n", getFd(), 
				sd->getValue()); 
#endif
			responseHeaders->remove(sd);
			delete sd;
			sd = nextSd;
		}
	}
	
#if BLD_FEATURE_RANGES
	if (flags & MPR_HTTP_OUTPUT_MULTI && range) {
		mprAllocSprintf(&rangeBoundary, MPR_HTTP_BUFSIZE, 
			"%08x%08x", 
			fileInfo.inode + fileInfo.mtime, fileInfo.size + time(0));
		outputHeader("Content-type: multipart/byteranges; boundary=%s",
			rangeBoundary);
	}
#endif

	//
	//	This marks the end of the headers. If using chunked output, the
	//	blank line will be output with the initial chunk length.
	//
	if (! (flags & MPR_HTTP_CHUNKED)) {
		write("\r\n");
#if BLD_DEBUG
		mprLog(3, MPR_RAW, tMod, "\r\n"); 
#endif
	}

	mprLog(6, tMod, "%d: writeHeaders. Headers =>\n%s", getFd(), 
		hdrBuf->buf.getStart());
	writeBuf = saveBuf;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Caller must delete the request object
//

void MaRequest::cancelRequest()
{
	lock();

	resetEnvObj();
	reset();

	responseCode = 503;
	flags |= MPR_HTTP_INCOMPLETE;
	if (sock != 0) {
		mprLog(3, tMod, "%d: cancelRequest\n", getFd());
		//
		//	Take advantage that close() is idempotent
		//
		sock->close(MPR_SOCKET_LINGER);
		sock = 0;
	}

	unlock();
}

////////////////////////////////////////////////////////////////////////////////

void MaRequest::finishRequest(int code, bool alsoCloseSocket)
{
	responseCode = code;
	finishRequest(alsoCloseSocket);
}

////////////////////////////////////////////////////////////////////////////////

void MaRequest::finishRequest(bool alsoCloseSocket)
{
	mprLog(5, tMod, "%d: finishRequest: alsoCloseSocket %d\n", getFd(), 
		alsoCloseSocket);

	//
	//	Need to synchronize with timeoutCheck() and CGI handler
	//
	lock();
	mprAssert(MPR_HTTP_START <= state && state <= MPR_HTTP_DONE);

	if (MPR_HTTP_START < state && state < MPR_HTTP_DONE) {
		if (flags & MPR_HTTP_CHUNKED) {
			blockingWrite("\r\n0\r\n\r\n", 7);
#if BLD_DEBUG
			mprLog(4, MPR_RAW, tMod, "\r\n0 ; chunk length 0\r\n");
#endif
		}
		state = MPR_HTTP_DONE;
		cancelTimeout();

		deleteHandlers();

		if (flags & MPR_HTTP_REUSE) {
			stats.keptAlive++;
		}

		if (flags & MPR_HTTP_OPENED_DOC) {
			file->close();
			flags &= ~MPR_HTTP_OPENED_DOC;
		}

#if BLD_FEATURE_ACCESS_LOG
		if (! (flags & MPR_HTTP_INCOMPLETE)) {
			logRequest();
		}
#endif
	}

#if BLD_FEATURE_KEEP_ALIVE
	if (!alsoCloseSocket && (flags & MPR_HTTP_KEEP_ALIVE) && remainingKeepAlive > 0) {
		if (state != MPR_HTTP_START) {
			mprLog(3, tMod, 
				"%d: finishMaRequest: Attempting keep-alive\n", getFd());
			resetEnvObj();
			reset();
			remainingKeepAlive--;
			flags |= MPR_HTTP_REUSE;
			if (!mprGetDebugMode()) {
				mprAssert(timer == 0);
				timeout = host->getKeepAliveTimeout();
				timer = new MprTimer(MPR_HTTP_TIMER_PERIOD, timeoutWrapper,
					(void*) this);
			}
			enableReadEvents(1);
		}

	} else 
#endif
	{
		//	TODO - may be better to let the client close first
		closeSocket();
	}
	unlock();
}

////////////////////////////////////////////////////////////////////////////////

void MaRequest::closeSocket()
{
	MprSocket	*s;
	
	lock();
	if (sock != 0) {
		mprLog(5, tMod, "%d: closeSocket: closing socket\n", getFd());
		s = sock;
		sock = 0;
		s->close(MPR_SOCKET_LINGER);
		s->dispose();
	}
	if (head) {
		host->removeRequest(this);
	}
	flags |= MPR_HTTP_CONN_CLOSED;
	unlock();
}

////////////////////////////////////////////////////////////////////////////////
#if BLD_FEATURE_RANGES
/*
 *	Format is:  Range: bytes=n1-n2,n3-n4,...
 *	Where n1 is first byte pos and n2 is last byte pos
 *
 *	Examples:
 *		Range: 0-49				first 50 bytes
 *		Range: 50-99,200-249	Two 50 byte ranges from 50 and 200
 *		Range: -50				Last 50 bytes
 *		Range: 1-				Skip first byte then emit the rest
 *
 *	Return 1 if more ranges, 0 if end of ranges, -1 if bad range.
 */

int MaRequest::getNextRange(char **nextRange)
{
	char	*tok, *ep;

	outputStart = outputEnd = -1;

	tok = mprStrTok(*nextRange, ",", nextRange);

	if (tok == 0 || *tok == '\0') {
		return 0;
	}

	if (*tok != '-') {
		outputStart = mprAtoi(tok, 10);
	}

	if ((ep = strchr(tok, '-')) != 0) {
		if (*++ep != '\0') {
			//
			//	outputEnd is one beyond the range. Makes the math easier.
			//
			outputEnd = mprAtoi(ep, 10) + 1;
		}
	}

	if (outputStart >= 0 && outputEnd >= 0 && (outputStart >= outputEnd)) {
		return -1;
	}
	if (outputStart < 0 && outputEnd < 0) {
		return -1;
	}

	return 1;
}

////////////////////////////////////////////////////////////////////////////////

void MaRequest::deRangeOutput()
{
	mprFree(range);
	range = 0;
}

#endif
////////////////////////////////////////////////////////////////////////////////
#if BLD_FEATURE_ACCESS_LOG

void MaRequest::logRequest()
{
	MaHost		*logHost;
	MprBuf		buf;
	time_t		tm;
	char		localBuffer[MPR_HTTP_MAX_URL + 256], timeBuf[64];
	char		*fmt, *cp, *value, *qualifier;
	char		c;

	logHost = host->getLogHost();
	if (logHost == 0) {
		return;
	}
	fmt = logHost->getLogFormat();
	if (fmt == 0) {
		return;
	}

	buf.setBuf((uchar*) localBuffer, (int) sizeof(localBuffer) - 1);

	while ((c = *fmt++) != '\0') {
		if (c != '%' || (c = *fmt++) == '%') {
			buf.put(c);
			continue;
		}

		switch (c) {
		case 'a':							// Remote IP
			buf.put(remoteIpAddr);
			break;

		case 'A':							// Local IP
			buf.put(listenSock->getIpAddr());
			break;

		case 'b':
			if (bytesWritten == 0) {
				buf.put('-');
			} else {
				buf.putInt(bytesWritten);
			} 
			break;

		case 'B':							// Bytes written (minus headers)
			buf.putInt(bytesWritten - hdrBuf->size);
			break;

		case 'h':							// Remote host
			buf.put(remoteIpAddr);
			break;

		case 'n':							// Local host
			if (header.host) {
				buf.put(header.host);
			} else {
				buf.put(url.host);
			}
			break;

		case 'l':							// Supplied in authorization
			if (user == 0) {
				buf.put('-');
			} else {
				buf.put(user);
			}
			break;

		case 'O':							// Bytes written (including headers)
			buf.put(bytesWritten);
			break;

		case 'r':							// First line of request
			buf.put(header.firstLine);
			break;

		case 's':							// Response code
			buf.putInt(responseCode);
			break;

		case 't':							// Time
			time(&tm);
			mprCtime(&tm, timeBuf, sizeof(timeBuf));
			if ((cp = strchr(timeBuf, '\n')) != 0) {
				*cp = '\0';
			}
			buf.put('[');
			buf.put(timeBuf);
			buf.put(']');
			break;

		case 'u':							// Remote username
			if (user == 0) {
				buf.put('-');
			} else {
				buf.put(user);
			}
			break;

		case '{':							// Header line
			qualifier = fmt;
			if ((cp = strchr(qualifier, '}')) != 0) {
				fmt = &cp[1];
				*cp = '\0';
				c = *fmt++;
				switch (c) {
				case 'i':
					if ((value = getVar(MA_HEADERS_OBJ, qualifier, 0)) != 0) {
						buf.put(value);
					}
					break;
				default:
					buf.put(qualifier);
				}
				*cp = '}';

			} else {
				buf.put(c);
			}
			break;

		case '>':
			if (*fmt == 's') {
				fmt++;
				buf.putInt(responseCode);
			}
			break;

		default:
			buf.put(c);
			break;
		}
	}
	buf.put('\n');
	buf.addNull();

	logHost->writeLog(buf.getStart(), buf.getLength());
}

#endif // BLD_FEATURE_HTTP_ACCESS_LOG 
////////////////////////////////////////////////////////////////////////////////

int MaRequest::parseUri()
{
	int		len;

	//
	//	Re-examine the new URI
	//
	if (url.parse(uri) < 0) {
		return MPR_ERR_BAD_SYNTAX;
	}
	mprFree(uri);
	uri = mprStrdup(url.uri);
	len = strlen(uri);
	if (maUrlDecode(uri, len + 1, uri, 1, 0) == 0) {
		return MPR_ERR_BAD_SYNTAX;
	}
	if (maValidateUri(uri) == 0) {
		return MPR_ERR_BAD_SYNTAX;
	}
	if (url.ext == 0 || 
			(requestMimeType = host->lookupMimeType(url.ext)) == 0) {
		requestMimeType = "text/plain";
	}

	mprFree(responseMimeType);
	responseMimeType = mprStrdup(requestMimeType);

	return 0;
}

////////////////////////////////////////////////////////////////////////////////

void MaRequest::reRunHandlers()
{
	if (parseUri() < 0) {
		requestError(400, "Bad URL format");
		finishRequest();
		return;
	}

	mprLog(5, tMod, "%d: reRunHandlers: for %s\n", getFd(), uri);

	dir = 0;
	location = 0;

	deleteHandlers();
	if (setupHandlers() != 0) {
		return;
	}
	runHandlers();
}

////////////////////////////////////////////////////////////////////////////////
//
//	WARNING: the request can actually be processed here if it requires 
//	redirection. This can happen during matchHandlers()
//

int MaRequest::setupHandlers()
{
	MaHandler			*hp;

#if BLD_FEATURE_LOG
	mprLog(3, tMod, "%d: %s: is the serving host\n", getFd(), host->getName());
#endif

	if (file == 0) {
		file = fileSystem->newFile();
	}

	if (matchHandlers() != 0) {
		return 1;
	}

	hp = (MaHandler*) handlers.getFirst();
	while (hp) {
		mprLog(5, tMod, "%d: setupHandlers: %s\n", getFd(), hp->getName());
		hp->setup(this);
		hp = (MaHandler*) handlers.getNext(hp);
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

void MaRequest::runHandlers()
{
	MaHandler	*hp, *terminal;

	state = MPR_HTTP_RUNNING;
	terminal = terminalHandler;

	hp = (MaHandler*) handlers.getFirst();
	while (hp) {
		if ((hp->getFlags() & methodFlags) || 
				(hp->getFlags() & MPR_HANDLER_ALWAYS)) {
			mprLog(4, tMod, "%d: runHandlers running: %s\n", 
				getFd(), hp->getName());
			currentHandler = hp;

			hp->run(this);

			//
			//	NOTE: the request may have been finished and the request 
			//	structure may be reset here.
			//

			if (state == MPR_HTTP_RUN_HANDLERS) {
				reRunHandlers();
				return;
			}
			if (hp == terminal) {
				return;
			}
			if (state == MPR_HTTP_DONE || state == MPR_HTTP_START) {
				return;
			}

		} else {
			if (hp == terminalHandler) {
				if (methodFlags & (MPR_HANDLER_HEAD | 
						MPR_HANDLER_OPTIONS | MPR_HANDLER_TRACE)) {
					responseCode = MPR_HTTP_OK;
					flushOutput(MPR_HTTP_BACKGROUND_FLUSH, 
						MPR_HTTP_FINISH_REQUEST);
					return;
				}
				requestError(MPR_HTTP_BAD_METHOD, 
					"HTTP method \"%s\" is not supported by handler %s", 
					getMethod(), hp->getName());
				finishRequest();
				return;
			}
		}
		hp = (MaHandler*) handlers.getNext(hp);
	}
	requestError(MPR_HTTP_INTERNAL_SERVER_ERROR, "Request not processed");
	finishRequest();
}

////////////////////////////////////////////////////////////////////////////////

void MaRequest::deleteHandlers()
{
	MaHandler	*hp, *nextHp;

	hp = (MaHandler*) handlers.getFirst();
	while (hp) {
		mprLog(5, tMod, "%d: deleteHandlers: %s\n", getFd(), hp->getName());
		nextHp = (MaHandler*) handlers.getNext(hp);
		handlers.remove(hp);
		if (hp == terminalHandler) {
			terminalHandler = 0;
		}
		delete hp;
		hp = nextHp;
	}
}

////////////////////////////////////////////////////////////////////////////////

void MaRequest::createEnvironmentStore()
{
	if (variables[MA_REQUEST_OBJ].name != 0) {
		resetEnvObj();
	}

	//
	//	MA_HEADERS_OBJ is already created. MA_GLOBAL_OBJ and MA_LOCAL_OBJ will
	//	be created if the ESP handler is run.
	//
	variables[MA_REQUEST_OBJ]	= mprCreateObjVar("request", MA_HTTP_HASH_SIZE);
	variables[MA_SERVER_OBJ] 	= mprCreateObjVar("server", MA_HTTP_HASH_SIZE);
	variables[MA_COOKIES_OBJ] 	= mprCreateObjVar("cookies", MA_HTTP_HASH_SIZE);
	variables[MA_FILES_OBJ] 	= mprCreateObjVar("files", MA_HTTP_HASH_SIZE);
	variables[MA_FORM_OBJ] 		= mprCreateObjVar("form", MA_HTTP_HASH_SIZE);

	if (flags & MPR_HTTP_OWN_GLOBAL) {
		variables[MA_GLOBAL_OBJ] = mprCreateObjVar("global", MA_HTTP_HASH_SIZE);
	}

	//
	//	This will copy references to the application and session objects and
	//	increment the reference counts.
	//
	mprCopyVar(&variables[MA_APPLICATION_OBJ], &host->server->appObj, 0);
#if BLD_FEATURE_SESSION
	if (session) {
		mprCopyVar(&variables[MA_SESSION_OBJ], session->getSessionData(), 0);
	}
#endif
}

////////////////////////////////////////////////////////////////////////////////

void MaRequest::createEnvironment()
{
	createEnvironmentStore();

	//
	//	FUTURE -- OPT. Can now provide automatic data to setVar so 
	//	some storage in request can probably be reclaimed.
	//
	setVar(MA_REQUEST_OBJ, "AUTH_TYPE", header.authType);
	mprItoa(contentLength, contentLengthStr, sizeof(contentLengthStr));
	setVar(MA_REQUEST_OBJ, "CONTENT_LENGTH", contentLengthStr);
	setVar(MA_REQUEST_OBJ, "CONTENT_TYPE", header.contentMimeType);
	setVar(MA_REQUEST_OBJ, "PATH_INFO", (extraPath) ? extraPath : (char*) "");
	setVar(MA_REQUEST_OBJ, "QUERY_STRING", url.query);
	setVar(MA_REQUEST_OBJ, "REMOTE_ADDR", remoteIpAddr);

	if (user && *user) {
		setVar(MA_REQUEST_OBJ, "REMOTE_USER", user);
	} else {
		setVar(MA_REQUEST_OBJ, "REMOTE_USER", 0);
	}

#if BLD_FEATURE_REVERSE_DNS && !WIN
	/*
	 * 	This feature has denial of service risks. Doing a reverse DNS will be slower,
	 * 	and can potentially hang the web server. Use at your own risk!!
 	 *	Not yet supported for windows.
	 */
	{
		struct addrinfo *result;
		char			name[MPR_MAX_STRING];
		int 			rc;

		if (getaddrinfo(remoteIpAddr, NULL, NULL, &result) == 0) {
			rc = getnameinfo(result->ai_addr, sizeof(struct sockaddr), name, sizeof(name), 
				NULL, 0, NI_NAMEREQD);
			freeaddrinfo(result);
			if (rc == 0) {
				setVar(MA_REQUEST_OBJ, "REMOTE_HOST", remoteIpAddr);
			}
		}
		setVar(MA_REQUEST_OBJ, "REMOTE_HOST", (rc == 0) ? name : remoteIpAddr);
	}
#else
	setVar(MA_REQUEST_OBJ, "REMOTE_HOST", remoteIpAddr);
#endif

	setVar(MA_REQUEST_OBJ, "REQUEST_METHOD", header.method);
	setVar(MA_REQUEST_OBJ, "REQUEST_URI", header.uri);

#if BLD_FEATURE_SSL_MODULE
	setVar(MA_REQUEST_OBJ, "REQUEST_TRANSPORT", 
		(char*) ((host->isSecure()) ? "https" : "http"));
#else
	setVar(MA_REQUEST_OBJ, "REQUEST_TRANSPORT", "http");
#endif
	setVar(MA_REQUEST_OBJ, "SCRIPT_NAME", scriptName);

	setVar(MA_SERVER_OBJ, "DOCUMENT_ROOT", host->getDocumentRoot());
	setVar(MA_SERVER_OBJ, "GATEWAY_INTERFACE", "CGI/1.1");
	setVar(MA_SERVER_OBJ, "SERVER_ADDR", listenSock->getIpAddr());
	mprItoa(listenSock->getPort(), localPort, sizeof(localPort) - 1);
	setVar(MA_SERVER_OBJ, "SERVER_PORT", localPort);
	setVar(MA_SERVER_OBJ, "SERVER_PROTOCOL", header.proto);
	setVar(MA_SERVER_OBJ, "SERVER_SOFTWARE", MPR_HTTP_SERVER_NAME);

	//	FUTURE: What is the difference between SERVER_NAME, HOST & URL
	setVar(MA_SERVER_OBJ, "SERVER_HOST", host->getName());
	setVar(MA_SERVER_OBJ, "SERVER_NAME", host->getName());
	setVar(MA_SERVER_OBJ, "SERVER_URL", host->getName());


#if UNUSED
	//
	//	Ensure some variables are always defined
	//
	if (testVar(MA_HEADERS_OBJ, "HOST") == 0) {
		setVar(MA_HEADERS_OBJ, "HOST", "");
	}
	if (testVar(MA_HEADERS_OBJ, "USER_AGENT") == 0) {
		setVar(MA_HEADERS_OBJ, "USER_AGENT", "");
	}
	if (testVar(MA_HEADERS_OBJ, "ACCEPT") == 0) {
		setVar(MA_HEADERS_OBJ, "ACCEPT", "");
	}
	if (testVar(MA_HEADERS_OBJ, "CONNECTION") == 0) {
		setVar(MA_HEADERS_OBJ, "CONNECTION", "");
	}
	if (testVar(MA_REQUEST_OBJ, "REMOTE_USER") == 0) {
		setVar(MA_REQUEST_OBJ, "REMOTE_USER", "");
	}
#endif

#if BLD_FEATURE_SESSION
	if (sessionId) {
		mprSetPropertyValue(&variables[MA_REQUEST_OBJ], "SESSION_ID", 
			mprCreateStringVar(sessionId, 0));
	}
#endif

	//
	//	Define variables for each keyword of the query. We don't do post data.
	//
	createQueryVars(url.query, strlen(url.query));
}

////////////////////////////////////////////////////////////////////////////////
//
//	Make variables for each keyword in a query. The buffer must be url encoded 
//	(ie. key=value&key2=value2..., spaces converted to '+' and all else should 
//	be %HEX encoded).
//

void MaRequest::createQueryVars(char *buf, int len)
{
	char	*newValue, *decoded, *keyword, *value, *oldValue, *tok;

	decoded = (char*) mprMalloc(len + 1);
	decoded[len] = '\0';
	memcpy(decoded, buf, len);

	keyword = mprStrTok(decoded, "&", &tok);
	while (keyword != 0) {
		if ((value = strchr(keyword, '=')) != 0) {
			*value++ = '\0';
			maUrlDecode(keyword, strlen(keyword) + 1, keyword, 0, 1);
			maUrlDecode(value, strlen(value) + 1, value, 0, 1);

		} else {
			value = "";
		}

		if (*keyword) {
			//
			//	Append to existing keywords.
			//
			oldValue = getVar(MA_FORM_OBJ, keyword, 0);
			if (oldValue != 0) {
				mprAllocSprintf(&newValue, MPR_HTTP_MAX_HEADER, "%s %s", 
					oldValue, value);
				setVar(MA_FORM_OBJ, keyword, newValue);
				mprFree(newValue);
			} else {
				setVar(MA_FORM_OBJ, keyword, value);
			}
		}
		keyword = mprStrTok(0, "&", &tok);
	}
	mprFree(decoded);
}

////////////////////////////////////////////////////////////////////////////////
#if BLD_FEATURE_LEGACY_API
//
//	DEPRECATED: 2.0
//

int MaRequest::testVar(char *var)
{
	return (mprGetProperty(&variables[MA_GLOBAL_OBJ], var, 0) != 0);
}

////////////////////////////////////////////////////////////////////////////////

char *MaRequest::getVar(char *var, char *defaultValue)
{
	MprVar		*vp;

	vp = mprGetProperty(&variables[MA_GLOBAL_OBJ], var, 0);
	if (vp && vp->type == MPR_TYPE_STRING) {
		return vp->string;
	}
	return defaultValue;
}

////////////////////////////////////////////////////////////////////////////////

void MaRequest::setVar(char *var, char *value) 
{
	if (flags & MPR_HTTP_CREATE_ENV) {
		mprSetPropertyValue(&variables[MA_GLOBAL_OBJ], var, 
			mprCreateStringVar(value, 0));
	}
}

////////////////////////////////////////////////////////////////////////////////

void MaRequest::unsetVar(char *var) 
{
	if (flags & MPR_HTTP_CREATE_ENV) {
		mprDeleteProperty(&variables[MA_GLOBAL_OBJ], var);
	}
}

////////////////////////////////////////////////////////////////////////////////

int MaRequest::compareVar(char *var, char *value)
{
	mprAssert(var && *var);
	mprAssert(value && *value);
 
	if (! (flags & MPR_HTTP_CREATE_ENV)) {
		return 0;
	}
	if (strcmp(value, getVar(var, " __UNDEF__ ")) == 0) {
		return 1;
	}
	return 0;
}

#endif // BLD_FEATURE_LEGACY_API
////////////////////////////////////////////////////////////////////////////////

int MaRequest::testVar(MaEnvType objType, char *var)
{
	return (mprGetProperty(&variables[objType], var, 0) != 0);
}

////////////////////////////////////////////////////////////////////////////////

char *MaRequest::getVar(MaEnvType objType, char *var, char *defaultValue)
{
	MprVar		*vp;

	vp = mprGetProperty(&variables[objType], var, 0);
	if (vp && vp->type == MPR_TYPE_STRING) {
		return vp->string;
	}
	return defaultValue;
}

////////////////////////////////////////////////////////////////////////////////

int MaRequest::getIntVar(MaEnvType objType, char *var, int defaultValue)
{
	MprVar		*vp;

	vp = mprGetProperty(&variables[objType], var, 0);
	if (vp && vp->type == MPR_TYPE_INT) {
		return vp->integer;
	}
	return defaultValue;
}

////////////////////////////////////////////////////////////////////////////////

void MaRequest::setVar(MaEnvType objType, char *var, char *value) 
{
	if (flags & MPR_HTTP_CREATE_ENV) {
		//
		//	FUTURE -- really need to push locking down into var
		//
		if (objType == MA_SERVER_OBJ) {
			host->server->http->lock();
		} else if (objType == MA_SESSION_OBJ) {
			host->lock();
		}

		mprSetPropertyValue(&variables[objType], var, 
			mprCreateStringVar(value, 0));

		if (objType == MA_SERVER_OBJ) {
			host->server->http->unlock();
		} else if (objType == MA_SESSION_OBJ) {
			host->unlock();
		}
	}
}

////////////////////////////////////////////////////////////////////////////////

void MaRequest::setIntVar(MaEnvType objType, char *var, int value) 
{
	if (flags & MPR_HTTP_CREATE_ENV) {
		//
		//	FUTURE -- really need to push locking down into var
		//
		if (objType == MA_SERVER_OBJ) {
			host->server->http->lock();
		} else if (objType == MA_SESSION_OBJ) {
			host->lock();
		}

		mprSetPropertyValue(&variables[objType], var, 
			mprCreateIntegerVar(value));

		if (objType == MA_SERVER_OBJ) {
			host->server->http->unlock();
		} else if (objType == MA_SESSION_OBJ) {
			host->unlock();
		}
	}
}

////////////////////////////////////////////////////////////////////////////////

void MaRequest::unsetVar(MaEnvType objType, char *var) 
{
	if (flags & MPR_HTTP_CREATE_ENV) {
		mprDeleteProperty(&variables[objType], var);
	}
}

////////////////////////////////////////////////////////////////////////////////

int MaRequest::compareVar(MaEnvType objType, char *var, char *value)
{
	mprAssert(var && *var);
	mprAssert(value && *value);
 
	if (! (flags & MPR_HTTP_CREATE_ENV)) {
		return 0;
	}
	if (strcmp(value, getVar(objType, var, " __UNDEF__ ")) == 0) {
		return 1;
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
 
void MaRequest::cancelTimeout()
{
	if (timer) {
		timer->stop(MPR_TIMEOUT_STOP);
		timer->dispose();
		timer = 0;
	}
}

////////////////////////////////////////////////////////////////////////////////
 
int MaRequest::getFlags()
{
	return flags;
}

////////////////////////////////////////////////////////////////////////////////

void MaRequest::setFlags(int orFlags, int andFlags)
{
	flags |= orFlags;
	flags &= andFlags;
}

////////////////////////////////////////////////////////////////////////////////
#if BLD_FEATURE_KEEP_ALIVE

void MaRequest::setNoKeepAlive()
{
	flags &= ~MPR_HTTP_KEEP_ALIVE;
}

#endif
////////////////////////////////////////////////////////////////////////////////

char *MaRequest::getOriginalUri()
{
	return header.uri;
}

////////////////////////////////////////////////////////////////////////////////

char *MaRequest::getUri()
{
	return uri;
}

////////////////////////////////////////////////////////////////////////////////

int MaRequest::getBytesWritten()
{
	return bytesWritten;
}

////////////////////////////////////////////////////////////////////////////////

int MaRequest::setFileName(char *newPath)
{
	char	tagBuf[64];

	mprFree(fileName);
	fileName = mprStrdup(newPath);

	// 
	//  Must not let the user set a non-regular file.
	//
	if (fileSystem->stat(newPath, &fileInfo) < 0 || !fileInfo.isReg) {

		mprAssert(terminalHandler);
		//
		//	Map virtual means the handler does not map the URL onto physical 
		//	storage. E.g. The EGI handler.
		//
		if (! (terminalHandler->getFlags() & MPR_HANDLER_MAP_VIRTUAL) &&
				!(flags & MPR_HTTP_PUT_REQUEST)) {
			requestError(404, "Can't access URL");
			finishRequest();
			return MPR_ERR_CANT_ACCESS;
		}
		if (etag) {
			mprFree(etag);
			etag = 0;
		}
		return 0;
	}

	mprSprintf(tagBuf, sizeof(tagBuf), "%x-%x-%x", fileInfo.inode, 
		fileInfo.size, fileInfo.mtime);
	mprFree(etag);
	etag = mprStrdup(tagBuf);
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

void MaRequest::setUri(char *newUri)
{
	if (uri) {
		mprFree(uri);
	}
	uri = mprStrdup(newUri);
}

////////////////////////////////////////////////////////////////////////////////

int MaRequest::setExtraPath(char *prefix, int prefixLen)
{
	char	*cp;

	mprFree(scriptName);
	mprFree(extraPath);

	scriptName = mprStrdup(uri);

	//
	//	Careful, extraPath must either zero or be duped below
	//
	if (prefix) {
		extraPath = strchr(&scriptName[prefixLen + 1], '/');
	} else {
		extraPath = 0;
	}
	if (extraPath) {
		if (maValidateUri(extraPath) == 0) {
			return MPR_ERR_BAD_ARGS;
		}
		cp = extraPath;
		extraPath = mprStrdup(extraPath);
		*cp = 0;

		mprFree(uri);
		uri = mprStrdup(scriptName);
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

void MaRequest::setBytesWritten(int n)
{
	bytesWritten = n;
}

////////////////////////////////////////////////////////////////////////////////

void MaRequest::setResponseMimeType(char *mimeType)
{
	mprFree(responseMimeType);
	responseMimeType = mprStrdup(mimeType);
}

////////////////////////////////////////////////////////////////////////////////

void MaRequest::setTimeMark()
{
	timestamp = mprGetTime(0);
}

////////////////////////////////////////////////////////////////////////////////

int MaRequest::getTimeSinceLastActivity()
{
	int		elapsed;

	elapsed = mprGetTime(0) - timestamp;
	if (elapsed < 0) {
		elapsed = 0;
	}
	return elapsed;
}

////////////////////////////////////////////////////////////////////////////////

void MaRequest::insertHandler(MaHandler *hp)
{
	handlers.insert(hp);
}

////////////////////////////////////////////////////////////////////////////////

int MaRequest::openDoc(char *path)
{
	int		fd;

	fd = file->open(path, O_RDONLY | O_BINARY, 0666);
	if (fd >= 0) {
		flags |= MPR_HTTP_OPENED_DOC;
	}
	return fd;
}

////////////////////////////////////////////////////////////////////////////////

void MaRequest::closeDoc()
{
	flags &= ~MPR_HTTP_OPENED_DOC;
	file->close();
}

////////////////////////////////////////////////////////////////////////////////

int MaRequest::statDoc(MprFileInfo *fi)
{
	return fileSystem->stat(fileName, fi);
}

////////////////////////////////////////////////////////////////////////////////

bool MaRequest::isDir(char *path)
{
	return fileSystem->isDir(path);
}

////////////////////////////////////////////////////////////////////////////////

int MaRequest::readDoc(char *buf, int nBytes)
{
	return file->read(buf, nBytes);
}

////////////////////////////////////////////////////////////////////////////////

void MaRequest::seekDoc(long offset, int origin)
{
	file->lseek(offset, origin);
}

////////////////////////////////////////////////////////////////////////////////

static int refillDoc(MprBuf *bp, void *arg)
{
	MaRequest	*rq;
	int			len, rc;

	rq = (MaRequest*) arg;
	bp->flush();
	len = bp->getLinearSpace();
	rc = rq->readDoc(bp->getEnd(), bp->getLinearSpace());
	if (rc < 0) {
		return rc;
	}
	bp->adjustEnd(rc);
	return rc;
}

////////////////////////////////////////////////////////////////////////////////

MaAuth *MaRequest::getAuth()
{
	if (location) {
		return location->getAuth();
	} else if (dir) {
		return dir->getAuth();
	} else {
		mprAssert(0);
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

char *MaRequest::getAuthDetails()
{
	return header.authDetails;
}

////////////////////////////////////////////////////////////////////////////////

char *MaRequest::getAuthType()
{
	return header.authType;
}

////////////////////////////////////////////////////////////////////////////////

char *MaRequest::getQueryString()
{
	return getVar(MA_REQUEST_OBJ, "QUERY_STRING", 0);
}

////////////////////////////////////////////////////////////////////////////////
//
//	Get count of enumerable properties (excludes methods)
// 

int MaRequest::getNumEnvProperties()
{
	int 	i, numItems;

	numItems = 0;
	for (i = 0; i < MA_HTTP_OBJ_MAX; i++) {
		if (variables[i].type == MPR_TYPE_OBJECT) {
			numItems += mprGetPropertyCount(&variables[i], MPR_ENUM_DATA);
		}
	}
	return numItems;
}

////////////////////////////////////////////////////////////////////////////////

void MaRequest::insertDataStream(MaDataStream *dp)
{
	outputStreams.insert(dp);
}

////////////////////////////////////////////////////////////////////////////////
//
//	Return the total length of output stream. Note: this is all the data 
//	available and not the subset requested by a ranged request.
//

int MaRequest::getOutputStreamLength(bool countHeaders)
{
	MaDataStream	*dp;
	MprBuf			*buf;
	int				bytes;

	bytes = 0;
	dp = (MaDataStream*) outputStreams.getFirst();
	while (dp) {
		buf = &dp->buf;
		if (! countHeaders && (dp == hdrBuf)) {
			dp = (MaDataStream*) outputStreams.getNext(dp);
			continue;
		}

		//
		//	Streams containing no data but refering to other documents
		//	will set the buffer size. The copyHandler does this.
		//
		if (dp->getSize() >= 0) {
			bytes += dp->getSize();
		} else {
			bytes += dp->buf.getLength();
		}
		dp = (MaDataStream*) outputStreams.getNext(dp);
	}
	return bytes;
}

////////////////////////////////////////////////////////////////////////////////

char *MaRequest::getErrorMsg(int code)
{
	return maGetHttpErrorMsg(code);
}

////////////////////////////////////////////////////////////////////////////////

void MaRequest::cancelOutput()
{
	MaDataStream	*dp;
	MaDataStream	*nextDp;

	dp = (MaDataStream*) outputStreams.getFirst();
	while (dp) {
		nextDp = (MaDataStream*) outputStreams.getNext(dp);
		if (dp != hdrBuf) {
			outputStreams.remove(dp);
		}
		dp = nextDp;
	}
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
///////////////////////////////// MaRequestMatch ///////////////////////////////
////////////////////////////////////////////////////////////////////////////////
#if BLD_FEATURE_IF_MODIFIED

MaRequestMatch::MaRequestMatch()
{
	ifMatch = true;
}

////////////////////////////////////////////////////////////////////////////////

void MaRequestMatch::addEtag(char *newEtag)
{
	etags.insert(newEtag);
}

////////////////////////////////////////////////////////////////////////////////
//
//	Return TRUE if the client's cached copy matches an entity's etag.
//

bool MaRequestMatch::matches(char *responseEtag)
{
	MprStringData *ptr = (MprStringData*) etags.getFirst();

	if (ptr == 0) {
		//	If-Match or If-UnMatch not supplied
		return true;
	}

	while (ptr) {
		if (strcmp(ptr->getValue(), responseEtag) == 0)
			return (ifMatch) ? false : true;

		ptr = (MprStringData*) etags.getNext(ptr);
	}
	return (ifMatch) ? true : false;
}

////////////////////////////////////////////////////////////////////////////////

void MaRequestMatch::setMatch(bool match) 
{
	ifMatch = match;
}

////////////////////////////////////////////////////////////////////////////////
/////////////////////////////// MaRequestModified //////////////////////////////
////////////////////////////////////////////////////////////////////////////////

MaRequestModified::MaRequestModified()
{
	since = 0;
	ifModified = true;
}

////////////////////////////////////////////////////////////////////////////////
//
//	If an IF-MODIFIED-SINCE was specified, then return true if the resource 
//	has not been modified. If using IF-UNMODIFIED, then return true if the 
//	resource was modified.
//

bool MaRequestModified::matches(uint time)
{
	if (since == 0) {
		//	If-Modified or UnModified not supplied.
		return true;
	}

	if (ifModified) {
		//
		//	Return true if the file has not been modified.
		//
		return ! (time > since);
	} else {
		//
		//	Return true if the file has been modified.
		//
		return (time > since);
	}
}

////////////////////////////////////////////////////////////////////////////////

void MaRequestModified::setDate(uint when, bool ifMod) 
{
	since = when;
	ifModified = ifMod;
}
	
#endif // BLD_FEATURE_IF_MODIFIED

////////////////////////////////////////////////////////////////////////////////
#if BLD_FEATURE_COOKIE || BLD_FEATURE_SESSION

char *MaRequest::getCookies()
{
	return header.cookie;
}

////////////////////////////////////////////////////////////////////////////////
/* 
 *	Use getCookies now
 */
char *MaRequest::getCookie()
{
	return header.cookie;
}

////////////////////////////////////////////////////////////////////////////////
//
//	This routine parses the cookie and returns the cookie name, value and path.
//	It can handle quoted and back-quoted args. All args may be null. Caller 
//	must free supplied args. 
//

int MaRequest::getCrackedCookie(char *cookie, char **name, char **value, 
	char **path)
{
	char	*details, *keyValue, *tok, *key, *dp, *sp;
	int		seenSemi, seenValue;

	if (path) {
		*path = 0;
	}
	if (name) {
		*name = 0;
	}
	if (value) {
		*value = 0;
	}
	seenValue = 0;

	details = mprStrdup(cookie);
	key = details;

	while (*key) {
		while (*key && isspace(*key)) {
			key++;
		}
		tok = key;
		while (*tok && !isspace(*tok) && *tok != ';' && *tok != '=') {
			tok++;
		}
		if (*tok) {
			*tok++ = '\0';
		}

		while (isspace(*tok)) {
			tok++;
		}

		seenSemi = 0;
		if (*tok == '\"') {
			keyValue = ++tok;
			while (*tok != '\"' && *tok != '\0') {
				tok++;
			}
			if (*tok) {
				*tok++ = '\0';
			}

		} else {
			keyValue = tok;
			while (*tok != ';' && *tok != '\0') {
				tok++;
			}
			if (*tok) {
				seenSemi++;
				*tok++ = '\0';
			}
		}

		//
		//	Handle back-quoting
		//
		if (strchr(keyValue, '\\')) {
			for (dp = sp = keyValue; *sp; sp++) {
				if (*sp == '\\') {
					sp++;
				}
				*dp++ = *sp++;
			}
			*dp = '\0';
		}

		if (! seenValue) {
			if (name) {
				*name = mprStrdup(key);
			}
			if (value) {
				*value = mprStrdup(keyValue);
			}
			seenValue++;

		} else {
			switch (tolower(*key)) {
			case 'p':
				if (path && mprStrCmpAnyCase(key, "path") == 0) {
					*path = mprStrdup(keyValue);
				}
				break;

			default:
				//	Just ignore keywords we don't understand
				;
			}
		}

		key = tok;
		if (!seenSemi) {
			while (*key && *key != ';') {
				key++;
			}
			if (*key) {
				key++;
			}
		}
	}
	mprFree(details);
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

void MaRequest::setCookie(char *name, char *value, int lifetime, char *path, 
	bool secure)
{
	struct tm 	tm;
	time_t		when;
	char 		dateStr[64];
	char		*cookieBuf;

	if (path == 0) {
		path = "/";
	}

	if (lifetime > 0) {
		when = time(0) + lifetime;
		mprGmtime(&when, &tm);
		mprRfcTime(dateStr, sizeof(dateStr), &tm);

		//
		//	Other keywords:
		//		Domain=%s
		//
		mprAllocSprintf(&cookieBuf, MPR_HTTP_MAX_HEADER, 
			"Set-Cookie: %s=%s; path=%s; Expires=%s; %s",
			name, value, path, dateStr, secure ? "secure" : "");

	} else {
		mprAllocSprintf(&cookieBuf, MPR_HTTP_MAX_HEADER, 
			"Set-Cookie: %s=%s; path=%s; %s",
			name, value, path, secure ? "secure" : "");
	}

	//
	//	Do not allow multiple cookies
	//
	setHeader(cookieBuf, 0);
	setHeader("Cache-control: no-cache=\"set-cookie\"", 0);
	mprFree(cookieBuf);
}

#endif // BLD_FEATURE_HTTP_COOKIE || BLD_FEATURE_SESSION
////////////////////////////////////////////////////////////////////////////////
#if BLD_FEATURE_SESSION
//
//	Create a new session if one does not already exist. This will allocate a 
//	new session ID and set a cookie in the response header.
//

void MaRequest::createSession(int timeout)
{
	if (session) {
		return;
	}

	session = host->createSession(timeout);
	mprAssert(session);

	sessionId = mprStrdup(session->getId());

	//
	//	Create a cookie that will only live while the browser is not exited.
	//	(Set timeout to zero).
	//
	setCookie(MA_HTTP_SESSION_PREFIX, sessionId, 0, "/", 
#if BLD_FEATURE_SSL_MODULE
		host->isSecure()
#else
		0
#endif
		);
	mprSetPropertyValue(&variables[MA_REQUEST_OBJ], "SESSION_ID", 
		mprCreateStringVar(sessionId, 0));

	/*
	 *	This will cause two more references to exist for the session object.
	 */
	mprCopyVar(&variables[MA_SESSION_OBJ], session->getSessionData(), 0);
	if (variables[MA_GLOBAL_OBJ].type == MPR_TYPE_OBJECT) {
		mprSetProperty(&variables[MA_GLOBAL_OBJ], "session", 
			&variables[MA_SESSION_OBJ]);
	}
}

////////////////////////////////////////////////////////////////////////////////
//
//	Destroy the session. This can be called while other users are still
//	utilizing the session. We take advantage of the MprVar reference counting
//	so the session data store is only destroyed after the last user calls
//	destroySession. NOTE: WebServer is a single-threaded app.
//

void MaRequest::destroySession()
{
	if (session == 0) {
		return;
	}

	//	FUTURE -- need locking here
	mprDestroyVar(&variables[MA_SESSION_OBJ]);
	variables[MA_SESSION_OBJ] = mprCreateUndefinedVar();
	mprDeleteProperty(&variables[MA_REQUEST_OBJ], "SESSION_ID");

	if (variables[MA_GLOBAL_OBJ].type == MPR_TYPE_OBJECT) {
		mprDeleteProperty(&variables[MA_GLOBAL_OBJ], "session");
	}

	//
	//	This is safe even if others are using the session. The sesssionData
	//	is preserved until the last user calls DestroyVar
	//
	host->destroySession(session);

	mprFree(sessionId);
	sessionId = 0;
	session = 0;
}

#endif // BLD_FEATURE_SESSION
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void MaRequest::setPassword(char *password)
{
	mprFree(this->password);
	this->password = mprStrdup(password);
}

////////////////////////////////////////////////////////////////////////////////

void MaRequest::setUser(char *user)
{
	mprFree(this->user);
	this->user = mprStrdup(user);

	setVar(MA_REQUEST_OBJ, "REMOTE_USER", user);
}

////////////////////////////////////////////////////////////////////////////////

void MaRequest::setGroup(char *group)
{
	mprFree(this->group);
	this->group = mprStrdup(group);
}

////////////////////////////////////////////////////////////////////////////////

int MaRequest::blockingWrite(char *buf, int len)
{
	bool	oldMode;
	int		rc;

	oldMode = sock->getBlockingMode();
	sock->setBlockingMode(1);

	rc = sock->write(buf, len);

	if (! oldMode) {
		sock->setBlockingMode(oldMode);
	}

	return rc;
}

////////////////////////////////////////////////////////////////////////////////
///////////////////////////////// MaDataStream /////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

MaDataStream::MaDataStream(char *name, int initial, int max)
{
	this->name = mprStrdup(name);
	buf.setBuf(initial, max);
	size = 0;
}

////////////////////////////////////////////////////////////////////////////////

MaDataStream::~MaDataStream()
{
	mprFree(name);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////// MaHeader ////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

MaHeader::MaHeader()
{
	authType = 0;
	authDetails = 0;
	buf = 0;
	firstLine = 0;
	method = 0;
	proto = 0;
	uri = 0;
	contentMimeType = 0;
	userAgent = 0;
	authType = 0;
	host = 0;
#if BLD_FEATURE_COOKIE || BLD_FEATURE_SESSION
	cookie = 0;
#endif
}

////////////////////////////////////////////////////////////////////////////////

MaHeader::~MaHeader()
{
	reset();
}

////////////////////////////////////////////////////////////////////////////////

void MaHeader::reset()
{
	if (authDetails) {
		mprFree(authDetails);
		authDetails = 0;
	}
	if (authType) {
		mprFree(authType);
		authType = 0;
	}
	if (firstLine) {
		mprFree(firstLine);
		firstLine = 0;
	}
	if (contentMimeType) {
		mprFree(contentMimeType);
		contentMimeType = 0;
	}
	if (userAgent) {
		mprFree(userAgent);
		userAgent = 0;
	}
#if BLD_FEATURE_COOKIE || BLD_FEATURE_SESSION
	if (cookie) {
		mprFree(cookie);
		cookie = 0;
	}
#endif
	if (host) {
		mprFree(host);
		host = 0;
	}

	if (buf) {
		mprFree(buf);
		buf = 0;
	}
	method = 0;
	proto = 0;
	uri = 0;
}

////////////////////////////////////////////////////////////////////////////////

//
// Local variables:
// tab-width: 4
// c-basic-offset: 4
// End:
// vim: sw=4 ts=4 
//
