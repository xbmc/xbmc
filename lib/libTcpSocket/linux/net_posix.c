/*
 *  Networking under POSIX
 *  Copyright (C) 2007-2008 Andreas Öman
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

#ifdef __APPLE__
/* Needed on Mac OS/X */
#ifndef SOL_TCP
#define SOL_TCP IPPROTO_TCP
#endif
#include "OSXGNUReplacements.h"
#elif defined(__FreeBSD__)
#ifndef SOL_TCP
#define SOL_TCP IPPROTO_TCP
#endif
#else
#include <sys/epoll.h>
#endif
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <fcntl.h>

#include "../os-dependent_socket.h"

socket_t
tcp_connect_addr(struct addrinfo* addr, char *szErrbuf, size_t nErrbufSize,
    int nTimeout)
{
  socket_t fdSock;

  fdSock = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
  if(fdSock == -1) {
    snprintf(szErrbuf, nErrbufSize, "Unable to create socket: %s", strerror(errno));
    return SOCKET_ERROR;
  }

  /**
   * Switch to nonblocking
   */
  if (tcp_connect_addr_socket_nonblocking(addr, fdSock, szErrbuf, nErrbufSize, nTimeout) == SOCKET_ERROR)
    return SOCKET_ERROR;

  return fdSock;
}

int
tcp_connect_addr_socket_nonblocking(struct addrinfo* addr, socket_t fdSock, char *szErrbuf, size_t nErrbufSize,
    int nTimeout)
{
  int r, err, val;
  socklen_t errlen = sizeof(int);

  fcntl(fdSock, F_SETFL, fcntl(fdSock, F_GETFL) | O_NONBLOCK);

  r = connect(fdSock, addr->ai_addr, addr->ai_addrlen);

  if(r == -1) {
    if(errno == EINPROGRESS) {
      struct pollfd pfd;

      pfd.fd = fdSock;
      pfd.events = POLLOUT;
      pfd.revents = 0;

      r = poll(&pfd, 1, nTimeout);
      if(r == 0) {
        /* Timeout */
        snprintf(szErrbuf, nErrbufSize, "Connection attempt timed out");
        close(fdSock);
        return SOCKET_ERROR;
      }

      if(r == -1) {
        snprintf(szErrbuf, nErrbufSize, "poll() error: %s", strerror(errno));
        close(fdSock);
        return SOCKET_ERROR;
      }

      getsockopt(fdSock, SOL_SOCKET, SO_ERROR, (void *)&err, &errlen);
    } else {
      err = errno;
    }
  } else {
    err = 0;
  }

  if(err != 0) {
    snprintf(szErrbuf, nErrbufSize, "%s", strerror(err));
    close(fdSock);
    return SOCKET_ERROR;
  }

  fcntl(fdSock, F_SETFL, fcntl(fdSock, F_GETFL) & ~O_NONBLOCK);

  val = 1;
  setsockopt(fdSock, SOL_TCP, TCP_NODELAY, &val, sizeof(val));

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
      snprintf(szErrbuf, nErrbufSize, "The specified host is unknown");
      break;

    case EAI_FAIL:
      snprintf(szErrbuf, nErrbufSize, "A nonrecoverable failure in name resolution occurred");
      break;

    case EAI_MEMORY:
      snprintf(szErrbuf, nErrbufSize, "A memory allocation failure occurred");
      break;

    case EAI_AGAIN:
      snprintf(szErrbuf, nErrbufSize, "A temporary error occurred on an authoritative name server");
      break;

    default:
      snprintf(szErrbuf, nErrbufSize, "Unknown error %d", res);
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
    return errno;
  if(x != (int)nLen)
    return ECONNRESET;
  return 0;
}

int
tcp_read_timeout(socket_t fdSock, void *buf, size_t nLen, int nTimeout)
{
  int x, tot = 0;
  struct pollfd fds;

  if(nTimeout <= 0)
    return EINVAL;

  fds.fd = fdSock;
  fds.events = POLLIN;
  fds.revents = 0;

  while(tot != (int)nLen) {

    x = poll(&fds, 1, nTimeout);
    if(x == 0)
      return ETIMEDOUT;

    x = recv(fdSock, buf + tot, nLen - tot, MSG_DONTWAIT);
    if(x == -1) {
      if(errno == EAGAIN)
        continue;
      return errno;
    }

    if(x == 0)
      return ECONNRESET;

    tot += x;
  }
  return 0;
}

void
tcp_close(socket_t fdSock)
{
  close(fdSock);
}
