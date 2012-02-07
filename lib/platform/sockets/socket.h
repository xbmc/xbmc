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

#include "../threads/mutex.h"
#include "../util/StdString.h"

#if defined(__WINDOWS__)
#include "../windows/os-socket.h"
#else
#include "../posix/os-socket.h"
#endif

// Common socket operations

namespace PLATFORM
{
  class ISocket : public PreventCopy
  {
  public:
    ISocket(void) {};
    virtual ~ISocket(void) {}

    virtual bool Open(uint64_t iTimeoutMs = 0) = 0;
    virtual void Close(void) = 0;
    virtual void Shutdown(void) = 0;
    virtual bool IsOpen(void) = 0;
    virtual ssize_t Write(void* data, size_t len) = 0;
    virtual ssize_t Read(void* data, size_t len, uint64_t iTimeoutMs = 0) = 0;
    virtual CStdString GetError(void) = 0;
    virtual int GetErrorNumber(void) = 0;
    virtual CStdString GetName(void) = 0;
  };

  template <typename _SType>
  class CCommonSocket : public ISocket
  {
  public:
    CCommonSocket(_SType initialSocketValue, const CStdString &strName) :
      m_socket(initialSocketValue),
      m_strName(strName),
      m_iError(0) {}

    virtual ~CCommonSocket(void) {}

    virtual CStdString GetError(void)
    {
      CStdString strError;
      strError = m_strError.IsEmpty() && m_iError != 0 ? strerror(m_iError) : m_strError;
      return strError;
    }

    virtual int GetErrorNumber(void)
    {
      return m_iError;
    }

    virtual CStdString GetName(void)
    {
      CStdString strName;
      strName = m_strName;
      return strName;
    }

  protected:
    _SType     m_socket;
    CStdString m_strError;
    CStdString m_strName;
    int        m_iError;
    CMutex     m_mutex;
  };

  template <typename _Socket>
  class CProtectedSocket : public ISocket
  {
  public:
    CProtectedSocket(_Socket *socket) :
      m_socket(socket),
      m_iUseCount(0) {}

    virtual ~CProtectedSocket(void)
    {
      Close();
      delete m_socket;
    }

    virtual bool Open(uint64_t iTimeoutMs = 0)
    {
      bool bReturn(false);
      if (m_socket && WaitReady())
      {
        bReturn = m_socket->Open(iTimeoutMs);
        MarkReady();
      }
      return bReturn;
    }

    virtual void Close(void)
    {
      if (m_socket && WaitReady())
      {
        m_socket->Close();
        MarkReady();
      }
    }

    virtual void Shutdown(void)
    {
      if (m_socket && WaitReady())
      {
        m_socket->Shutdown();
        MarkReady();
      }
    }

    virtual bool IsOpen(void)
    {
      CLockObject lock(m_mutex);
      return m_socket && m_socket->IsOpen();
    }

    virtual bool IsBusy(void)
    {
      CLockObject lock(m_mutex);
      return m_socket && m_iUseCount > 0;
    }

    virtual int GetUseCount(void)
    {
      CLockObject lock(m_mutex);
      return m_iUseCount;
    }

    virtual ssize_t Write(void* data, size_t len)
    {
      if (!m_socket || !WaitReady())
        return EINVAL;

      ssize_t iReturn = m_socket->Write(data, len);
      MarkReady();

      return iReturn;
    }

    virtual ssize_t Read(void* data, size_t len, uint64_t iTimeoutMs = 0)
    {
      if (!m_socket || !WaitReady())
        return EINVAL;

      ssize_t iReturn = m_socket->Read(data, len, iTimeoutMs);
      MarkReady();

      return iReturn;
    }

    virtual CStdString GetError(void)
    {
      CStdString strError;
      CLockObject lock(m_mutex);
      strError = m_socket ? m_socket->GetError() : "";
      return strError;
    }

    virtual int GetErrorNumber(void)
    {
      CLockObject lock(m_mutex);
      return m_socket ? m_socket->GetErrorNumber() : EINVAL;
    }

    virtual CStdString GetName(void)
    {
      CStdString strName;
      CLockObject lock(m_mutex);
      strName = m_socket ? m_socket->GetName() : "";
      return strName;
    }

  private:
    bool WaitReady(void)
    {
      CLockObject lock(m_mutex);
      if (m_iUseCount > 0)
        m_condition.Wait(m_mutex);

      if (m_iUseCount > 0)
        return false;

      ++m_iUseCount;
      return true;
    }

    void MarkReady(void)
    {
      CLockObject lock(m_mutex);
      if (m_iUseCount > 0)
        --m_iUseCount;
      m_condition.Signal();
    }

    _Socket   *m_socket;
    CMutex     m_mutex;
    CCondition m_condition;
    int        m_iUseCount;
  };
};
