#pragma once
/*
 * This file is part of the libCEC(R) library.
 *
 * libCEC(R) is Copyright (C) 2011-2012 Pulse-Eight Limited.  All rights reserved.
 * libCEC(R) is an original work, containing original code.
 *
 * libCEC(R) is a trademark of Pulse-Eight Limited.
 *
 * This program is dual-licensed; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 *
 * Alternatively, you can license this library under a commercial license,
 * please contact Pulse-Eight Licensing for more information.
 *
 * For more information contact:
 * Pulse-Eight Licensing       <license@pulse-eight.com>
 *     http://www.pulse-eight.com/
 *     http://www.pulse-eight.net/
 */


#include "../os.h"
#include "../util/timeutils.h"
#include <stdio.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <poll.h>

/* Needed on Mac OS/X */
#ifndef SOL_TCP
#define SOL_TCP IPPROTO_TCP
#endif

namespace PLATFORM
{
  // Standard sockets
  //@{
  inline void SocketClose(socket_t socket)
  {
    if (socket != INVALID_SOCKET_VALUE)
      close(socket);
  }

  inline void SocketSetBlocking(socket_t socket, bool bSetTo)
  {
    if (socket != INVALID_SOCKET_VALUE)
    {
      if (bSetTo)
        fcntl(socket, F_SETFL, fcntl(socket, F_GETFL) & ~O_NONBLOCK);
      else
        fcntl(socket, F_SETFL, fcntl(socket, F_GETFL) | O_NONBLOCK);
    }
  }

  inline ssize_t SocketWrite(socket_t socket, int *iError, void* data, size_t len)
  {
    fd_set port;

    if (socket == INVALID_SOCKET_VALUE)
    {
      *iError = EINVAL;
      return -1;
    }

    ssize_t iBytesWritten(0);
    struct timeval *tv(NULL);

    while (iBytesWritten < (ssize_t)len)
    {
      FD_ZERO(&port);
      FD_SET(socket, &port);
      int returnv = select(socket + 1, NULL, &port, NULL, tv);
      if (returnv < 0)
      {
        *iError = errno;
        return -1;
      }
      else if (returnv == 0)
      {
        *iError = ETIMEDOUT;
        return -1;
      }

      returnv = write(socket, (char*)data + iBytesWritten, len - iBytesWritten);
      if (returnv == -1)
      {
        *iError = errno;
        return -1;
      }
      iBytesWritten += returnv;
    }

    return iBytesWritten;
  }

  inline ssize_t SocketRead(socket_t socket, int *iError, void* data, size_t len, uint64_t iTimeoutMs /*= 0*/)
  {
    fd_set port;
    struct timeval timeout, *tv;
    int64_t iNow(0), iTarget(0);
    ssize_t iBytesRead(0);
    *iError = 0;

    if (socket == INVALID_SOCKET_VALUE)
    {
      *iError = EINVAL;
      return -1;
    }

    if (iTimeoutMs > 0)
    {
      iNow    = GetTimeMs();
      iTarget = iNow + (int64_t) iTimeoutMs;
    }

    while (iBytesRead >= 0 && iBytesRead < (ssize_t)len && (iTimeoutMs == 0 || iTarget > iNow))
    {
      if (iTimeoutMs == 0)
      {
        tv = NULL;
      }
      else
      {
        timeout.tv_sec  = ((long int)iTarget - (long int)iNow) / (long int)1000.;
        timeout.tv_usec = ((long int)iTarget - (long int)iNow) % (long int)1000.;
        tv = &timeout;
      }

      FD_ZERO(&port);
      FD_SET(socket, &port);
      int32_t returnv = select(socket + 1, &port, NULL, NULL, tv);

      if (returnv == -1)
      {
        *iError = errno;
        return -1;
      }
      else if (returnv == 0)
      {
        break; //nothing to read
      }

      returnv = read(socket, (char*)data + iBytesRead, len - iBytesRead);
      if (returnv == -1)
      {
        *iError = errno;
        return -1;
      }

      iBytesRead += returnv;

      if (iTimeoutMs > 0)
        iNow = GetTimeMs();
    }

    return iBytesRead;
  }
  //@}

  // TCP
  //@{
  inline void TcpSocketClose(tcp_socket_t socket)
  {
    SocketClose(socket);
  }

