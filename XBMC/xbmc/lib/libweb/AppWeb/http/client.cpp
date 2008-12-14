///
///	@file 	client.cpp
/// @brief 	Client HTTP facility  
///	@overview MaClient is a flexible client-side HTTP access class. 
///		It supports retries, basic and digest authorization. 
///	@remarks This module is thread-safe.
///	@todo client.cpp: Implement pipelining. pipelining may be hard to 
///	implement because the client object serves two purposed (1. the overall 
///	client interaction object and 2. the lower connection). To implement 
///	pipelining, the connection needs to be separated from the client object. 
///	Probably should do this for Request also.
//
////////////////////////////////// Copyright ///////////////////////////////////
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

#define	IN_CLIENT_LIBRARY 1

#include	"client.h"

///////////////////////////////////// Locals ///////////////////////////////////

static MprList	clients;

////////////////////////////// Forward Declarations ////////////////////////////

static void	 timeoutWrapper(void *data, MprTimer *tp);
static void	 readEventWrapper(void *data, MprSocket *sp, int mask, int isPool);

//////////////////////////////////// Code //////////////////////////////////////

MaClient::MaClient()
{
	authCnonce = 0;
	authNc = 0;
	serverAlgorithm = 0;
	serverDomain = 0;
	serverNonce = 0;
	serverOpaque = 0;
	serverRealm = 0;
	serverQop = 0;
	serverStale = 0;
	serverAuthType = 0;
	callbackArg = 0;
	callback = 0;
	contentLength = 0;
	contentRemaining = -1;
	currentHost = 0;
	currentPort = -1;
	defaultHost = 0;
	defaultPort = -1;
	errorMsg = 0;
	flags = 0;
	fd = -1;

	inBuf = new MprBuf(MPR_HTTP_CLIENT_BUFSIZE + 1, -1);
	headerValues = 0;

	method = 0;
	outBuf = new MprBuf(MPR_HTTP_CLIENT_BUFSIZE, MPR_HTTP_CLIENT_BUFSIZE);
	password = 0;
	proxyHost = 0;
	proxyPort = -1;
	realm = 0;
	retries = MPR_HTTP_CLIENT_RETRIES;
	responseCode = -1;
	responseProto = 0;
	responseContent = new MprBuf(MPR_HTTP_CLIENT_BUFSIZE, -1);
	responseHeader = new MprBuf(MPR_HTTP_CLIENT_BUFSIZE, MPR_HTTP_MAX_HEADER);
	responseText = 0;
	secret = 0;
	sock = 0;
	state = MPR_HTTP_CLIENT_START;
	timeoutPeriod = MPR_HTTP_CLIENT_TIMEOUT;
	timer = 0;
	timestamp = 0;
	user = 0;
	userFlags = MPR_HTTP_KEEP_ALIVE;
	userHeaders = 0;

#if BLD_FEATURE_LOG
	tMod = new MprLogModule("client");
#endif

#if BLD_FEATURE_MULTITHREAD
	completeCond = new MprCond();
	mutex = new MprMutex();
#endif

#if BLD_FEATURE_MULTITHREAD
	//	FUTURE -- ideal case for spin-locks
	mprGetMpr()->lock();
#endif
	clients.insert(this);
#if BLD_FEATURE_MULTITHREAD
	mprGetMpr()->unlock();
#endif
}

////////////////////////////////////////////////////////////////////////////////

MaClient::~MaClient()
{
	//
	//	Must be careful because readEventWrapper is passed "this" and must
	//	not accessed a deleted object. Remove from the list of clients safely
	//	and readEventWrapper will check if it has been deleted.
	//
	mprGetMpr()->lock();
	lock();
	mprGetMpr()->unlock();

	clients.remove(this);

	if (sock) {
		sock->setCallback(readEventWrapper, (void*) this, 0, 0);
		mprLog(3, tMod, "%d: ~MaClient: close sock\n", sock->getFd());
		sock->close(0);
		sock->dispose();
		sock = 0;
	}

	delete headerValues;
	delete inBuf;
	delete outBuf;
	delete responseContent;
	delete responseHeader;

	mprFree(authCnonce);
	mprFree(method);
	mprFree(serverAlgorithm);
	mprFree(serverDomain);
	mprFree(serverNonce);
	mprFree(serverOpaque);
	mprFree(serverRealm);
	mprFree(serverQop);
	mprFree(serverStale);
	mprFree(serverAuthType);
	mprFree(errorMsg);
	mprFree(defaultHost);
	mprFree(proxyHost);
	mprFree(realm);
	mprFree(responseProto);
	mprFree(responseText);
	mprFree(password);
	mprFree(user);
	mprFree(userHeaders);
	mprFree(secret);

	if (timer) {
		timer->stop(MPR_TIMEOUT_STOP);
		timer->dispose();
		timer = 0;
	}

#if BLD_FEATURE_LOG
	delete tMod;
#endif
#if BLD_FEATURE_MULTITHREAD
	delete completeCond;
	delete mutex;
#endif
}

////////////////////////////////////////////////////////////////////////////////
//
//	This reset routine supports keep-alive but not pipelining. ie. the socket
//	and input buffer will be preserved for multiple requests, but the user
//	can have only one request outstanding at a time.
//

