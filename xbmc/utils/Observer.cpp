/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include "Application.h"
#include "Observer.h"
#include "threads/SingleLock.h"
#include "utils/JobManager.h"

#include <algorithm>

using namespace std;

Observer::~Observer(void)
{
  StopObserving();
}

void Observer::StopObserving(void)
{
  CSingleLock lock(m_obsCritSection);
  std::vector<Observable *> observables = m_observables;
  for (unsigned int iObsPtr = 0; iObsPtr < observables.size(); iObsPtr++)
    observables.at(iObsPtr)->UnregisterObserver(this);
}

bool Observer::IsObserving(const Observable &obs) const
{
  CSingleLock lock(m_obsCritSection);
  return find(m_observables.begin(), m_observables.end(), &obs) != m_observables.end();
}

void Observer::RegisterObservable(Observable *obs)
{
  CSingleLock lock(m_obsCritSection);
  if (!IsObserving(*obs))
    m_observables.push_back(obs);
}

void Observer::UnregisterObservable(Observable *obs)
{
  CSingleLock lock(m_obsCritSection);
  vector<Observable *>::iterator it = find(m_observables.begin(), m_observables.end(), obs);
  if (it != m_observables.end())
    m_observables.erase(it);
}

Observable::Observable() :
    m_bObservableChanged(false)
{
}

Observable::~Observable()
{
  StopObserver();
}

Observable &Observable::operator=(const Observable &observable)
{
  CSingleLock lock(m_obsCritSection);

  m_bObservableChanged = observable.m_bObservableChanged;
  m_observers.clear();
  for (unsigned int iObsPtr = 0; iObsPtr < observable.m_observers.size(); iObsPtr++)
    m_observers.push_back(observable.m_observers.at(iObsPtr));

  return *this;
}

void Observable::StopObserver(void)
{
  CSingleLock lock(m_obsCritSection);
  std::vector<Observer *> observers = m_observers;
  for (unsigned int iObsPtr = 0; iObsPtr < observers.size(); iObsPtr++)
    observers.at(iObsPtr)->UnregisterObservable(this);
}

bool Observable::IsObserving(const Observer &obs) const
{
  CSingleLock lock(m_obsCritSection);
  return find(m_observers.begin(), m_observers.end(), &obs) != m_observers.end();
}

void Observable::RegisterObserver(Observer *obs)
{
  CSingleLock lock(m_obsCritSection);
  if (!IsObserving(*obs))
  {
    m_observers.push_back(obs);
    obs->RegisterObservable(this);
  }
}

void Observable::UnregisterObserver(Observer *obs)
{
  CSingleLock lock(m_obsCritSection);
  vector<Observer *>::iterator it = find(m_observers.begin(), m_observers.end(), obs);
  if (it != m_observers.end())
  {
    obs->UnregisterObservable(this);
    m_observers.erase(it);
  }
}

void Observable::NotifyObservers(const ObservableMessage message /* = ObservableMessageNone */)
{
  bool bNotify(false);
  {
    CSingleLock lock(m_obsCritSection);
    if (m_bObservableChanged && !g_application.m_bStop)
      bNotify = true;
    m_bObservableChanged = false;
  }

  if (bNotify)
    SendMessage(*this, message);
}

void Observable::SetChanged(bool SetTo)
{
  CSingleLock lock(m_obsCritSection);
  m_bObservableChanged = SetTo;
}

void Observable::SendMessage(const Observable& obs, const ObservableMessage message)
{
  CSingleLock lock(obs.m_obsCritSection);
  for(int ptr = obs.m_observers.size() - 1; ptr >= 0; ptr--)
  {
    if (ptr < (int)obs.m_observers.size())
    {
      Observer *observer = obs.m_observers.at(ptr);
      if (observer)
      {
        lock.Leave();
        observer->Notify(obs, message);
        lock.Enter();
      }
    }
  }
}
