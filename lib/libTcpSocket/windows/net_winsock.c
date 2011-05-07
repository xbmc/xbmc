/*
 *  Networking under WINDOWS
 *  Copyright (C) 2007-2008 Andreas Öman
 *  Copyright (C) 2007-2008 Joakim Plate
 *  Copyright (C) 2011 Team XBMC
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "../os-dependent_socket.h"

#ifndef EINPROGRESS
#define EINPROGRESS WSAEINPROGRESS
#endif

#ifndef ECONNRESET
#define ECONNRESET  WSAECONNRESET
#endif

#ifndef EAGAIN
#define EAGAIN      WSAEWOULDBLOCK
#endif

#ifndef EINVAL
#define EINVAL      WSAEINVAL
#endif

#ifndef MSG_WAITALL
#define MSG_WAITALL 0x8
#endif

static int recv_fixed (__in SOCKET fdSock, __out_bcount_part(len, return) __out_data_source(NETWORK) char FAR *szBuf, __in int nLen, __in int nFlags)
{
  char* org = szBuf;
  int   nRes = 1;

  if ((nFlags & MSG_WAITALL) == 0)
    return recv(fdSock, szBuf, nLen, nFlags);

  nFlags &= ~MSG_WAITALL;
  while(nLen > 0 && nRes > 0)
  {
    nRes = recv(fdSock, szBuf, nLen, nFlags);
    if (nRes < 0)
      return nRes;

    szBuf += nRes;
    nLen -= nRes;
  }
  return szBuf - org;
}

int
tcp_connect_poll(struct addrinfo* addr, socket_t fdSock, char *szErrbuf, size_t nErrbufSize,
    int nTimeout)
{
  int nRes, nErr = 0;
  socklen_t errlen = sizeof(int);

  /* switch to non blocking */
  u_long nVal = 1;
  ioctlsocket(fdSock, FIONBIO, &nVal);

  /* connect to the other side */
  nRes = connect(fdSock, addr->ai_addr, addr->ai_addrlen);

  /* poll until a connection is established */
  if (nRes == -1)
  {
    if (WSAGetLastError() == EINPROGRESS ||
       WSAGetLastError() == EAGAIN)
    {
      fd_set fd_write, fd_except;
      struct timeval tv;
      tv.tv_sec  =         nTimeout / 1000;
      tv.tv_usec = 1000 * (nTimeout % 1000);

      FD_ZERO(&fd_write);
      FD_ZERO(&fd_except);
      FD_SET(fdSock, &fd_write);
      FD_SET(fdSock, &fd_except);

      nRes = select(sizeof(fdSock)*8, NULL, &fd_write, &fd_except, &tv);
      if (nRes == 0)
      {
        _snprintf(szErrbuf, nErrbufSize, "attempt timed out after %d milliseconds", nTimeout);
        return SOCKET_ERROR;
      }

      else if (nRes == -1)
      {
        _snprintf(szErrbuf, nErrbufSize, "select() error: %s", strerror(WSAGetLastError()));
        return SOCKET_ERROR;
      }

      /* check for errors */
      getsockopt(fdSock, SOL_SOCKET, SO_ERROR, (char *)&nErr, &errlen);
    }
    else
    {
      nErr = WSAGetLastError();
    }
  }

  if (nErr != 0)
  {
    _snprintf(szErrbuf, nErrbufSize, "%s", strerror(nErr));
    return SOCKET_ERROR;
  }

  nVal = 0;
  ioctlsocket(fdSock, FIONBIO, &nVal);

  return 0;
}