void MaClient::reset()
{
	flags = 0;
	if (userFlags & MPR_HTTP_KEEP_ALIVE) {
		flags |= MPR_HTTP_KEEP_ALIVE;
	}
	state = MPR_HTTP_CLIENT_START;
	contentLength = 0;
	contentRemaining = -1;
	responseCode = -1;

	if (method) {
		mprFree(method);
		method = 0;
	}
	if (errorMsg) {
		mprFree(errorMsg);
		errorMsg = 0;
	}

	if (headerValues) {
		delete headerValues;
	}
	headerValues = new MprHashTable(31);

	outBuf->flush();
	responseContent->flush();
	responseHeader->flush();
	if (responseProto) {
		mprFree(responseProto);
		responseProto = 0;
	}
	if (responseText) {
		mprFree(responseText);
		responseText = 0;
	}
	if (timer) {
		timer->stop(MPR_TIMEOUT_STOP);
		timer->dispose();
		timer = 0;
	}
}
 
////////////////////////////////////////////////////////////////////////////////

int MaClient::createSecret()
{
	char	*hex = "0123456789abcdef";
	uchar	bytes[MPR_HTTP_MAX_SECRET];
	char	ascii[MPR_HTTP_MAX_SECRET * 2 + 1], *ap;
	int		i;

	//
	//	Create a random secret for use in authentication. Don't block
	//	waiting for entropy, just take what we can get. Weaker 
	//	cryptographically, but otherwise users who create lots of clients
	//	can block.
	//
	if (mprGetRandomBytes(bytes, sizeof(bytes), 0) < 0) {
		mprAssert(0);
		return MPR_ERR_CANT_INITIALIZE;
	}
	ap = ascii;
	for (i = 0; i < (int) sizeof(bytes); i++) {
		*ap++ = hex[bytes[i] >> 4];
		*ap++ = hex[bytes[i] & 0xf];
	}
	*ap = '\0';
	secret = mprStrdup(ascii);
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

int MaClient::getRequest(char *requestUrl)
{
	return sendRetry("GET", requestUrl, 0, 0);
}

////////////////////////////////////////////////////////////////////////////////

int MaClient::postRequest(char *requestUrl, char *postData, int postLen)
{
	return sendRetry("POST", requestUrl, postData, postLen);
}

////////////////////////////////////////////////////////////////////////////////

int MaClient::headRequest(char *requestUrl)
{
	return sendRetry("HEAD", requestUrl, 0, 0);
}

////////////////////////////////////////////////////////////////////////////////

int MaClient::traceRequest(char *requestUrl)
{
	return sendRetry("TRACE", requestUrl, 0, 0);
}

////////////////////////////////////////////////////////////////////////////////

int MaClient::optionsRequest(char *requestUrl)
{
	return sendRetry("OPTIONS", requestUrl, 0, 0);
}

////////////////////////////////////////////////////////////////////////////////

int MaClient::deleteRequest(char *requestUrl)
{
	return sendRetry("DELETE", requestUrl, 0, 0);
}

////////////////////////////////////////////////////////////////////////////////
//
//	Can only do retries if the caller has not specified a callback
//

int MaClient::sendRetry(char *op, char *requestUrl, char *postData, int postLen)
{
	int		authCount, count, rc;

	authCount = count = 0;
	do {
again:
		rc = sendCore(op, requestUrl, postData, postLen);
		if (rc == 0 && !(flags & MPR_HTTP_TERMINATED)) {
			if (responseCode == 401) {
				//
				//	Carefull altering this code. Ran into GCC compiler bugs.
				//
				if (authCount++ == 0 && user && password && realm) {
					goto again;
				}
				count = retries;
			} else {
				break;
			}
		}
	} while (++count < retries && !mprGetMpr()->isExiting());

	if (rc < 0 && count >= retries) {
		mprError(MPR_L, MPR_LOG, "sendRetry: failed to get %s %s, %d", 
			op, requestUrl, rc);
		return MPR_ERR_TOO_MANY;
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

int MaClient::sendCore(char *method, char *requestUrl, char *postData, 
	int postLen)
{
	char	abuf[MPR_HTTP_MAX_PASS * 2], encDetails[MPR_HTTP_MAX_PASS * 2];
	char	*host;
	int		port, len, rc, nbytes;

	mprAssert(requestUrl && *requestUrl);

	lock();
	reset();

	mprLog(3, tMod, "sendCore: %s %s\n", method, requestUrl);

	this->method = mprStrdup(method);
	timestamp = mprGetTime(0);
	if (timeoutPeriod < 0) {
		timeoutPeriod = MPR_HTTP_CLIENT_TIMEOUT;
	}
	if (timeoutPeriod > 0) {
		if (!mprGetDebugMode()) {
			timer = new MprTimer(MPR_HTTP_TIMER_PERIOD, timeoutWrapper, 
				(void *) this);
		}
	}
	
	if (*requestUrl == '/') {
		url.parse(requestUrl);
		host = (proxyHost) ? proxyHost : defaultHost;
		port = (proxyHost) ? proxyPort : defaultPort;
	} else {
		url.parse(requestUrl);
		host = (proxyHost) ? proxyHost : url.host;
		port = (proxyHost) ? proxyPort : url.port;
	}

	if (sock) {
		if (port != currentPort || strcmp(host, currentHost) != 0) {
			//
			//	This request is for a different host or port. Must close socket.
			//
			sock->close(0);
			sock->dispose();
			sock = 0;
		}
	}

	if (sock == 0) {
		sock = new MprSocket();
		mprLog(3, tMod, "Opening new socket on: %s:%d\n", host, port);
		rc = sock->openClient(host, port, MPR_SOCKET_NODELAY);
		if (rc < 0) {
			mprLog(MPR_ERROR, tMod, "Can't open socket on %s:%d, %d\n", 
				host, port, rc);
			unlock();
			sock->dispose();
			sock = 0;
			return rc;
		}
		sock->setBufSize(-1, MPR_HTTP_CLIENT_BUFSIZE);
		currentHost = mprStrdup(host);
		currentPort = port;

	} else {
		mprLog(3, tMod, "Reusing Keep-Alive socket on: %s:%d\n", host, port);
	}

	//
	//	Remove this flush when pipelining is supported
	//
	inBuf->flush();
	fd = sock->getFd();

	if (proxyHost && *proxyHost) {
		if (url.query && *url.query) {
			outBuf->putFmt("%s http://%s:%d%s?%s HTTP/1.1\r\n",
				method, proxyHost, proxyPort, url.uri, url.query);
		} else {
			outBuf->putFmt("%s http://%s:%d%s HTTP/1.1\r\n",
				method, proxyHost, proxyPort, url.uri);
		}
	} else {
		if (url.query && *url.query) {
			outBuf->putFmt("%s %s?%s HTTP/1.1\r\n", method, url.uri, url.query);
		} else {
			outBuf->putFmt("%s %s HTTP/1.1\r\n", method, url.uri);
		}
	}

	if (serverAuthType) {
		if (strcmp(serverAuthType, "basic") == 0) {
			mprSprintf(abuf, sizeof(abuf), "%s:%s", user, password);
			maEncode64(encDetails, sizeof(encDetails), abuf);
			outBuf->putFmt("Authorization: %s %s\r\n", serverAuthType, 
				encDetails);

#if BLD_FEATURE_DIGEST
		} else if (strcmp(serverAuthType, "digest") == 0) {
			char	a1Buf[256], a2Buf[256], digestBuf[256];
			char	*ha1, *ha2, *digest, *qop;

			authNc++;
			if (secret == 0) {
				if (createSecret() < 0) {
					mprLog(MPR_ERROR, tMod, "Can't create secret\n");
					return MPR_ERR_CANT_INITIALIZE;
				}
			}
			mprFree(authCnonce);
			maCalcNonce(&authCnonce, secret, 0, realm);

			mprSprintf(a1Buf, sizeof(a1Buf), "%s:%s:%s", user, realm, password);
			ha1 = maMD5(a1Buf);

			mprSprintf(a2Buf, sizeof(a2Buf), "%s:%s", method, url.uri);
			ha2 = maMD5(a2Buf);

			qop = (serverQop) ? serverQop : (char*) "";
			if (mprStrCmpAnyCase(serverQop, "auth") == 0) {
				mprSprintf(digestBuf, sizeof(digestBuf), "%s:%s:%08x:%s:%s:%s", 
					ha1, serverNonce, authNc, authCnonce, serverQop, ha2);

			} else if (mprStrCmpAnyCase(serverQop, "auth-int") == 0) {
				mprSprintf(digestBuf, sizeof(digestBuf), "%s:%s:%08x:%s:%s:%s", 
					ha1, serverNonce, authNc, authCnonce, serverQop, ha2);

			} else {
				qop = "";
				mprSprintf(digestBuf, sizeof(digestBuf), "%s:%s:%s", ha1, 
					serverNonce, ha2);
			}

			mprFree(ha1);
			mprFree(ha2);
			digest = maMD5(digestBuf);

			if (*qop == '\0') {
				outBuf->putFmt("Authorization: Digest "
					"username=\"%s\", realm=\"%s\", nonce=\"%s\", "
					"uri=\"%s\", response=\"%s\"\r\n",
					user, realm, serverNonce, url.uri, digest);

			} else if (strcmp(qop, "auth") == 0) {
				outBuf->putFmt("Authorization: Digest "
					"username=\"%s\", realm=\"%s\", domain=\"%s\", "
					"algorithm=\"MD5\", qop=\"%s\", cnonce=\"%s\", "
					"nc=\"%08x\", nonce=\"%s\", opaque=\"%s\", "
					"stale=\"FALSE\", uri=\"%s\", response=\"%s\"\r\n",
					user, realm, serverDomain, serverQop, authCnonce, authNc, 
					serverNonce, serverOpaque, url.uri, digest);

			} else if (strcmp(qop, "auth-int") == 0) {
				;
			}
			mprFree(digest);
#endif // BLD_FEATURE_HTTP_DIGEST
		}
	}

	outBuf->putFmt("Host: %s\r\n", host);
	outBuf->putFmt("User-Agent: %s\r\n", MPR_HTTP_CLIENT_NAME);
	if (userFlags & MPR_HTTP_KEEP_ALIVE) {
		outBuf->putFmt("Connection: Keep-Alive\r\n");
	} else {
		outBuf->putFmt("Connection: close\r\n");
	}
	if (postLen > 0) {
		outBuf->putFmt("Content-Length: %d\r\n", postLen);
	}
	if (postData) {
		outBuf->putFmt("Content-Type: application/x-www-form-urlencoded\r\n");
	}
	if (userHeaders) {
		outBuf->put(userHeaders);
	}
	outBuf->put("\r\n");
	outBuf->addNull();

	//
	//	Flush to the socket with any post data. Writes can fail because the
	//	server prematurely closes a keep-alive connection.
	//
	len = outBuf->getLength();
	if ((rc = sock->write(outBuf->getStart(), len)) != len) {
		flags |= MPR_HTTP_TERMINATED;
		unlock();
		mprLog(MPR_ERROR, tMod, 
			"Can't write to socket on %s:%d, %d\n", host, port, rc);
		return rc;
	}

#if BLD_DEBUG
	mprLog(3, MPR_RAW, tMod, "Request >>>>\n%s\n", outBuf->getStart());
#endif

	if (postData) {
		sock->setBlockingMode(1);
		for (len = 0; len < postLen; ) {
			nbytes = postLen - len;
			rc = sock->write(&postData[len], nbytes);
#if BLD_DEBUG
			mprLog(3, MPR_RAW, tMod, "POST DATA %s\n", &postData[len]);
#endif
			if (rc < 0) {
				unlock();
				mprLog(MPR_ERROR, tMod, 
					"Can't write post data to socket on %s:%d, %d\n", 
					host, port, rc);
				flags |= MPR_HTTP_TERMINATED;
				sock->dispose();
				sock = 0;
				return rc;
			}
			len += rc;
		}
		sock->setBlockingMode(0);
	}
	sock->setCallback(readEventWrapper, (void*) this, 0, MPR_READABLE);

	//
	//	If no callback, then we must block
	//
	if (callback == 0) {
		unlock();
		while (state != MPR_HTTP_CLIENT_DONE) {
			//
			//	If multithreaded and the events thread is not yet running,
			//	we still want to work.
			//
#if BLD_FEATURE_MULTITHREAD
			if (mprGetMpr()->isRunningEventsThread() &&
					mprGetMpr()->poolService->getMaxPoolThreads() > 0) {
				completeCond->waitForCond(250);
			} else
#endif
				mprGetMpr()->serviceEvents(1, 100);
		}
	} else {
		unlock();
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Low level send request. Allow the user to supply complete headers
//

int MaClient::sendRequest(char *host, int port, MprBuf* hdrBuf, char *postData, 
	int postLen)
{
	int		len, rc;

	lock();
	reset();

	mprLog(3, tMod, "sendRequest: %s:%d\n", host, port);
	timestamp = mprGetTime(0);
	if (timeoutPeriod < 0) {
		timeoutPeriod = MPR_HTTP_CLIENT_TIMEOUT;
	}
	if (timeoutPeriod > 0) {
		if (!mprGetDebugMode()) {
			timer = new MprTimer(MPR_HTTP_TIMER_PERIOD, timeoutWrapper, 
				(void *) this);
		}
	}
	
	if (sock == 0) {
		sock = new MprSocket();
		mprLog(3, tMod, "Opening new socket on: %s:%d\n", host, port);
		rc = sock->openClient(host, port, MPR_SOCKET_NODELAY);
		if (rc < 0) {
			mprLog(MPR_ERROR, tMod, "Can't open socket on %s:%d, %d\n", 
				host, port, rc);
			unlock();
			sock->dispose();
			sock = 0;
			return rc;
		}
		sock->setBufSize(-1, MPR_HTTP_CLIENT_BUFSIZE);

	} else {
		mprLog(3, tMod, "Reusing Keep-Alive socket on: %s:%d\n", host, port);
	}

	//
	//	Remove this flush when pipelining is supported
	//
	inBuf->flush();
	fd = sock->getFd();

	//
	//	Flush to the socket with any post data. Writes can fail because the
	//	server prematurely closes a keep-alive connection.
	//
	len = hdrBuf->getLength();
	if ((rc = sock->write(hdrBuf->getStart(), len)) != len) {
		flags |= MPR_HTTP_TERMINATED;
		unlock();
		mprLog(MPR_ERROR, tMod, 
			"Can't write to socket on %s:%d, %d\n", host, port, rc);
		return rc;
	}

	hdrBuf->addNull();

	if (postData) {
		sock->setBlockingMode(1);
		if ((rc = sock->write(postData, postLen)) != postLen) {
			flags |= MPR_HTTP_TERMINATED;
			unlock();
			mprLog(MPR_ERROR, tMod, 
				"Can't write post data to socket on %s:%d, %d\n", 
				host, port, rc);
			return rc;
		}
		sock->setBlockingMode(0);
	}
	sock->setCallback(readEventWrapper, (void*) this, 0, MPR_READABLE);

	//
	//	If no callback, then we must block
	//
	if (callback == 0) {
		unlock();
		while (state != MPR_HTTP_CLIENT_DONE) {
			//
			//	If multithreaded and the events thread is not yet running,
			//	we still want to work.
			//
#if BLD_FEATURE_MULTITHREAD
			if (mprGetMpr()->isRunningEventsThread()) {
				completeCond->waitForCond(250);
			} else
#endif
				mprGetMpr()->serviceEvents(1, 100);
		}
	} else {
		unlock();
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

static void readEventWrapper(void *data, MprSocket *sp, int mask, 
	int isPoolThread)
{
	MaClient	*cp;
	int			moreData, loopCount;

	mprLog(5, "%d: readEventWrapper: mask %x, isPool %d\n", 
		sp->getFd(), mask, isPoolThread);

	//
	//	Make sure we are not being deleted
	//
	mprGetMpr()->lock();
	cp = (MaClient*) clients.getFirst();
	while (cp) {
		if (cp == (MaClient*) data) {
			break;
		}
		cp = (MaClient*) clients.getNext(cp);
	}

	if (cp == 0) {
		mprError(MPR_L, MPR_LOG, "Client deleted prematurely.");
		return;
	}
	cp->lock();
	mprGetMpr()->unlock();

	//
	//	If we are multi-threaded and called on a pool thread, we can block and
	//	read as much data as we can. If single threaded, just do 25 reads.
	//
	loopCount = 25;
	do {
		moreData = cp->readEvent();

		if (cp->getState() == MPR_HTTP_CLIENT_DONE) {
			cp->signalComplete();
			break;
		}

	} while (moreData > 0 && (isPoolThread || loopCount-- > 0));

	cp->unlock();
}

////////////////////////////////////////////////////////////////////////////////
//
//	This routine and all routines called from it below are called with the 
//	lock asserted. Return 1 if socket is still open and usable, 0 if the 
//	socket was closed gracefully, otherwise -1 is returned (disconnect)
//

int MaClient::readEvent()
{
	int		nbytes, len;

	len = inBuf->getLinearSpace();
	if (contentRemaining > 0 && state == MPR_HTTP_CLIENT_CONTENT) {
		len = (contentRemaining > len) ? len: contentRemaining;
	}

	nbytes = sock->read(inBuf->getEnd(), len);

	if (nbytes < 0) {
		mprLog(MPR_ERROR, tMod, "readEvent: %d\n", nbytes);
	} else {
		mprLog(5, tMod, 
			"readEvent: nbytes %d, eof %d\n", nbytes, sock->getEof());
	}

	if (nbytes < 0) {						// Disconnect
		if (state != MPR_HTTP_CLIENT_DONE &&
			   (state != MPR_HTTP_CLIENT_CONTENT || contentRemaining > 0)) {
			flags |= MPR_HTTP_TERMINATED;
		}
		finishRequest(1);
		return -1;
		
	} else if (nbytes == 0) {
		if (sock->getEof()) {				// EOF
			if (state != MPR_HTTP_CLIENT_DONE &&
				   (state != MPR_HTTP_CLIENT_CONTENT || contentRemaining > 0)) {
				flags |= MPR_HTTP_TERMINATED;
			}
			finishRequest(1);
			return -1;

		} else {							// No data available
			//
			//	This happens because we call readEvent multiple times for one
			//	select event so we can read all the data possible
			//
			return 0;
		}

	} else {								// Data available
		inBuf->adjustEnd(nbytes);
		inBuf->addNull();
		processResponseData();
		//	Request may or may not have completed yet
		return 1;
	}
}

////////////////////////////////////////////////////////////////////////////////

int MaClient::processResponseData()
{
	char		*line, *cp;
	int			nbytes;

	mprLog(6, tMod, "READ DATA: %s\n", inBuf->getStart());
	timestamp = mprGetTime(0);

	line = 0;
	while (state != MPR_HTTP_CLIENT_DONE && inBuf->getLength() > 0) {

		line = inBuf->getStart();
		if (state != MPR_HTTP_CLIENT_CONTENT) {
			if ((cp = strchr(line, '\n')) == 0) {
				//	Wait for more data
				return 0;
			}
			*cp = '\0';
			if (cp[-1] == '\r') {
				nbytes = cp - line;
				*--cp = '\0';
			} else {
				nbytes = cp - line;
			}
			inBuf->adjustStart(nbytes + 1);

		} else {
			if (contentLength <= 0) {
				nbytes = inBuf->getLength();
			} else {
				nbytes = min(contentRemaining, inBuf->getLength());
			}
			inBuf->adjustStart(nbytes);
		}

		switch(state) {
		case MPR_HTTP_CLIENT_START:
			mprLog(3, tMod, "processResponseData: %s\n", line);
			if (line[0] == '\0') {
				return 0;
			}
			responseHeader->put(line);
			responseHeader->put('\n');
			if (parseFirst(line) < 0) {
				return MPR_ERR_BAD_STATE;
			}
			state = MPR_HTTP_CLIENT_HEADER;
			break;
		
		case MPR_HTTP_CLIENT_HEADER:
			if (nbytes > 1) {
				mprLog(3, tMod, "processResponseData: %s\n", line);
				responseHeader->put(line);
				responseHeader->put('\n');
				if (parseHeader(line) < 0) {
					return MPR_ERR_BAD_STATE;
				}
			} else {
				//
				//	Blank line means end of headers
				//
				if (flags & MPR_HTTP_INPUT_CHUNKED) {
					if (flags & MPR_HTTP_END_CHUNK_DATA) {
						finishRequest(0);
					} else {
						state = MPR_HTTP_CLIENT_CHUNK;
					}
				} else {
					state = MPR_HTTP_CLIENT_CONTENT;
					//
					//	This means there was an explicit zero content length
					//
					if (contentRemaining == 0) {
						finishRequest(0);
					} else if (mprStrCmpAnyCase(method, "HEAD") == 0) {
						finishRequest(0);
					}
				}
			}
			break;

		case MPR_HTTP_CLIENT_CHUNK:
			mprLog(3, tMod, "processResponseData: %s\n", line);
			contentRemaining = contentLength = mprAtoi(line, 16);
			if (contentLength <= 0) {
				flags |= MPR_HTTP_END_CHUNK_DATA;
				state = MPR_HTTP_CLIENT_HEADER;
			} else {
				state = MPR_HTTP_CLIENT_CONTENT;
			}
			if (contentLength > MPR_HTTP_CLIENT_BUFSIZE) {
				delete responseContent;
				responseContent = new MprBuf(contentLength + 1, -1);
			}
			break;

		case MPR_HTTP_CLIENT_CONTENT:
			responseContent->put((uchar*) line, nbytes);
			responseContent->addNull();
			mprLog(3, tMod, 
				"processResponseData: %d bytes, %d remaining, %d sofar\n", 
				nbytes, contentRemaining, responseContent->getLength());
			if (contentRemaining > 0 || nbytes <= 0) {
				contentRemaining -= nbytes;
				if (contentRemaining <= 0) {
					/* if (!(flags & MPR_HTTP_INPUT_CHUNKED)) */
					finishRequest(0);
				}
			}
			break;

		default:
			formatError("Bad state");
			responseCode = MPR_HTTP_CLIENT_ERROR;
			finishRequest(1);
			return MPR_ERR_BAD_STATE;
		}
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

int MaClient::parseFirst(char *line)
{
	char	*code, *tok;

	mprAssert(line && *line);

	responseProto = mprStrTok(line, " \t", &tok);
	if (responseProto == 0 || responseProto[0] == '\0') {
		formatError("Bad HTTP response");
		responseCode = MPR_HTTP_CLIENT_ERROR;
		finishRequest(1);
		return MPR_ERR_BAD_STATE;
	}
	responseProto = mprStrdup(responseProto);

	if (strncmp(responseProto, "HTTP/1.", 7) != 0) {
		formatError("Unsupported protocol: %s", responseProto);
		responseCode = MPR_HTTP_CLIENT_ERROR;
		finishRequest(1);
		return MPR_ERR_BAD_STATE;
	}

	code = mprStrTok(0, " \t\r\n", &tok);
	if (code == 0 || *code == '\0') {
		formatError("Bad HTTP response");
		responseCode = MPR_HTTP_CLIENT_ERROR;
		finishRequest(1);
		return MPR_ERR_BAD_STATE;
	}
	responseCode = atoi(code);

	responseText = mprStrTok(0, "\r\n", &tok);
	if (responseText && *responseText) {
		responseText = mprStrdup(responseText);
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Parse a line of the response header
// 

int MaClient::parseHeader(char *line)
{
	char	*key, *value, *tok, *tp;

	mprAssert(line && *line);

	if ((key = mprStrTok(line, ": \t\n", &tok)) == 0) {
		formatError("Bad HTTP header");
		responseCode = MPR_HTTP_CLIENT_ERROR;
		finishRequest(1);
		return MPR_ERR_BAD_STATE;
	}
	if ((value = mprStrTok(0, "\n", &tok)) == 0) {
		value = "";
	}
	while (isspace(*value)) {
		value++;
	}

	//	Upper to be consistent with Request?
	mprStrLower(key);
	headerValues->insert(new MprStringHashEntry(key, value));

	if (strcmp("www-authenticate", key) == 0) {
		tp = value;
		while (*value && !isspace(*value)) {
			value++;
		}
		*value++ = '\0';
		mprStrLower(tp);
		mprFree(serverAuthType);
		serverAuthType = mprStrdup(tp);

		if (parseAuthenticate(value) < 0) {
			formatError("Bad Authenticate header");
			responseCode = MPR_HTTP_CLIENT_ERROR;
			finishRequest(1);
			return MPR_ERR_BAD_STATE;
		}

	} else if (strcmp("content-length", key) == 0) {
		contentLength = atoi(value);
		if (mprStrCmpAnyCase(method, "HEAD") != 0) {
			contentRemaining = atoi(value);
		}

	} else if (strcmp("connection", key) == 0) {
		mprStrLower(value);
		if (strcmp(value, "close") == 0) {
			flags &= ~MPR_HTTP_KEEP_ALIVE;
#if BLD_FEATURE_KEEP_ALIVE
		} else if (strcmp(value, "keep-alive") == 0) {
			if (userFlags & MPR_HTTP_KEEP_ALIVE) {
				flags |= MPR_HTTP_KEEP_ALIVE;
			}
#endif
		}
	} else if (strcmp("transfer-encoding", key) == 0) {
		mprStrLower(value);
		if (strcmp(value, "chunked") == 0) {
			flags |= MPR_HTTP_INPUT_CHUNKED;
		}
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

int MaClient::parseAuthenticate(char *authDetails)
{
	char	*value, *tok, *key, *dp, *sp;
	int		seenComma;

	key = authDetails;

	while (*key) {
		while (*key && isspace(*key)) {
			key++;
		}
		tok = key;
		while (*tok && !isspace(*tok) && *tok != ',' && *tok != '=') {
			tok++;
		}
		*tok++ = '\0';

		while (isspace(*tok)) {
			tok++;
		}
		seenComma = 0;
		if (*tok == '\"') {
			value = ++tok;
			while (*tok != '\"' && *tok != '\0') {
				tok++;
			}
		} else {
			value = tok;
			while (*tok != ',' && *tok != '\0') {
				tok++;
			}
			seenComma++;
		}
		*tok++ = '\0';

		//
		//	Handle back-quoting
		//
		if (strchr(value, '\\')) {
			for (dp = sp = value; *sp; sp++) {
				if (*sp == '\\') {
					sp++;
				}
				*dp++ = *sp++;
			}
			*dp = '\0';
		}

		//
		//	algorithm, domain, nonce, oqaque, realm, qop, stale
		//	
		switch (tolower(*key)) {
		case 'a':
			if (mprStrCmpAnyCase(key, "algorithm") == 0) {
				mprFree(serverAlgorithm);
				serverAlgorithm = mprStrdup(value);
				break;
//			} else if (mprStrCmpAnyCase(key, "server-param") == 0) {
//				break;
			}
			break;

		case 'd':
			if (mprStrCmpAnyCase(key, "domain") == 0) {
				mprFree(serverDomain);
				serverDomain = mprStrdup(value);
				break;
			}
			break;

		case 'n':
			if (mprStrCmpAnyCase(key, "nonce") == 0) {
				mprFree(serverNonce);
				serverNonce = mprStrdup(value);
				authNc = 0;
			}
			break;

		case 'o':
			if (mprStrCmpAnyCase(key, "opaque") == 0) {
				mprFree(serverOpaque);
				serverOpaque = mprStrdup(value);
			}
			break;

		case 'q':
			if (mprStrCmpAnyCase(key, "qop") == 0) {
				mprFree(serverQop);
				serverQop = mprStrdup(value);
			}
			break;

		case 'r':
			if (mprStrCmpAnyCase(key, "realm") == 0) {
				mprFree(serverRealm);
				serverRealm = mprStrdup(value);
			}
			break;

		case 's':
			if (mprStrCmpAnyCase(key, "stale") == 0) {
				mprFree(serverStale);
				serverStale = mprStrdup(value);
				break;
			}
		
		default:
			//	For upward compatibility --  ignore keywords we don't understand
			;
		}
		key = tok;
		if (!seenComma) {
			while (*key && *key != ',') {
				key++;
			}
			if (*key) {
				key++;
			}
		}
	}
	if (strcmp(serverAuthType, "basic") == 0) {
		if (serverRealm == 0) {
			return MPR_ERR_BAD_ARGS;
		} else {
			return 0;
		}
	}
	if (serverRealm == 0 || serverNonce == 0) {
		return MPR_ERR_BAD_ARGS;
	}
	if (serverQop) {
		//	FUTURE -- add checking for auth-int here
		if (serverDomain == 0 || serverOpaque == 0 || 
				serverAlgorithm == 0 || serverStale == 0) {
			return MPR_ERR_BAD_ARGS;
		}
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

int MaClient::getResponseCode()
{
	if (state != MPR_HTTP_CLIENT_DONE) {
		return MPR_ERR_BAD_STATE;
	}
	return responseCode;
}

////////////////////////////////////////////////////////////////////////////////

char *MaClient::getResponseContent(int *contentLen)
{
	mprAssert(contentLen);

	if (state != MPR_HTTP_CLIENT_DONE) {
		return 0;
	}
	if (contentLen) {
		*contentLen = responseContent->getLength();
	}
	if (responseContent->getStart() == 0) {
		return "";
	} else {
		return responseContent->getStart();
	}
}

////////////////////////////////////////////////////////////////////////////////

char *MaClient::getResponseHeader()
{
	if (state != MPR_HTTP_CLIENT_DONE) {
		return 0;
	}
	if (responseHeader->getStart() == 0) {
		return "";
	} else {
		return responseHeader->getStart();
	}
}

////////////////////////////////////////////////////////////////////////////////

char *MaClient::getResponseMessage()
{
	if (state != MPR_HTTP_CLIENT_DONE) {
		return "";
	}
	return (errorMsg == 0) ? (char*) "" : errorMsg;
}

////////////////////////////////////////////////////////////////////////////////

char *MaClient::getHeaderVar(char *key)
{
	MprStringHashEntry	*hp;

	hp = (MprStringHashEntry*) headerValues->lookup(key);
	return hp->getValue();
}

////////////////////////////////////////////////////////////////////////////////

void MaClient::finishRequest(bool closeSocket)
{
	mprLog(3, tMod, "finishRequest: closeSocket %d, code %d\n", 
		closeSocket, responseCode);

	if (timer) {
		timer->stop(MPR_TIMEOUT_STOP);
		timer->dispose();
		timer = 0;
	}

	state = MPR_HTTP_CLIENT_DONE;
	responseContent->addNull();
	responseHeader->addNull();

	if (sock) {
		if (flags & MPR_HTTP_KEEP_ALIVE && !closeSocket) {
			mprLog(3, tMod, "finishRequest: Attempting keep-alive\n");
			sock->setCallback(readEventWrapper, (void*) this, 0, 0);
		} else {
			mprLog(3, tMod, "%d: finishRequest: Close socket\n", sock->getFd());
			sock->close(0);
			sock->dispose();
			sock = 0;
		}
	}

	if (callback) {
		(callback)(this, callbackArg);
	}
}

////////////////////////////////////////////////////////////////////////////////
//	
//	Forman an error message 
// 

void MaClient::formatError(char *fmt, ...)
{
	va_list		args;

	mprAssert(fmt);

	if (errorMsg) {
		mprFree(errorMsg);
	}
	va_start(args, fmt);
	mprAllocVsprintf(&errorMsg, MPR_MAX_STRING, fmt, args);
	va_end(args);
}

////////////////////////////////////////////////////////////////////////////////

void MaClient::setProxy(char *host, int port)
{
	if (proxyHost) {
		mprFree(proxyHost);
	}
	proxyHost = mprStrdup(host);
	proxyPort = port;
}

////////////////////////////////////////////////////////////////////////////////

static void timeoutWrapper(void *data, MprTimer *tp)
{
	MaClient	*cp;

	cp = (MaClient*) data;
	cp->timeout(tp);
}

////////////////////////////////////////////////////////////////////////////////

void MaClient::timeout(MprTimer *tp)
{
	int			now;

	now = mprGetTime(0);
	if (now >= (timestamp + timeoutPeriod)) {
		mprError(MPR_L, MPR_LOG, "Timeout for %s", url.uri);
		lock();
		if (timer) {
			timer->dispose();
			timer = 0;
		}
		responseCode = 408;
		formatError("Timeout");
		finishRequest(1);
#if BLD_FEATURE_MULTITHREAD
		if (callback == 0) {
			completeCond->signalCond();
		}
#endif
		//
		//	finishRequest will have deleted the timer
		//
		unlock();
		return;
	}
	tp->reschedule(MPR_HTTP_TIMER_PERIOD);
}

////////////////////////////////////////////////////////////////////////////////

void MaClient::setTimeout(int timeout)
{
	timeoutPeriod = timeout;
}

////////////////////////////////////////////////////////////////////////////////

void MaClient::setCallback(MaClientProc fn, void *arg)
{
	callback = fn;
	callbackArg = arg;
}

////////////////////////////////////////////////////////////////////////////////

void MaClient::getParsedUrl(MaUrl **urlp)
{
	mprAssert(urlp);

	if (urlp) {
		*urlp = &url;
	}
}

////////////////////////////////////////////////////////////////////////////////

int MaClient::getState()
{
	return state;
}

////////////////////////////////////////////////////////////////////////////////

int MaClient::getFlags()
{
	return flags;
}

////////////////////////////////////////////////////////////////////////////////

void MaClient::setKeepAlive(bool on)
{
	lock();
	if (on) {
		userFlags |= MPR_HTTP_KEEP_ALIVE;
	} else {
		userFlags = ~MPR_HTTP_KEEP_ALIVE;
	}
	unlock();
}

////////////////////////////////////////////////////////////////////////////////

void MaClient::setRetries(int num)
{
	retries = num;
}

////////////////////////////////////////////////////////////////////////////////

int MaClient::getPort()
{
	return defaultPort;
}

////////////////////////////////////////////////////////////////////////////////

char *MaClient::getHost()
{
	return defaultHost;
}

////////////////////////////////////////////////////////////////////////////////

void MaClient::setPort(int num)
{
	defaultPort = num;
}

////////////////////////////////////////////////////////////////////////////////

void MaClient::setHost(char *host)
{
	if (defaultHost) {
		mprFree(defaultHost);
	}
	defaultHost = mprStrdup(host);
}

////////////////////////////////////////////////////////////////////////////////

void MaClient::setAuth(char *realm, char *username, char *password)
{
	mprFree(this->user);
	mprFree(this->realm);
	mprFree(this->password);
	this->realm = mprStrdup(realm);
	this->user = mprStrdup(username);
	this->password = mprStrdup(password);
}

////////////////////////////////////////////////////////////////////////////////

void MaClient::resetAuth()
{
	mprFree(serverAuthType);
	serverAuthType = 0;
}

////////////////////////////////////////////////////////////////////////////////

void MaClient::setUserHeaders(char *headers)
{
	mprFree(userHeaders);
	userHeaders = mprStrdup(headers);
}

////////////////////////////////////////////////////////////////////////////////

void MaClient::signalComplete()
{
#if BLD_FEATURE_MULTITHREAD
	completeCond->signalCond();
#endif
}

////////////////////////////////////////////////////////////////////////////////

//
// Local variables:
// tab-width: 4
// c-basic-offset: 4
// End:
// vim: sw=4 ts=4 
//
