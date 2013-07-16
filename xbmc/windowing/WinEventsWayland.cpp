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

#include <algorithm>
#include <sstream>
#include <vector>

#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/noncopyable.hpp>
#include <boost/scope_exit.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

#include <sys/poll.h>
#include <sys/mman.h>

#include <wayland-client.h>
#include <xkbcommon/xkbcommon.h>

#include "Application.h"
#include "WindowingFactory.h"
#include "WinEvents.h"
#include "WinEventsWayland.h"
#include "utils/Stopwatch.h"

#include "DllWaylandClient.h"
#include "DllXKBCommon.h"
#include "WaylandProtocol.h"

#include "wayland/Seat.h"
#include "wayland/Pointer.h"

namespace
{
IDllWaylandClient *g_clientLibrary = NULL;
struct wl_display *g_display = NULL;
}

namespace xbmc
{
/* IEventListener defines an interface for WaylandInput to deliver
 * simple events to. This interface is more or less artificial and used
 * to break the dependency between the event transformation code
 * and the rest of the system for testing purposes */
class IEventListener
{
public:

  virtual ~IEventListener() {}
  virtual bool OnEvent(XBMC_Event &) = 0;
  virtual bool OnFocused() = 0;
  virtual bool OnUnfocused() = 0;
};

class ICursorManager
{
public:

  virtual ~ICursorManager() {}
  virtual void SetCursor(uint32_t serial,
                         struct wl_surface *surface,
                         double surfaceX,
                         double surfaceY) = 0;
};

/* PointerProcessor implements IPointerReceiver and transforms input
 * wayland mouse event callbacks into XBMC events. It also handles
 * changing the cursor image on surface entry */
class PointerProcessor :
  public wayland::IPointerReceiver
{
public:

  PointerProcessor(IEventListener &,
                   ICursorManager &);

private:

  void Motion(uint32_t time,
              const float &x,
              const float &y);
  void Button(uint32_t serial,
              uint32_t time,
              uint32_t button,
              enum wl_pointer_button_state state);
  void Axis(uint32_t time,
            uint32_t axis,
            float value);
  void Enter(struct wl_surface *surface,
             double surfaceX,
             double surfaceY);

  IEventListener &m_listener;
  ICursorManager &m_cursorManager;

  uint32_t m_currentlyPressedButton;
  float    m_lastPointerX;
  float    m_lastPointerY;

  /* There is no defined export for these buttons -
   * wayland appears to just be using the evdev codes
   * directly */
  static const unsigned int WaylandLeftButton = 272;
  static const unsigned int WaylandRightButton = 273;
  static const unsigned int WaylandMiddleButton = 274;

  static const unsigned int WheelUpButton = 4;
  static const unsigned int WheelDownButton = 5;
  
};

class EventDispatch :
  public IEventListener
{
public:

  bool OnEvent(XBMC_Event &);
  bool OnFocused();
  bool OnUnfocused();
};
}

namespace xw = xbmc::wayland;

namespace
{
/* WaylandInput is effectively just a manager class that encapsulates
 * all input related information and ties together a wayland seat with
 * the rest of the XBMC input handling subsystem. It is an internal
 * class just for this file */
class WaylandInput :
  public xw::IInputReceiver,
  public xbmc::ICursorManager
{
public:

  WaylandInput(IDllWaylandClient &clientLibrary,
               IDllXKBCommon &xkbCommonLibrary,
               struct wl_seat *seat,
               xbmc::EventDispatch &dispatch);

  void SetXBMCSurface(struct wl_surface *s);

private:

  void SetCursor(uint32_t serial,
                 struct wl_surface *surface,
                 double surfaceX,
                 double surfaceY);

  bool InsertPointer(struct wl_pointer *);

  void RemovePointer();

  bool OnEvent(XBMC_Event &);

  IDllWaylandClient &m_clientLibrary;
  IDllXKBCommon &m_xkbCommonLibrary;

  xbmc::PointerProcessor m_pointerProcessor;

  boost::scoped_ptr<xw::Seat> m_seat;
  boost::scoped_ptr<xw::Pointer> m_pointer;
};

xbmc::EventDispatch g_dispatch;
boost::scoped_ptr <WaylandInput> g_inputInstance;
}

xbmc::PointerProcessor::PointerProcessor(IEventListener &listener,
                                         ICursorManager &manager) :
  m_listener(listener),
  m_cursorManager(manager)
{
}

void xbmc::PointerProcessor::Motion(uint32_t time,
                                    const float &x,
                                    const float &y)
{
  XBMC_Event event;

  event.type = XBMC_MOUSEMOTION;
  event.motion.xrel = ::round(x);
  event.motion.yrel = ::round(y);
  event.motion.state = 0;
  event.motion.type = XBMC_MOUSEMOTION;
  event.motion.which = 0;
  event.motion.x = event.motion.xrel;
  event.motion.y = event.motion.yrel;

  m_lastPointerX = x;
  m_lastPointerY = y;

  m_listener.OnEvent(event);
}

void xbmc::PointerProcessor::Button(uint32_t serial,
                                    uint32_t time,
                                    uint32_t button,
                                    enum wl_pointer_button_state state)
{
  static const struct ButtonTable
  {
    unsigned int WaylandButton;
    unsigned int XBMCButton;
  } buttonTable[] =
  {
    { WaylandLeftButton, 1 },
    { WaylandMiddleButton, 2 },
    { WaylandRightButton, 3 }
  };

