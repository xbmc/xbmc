#pragma once

/*
*      Copyright (C) 2015 Team XBMC
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

#include <vector>
#include <algorithm>

#include "threads/CriticalSection.h"
#include "threads/SingleLock.h"

template<typename IObserver>
class IObservable
{
public:
  virtual ~IObservable() {}

  /*! \brief Add an Observer to be notified of events
  *
  * \param[in] observer A class inheriting from an implementation  of IObservableEvent
  */
  void AddObserver(IObserver* observer)
  {
    CSingleLock lock(m_ObserverCS);
    m_Observers.push_back(observer);
  }

  /*! \brief Remove an Observer that no longer want to be notified of events
  *
  * \param[in] observer A class inheriting from an implementation  of IObservableEvent
  */
  void RemoveObserver(IObserver* observer)
  {
    CSingleLock lock(m_ObserverCS);

    auto obs = std::find(m_Observers.begin(), m_Observers.end(), observer);
    if (obs != m_Observers.end())
      m_Observers.erase(obs);
  }

  /*! \brief Add an Observer to be notified of events and insert it first in the list
  *   of observers.
  *
  * \param[in] observer A class inheriting from an implementation  of IObservableEvent
  */
  void AddObserverFirst(IObserver* observer)
  {
    CSingleLock lock(m_ObserverCS);
    m_Observers.insert(m_Observers.begin(), observer);
  }

  /*! \brief Add an Observer to be notified of events. Use in case there's
  *   an abortable event and you want to have control over the order in
  *   which observers are notified
  *
  * \param[in] observer Observer that want to be notified
  * \param[in] before   Observer to be inserted before in the notification list
  */
  void AddObserverBefore(IObserver* observer, IObserver* before)
  {
    CSingleLock lock(m_ObserverCS);
    auto obs = std::find(m_Observers.begin(), m_Observers.end(), before);

    if (obs != m_Observers.end())
      m_Observers.insert(obs, observer);
    else
      m_Observers.push_back(observer);
  }

protected:

  /*! \brief Notifies all observers about an event
  *
  * \param[in] tag   the event that triggered
  * \param[in] args  variable amount of arguments depending on the event
  */
  template<typename Tag, typename... Arguments>
  void NotifyObservers(Tag&& tag, Arguments&&... args)
  {
    CSingleLock lock(m_ObserverCS);
    auto observers = m_Observers;
    lock.Leave();

    for (auto& observer : observers)
      observer->On(tag, args...);
  }

  /*! \brief Notifies observers about an event and allows
  *  an observer to return false to stop processing. Observers
  *  later in the list does not get notified about the event.
  *
  * \param[in] tag   the event that triggered
  * \param[in] args  variable amount of arguments depending on the event
  * \returns true on success, false if any observer cancelled the event
  */
  template<typename Tag, typename... Arguments>
  bool AskObservers(Tag&& tag, Arguments&&... args)
  {
    CSingleLock lock(m_ObserverCS);
    auto observers = m_Observers;
    lock.Leave();

    for (auto& observer : observers)
    {
      if (!observer->On(tag, args...))
        return false;
    }

    return true;
  }

private:
  std::vector<IObserver*> m_Observers;
  CCriticalSection m_ObserverCS;
};
