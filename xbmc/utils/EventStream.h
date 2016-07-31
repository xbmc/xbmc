/*
 *      Copyright (C) 2016 Team Kodi
 *      http://kodi.tv
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
#pragma once

#include "EventStreamDetail.h"
#include "JobManager.h"
#include "threads/CriticalSection.h"
#include "threads/SingleLock.h"
#include <algorithm>
#include <memory>
#include <vector>


template<typename Event>
class CEventStream
{
public:

  template<typename A>
  void Subscribe(A* owner, void (A::*fn)(const Event&))
  {
    auto subscription = std::make_shared<detail::CSubscription<Event, A>>(owner, fn);
    CSingleLock lock(m_criticalSection);
    m_subscriptions.emplace_back(std::move(subscription));
  }

  template<typename A>
  void Unsubscribe(A* obj)
  {
    std::vector<std::shared_ptr<detail::ISubscription<Event>>> toCancel;
    {
      CSingleLock lock(m_criticalSection);
      auto it = m_subscriptions.begin();
      while (it != m_subscriptions.end())
      {
        if ((*it)->IsOwnedBy(obj))
        {
          toCancel.push_back(*it);
          it = m_subscriptions.erase(it);
        }
        else
        {
          ++it;
        }
      }
    }
    for (auto& subscription : toCancel)
      subscription->Cancel();
  }

protected:
  std::vector<std::shared_ptr<detail::ISubscription<Event>>> m_subscriptions;
  CCriticalSection m_criticalSection;
};


template<typename Event>
class CEventSource : public CEventStream<Event>
{
public:
  template<typename A>
  void Publish(A event)
  {
    CSingleLock lock(this->m_criticalSection);
    auto& subscriptions = this->m_subscriptions;
    auto task = [subscriptions, event](){
      for (auto& s: subscriptions)
        s->HandleEvent(event);
    };
    lock.Leave();
    CJobManager::GetInstance().Submit(std::move(task));
  }
};
