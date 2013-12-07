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
#include <vector>

#include <boost/weak_ptr.hpp>

#include "utils/Stopwatch.h"

#include "EventListener.h"
#include "EventQueueStrategy.h"
#include "TimeoutManager.h"

class IDllWaylandClient;

struct wl_display;

namespace xbmc
{
namespace wayland
{
namespace events
{
class IEventQueueStrategy;

/* Loop encapsulates the entire process of dispatching
 * wayland events and timers that might be in place for duplicate
 * processing. Calling its Dispatch() method will cause any pending
 * timers and events to be dispatched. It implements ITimeoutManager
 * and timeouts can be added directly to it */
class Loop :
  public xbmc::IEventListener,
  public xbmc::ITimeoutManager
{
public:

  Loop(xbmc::IEventListener &listener,
       IEventQueueStrategy &strategy);
  
  void Dispatch();
  
  struct CallbackTracker
  {
    typedef boost::weak_ptr <Callback> CallbackObserver;
    
    CallbackTracker(uint32_t time,
                    uint32_t initial,
                    const CallbackPtr &callback);
    
    uint32_t time;
    uint32_t remaining;
    CallbackObserver callback;
  };
  
private:

  CallbackPtr RepeatAfterMs(const Callback &callback,
                            uint32_t initial,
                            uint32_t timeout);
  void DispatchTimers();
  
  void OnEvent(XBMC_Event &);
  void OnFocused();
  void OnUnfocused();
  
  std::vector<CallbackTracker> m_callbackQueue;
  CStopWatch m_stopWatch;
  
  IEventQueueStrategy &m_eventQueue;
  xbmc::IEventListener &m_queueListener;
};
}
}
}