  size_t buttonTableSize = sizeof(buttonTable) / sizeof(buttonTable[0]);

  unsigned int xbmcButton = 0;

  /* Find the xbmc button number that corresponds to the evdev
   * button that we just received. There may be some buttons we don't
   * recognize so just ignore them */
  for (size_t i = 0; i < buttonTableSize; ++i)
    if (buttonTable[i].WaylandButton == button)
      xbmcButton = buttonTable[i].XBMCButton;

  if (!xbmcButton)
    return;

  /* Keep track of currently pressed buttons, we need that for
   * motion events */
  if (state & WL_POINTER_BUTTON_STATE_PRESSED)
    m_currentlyPressedButton |= 1 << button;
  else if (state & WL_POINTER_BUTTON_STATE_RELEASED)
    m_currentlyPressedButton &= ~(1 << button);

  XBMC_Event event;

  event.type = state & WL_POINTER_BUTTON_STATE_PRESSED ?
               XBMC_MOUSEBUTTONDOWN : XBMC_MOUSEBUTTONUP;
  event.button.button = xbmcButton;
  event.button.state = 0;
  event.button.type = event.type;
  event.button.which = 0;
  event.button.x = ::round(m_lastPointerX);
  event.button.y = ::round(m_lastPointerY);

  m_listener.OnEvent(event);
}

void xbmc::PointerProcessor::Axis(uint32_t time,
                                  uint32_t axis,
                                  float value)
{
  if (axis == WL_POINTER_AXIS_VERTICAL_SCROLL)
  {
    /* Negative is up */
    bool direction = value < 0.0f;
    int  button = direction ? WheelUpButton :
                              WheelDownButton;

    /* For axis events we only care about the vector direction
     * and not the scalar magnitude. Every axis event callback
     * generates one scroll button event for XBMC */
    XBMC_Event event;

    event.type = XBMC_MOUSEBUTTONDOWN;
    event.button.button = button;
    event.button.state = 0;
    event.button.type = XBMC_MOUSEBUTTONDOWN;
    event.button.which = 0;
    event.button.x = ::round(m_lastPointerX);
    event.button.y = ::round(m_lastPointerY);

    m_listener.OnEvent(event);
    
    /* We must also send a button up on the same
     * wheel "button" */
    event.type = XBMC_MOUSEBUTTONUP;
    event.button.type = XBMC_MOUSEBUTTONUP;
    
    m_listener.OnEvent(event);
  }
}

void
xbmc::PointerProcessor::Enter(struct wl_surface *surface,
                              double surfaceX,
                              double surfaceY)
{
  m_cursorManager.SetCursor(0, NULL, 0, 0);
}

/* Once EventDispatch recieves some information it just forwards
 * it all on to XBMC */
bool xbmc::EventDispatch::OnEvent(XBMC_Event &e)
{
  return g_application.OnEvent(e);
}

bool xbmc::EventDispatch::OnFocused()
{
  g_application.m_AppFocused = true;
  g_Windowing.NotifyAppFocusChange(g_application.m_AppFocused);
  return true;
}

bool xbmc::EventDispatch::OnUnfocused()
{
  g_application.m_AppFocused = false;
  g_Windowing.NotifyAppFocusChange(g_application.m_AppFocused);
  return true;
}

WaylandInput::WaylandInput(IDllWaylandClient &clientLibrary,
                           IDllXKBCommon &xkbCommonLibrary,
                           struct wl_seat *seat,
                           xbmc::EventDispatch &dispatch) :
  m_clientLibrary(clientLibrary),
  m_xkbCommonLibrary(xkbCommonLibrary),
  m_pointerProcessor(dispatch, *this),
  m_seat(new xw::Seat(clientLibrary, seat, *this))
{
}

void
WaylandInput::SetXBMCSurface(struct wl_surface *s)
{
}

void WaylandInput::SetCursor(uint32_t serial,
                             struct wl_surface *surface,
                             double surfaceX,
                             double surfaceY)
{
  m_pointer->SetCursor(serial, surface, surfaceX, surfaceY);
}

bool WaylandInput::InsertPointer(struct wl_pointer *p)
{
  if (m_pointer.get())
    return false;

  m_pointer.reset(new xw::Pointer(m_clientLibrary,
                                  p,
                                  m_pointerProcessor));
  return true;
}

void WaylandInput::RemovePointer()
{
  m_pointer.reset();
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

/* Once we know about a wayland seat, we can just create our manager
 * object to encapsulate all of that state. When the seat goes away
 * we just unset the manager object and it is all cleaned up at that
 * point */
void CWinEventsWayland::SetWaylandSeat(IDllWaylandClient &clientLibrary,
                                       IDllXKBCommon &xkbCommonLibrary,
                                       struct wl_seat *s)
{
  if (!g_display)
    throw std::logic_error("Must have a wl_display set before setting "
                           "the wl_seat in CWinEventsWayland ");
  
  g_inputInstance.reset(new WaylandInput(clientLibrary,
                                         xkbCommonLibrary,
                                         s,
                                         g_dispatch));
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
}

#endif
