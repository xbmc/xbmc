
/*
 *	sockGen.c -- Posix Socket support module for general posix use
 *
 *	Copyright (c) GoAhead Software Inc., 1995-2000. All Rights Reserved.
 *
 * $Id: sockGen.c,v 1.6 2003/04/11 18:00:12 bporter Exp $
 */

/******************************** Description *********************************/

/*
 *	Posix Socket Module.  This supports blocking and non-blocking buffered 
 *	socket I/O.
 */

#if (!defined (WIN) || defined (LITTLEFOOT) || defined (WEBS))

/********************************** Includes **********************************/
#ifndef CE
#include	<errno.h>
#include	<fcntl.h>
#include	<string.h>
#include	<stdlib.h>
#endif

#ifdef UEMF
	#include	"uemf.h"
#else
	#include	<socket.h>
	#include	<types.h>
	#include	<unistd.h>
	#include	"emfInternal.h"
#endif

#ifdef VXWORKS
	#include	<hostLib.h>
#endif

#include "socketutil.h"

/************************************ Locals **********************************/

extern socket_t		**socketList;			/* List of open sockets */
extern int			socketMax;				/* Maximum size of socket */
extern int			socketHighestFd;		/* Highest socket fd opened */
static int			socketOpenCount = 0;	/* Number of task using sockets */

/***************************** Forward Declarations ***************************/

static void socketAccept(socket_t *sp);
static int 	socketDoEvent(socket_t *sp);
static int	tryAlternateConnect(int sock, struct sockaddr *sockaddr);

/*********************************** Code *************************************/
/*
 *	Open socket module
 */

int socketOpen()
{
#if (defined (CE) || defined (WIN))
    WSADATA 	wsaData;
#endif

	if (++socketOpenCount > 1) {
		return 0;
	}

#if (defined (CE) || defined (WIN))
	if (WSAStartup(MAKEWORD(1,1), &wsaData) != 0) {
		return -1;
	}
	if (wsaData.wVersion != MAKEWORD(1,1)) {
		WSACleanup();
		return -1;
	}
#endif

	socketList = NULL;
	socketMax = 0;
	socketHighestFd = -1;

	return 0;
}

/******************************************************************************/
/*
 *	Close the socket module, by closing all open connections
 */

void socketClose()
{
	int		i;

	if (--socketOpenCount <= 0) {
		for (i = socketMax; i >= 0; i--) {
			if (socketList && socketList[i]) {
				socketCloseConnection(i);
			}
		}
		socketOpenCount = 0;
	}
}

/******************************************************************************/
/*
 *	Open a client or server socket. Host is NULL if we want server capability.
 */

