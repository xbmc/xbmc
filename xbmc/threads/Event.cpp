/*
 *  Copyright (c) 2002 Frodo
 *      Portions Copyright (c) by the authors of ffmpeg and xvid
 *  Copyright (C) 2002-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Event.h"

#include <algorithm>
#include <limits>
#include <memory>
#include <mutex>

using namespace std::chrono_literals;

void CEvent::addGroup(XbmcThreads::CEventGroup* group)
{
  std::unique_lock<CCriticalSection> lock(groupListMutex);
  if (!groups)
    groups = std::make_unique<std::vector<XbmcThreads::CEventGroup*>>();

  groups->push_back(group);
}

void CEvent::removeGroup(XbmcThreads::CEventGroup* group)
{
  std::unique_lock<CCriticalSection> lock(groupListMutex);
  if (groups)
  {
    groups->erase(std::remove(groups->begin(), groups->end(), group), groups->end());
    if (groups->empty())
    {
      groups.reset();
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
    std::unique_lock<CCriticalSection> slock(mutex);
    signaled = true;
  }

  actualCv.notifyAll();

  std::unique_lock<CCriticalSection> l(groupListMutex);
  if (groups)
  {
    for (auto* group : *groups)
      group->Set(this);
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
    return wait(std::chrono::milliseconds::max());
  }

  CEventGroup::CEventGroup(std::initializer_list<CEvent*> eventsList)
  : events{eventsList}
  {
    // we preping for a wait, so we need to set the group value on
    // all of the CEvents.
    for (auto* event : events)
    {
      event->addGroup(this);
    }
  }

  CEventGroup::~CEventGroup()
  {
    for (auto* event : events)
    {
      event->removeGroup(this);
    }
  }
}
