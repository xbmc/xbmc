/*
 * sock.c -- Posix Socket upper layer support module for general posix use
 *
 * Copyright (c) GoAhead Software Inc., 1995-2000. All Rights Reserved.
 *
 * $Id: sock.c,v 1.3 2002/10/24 14:44:50 bporter Exp $
 */

/******************************** Description *********************************/

/*
 *	Posix Socket Module.  This supports blocking and non-blocking buffered 
 *	socket I/O.
 */

/********************************** Includes **********************************/

#include	<string.h>
#include	<stdlib.h>

#ifdef UEMF
	#include	"uemf.h"
#else
	#include	<socket.h>
	#include	<types.h>
	#include	<unistd.h>
	#include	"emfInternal.h"
#endif

#ifdef _XBOX
	#define FD_WRITE        0x02
	#define FD_CONNECT      0x10
#endif //_XBOX
/************************************ Locals **********************************/

socket_t	**socketList;			/* List of open sockets */
int			socketMax;				/* Maximum size of socket */
int			socketHighestFd = -1;	/* Highest socket fd opened */

/***************************** Forward Declarations ***************************/

static int	socketDoOutput(socket_t *sp, char *buf, int toWrite, int *errCode);
static int	tryAlternateSendTo(int sock, char *buf, int toWrite, int i,
			struct sockaddr *server);

/*********************************** Code *************************************/
/*
 *	Write to a socket. Absorb as much data as the socket can buffer. Block if 
 *	the socket is in blocking mode. Returns -1 on error, otherwise the number 
 *	of bytes written.
 */

int	socketWrite(int sid, char *buf, int bufsize)
{
	socket_t	*sp;
	ringq_t		*rq;
	int			len, bytesWritten, room;

	a_assert(buf);
	a_assert(bufsize >= 0);

	if ((sp = socketPtr(sid)) == NULL) {
		return -1;
	}

/*
 *	Loop adding as much data to the output ringq as we can absorb. Initiate a 
 *	flush when the ringq is too full and continue. Block in socketFlush if the
 *	socket is in blocking mode.
 */
	rq = &sp->outBuf;
	for (bytesWritten = 0; bufsize > 0; ) {
		if ((room = ringqPutBlkMax(rq)) == 0) {
			if (socketFlush(sid) < 0) {
				return -1;
			}
			if ((room = ringqPutBlkMax(rq)) == 0) {
				if (sp->flags & SOCKET_BLOCK) {
#if (defined (WIN) || defined (CE))
					int		errCode;
					if (! socketWaitForEvent(sp,  FD_WRITE | SOCKET_WRITABLE,
						&errCode)) {
						return -1;
					}
#endif
					continue;
				}
				break;
			}
			continue;
		}
		len = min(room, bufsize);
		ringqPutBlk(rq, (unsigned char *) buf, len);
		bytesWritten += len;
		bufsize -= len;
		buf += len;
	}
	return bytesWritten;
}

/******************************************************************************/
/*
 *	Write a string to a socket
 */

int	socketWriteString(int sid, char_t *buf)
{
 #ifdef UNICODE
 	char	*byteBuf;
 	int		r, len;
 
 	len = gstrlen(buf);
 	byteBuf = ballocUniToAsc(buf, len);
 	r = socketWrite(sid, byteBuf, len);
 	bfreeSafe(B_L, byteBuf);
 	return r;
 #else
 	return socketWrite(sid, buf, strlen(buf));
 #endif /* UNICODE */
}

/******************************************************************************/
/*
 *	Read from a socket. Return the number of bytes read if successful. This
 *	may be less than the requested "bufsize" and may be zero. Return -1 for
 *	errors. Return 0 for EOF. Otherwise return the number of bytes read. 
 *	If this routine returns zero it indicates an EOF condition.
 *  which can be verified with socketEof()
 
 *	Note: this ignores the line buffer, so a previous socketGets
 *	which read a partial line may cause a subsequent socketRead to miss some 
 *	data. This routine may block if the socket is in blocking mode.
 *
 */