int socketOpenConnection(char *host, int port, socketAccept_t accept, int flags)
{
#if (!defined (NO_gethostbyname) && !defined (VXWORKS))
	struct hostent		*hostent;					/* Host database entry */
#endif /* ! (NO_gethostbyname || VXWORKS) */
	socket_t			*sp;
	struct sockaddr_in	sockaddr;
	int					sid, bcast, dgram, rc;

	if (port > SOCKET_PORT_MAX) {
		return -1;
	}
/*
 *	Allocate a socket structure
 */
	if ((sid = socketAlloc(host, port, accept, flags)) < 0) {
		return -1;
	}
	sp = socketList[sid];
	a_assert(sp);

/*
 *	Create the socket address structure
 */
	memset((char *) &sockaddr, '\0', sizeof(struct sockaddr_in));
	sockaddr.sin_family = AF_INET;
	sockaddr.sin_port = htons((short) (port & 0xFFFF));

	if (host == NULL) {
		sockaddr.sin_addr.s_addr = INADDR_ANY;
	} else {
		sockaddr.sin_addr.s_addr = inet_addr(host);
		if (sockaddr.sin_addr.s_addr == INADDR_NONE) {
/*
 *			If the OS does not support gethostbyname functionality, the macro:
 *			NO_gethostbyname should be defined to skip the use of gethostbyname.
 *			Unfortunatly there is no easy way to recover, the following code
 *			simply uses the basicGetHost IP for the sockaddr.
 */

#ifdef NO_gethostbyname
			if (strcmp(host, basicGetHost()) == 0) {
				sockaddr.sin_addr.s_addr = inet_addr(basicGetAddress());
			}
			if (sockaddr.sin_addr.s_addr == INADDR_NONE) {
				socketFree(sid);
				return -1;
			}
#elif (defined (VXWORKS))
			sockaddr.sin_addr.s_addr = (unsigned long) hostGetByName(host);
			if (sockaddr.sin_addr.s_addr == NULL) {
				errno = ENXIO;
				socketFree(sid);
				return -1;
			}
#else
			hostent = webs_gethostbyname(host);
			if (hostent != NULL) {
				memcpy((char *) &sockaddr.sin_addr, 
					(char *) hostent->h_addr_list[0],
					(size_t) hostent->h_length);
			} else {
				char	*asciiAddress;
				char_t	*address;

				address = basicGetAddress();
				asciiAddress = ballocUniToAsc(address, gstrlen(address));
				sockaddr.sin_addr.s_addr = inet_addr(asciiAddress);
				bfree(B_L, asciiAddress);
				if (sockaddr.sin_addr.s_addr == INADDR_NONE) {
					errno = ENXIO;
					socketFree(sid);
					return -1;
				}
			}
#endif /* (NO_gethostbyname || VXWORKS) */
		}
	}

	bcast = sp->flags & SOCKET_BROADCAST;
	if (bcast) {
		sp->flags |= SOCKET_DATAGRAM;
	}
	dgram = sp->flags & SOCKET_DATAGRAM;

/*
 *	Create the socket. Support for datagram sockets. Set the close on
 *	exec flag so children don't inherit the socket.
 */
	sp->sock = socket(AF_INET, dgram ? SOCK_DGRAM: SOCK_STREAM, 0);
	if (sp->sock == SOCKET_ERROR) {
		socketFree(sid);
		return -1;
	}
#ifndef __NO_FCNTL
	fcntl(sp->sock, F_SETFD, FD_CLOEXEC);
#endif
	socketHighestFd = max(socketHighestFd, sp->sock);

/*
 *	If broadcast, we need to turn on broadcast capability.
 */
	if (bcast) {
		int broadcastFlag = 1;
		if (setsockopt(sp->sock, SOL_SOCKET, SO_BROADCAST,
				(char *) &broadcastFlag, sizeof(broadcastFlag)) == SOCKET_ERROR) {
			socketFree(sid);
			return -1;
		}
	}

/*
 *	Host is set if we are the client
 */
	if (host) {
/*
 *		Connect to the remote server in blocking mode, then go into 
 *		non-blocking mode if desired.
 */
		if (!dgram) {
			if (! (sp->flags & SOCKET_BLOCK)) {
/*
 *				sockGen.c is only used for Windows products when blocking
 *				connects are expected.  This applies to FieldUpgrader
 *				agents and open source webserver connectws.  Therefore the
 *				asynchronous connect code here is not compiled.
 */
#if (defined (WIN) || defined (CE)) && (!defined (LITTLEFOOT) && !defined (WEBS))
				int flag;

				sp->flags |= SOCKET_ASYNC;
/*
 *				Set to non-blocking for an async connect
 */
				flag = 1;
				if (ioctlsocket(sp->sock, FIONBIO, &flag) == SOCKET_ERROR) {
					socketFree(sid);
					return -1;
				}
#else
				socketSetBlock(sid, 1);
#endif /* #if (WIN || CE) && !(LITTLEFOOT || WEBS) */

			}
			if ((rc = connect(sp->sock, (struct sockaddr *) &sockaddr,
				sizeof(sockaddr))) == SOCKET_ERROR && 
				(rc = tryAlternateConnect(sp->sock,
				(struct sockaddr *) &sockaddr)) < 0) {
#if (defined (WIN) || defined (CE))
				if (socketGetError() != EWOULDBLOCK) {
					socketFree(sid);
					return -1;
				}
#else
				socketFree(sid);
				return -1;

#endif /* WIN || CE */

			}
		}
	} else {
/*
 *		Bind to the socket endpoint and the call listen() to start listening
 */
		rc = 1;
		setsockopt(sp->sock, SOL_SOCKET, SO_REUSEADDR, (char *)&rc, sizeof(rc));
		if (bind(sp->sock, (struct sockaddr *) &sockaddr, 
				sizeof(sockaddr)) == SOCKET_ERROR) {
			socketFree(sid);
			return -1;
		}

		if (! dgram) {
			if (listen(sp->sock, SOMAXCONN) == SOCKET_ERROR) {
				socketFree(sid);
				return -1;
			}
#ifndef UEMF
			sp->fileHandle = emfCreateFileHandler(sp->sock, SOCKET_READABLE,
				(emfFileProc *) socketAccept, (void *) sp);
#else
			sp->flags |= SOCKET_LISTENING;
#endif
		}
		sp->handlerMask |= SOCKET_READABLE;
	}

/*
 *	Set the blocking mode
 */

	if (flags & SOCKET_BLOCK) {
		socketSetBlock(sid, 1);
	} else {
		socketSetBlock(sid, 0);
	}
	return sid;
}


