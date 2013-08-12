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

#include "wayland/Seat.h"
#include "wayland/Pointer.h"
#include "wayland/Keyboard.h"
#include "wayland/EventQueueStrategy.h"

#include "input/linux/XKBCommonKeymap.h"
#include "input/linux/Keymap.h"

namespace xbmc
{
/* ITimeoutManager defines an interface for arbitary classes
 * to register full closures to be called initially on a timeout
 * specified by initial and then subsequently on a timeout specified
 * by timeout. The interface is more or less artificial and exists to
 * break the dependency between keyboard processing code and
 * actual system timers, whcih is useful for testing purposes */
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

/* IEventListener defines an interface for WaylandInput to deliver
 * simple events to. This interface is more or less artificial and used
 * to break the dependency between the event transformation code
 * and the rest of the system for testing purposes */
class IEventListener
{
public:

  virtual ~IEventListener() {}
  virtual void OnEvent(XBMC_Event &) = 0;
  virtual void OnFocused() = 0;
  virtual void OnUnfocused() = 0;
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

/* KeyboardProcessor implements IKeyboardReceiver and transforms
 * keyboard events into XBMC events for further processing.
 * 
 * It needs to know whether or not a surface is in focus, so as soon
 * as a surface is available, SetXBMCSurface should be called.
 * 
 * KeyboardProcessor also performs key-repeat and registers a callback
 * function to repeat the currently depressed key if it has not been
 * released within a certain period. As such it depends on
 * ITimeoutManager */
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

  /* KeyboardProcessor owns a keymap and does parts of its processing
   * by delegating to the keymap the job of looking up generic keysyms
   * for keycodes */
  boost::scoped_ptr<ILinuxKeymap> m_keymap;
  IEventListener &m_listener;
  ITimeoutManager &m_timeouts;
  struct wl_surface *m_xbmcWindow;
  
  ITimeoutManager::CallbackPtr m_repeatCallback;
  uint32_t m_repeatSym;
  
  struct xkb_context *m_context;
};
}

namespace xw = xbmc::wayland;
namespace xwe = xbmc::wayland::events;

namespace
{
/* WaylandEventLoop encapsulates the entire process of dispatching
 * wayland events and timers that might be in place for duplicate
 * processing. Calling its Dispatch() method will cause any pending
 * timers and events to be dispatched. It implements ITimeoutManager
 * and timeouts can be added directly to it */
class WaylandEventLoop :
  public xbmc::IEventListener,
  public xbmc::ITimeoutManager
{
public:

  WaylandEventLoop(IDllWaylandClient &clientLibrary,
                   xwe::IEventQueueStrategy &strategy,
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
  
  void OnEvent(XBMC_Event &);
  void OnFocused();
  void OnUnfocused();
  
  IDllWaylandClient &m_clientLibrary;
  
  struct wl_display *m_display;
  std::vector<CallbackTracker> m_callbackQueue;
  CStopWatch m_stopWatch;
  
  xwe::IEventQueueStrategy &m_eventQueue;
};

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
               xbmc::IEventListener &dispatch,
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

  IDllWaylandClient &m_clientLibrary;
  IDllXKBCommon &m_xkbCommonLibrary;

  xbmc::PointerProcessor m_pointerProcessor;
  xbmc::KeyboardProcessor m_keyboardProcessor;

  boost::scoped_ptr<xw::Seat> m_seat;
  boost::scoped_ptr<xw::Pointer> m_pointer;
  boost::scoped_ptr<xw::Keyboard> m_keyboard;
};

boost::scoped_ptr <WaylandInput> g_inputInstance;
boost::scoped_ptr <WaylandEventLoop> g_eventLoop;
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

