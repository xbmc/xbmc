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
#include <algorithm>
#include <vector>

#include <boost/bind.hpp>
#include <boost/function.hpp>

#include "utils/Stopwatch.h"

#include "EventQueueStrategy.h"
#include "EventLoop.h"

namespace xwe = xbmc::wayland::events;

/* Once xwe::Loop recieves some information we need to enqueue
 * it to be dispatched on MessagePump. This is done by using
 * a command pattern to wrap the incoming data in function objects
 * and then pushing it to a queue.
 * 
 * The reason for this is that these three functions may or may not
 * be running in a separate thread depending on the dispatch
 * strategy in place.  */
void xwe::Loop::OnEvent(XBMC_Event &e)
{
  m_eventQueue.PushAction(boost::bind(&IEventListener::OnEvent,
                                      &m_queueListener, e));
}

void xwe::Loop::OnFocused()
{
  m_eventQueue.PushAction(boost::bind(&IEventListener::OnFocused,
                                      &m_queueListener));
}

void xwe::Loop::OnUnfocused()
{
  m_eventQueue.PushAction(boost::bind(&IEventListener::OnUnfocused,
                                      &m_queueListener));
}

xwe::Loop::Loop(IEventListener &queueListener,
                IEventQueueStrategy &strategy) :
  m_eventQueue(strategy),
  m_queueListener(queueListener)
{
  m_stopWatch.StartZero();
}

namespace
{
bool TimeoutInactive(const xwe::Loop::CallbackTracker &tracker)
{
  return tracker.callback.expired();
}

void SubtractTimeoutAndTrigger(xwe::Loop::CallbackTracker &tracker,
                               int time)
{
  int value = std::max(0, static_cast <int> (tracker.remaining - time));
  if (value == 0)
  {
    tracker.remaining = time;
    xbmc::ITimeoutManager::CallbackPtr callback (tracker.callback.lock());
    
    (*callback) ();
  }
  else
    tracker.remaining = value;
}

bool ByRemaining(const xwe::Loop::CallbackTracker &a,
                 const xwe::Loop::CallbackTracker &b)
{
  return a.remaining < b.remaining; 
}
}

xwe::Loop::CallbackTracker::CallbackTracker(uint32_t time,
                                            uint32_t initial,
                                            const xbmc::ITimeoutManager::CallbackPtr &cb) :
  time(time),
  remaining(time > initial ? time : initial),
  callback(cb)
{
}

void xwe::Loop::DispatchTimers()
{
  float elapsedMs = m_stopWatch.GetElapsedMilliseconds();
  m_stopWatch.Stop();
  /* We must subtract the elapsed time from each tracked timeout and
   * trigger any remaining ones. If a timeout is triggered, then its
   * remaining time will return to the original timeout value */
  std::for_each(m_callbackQueue.begin(), m_callbackQueue.end (),
                boost::bind(SubtractTimeoutAndTrigger,
                            _1,
                            static_cast<int>(elapsedMs)));
  /* Timeout times may have changed so that the timeouts are no longer
   * in order. Sort them so that they are. If they are unsorted,
   * the ordering of two timeouts, one which was added just before
   * the other which both reach a zero value at the same time,
   * will be undefined. */
  std::sort(m_callbackQueue.begin(), m_callbackQueue.end(),
            ByRemaining);
  m_stopWatch.StartZero();
}

void xwe::Loop::Dispatch()
{
  /* Remove any timers which are no longer active */
  m_callbackQueue.erase (std::remove_if(m_callbackQueue.begin(),
                                        m_callbackQueue.end(),
                                        TimeoutInactive),
                         m_callbackQueue.end());

  DispatchTimers();
  
  /* Calculate the poll timeout based on any current
   * timers on the main loop. */
  uint32_t minTimeout = 0;
  for (std::vector<CallbackTracker>::iterator it = m_callbackQueue.begin();
       it != m_callbackQueue.end();
       ++it)
  {
    if (minTimeout < it->remaining)
      minTimeout = it->remaining;
  }
  
  m_eventQueue.DispatchEventsFromMain();
}

xbmc::ITimeoutManager::CallbackPtr
xwe::Loop::RepeatAfterMs(const xbmc::ITimeoutManager::Callback &cb,
                         uint32_t initial,
                         uint32_t time)
{
  CallbackPtr ptr(new Callback(cb));
  
  bool     inserted = false;
  
  for (std::vector<CallbackTracker>::iterator it = m_callbackQueue.begin();
       it != m_callbackQueue.end();
       ++it)
  {
    /* The appropriate place to insert is just before an existing
     * timer which has a greater remaining time than ours */
    if (it->remaining > time)
    {
      m_callbackQueue.insert(it, CallbackTracker(time, initial, ptr));
      inserted = true;
      break;
    }
  }
  
  /* Insert at the back */
  if (!inserted)
    m_callbackQueue.push_back(CallbackTracker(time, initial, ptr));

  return ptr;
}