/******************************************************************************/
/*
 *	If the connection failed, swap the first two bytes in the 
 *	sockaddr structure.  This is a kludge due to a change in
 *	VxWorks between versions 5.3 and 5.4, but we want the 
 *	product to run on either.
 */

static int tryAlternateConnect(int sock, struct sockaddr *sockaddr)
{
#ifdef VXWORKS
	char *ptr;

	ptr = (char *)sockaddr;
	*ptr = *(ptr+1);
	*(ptr+1) = 0;
	return connect(sock, sockaddr, sizeof(struct sockaddr));
#else
	return -1;
#endif /* VXWORKS */
}

/******************************************************************************/
/*
 *	Close a socket
 */

void socketCloseConnection(int sid)
{
	socket_t	*sp;

	if ((sp = socketPtr(sid)) == NULL) {
		return;
	}
	socketFree(sid);
}

/******************************************************************************/
/*
 *	Accept a connection. Called as a callback on incoming connection.
 */

static void socketAccept(socket_t *sp)
{
	struct sockaddr_in	addr;
	socket_t 			*nsp;
	size_t				len;
	char				*pString;
	int 				newSock, nid;


#ifdef NW
	NETINET_DEFINE_CONTEXT;
#endif

	a_assert(sp);

/*
 *	Accept the connection and prevent inheriting by children (F_SETFD)
 */
	len = sizeof(struct sockaddr_in);
	if ((newSock = accept(sp->sock, (struct sockaddr *) &addr, (unsigned int *) &len)) == -1) {
		return;
	}
#ifndef __NO_FCNTL
	fcntl(newSock, F_SETFD, FD_CLOEXEC);
#endif
	socketHighestFd = max(socketHighestFd, newSock);

/*
 *	Create a socket structure and insert into the socket list
 */
	nid = socketAlloc(sp->host, sp->port, sp->accept, sp->flags);
	nsp = socketList[nid];
	a_assert(nsp);
	nsp->sock = newSock;
	nsp->flags &= ~SOCKET_LISTENING;

	if (nsp == NULL) {
		return;
	}
/*
 *	Set the blocking mode before calling the accept callback.
 */

	socketSetBlock(nid, (nsp->flags & SOCKET_BLOCK) ? 1: 0);
/*
 *	Call the user accept callback. The user must call socketCreateHandler
 *	to register for further events of interest.
 */
	if (sp->accept != NULL) {
		pString = webs_inet_ntoa(addr.sin_addr);
		if ((sp->accept)(nid, pString, ntohs(addr.sin_port), sp->sid) < 0) {
			socketFree(nid);
		}
#ifdef VXWORKS
		free(pString);
#endif
	}
}

/******************************************************************************/
/*
 *	Get more input from the socket and return in buf.
 *	Returns 0 for EOF, -1 for errors and otherwise the number of bytes read.
 */