  /* KeyboardProcessor and not XKBKeymap owns the xkb_context. The
   * xkb_context is merely just a detail for construction of the
   * more interesting xkb_state and xkb_keymap objects.
   * 
   * Failure to create the context effectively means that we will
   * be unable to create a keymap or serve any useful purpose in
   * processing key events. As such, it makes this an incomplete
   * object and a runtime_error will be thrown */
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

/* Creates a new internal keymap representation for a serialized
 * keymap as represented in shared memory as referred to by fd.
 * 
 * Since the fd is sent to us via sendmsg(), the currently running
 * process has ownership over it. As such, it MUST close the file
 * descriptor after it has decided what to do with it in order to
 * avoid a leak.
 */
void
xbmc::KeyboardProcessor::UpdateKeymap(uint32_t format,
                                      int fd,
                                      uint32_t size)
{
  /* The file descriptor must always be closed */
  BOOST_SCOPE_EXIT((fd))
  {
    close(fd);
  } BOOST_SCOPE_EXIT_END

  /* We don't understand anything other than xkbv1. If we get some
   * other keyboard, then we can't process keyboard events reliably
   * and that's a runtime error. */
  if (format != WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1)
    throw std::runtime_error("Server gave us a keymap we don't understand");

  bool successfullyCreatedKeyboard = false;
  
  /* Either throws or returns a valid struct xkb_keymap * */
  struct xkb_keymap *keymap =
    CXKBKeymap::ReceiveXKBKeymapFromSharedMemory(m_xkbCommonLibrary, m_context, fd, size);

  BOOST_SCOPE_EXIT((&m_xkbCommonLibrary)(&successfullyCreatedKeyboard)(keymap))
  {
    if (!successfullyCreatedKeyboard)
      m_xkbCommonLibrary.xkb_keymap_unref(keymap);
  } BOOST_SCOPE_EXIT_END

  struct xkb_state *state =
    CXKBKeymap::CreateXKBStateFromKeymap(m_xkbCommonLibrary, keymap);

  m_keymap.reset(new CXKBKeymap(m_xkbCommonLibrary,
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
  event.key.keysym.mod =
    static_cast<XBMCMod>(m_keymap->ActiveXBMCModifiers());
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

/* If this function is called before a keymap is set, then that
 * is a precondition violation and a logic_error results */
void
xbmc::KeyboardProcessor::Key(uint32_t serial,
                             uint32_t time,
                             uint32_t key,
                             enum wl_keyboard_key_state state)
{
  if (!m_keymap.get())
    throw std::logic_error("a keymap must be set before processing key events");

  uint32_t sym = XKB_KEY_NoSymbol;

  /* If we're unable to process a single key, then catch the error
   * and report it, but don't allow it to be fatal */
  try
  {
    sym = m_keymap->XBMCKeysymForKeycode(key);
  }
  catch (const std::runtime_error &err)
  {
    CLog::Log(LOGERROR, "%s: Failed to process keycode %i: %s",
              __FUNCTION__, key, err.what());
    return;
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
  
  /* Key-repeat is handled on the client side so we need to add a new
   * timeout here to repeat this symbol if it is still being held down
   */
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

/* We MUST update the keymap mask whenever we receive a new modifier
 * event */
void
xbmc::KeyboardProcessor::Modifier(uint32_t serial,
                                  uint32_t depressed,
                                  uint32_t latched,
                                  uint32_t locked,
                                  uint32_t group)
{
  m_keymap->UpdateMask(depressed, latched, locked, group);
}

<<<<<<< HEAD
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

=======
>>>>>>> c5aa463... Read or dispatch events in a separate thread.
WaylandInput::WaylandInput(IDllWaylandClient &clientLibrary,
                           IDllXKBCommon &xkbCommonLibrary,
                           struct wl_seat *seat,
                           xbmc::IEventListener &dispatch,
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

namespace
{
void DispatchEventAction(XBMC_Event &e)
{
  g_application.OnEvent(e);
}

void DispatchFocusedAction()
{
  g_application.m_AppFocused = true;
  g_Windowing.NotifyAppFocusChange(g_application.m_AppFocused);
}

void DispatchUnfocusedAction()
{
  g_application.m_AppFocused = false;
  g_Windowing.NotifyAppFocusChange(g_application.m_AppFocused);
}
}

/* Once WaylandEventLoop recieves some information we need to enqueue
 * it to be dispatched on MessagePump. This is done by using
 * a command pattern to wrap the incoming data in function objects
 * and then pushing it to a queue.
 * 
 * The reason for this is that these three functions may or may not
 * be running in a separate thread depending on the dispatch
 * strategy in place.  */
void WaylandEventLoop::OnEvent(XBMC_Event &e)
{
  m_eventQueue.PushAction(boost::bind(DispatchEventAction, e));
}

void WaylandEventLoop::OnFocused()
{
  m_eventQueue.PushAction(boost::bind(DispatchFocusedAction));
}

void WaylandEventLoop::OnUnfocused()
{
  m_eventQueue.PushAction(boost::bind(DispatchUnfocusedAction));
}

WaylandEventLoop::WaylandEventLoop(IDllWaylandClient &clientLibrary,
                                   xwe::IEventQueueStrategy &strategy,
                                   struct wl_display *display) :
  m_clientLibrary(clientLibrary),
  m_display(display),
  m_eventQueue(strategy)
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
  
  m_eventQueue.DispatchEventsFromMain();
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

void CWinEventsWayland::SetWaylandDisplay(IDllWaylandClient &clientLibrary,
                                          xwe::IEventQueueStrategy &strategy,
                                          struct wl_display *d)
{
  g_eventLoop.reset(new WaylandEventLoop(clientLibrary, strategy, d));
}

void CWinEventsWayland::DestroyWaylandDisplay()
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

  g_inputInstance.reset(new WaylandInput(clientLibrary,
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
