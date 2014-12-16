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
#include "system.h"

#if defined (HAVE_WAYLAND)

#include <boost/scoped_ptr.hpp>

#include "Application.h"
#include "xbmc/windowing/WindowingFactory.h"
#include "WinEventsWayland.h"

#include "wayland/EventListener.h"
#include "wayland/InputFactory.h"
#include "wayland/EventLoop.h"

namespace xwe = xbmc::wayland::events;

namespace
{
class XBMCListener :
  public xbmc::IEventListener
{
public:

  virtual void OnEvent(XBMC_Event &event);
  virtual void OnFocused();
  virtual void OnUnfocused();
};

XBMCListener g_listener;
boost::scoped_ptr <xbmc::InputFactory> g_inputInstance;
boost::scoped_ptr <xwe::Loop> g_eventLoop;
}

void XBMCListener::OnEvent(XBMC_Event &e)
{
  g_application.OnEvent(e);
}

void XBMCListener::OnFocused()
{
  g_application.m_AppFocused = true;
  g_Windowing.NotifyAppFocusChange(g_application.m_AppFocused);
}

void XBMCListener::OnUnfocused()
{
  g_application.m_AppFocused = false;
  g_Windowing.NotifyAppFocusChange(g_application.m_AppFocused);
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
  if (!g_eventLoop.get())
    return false;

  g_eventLoop->Dispatch();

  return true;
}

size_t CWinEventsWayland::GetQueueSize()
{
  /* We can't query the size of the queue */
  return 0;
}

void CWinEventsWayland::SetEventQueueStrategy(xwe::IEventQueueStrategy &strategy)
{
  g_eventLoop.reset(new xwe::Loop(g_listener, strategy));
}

void CWinEventsWayland::DestroyEventQueueStrategy()
{
  g_eventLoop.reset();
}

/* Once we know about a wayland seat, we can just create our manager
 * object to encapsulate all of that state. When the seat goes away
 * we just unset the manager object and it is all cleaned up at that
 * point */
void CWinEventsWayland::SetWaylandSeat(IDllWaylandClient &clientLibrary,
                                       IDllXKBCommon &xkbCommonLibrary,
                                       struct wl_seat *s)
{
  if (!g_eventLoop.get())
    throw std::logic_error("Must have a wl_display set before setting "
                           "the wl_seat in CWinEventsWayland ");

  g_inputInstance.reset(new xbmc::InputFactory(clientLibrary,
                                               xkbCommonLibrary,
                                               s,
                                               *g_eventLoop,
                                               *g_eventLoop));
}

void CWinEventsWayland::DestroyWaylandSeat()
{
  g_inputInstance.reset();
}

/* When a surface becomes available, this function should be called
 * to register it as the current one for processing input events on.
 * 
 * It is a precondition violation to call this function before
 * a seat has been registered */
void CWinEventsWayland::SetXBMCSurface(struct wl_surface *s)
{
  if (!g_inputInstance.get())
    throw std::logic_error("Must have a wl_seat set before setting "
                           "the wl_surface in CWinEventsWayland");
  
  g_inputInstance->SetXBMCSurface(s);
}

#endif