int socketGetInput(int sid, char *buf, int toRead, int *errCode)
{
	struct sockaddr_in 	server;
	socket_t			*sp;
	int 				len, bytesRead;

	a_assert(buf);
	a_assert(errCode);

	*errCode = 0;

	if ((sp = socketPtr(sid)) == NULL) {
		return -1;
	}

/*
 *	If we have previously seen an EOF condition, then just return
 */
	if (sp->flags & SOCKET_EOF) {
		return 0;
	}
#if ((defined (WIN) || defined (CE)) && (!defined (LITTLEFOOT) && !defined  (WEBS)))
	if ( !(sp->flags & SOCKET_BLOCK)
			&& ! socketWaitForEvent(sp,  FD_CONNECT, errCode)) {
		return -1;
	}
#endif

/*
 *	Read the data
 */
	if (sp->flags & SOCKET_DATAGRAM) {
		len = sizeof(server);
		bytesRead = recvfrom(sp->sock, buf, toRead, 0,
			(struct sockaddr *) &server, (unsigned int*) &len);
	} else {
		bytesRead = recv(sp->sock, buf, toRead, 0);
	}
   
   /*
    * BUG 01865 -- CPU utilization hangs on Windows. The original code used 
    * the 'errno' global variable, which is not set by the winsock functions
    * as it is under *nix platforms. We use the platform independent
    * socketGetError() function instead, which does handle Windows correctly. 
    * Other, *nix compatible platforms should work as well, since on those
    * platforms, socketGetError() just returns the value of errno.
    * Thanks to Jonathan Burgoyne for the fix.
    */
   if (bytesRead < 0) 
   {
      *errCode = socketGetError();
      if (*errCode == ECONNRESET) 
      {
         sp->flags |= SOCKET_CONNRESET;
         return 0;
      }
      return -1;
   }
	return bytesRead;
}

/******************************************************************************/
/*
 *	Process an event on the event queue
 */

#ifndef UEMF

static int socketEventProc(void *data, int mask)
{
	socket_t		*sp;
	ringq_t			*rq;
	int 			sid;

	sid = (int) data;

	a_assert(sid >= 0 && sid < socketMax);
	a_assert(socketList[sid]);

	if ((sp = socketPtr(sid)) == NULL) {
		return 1;
	}

/*
 *	If now writable and flushing in the background, continue flushing
 */
	if (mask & SOCKET_WRITABLE) {
		if (sp->flags & SOCKET_FLUSHING) {
			rq = &sp->outBuf;
			if (ringqLen(rq) > 0) {
				socketFlush(sp->sid);
			} else {
				sp->flags &= ~SOCKET_FLUSHING;
			}
		}
	}

/*
 *	Now invoke the users socket handler. NOTE: the handler may delete the
 *	socket, so we must be very careful after calling the handler.
 */
	if (sp->handler && (sp->handlerMask & mask)) {
		(sp->handler)(sid, mask & sp->handlerMask, sp->handler_data);
	}
	if (socketList && sid < socketMax && socketList[sid] == sp) {
		socketRegisterInterest(sp, sp->handlerMask);
	}
	return 1;
}
#endif /* ! UEMF */

/******************************************************************************/
/*
 *	Define the events of interest
 */

void socketRegisterInterest(socket_t *sp, int handlerMask)
{
	a_assert(sp);

	sp->handlerMask = handlerMask;
#ifndef UEMF
	if (handlerMask) {
		sp->fileHandle = emfCreateFileHandler(sp->sock, handlerMask,
			(emfFileProc *) socketEventProc, (void *) sp->sid);
	} else {
		emfDeleteFileHandler(sp->fileHandle);
		sp->fileHandle = -1;
	}
#endif /* ! UEMF */
}

/******************************************************************************/
/*
 *	Wait until an event occurs on a socket. Return 1 on success, 0 on failure.
 *	or -1 on exception (UEMF only)
 */

int socketWaitForEvent(socket_t *sp, int handlerMask, int *errCode)
{
	int	mask;

	a_assert(sp);

	mask = sp->handlerMask;
	sp->handlerMask |= handlerMask;
	while (socketSelect(sp->sid, 1000)) {
		if (sp->currentEvents & (handlerMask | SOCKET_EXCEPTION)) {
			break;
		}
	}
	sp->handlerMask = mask;
	if (sp->currentEvents & SOCKET_EXCEPTION) {
		return -1;
	} else if (sp->currentEvents & handlerMask) {
		return 1;
	}
	if (errCode) {
		*errCode = errno = EWOULDBLOCK;
	}
	return 0;
}