socket_t
tcp_connect_addr(struct addrinfo* addr, char *szErrbuf, size_t nErrbufSize,
    int nTimeout)
{
  int nVal;
  socket_t fdSock;

  /* create the socket */
  fdSock = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
  if (fdSock == -1)
  {
    _snprintf(szErrbuf, nErrbufSize, "Unable to create socket: %s", strerror(WSAGetLastError()));
    return SOCKET_ERROR;
  }

  /* connect to the socket */
  if (tcp_connect_poll(addr, fdSock, szErrbuf, nErrbufSize, nTimeout) != 0)
  {
    closesocket(fdSock);
    return SOCKET_ERROR;
  }

  /* set TCP_NODELAY socket option */
  nVal = 1;
  setsockopt(fdSock, IPPROTO_TCP, TCP_NODELAY, (const char*)&nVal, sizeof(nVal));

  return fdSock;
}

socket_t
tcp_connect(const char *szHostname, int nPort, char *szErrbuf, size_t nErrbufSize,
    int nTimeout)
{
  struct   addrinfo hints;
  struct   addrinfo *result, *addr;
  char     service[33];
  int      nRes;
  socket_t fdSock = INVALID_SOCKET;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family   = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;
  sprintf(service, "%d", nPort);

  nRes = getaddrinfo(szHostname, service, &hints, &result);
  if (nRes)
  {
    switch(nRes)
    {
    case EAI_NONAME:
      _snprintf(szErrbuf, nErrbufSize, "the specified host is unknown");
      break;

    case EAI_FAIL:
      _snprintf(szErrbuf, nErrbufSize, "a nonrecoverable failure in name resolution occurred");
      break;

    case EAI_MEMORY:
      _snprintf(szErrbuf, nErrbufSize, "a memory allocation failure occurred");
      break;

    case EAI_AGAIN:
      _snprintf(szErrbuf, nErrbufSize, "a temporary error occurred on an authoritative name server");
      break;

    default:
      _snprintf(szErrbuf, nErrbufSize, "unknown error %d", nRes);
      break;
    }

    return SOCKET_ERROR;
  }

  for(addr = result; addr; addr = addr->ai_next)
  {
    fdSock = tcp_connect_addr(addr, szErrbuf, nErrbufSize, nTimeout);
    if (fdSock != INVALID_SOCKET)
      break;
  }

  freeaddrinfo(result);
  return fdSock;
}

int
tcp_read(socket_t fdSock, void *buf, size_t nLen)
{
  int x = recv_fixed(fdSock, (char *)buf, nLen, MSG_WAITALL);

  if (x == -1)
    return WSAGetLastError();
  if (x != (int)nLen)
    return ECONNRESET;

  return 0;
}

int
tcp_read_timeout(socket_t fdSock, void *buf, size_t nLen, int nTimeout)
{
  int x, tot = 0, nErr;
  u_long nVal;
  fd_set fd_read;
  struct timeval tv;

  if (nTimeout <= 0)
    return EINVAL;

  while(tot != (int)nLen)
  {
    tv.tv_sec  =         nTimeout / 1000;
    tv.tv_usec = 1000 * (nTimeout % 1000);

    FD_ZERO(&fd_read);
    FD_SET(fdSock, &fd_read);

    x = select(sizeof(fdSock)*8, &fd_read, NULL, NULL, &tv);

    if (x == 0)
      return ETIMEDOUT;

    nVal = 1;
    ioctlsocket(fdSock, FIONBIO, &nVal);

    x = recv_fixed(fdSock, (char *)buf + tot, nLen - tot, 0);
    nErr = WSAGetLastError();

    nVal = 0;
    ioctlsocket(fdSock, FIONBIO, &nVal);

    if (x == -1)
    {
      if (nErr == EAGAIN)
        continue;
      return nErr;
    }

    if (x == 0)
      return ECONNRESET;

    tot += x;
  }
  return 0;
}

void
tcp_close(socket_t fdSock)
{
  if (fdSock != SOCKET_ERROR)
    closesocket(fdSock);
}

int
tcp_send(__in SOCKET fdSock, __in_bcount(len) const char FAR * buf, __in int len, __in int flags)
{
  if (fdSock != SOCKET_ERROR)
    return send(fdSock, buf, len, flags);
  else
    return -1;
}
