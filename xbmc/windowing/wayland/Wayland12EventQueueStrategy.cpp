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
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/noncopyable.hpp>

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "windowing/DllWaylandClient.h"
#include "utils/log.h"

#include "Wayland12EventQueueStrategy.h"

namespace xw12 = xbmc::wayland::version_12;

namespace
{
/* Pending events dispatch happens in the main thread, so there is
 * no before-poll action that occurrs here */
void Nothing()
{
}

void Read(IDllWaylandClient &clientLibrary,
          struct wl_display *display)
{
  /* If wl_display_prepare_read() returns a nonzero value it means
   * that we still have more events to dispatch. So let the main thread
   * dispatch all the pending events first before trying to read
   * more events from the pipe */
  if ((*(clientLibrary.wl_display_prepare_read_proc()))(display) == 0)
    (*(clientLibrary.wl_display_read_events_proc()))(display);
}
}

xw12::EventQueueStrategy::EventQueueStrategy(IDllWaylandClient &clientLibrary,
                                             struct wl_display *display) :
  m_clientLibrary(clientLibrary),
  m_display(display),
  m_thread(boost::bind(Read, boost::ref(m_clientLibrary), display),
           boost::bind(Nothing),
           m_clientLibrary.wl_display_get_fd(m_display))
{
}

void
xw12::EventQueueStrategy::DispatchEventsFromMain()
{
  /* It is very important that these functions occurr in the order.
   * that they are written below. Deadlocks might occurr otherwise.
   * 
   * The first function dispatches any pending events that have been
   * determined from prior reads of the event queue without *also*
   * reading the event queue.
   * 
   * The second function function flushes the output buffer so that our
   * just-dispatched events will cause data to be processed on the
   * compositor which results in more events for us to read.
   * 
   * The input buffer will be read periodically (on the render loop,
   * which never sleeps)
   * 
   * If the functions are not called in this order, you might run into
   * a situation where pending-dispatch events might have generated a
   * write to the event queue in order to keep us awake (frame events
   * are a particular culprit here), or where events that we need to
   * dispatch in order to keep going are never read.
   */
  m_clientLibrary.wl_display_dispatch_pending(m_display);
  /* We perform the flush here and not in the reader thread
   * as it needs to come after eglSwapBuffers */
  m_clientLibrary.wl_display_flush(m_display);
}

void
xw12::EventQueueStrategy::PushAction(const Action &action)
{
  action();
}