/******************************************************************************/
/*
 *	Return TRUE if there is a socket with an event ready to process,
 */

int socketReady(int sid)
{
	socket_t 	*sp;
	int			all;

	all = 0;
	if (sid < 0) {
		sid = 0;
		all = 1;
	}

	for (; sid < socketMax; sid++) {
		if ((sp = socketList[sid]) == NULL) {
			if (! all) {
				break;
			} else {
				continue;
			}
		} 
		if (sp->flags & SOCKET_CONNRESET) {
			socketCloseConnection(sid);
			return 0;
		}
		if (sp->currentEvents & sp->handlerMask) {
			return 1;
		}
/*
 *		If there is input data, also call select to test for new events
 */
		if (sp->handlerMask & SOCKET_READABLE && socketInputBuffered(sid) > 0) {
			socketSelect(sid, 0);
			return 1;
		}
		if (! all) {
			break;
		}
	}
	return 0;
}

/******************************************************************************/
/*
 * 	Wait for a handle to become readable or writable and return a number of 
 *	noticed events. Timeout is in milliseconds.
 */

#if (defined (WIN) || defined (CE) || defined (NW))

int socketSelect(int sid, int timeout)
{
	struct timeval	tv;
	socket_t		*sp;
	fd_set		 	readFds, writeFds, exceptFds;
	int 			nEvents;
	int				all, socketHighestFd;	/* Highest socket fd opened */

	FD_ZERO(&readFds);
	FD_ZERO(&writeFds);
	FD_ZERO(&exceptFds);
	socketHighestFd = -1;

	tv.tv_sec = timeout / 1000;
	tv.tv_usec = (timeout % 1000) * 1000;

/*
 *	Set the select event masks for events to watch
 */
	all = nEvents = 0;

	if (sid < 0) {
		all++;
		sid = 0;
	}

	for (; sid < socketMax; sid++) {
		if ((sp = socketList[sid]) == NULL) {
			continue;
		}
		a_assert(sp);
/*
 * 		Set the appropriate bit in the ready masks for the sp->sock.
 */
		if (sp->handlerMask & SOCKET_READABLE) {
			FD_SET(sp->sock, &readFds);
			nEvents++;
			if (socketInputBuffered(sid) > 0) {
				tv.tv_sec = 0;
				tv.tv_usec = 0;
			}
		}
		if (sp->handlerMask & SOCKET_WRITABLE) {
			FD_SET(sp->sock, &writeFds);
			nEvents++;
		}
		if (sp->handlerMask & SOCKET_EXCEPTION) {
			FD_SET(sp->sock, &exceptFds);
			nEvents++;
		}
		if (! all) {
			break;
		}
	}

/*
 *	Windows select() fails if no descriptors are set, instead of just sleeping
 *	like other, nice select() calls. So, if WIN, sleep.
 */
	if (nEvents == 0) {
		Sleep(timeout);
		return 0;
	}

/*
 * 	Wait for the event or a timeout.
 */
	nEvents = select(socketHighestFd+1, &readFds, &writeFds, &exceptFds, &tv);

	if (all) {
		sid = 0;
	}
	for (; sid < socketMax; sid++) {
		if ((sp = socketList[sid]) == NULL) {
			continue;
		}

		if (FD_ISSET(sp->sock, &readFds) || socketInputBuffered(sid) > 0) {
				sp->currentEvents |= SOCKET_READABLE;
		}
		if (FD_ISSET(sp->sock, &writeFds)) {
				sp->currentEvents |= SOCKET_WRITABLE;
		}
		if (FD_ISSET(sp->sock, &exceptFds)) {
				sp->currentEvents |= SOCKET_EXCEPTION;
		}
		if (! all) {
			break;
		}
	}

	return nEvents;
}

#else /* not WIN || CE || NW */

