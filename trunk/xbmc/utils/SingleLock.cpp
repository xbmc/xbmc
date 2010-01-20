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

#include "SingleLock.h"


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CSingleLock::CSingleLock(CCriticalSection& cs)
    : m_cs( cs )
    , m_bIsOwner( false )
{
  Enter();
}

CSingleLock::CSingleLock(const CCriticalSection& cs)
    : m_cs( const_cast<CCriticalSection&>(cs) )
    , m_bIsOwner( false )
{
  Enter();
}

CSingleLock::~CSingleLock()
{
  Leave();
}

bool CSingleLock::IsOwner() const
{
  return m_bIsOwner;
}

bool CSingleLock::Enter()
{
  // Test if we already own the critical section
  if ( true == m_bIsOwner )
  {
    return true;
  }

  // Blocking call
  ::EnterCriticalSection( m_cs );
  m_bIsOwner = true;

  return m_bIsOwner;
}

void CSingleLock::Leave()
{
  if ( false == m_bIsOwner )
  {
    return ;
  }

  ::LeaveCriticalSection( m_cs );
  m_bIsOwner = false;
}

CSingleExit::CSingleExit(CCriticalSection& cs)
    : m_cs( cs )
    , m_count(0)
{
  Exit();
}

 CSingleExit::CSingleExit(const CCriticalSection& cs)
    : m_cs(const_cast<CCriticalSection&>(cs))
    , m_count(0)
{
  Exit();
}

CSingleExit::~CSingleExit()
{
  Restore();
}

void CSingleExit::Exit()
{
  if(m_count == 0)
    m_count = ::ExitCriticalSection(m_cs);
}

void CSingleExit::Restore()
{
  if(m_count)
  {
    RestoreCriticalSection(m_cs, m_count);
    m_count = 0;
  }
}

 