  inline void TcpSocketShutdown(tcp_socket_t socket)
  {
    if (socket != INVALID_SOCKET_VALUE)
      shutdown(socket, SHUT_RDWR);
  }

  inline ssize_t TcpSocketWrite(tcp_socket_t socket, int *iError, void* data, size_t len)
  {
    if (socket == INVALID_SOCKET_VALUE)
    {
      *iError = EINVAL;
      return -1;
    }

    ssize_t iReturn = send(socket, data, len, 0);
    if (iReturn < (ssize_t)len)
      *iError = errno;
    return iReturn;
  }

  inline ssize_t TcpSocketRead(tcp_socket_t socket, int *iError, void* data, size_t len, uint64_t iTimeoutMs /*= 0*/)
  {
    int64_t iNow(0), iTarget(0);
    ssize_t iBytesRead(0);
    *iError = 0;

    if (socket == INVALID_SOCKET_VALUE)
    {
      *iError = EINVAL;
      return -1;
    }

    if (iTimeoutMs > 0)
    {
      iNow    = GetTimeMs();
      iTarget = iNow + (int64_t) iTimeoutMs;
    }

    struct pollfd fds;
    fds.fd = socket;
    fds.events = POLLIN;
    fds.revents = 0;

    while (iBytesRead >= 0 && iBytesRead < (ssize_t)len && (iTimeoutMs == 0 || iTarget > iNow))
    {
      if (iTimeoutMs > 0)
      {
        int iPollResult = poll(&fds, 1, iTarget - iNow);
        if (iPollResult == 0)
        {
          *iError = ETIMEDOUT;
          return -ETIMEDOUT;
        }
      }

      ssize_t iReadResult = (iTimeoutMs > 0) ?
          recv(socket, (char*)data + iBytesRead, len - iBytesRead, MSG_DONTWAIT) :
          recv(socket, data, len, MSG_WAITALL);
      if (iReadResult < 0)
      {
        if (errno == EAGAIN && iTimeoutMs > 0)
          continue;
        *iError = errno;
        return -errno;
      }
      else if (iReadResult == 0 || (iReadResult != (ssize_t)len && iTimeoutMs == 0))
      {
        *iError = ECONNRESET;
        return -ECONNRESET;
      }

      iBytesRead += iReadResult;

      if (iTimeoutMs > 0)
        iNow = GetTimeMs();
    }

    if (iBytesRead < (ssize_t)len)
      *iError = ETIMEDOUT;
    return iBytesRead;
  }

  inline bool TcpResolveAddress(const char *strHost, uint16_t iPort, int *iError, struct addrinfo **info)
  {
    struct   addrinfo hints;
    char     service[33];
    memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    sprintf(service, "%d", iPort);

    *iError = getaddrinfo(strHost, service, &hints, info);
    return !(*iError);
  }

  inline int TcpGetSocketError(tcp_socket_t socket)
  {
    int iReturn(0);
    socklen_t optLen = sizeof(socket_t);
    getsockopt(socket, SOL_SOCKET, SO_ERROR, (void *)&iReturn, &optLen);
    return iReturn;
  }

  inline bool TcpSetNoDelay(tcp_socket_t socket)
  {
    int iSetTo(1);
    setsockopt(socket, SOL_TCP, TCP_NODELAY, &iSetTo, sizeof(iSetTo));
    return true;
  }

  inline bool TcpConnectSocket(tcp_socket_t socket, struct addrinfo* addr, int *iError, uint64_t iTimeout = 0)
  {
    *iError = 0;
    int iConnectResult = connect(socket, addr->ai_addr, addr->ai_addrlen);
    if (iConnectResult == -1)
    {
      if (errno == EINPROGRESS)
      {
        struct pollfd pfd;
        pfd.fd = socket;
        pfd.events = POLLOUT;
        pfd.revents = 0;

        int iPollResult = poll(&pfd, 1, iTimeout);
        if (iPollResult == 0)
          *iError = ETIMEDOUT;
        else if (iPollResult == -1)
          *iError = errno;

        socklen_t errlen = sizeof(int);
        getsockopt(socket, SOL_SOCKET, SO_ERROR, (void *)iError, &errlen);
      }
      else
      {
        *iError = errno;
      }
    }

    return *iError == 0;
  }
  //@}
}