int socketSelect(int sid, int timeout)
{
	socket_t		*sp;
	struct timeval	tv;
	fd_mask 		*readFds, *writeFds, *exceptFds;
	int 			all, len, nwords, index, bit, nEvents;

/*
 *	Allocate and zero the select masks
 */
	nwords = (socketHighestFd + NFDBITS) / NFDBITS;
	len = nwords * sizeof(int);

	readFds = balloc(B_L, len);
	memset(readFds, 0, len);
	writeFds = balloc(B_L, len);
	memset(writeFds, 0, len);
	exceptFds = balloc(B_L, len);
	memset(exceptFds, 0, len);

	tv.tv_sec = timeout / 1000;
	tv.tv_usec = (timeout % 1000) * 1000;

/*
 *	Set the select event masks for events to watch
 */
	all = nEvents = 0;

	if (sid < 0) {
		all++;
		sid = 0;
	}

	for (; sid < socketMax; sid++) {
		if ((sp = socketList[sid]) == NULL) {
			if (all == 0) {
				break;
			} else {
				continue;
			}
		}
		a_assert(sp);

/*
 * 		Initialize the ready masks and compute the mask offsets.
 */
		index = sp->sock / (NBBY * sizeof(fd_mask));
		bit = 1 << (sp->sock % (NBBY * sizeof(fd_mask)));
		
/*
 * 		Set the appropriate bit in the ready masks for the sp->sock.
 */
		if (sp->handlerMask & SOCKET_READABLE) {
			readFds[index] |= bit;
			nEvents++;
			if (socketInputBuffered(sid) > 0) {
				tv.tv_sec = 0;
				tv.tv_usec = 0;
			}
		}
		if (sp->handlerMask & SOCKET_WRITABLE) {
			writeFds[index] |= bit;
			nEvents++;
		}
		if (sp->handlerMask & SOCKET_EXCEPTION) {
			exceptFds[index] |= bit;
			nEvents++;
		}
		if (! all) {
			break;
		}
	}

/*
 * 	Wait for the event or a timeout. Reset nEvents to be the number of actual
 *	events now.
 */
	nEvents = select(socketHighestFd + 1, (fd_set *) readFds,
		(fd_set *) writeFds, (fd_set *) exceptFds, &tv);

	if (nEvents > 0) {
		if (all) {
			sid = 0;
		}
		for (; sid < socketMax; sid++) {
			if ((sp = socketList[sid]) == NULL) {
				if (all == 0) {
					break;
				} else {
					continue;
				}
			}

			index = sp->sock / (NBBY * sizeof(fd_mask));
			bit = 1 << (sp->sock % (NBBY * sizeof(fd_mask)));

			if (readFds[index] & bit || socketInputBuffered(sid) > 0) {
				sp->currentEvents |= SOCKET_READABLE;
			}
			if (writeFds[index] & bit) {
				sp->currentEvents |= SOCKET_WRITABLE;
			}
			if (exceptFds[index] & bit) {
				sp->currentEvents |= SOCKET_EXCEPTION;
			}
			if (! all) {
				break;
			}
		}
	}

	bfree(B_L, readFds);
	bfree(B_L, writeFds);
	bfree(B_L, exceptFds);

	return nEvents;
}
#endif /* WIN || CE */

/******************************************************************************/
/*
 *	Process socket events
 */

void socketProcess(int sid)
{
	socket_t	*sp;
	int			all;

	all = 0;
	if (sid < 0) {
		all = 1;
		sid = 0;
	}
/*
 * 	Process each socket
 */
	for (; sid < socketMax; sid++) {
		if ((sp = socketList[sid]) == NULL) {
			if (! all) {
				break;
			} else {
				continue;
			}
		}
		if (socketReady(sid)) {
			socketDoEvent(sp);
		}
		if (! all) {
			break;
		}
	}
}

/******************************************************************************/
/*
 *	Process an event on the event queue
 */

