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

#include <stdarg.h>

#include "Event.h"

void CEvent::groupSet()
{
  // no locking, the lock should already be held
  if (groups)
  {
    for (std::vector<XbmcThreads::CEventGroup*>::iterator iter = groups->begin(); 
         iter != groups->end(); iter++)
      (*iter)->Set(this);
  }
}

void CEvent::addGroup(XbmcThreads::CEventGroup* group)
{
  CSingleLock lock(groupListMutex);
  if (groups == NULL)
    groups = new std::vector<XbmcThreads::CEventGroup*>();

  groups->push_back(group);
}

void CEvent::removeGroup(XbmcThreads::CEventGroup* group)
{
  CSingleLock lock(groupListMutex);
  if (groups)
  {
    for (std::vector<XbmcThreads::CEventGroup*>::iterator iter = groups->begin(); iter != groups->end(); iter++)
    {
      if ((*iter) == group)
      {
        groups->erase(iter);
        break;
      }
    }

    if (groups->size() <= 0)
    {
      delete groups;
      groups = NULL;
    }
  }
}

void CEvent::lockGroups()
{
  CSingleLock lock(groupListMutex);
  if (groups)
  {
    for (std::vector<XbmcThreads::CEventGroup*>::iterator iter = groups->begin(); iter != groups->end(); iter++)
      (*iter)->mutex.lock();
  }
}

void CEvent::unlockGroups()
{
  CSingleLock lock(groupListMutex);
  if (groups)
  {
    for (std::vector<XbmcThreads::CEventGroup*>::iterator iter = groups->begin(); iter != groups->end(); iter++)
      (*iter)->mutex.unlock();
  }
}

#define XB_MAX_UNSIGNED_INT ((unsigned int)-1)

namespace XbmcThreads
{
  /**
   * This will block until any one of the CEvents in the group are
   * signaled at which point a pointer to that CEvents will be 
   * returned.
   */
  CEvent* CEventGroup::wait() 
  { 
    return wait(XB_MAX_UNSIGNED_INT);
  }

  /**
   * This will block until any one of the CEvents in the group are
   * signaled or the timeout is reachec. If an event is signaled then
   * it will return a pointer to that CEvent, otherwise it will return
   * NULL.
   */
  CEvent* CEventGroup::wait(unsigned int milliseconds)  
  { 
    CSingleLock lock(mutex);
    numWaits++; 
    signaled = anyEventsSignaled(); 
    if(!signaled)
    {
      if (milliseconds == XB_MAX_UNSIGNED_INT)
        condVar.wait(mutex); 
      else
        condVar.wait(mutex,milliseconds); 
    }
    numWaits--; 

    // signaled should have been set by a call to CEventGroup::Set
    CEvent* ret = signaled;
    if (numWaits == 0) 
    {
      if (signaled)
        signaled->WaitMSec(0); // reset the event if needed
      signaled = NULL;  // clear the signaled if all the waiters are gone
    }

    // there is a very small chance that Set was called from a second
    // child event that will need resetting
    for (std::vector<CEvent*>::iterator iter = events.begin();
         iter != events.end(); iter++)
    {
      CEvent* cur = *iter;
      if (cur != signaled)
        cur->WaitMSec(0); // reset child if necessary
    }

    return ret;
  }

  CEventGroup::CEventGroup(int num, CEvent* v1, ...) : signaled(NULL), condVar(signaled), numWaits(0)
  {
    va_list ap;

    va_start(ap, v1);
    events.push_back(v1);
    num--; // account for v1
    for (;num > 0; num--)
      events.push_back(va_arg(ap,CEvent*));
    va_end(ap);

    // we preping for a wait, so we need to set the group value on
    // all of the CEvents. 
    for (std::vector<CEvent*>::iterator iter = events.begin();
         iter != events.end(); iter++)
      (*iter)->addGroup(this);
  }

  CEventGroup::CEventGroup(CEvent* v1, ...) : signaled(NULL), condVar(signaled), numWaits(0)
  {
    va_list ap;

    va_start(ap, v1);
    events.push_back(v1);
    bool done = false;
    while(!done)
    {
      CEvent* cur = va_arg(ap,CEvent*);
      if (cur)
        events.push_back(cur);
      else
        done = true;
    }
    va_end(ap);

    // we preping for a wait, so we need to set the group value on
    // all of the CEvents. 
    for (std::vector<CEvent*>::iterator iter = events.begin();
         iter != events.end(); iter++)
      (*iter)->addGroup(this);
  }

  CEventGroup::~CEventGroup()
  {
    // we preping for a wait, so we need to set the group value on
    // all of the CEvents. 
    for (std::vector<CEvent*>::iterator iter = events.begin();
         iter != events.end(); iter++)
      (*iter)->removeGroup(this);
  }

  CEvent* CEventGroup::anyEventsSignaled()
  {
    for (std::vector<CEvent*>::iterator iter = events.begin();
         iter != events.end(); iter++)
    {
      CEvent* cur = *iter;
      if (cur->signaled) return cur;
    }
    return NULL;
  }

}
