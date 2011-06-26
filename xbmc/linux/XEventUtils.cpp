/*
 *      Copyright (C) 2005-2008 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "system.h"
#include "PlatformDefs.h"
#include "XEventUtils.h"
#include "XHandle.h"
#include "utils/log.h"

using namespace std;

HANDLE WINAPI CreateEvent(void *pDummySec, bool bManualReset, bool bInitialState, char *szDummyName)
{
  CXHandle *pHandle = new CXHandle(CXHandle::HND_EVENT);
  pHandle->m_bManualEvent = bManualReset;
  pHandle->m_hCond = new XbmcThreads::ConditionVariable();
  pHandle->m_hMutex = new CCriticalSection();
  pHandle->m_bEventSet = false;

  if (bInitialState)
    SetEvent(pHandle);

  return pHandle;
}

//
// The state of a manual-reset event object remains signaled until it is set explicitly to the nonsignaled
// state by the ResetEvent function. Any number of waiting threads, or threads that subsequently begin wait
// operations for the specified event object by calling one of the wait functions, can be released while the
// object's state is signaled.
//
// The state of an auto-reset event object remains signaled until a single waiting thread is released, at
// which time the system automatically sets the state to nonsignaled. If no threads are waiting, the event
// object's state remains signaled.
//
bool WINAPI SetEvent(HANDLE hEvent)
{
  if (hEvent == NULL || hEvent->m_hCond == NULL || hEvent->m_hMutex == NULL)
    return false;

  CSingleLock lock(*(hEvent->m_hMutex));
  hEvent->m_bEventSet = true;

  // we must guarantee that these handle's won't be deleted, until we are done
  list<CXHandle*> events = hEvent->m_hParents;
  for(list<CXHandle*>::iterator it = events.begin();it != events.end();it++)
    DuplicateHandle(GetCurrentProcess(), *it, GetCurrentProcess(), NULL, 0, FALSE, DUPLICATE_SAME_ACCESS);

  lock.Leave();

  for(list<CXHandle*>::iterator it = events.begin();it != events.end();it++)
  {
    SetEvent(*it);
    CloseHandle(*it);
  }

  DuplicateHandle(GetCurrentProcess(), hEvent, GetCurrentProcess(), NULL, 0, FALSE, DUPLICATE_SAME_ACCESS);

  if (hEvent->m_bManualEvent == true)
    hEvent->m_hCond->notifyAll();
  else
    hEvent->m_hCond->notify();

  CloseHandle(hEvent);

  return true;
}

bool WINAPI ResetEvent(HANDLE hEvent)
{
  if (hEvent == NULL || hEvent->m_hCond == NULL || hEvent->m_hMutex == NULL)
    return false;

  CSingleLock lock(*(hEvent->m_hMutex));
  hEvent->m_bEventSet = false;

  return true;
}

bool WINAPI PulseEvent(HANDLE hEvent)
{
  if (hEvent == NULL || hEvent->m_hCond == NULL || hEvent->m_hMutex == NULL)
    return false;

  CSingleLock lock(*(hEvent->m_hMutex));
  // we must guarantee that these handle's won't be deleted, until we are done
  list<CXHandle*> events = hEvent->m_hParents;
  for(list<CXHandle*>::iterator it = events.begin();it != events.end();it++)
    DuplicateHandle(GetCurrentProcess(), *it, GetCurrentProcess(), NULL, 0, FALSE, DUPLICATE_SAME_ACCESS);

  if(events.size())
  {
    CLog::Log(LOGWARNING,"PulseEvent - ineffecient multiwait detected");
    hEvent->m_bEventSet = true;
  }

  lock.Leave();

  for(list<CXHandle*>::iterator it = events.begin();it != events.end();it++)
  {
    SetEvent(*it);
    CloseHandle(*it);

    if (hEvent->m_bManualEvent == false)
      break;
  }

  // for multiwaits, we must yield some time to get the multiwaits to notice it was signaled
  if(events.size())
    Sleep(10);

  // we should always unset the event on pulse
  {
    CSingleLock lock2(*(hEvent->m_hMutex));
    hEvent->m_bEventSet = false;
  }

  if (hEvent->m_bManualEvent == true)
    hEvent->m_hCond->notifyAll();
  else
    hEvent->m_hCond->notify();

  return true;
}

