/*
* XBMC Media Center
* Copyright (c) 2002 Frodo
* Portions Copyright (c) by the authors of ffmpeg and xvid
*
* This program is free software; you can redistribute it and/or modify
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
* Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "Event.h"
#include "utils/log.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CEvent::CEvent(bool manual)
{
  if(manual)
    m_hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
  else
    m_hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
}

CEvent::CEvent(const CEvent& src)
{
  if(DuplicateHandle( GetCurrentProcess()
                    , src.m_hEvent
                    , GetCurrentProcess()
                    , &m_hEvent
                    , 0
                    , TRUE
                    , DUPLICATE_SAME_ACCESS ))
  {
    CLog::Log(LOGERROR, "CEvent - failed to duplicate handle");
    m_hEvent = INVALID_HANDLE_VALUE;
  }
}

CEvent::~CEvent()
{
  CloseHandle(m_hEvent);
}

CEvent& CEvent::operator=(const CEvent& src)
{
  CloseHandle(m_hEvent);

  if(DuplicateHandle( GetCurrentProcess()
                    , src.m_hEvent
                    , GetCurrentProcess()
                    , &m_hEvent
                    , 0
                    , TRUE
                    , DUPLICATE_SAME_ACCESS ))
  {
    CLog::Log(LOGERROR, "CEvent - failed to duplicate handle");
    m_hEvent = INVALID_HANDLE_VALUE;
  }
  return *this;
}


void CEvent::Wait()
{
  if (m_hEvent)
  {
    WaitForSingleObject(m_hEvent, INFINITE);
  }
}

void CEvent::Set()
{
  if (m_hEvent) SetEvent(m_hEvent);
}

void CEvent::Reset()
{

  if (m_hEvent) ResetEvent(m_hEvent);
}

HANDLE CEvent::GetHandle()
{
  return m_hEvent;
}

bool CEvent::WaitMSec(unsigned int milliSeconds)
{

  if (m_hEvent)
  {
    DWORD dwResult = WaitForSingleObject(m_hEvent, milliSeconds);
    if (dwResult == WAIT_OBJECT_0) return true;
  }
  return false;
}

