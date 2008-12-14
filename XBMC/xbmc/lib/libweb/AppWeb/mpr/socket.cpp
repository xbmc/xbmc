///
///	@file 	socket.cpp
/// @brief 	Convenience class for the management of sockets
/// @overview This module provides a higher level C++ interface 
///		to interact with the standard sockets API. It does not 
///		perform buffering.
///	@remarks This module is thread-safe.
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
/////////////////////////////////// Includes ///////////////////////////////////

#include	"mpr.h"

////////////////////////////////////////////////////////////////////////////////

static void ioProcWrapper(void *data, int mask, int isMprPoolThread);
static void acceptProcWrapper(void *data, int mask, int isMprPoolThread);

////////////////////////////////////////////////////////////////////////////////
/////////////////////////////// MprSocketService ///////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
//	Open socket service
//

MprSocketService::MprSocketService()
{
	maxClients = 0;
#if BLD_FEATURE_LOG
	log = new MprLogModule("socket");
#endif

#if BLD_FEATURE_MULTITHREAD
	mutex = new MprMutex();
#endif
}

////////////////////////////////////////////////////////////////////////////////
//
//	Close the socket service
//

MprSocketService::~MprSocketService()
{
	lock();
#if BLD_DEBUG
	if (socketList.getNumItems() > 0) {
		MprSocket	*sp;
		mprError(MPR_L, MPR_LOG, "Exiting with %d sockets unfreed",
			socketList.getNumItems());
		sp = (MprSocket*) socketList.getFirst();
		while (sp) {
			mprLog(2, "~MprSocketService: open socket %d, sp %x\n", 
				sp->getFd(), sp);
			sp = (MprSocket*) socketList.getNext(sp);
		}
	}
#endif

#if DEPRECATED
{
	MprInterface	*ip, *nextIp;

	ip = (MprInterface*) ipList.getFirst();
	while (ip) {
		nextIp = (MprInterface*) ipList.getNext(ip);
		ipList.remove(ip);
		delete ip;
		ip = nextIp;
	}
}
#endif

	delete log;
#if BLD_FEATURE_MULTITHREAD
	delete mutex;
#endif
}

////////////////////////////////////////////////////////////////////////////////
//
//	Start the socket service
//