int	socketRead(int sid, char *buf, int bufsize)
{
	socket_t	*sp;
	ringq_t		*rq;
	int			len, room, errCode, bytesRead;

	a_assert(buf);
	a_assert(bufsize > 0);

	if ((sp = socketPtr(sid)) == NULL) {
		return -1;
	}

	if (sp->flags & SOCKET_EOF) {
		return 0;
	}

	rq = &sp->inBuf;
	for (bytesRead = 0; bufsize > 0; ) {
		len = min(ringqLen(rq), bufsize);
		if (len <= 0) {
/*
 *			if blocking mode and already have data, exit now or it may block
 *			forever.
 */
			if ((sp->flags & SOCKET_BLOCK) &&
				(bytesRead > 0)) {
				break;
			}
/*
 *			This flush is critical for readers of datagram packets. If the
 *			buffer is not big enough to read the whole datagram in one hit,
 *			the recvfrom call will fail. 
 */
			ringqFlush(rq);
			room = ringqPutBlkMax(rq);
			len = socketGetInput(sid, (char *) rq->endp, room, &errCode);
			if (len < 0) {
				if (errCode == EWOULDBLOCK) {
					if ((sp->flags & SOCKET_BLOCK) &&
						(bytesRead ==  0)) {
						continue;
					}
					if (bytesRead >= 0) {
						return bytesRead;
					}
				}
				return -1;

			} else if (len == 0) {
/*
 *				If bytesRead is 0, this is EOF since socketRead should never
 *				be called unless there is data yet to be read.  Set the flag.  
 *				Then pass back the number of bytes read.
 */
				if (bytesRead == 0) {
					sp->flags |= SOCKET_EOF;
				}
				return bytesRead;
			}
			ringqPutBlkAdj(rq, len);
			len = min(len, bufsize);
		}
		memcpy(&buf[bytesRead], rq->servp, len);
		ringqGetBlkAdj(rq, len);
		bufsize -= len;
		bytesRead += len;
	}
	return bytesRead;
}

/******************************************************************************/
/*
 *	Get a string from a socket. This returns data in *buf in a malloced string
 *	after trimming the '\n'. If there is zero bytes returned, *buf will be set
 *	to NULL. If doing non-blocking I/O, it returns -1 for error, EOF or when 
 *	no complete line yet read. If doing blocking I/O, it will block until an
 *	entire line is read. If a partial line is read socketInputBuffered or 
 *	socketEof can be used to distinguish between EOF and partial line still 
 *	buffered. This routine eats and ignores carriage returns.
 */

int	socketGets(int sid, char_t **buf)
{
	socket_t	*sp;
	ringq_t		*lq;
	char		c;
	int			rc, len;

	a_assert(buf);
	*buf = NULL;

	if ((sp = socketPtr(sid)) == NULL) {
		return -1;
	}
	lq = &sp->lineBuf;

	while (1) {

		if ((rc = socketRead(sid, &c, 1)) < 0) {
			return rc;
		}
		
		if (rc == 0) {
/*
 *			If there is a partial line and we are at EOF, pretend we saw a '\n'
 */
			if (ringqLen(lq) > 0 && (sp->flags & SOCKET_EOF)) {
				c = '\n';
			} else {
				return -1;
			}
		}
/*
 *		If a newline is seen, return the data excluding the new line to the
 *		caller. If carriage return is seen, just eat it.
 */
		if (c == '\n') {
			len = ringqLen(lq);
			if (len > 0) {
				*buf = ballocAscToUni((char *)lq->servp, len);
			} else {
				*buf = NULL;
			}
			ringqFlush(lq);
			return len;

		} else if (c == '\r') {
			continue;
		}
		ringqPutcA(lq, c);
	}
	return 0;
}

/******************************************************************************/
/*
 *	Flush the socket. Block if the socket is in blocking mode.
 *	This will return -1 on errors and 0 if successful.
 */

