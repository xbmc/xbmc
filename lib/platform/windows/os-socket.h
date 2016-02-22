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

#include <ws2spi.h>
#include <ws2ipdef.h>
#include <ws2tcpip.h>

#define SHUT_RDWR SD_BOTH

#ifndef ETIMEDOUT
#define ETIMEDOUT 138
#endif

namespace PLATFORM
{
  #ifndef MSG_WAITALL
  #define MSG_WAITALL 0x8
  #endif

  inline int GetSocketError(void)
  {
    int error = WSAGetLastError();
    switch(error)
    {
      case WSAEINPROGRESS: return EINPROGRESS;
      case WSAECONNRESET : return ECONNRESET;
      case WSAETIMEDOUT  : return ETIMEDOUT;
      case WSAEWOULDBLOCK: return EAGAIN;
      default            : return error;
    }
  }

  // Serial port
  //@{
  inline void SerialSocketClose(serial_socket_t socket)
  {
    if (socket != INVALID_HANDLE_VALUE)
      CloseHandle(socket);
  }

  inline ssize_t SerialSocketWrite(serial_socket_t socket, int *iError, void* data, size_t len)
  {
    if (len != (DWORD)len)
    {
      *iError = EINVAL;
      return -1;
    }

    DWORD iBytesWritten(0);
    if (socket != INVALID_HANDLE_VALUE)
    {
      if (!WriteFile(socket, data, (DWORD)len, &iBytesWritten, NULL))
      {
        *iError = GetLastError();
        return -1;
      }
      return (ssize_t)iBytesWritten;
    }

    return -1;
  }

  inline ssize_t SerialSocketRead(serial_socket_t socket, int *iError, void* data, size_t len, uint64_t iTimeoutMs /*= 0*/)
  {
    if (len != (DWORD)len)
    {
      *iError = EINVAL;
      return -1;
    }

    DWORD iBytesRead(0);
    if (socket != INVALID_HANDLE_VALUE)
    {
      if(!ReadFile(socket, data, (DWORD)len, &iBytesRead, NULL) != 0)
      {
        *iError = GetLastError();
        return -1;
      }
      return (ssize_t)iBytesRead;
    }
    return -1;
  }
  //@}

  // TCP
  //@{
  inline void TcpSocketSetBlocking(tcp_socket_t socket, bool bSetTo)
  {
    u_long iSetTo = bSetTo ? 0 : 1;
    ioctlsocket(socket, FIONBIO, &iSetTo);
  }

  inline void TcpSocketClose(tcp_socket_t socket)
  {
    closesocket(socket);
  }

  inline void TcpSocketShutdown(tcp_socket_t socket)
  {
    if (socket != INVALID_SOCKET &&
        socket != SOCKET_ERROR)
      shutdown(socket, SHUT_RDWR);
  }

  inline ssize_t TcpSocketWrite(tcp_socket_t socket, int *iError, void* data, size_t len)
  {
    if (socket == INVALID_SOCKET ||
        socket == SOCKET_ERROR ||
        len != (int)len)
    {
      *iError = EINVAL;
      return -1;
    }

    ssize_t iReturn = send(socket, (char*)data, (int)len, 0);
    if (iReturn < (ssize_t)len)
      *iError = GetSocketError();
    return iReturn;
  }

  inline ssize_t TcpSocketRead(tcp_socket_t socket, int *iError, void* data, size_t len, uint64_t iTimeoutMs /*= 0*/)
  {
    int64_t iNow(0), iTarget(0);
    ssize_t iBytesRead(0);
    *iError = 0;

    if (socket == INVALID_SOCKET ||
        socket == SOCKET_ERROR ||
        len != (int)len)
    {
      *iError = EINVAL;
      return -1;
    }

    if (iTimeoutMs > 0)
    {
      iNow    = GetTimeMs();
      iTarget = iNow + (int64_t) iTimeoutMs;
    }

    fd_set fd_read;
    struct timeval tv;
    while (iBytesRead >= 0 && iBytesRead < (ssize_t)len && (iTimeoutMs == 0 || iTarget > iNow))
    {
      if (iTimeoutMs > 0)
      {
        tv.tv_sec  =        (long)(iTimeoutMs / 1000);
        tv.tv_usec = 1000 * (long)(iTimeoutMs % 1000);

        FD_ZERO(&fd_read);
        FD_SET(socket, &fd_read);

        if (select((int)socket + 1, &fd_read, NULL, NULL, &tv) == 0)
        {
          *iError = ETIMEDOUT;
          return ETIMEDOUT;
        }
        TcpSocketSetBlocking(socket, false);
      }

      ssize_t iReadResult = (iTimeoutMs > 0) ?
          recv(socket, (char*)data + iBytesRead, (int)(len - iBytesRead), 0) :
          recv(socket, (char*)data, (int)len, MSG_WAITALL);
      *iError = GetSocketError();

      if (iTimeoutMs > 0)
      {
        TcpSocketSetBlocking(socket, true);
        iNow = GetTimeMs();
      }

      if (iReadResult < 0)
      {
        if (*iError == EAGAIN && iTimeoutMs > 0)
          continue;
        return -1;
      }
      else if (iReadResult == 0 || (iReadResult != (ssize_t)len && iTimeoutMs == 0))
      {
        *iError = ECONNRESET;
        return -1;
      }

      iBytesRead += iReadResult;
    }

    if (iBytesRead < (ssize_t)len && *iError == 0)
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
    socklen_t optLen = sizeof(tcp_socket_t);
    getsockopt(socket, SOL_SOCKET, SO_ERROR, (char *)&iReturn, &optLen);
    return iReturn;
  }

  inline bool TcpSetNoDelay(tcp_socket_t socket)
  {
    int iSetTo(1);
    setsockopt(socket, IPPROTO_TCP, TCP_NODELAY, (const char*)&iSetTo, sizeof(iSetTo));
    return true;
  }

  inline bool TcpConnectSocket(tcp_socket_t socket, struct addrinfo* addr, int *iError, uint64_t iTimeout = 0)
  {
    TcpSocketSetBlocking(socket, false);

    *iError = 0;
    int iConnectResult = connect(socket, addr->ai_addr, (int)addr->ai_addrlen);
    if (iConnectResult == -1)
    {
      if (GetSocketError() == EINPROGRESS ||
          GetSocketError() == EAGAIN)
      {
        fd_set fd_write, fd_except;
        struct timeval tv;
        tv.tv_sec  =        (long)(iTimeout / 1000);
        tv.tv_usec = 1000 * (long)(iTimeout % 1000);

        FD_ZERO(&fd_write);
        FD_ZERO(&fd_except);
        FD_SET(socket, &fd_write);
        FD_SET(socket, &fd_except);

        int iPollResult = select(sizeof(socket)*8, NULL, &fd_write, &fd_except, &tv);
        if (iPollResult == 0)
          *iError = ETIMEDOUT;
        else if (iPollResult == -1)
          *iError = GetSocketError();
        else
        {
          socklen_t errlen = sizeof(int);
          getsockopt(socket, SOL_SOCKET, SO_ERROR, (char *)iError, &errlen);
        }
      }
      else
      {
        *iError = GetSocketError();
      }
    }

    TcpSocketSetBlocking(socket, true);

    return *iError == 0;
  }
}
