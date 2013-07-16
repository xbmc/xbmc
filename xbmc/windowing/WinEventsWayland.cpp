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

namespace
{
IDllWaylandClient *g_clientLibrary = NULL;
struct wl_display *g_display = NULL;
}

namespace xbmc
{
namespace wayland
{
class IInputReceiver
{
public:

  virtual ~IInputReceiver() {}

  virtual bool InsertPointer(struct wl_pointer *pointer) = 0;

  virtual void RemovePointer() = 0;
};

class IPointerReceiver
{
public:

  virtual ~IPointerReceiver() {}
  virtual void Motion(uint32_t time,
                      const float &x,
                      const float &y) = 0;
  virtual void Button(uint32_t serial,
                      uint32_t time,
                      uint32_t button,
                      enum wl_pointer_button_state state) = 0;
  virtual void Axis(uint32_t time,
                    uint32_t axis,
                    float value) = 0;
  virtual void Enter(struct wl_surface *surface,
                     double surfaceX,
                     double surfaceY) = 0;
};

class Pointer
{
public:

  Pointer(IDllWaylandClient &,
          struct wl_pointer *,
          IPointerReceiver &);
  ~Pointer();

  struct wl_pointer * GetWlPointer();

  void SetCursor(uint32_t serial,
                 struct wl_surface *surface,
                 int32_t hotspot_x,
                 int32_t hotspot_y);

  static void HandleEnterCallback(void *,
                                  struct wl_pointer *,
                                  uint32_t,
                                  struct wl_surface *,
                                  wl_fixed_t, 
                                  wl_fixed_t);
  static void HandleLeaveCallback(void *,
                                  struct wl_pointer *,
                                  uint32_t,
                                  struct wl_surface *);
  static void HandleMotionCallback(void *,
                                   struct wl_pointer *,
                                   uint32_t,
                                   wl_fixed_t,
                                   wl_fixed_t);
  static void HandleButtonCallback(void *,
                                   struct wl_pointer *,
                                   uint32_t,
                                   uint32_t,
                                   uint32_t,
                                   uint32_t);
  static void HandleAxisCallback(void *,
                                 struct wl_pointer *,
                                 uint32_t,
                                 uint32_t,
                                 wl_fixed_t);

private:

  void HandleEnter(uint32_t serial,
                   struct wl_surface *surface,
                   wl_fixed_t surfaceXFixed,
                   wl_fixed_t surfaceYFixed);
  void HandleLeave(uint32_t serial,
                   struct wl_surface *surface);
  void HandleMotion(uint32_t time,
                    wl_fixed_t surfaceXFixed,
                    wl_fixed_t surfaceYFixed);
  void HandleButton(uint32_t serial,
                    uint32_t time,
                    uint32_t button,
                    uint32_t state);
  void HandleAxis(uint32_t time,
                  uint32_t axis,
                  wl_fixed_t value);

  static const struct wl_pointer_listener m_listener;

