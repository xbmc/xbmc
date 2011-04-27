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

static int recv_fixed (SOCKET fdSock, char * szBuf, int nLen, int nFlags)
{
  char* org = szBuf;
  int   res = 1;

  if((nFlags & MSG_WAITALL) == 0)
    return recv(fdSock, szBuf, nLen, nFlags);

  nFlags &= ~MSG_WAITALL;
  while(nLen > 0 && res > 0)
  {
    res = recv(fdSock, szBuf, nLen, nFlags);
    if(res < 0)
      return res;

    szBuf += res;
    nLen -= res;
  }
  return szBuf - org;
}

#define recv(fdSock, szBuf, nLen, nFlags) recv_fixed(fdSock, szBuf, nLen, nFlags)

socket_t
tcp_connect_addr(struct addrinfo* addr, char *szErrbuf, size_t nErrbufSize,
    int nTimeout)
{
  socket_t fdSock;

  fdSock = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
  if(fdSock == -1) {
    _snprintf(szErrbuf, nErrbufSize, "Unable to create socket: %s",
     strerror(WSAGetLastError()));
    return SOCKET_ERROR;
  }

  /**
   * Switch to nonblocking
   */
  if(tcp_connect_addr_socket_nonblocking(addr, fdSock, szErrbuf, nErrbufSize, nTimeout) == SOCKET_ERROR)
    return SOCKET_ERROR;

  return fdSock;
}

int
tcp_connect_addr_socket_nonblocking(struct addrinfo* addr, socket_t fdSock,
    char *szErrbuf, size_t nErrbufSize, int nTimeout)
{
  int r, err, val;
  socklen_t errlen = sizeof(int);

  val = 1;
  ioctlsocket(fdSock, FIONBIO, &val);

  r = connect(fdSock, addr->ai_addr, addr->ai_addrlen);

  if(r == -1) {
    if(WSAGetLastError() == EINPROGRESS ||
       WSAGetLastError() == EAGAIN) {
      fd_set fd_write, fd_except;
      struct timeval tv;

      tv.tv_sec  =         nTimeout / 1000;
      tv.tv_usec = 1000 * (nTimeout % 1000);

      FD_ZERO(&fd_write);
      FD_ZERO(&fd_except);

      FD_SET(fdSock, &fd_write);
      FD_SET(fdSock, &fd_except);

      r = select((int)fdSock+1, NULL, &fd_write, &fd_except, &tv);

      if(r == 0) {
        /* Timeout */
        _snprintf(szErrbuf, nErrbufSize, "Connection attempt timed out");
        tcp_close(fdSock);
        return SOCKET_ERROR;
      }

      if(r == -1) {
        _snprintf(szErrbuf, nErrbufSize, "select() error: %s", strerror(WSAGetLastError()));
        tcp_close(fdSock);
        return SOCKET_ERROR;
      }

      getsockopt(fdSock, SOL_SOCKET, SO_ERROR, (void *)&err, &errlen);
    } else {
      err = WSAGetLastError();
    }
  } else {
    err = 0;
  }

  if(err != 0) {
    _snprintf(szErrbuf, nErrbufSize, "%s", strerror(err));
    tcp_close(fdSock);
    return SOCKET_ERROR;
  }

  val = 0;
  ioctlsocket(fdSock, FIONBIO, &val);

  val = 1;
  setsockopt(fdSock, IPPROTO_TCP, TCP_NODELAY, (const char*)&val, sizeof(val));

  return 0;
}

socket_t
tcp_connect(const char *szHostname, int nPort, char *szErrbuf, size_t nErrbufSize,
    int nTimeout)
{
  struct   addrinfo hints;
  struct   addrinfo *result, *addr;
  char     service[33];
  int      res;
  socket_t fdSock = INVALID_SOCKET;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family   = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;
  sprintf(service, "%d", nPort);

  res = getaddrinfo(szHostname, service, &hints, &result);
  if(res) {
    switch(res) {
    case EAI_NONAME:
      _snprintf(szErrbuf, nErrbufSize, "The specified host is unknown");
      break;

    case EAI_FAIL:
      _snprintf(szErrbuf, nErrbufSize, "A nonrecoverable failure in name resolution occurred");
      break;

    case EAI_MEMORY:
      _snprintf(szErrbuf, nErrbufSize, "A memory allocation failure occurred");
      break;

    case EAI_AGAIN:
      _snprintf(szErrbuf, nErrbufSize, "A temporary error occurred on an authoritative name server");
      break;

    default:
      _snprintf(szErrbuf, nErrbufSize, "Unknown error %d", res);
      break;
    }
    return SOCKET_ERROR;
  }

  for(addr = result; addr; addr = addr->ai_next) {
    fdSock = tcp_connect_addr(addr, szErrbuf, nErrbufSize, nTimeout);
    if(fdSock != INVALID_SOCKET)
      break;
  }

  freeaddrinfo(result);
  return fdSock;
}

int
tcp_read(socket_t fdSock, void *buf, size_t nLen)
{
  int x = recv(fdSock, buf, nLen, MSG_WAITALL);

  if(x == -1)
    return WSAGetLastError();
  if(x != nLen)
    return ECONNRESET;
  return 0;

}

int
tcp_read_timeout(socket_t fdSock, char *buf, size_t nLen, int nTimeout)
{
  int x, tot = 0, val, err;
  fd_set fd_read;
  struct timeval tv;

  if(nTimeout <= 0)
    return EINVAL;

  while(tot != nLen) {

    tv.tv_sec  =         nTimeout / 1000;
    tv.tv_usec = 1000 * (nTimeout % 1000);

    FD_ZERO(&fd_read);
    FD_SET(fdSock, &fd_read);

    x = select((int)fdSock+1, &fd_read, NULL, NULL, &tv);

    if(x == 0)
      return ETIMEDOUT;

    val = 1;
    ioctlsocket(fdSock, FIONBIO, &val);

    x   = recv(fdSock, buf + tot, nLen - tot, 0);
    err = WSAGetLastError();

    val = 0;
    ioctlsocket(fdSock, FIONBIO, &val);

    if(x == 0)
      return ECONNRESET;
    else if(x == -1)
    {
      if(err == EAGAIN)
        continue;
      return err;
    }

    tot += x;
  }
  return 0;
}

void
tcp_close(socket_t fdSock)
{
  closesocket(fdSock);
}