int socketFlush(int sid)
{
	socket_t	*sp;
	ringq_t		*rq;
	int			len, bytesWritten, errCode;

	if ((sp = socketPtr(sid)) == NULL) {
		return -1;
	}
	rq = &sp->outBuf;

/*
 *	Set the background flushing flag which socketEventProc will check to
 *	continue the flush.
 */
	if (! (sp->flags & SOCKET_BLOCK)) {
		sp->flags |= SOCKET_FLUSHING;
	}

/*
 *	Break from loop if not blocking after initiating output. If we are blocking
 *	we wait for a write event.
 */
	while (ringqLen(rq) > 0) {
		len = ringqGetBlkMax(&sp->outBuf);
		bytesWritten = socketDoOutput(sp, (char*) rq->servp, len, &errCode);
		if (bytesWritten < 0) {
			if (errCode == EINTR) {
				continue;
			} else if (errCode == EWOULDBLOCK || errCode == EAGAIN) {
#if (defined (WIN) || defined (CE))
				if (sp->flags & SOCKET_BLOCK) {
					int		errCode;
					if (! socketWaitForEvent(sp,  FD_WRITE | SOCKET_WRITABLE,
						&errCode)) {
						return -1;
					}
					continue;
				} 
#endif
/*
 *				Ensure we get a FD_WRITE message when the socket can absorb
 *				more data (non-blocking only.) Store the user's mask if we
 *				haven't done it already.
 */
				if (sp->saveMask < 0 ) {
					sp->saveMask = sp->handlerMask;
					socketRegisterInterest(sp, 
					sp->handlerMask | SOCKET_WRITABLE);
				}
				return 0;
			}
			return -1;
		}
		ringqGetBlkAdj(rq, bytesWritten);
	}
/*
 *	If the buffer is empty, reset the ringq pointers to point to the start
 *	of the buffer. This is essential to ensure that datagrams get written
 *	in one single I/O operation.
 */
	if (ringqLen(rq) == 0) {
		ringqFlush(rq);
	}
/*
 *	Restore the users mask if it was saved by the non-blocking code above.
 *	Note: saveMask = -1 if empty. socketRegisterInterest will set handlerMask
 */
	if (sp->saveMask >= 0) {
		socketRegisterInterest(sp, sp->saveMask);
		sp->saveMask = -1;
	}
	sp->flags &= ~SOCKET_FLUSHING;
	return 0;
}

/******************************************************************************/
/*
 *	Return the count of input characters buffered. We look at both the line
 *	buffer and the input (raw) buffer. Return -1 on error or EOF.
 */

int socketInputBuffered(int sid)
{
	socket_t	*sp;

	if ((sp = socketPtr(sid)) == NULL) {
		return -1;
	}
	if (socketEof(sid)) {
		return -1;
	}
	return ringqLen(&sp->lineBuf) + ringqLen(&sp->inBuf);
}

/******************************************************************************/
/*
 *	Return true if EOF
 */

int socketEof(int sid)
{
	socket_t	*sp;

	if ((sp = socketPtr(sid)) == NULL) {
		return -1;
	}
	return sp->flags & SOCKET_EOF;
}

/******************************************************************************/
/*
 *	Return the number of bytes the socket can absorb without blocking
 */

int socketCanWrite(int sid)
{
	socket_t	*sp;

	if ((sp = socketPtr(sid)) == NULL) {
		return -1;
	}
	return sp->outBuf.buflen - ringqLen(&sp->outBuf) - 1;
}

/******************************************************************************/
/*
 *	Add one to allow the user to write exactly SOCKET_BUFSIZ
 */

void socketSetBufferSize(int sid, int in, int line, int out)
{
	socket_t	*sp;

	if ((sp = socketPtr(sid)) == NULL) {
		return;
	}

	if (in >= 0) {
		ringqClose(&sp->inBuf);
		in++;
		ringqOpen(&sp->inBuf, in, in);
	}

	if (line >= 0) {
		ringqClose(&sp->lineBuf);
		line++;
		ringqOpen(&sp->lineBuf, line, line);
	}

	if (out >= 0) {
		ringqClose(&sp->outBuf);
		out++;
		ringqOpen(&sp->outBuf, out, out);
	}
}

/******************************************************************************/
/*
 *	Create a user handler for this socket. The handler called whenever there
 *	is an event of interest as defined by handlerMask (SOCKET_READABLE, ...)
 */

void socketCreateHandler(int sid, int handlerMask, socketHandler_t handler, 
		int data)
{
	socket_t	*sp;

	if ((sp = socketPtr(sid)) == NULL) {
		return;
	}
	sp->handler = handler;
	sp->handler_data = data;
	socketRegisterInterest(sp, handlerMask);
}

/******************************************************************************/
/*
 *	Delete a handler
 */

void socketDeleteHandler(int sid)
{
	socket_t	*sp;

	if ((sp = socketPtr(sid)) == NULL) {
		return;
	}
	sp->handler = NULL;
	socketRegisterInterest(sp, 0);
}

