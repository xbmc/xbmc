/*
*      Copyright (C) 2005-2013 Team XBMC
*      http://www.xbmc.org
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
#include "system.h"

#if defined (HAVE_WAYLAND)

#include <memory>
#include <sstream>

#include <boost/noncopyable.hpp>
#include <boost/scope_exit.hpp>
#include <boost/scoped_ptr.hpp>

#include <sys/mman.h>

#include <wayland-client.h>
#include <xkbcommon/xkbcommon.h>

#include "Application.h"
#include "WindowingFactory.h"
#include "WinEvents.h"
#include "WinEventsWayland.h"

#include "DllWaylandClient.h"
#include "DllXKBCommon.h"
#include "WaylandProtocol.h"

namespace
{
IDllWaylandClient *g_clientLibrary = NULL;
struct wl_display *g_display = NULL;
}

CWinEventsWayland::CWinEventsWayland()
{
}

void CWinEventsWayland::RefreshDevices()
{
}

bool CWinEventsWayland::IsRemoteLowBattery()
{
  return false;
}

/* This function reads the display connection and dispatches
 * any events through the specified object listeners */
bool CWinEventsWayland::MessagePump()
{
  if (!g_display)
    return false;

  /* It is very important that these functions occurr in this order.
   * Deadlocks might occurr otherwise.
   * 
   * The first function dispatches any pending events that have been
   * determined from prior reads of the event queue without *also*
   * reading the event queue.
   * 
   * The second function flushes the output buffer of any requests
   * to be made to the server, including requests that should have
   * been made in response to just-dispatched events earlier.
   * 
   * The third function reads the input buffer and dispatches any events
   * that occurred.
   * 
   * If the functions are not called in this order, you might run into
   * a situation where pending-dispatch events might have generated a
   * write to the event queue in order to keep us awake (frame events
   * are a particular culprit here), or where events that we need to
   * dispatch in order to keep going are never read.
   */
  g_clientLibrary->wl_display_dispatch_pending(g_display);
  g_clientLibrary->wl_display_flush(g_display);
  g_clientLibrary->wl_display_dispatch(g_display);

  return true;
}

size_t CWinEventsWayland::GetQueueSize()
{
  /* We can't query the size of the queue */
  return 0;
}

void CWinEventsWayland::SetWaylandDisplay(IDllWaylandClient *clientLibrary,
                                          struct wl_display *d)
{
  g_clientLibrary = clientLibrary;
  g_display = d;
}

void CWinEventsWayland::DestroyWaylandDisplay()
{
  /* We should make sure that everything else is gone first before
   * destroying the display */
  MessagePump();
  g_display = NULL;
}

#endif