int MprSocketService::start()
{
	Mpr		*mpr;
	char	hostName[MPR_MAX_IP_NAME];
	char	serverName[MPR_MAX_IP_NAME];
	char	domainName[MPR_MAX_IP_NAME];
	char	*dp;

	mpr = mprGetMpr();

	serverName[0] = '\0';
	domainName[0] = '\0';
	hostName[0] = '\0';

	if (gethostname(serverName, sizeof(serverName)) < 0) {
		mprStrcpy(serverName, sizeof(serverName), "localhost");
		mprError(MPR_L, MPR_USER, "Can't get host name");
		// Keep going
	}
	if ((dp = strchr(serverName, '.')) != 0) {
		mprStrcpy(hostName, sizeof(hostName), serverName);
		*dp++ = '\0';
		mprStrcpy(domainName, sizeof(domainName), dp);
	} else {
		mprStrcpy(hostName, sizeof(hostName), serverName);
	}

	lock();
	mpr->setServerName(serverName);
	mpr->setDomainName(domainName);
	mpr->setHostName(hostName);

#if DEPRECATED
	getInterfaces();
#endif

	unlock();
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Stop the socket service. Must be idempotent.
//

int MprSocketService::stop()
{
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Add a socket to the socket list
//

void MprSocketService::insertMprSocket(MprSocket *sp)
{
	lock();
	socketList.insert(sp); 
	unlock();
}

////////////////////////////////////////////////////////////////////////////////
//
//	Remove a socket from the socket list
//

void MprSocketService::removeMprSocket(MprSocket *sp) 
{
	lock();
	socketList.remove(sp); 
	unlock();
}

////////////////////////////////////////////////////////////////////////////////

int MprSocketService::getNumClients()
{
	MprSocket	*sp;
	int			count;

	count = 0;

	lock();
	sp = (MprSocket*) socketList.getFirst();
	while (sp) {
		if (! (sp->flags & MPR_SOCKET_LISTENER)) {
			count++;
		}
		sp = (MprSocket*) socketList.getNext(sp);
	}
	unlock();
	return count;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////// MprSocket ///////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
//	Allocate a socket class
//

MprSocket::MprSocket()
{
	Mpr		*mpr;

	mpr = mprGetMpr();

	acceptCallback = 0;
	acceptData = 0;
	currentEvents = 0;
	error = 0;
	flags = 0;
	handler = 0;
	handlerMask = 0;
	handlerPriority = MPR_NORMAL_PRIORITY;
	interestEvents = 0;
	inUse = 0;
	ioCallback = 0;
	ioData = 0;
	ioData2 = 0;
	ipAddr = 0;
	port = -1;

	//	TODO - not used
	secure = 0;
	selectEvents = 0;
	sock = -1;

#if BLD_FEATURE_MULTITHREAD
	mutex = new MprMutex();
#endif

#if BLD_FEATURE_LOG
	log = mpr->socketService->getLogModule();
	mprLog(7, log, "0: new MprSocket\n");
#endif

	mpr->socketService->insertMprSocket(this);
}

////////////////////////////////////////////////////////////////////////////////

bool MprSocket::dispose()
{
	lock();
	if (!(flags & MPR_SOCKET_DISPOSED)) {
		if (handler) {
			handler->dispose();
			handler = 0;
		}
	}
	flags |= MPR_SOCKET_DISPOSED;
	mprLog(8, log, "dispose: inUse %d\n", inUse);
	if (inUse == 0) {
		delete this;
	} else {
		unlock();
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Destroy a socket. Must be locked on entry
//	FUTURE. Would be great to be able to do mprAssert(mutex.isLocked());
//

MprSocket::~MprSocket()
{
	mprLog(8, log, "~MprSocket: Destroying\n");
	if (sock >= 0) {
		mprLog(7, log, "%d: ~MprSocket: closing %x\n", sock);
		this->close(MPR_SOCKET_LINGER);
		sock = 0;
	}
	mprGetMpr()->socketService->removeMprSocket(this);
	if (ipAddr) {
		mprFree(ipAddr);
		ipAddr = 0;
	}
#if BLD_FEATURE_MULTITHREAD
	delete mutex;
#endif
}

////////////////////////////////////////////////////////////////////////////////

MprSocket *MprSocket::newSocket()
{
	return new MprSocket();
}

////////////////////////////////////////////////////////////////////////////////
//
//	Open a server socket connection
//
//	IPv6 addresses are distinguished by starting with "[". 
//	If addr is "" or "[]", then it will bind to all interfaces.
//

int MprSocket::openServer(char *addr, int portNum, MprSocketAcceptProc acceptFn, 
						  void *data, int initialFlags)
{
#if BLD_FEATURE_IPV6
	struct addrinfo		hints, *res;
	struct sockaddr_storage	sockAddr6;
	char 				portNumString[MPR_MAX_IP_PORT];
	char 				addrBuf[MPR_MAX_IP_ADDR];
	char 				*bindName;
#endif
	struct sockaddr_in	sockAddr;
	struct sockaddr 	*sa;
	struct hostent		*hostent;
	MprSocklen 			addrlen;
	int					datagram, rc;

	mprAssert(addr);

	if (addr == 0 || *addr == '\0') {
		mprLog(6, log, "openServer: *:%d, flags %x\n", portNum, initialFlags);
	} else {
		mprLog(6, log, "openServer: %s:%d, flags %x\n", addr, portNum, initialFlags);
	}


#if BLD_FEATURE_IPV6

	if (addr[0] == '[') {
		ipv6 = 1;
		mprStrcpy(addrBuf, sizeof(addrBuf), &addr[1]);
		mprAssert(addrBuf[strlen(addrBuf) - 1] == ']');
		addrBuf[strlen(addrBuf) - 1] = '\0';
		addr = addrBuf;
	}

	if (ipv6) {
		memset((char *) &hints, '\0', sizeof(hints));
		memset((char *) &sockAddr6, '\0', sizeof(struct sockaddr_storage));

		mprSprintf(portNumString, sizeof(portNumString), "%d", portNum);

		hints.ai_socktype = SOCK_STREAM;
		hints.ai_family = AF_INET6;

		if (strcmp(addr, "") != 0) {
			bindName = addr;
		} else {
			bindName = NULL;
			hints.ai_flags |= AI_PASSIVE; 				/* Bind to 0.0.0.0 and :: */
														/* Sets to IN6ADDR_ANY_INIT */
		}
			
		rc = getaddrinfo(bindName, portNumString, &hints, &res);
		if (rc) {
			return MPR_ERR_CANT_OPEN;

		}
		sa = (struct sockaddr*) &sockAddr6;
		memcpy(sa, res->ai_addr, res->ai_addrlen);
		addrlen = res->ai_addrlen;
		freeaddrinfo(res);

	} else 
#endif
	{
		/*
		 *	TODO could we use getaddrinfo in all cases. ie. merge with IPV6 code
		 */
		memset((char *) &sockAddr, '\0', sizeof(struct sockaddr_in));
		addrlen = sizeof(struct sockaddr_in);
		sockAddr.sin_family = AF_INET;

		sockAddr.sin_port = htons((short) (portNum & 0xFFFF));
		if (strcmp(addr, "") != 0) {
			sockAddr.sin_addr.s_addr = inet_addr(addr);
			if (sockAddr.sin_addr.s_addr == INADDR_NONE) {
				hostent = mprGetHostByName(addr);
				if (hostent != 0) {
					memcpy((char*) &sockAddr.sin_addr, (char*) hostent->h_addr_list[0], 
						(size_t) hostent->h_length);
					mprFreeGetHostByName(hostent);
				} else {
					return MPR_ERR_NOT_FOUND;
				}
			}
		} else {
			sockAddr.sin_addr.s_addr = INADDR_ANY;
		}
		sa = (struct sockaddr*) &sockAddr;
	}

	lock();
	port = portNum;
	acceptCallback = acceptFn;
	acceptData = data;

	flags = (initialFlags & 
		(MPR_SOCKET_BROADCAST | MPR_SOCKET_DATAGRAM | MPR_SOCKET_BLOCK | 
		 MPR_SOCKET_LISTENER | MPR_SOCKET_NOREUSE | MPR_SOCKET_NODELAY));

	ipAddr = mprStrdup(addr);

	datagram = flags & MPR_SOCKET_DATAGRAM;

	//
	//	Create the O/S socket
	//
	sock = socket(sa->sa_family, datagram ? SOCK_DGRAM: SOCK_STREAM, 0);
	if (sock < 0) {
		unlock();
		return MPR_ERR_CANT_OPEN;
	}
#if !WIN && !VXWORKS
	fcntl(sock, F_SETFD, FD_CLOEXEC);		// Children won't inherit this fd
#endif

#if CYGWIN || LINUX || MACOSX || VXWORKS || FREEBSD
	if (!(flags & MPR_SOCKET_NOREUSE)) {
		rc = 1;
		setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char*) &rc, sizeof(rc));
	}
#endif

	rc = bind(sock, sa, addrlen);
	// rc = bind(sock, res->ai_addr, res->ai_addrlen);
	if (rc < 0) {
		int err = errno;
		err = err;
		::closesocket(sock);
		sock = -1;
		unlock();
		return MPR_ERR_CANT_OPEN;
	}

	if (! datagram) {
		flags |= MPR_SOCKET_LISTENER;
		if (listen(sock, SOMAXCONN) < 0) {
			::closesocket(sock);
			sock = -1;
			unlock();
			return MPR_ERR_CANT_OPEN;
		}
		handler = new MprSelectHandler(sock, MPR_SOCKET_READABLE, 
			(MprSelectProc) acceptProcWrapper, (void*) this, handlerPriority);
	}
	handlerMask |= MPR_SOCKET_READABLE;

#if WIN
	//
	//	Delay setting reuse until now so that we can be assured that we
	//	have exclusive use of the port.
	//
	if (!(flags & MPR_SOCKET_NOREUSE)) {
		rc = 1;
		setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char*) &rc, sizeof(rc));
	}
#endif

	setBlockingMode((bool) (flags & MPR_SOCKET_BLOCK));

	//
	//	TCP/IP stacks have the No delay option (nagle algorithm) on by default.
	//
	if (flags & MPR_SOCKET_NODELAY) {
		setNoDelay(1);
	}
	unlock();
	return sock;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Open a client socket connection
//

int MprSocket::openClient(char *addr, int portNum, int initialFlags)
{
#if BLD_FEATURE_IPV6
	struct addrinfo 	hints, *res;
	struct sockaddr_storage	remoteAddr6;
	char 				portNum_string[MPR_MAX_IP_PORT];
	char 				addrBuf[MPR_MAX_IP_ADDR];
#endif
	struct sockaddr_in	remoteAddr;
	struct hostent		*hostent;
	struct sockaddr 	*sa;
	MprSocklen			addrlen;
	int					broadcast, datagram, rc, err;

	mprLog(6, log, "openClient: %s:%d, flags %x\n", addr, portNum, 
		initialFlags);

#if BLD_FEATURE_IPV6
	if (addr[0] == '[') {
		ipv6 = 1;
		mprStrcpy(addrBuf, sizeof(addr), &addr[1]);
		mprAssert(addrBuf[strlen(addrBuf) - 2] == ']');
		addrBuf[strlen(addrBuf) - 2] = '\0';
		addr = addrBuf;
	}

	if (ipv6) {
		memset((char*) &hints, '\0', sizeof(hints));
		memset((char*) &remoteAddr6, '\0', sizeof(struct sockaddr_storage));

		mprSprintf(portNum_string, sizeof(portNum_string), "%d", portNum);

		hints.ai_socktype = SOCK_STREAM;

		rc = getaddrinfo(addr, portNum_string, &hints, &res);
		if (rc) {
			/* no need to unlock yet */
			return MPR_ERR_CANT_OPEN;

		}
		sa = (struct sockaddr*) &remoteAddr6;
		memcpy(sa, res->ai_addr, res->ai_addrlen);
		addrlen = res->ai_addrlen;
		freeaddrinfo(res);
		
	} else 
#endif
	{
		memset((char *) &remoteAddr, '\0', sizeof(struct sockaddr_in));
		remoteAddr.sin_family = AF_INET;
		remoteAddr.sin_port = htons((short) (portNum & 0xFFFF));
		sa = (struct sockaddr*) &remoteAddr;
		addrlen = sizeof(remoteAddr);
	}
		
	lock();
	port = portNum;
	flags = (initialFlags & 
		(MPR_SOCKET_BROADCAST | MPR_SOCKET_DATAGRAM | MPR_SOCKET_BLOCK | 
		 MPR_SOCKET_LISTENER | MPR_SOCKET_NOREUSE | MPR_SOCKET_NODELAY));

	//	Save copy of the address
	ipAddr = mprStrdup(addr);

#if BLD_FEATURE_IPV6
	if (!ipv6) {
		// Nothing here
	} else 
#endif
	{
		remoteAddr.sin_addr.s_addr = inet_addr(ipAddr);
		if (remoteAddr.sin_addr.s_addr == INADDR_NONE) {
			hostent = mprGetHostByName(ipAddr);
			if (hostent != 0) {
				memcpy((char*) &remoteAddr.sin_addr, (char*) hostent->h_addr_list[0], 
					(size_t) hostent->h_length);
				mprFreeGetHostByName(hostent);
			} else {
				unlock();
				return MPR_ERR_NOT_FOUND;
			}
		}
	}

	broadcast = flags & MPR_SOCKET_BROADCAST;
	if (broadcast) {
		flags |= MPR_SOCKET_DATAGRAM;
	}
	datagram = flags & MPR_SOCKET_DATAGRAM;

	//
	//	Create the O/S socket
	//
	sock = socket(sa->sa_family, datagram ? SOCK_DGRAM: SOCK_STREAM, 0);
	if (sock < 0) {
		err = getError();
		unlock();
		return -err;
	}
#if !WIN && !VXWORKS
	fcntl(sock, F_SETFD, FD_CLOEXEC);		// Children won't inherit this fd
#endif

	if (broadcast) {
		int	flag = 1;
		if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (char *) &flag, sizeof(flag)) < 0) {
			err = getError();
			::closesocket(sock);
			sock = -1;
			unlock();
			return -err;
		}
	}
 
	if (!datagram) {
		flags |= MPR_SOCKET_CONNECTING;
		rc = connect(sock, sa, addrlen);
		if (rc < 0) {
			err = getError();
			::closesocket(sock);
			sock = -1;
			unlock();
#if UNUSED
			//
			//	If the listen backlog is too high, ECONNREFUSED is returned
			//
			if (err == EADDRINUSE || err == ECONNREFUSED) {
				return MPR_ERR_BUSY;
			}
#endif
			return -err;
		}
	}

	setBlockingMode((bool) (flags & MPR_SOCKET_BLOCK));

	//
	//	TCP/IP stacks have the No delay option (nagle algorithm) on by default.
	//
	if (flags & MPR_SOCKET_NODELAY) {
		setNoDelay(1);
	}
	unlock();
	return sock;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Close a socket (gracefully)
//

void MprSocket::close(int timeout)
{
	MprSelectService	*ss;
	Mpr					*mpr;
	char				buf[1024];
	int					handlerFlags, timesUp;

	mpr = mprGetMpr();

	mprLog(7, log, "%d: close\n", sock);
	ss = mpr->selectService;

	lock();
	mprAssert(!(flags & MPR_SOCKET_CLOSED));
	if (flags & MPR_SOCKET_CLOSED) {
		unlock();
		return;
	}
	flags |= MPR_SOCKET_CLOSED;
	handlerFlags = (handler) ? handler->getFlags() : 0;

	if (handler) {
		handler->dispose();
		handler = 0;
	}

	if (sock >= 0) {
		//
		//	Do a graceful shutdown. Read any outstanding read data to prevent
		//	resets. Then do a shutdown to send a FIN and read outstanding 
		//	data. All non-blocking.
		//
#if WIN
		if (ss->getFlags() & MPR_ASYNC_SELECT) {

			if (handlerFlags & MPR_SELECT_CLIENT_CLOSED) {
				//
				//	Client initiated close. We have already received an FD_CLOSE
				//
				closesocket(sock);
				sock = -1;

			} else {

				if (shutdown(sock, SHUT_WR) == 0) {
					//
					//	Do a graceful shutdown. Read any outstanding read data to 
					//	prevent resets. Then do a shutdown to send a FIN and lastly
					//	read data when the FD_CLOSE is received (see select.cpp). 
					//	All done non-blocking.
					//
					timesUp = mprGetTime(0) + timeout;
					do {
    					if (recv(sock, buf, sizeof(buf), 0) <= 0) {
							break;
						}
					} while (mprGetTime(0) < timesUp);
				}

				//
				//	Delayed close call must be first so we are ready when the
				//	FD_CLOSE arrives. Other way round and there is a race if 
				//	multi-threaded. 
				//
				ss->delayedClose(sock);

				//
				//	We need to ensure we receive an FD_CLOSE to complete the
				//	delayed close. Despite disposing the hander above, socket 
				//	messages will still be sent from windows and so select can 
				//	cleanup the delayed close socket.
				//
				WSAAsyncSelect(sock, ss->getHwnd(), ss->getMessage(), FD_CLOSE);
			}
		
		} else {
#endif
			if (shutdown(sock, SHUT_WR) < 0) {
				ss->delayedClose(sock);

			} else {
				setBlockingMode(0);
				timesUp = mprGetTime(0) + timeout;
				do {
					if (recv(sock, buf, sizeof(buf), 0) <= 0) {
						break;
					}
					
				} while (mprGetTime(0) < timesUp);
			}

			//
			//	Use delayed close to prevent anyone else reusing the socket
			//	while select has not fully cleaned it out of its masks.
			//
			ss->delayedClose(sock);
			ss->awaken(0);
		}
#if WIN
	}
#endif

	//
	//	Re-initialize all socket variables so the Socket can be reused.
	//
	acceptCallback = 0;
	acceptData = 0;
	selectEvents = 0;
	currentEvents = 0;
	error = 0;
	flags = MPR_SOCKET_CLOSED;
	ioCallback = 0;
	ioData = 0;
	ioData2 = 0;
	handlerMask = 0;
	handlerPriority = MPR_NORMAL_PRIORITY;
	interestEvents = 0;
	port = -1;
	sock = -1;

	if (ipAddr) {
		mprFree(ipAddr);
		ipAddr = 0;
	}

	unlock();
}

////////////////////////////////////////////////////////////////////////////////
//
//	Forcibly close a socket. Used to unblock a blocking read / write.
//

void MprSocket::forcedClose()
{
	mprLog(9, log, "%d: forcedClose\n", sock);

	//
	//	Delay calling lock until we call close() below as we wan't to ensure
	//	we don't block before we get to call close.
	//
	lock();

	if (sock >= 0) {
#if CYGWIN || LINUX || MACOSX || SOLARIS || VXWORKS || FREEBSD
		shutdown(sock, MPR_SHUTDOWN_BOTH);
#endif
		::closesocket(sock);
		sock = -1;
		this->close(MPR_SOCKET_LINGER);
	}

	unlock();
}

////////////////////////////////////////////////////////////////////////////////
//
//	Accept handler. May be called directly if single-threaded or on a pool
//	thread.
//

static void acceptProcWrapper(void *data, int mask, int isMprPoolThread)
{
	MprSocket		*sp;

	sp = (MprSocket*) data;
	sp->acceptProc(isMprPoolThread);
}

////////////////////////////////////////////////////////////////////////////////
//
//	Accept an incoming connection. (private)
//

void MprSocket::acceptProc(int isMprPoolThread)
{
	MprSocket			*nsp;
	MprSocketService	*ss;
#if BLD_FEATURE_IPV6
	struct sockaddr_storage	addr6;
	char 				hbuf[NI_MAXHOST]; char sbuf[NI_MAXSERV];
	int 				rc;
#endif
	struct sockaddr_in	addr;
	struct sockaddr 	*sa;
	char				callerIpAddr[MPR_MAX_IP_ADDR];
	MprSocklen			addrlen;
	int					fd, max, numClients;

	if (acceptCallback == 0) {
		return;
	}

	lock();

#if BLD_FEATURE_IPV6
	if (ipv6) {
		sa = (struct sockaddr*) &addr6;
		addrlen = sizeof(addr6);
	} else
#endif
	{
		sa = (struct sockaddr*) &addr;
		addrlen = sizeof(addr);
	}

	fd = accept(sock, sa, (SocketLenPtr) &addrlen);
	if (fd < 0) {
		mprLog(0, log, "%d: acceptProc: accept failed %d\n", sock, getError());
		unlock();
		return;
	}
#if !WIN && !VXWORKS
	fcntl(fd, F_SETFD, FD_CLOEXEC);		// Prevent children inheriting
#endif

	ss = mprGetMpr()->socketService;
	max = ss->getMaxClients();
	numClients = ss->getNumClients();

	if (max > 0 && numClients >= max) {
		mprLog(3, "Rejecting connection, too many client connections (%d)\n", numClients);
#if CYGWIN || LINUX || MACOSX || SOLARIS || VXWORKS || FREEBSD
		shutdown(fd, MPR_SHUTDOWN_BOTH);
#endif
		::closesocket(fd);
		mprGetMpr()->selectService->awaken(0);
		unlock();
		return;
	}

	nsp = newSocket();

	nsp->lock();
	nsp->sock = fd;
	nsp->ipAddr = mprStrdup(ipAddr);
	nsp->acceptData = acceptData;
	nsp->ioData = ioData;
	nsp->ioData2 = ioData2;
	nsp->port = port;
	nsp->acceptCallback = acceptCallback;
	nsp->flags = flags;
	nsp->flags &= ~MPR_SOCKET_LISTENER;

	nsp->setBlockingMode((nsp->flags & MPR_SOCKET_BLOCK) ? 1: 0);

	if (nsp->flags & MPR_SOCKET_NODELAY) {
		nsp->setNoDelay(1);
	}
	nsp->inUse++;
	mprLog(6, log, "%d: acceptProc: isMprPoolThread %d, newSock %d\n", sock, isMprPoolThread, fd);

	nsp->unlock();

	//
	//	Call the user accept callback.
	//
#if BLD_FEATURE_IPV6
	if (ipv6) {
		rc = getnameinfo(sa, addrlen, hbuf, sizeof(hbuf), sbuf, sizeof(sbuf),
			NI_NUMERICHOST | NI_NUMERICSERV);
		if (rc) {
			mprError(MPR_L, MPR_LOG, "Exiting acceptProc with failed getnameinfo");
			delete nsp;
			return;
		}
		(nsp->acceptCallback)(nsp->acceptData, nsp, hbuf, atoi(sbuf), this, isMprPoolThread);
	} else 
#endif
	{
		mprInetNtoa(callerIpAddr, sizeof(callerIpAddr), addr.sin_addr);
		(nsp->acceptCallback)(nsp->acceptData, nsp, callerIpAddr, 
				ntohs(addr.sin_port), this, isMprPoolThread);
	}

	nsp->lock();
	if (--nsp->inUse == 0 && nsp->flags & MPR_SOCKET_DISPOSED) {
		mprLog(9, log, "%d: acceptProc: Leaving deleted\n", sock);
		delete nsp;
	} else{
		nsp->unlock();
	}
	unlock();
}

////////////////////////////////////////////////////////////////////////////////
//
//	Write data. Return the number of bytes written or -1 on errors.
//	NOTE: this routine will return with a short write if the underlying socket
//	can't accept any more data.
//

int	MprSocket::write(char *buf, int bufsize)
{
#if BLD_FEATURE_IPV6
	struct addrinfo 	hints, *res;
	struct sockaddr_storage	server6;
	char 				port_string[MPR_MAX_IP_PORT];
	int 				rc;
#endif				
	struct sockaddr_in	server;
	struct sockaddr 	*sa;
	MprSocklen 			addrlen;
	int					sofar, errCode, len, written;

	mprAssert(buf);
	mprAssert(bufsize >= 0);
	mprAssert((flags & MPR_SOCKET_CLOSED) == 0);

	addrlen = 0;
	sa = 0;

	lock();

	if (flags & (MPR_SOCKET_BROADCAST | MPR_SOCKET_DATAGRAM)) {
#if BLD_FEATURE_IPV6
		if (ipv6) {
			memset((char *) &hints, '\0', sizeof(hints));
			memset((char *) &server, '\0', sizeof(struct sockaddr_storage));
			
			mprSprintf(port_string, sizeof(port_string), "%d", port);

			hints.ai_socktype = SOCK_STREAM;
			hints.ai_flags = AI_NUMERICHOST;

			if (strcmp(ipAddr, "") == 0) {
				//	Note that IPv6 does not support broadcast, there is no
				//	255.255.255.255 equiv. Multicast can be used over a specific 
				//	link, but the user must provide that address plus %scope_id.
				unlock();
				return -1;
			}

			rc = getaddrinfo(ipAddr, port_string, &hints, &res);
			if (rc) {
				unlock();
				return -1;

			} else {
				memcpy(&server, res->ai_addr, res->ai_addrlen);
				addrlen = res->ai_addrlen;
				freeaddrinfo(res);
			}
			sa = (struct sockaddr*) &server6;

		} else 
#endif
		{
			memset((char*) &server, '\0', sizeof(struct sockaddr_in));
			addrlen = sizeof(struct sockaddr_in);
			server.sin_family = AF_INET;

			server.sin_port = htons((short) (port & 0xFFFF));
			if (strcmp(ipAddr, "") != 0) {
				server.sin_addr.s_addr = inet_addr(ipAddr);
			} else {
				server.sin_addr.s_addr = INADDR_ANY;
			}
			sa = (struct sockaddr*) &server;
		}
	}
	
		
	if (flags & MPR_SOCKET_EOF) {
		sofar = bufsize;

	} else {
		errCode = 0;
		len = bufsize;
		sofar = 0;
		while (len > 0) {
			if ((flags & MPR_SOCKET_BROADCAST) || 
					(flags & MPR_SOCKET_DATAGRAM)) {
				written = sendto(sock, &buf[sofar], len, MSG_NOSIGNAL, sa, addrlen);
			} else {
				written = send(sock, &buf[sofar], len, MSG_NOSIGNAL);
			}
			if (written < 0) {
				errCode = getError();
				if (errCode == EINTR) {
					mprLog(8, log, "%d: write: EINTR\n", sock);
					continue;
				} else if (errCode == EAGAIN || errCode == EWOULDBLOCK) {
					mprLog(8, log, "%d: write: EAGAIN returning %d\n", sock, sofar);
					unlock();
					return sofar;
				}
				mprLog(8, log, "%d: write: error %d\n", sock, -errCode);
				unlock();
				return -errCode;
			}
			len -= written;
			sofar += written;
		}
	}

	mprLog(8, log, "%d: write: %d bytes, ask %d, flags %x\n", sock, sofar, bufsize, flags);

	unlock();
	return sofar;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Write a string.
//

int MprSocket::write(char *s)
{
	return this->write(s, strlen(s));
}

////////////////////////////////////////////////////////////////////////////////
//
//	Read data. Return zero for EOF or no data if in non-blocking mode. Return
//	-1 for errors. On success, return the number of bytes read. Use getEof()
//	to tell if we are EOF or just no data (in non-blocking mode).
//
 
int	MprSocket::read(char *buf, int bufsize)
{
#if BLD_FEATURE_IPV6
	struct sockaddr_storage	server6;
#endif				
	struct sockaddr_in	server;
	struct sockaddr 	*sa;
	MprSocklen			addrlen;
	int					bytes, errCode;

	mprAssert(buf);
	mprAssert(bufsize > 0);
	mprAssert(~(flags & MPR_SOCKET_CLOSED));

	lock();

	if (flags & MPR_SOCKET_EOF) {
		unlock();
		return 0;
	}

again:
	if (flags & MPR_SOCKET_DATAGRAM) {
#if BLD_FEATURE_IPV6
		if (ipv6) {
			sa = (struct sockaddr*) &server6;
			addrlen = sizeof(server6);
		} else
#endif
		{
			sa = (struct sockaddr*) &server;
			addrlen = sizeof(server);
		}
		bytes = recvfrom(sock, buf, bufsize, MSG_NOSIGNAL, sa, (SocketLenPtr) &addrlen);

	} else {
		bytes = recv(sock, buf, bufsize, MSG_NOSIGNAL);
	}

	if (bytes < 0) {
		errCode = getError();
		if (errCode == EINTR) {
			goto again;

		} else if (errCode == EAGAIN || errCode == EWOULDBLOCK) {
			bytes = 0;							// No data available

		} else if (errCode == ECONNRESET) {
			flags |= MPR_SOCKET_EOF;				// Disorderly disconnect
			bytes = 0;

		} else {
			flags |= MPR_SOCKET_EOF;				// Some other error
			bytes = -errCode;
		}

	} else if (bytes == 0) {					// EOF
		flags |= MPR_SOCKET_EOF;
		mprLog(8, log, "%d: read: %d bytes, EOF\n", sock, bytes);

	} else {
		mprLog(8, log, "%d: read: %d bytes\n", sock, bytes);
	}


	unlock();
	return bytes;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Return true if end of file
//

bool MprSocket::getEof()
{
	bool	rc;

	lock();
	rc = ((flags & MPR_SOCKET_EOF) != 0);
	unlock();
	return rc;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Define an IO callback for this socket. The callback called whenever there
//	is an event of interest as defined by handlerMask (MPR_SOCKET_READABLE, ...)
//

void MprSocket::setCallback(MprSocketIoProc fn, void *data, void *data2, 
	int mask, int pri)
{
	lock();
	ioCallback = fn;
	ioData = data;
	ioData2 = data2;
	handlerPriority = pri;
	handlerMask = mask;
	setMask(handlerMask);
	unlock();
}

////////////////////////////////////////////////////////////////////////////////

void MprSocket::getCallback(MprSocketIoProc *fn, void **data, void **data2,
	int *mask)
{
	if (fn) {
		*fn = ioCallback;
	}
	if (data) {
		*data = ioData;
	}
	if (data2) {
		*data2 = ioData2;
	}
	if (mask) {
		*mask = handlerMask;
	}
}

////////////////////////////////////////////////////////////////////////////////

void MprSocket::getAcceptCallback(MprSocketAcceptProc *fn, void **data)
{
	if (fn) {
		*fn = acceptCallback;
	}
	if (data) {
		*data = acceptData;
	}
}

////////////////////////////////////////////////////////////////////////////////
//
//	Return the O/S socket file handle
//

int MprSocket::getFd()
{
	return sock;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Return the blocking mode of the socket
//

bool MprSocket::getBlockingMode()
{
	bool	rc;

	lock();
	rc = flags & MPR_SOCKET_BLOCK;
	unlock();
	return rc;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Get the socket flags
//

int MprSocket::getFlags()
{
	int		rc;

	//
	//	These routines must still lock as the code will sometimes modify
	//	flags such that it can have invalid settings for a small window,
	//	see setBlockingMode() for an example.
	//
	lock();
	rc = flags;
	unlock();
	return rc;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Set whether the socket blocks or not on read/write
//

void MprSocket::setBlockingMode(bool on)
{
	int		flag;

	lock();
	mprLog(8, log, "%d: setBlockingMode: %d\n", sock, on);

	flags &= ~(MPR_SOCKET_BLOCK);
	if (on) {
		flags |= MPR_SOCKET_BLOCK;
	}

	flag = (flags & MPR_SOCKET_BLOCK) ? 0 : 1;
#if WIN
	ioctlsocket(sock, FIONBIO, (ulong*) &flag);
#elif VXWORKS
	ioctl(sock, FIONBIO, (int) &flag);
#else
	if (on) {
		fcntl(sock, F_SETFL, fcntl(sock, F_GETFL) & ~O_NONBLOCK);
	} else {
		fcntl(sock, F_SETFL, fcntl(sock, F_GETFL) | O_NONBLOCK);
	}
#endif
	unlock();
}

////////////////////////////////////////////////////////////////////////////////
//
//	Disable normal TCP delay behavior (nagle algorithm)
//

void MprSocket::setNoDelay(bool on)
{
	lock();
	if (on) {
		flags |= MPR_SOCKET_NODELAY;
	} else {
		flags &= ~(MPR_SOCKET_NODELAY);
	}
	{
#if WIN
		BOOL	noDelay;
		noDelay = on ? 1 : 0;
		setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (FAR char *) &noDelay, 
			sizeof(BOOL));
#else
		int		noDelay;
		noDelay = on ? 1 : 0;
		setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (char *) &noDelay, 
			sizeof(int));
#endif // WIN 
	}
	unlock();
}

////////////////////////////////////////////////////////////////////////////////
#if WIN
#define OPT_CAST const char*
#elif VXWORKS
#define OPT_CAST char*
#else
#define OPT_CAST void*
#endif

int MprSocket::setBufSize(int sendSize, int recvSize)
{
	if (sock < 0) {
		return MPR_ERR_BAD_STATE;
	}
	if (sendSize > 0) {
		if (setsockopt(sock, SOL_SOCKET, SO_SNDBUF, (OPT_CAST) &sendSize, 
				sizeof(int)) == -1) {
			return MPR_ERR_CANT_INITIALIZE;
		}
	}
	if (recvSize > 0) {
		if (setsockopt(sock, SOL_SOCKET, SO_SNDBUF, (OPT_CAST) &recvSize, 
				sizeof(int)) == -1) {
			return MPR_ERR_CANT_INITIALIZE;
		}
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Get the port number
//
 
int MprSocket::getPort()
{
	return port;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Select handler. May be called directly if single-threaded or on a pool
//	thread. User may call dispose() on the socket in the callback. This will
//	just mark it for deletion.
//

static void ioProcWrapper(void *data, int mask, int isMprPoolThread)
{
	MprSocket			*sp;

	sp = (MprSocket*) data;
	sp->ioProc(mask, isMprPoolThread);
}

////////////////////////////////////////////////////////////////////////////////

void MprSocket::ioProc(int mask, int isMprPoolThread)
{
	mprLog(7, log, "%d: ioProc: %x, mask %x\n", sock, ioData, mask);

	lock();
	if (ioCallback == 0 || (handlerMask & mask) == 0) {
		unlock();
		mprLog(7, log, "%d: ioProc: returning, ioCallback %x, mask %x\n", 
			sock, ioCallback, mask);
		return;
	}
	mask &= handlerMask;
	mprLog(8, log, "%d: ioProc: %x, mask %x\n", sock, ioData, mask);
	inUse++;
	unlock();

	(ioCallback)(ioData, this, mask, isMprPoolThread);

	lock();
	if (--inUse == 0 && flags & MPR_SOCKET_DISPOSED) {
		mprLog(8, log, "%d: ioProc: Leaving deleted, inUse %d, flags %x\n", 
			sock, inUse, flags);
		delete this;
	} else {
		unlock();
	}
}

////////////////////////////////////////////////////////////////////////////////
//
//	Define the events of interest. Must only be called with a locked socket.
//

void MprSocket::setMask(int handlerMask)
{
	lock();
	if (handlerMask) {
		if (handler) {
			handler->setInterest(handlerMask);
		} else {
			handler = new MprSelectHandler(sock, handlerMask,
				(MprSelectProc) ioProcWrapper, (void*) this, handlerPriority);
		}
	} else if (handler) {
		handler->setInterest(handlerMask);
	}
	unlock();
}

////////////////////////////////////////////////////////////////////////////////
//
//	Map the O/S error code to portable error codes.
//

int MprSocket::getError()
{
#if WIN
	int		rc;
	switch (rc = WSAGetLastError()) {
	case WSAEINTR:
		return EINTR;

	case WSAENETDOWN:
		return ENETDOWN;

	case WSAEWOULDBLOCK:
		return EWOULDBLOCK;

	case WSAEPROCLIM:
		return EAGAIN;

	case WSAECONNRESET:
	case WSAECONNABORTED:
		return ECONNRESET;

	case WSAECONNREFUSED:
		return ECONNREFUSED;

	case WSAEADDRINUSE:
		return EADDRINUSE;
	default:
		return EINVAL;
	}
#else
	return errno;
#endif
}

////////////////////////////////////////////////////////////////////////////////
///////////////////////////////// MprInterface /////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

MprInterface::MprInterface(char *nam, char *ip, char *bcast, char *msk)
{
	mprAssert(ip);

	name = mprStrdup(nam);
	ipAddr = mprStrdup(ip);
	broadcast = mprStrdup(bcast);
	mask = mprStrdup(msk);
}

////////////////////////////////////////////////////////////////////////////////

MprInterface::~MprInterface()
{
	mprFree(name);
	mprFree(ipAddr);
	mprFree(broadcast);
	mprFree(mask);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
extern "C" {
//
//	Replacement for gethostbyname that is multi-thread safe
//
//	FUTURE -- convention. Should take hostent as a parameter
//

//	TODO - should support ipv6

struct hostent *mprGetHostByName(char *name)
{
	Mpr				*mpr;
	struct hostent	*hp;

	mpr = mprGetMpr();
	hp = new hostent;
	memset(hp, 0, sizeof(struct hostent));

	mpr->lock();

#if VXWORKS
	struct in_addr inaddr;
	inaddr.s_addr = (ulong) hostGetByName(name);
	if (inaddr.s_addr < 0) {
		mpr->unlock();
		mprAssert(0);
		return 0;
	}
	hp->h_addrtype = AF_INET;
	hp->h_length = sizeof(int);
	hp->h_name = mprStrdup(name);
	hp->h_addr_list = 0;
	hp->h_aliases = 0;

	hp->h_addr_list = new char*[2];
	hp->h_addr_list[0] = (char *) mprMalloc(sizeof(struct in_addr));
	memcpy(&hp->h_addr_list[0], &inaddr, hp->h_length);
	hp->h_addr_list[1] = 0;

#else
	struct hostent	*ip;
	int				count, i;

	#undef gethostbyname
	ip = gethostbyname(name);
	if (ip == 0) {
		mpr->unlock();
		return 0;
	}
	hp->h_addrtype = ip->h_addrtype;
	hp->h_length = ip->h_length;
	hp->h_name = mprStrdup(ip->h_name);
	hp->h_addr_list = 0;
	hp->h_aliases = 0;

	for (count = 0; ip->h_addr_list[count] != 0; ) {
		count++;
	}
	if (count > 0) {
		count++;
		hp->h_addr_list = new char*[count];
		for (i = 0; ip->h_addr_list[i] != 0; i++) {
			memcpy(&hp->h_addr_list[i], &ip->h_addr_list[i], ip->h_length);
		}
		hp->h_addr_list[i] = 0;
	}

	for (count = 0; ip->h_aliases[count] != 0; ) {
		count++;
	}
	if (count > 0) {
		count++;
		hp->h_aliases = new char*[count];
		for (i = 0; ip->h_aliases[i] != 0; i++) {
			hp->h_aliases[i] = mprStrdup(ip->h_aliases[i]);
		}
		hp->h_aliases[i] = 0;
	}
#endif
	mpr->unlock();
	return hp;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Free the allocated host entry structure 
//

void mprFreeGetHostByName(struct hostent *hostp)
{
	int	i;

	mprAssert(hostp);

	mprFree((void*) hostp->h_name);

	if (hostp->h_addr_list) {
		delete[] hostp->h_addr_list;
	}

	if (hostp->h_aliases) {
		for (i = 0; hostp->h_aliases[i] != 0; i++) {
			mprFree(hostp->h_aliases[i]);
		}
		delete[] hostp->h_aliases;
	}
	delete hostp;
}


/*
 *	Parse ipSpec to handle ipv4 and ipv6 addresses.
 *	When an ipSpec contains an ipv6  port it should be written as
 *
 *	[aaaa:bbbb:cccc:dddd:eeee:ffff:gggg:hhhh:iiii]:port
 * 
 *	Returns IP portion which must freed by caller. If port is present,
 *	it will be returned. If no port is present in the ipSpec, the default_port
 *	will be returned - this allows the caller to set a 'default' to be used
 *	such as 0 or -1.
 */

char *mprGetIpAddrPort(char *ipSpec, int *port, int default_port)
{
	char	*ipAddr;
	char	*cp;
	int 	num_colons;
		
	ipAddr = NULL;
	num_colons = 0;

//	TODO - determine ipv6 if using []
	/*
 	 * First check if ipv6 or ipv4 address by looking for > 1 colons.
 	 */
	cp = ipSpec;
	for (cp = ipSpec; ((*cp != '\0') && (num_colons < 2)) ; cp++) {
		if (*cp == ':') {
			num_colons++;
		}
	}

	// ipv6 family address
	if (num_colons > 1) {
		// If port is present, it will follow a closing bracket ']'
		// Handles [a:b:c:d:e:f:g:h:i]:port case 
		if ((cp = strchr(ipSpec, ']')) != 0) {
			cp++;
			if ((*cp) && (*cp == ':')) {
				cp++;
				if (*cp == '*') {
					*port = -1;
				} else {
					*port = atoi(cp);
				}

				// set ipAddr to ipv6 address without brackets
				ipAddr = mprStrdup(ipSpec+1);
				cp = strchr(ipAddr, ']');
				*cp = '\0';

			} else {
				// Handles [a:b:c:d:e:f:g:h:i] case (no port)- should not occur
				ipAddr = mprStrdup(ipSpec+1);
				cp = strchr(ipAddr, ']');
				*cp = '\0';
								
				// No port present, use callers default
				*port = default_port;
			}
		} else {
			// Handles a:b:c:d:e:f:g:h:i case (no port)
			ipAddr = mprStrdup(ipSpec);

			// No port present, use callers default
			*port = default_port;
		}

				
	} else {
		// ipv4 family address
		ipAddr = mprStrdup(ipSpec);
				
		if ((cp = strchr(ipAddr, ':')) != 0) {
			*cp++ = '\0';
			if (*cp == '*') {
				*port = -1;
			} else {
				*port = atoi(cp);
			}
		} else {
			// No port present, use callers default
			*port = default_port;
		}
	}

	return ipAddr;
}

} // extern "C"
////////////////////////////////////////////////////////////////////////////////

//
// Local variables:
// tab-width: 4
// c-basic-offset: 4
// End:
// vim: sw=4 ts=4 
//