/******************************************************************************/
/*
 *	Socket output procedure. Return -1 on errors otherwise return the number
 *	of bytes written.
 */

static int socketDoOutput(socket_t *sp, char *buf, int toWrite, int *errCode)
{
	struct sockaddr_in	server;
	int					bytes;

	a_assert(sp);
	a_assert(buf);
	a_assert(toWrite > 0);
	a_assert(errCode);

	*errCode = 0;

#if (defined (WIN) || defined (CE))
	if ((sp->flags & SOCKET_ASYNC)
			&& ! socketWaitForEvent(sp,  FD_CONNECT, errCode)) {
		return -1;
	}
#endif

/*
 *	Write the data
 */
	if (sp->flags & SOCKET_BROADCAST) {
		server.sin_family = AF_INET;
#if (defined (UEMF) || defined (LITTLEFOOT))
		server.sin_addr.s_addr = INADDR_BROADCAST;
#else
		server.sin_addr.s_addr = inet_addr(basicGetBroadcastAddress());
#endif
		server.sin_port = htons((short)(sp->port & 0xFFFF));
		if ((bytes = sendto(sp->sock, buf, toWrite, 0,
			(struct sockaddr *) &server, sizeof(server))) < 0) {
			bytes = tryAlternateSendTo(sp->sock, buf, toWrite, 0,
			(struct sockaddr *) &server);
		}
	} else if (sp->flags & SOCKET_DATAGRAM) {
		server.sin_family = AF_INET;
		server.sin_addr.s_addr = inet_addr(sp->host);
		server.sin_port = htons((short)(sp->port & 0xFFFF));
		bytes = sendto(sp->sock, buf, toWrite, 0,
			(struct sockaddr *) &server, sizeof(server));

	} else {
		bytes = send(sp->sock, buf, toWrite, 0);
	}

	if (bytes < 0) {
		*errCode = socketGetError();
#if (defined (WIN) || defined (CE))
		sp->currentEvents &= ~FD_WRITE;
#endif

		return -1;

	} else if (bytes == 0 && bytes != toWrite) {
		*errCode = EWOULDBLOCK;
#if (defined (WIN) || defined (CE))
		sp->currentEvents &= ~FD_WRITE;
#endif
		return -1;
	}

/*
 *	Ensure we get to write some more data real soon if the socket can absorb
 *	more data
 */
#ifndef UEMF
#ifdef WIN 
	if (sp->interestEvents & FD_WRITE) {
		emfTime_t blockTime = { 0, 0 };
		emfSetMaxBlockTime(&blockTime);
	}
#endif /* WIN */
#endif
	return bytes;
}

/******************************************************************************/
/*
 *		If the sendto failed, swap the first two bytes in the 
 *		sockaddr structure.  This is a kludge due to a change in
 *		VxWorks between versions 5.3 and 5.4, but we want the 
 *		product to run on either.
 */
static int tryAlternateSendTo(int sock, char *buf, int toWrite, int i,
			struct sockaddr *server)
{
#ifdef VXWORKS
	char *ptr;

	ptr = (char *)server;
	*ptr = *(ptr+1);
	*(ptr+1) = 0;
	return sendto(sock, buf, toWrite, i, server, sizeof(struct sockaddr));
#else
	return -1;
#endif /* VXWORKS */
}

/******************************************************************************/
/*
 *	Allocate a new socket structure
 */

int socketAlloc(char *host, int port, socketAccept_t accept, int flags)
{
	socket_t	*sp;
	int			sid;

	if ((sid = hAllocEntry((void***) &socketList, &socketMax,
			sizeof(socket_t))) < 0) {
		return -1;
	}
	sp = socketList[sid];

	sp->sid = sid;
	sp->accept = accept;
	sp->port = port;
	sp->fileHandle = -1;
	sp->saveMask = -1;

	if (host) {
		strncpy(sp->host, host, sizeof(sp->host));
	}

/*
 *	Preserve only specified flags from the callers open
 */
	a_assert((flags & ~(SOCKET_BROADCAST|SOCKET_DATAGRAM|SOCKET_BLOCK|
						SOCKET_LISTENING)) == 0);
	sp->flags = flags & (SOCKET_BROADCAST | SOCKET_DATAGRAM | SOCKET_BLOCK|
						SOCKET_LISTENING);

/*
 *	Add one to allow the user to write exactly SOCKET_BUFSIZ
 */
	ringqOpen(&sp->inBuf, SOCKET_BUFSIZ, SOCKET_BUFSIZ);
	ringqOpen(&sp->outBuf, SOCKET_BUFSIZ + 1, SOCKET_BUFSIZ + 1);
	ringqOpen(&sp->lineBuf, SOCKET_BUFSIZ, -1);

	return sid;
}

