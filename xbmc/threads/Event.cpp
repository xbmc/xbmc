/*
 *      Copyright (c) 2002 Frodo
 *      Portions Copyright (c) by the authors of ffmpeg and xvid
 *      Copyright (C) 2002-2013 Team XBMC
 *      http://xbmc.org
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

#include <stdarg.h>
#include <limits>

#include "Event.h"

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
    for (std::vector<XbmcThreads::CEventGroup*>::iterator iter = groups->begin(); iter != groups->end(); ++iter)
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

// locking is ALWAYS done in this order:
//  CEvent::groupListMutex -> CEventGroup::mutex -> CEvent::mutex
void CEvent::Set()
{
  // Originally I had this without locking. Thanks to FernetMenta who
  // pointed out that this creates a race condition between setting
  // checking the signal and calling wait() on the Wait call in the
  // CEvent class. This now perfectly matches the boost example here:
  // http://www.boost.org/doc/libs/1_41_0/doc/html/thread/synchronization.html#thread.synchronization.condvar_ref
  {
    CSingleLock slock(mutex);
    signaled = true; 
  }

  condVar.notifyAll();

  CSingleLock l(groupListMutex);
  if (groups)
  {
    for (std::vector<XbmcThreads::CEventGroup*>::iterator iter = groups->begin(); 
         iter != groups->end(); ++iter)
      (*iter)->Set(this);
  }
}

namespace XbmcThreads
{
  /**
   * This will block until any one of the CEvents in the group are
   * signaled at which point a pointer to that CEvents will be 
   * returned.
   */
  CEvent* CEventGroup::wait() 
  { 
    return wait(std::numeric_limits<unsigned int>::max());
  }

  /**
   * This will block until any one of the CEvents in the group are
   * signaled or the timeout is reachec. If an event is signaled then
   * it will return a pointer to that CEvent, otherwise it will return
   * NULL.
   */
  // locking is ALWAYS done in this order:
  //  CEvent::groupListMutex -> CEventGroup::mutex -> CEvent::mutex
  //
  // Notice that this method doesn't grab the CEvent::groupListMutex at all. This
  // is fine. It just grabs the CEventGroup::mutex and THEN the individual 
  // CEvent::mutex's
  CEvent* CEventGroup::wait(unsigned int milliseconds)  
  { 
    CSingleLock lock(mutex); // grab CEventGroup::mutex
    numWaits++; 

    // ==================================================
    // This block checks to see if any child events are 
    // signaled and sets 'signaled' to the first one it
    // finds.
    // ==================================================
    signaled = NULL;
    for (std::vector<CEvent*>::iterator iter = events.begin();
         signaled == NULL && iter != events.end(); ++iter)
    {
      CEvent* cur = *iter;
      CSingleLock lock2(cur->mutex);
      if (cur->signaled) 
        signaled = cur;
    }
    // ==================================================

    if(!signaled)
    {
      // both of these release the CEventGroup::mutex
      if (milliseconds == std::numeric_limits<unsigned int>::max())
        condVar.wait(mutex); 
      else
        condVar.wait(mutex,milliseconds); 
    } // at this point the CEventGroup::mutex is reacquired
    numWaits--; 

    // signaled should have been set by a call to CEventGroup::Set
    CEvent* ret = signaled;
    if (numWaits == 0) 
    {
      if (signaled)
        // This acquires and releases the CEvent::mutex. This is fine since the
        //  CEventGroup::mutex is already being held
        signaled->WaitMSec(0); // reset the event if needed
      signaled = NULL;  // clear the signaled if all the waiters are gone
    }
    return ret;
  }

  CEventGroup::CEventGroup(int num, CEvent* v1, ...) : signaled(NULL), condVar(actualCv,signaled), numWaits(0)
  {
    va_list ap;

    va_start(ap, v1);
    if (v1)
      events.push_back(v1);
    num--; // account for v1
    for (; num > 0; num--)
    {
      CEvent* const cur = va_arg(ap, CEvent*);
      if (cur)
        events.push_back(cur);
    }
    va_end(ap);

    // we preping for a wait, so we need to set the group value on
    // all of the CEvents. 
    for (std::vector<CEvent*>::iterator iter = events.begin();
         iter != events.end(); ++iter)
      (*iter)->addGroup(this);
  }

  CEventGroup::CEventGroup(CEvent* v1, ...) : signaled(NULL), condVar(actualCv,signaled), numWaits(0)
  {
    va_list ap;

    va_start(ap, v1);
    if (v1)
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
         iter != events.end(); ++iter)
      (*iter)->addGroup(this);
  }

  CEventGroup::~CEventGroup()
  {
    for (std::vector<CEvent*>::iterator iter = events.begin();
         iter != events.end(); ++iter)
      (*iter)->removeGroup(this);
  }
}
