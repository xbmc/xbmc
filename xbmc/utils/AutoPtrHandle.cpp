/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "AutoPtrHandle.h"

using namespace AUTOPTR;

CAutoPtrHandle::CAutoPtrHandle(HANDLE hHandle)
    : m_hHandle(hHandle)
{}

CAutoPtrHandle::~CAutoPtrHandle(void)
{
  Cleanup();
}

CAutoPtrHandle::operator HANDLE()
{
  return m_hHandle;
}

void CAutoPtrHandle::attach(HANDLE hHandle)
{
  Cleanup();
  m_hHandle = hHandle;
}

HANDLE CAutoPtrHandle::release()
{
  HANDLE hTmp = m_hHandle;
  m_hHandle = INVALID_HANDLE_VALUE;
  return hTmp;
}

void CAutoPtrHandle::Cleanup()
{
  if ( isValid() )
  {
    CloseHandle(m_hHandle);
    m_hHandle = INVALID_HANDLE_VALUE;
  }
}

bool CAutoPtrHandle::isValid() const
{
  if ( INVALID_HANDLE_VALUE != m_hHandle)
    return true;
  return false;
}
void CAutoPtrHandle::reset()
{
  Cleanup();
}

//-------------------------------------------------------------------------------
CAutoPtrFind ::CAutoPtrFind(HANDLE hHandle)
    : CAutoPtrHandle(hHandle)
{}
CAutoPtrFind::~CAutoPtrFind(void)
{
  Cleanup();
}

void CAutoPtrFind::Cleanup()
{
  if ( isValid() )
  {
    FindClose(m_hHandle);
    m_hHandle = INVALID_HANDLE_VALUE;
  }
}

//-------------------------------------------------------------------------------
CAutoPtrSocket::CAutoPtrSocket(SOCKET hSocket)
    : m_hSocket(hSocket)
{}

CAutoPtrSocket::~CAutoPtrSocket(void)
{
  Cleanup();
}

CAutoPtrSocket::operator SOCKET()
{
  return m_hSocket;
}

void CAutoPtrSocket::attach(SOCKET hSocket)
{
  Cleanup();
  m_hSocket = hSocket;
}

SOCKET CAutoPtrSocket::release()
{
  SOCKET hTmp = m_hSocket;
  m_hSocket = INVALID_SOCKET;
  return hTmp;
}

void CAutoPtrSocket::Cleanup()
{
  if ( isValid() )
  {
    closesocket(m_hSocket);
    m_hSocket = INVALID_SOCKET;
  }
}

bool CAutoPtrSocket::isValid() const
{
  if ( INVALID_SOCKET != m_hSocket)
    return true;
  return false;
}
void CAutoPtrSocket::reset()
{
  Cleanup();
}
