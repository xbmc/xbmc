/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */


#include "Observer.h"
#include "threads/SingleLock.h"

#include <algorithm>

Observable &Observable::operator=(const Observable &observable)
{
  CSingleLock lock(m_obsCritSection);

  m_bObservableChanged = static_cast<bool>(observable.m_bObservableChanged);
  m_observers = observable.m_observers;

  return *this;
}

bool Observable::IsObserving(const Observer &obs) const
{
  CSingleLock lock(m_obsCritSection);
  return std::find(m_observers.begin(), m_observers.end(), &obs) != m_observers.end();
}

void Observable::RegisterObserver(Observer *obs)
{
  CSingleLock lock(m_obsCritSection);
  if (!IsObserving(*obs))
  {
    m_observers.push_back(obs);
  }
}

void Observable::UnregisterObserver(Observer *obs)
{
  CSingleLock lock(m_obsCritSection);
  auto iter = std::remove(m_observers.begin(), m_observers.end(), obs);
  if (iter != m_observers.end())
    m_observers.erase(iter);
}

void Observable::NotifyObservers(const ObservableMessage message /* = ObservableMessageNone */)
{
  // Make sure the set/compare is atomic
  // so we don't clobber the variable in a race condition
  auto bNotify = m_bObservableChanged.exchange(false);

  if (bNotify)
    SendMessage(message);
}

void Observable::SetChanged(bool SetTo)
{
  m_bObservableChanged = SetTo;
}

void Observable::SendMessage(const ObservableMessage message)
{
  CSingleLock lock(m_obsCritSection);

  for (auto& observer : m_observers)
  {
    observer->Notify(*this, message);
  }
}
