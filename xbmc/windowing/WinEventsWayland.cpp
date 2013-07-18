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
#include "utils/log.h"

#include "DllWaylandClient.h"
#include "DllXKBCommon.h"
#include "WaylandProtocol.h"

namespace xbmc
{
namespace wayland
{
class IInputReceiver
{
public:

  virtual ~IInputReceiver() {}

  virtual bool InsertPointer(struct wl_pointer *pointer) = 0;
  virtual bool InsertKeyboard(struct wl_keyboard *keyboard) = 0;

  virtual void RemovePointer() = 0;
  virtual void RemoveKeyboard() = 0;
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

class IKeymap
{
public:

  virtual ~IKeymap() {};
  
  virtual uint32_t KeysymForKeycode(uint32_t code) const = 0;
  virtual void UpdateMask(uint32_t depressed,
                          uint32_t latched,
                          uint32_t locked,
                          uint32_t group) = 0;
  virtual uint32_t CurrentModifiers() = 0;
};

class IKeyboardReceiver
{
public:

  virtual ~IKeyboardReceiver() {}

  virtual void UpdateKeymap(uint32_t format,
                            int fd,
                            uint32_t size) = 0;
  virtual void Enter(uint32_t serial,
                     struct wl_surface *surface,
                     struct wl_array *keys) = 0;
  virtual void Leave(uint32_t serial,
                     struct wl_surface *surface) = 0;
  virtual void Key(uint32_t serial,
                   uint32_t time,
                   uint32_t key,
                   enum wl_keyboard_key_state state) = 0;
  virtual void Modifier(uint32_t serial,
                        uint32_t depressed,
                        uint32_t latched,
                        uint32_t locked,
                        uint32_t group) = 0;
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

class Keyboard :
  public boost::noncopyable
{
public:

  Keyboard(IDllWaylandClient &,
           struct wl_keyboard *,
           IKeyboardReceiver &);
  ~Keyboard();

  struct wl_keyboard * GetWlKeyboard();

  static void HandleKeymapCallback(void *,
                                   struct wl_keyboard *,
                                   uint32_t,
                                   int,
                                   uint32_t);
  static void HandleEnterCallback(void *,
                                  struct wl_keyboard *,
                                  uint32_t,
                                  struct wl_surface *,
                                  struct wl_array *);
  static void HandleLeaveCallback(void *,
                                  struct wl_keyboard *,
                                  uint32_t,
                                  struct wl_surface *);
  static void HandleKeyCallback(void *,
                                struct wl_keyboard *,
                                uint32_t,
                                uint32_t,
                                uint32_t,
                                uint32_t);
  static void HandleModifiersCallback(void *,
                                      struct wl_keyboard *,
                                      uint32_t,
                                      uint32_t,
                                      uint32_t,
                                      uint32_t,
                                      uint32_t);

private:

  void HandleKeymap(uint32_t format,
                    int fd,
                    uint32_t size);
  void HandleEnter(uint32_t serial,
                   struct wl_surface *surface,
                   struct wl_array *keys);
  void HandleLeave(uint32_t serial,
                   struct wl_surface *surface);
  void HandleKey(uint32_t serial,
                 uint32_t time,
                 uint32_t key,
                 uint32_t state);
  void HandleModifiers(uint32_t serial,
                       uint32_t mods_depressed,
                       uint32_t mods_latched,
                       uint32_t mods_locked,
                       uint32_t group);

  static const struct wl_keyboard_listener m_listener;

  IDllWaylandClient &m_clientLibrary;
  struct wl_keyboard *m_keyboard;
  IKeyboardReceiver &m_reciever;
};

const struct wl_keyboard_listener Keyboard::m_listener =
{
  Keyboard::HandleKeymapCallback,
  Keyboard::HandleEnterCallback,
  Keyboard::HandleLeaveCallback,
  Keyboard::HandleKeyCallback,
  Keyboard::HandleModifiersCallback
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

class ITimeoutManager
{
public:
  
  typedef boost::function<void()> Callback;
  typedef boost::shared_ptr <Callback> CallbackPtr;
  
  virtual ~ITimeoutManager() {}
  virtual CallbackPtr RepeatAfterMs (const Callback &callback,
                                     uint32_t initial, 
                                     uint32_t timeout) = 0;
};

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

class XKBKeymap :
  public wayland::IKeymap
{
public:

  XKBKeymap(IDllXKBCommon &m_xkbCommonLibrary,
            struct xkb_keymap *keymap,
            struct xkb_state *state);
  ~XKBKeymap();

private:

  uint32_t KeysymForKeycode(uint32_t code) const;
  void UpdateMask(uint32_t depressed,
                  uint32_t latched,
                  uint32_t locked,
                  uint32_t group);
  uint32_t CurrentModifiers();

  IDllXKBCommon &m_xkbCommonLibrary;

  struct xkb_keymap *m_keymap;
  struct xkb_state *m_state;

  xkb_mod_index_t m_internalLeftControlIndex;
  xkb_mod_index_t m_internalLeftShiftIndex;
  xkb_mod_index_t m_internalLeftSuperIndex;
  xkb_mod_index_t m_internalLeftAltIndex;
  xkb_mod_index_t m_internalLeftMetaIndex;

  xkb_mod_index_t m_internalRightControlIndex;
  xkb_mod_index_t m_internalRightShiftIndex;
  xkb_mod_index_t m_internalRightSuperIndex;
  xkb_mod_index_t m_internalRightAltIndex;
  xkb_mod_index_t m_internalRightMetaIndex;

  xkb_mod_index_t m_internalCapsLockIndex;
  xkb_mod_index_t m_internalNumLockIndex;
  xkb_mod_index_t m_internalModeIndex;
};

class KeyboardProcessor :
  public wayland::IKeyboardReceiver
{
public:

  KeyboardProcessor(IDllXKBCommon &m_xkbCommonLibrary,
                    IEventListener &listener,
                    ITimeoutManager &timeouts);
  ~KeyboardProcessor();
  
  void SetXBMCSurface(struct wl_surface *xbmcWindow);

private:

  void UpdateKeymap(uint32_t format,
                    int fd,
                    uint32_t size);
  void Enter(uint32_t serial,
             struct wl_surface *surface,
             struct wl_array *keys);
  void Leave(uint32_t serial,
             struct wl_surface *surface);
  void Key(uint32_t serial,
           uint32_t time,
           uint32_t key,
           enum wl_keyboard_key_state state);
  void Modifier(uint32_t serial,
                uint32_t depressed,
                uint32_t latched,
                uint32_t locked,
                uint32_t group);
                
  void SendKeyToXBMC(uint32_t key,
                     uint32_t sym,
                     uint32_t type);
  void RepeatCallback(uint32_t key,
                      uint32_t sym);

  IDllXKBCommon &m_xkbCommonLibrary;

  boost::scoped_ptr<wayland::IKeymap> m_keymap;
  IEventListener &m_listener;
  ITimeoutManager &m_timeouts;
  struct wl_surface *m_xbmcWindow;
  
  ITimeoutManager::CallbackPtr m_repeatCallback;
  uint32_t m_repeatSym;
  
  struct xkb_context *m_context;
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
class WaylandEventLoop :
  public xbmc::ITimeoutManager
{
public:

  WaylandEventLoop(IDllWaylandClient &clientLibrary,
                   struct wl_display *display);
  
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
  
  IDllWaylandClient &m_clientLibrary;
  
  struct wl_display *m_display;
  std::vector<CallbackTracker> m_callbackQueue;
  CStopWatch m_stopWatch;
};

class WaylandInput :
  public xw::IInputReceiver,
  public xbmc::ICursorManager
{
public:

  WaylandInput(IDllWaylandClient &clientLibrary,
               IDllXKBCommon &xkbCommonLibrary,
               struct wl_seat *seat,
               xbmc::EventDispatch &dispatch,
               xbmc::ITimeoutManager &timeouts);

  void SetXBMCSurface(struct wl_surface *s);

private:

  void SetCursor(uint32_t serial,
                 struct wl_surface *surface,
                 double surfaceX,
                 double surfaceY);

  bool InsertPointer(struct wl_pointer *);
  bool InsertKeyboard(struct wl_keyboard *);

  void RemovePointer();
  void RemoveKeyboard();

  bool OnEvent(XBMC_Event &);

  IDllWaylandClient &m_clientLibrary;
  IDllXKBCommon &m_xkbCommonLibrary;

  xbmc::PointerProcessor m_pointerProcessor;
  xbmc::KeyboardProcessor m_keyboardProcessor;

  boost::scoped_ptr<xw::Seat> m_seat;
  boost::scoped_ptr<xw::Pointer> m_pointer;
  boost::scoped_ptr<xw::Keyboard> m_keyboard;
};

xbmc::EventDispatch g_dispatch;
boost::scoped_ptr <WaylandInput> g_inputInstance;
boost::scoped_ptr <WaylandEventLoop> g_eventLoop;
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

  if (newCaps & WL_SEAT_CAPABILITY_KEYBOARD)
  {
    struct wl_keyboard *keyboard =
      protocol::CreateWaylandObject<struct wl_keyboard *,
                                    struct wl_seat *>(m_clientLibrary,
                                                      m_seat,
                                                      m_clientLibrary.Get_wl_keyboard_interface());
    protocol::CallMethodOnWaylandObject(m_clientLibrary,
                                        m_seat,
                                        WL_SEAT_GET_KEYBOARD,
                                        keyboard);
    m_input.InsertKeyboard(keyboard);
  }

  if (lostCaps & WL_SEAT_CAPABILITY_POINTER)
    m_input.RemovePointer();
  if (lostCaps & WL_SEAT_CAPABILITY_KEYBOARD)
    m_input.RemoveKeyboard();
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

xw::Keyboard::Keyboard(IDllWaylandClient &clientLibrary,
                       struct wl_keyboard *keyboard,
                       IKeyboardReceiver &receiver) :
  m_clientLibrary(clientLibrary),
  m_keyboard(keyboard),
  m_reciever(receiver)
{
  protocol::AddListenerOnWaylandObject(m_clientLibrary,
                                       m_keyboard,
                                       &m_listener,
                                       this);
}

xw::Keyboard::~Keyboard()
{
  protocol::DestroyWaylandObject(m_clientLibrary,
                                 m_keyboard);
}

void xw::Keyboard::HandleKeymapCallback(void *data,
                                        struct wl_keyboard *keyboard,
                                        uint32_t format,
                                        int fd,
                                        uint32_t size)
{
  static_cast <Keyboard *>(data)->HandleKeymap(format,
                                               fd,
                                               size);
}

void xw::Keyboard::HandleEnterCallback(void *data,
                                       struct wl_keyboard *keyboard,
                                       uint32_t serial,
                                       struct wl_surface *surface,
                                       struct wl_array *keys)
{
  static_cast<Keyboard *>(data)->HandleEnter(serial,
                                             surface,
                                             keys);
}

void xw::Keyboard::HandleLeaveCallback(void *data,
                                       struct wl_keyboard *keyboard,
                                       uint32_t serial,
                                       struct wl_surface *surface)
{
  static_cast<Keyboard *>(data)->HandleLeave(serial,
                                             surface);
}

void xw::Keyboard::HandleKeyCallback(void *data,
                                     struct wl_keyboard *keyboard,
                                     uint32_t serial,
                                     uint32_t time,
                                     uint32_t key,
                                     uint32_t state)
{
  static_cast<Keyboard *>(data)->HandleKey(serial,
                                           time,
                                           key,
                                           state);
}

void xw::Keyboard::HandleModifiersCallback(void *data,
                                           struct wl_keyboard *keyboard,
                                           uint32_t serial,
                                           uint32_t mods_depressed,
                                           uint32_t mods_latched,
                                           uint32_t mods_locked,
                                           uint32_t group)
{
  static_cast<Keyboard *>(data)->HandleModifiers(serial,
                                                 mods_depressed,
                                                 mods_latched,
                                                 mods_locked,
                                                 group);
}

void xw::Keyboard::HandleKeymap(uint32_t format,
                                int fd,
                                uint32_t size)
{
  m_reciever.UpdateKeymap(format, fd, size);
}

void xw::Keyboard::HandleEnter(uint32_t serial,
                               struct wl_surface *surface,
                               struct wl_array *keys)
{
  m_reciever.Enter(serial, surface, keys);
}

void xw::Keyboard::HandleLeave(uint32_t serial,
                               struct wl_surface *surface)
{
  m_reciever.Leave(serial, surface);
}

void xw::Keyboard::HandleKey(uint32_t serial,
                             uint32_t time,
                             uint32_t key,
                             uint32_t state)
{
  m_reciever.Key(serial,
                 time,
                 key,
                 static_cast<enum wl_keyboard_key_state>(state));
}

void xw::Keyboard::HandleModifiers(uint32_t serial,
                                   uint32_t mods_depressed,
                                   uint32_t mods_latched,
                                   uint32_t mods_locked,
                                   uint32_t group)
{
  m_reciever.Modifier(serial,
                      mods_depressed,
                      mods_latched,
                      mods_locked,
                      group);
}

xbmc::XKBKeymap::XKBKeymap(IDllXKBCommon &xkbCommonLibrary,
                           struct xkb_keymap *keymap,
                           struct xkb_state *state) :
  m_xkbCommonLibrary(xkbCommonLibrary),
  m_keymap(keymap),
  m_state(state),
  m_internalLeftControlIndex(m_xkbCommonLibrary.xkb_keymap_mod_get_index(m_keymap,
                                                                         XKB_MOD_NAME_CTRL)),
  m_internalLeftShiftIndex(m_xkbCommonLibrary.xkb_keymap_mod_get_index(m_keymap,
                                                                       XKB_MOD_NAME_SHIFT)),
  m_internalLeftSuperIndex(m_xkbCommonLibrary.xkb_keymap_mod_get_index(m_keymap,
                                                                       XKB_MOD_NAME_LOGO)),
  m_internalLeftAltIndex(m_xkbCommonLibrary.xkb_keymap_mod_get_index(m_keymap,
                                                                     XKB_MOD_NAME_ALT)),
  m_internalLeftMetaIndex(m_xkbCommonLibrary.xkb_keymap_mod_get_index(m_keymap,
                                                                      "Meta")),
  m_internalRightControlIndex(m_xkbCommonLibrary.xkb_keymap_mod_get_index(m_keymap,
                                                                          "RControl")),
  m_internalRightShiftIndex(m_xkbCommonLibrary.xkb_keymap_mod_get_index(m_keymap,
                                                                        "RShift")),
  m_internalRightSuperIndex(m_xkbCommonLibrary.xkb_keymap_mod_get_index(m_keymap,
                                                                        "Hyper")),
  m_internalRightAltIndex(m_xkbCommonLibrary.xkb_keymap_mod_get_index(m_keymap,
                                                                      "AltGr")),
  m_internalRightMetaIndex(m_xkbCommonLibrary.xkb_keymap_mod_get_index(m_keymap,
                                                                       "Meta")),
  m_internalCapsLockIndex(m_xkbCommonLibrary.xkb_keymap_mod_get_index(m_keymap,
                                                                      XKB_LED_NAME_CAPS)),
  m_internalNumLockIndex(m_xkbCommonLibrary.xkb_keymap_mod_get_index(m_keymap,
                                                                     XKB_LED_NAME_NUM)),
  m_internalModeIndex(m_xkbCommonLibrary.xkb_keymap_mod_get_index(m_keymap,
                                                                  XKB_LED_NAME_SCROLL))
{
}

xbmc::XKBKeymap::~XKBKeymap()
{
  m_xkbCommonLibrary.xkb_state_unref(m_state);
  m_xkbCommonLibrary.xkb_keymap_unref(m_keymap);
}

uint32_t
xbmc::XKBKeymap::KeysymForKeycode(uint32_t code) const
{
  const xkb_keysym_t *syms;
  uint32_t numSyms;

  numSyms = m_xkbCommonLibrary.xkb_state_key_get_syms(m_state, code + 8, &syms);

  if (numSyms == 1)
    return static_cast<uint32_t>(syms[0]);

  std::stringstream ss;
  ss << "Pressed key "
     << std::hex
     << code
     << std::dec
     << " is unspported";

  throw std::runtime_error(ss.str());
}

uint32_t
xbmc::XKBKeymap::CurrentModifiers()
{
  XBMCMod xbmcModifiers = XBMCKMOD_NONE;
  enum xkb_state_component components =
    static_cast <xkb_state_component>(XKB_STATE_DEPRESSED |
                                      XKB_STATE_LATCHED |
                                      XKB_STATE_LOCKED);
  xkb_mod_mask_t mask = m_xkbCommonLibrary.xkb_state_serialize_mods(m_state,
                                                                    components);
  struct ModTable
  {
    xkb_mod_index_t xkbMod;
    XBMCMod xbmcMod;
  } modTable[] =
  {
    { m_internalLeftShiftIndex, XBMCKMOD_LSHIFT },
    { m_internalRightShiftIndex, XBMCKMOD_RSHIFT },
    { m_internalLeftShiftIndex, XBMCKMOD_LSUPER },
    { m_internalRightSuperIndex, XBMCKMOD_RSUPER },
    { m_internalLeftControlIndex, XBMCKMOD_LCTRL },
    { m_internalRightControlIndex, XBMCKMOD_RCTRL },
    { m_internalLeftAltIndex, XBMCKMOD_LALT },
    { m_internalRightAltIndex, XBMCKMOD_RALT },
    { m_internalLeftMetaIndex, XBMCKMOD_LMETA },
    { m_internalRightMetaIndex, XBMCKMOD_RMETA },
    { m_internalNumLockIndex, XBMCKMOD_NUM },
    { m_internalCapsLockIndex, XBMCKMOD_CAPS },
    { m_internalModeIndex, XBMCKMOD_MODE }
  };

  size_t modTableSize = sizeof(modTable) / sizeof(modTable[0]);

  for (size_t i = 0; i < modTableSize; ++i)
  {
    if (mask & (1 << modTable[i].xkbMod))
      xbmcModifiers = static_cast<XBMCMod>(xbmcModifiers |
                                           modTable[i].xbmcMod);
  }

  return static_cast<uint32_t>(xbmcModifiers);
}

void
xbmc::XKBKeymap::UpdateMask(uint32_t depressed,
                            uint32_t latched,
                            uint32_t locked,
                            uint32_t group)
{
  m_xkbCommonLibrary.xkb_state_update_mask(m_state,
                                           depressed,
                                           latched,
                                           locked,
                                           0,
                                           0,
                                           group);
}

xbmc::KeyboardProcessor::KeyboardProcessor(IDllXKBCommon &xkbCommonLibrary,
                                           IEventListener &listener,
                                           ITimeoutManager &timeouts) :
  m_xkbCommonLibrary(xkbCommonLibrary),
  m_listener(listener),
  m_timeouts(timeouts),
  m_xbmcWindow(NULL),
  m_repeatSym(0),
  m_context(NULL)
{
  enum xkb_context_flags flags =
    static_cast<enum xkb_context_flags>(0);

  m_context = m_xkbCommonLibrary.xkb_context_new(flags);
  
  if (!m_context)
    throw std::runtime_error("Failed to create xkb context");
}

xbmc::KeyboardProcessor::~KeyboardProcessor()
{
  m_xkbCommonLibrary.xkb_context_unref(m_context);
}

void
xbmc::KeyboardProcessor::SetXBMCSurface(struct wl_surface *s)
{
  m_xbmcWindow = s;
}

void
xbmc::KeyboardProcessor::UpdateKeymap(uint32_t format,
                                      int fd,
                                      uint32_t size)
{
  BOOST_SCOPE_EXIT((fd))
  {
    close(fd);
  } BOOST_SCOPE_EXIT_END

  if (format != WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1)
    throw std::runtime_error("Server gave us a keymap we don't understand");

  const char *keymapString = static_cast<const char *>(mmap(NULL,
                                                            size,
                                                            PROT_READ,
                                                            MAP_SHARED,
                                                            fd,
                                                            0));
  if (keymapString == MAP_FAILED)
  {
    std::stringstream ss;
    ss << "mmap: " << strerror(errno);
    throw std::runtime_error(ss.str());
  }

  BOOST_SCOPE_EXIT((keymapString)(size))
  {
    munmap(const_cast<void *>(static_cast<const void *>(keymapString)),
                              size);
  } BOOST_SCOPE_EXIT_END

  bool successfullyCreatedKeyboard = false;

  enum xkb_keymap_compile_flags flags =
    static_cast<enum xkb_keymap_compile_flags>(0);
  struct xkb_keymap *keymap =
    m_xkbCommonLibrary.xkb_keymap_new_from_string(m_context,
                                                  keymapString,
                                                  XKB_KEYMAP_FORMAT_TEXT_V1,
                                                  flags);

  if (!keymap)
    throw std::runtime_error("Failed to compile keymap");

  BOOST_SCOPE_EXIT((&m_xkbCommonLibrary)(&successfullyCreatedKeyboard)(keymap))
  {
    if (!successfullyCreatedKeyboard)
      m_xkbCommonLibrary.xkb_keymap_unref(keymap);
  } BOOST_SCOPE_EXIT_END

  struct xkb_state *state = m_xkbCommonLibrary.xkb_state_new(keymap);

  if (!state)
    throw std::runtime_error("Failed to create keyboard state");

  m_keymap.reset(new XKBKeymap(m_xkbCommonLibrary,
                               keymap,
                               state));
  
  successfullyCreatedKeyboard = true;
}

void
xbmc::KeyboardProcessor::Enter(uint32_t serial,
                               struct wl_surface *surface,
                               struct wl_array *keys)
{
  if (surface == m_xbmcWindow)
  {
    m_listener.OnFocused();
  }
}

void
xbmc::KeyboardProcessor::Leave(uint32_t serial,
                               struct wl_surface *surface)
{
  if (surface == m_xbmcWindow)
  {
    m_listener.OnUnfocused();
  }
}

void
xbmc::KeyboardProcessor::SendKeyToXBMC(uint32_t key,
                                       uint32_t sym,
                                       uint32_t eventType)
{
  XBMC_Event event;
  event.type = eventType;
  event.key.keysym.scancode = key;
  event.key.keysym.sym = static_cast<XBMCKey>(sym);
  event.key.keysym.unicode = static_cast<XBMCKey>(sym);
  event.key.keysym.mod = static_cast<XBMCMod>(m_keymap->CurrentModifiers());
  event.key.state = 0;
  event.key.type = event.type;
  event.key.which = '0';

  m_listener.OnEvent(event);
}

void
xbmc::KeyboardProcessor::RepeatCallback(uint32_t key,
                                        uint32_t sym)
{
  /* Release and press the key again */
  SendKeyToXBMC(key, sym, XBMC_KEYUP);
  SendKeyToXBMC(key, sym, XBMC_KEYDOWN);
}

void
xbmc::KeyboardProcessor::Key(uint32_t serial,
                             uint32_t time,
                             uint32_t key,
                             enum wl_keyboard_key_state state)
{
  if (!m_keymap.get())
    throw std::logic_error("a keymap must be set before processing key events");

  uint32_t sym = XKB_KEY_NoSymbol;

  try
  {
    sym = m_keymap->KeysymForKeycode(key);
  }
  catch (const std::runtime_error &err)
  {
    /* TODO: Switch to CLog */
    CLog::Log(LOGERROR, "%s: %s", __FUNCTION__, err.what());
    return;
  }
  
  /* Strip high bits from functional keys */
  if ((sym & ~(0xff00)) < 0x1b)
    sym = sym & ~(0xff00);
  else if ((sym & ~(0xff00)) == 0xff)
    sym = static_cast<uint32_t>(XBMCK_DELETE);
  
  const bool isNavigationKey = (sym >= 0xff50 && sym <= 0xff58);
  const bool isModifierKey = (sym >= 0xffe1 && sym <= 0xffee);
  const bool isKeyPadKey = (sym >= 0xffbd && sym <= 0xffb9);
  const bool isFKey = (sym >= 0xffbe && sym <= 0xffcc);
  const bool isMediaKey = (sym >= 0x1008ff26 && sym <= 0x1008ffa2);

  if (isNavigationKey ||
      isModifierKey ||
      isKeyPadKey ||
      isFKey ||
      isMediaKey)
  {
    /* Navigation keys are not in line, so we need to
     * look them up */
    static const struct NavigationKeySyms
    {
      uint32_t xkb;
      XBMCKey xbmc;
    } navigationKeySyms[] =
    {
      { XKB_KEY_Home, XBMCK_HOME },
      { XKB_KEY_Left, XBMCK_LEFT },
      { XKB_KEY_Right, XBMCK_RIGHT },
      { XKB_KEY_Down, XBMCK_DOWN },
      { XKB_KEY_Up, XBMCK_UP },
      { XKB_KEY_Page_Up, XBMCK_PAGEUP },
      { XKB_KEY_Page_Down, XBMCK_PAGEDOWN },
      { XKB_KEY_End, XBMCK_END },
      { XKB_KEY_Insert, XBMCK_INSERT },
      { XKB_KEY_KP_0, XBMCK_KP0 },
      { XKB_KEY_KP_1, XBMCK_KP1 },
      { XKB_KEY_KP_2, XBMCK_KP2 },
      { XKB_KEY_KP_3, XBMCK_KP3 },
      { XKB_KEY_KP_4, XBMCK_KP4 },
      { XKB_KEY_KP_5, XBMCK_KP5 },
      { XKB_KEY_KP_6, XBMCK_KP6 },
      { XKB_KEY_KP_7, XBMCK_KP7 },
      { XKB_KEY_KP_8, XBMCK_KP8 },
      { XKB_KEY_KP_9, XBMCK_KP9 },
      { XKB_KEY_KP_Decimal, XBMCK_KP_PERIOD },
      { XKB_KEY_KP_Divide, XBMCK_KP_DIVIDE },
      { XKB_KEY_KP_Multiply, XBMCK_KP_MULTIPLY },
      { XKB_KEY_KP_Add, XBMCK_KP_PLUS },
      { XKB_KEY_KP_Separator, XBMCK_KP_MINUS },
      { XKB_KEY_KP_Equal, XBMCK_KP_EQUALS },
      { XKB_KEY_F1, XBMCK_F1 },
      { XKB_KEY_F2, XBMCK_F2 },
      { XKB_KEY_F3, XBMCK_F3 },
      { XKB_KEY_F4, XBMCK_F4 },
      { XKB_KEY_F5, XBMCK_F5 },
      { XKB_KEY_F6, XBMCK_F6 },
      { XKB_KEY_F7, XBMCK_F7 },
      { XKB_KEY_F8, XBMCK_F8 },
      { XKB_KEY_F9, XBMCK_F9 },
      { XKB_KEY_F10, XBMCK_F10 },
      { XKB_KEY_F11, XBMCK_F11 },
      { XKB_KEY_F12, XBMCK_F12 },
      { XKB_KEY_F13, XBMCK_F13 },
      { XKB_KEY_F14, XBMCK_F14 },
      { XKB_KEY_F15, XBMCK_F15 },
      { XKB_KEY_Caps_Lock, XBMCK_CAPSLOCK },
      { XKB_KEY_Shift_Lock, XBMCK_SCROLLOCK },
      { XKB_KEY_Shift_R, XBMCK_RSHIFT },
      { XKB_KEY_Shift_L, XBMCK_LSHIFT },
      { XKB_KEY_Alt_R, XBMCK_RALT },
      { XKB_KEY_Alt_L, XBMCK_LALT },
      { XKB_KEY_Control_R, XBMCK_RCTRL },
      { XKB_KEY_Control_L, XBMCK_LCTRL },
      { XKB_KEY_Meta_R, XBMCK_RMETA },
      { XKB_KEY_Meta_L, XBMCK_LMETA },
      { XKB_KEY_Super_R, XBMCK_RSUPER },
      { XKB_KEY_Super_L, XBMCK_LSUPER },
      { XKB_KEY_XF86Eject, XBMCK_EJECT },
      { XKB_KEY_XF86AudioStop, XBMCK_STOP },
      { XKB_KEY_XF86AudioRecord, XBMCK_RECORD },
      { XKB_KEY_XF86AudioRewind, XBMCK_REWIND },
      { XKB_KEY_XF86AudioPlay, XBMCK_PLAY },
      { XKB_KEY_XF86AudioRandomPlay, XBMCK_SHUFFLE },
      { XKB_KEY_XF86AudioForward, XBMCK_FASTFORWARD }
    };
  
    static const size_t navigationKeySymsSize = sizeof(navigationKeySyms) /
                                                sizeof(navigationKeySyms[0]);
                                   
    for (size_t i = 0; i < navigationKeySymsSize; ++i)
    {
      if (navigationKeySyms[i].xkb == sym)
      {
        sym = navigationKeySyms[i].xbmc;
        break;
      }
    }
  }

  uint32_t keyEventType = 0;

  switch (state)
  {
    case WL_KEYBOARD_KEY_STATE_PRESSED:
      keyEventType = XBMC_KEYDOWN;
      break;
    case WL_KEYBOARD_KEY_STATE_RELEASED:
      keyEventType = XBMC_KEYUP;
      break;
    default:
      CLog::Log(LOGERROR, "%s: Unrecognized key state", __FUNCTION__);
      return;
  }
  
  if (keyEventType == XBMC_KEYDOWN)
  {
    m_repeatCallback =
      m_timeouts.RepeatAfterMs(boost::bind (
                                 &KeyboardProcessor::RepeatCallback,
                                 this,
                                 key,
                                 sym),
                               1000,
                               250);
    m_repeatSym = sym;
  }
  else if (keyEventType == XBMC_KEYUP &&
           sym == m_repeatSym)
    m_repeatCallback.reset();
  
  SendKeyToXBMC(key, sym, keyEventType);
}

void
xbmc::KeyboardProcessor::Modifier(uint32_t serial,
                                  uint32_t depressed,
                                  uint32_t latched,
                                  uint32_t locked,
                                  uint32_t group)
{
  m_keymap->UpdateMask(depressed, latched, locked, group);
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
                           xbmc::EventDispatch &dispatch,
                           xbmc::ITimeoutManager &timeouts) :
  m_clientLibrary(clientLibrary),
  m_xkbCommonLibrary(xkbCommonLibrary),
  m_pointerProcessor(dispatch, *this),
  m_keyboardProcessor(m_xkbCommonLibrary, dispatch, timeouts),
  m_seat(new xw::Seat(clientLibrary, seat, *this))
{
}

void WaylandInput::SetXBMCSurface(struct wl_surface *s)
{
  m_keyboardProcessor.SetXBMCSurface(s);
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

bool WaylandInput::InsertKeyboard(struct wl_keyboard *k)
{
  if (m_keyboard.get())
    return false;

  m_keyboard.reset(new xw::Keyboard(m_clientLibrary,
                                    k,
                                    m_keyboardProcessor));
  return true;
}

void WaylandInput::RemovePointer()
{
  m_pointer.reset();
}

void WaylandInput::RemoveKeyboard()
{
  m_keyboard.reset();
}

WaylandEventLoop::WaylandEventLoop(IDllWaylandClient &clientLibrary,
                                   struct wl_display *display) :
  m_clientLibrary(clientLibrary),
  m_display(display)
{
  m_stopWatch.StartZero();
}

namespace
{
bool TimeoutInactive(const WaylandEventLoop::CallbackTracker &tracker)
{
  return tracker.callback.expired();
}

void SubtractTimeoutAndTrigger(WaylandEventLoop::CallbackTracker &tracker,
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

bool ByRemaining(const WaylandEventLoop::CallbackTracker &a,
                 const WaylandEventLoop::CallbackTracker &b)
{
  return a.remaining < b.remaining; 
}
}

WaylandEventLoop::CallbackTracker::CallbackTracker(uint32_t time,
                                                   uint32_t initial,
                                                   const xbmc::ITimeoutManager::CallbackPtr &cb) :
  time(time),
  remaining(time > initial ? time : initial),
  callback(cb)
{
}

void WaylandEventLoop::DispatchTimers()
{
  float elapsedMs = m_stopWatch.GetElapsedMilliseconds();
  m_stopWatch.Stop();
  std::for_each(m_callbackQueue.begin(), m_callbackQueue.end (),
                boost::bind(SubtractTimeoutAndTrigger,
                            _1,
                            static_cast<int>(elapsedMs)));
  std::sort(m_callbackQueue.begin(), m_callbackQueue.end(),
            ByRemaining);
  m_stopWatch.StartZero();
}

void WaylandEventLoop::Dispatch()
{
  m_clientLibrary.wl_display_dispatch_pending(m_display);
  m_clientLibrary.wl_display_flush(m_display);

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
  
  struct pollfd pfd;
  pfd.events = POLLIN | POLLHUP | POLLERR;
  pfd.revents = 0;
  pfd.fd = m_clientLibrary.wl_display_get_fd(m_display);
  
  int pollTimeout = minTimeout == 0 ?
                    -1 : minTimeout;

  if (poll(&pfd, 1, pollTimeout) == -1)
    throw std::runtime_error(strerror(errno));

  DispatchTimers();
  m_clientLibrary.wl_display_dispatch(m_display);
}

xbmc::ITimeoutManager::CallbackPtr
WaylandEventLoop::RepeatAfterMs(const xbmc::ITimeoutManager::Callback &cb,
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
  if (!g_eventLoop.get())
    return false;

  g_eventLoop->Dispatch();

  return true;
}

void CWinEventsWayland::SetWaylandDisplay(IDllWaylandClient &clientLibrary,
                                          struct wl_display *d)
{
  g_eventLoop.reset(new WaylandEventLoop(clientLibrary, d));
}

void CWinEventsWayland::DestroyWaylandDisplay()
{
  MessagePump();

  g_eventLoop.reset();
}

void CWinEventsWayland::SetWaylandSeat(IDllWaylandClient &clientLibrary,
                                       IDllXKBCommon &xkbCommonLibrary,
                                       struct wl_seat *s)
{
  if (!g_eventLoop.get())
    throw std::logic_error("Must have a wl_display set before setting "
                           "the wl_seat in CWinEventsWayland ");

  g_inputInstance.reset(new WaylandInput(clientLibrary,
                                         xkbCommonLibrary,
                                         s,
                                         g_dispatch,
                                         *g_eventLoop));
}

void CWinEventsWayland::DestroyWaylandSeat()
{
  g_inputInstance.reset();
}

void CWinEventsWayland::SetXBMCSurface(struct wl_surface *s)
{
  if (!g_inputInstance.get())
    throw std::logic_error("Must have a wl_seat set before setting "
                           "the wl_surface in CWinEventsWayland");
  
  g_inputInstance->SetXBMCSurface(s);
}

#endif
