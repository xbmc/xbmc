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

#include "threads/CriticalSection.h"
#include "threads/SingleLock.h"

namespace detail
{

template<typename Event>
class ISubscription
{
public:
  virtual void HandleEvent(const Event& event) = 0;
  virtual void Cancel() = 0;
  virtual bool IsOwnedBy(void* obj) = 0;
  virtual ~ISubscription() {}
};

template<typename Event, typename Owner>
class CSubscription : public ISubscription<Event>
{
public:
  typedef void (Owner::*Fn)(const Event&);
  CSubscription(Owner* owner, Fn fn);
  void HandleEvent(const Event& event) override;
  void Cancel() override;
  bool IsOwnedBy(void *obj) override;

private:
  Owner* m_owner;
  Fn m_eventHandler;
  CCriticalSection m_criticalSection;
};

template<typename Event, typename Owner>
CSubscription<Event, Owner>::CSubscription(Owner* owner, Fn fn)
    : m_owner(owner), m_eventHandler(fn)
{}

template<typename Event, typename Owner>
bool CSubscription<Event, Owner>::IsOwnedBy(void* obj)
{
  CSingleLock lock(m_criticalSection);
  return obj != nullptr && obj == m_owner;
}

template<typename Event, typename Owner>
void CSubscription<Event, Owner>::Cancel()
{
  CSingleLock lock(m_criticalSection);
  m_owner = nullptr;
}

template<typename Event, typename Owner>
void CSubscription<Event, Owner>::HandleEvent(const Event& event)
{
  CSingleLock lock(m_criticalSection);
  if (m_owner)
    (m_owner->*m_eventHandler)(event);
}
}