/******************************************************************************/
/*
 *	Free a socket structure
 */

void socketFree(int sid)
{
	socket_t	*sp;
	char_t		buf[256];
	int			i;

	if ((sp = socketPtr(sid)) == NULL) {
		return;
	}

/*
 *	To close a socket, remove any registered interests, set it to
 *	non-blocking so that the recv which follows won't block, do a
 *	shutdown on it so peers on the other end will receive a FIN,
 *	then read any data not yet retrieved from the receive buffer,
 *	and finally close it.  If these steps are not all performed
 *	RESETs may be sent to the other end causing problems.
 */
	socketRegisterInterest(sp, 0);
	// QS - changed from >= 0 to != SOCKET_ERROR
	if (sp->sock != SOCKET_ERROR) {
		socketSetBlock(sid, 0);
		// QS - changed from >= 0 to != SOCKET_ERROR
		if (shutdown(sp->sock, 1) != SOCKET_ERROR) {
			recv(sp->sock, buf, sizeof(buf), 0);
		}
#if (defined (WIN) || defined (CE))
		closesocket(sp->sock);
#else
		close(sp->sock);
#endif
	}

	ringqClose(&sp->inBuf);
	ringqClose(&sp->outBuf);
	ringqClose(&sp->lineBuf);

	bfree(B_L, sp);
	socketMax = hFree((void***) &socketList, sid);

/*
 *	Calculate the new highest socket number
 */
	socketHighestFd = -1;
	for (i = 0; i < socketMax; i++) {
		if ((sp = socketList[i]) == NULL) {
			continue;
		} 
		socketHighestFd = max(socketHighestFd, sp->sock);
	}
}

/******************************************************************************/
/*
 *	Validate a socket handle
 */

socket_t *socketPtr(int sid)
{
	if (sid < 0 || sid >= socketMax || socketList[sid] == NULL) {
		a_assert(NULL);
		errno = EBADF;
		return NULL;
	}

	a_assert(socketList[sid]);
	return socketList[sid];
}

/******************************************************************************/
/*
 *	Get the operating system error code
 */

int socketGetError()
{
#if (defined (WIN) || defined (CE))
	switch (WSAGetLastError()) {
	case WSAEWOULDBLOCK:
		return EWOULDBLOCK;
	case WSAECONNRESET:
		return ECONNRESET;
	case WSAENETDOWN:
		return ENETDOWN;
	case WSAEPROCLIM:
		return EAGAIN;
	case WSAEINTR:
		return EINTR;
	default:
		return EINVAL;
	}
#else
	return errno;
#endif
}

/******************************************************************************/
/*
 *	Return the underlying socket handle
 */

int socketGetHandle(int sid)
{
	socket_t	*sp;

	if ((sp = socketPtr(sid)) == NULL) {
		return -1;
	}
	return sp->sock;
}

/******************************************************************************/
/*
 *	Get blocking mode
 */

int socketGetBlock(int sid)
{
	socket_t	*sp;

	if ((sp = socketPtr(sid)) == NULL) {
		a_assert(0);
		return 0;
	}
	return (sp->flags & SOCKET_BLOCK);
}

/******************************************************************************/
/*
 *	Get mode
 */

int socketGetMode(int sid)
{
	socket_t	*sp;

	if ((sp = socketPtr(sid)) == NULL) {
		a_assert(0);
		return 0;
	}
	return sp->flags;
}

/******************************************************************************/
/*
 *	Set mode
 */

void socketSetMode(int sid, int mode)
{
	socket_t	*sp;

	if ((sp = socketPtr(sid)) == NULL) {
		a_assert(0);
		return;
	}
	sp->flags = mode;
}

/******************************************************************************/
/*
 *	Get port.
 */

int socketGetPort(int sid)
{
	socket_t	*sp;

	if ((sp = socketPtr(sid)) == NULL) {
		return -1;
	}
	return sp->port;
}

/******************************************************************************/

