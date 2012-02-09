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

#include "socket.h"

using namespace std;

namespace PLATFORM
{
  class CTcpSocket : public CCommonSocket<tcp_socket_t>
  {
    public:
      CTcpSocket(const CStdString &strHostname, uint16_t iPort) :
        CCommonSocket<tcp_socket_t>(INVALID_SOCKET_VALUE, strHostname),
        m_iPort(iPort) {}

      virtual ~CTcpSocket(void) {}

      virtual bool Open(uint64_t iTimeoutMs = 0)
      {
        bool bReturn(false);
        struct addrinfo *address(NULL), *addr(NULL);
        if (!TcpResolveAddress(m_strName.c_str(), m_iPort, &m_iError, &address))
        {
          m_strError = strerror(m_iError);
          return bReturn;
        }

        for(addr = address; !bReturn && addr; addr = addr->ai_next)
        {
          m_socket = TcpCreateSocket(addr, &m_iError, iTimeoutMs);
          if (m_socket != INVALID_SOCKET_VALUE)
            bReturn = true;
          else
            m_strError = strerror(m_iError);
        }

        freeaddrinfo(address);
        return bReturn;
      }

      virtual void Close(void)
      {
        TcpSocketClose(m_socket);
        m_socket = INVALID_SOCKET_VALUE;
      }

      virtual void Shutdown(void)
      {
        TcpSocketShutdown(m_socket);
        m_socket = INVALID_SOCKET_VALUE;
      }

      virtual ssize_t Write(void* data, size_t len)
      {
        return TcpSocketWrite(m_socket, &m_iError, data, len);
      }

      virtual ssize_t Read(void* data, size_t len, uint64_t iTimeoutMs = 0)
      {
        return TcpSocketRead(m_socket, &m_iError, data, len, iTimeoutMs);
      }

      virtual bool IsOpen(void)
      {
        return m_socket != INVALID_SOCKET_VALUE;
      }

    protected:
      virtual tcp_socket_t TcpCreateSocket(struct addrinfo* addr, int* iError, uint64_t iTimeout)
      {
        tcp_socket_t fdSock = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
        if (fdSock == INVALID_SOCKET_VALUE)
        {
          *iError = errno;
          return (tcp_socket_t)INVALID_SOCKET_VALUE;
        }

        if (!TcpConnectSocket(fdSock, addr, iError, iTimeout))
        {
          TcpSocketClose(fdSock);
          return (tcp_socket_t)INVALID_SOCKET_VALUE;
        }

        TcpSetNoDelay(fdSock);

        return fdSock;
      }

      uint16_t   m_iPort;
  };

  class CTcpConnection : public CProtectedSocket<CTcpSocket>
  {
  public:
    CTcpConnection(const CStdString &strHostname, uint16_t iPort) :
      CProtectedSocket<CTcpSocket> (new CTcpSocket(strHostname, iPort)) {}
    virtual ~CTcpConnection(void) {}
  };
};
