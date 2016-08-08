#pragma once

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

#include "threads/CriticalSection.h"
#include <atomic>
#include <vector>

class Observable;
class ObservableMessageJob;

typedef enum
{
  ObservableMessageNone,
  ObservableMessageCurrentItem,
  ObservableMessageAddons,
  ObservableMessageEpg,
  ObservableMessageEpgContainer,
  ObservableMessageEpgActiveItem,
  ObservableMessageChannelGroup,
  ObservableMessageChannelGroupReset,
  ObservableMessageTimers,
  ObservableMessageTimersReset,
  ObservableMessageRecordings,
  ObservableMessagePeripheralsChanged
} ObservableMessage;

class Observer
{
public:
  Observer() = default;
  virtual ~Observer() = default;
  /*!
   * @brief Process a message from an observable.
   * @param obs The observable that sends the message.
   * @param msg The message.
   */
  virtual void Notify(const Observable &obs, const ObservableMessage msg) = 0;
};

class Observable
{
  friend class ObservableMessageJob;

public:
  Observable() = default;
  virtual ~Observable() = default;
  virtual Observable &operator=(const Observable &observable);

  /*!
   * @brief Register an observer.
   * @param obs The observer to register.
   */
  virtual void RegisterObserver(Observer *obs);

  /*!
   * @brief Unregister an observer.
   * @param obs The observer to unregister.
   */
  virtual void UnregisterObserver(Observer *obs);

  /*!
   * @brief Send a message to all observers when m_bObservableChanged is true.
   * @param message The message to send.
   */
  virtual void NotifyObservers(const ObservableMessage message = ObservableMessageNone);

  /*!
   * @brief Mark an observable changed.
   * @param bSetTo True to mark the observable changed, false to mark it as unchanged.
   */
  virtual void SetChanged(bool bSetTo = true);

  /*!
   * @brief Check whether this observable is being observed by an observer.
   * @param obs The observer to check.
   * @return True if this observable is being observed by the given observer, false otherwise.
   */
  virtual bool IsObserving(const Observer &obs) const;

protected:
  /*!
   * @brief Send a message to all observer when m_bObservableChanged is true.
   * @param obs The observer that sends the message.
   * @param message The message to send.
   */
  void SendMessage(const ObservableMessage message);

  std::atomic<bool>       m_bObservableChanged{false}; /*!< true when the observable is marked as changed, false otherwise */
  std::vector<Observer *> m_observers;                 /*!< all observers */
  CCriticalSection        m_obsCritSection;            /*!< mutex */
};
