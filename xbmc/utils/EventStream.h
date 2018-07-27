/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
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
  explicit CEventSource() : m_queue(false, 1, CJob::PRIORITY_HIGH) {};

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
    m_queue.Submit(std::move(task));
  }

private:
  CJobQueue m_queue;
};

template<typename Event>
class CBlockingEventSource : public CEventStream<Event>
{
public:
  template<typename A>
  void HandleEvent(A event)
  {
    CSingleLock lock(this->m_criticalSection);
    for (const auto& subscription : this->m_subscriptions)
    {
      subscription->HandleEvent(event);
    }
  }
};
