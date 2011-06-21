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
  if (groups)
  {
    for (std::vector<XbmcThreads::CEventGroup*>::iterator iter = groups->begin(); 
         iter != groups->end(); iter++)
      (*iter)->Set(this);
  }
}

void CEvent::addGroup(XbmcThreads::CEventGroup* group)
{
  CSingleLock lock(mutex);
  if (groups == NULL)
    groups = new std::vector<XbmcThreads::CEventGroup*>();

  groups->push_back(group);
}

void CEvent::removeGroup(XbmcThreads::CEventGroup* group)
{
  CSingleLock lock(mutex);
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

namespace XbmcThreads
{
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

}