  IDllWaylandClient &m_clientLibrary;
  struct wl_pointer *m_pointer;
  IPointerReceiver &m_receiver;
};

const struct wl_pointer_listener Pointer::m_listener =
{
  Pointer::HandleEnterCallback,
  Pointer::HandleLeaveCallback,
  Pointer::HandleMotionCallback,
  Pointer::HandleButtonCallback,
  Pointer::HandleAxisCallback
};

class Seat :
  public boost::noncopyable
{
public:

  Seat(IDllWaylandClient &,
       struct wl_seat *,
       IInputReceiver &);
  ~Seat();

  struct wl_seat * GetWlSeat();

  static void HandleCapabilitiesCallback(void *,
                                         struct wl_seat *,
                                         uint32_t);

private:

  static const struct wl_seat_listener m_listener;

  void HandleCapabilities(enum wl_seat_capability);

  IDllWaylandClient &m_clientLibrary;
  struct wl_seat * m_seat;
  IInputReceiver &m_input;

  enum wl_seat_capability m_currentCapabilities;
};

const struct wl_seat_listener Seat::m_listener =
{
  Seat::HandleCapabilitiesCallback
};
}

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
  static const unsigned int WaylandMiddleButton = 273;
  static const unsigned int WaylandRightButton = 274;

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

xw::Seat::Seat(IDllWaylandClient &clientLibrary,
               struct wl_seat *seat,
               IInputReceiver &receiver) :
  m_clientLibrary(clientLibrary),
  m_seat(seat),
  m_input(receiver),
  m_currentCapabilities(static_cast<enum wl_seat_capability>(0))
{
  protocol::AddListenerOnWaylandObject(m_clientLibrary,
                                       m_seat,
                                       &m_listener,
                                       reinterpret_cast<void *>(this));
}

xw::Seat::~Seat()
{
  protocol::DestroyWaylandObject(m_clientLibrary,
                                 m_seat);
}

void xw::Seat::HandleCapabilitiesCallback(void *data,
                                          struct wl_seat *seat,
                                          uint32_t cap)
{
  enum wl_seat_capability capabilities =
    static_cast<enum wl_seat_capability>(cap);
  static_cast<Seat *>(data)->HandleCapabilities(capabilities);
}

void xw::Seat::HandleCapabilities(enum wl_seat_capability cap)
{
  enum wl_seat_capability newCaps =
    static_cast<enum wl_seat_capability>(~m_currentCapabilities & cap);
  enum wl_seat_capability lostCaps =
    static_cast<enum wl_seat_capability>(m_currentCapabilities & ~cap);

  m_currentCapabilities = cap;

  if (newCaps & WL_SEAT_CAPABILITY_POINTER)
  {
    struct wl_pointer *pointer =
      protocol::CreateWaylandObject<struct wl_pointer *,
                                    struct wl_seat *>(m_clientLibrary,
                                                      m_seat,
                                                      m_clientLibrary.Get_wl_pointer_interface());
    protocol::CallMethodOnWaylandObject(m_clientLibrary,
                                        m_seat,
                                        WL_SEAT_GET_POINTER,
                                        pointer);
    m_input.InsertPointer(pointer);
  }

  if (lostCaps & WL_SEAT_CAPABILITY_POINTER)
    m_input.RemovePointer();
}

xw::Pointer::Pointer(IDllWaylandClient &clientLibrary,
                     struct wl_pointer *pointer,
                     IPointerReceiver &receiver) :
  m_clientLibrary(clientLibrary),
  m_pointer(pointer),
  m_receiver(receiver)
{
  protocol::AddListenerOnWaylandObject(m_clientLibrary,
                                       pointer,
                                       &m_listener,
                                       this);
}

xw::Pointer::~Pointer()
{
  protocol::DestroyWaylandObject(m_clientLibrary,
                                 m_pointer);
}

void xw::Pointer::SetCursor(uint32_t serial,
                            struct wl_surface *surface,
                            int32_t hotspot_x,
                            int32_t hotspot_y)
{
  protocol::CallMethodOnWaylandObject(m_clientLibrary,
                                      m_pointer,
                                      WL_POINTER_SET_CURSOR,
                                      serial,
                                      surface,
                                      hotspot_x,
                                      hotspot_y);
}

void xw::Pointer::HandleEnterCallback(void *data,
                                      struct wl_pointer *pointer,
                                      uint32_t serial,
                                      struct wl_surface *surface,
                                      wl_fixed_t x,
                                      wl_fixed_t y)
{
  static_cast<Pointer *>(data)->HandleEnter(serial,
                                            surface,
                                            x,
                                            y);
}

void xw::Pointer::HandleLeaveCallback(void *data,
                                      struct wl_pointer *pointer,
                                      uint32_t serial,
                                      struct wl_surface *surface)
{
  static_cast<Pointer *>(data)->HandleLeave(serial, surface);
}

void xw::Pointer::HandleMotionCallback(void *data,
                                       struct wl_pointer *pointer,
                                       uint32_t time,
                                       wl_fixed_t x,
                                       wl_fixed_t y)
{
  static_cast<Pointer *>(data)->HandleMotion(time,
                                             x,
                                             y);
}

void xw::Pointer::HandleButtonCallback(void *data,
                                       struct wl_pointer * pointer,
                                       uint32_t serial,
                                       uint32_t time,
                                       uint32_t button,
                                       uint32_t state)
{
  static_cast<Pointer *>(data)->HandleButton(serial,
                                             time,
                                             button,
                                             state);
}

void xw::Pointer::HandleAxisCallback(void *data,
                                     struct wl_pointer *pointer,
                                     uint32_t time,
                                     uint32_t axis,
                                     wl_fixed_t value)
{
  static_cast<Pointer *>(data)->HandleAxis(time,
                                           axis,
                                           value);
}

void xw::Pointer::HandleEnter(uint32_t serial,
                              struct wl_surface *surface,
                              wl_fixed_t surfaceXFixed,
                              wl_fixed_t surfaceYFixed)
{
  m_receiver.Enter(surface,
                   wl_fixed_to_double(surfaceXFixed),
                   wl_fixed_to_double(surfaceYFixed));
}

void xw::Pointer::HandleLeave(uint32_t serial,
                              struct wl_surface *surface)
{
}

void xw::Pointer::HandleMotion(uint32_t time,
                               wl_fixed_t surfaceXFixed,
                               wl_fixed_t surfaceYFixed)
{
  m_receiver.Motion(time,
                    wl_fixed_to_double(surfaceXFixed),
                    wl_fixed_to_double(surfaceYFixed));
}

void xw::Pointer::HandleButton(uint32_t serial,
                               uint32_t time,
                               uint32_t button,
                               uint32_t state)
{
  m_receiver.Button(serial,
                    time,
                    button,
                    static_cast<enum wl_pointer_button_state>(state));
}

void xw::Pointer::HandleAxis(uint32_t time,
                             uint32_t axis,
                             wl_fixed_t value)
{
  m_receiver.Axis(time,
                  axis,
                  wl_fixed_to_double(value));
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

  for (size_t i = 0; i < buttonTableSize; ++i)
    if (buttonTable[i].WaylandButton == button)
      xbmcButton = buttonTable[i].XBMCButton;

  if (!xbmcButton)
    return;

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

bool CWinEventsWayland::MessagePump()
{
  if (!g_display)
    return false;

  g_clientLibrary->wl_display_dispatch_pending(g_display);
  g_clientLibrary->wl_display_flush(g_display);
  g_clientLibrary->wl_display_dispatch(g_display);

  return true;
}

void CWinEventsWayland::SetWaylandDisplay(IDllWaylandClient *clientLibrary,
                                          struct wl_display *d)
{
  g_clientLibrary = clientLibrary;
  g_display = d;
}

void CWinEventsWayland::DestroyWaylandDisplay()
{
  MessagePump();

  g_display = NULL;
}

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

void CWinEventsWayland::SetXBMCSurface(struct wl_surface *s)
{
}

#endif
