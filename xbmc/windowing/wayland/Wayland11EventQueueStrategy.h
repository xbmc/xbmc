#pragma once

/*
 *      Copyright (C) 2011-2013 Team XBMC
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
#include <queue>

#include <boost/noncopyable.hpp>

#include "threads/CriticalSection.h"
#include "EventQueueStrategy.h"
#include "PollThread.h"

class IDllWaylandClient;

struct wl_display;

namespace xbmc
{
namespace wayland
{
namespace version_11
{
/* The EventQueueStrategy for Wayland <= 1.1 requires that events
 * be dispatched and initally processed in a separate thread. This
 * means that all the wayland proxy object wrappers callbacks will
 * be running in the same thread that reads the event queue for events.
 * 
 * When those events are initially processed, they will be put into the
 * main WaylandEventLoop queue and dispatched sequentially from there
 * periodically (once every redraw) into the main thread.
 * 
 * The reason for this is that Wayland versions prior to 1.1 provide
 * no means to read the event queue without also dispatching pending
 * events and callbacks on proxy objects. But we also cannot block the
 * main thread, which may occurr if we call wl_display_dispatch
 * and there is no pending frame callback because our surface is not
 * visible.
 */
class EventQueueStrategy :
  public events::IEventQueueStrategy
{
public:

  EventQueueStrategy(IDllWaylandClient &clientLibrary,
                     struct wl_display *display);

  void PushAction(const Action &action);
  void DispatchEventsFromMain();

private:

  IDllWaylandClient &m_clientLibrary;
  struct wl_display *m_display;

  events::PollThread m_thread;
  
  CCriticalSection m_actionsMutex;
  std::queue<Action> m_actions;
};
}
}
}
