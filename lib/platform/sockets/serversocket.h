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
  class IServerSocket : public ISocket
  {
  public:
    IServerSocket() : ISocket() {}

    virtual ~IServerSocket(void) {}

    virtual bool Open(uint64_t iTimeoutMs = 0) = 0;
    virtual void Close(void) = 0;
    virtual void Shutdown(void) = 0;
    virtual bool IsOpen(void) = 0;
    ssize_t      Write(void* data, size_t len) { (void) data; (void) len; return EINVAL; }
    ssize_t      Read(void* data, size_t len, uint64_t iTimeoutMs = 0) { (void) data; (void) len; (void) iTimeoutMs; return EINVAL; }
    virtual CStdString GetError(void) = 0;
    virtual int GetErrorNumber(void) = 0;
    virtual CStdString GetName(void) = 0;

    virtual ISocket* Accept(void) = 0;
  };

  class CTcpServerSocket : public IServerSocket
  {
  public:
    CTcpServerSocket(uint16_t iPort) :
      IServerSocket(),
      m_iPort(iPort),
      m_socket(INVALID_SOCKET_VALUE),
      m_iError(0) {}

    virtual ~CTcpServerSocket(void) {}

    virtual bool Open(uint64_t iTimeoutMs = 0);
    virtual void Close(void);
    virtual void Shutdown(void);
    virtual bool IsOpen(void);
    virtual CStdString GetError(void);
    virtual int GetErrorNumber(void);
    virtual CStdString GetName(void);

    virtual ISocket* Accept(void);

  protected:
    virtual tcp_socket_t TcpCreateSocket(struct addrinfo* addr, int* iError);

  protected:
    uint16_t     m_iPort;
    tcp_socket_t m_socket;
    CStdString   m_strError;
    int          m_iError;
  };
}
