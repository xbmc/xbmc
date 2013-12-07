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

#include "EventQueueStrategy.h"
#include "PollThread.h"

class IDllWaylandClient;

struct wl_display;

namespace xbmc
{
namespace wayland
{
namespace version_12
{
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
};
}
}
}