static int socketDoEvent(socket_t *sp)
{
	ringq_t		*rq;
	int 		sid;

	a_assert(sp);

	sid = sp->sid;
	if (sp->currentEvents & SOCKET_READABLE) {
		if (sp->flags & SOCKET_LISTENING) { 
			socketAccept(sp);
			sp->currentEvents = 0;
			return 1;
		} 

	} else {
/*
 *		If there is still read data in the buffers, trigger the read handler
 *		NOTE: this may busy spin if the read handler doesn't read the data
 */
		if (sp->handlerMask & SOCKET_READABLE && socketInputBuffered(sid) > 0) {
			sp->currentEvents |= SOCKET_READABLE;
		}
	}


/*
 *	If now writable and flushing in the background, continue flushing
 */
	if (sp->currentEvents & SOCKET_WRITABLE) {
		if (sp->flags & SOCKET_FLUSHING) {
			rq = &sp->outBuf;
			if (ringqLen(rq) > 0) {
				socketFlush(sp->sid);
			} else {
				sp->flags &= ~SOCKET_FLUSHING;
			}
		}
	}

/*
 *	Now invoke the users socket handler. NOTE: the handler may delete the
 *	socket, so we must be very careful after calling the handler.
 */
	if (sp->handler && (sp->handlerMask & sp->currentEvents)) {
		(sp->handler)(sid, sp->handlerMask & sp->currentEvents, 
			sp->handler_data);
/*
 *		Make sure socket pointer is still valid, then reset the currentEvents.
 */ 
		if (socketList && sid < socketMax && socketList[sid] == sp) {
			sp->currentEvents = 0;
		}
	}
	return 1;
}

/******************************************************************************/
/*
 *	Set the socket blocking mode
 */

int socketSetBlock(int sid, int on)
{
	socket_t		*sp;
	unsigned long	flag;
	int				iflag;
	int				oldBlock;

	flag = iflag = !on;

	if ((sp = socketPtr(sid)) == NULL) {
		a_assert(0);
		return 0;
	}
	oldBlock = (sp->flags & SOCKET_BLOCK);
	sp->flags &= ~(SOCKET_BLOCK);
	if (on) {
		sp->flags |= SOCKET_BLOCK;
	}

/*
 *	Put the socket into block / non-blocking mode
 */
	if (sp->flags & SOCKET_BLOCK) {
#if (defined (CE) || defined (WIN))
		ioctlsocket(sp->sock, FIONBIO, &flag);
#elif (defined (ECOS))
		int off;
		off = 0;
		ioctl(sp->sock, FIONBIO, &off);
#elif (defined (VXWORKS) || defined (NW))
		ioctl(sp->sock, FIONBIO, (int)&iflag);
#else
		fcntl(sp->sock, F_SETFL, fcntl(sp->sock, F_GETFL) & ~O_NONBLOCK);
#endif

	} else {
#if (defined (CE) || defined (WIN))
		ioctlsocket(sp->sock, FIONBIO, &flag);
#elif (defined (ECOS))
		int on;
		on = 1;
		ioctl(sp->sock, FIONBIO, &on);
#elif (defined (VXWORKS) || defined (NW))
		ioctl(sp->sock, FIONBIO, (int)&iflag);
#else
		fcntl(sp->sock, F_SETFL, fcntl(sp->sock, F_GETFL) | O_NONBLOCK);
#endif
	}
	return oldBlock;
}

/******************************************************************************/
/*
 *	Return true if a readable socket has buffered data. - not public
 */

int socketDontBlock()
{
	socket_t	*sp;
	int			i;

	for (i = 0; i < socketMax; i++) {
		if ((sp = socketList[i]) == NULL || 
				(sp->handlerMask & SOCKET_READABLE) == 0) {
			continue;
		}
		if (socketInputBuffered(i) > 0) {
			return 1;
		}
	}
	return 0;
}

/******************************************************************************/
/*
 *	Return true if a particular socket buffered data. - not public
 */

int socketSockBuffered(int sock)
{
	socket_t	*sp;
	int			i;

	for (i = 0; i < socketMax; i++) {
		if ((sp = socketList[i]) == NULL || sp->sock != sock) {
			continue;
		}
		return socketInputBuffered(i);
	}
	return 0;
}

#endif /* (!WIN) | LITTLEFOOT | WEBS */

/******************************************************************************/


