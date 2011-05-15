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

int
tcp_connect_poll(struct addrinfo* addr, socket_t fdSock, char *szErrbuf, size_t nErrbufSize,
    int nTimeout)
{
  int nRes, nErr = 0;
  socklen_t errlen = sizeof(int);

  /* switch to non blocking */
  fcntl(fdSock, F_SETFL, fcntl(fdSock, F_GETFL) | O_NONBLOCK);

  /* connect to the other side */
  nRes = connect(fdSock, addr->ai_addr, addr->ai_addrlen);

  /* poll until a connection is established */
  if (nRes == -1)
  {
    if (errno == EINPROGRESS)
    {
      struct pollfd pfd;
      pfd.fd = fdSock;
      pfd.events = POLLOUT;
      pfd.revents = 0;

      nRes = poll(&pfd, 1, nTimeout);
      if (nRes == 0)
      {
        snprintf(szErrbuf, nErrbufSize, "attempt timed out after %d milliseconds", nTimeout);
        return SOCKET_ERROR;
      }
      else if (nRes == -1)
      {
        snprintf(szErrbuf, nErrbufSize, "poll() error '%s'", strerror(errno));
        return SOCKET_ERROR;
      }

      /* check for errors */
      getsockopt(fdSock, SOL_SOCKET, SO_ERROR, (void *)&nErr, &errlen);
    }
    else
    {
      nErr = errno;
    }
  }

  if (nErr != 0)
  {
    snprintf(szErrbuf, nErrbufSize, "%s", strerror(nErr));
    return SOCKET_ERROR;
  }

  /* switch back to blocking */
  fcntl(fdSock, F_SETFL, fcntl(fdSock, F_GETFL) & ~O_NONBLOCK);

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
    snprintf(szErrbuf, nErrbufSize, "Unable to create socket: %s", strerror(errno));
    return SOCKET_ERROR;
  }

  /* connect to the socket */
  if (tcp_connect_poll(addr, fdSock, szErrbuf, nErrbufSize, nTimeout) != 0)
  {
    close(fdSock);
    return SOCKET_ERROR;
  }

  /* set TCP_NODELAY socket option */
  nVal = 1;
  setsockopt(fdSock, SOL_TCP, TCP_NODELAY, &nVal, sizeof(nVal));

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
      snprintf(szErrbuf, nErrbufSize, "the specified host is unknown");
      break;

    case EAI_FAIL:
      snprintf(szErrbuf, nErrbufSize, "a nonrecoverable failure in name resolution occurred");
      break;

    case EAI_MEMORY:
      snprintf(szErrbuf, nErrbufSize, "a memory allocation failure occurred");
      break;

    case EAI_AGAIN:
      snprintf(szErrbuf, nErrbufSize, "a temporary error occurred on an authoritative name server");
      break;

    default:
      snprintf(szErrbuf, nErrbufSize, "unknown error %d", nRes);
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
  int x = recv(fdSock, buf, nLen, MSG_WAITALL);

  if (x == -1)
    return errno;
  if (x != (int)nLen)
    return ECONNRESET;

  return 0;
}

int
tcp_read_timeout(socket_t fdSock, void *buf, size_t nLen, int nTimeout)
{
  int x, tot = 0;
  struct pollfd fds;

  if (nTimeout <= 0)
    return EINVAL;

  fds.fd = fdSock;
  fds.events = POLLIN;
  fds.revents = 0;

  while(tot != (int)nLen)
  {
    x = poll(&fds, 1, nTimeout);
    if (x == 0)
      return ETIMEDOUT;

    x = recv(fdSock, buf + tot, nLen - tot, MSG_DONTWAIT);
    if (x == -1)
    {
      if (errno == EAGAIN)
        continue;
      return errno;
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
    close(fdSock);
}

void
tcp_shutdown(socket_t fdSock)
{
  if (fdSock != SOCKET_ERROR)
    shutdown(fdSock, SHUT_RDWR);
}

int
tcp_send(socket_t fdSock, void *buf, int len, int flags)
{
  if (fdSock != SOCKET_ERROR)
    return (int) send(fdSock, buf, len, flags); // safe since "len" is an int
  else
    return -1;
}
