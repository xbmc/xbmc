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
#define WL_EGL_PLATFORM

#include <stdexcept>

#include <boost/bind.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>

#include <gtest/gtest.h>

#include <wayland-client.h>
#include <wayland-client-protocol.h>
#include <wayland-version.h>
#include "xbmc_wayland_test_client_protocol.h"
#include "windowing/DllWaylandClient.h"
#include "windowing/DllWaylandEgl.h"
#include "windowing/DllXKBCommon.h"

#include "windowing/egl/wayland/Callback.h"
#include "windowing/egl/wayland/Compositor.h"
#include "windowing/egl/wayland/Display.h"
#include "windowing/egl/wayland/OpenGLSurface.h"
#include "windowing/egl/wayland/Registry.h"
#include "windowing/egl/wayland/Surface.h"
#include "windowing/egl/wayland/Shell.h"
#include "windowing/egl/wayland/ShellSurface.h"
#include "windowing/egl/EGLNativeTypeWayland.h"
#include "windowing/wayland/EventLoop.h"
#include "windowing/wayland/EventQueueStrategy.h"
#include "windowing/wayland/TimeoutManager.h"
#include "windowing/wayland/CursorManager.h"
#include "windowing/wayland/InputFactory.h"
#include "windowing/wayland/Wayland11EventQueueStrategy.h"
#include "windowing/wayland/Wayland12EventQueueStrategy.h"

#include "input/linux/XKBCommonKeymap.h"
#include "input/XBMC_keysym.h"

#include "StubCursorManager.h"
#include "StubEventListener.h"
#include "TmpEnv.h"
#include "WestonTest.h"
#include "XBMCWayland.h"

#define WAYLAND_VERSION_NUMBER ((WAYLAND_VERSION_MAJOR << 16) | (WAYLAND_VERSION_MINOR << 8) | (WAYLAND_VERSION_MICRO))
#define WAYLAND_VERSION_CHECK(major, minor, micro) ((major << 16) | (minor << 8) | (micro))

namespace xw = xbmc::wayland;
namespace xtw = xbmc::test::wayland;
namespace xwe = xbmc::wayland::events;

namespace
{
class SingleThreadedEventQueue :
  public xwe::IEventQueueStrategy
{
public:

  SingleThreadedEventQueue(IDllWaylandClient &clientLibrary,
                           struct wl_display *display);

private:

  void PushAction(const Action &action);
  void DispatchEventsFromMain();
  
  IDllWaylandClient &m_clientLibrary;
  struct wl_display *m_display;
};

SingleThreadedEventQueue::SingleThreadedEventQueue(IDllWaylandClient &clientLibrary,
                                                   struct wl_display *display) :
  m_clientLibrary(clientLibrary),
  m_display(display)
{
}

void SingleThreadedEventQueue::PushAction(const Action &action)
{
  action();
}

void SingleThreadedEventQueue::DispatchEventsFromMain()
{
  m_clientLibrary.wl_display_dispatch_pending(m_display);
  m_clientLibrary.wl_display_flush(m_display);
  m_clientLibrary.wl_display_dispatch(m_display);
}
}

class InputEventsWestonTest :
  public WestonTest,
  public xw::IWaylandRegistration
{
public:

  InputEventsWestonTest();
  virtual void SetUp();

protected:

  DllWaylandClient clientLibrary;
  DllWaylandEGL eglLibrary;
  DllXKBCommon xkbCommonLibrary;
  
  StubCursorManager cursors;
  StubEventListener listener;

  boost::shared_ptr<struct xkb_context> xkbContext;
  boost::scoped_ptr<CXKBKeymap> keymap;

  boost::scoped_ptr<xw::Display> display;
  boost::scoped_ptr<xwe::IEventQueueStrategy> queue;
  boost::scoped_ptr<xw::Registry> registry;

  boost::scoped_ptr<xwe::Loop> loop;
  boost::scoped_ptr<xbmc::InputFactory> input;

  boost::scoped_ptr<xw::Compositor> compositor;
  boost::scoped_ptr<xw::Shell> shell;
  boost::scoped_ptr<xtw::XBMCWayland> xbmcWayland;

  boost::scoped_ptr<xw::Surface> surface;
  boost::scoped_ptr<xw::ShellSurface> shellSurface;
  boost::scoped_ptr<xw::OpenGLSurface> openGLSurface;

  virtual xwe::IEventQueueStrategy * CreateEventQueue() = 0;

  void WaitForSynchronize();
  
  static const unsigned int SurfaceWidth = 512;
  static const unsigned int SurfaceHeight = 512;

private:

  virtual bool OnGlobalInterfaceAvailable(uint32_t name,
                                          const char *interface,
                                          uint32_t version);

  bool synchronized;
  void Synchronize();
  boost::scoped_ptr<xw::Callback> syncCallback;
  
  TmpEnv m_waylandDisplayEnv;
};

InputEventsWestonTest::InputEventsWestonTest() :
  m_waylandDisplayEnv("WAYLAND_DISPLAY",
                      TempSocketName().c_str())
{
}

void InputEventsWestonTest::SetUp()
{
  WestonTest::SetUp();
  
  clientLibrary.Load();
  eglLibrary.Load();
  xkbCommonLibrary.Load();
  
  xkbContext.reset(CXKBKeymap::CreateXKBContext(xkbCommonLibrary),
                   boost::bind(&IDllXKBCommon::xkb_context_unref,
                               &xkbCommonLibrary, _1));
  keymap.reset(new CXKBKeymap(
                 xkbCommonLibrary, 
                 CXKBKeymap::CreateXKBKeymapFromNames(xkbCommonLibrary,
                                                      xkbContext.get(),
                                                      "evdev",
                                                      "pc105",
                                                      "us",
                                                      "",
                                                      "")));
  
  display.reset(new xw::Display(clientLibrary));
  queue.reset(CreateEventQueue());
  registry.reset(new xw::Registry(clientLibrary,
                                  display->GetWlDisplay(),
                                  *this));
  loop.reset(new xwe::Loop(listener, *queue));

  /* Wait for the seat, shell, compositor to appear */
  WaitForSynchronize();
  
  ASSERT_TRUE(input.get() != NULL);
  ASSERT_TRUE(compositor.get() != NULL);
  ASSERT_TRUE(shell.get() != NULL);
  ASSERT_TRUE(xbmcWayland.get() != NULL);
  
  /* Wait for input devices to appear etc */
  WaitForSynchronize();
  
  surface.reset(new xw::Surface(clientLibrary,
                                compositor->CreateSurface()));
  shellSurface.reset(new xw::ShellSurface(clientLibrary,
                                          shell->CreateShellSurface(
                                            surface->GetWlSurface())));
  openGLSurface.reset(new xw::OpenGLSurface(eglLibrary,
                                            surface->GetWlSurface(),
                                            SurfaceWidth,
                                            SurfaceHeight));

  wl_shell_surface_set_toplevel(shellSurface->GetWlShellSurface());
  surface->Commit();
}

bool InputEventsWestonTest::OnGlobalInterfaceAvailable(uint32_t name,
                                                       const char *interface,
                                                       uint32_t version)
{
  if (strcmp(interface, "wl_seat") == 0)
  {
    /* We must use the one provided by dlopen, as the address
     * may be different */
    struct wl_interface **seatInterface =
      clientLibrary.Get_wl_seat_interface();
    struct wl_seat *seat =
      registry->Bind<struct wl_seat *>(name,
                                       seatInterface,
                                       1);
    input.reset(new xbmc::InputFactory(clientLibrary,
                                       xkbCommonLibrary,
                                       seat,
                                       listener,
                                       *loop));
    return true;
  }
  else if (strcmp(interface, "wl_compositor") == 0)
  {
    struct wl_interface **compositorInterface =
      clientLibrary.Get_wl_compositor_interface();
    struct wl_compositor *wlcompositor =
      registry->Bind<struct wl_compositor *>(name,
                                             compositorInterface,
                                             1);
    compositor.reset(new xw::Compositor(clientLibrary, wlcompositor));
    return true;
  }
  else if (strcmp(interface, "wl_shell") == 0)
  {
    struct wl_interface **shellInterface =
      clientLibrary.Get_wl_shell_interface();
    struct wl_shell *wlshell =
      registry->Bind<struct wl_shell *>(name,
                                        shellInterface,
                                        1);
    shell.reset(new xw::Shell(clientLibrary, wlshell));
    return true;
  }
  else if (strcmp(interface, "xbmc_wayland") == 0)
  {
    struct wl_interface **xbmcWaylandInterface =
      (struct wl_interface **) &xbmc_wayland_interface;
    struct xbmc_wayland *wlxbmc_wayland =
      registry->Bind<struct xbmc_wayland *>(name,
                                            xbmcWaylandInterface,
                                            version);
    xbmcWayland.reset(new xtw::XBMCWayland(wlxbmc_wayland));
    return true;
  }
  
  return false;
}

void InputEventsWestonTest::WaitForSynchronize()
{
  synchronized = false;
  syncCallback.reset(new xw::Callback(clientLibrary,
                                      display->Sync(),
                                      boost::bind(&InputEventsWestonTest::Synchronize,
                                                  this)));
  
  while (!synchronized)
    loop->Dispatch();
}

void InputEventsWestonTest::Synchronize()
{
  synchronized = true;
}

template <typename EventQueue>
class InputEventQueueWestonTest :
  public InputEventsWestonTest
{
private:

  virtual xwe::IEventQueueStrategy * CreateEventQueue()
  {
    return new EventQueue(clientLibrary, display->GetWlDisplay());
  }
};
TYPED_TEST_CASE_P(InputEventQueueWestonTest);

TYPED_TEST_P(InputEventQueueWestonTest, Construction)
{
}

TYPED_TEST_P(InputEventQueueWestonTest, MotionEvent)
{
  typedef InputEventsWestonTest Base;
  int x = Base::SurfaceWidth / 2;
  int y = Base::SurfaceHeight / 2;
  Base::xbmcWayland->MovePointerTo(Base::surface->GetWlSurface(),
                                   wl_fixed_from_int(x),
                                   wl_fixed_from_int(y));
  Base::WaitForSynchronize();
  XBMC_Event event(Base::listener.FetchLastEvent());

  EXPECT_EQ(XBMC_MOUSEMOTION, event.type);
  EXPECT_EQ(x, event.motion.xrel);
  EXPECT_EQ(y, event.motion.yrel);
}

TYPED_TEST_P(InputEventQueueWestonTest, ButtonEvent)
{
  typedef InputEventsWestonTest Base;
  int x = Base::SurfaceWidth / 2;
  int y = Base::SurfaceHeight / 2;
  const unsigned int WaylandLeftButton = 272;
  
  Base::xbmcWayland->MovePointerTo(Base::surface->GetWlSurface(),
                                   wl_fixed_from_int(x),
                                   wl_fixed_from_int(y));
  Base::xbmcWayland->SendButtonTo(Base::surface->GetWlSurface(),
                                  WaylandLeftButton,
                                  WL_POINTER_BUTTON_STATE_PRESSED);
  Base::WaitForSynchronize();
  
  /* Throw away motion event */
  Base::listener.FetchLastEvent();

  XBMC_Event event(Base::listener.FetchLastEvent());

  EXPECT_EQ(XBMC_MOUSEBUTTONDOWN, event.type);
  EXPECT_EQ(1, event.button.button);
  EXPECT_EQ(x, event.button.x);
  EXPECT_EQ(y, event.button.y);
}

TYPED_TEST_P(InputEventQueueWestonTest, AxisEvent)
{
  typedef InputEventsWestonTest Base;
  int x = Base::SurfaceWidth / 2;
  int y = Base::SurfaceHeight / 2;
  
  Base::xbmcWayland->MovePointerTo(Base::surface->GetWlSurface(),
                                   wl_fixed_from_int(x),
                                   wl_fixed_from_int(y));
  Base::xbmcWayland->SendAxisTo(Base::surface->GetWlSurface(),
                                WL_POINTER_AXIS_VERTICAL_SCROLL,
                                wl_fixed_from_int(10));
  Base::WaitForSynchronize();
  
  /* Throw away motion event */
  Base::listener.FetchLastEvent();

  /* Should get button up and down */
  XBMC_Event event(Base::listener.FetchLastEvent());

  EXPECT_EQ(XBMC_MOUSEBUTTONDOWN, event.type);
  EXPECT_EQ(5, event.button.button);
  EXPECT_EQ(x, event.button.x);
  EXPECT_EQ(y, event.button.y);
  
  event = Base::listener.FetchLastEvent();

  EXPECT_EQ(XBMC_MOUSEBUTTONUP, event.type);
  EXPECT_EQ(5, event.button.button);
  EXPECT_EQ(x, event.button.x);
  EXPECT_EQ(y, event.button.y);
}

namespace
{
/* Brute-force lookup functions to compensate for the fact that
 * Keymap interface only supports conversion from scancodes
 * to keysyms and not vice-versa (as such is not implemented in
 * xkbcommon)
 */
uint32_t LookupKeycodeForKeysym(ILinuxKeymap &keymap,
                                XBMCKey sym)
{
  uint32_t code = 0;
  
  while (code < XKB_KEYCODE_MAX)
  {
    /* Supress exceptions from unsupported keycodes */
    try
    {
      if (keymap.XBMCKeysymForKeycode(code) == sym)
        return code;
    }
    catch (std::runtime_error &err)
    {
    }
    
    ++code;
  }
  
  throw std::logic_error("Keysym has no corresponding keycode");
}

uint32_t LookupModifierIndexForModifier(ILinuxKeymap &keymap,
                                        XBMCMod modifier)
{
  uint32_t maxIndex = std::numeric_limits<uint32_t>::max();
  uint32_t index = 0;
  
  while (index < maxIndex)
  {
    keymap.UpdateMask(1 << index, 0, 0, 0);
    XBMCMod mask = static_cast<XBMCMod>(keymap.ActiveXBMCModifiers());
    keymap.UpdateMask(0, 0, 0, 0);
    if (mask & modifier)
      return index;
    
    ++index;
  }
  
  throw std::logic_error("Modifier has no corresponding keymod index");
}
}

TYPED_TEST_P(InputEventQueueWestonTest, KeyEvent)
{
  typedef InputEventsWestonTest Base;
  
  const unsigned int oKeycode = LookupKeycodeForKeysym(*Base::keymap,
                                                       XBMCK_o);

  Base::xbmcWayland->GiveSurfaceKeyboardFocus(Base::surface->GetWlSurface());
  Base::xbmcWayland->SendKeyToKeyboard(Base::surface->GetWlSurface(),
                                       oKeycode,
                                       WL_KEYBOARD_KEY_STATE_PRESSED);
  Base::WaitForSynchronize();
  
  XBMC_Event event(Base::listener.FetchLastEvent());
  EXPECT_EQ(XBMC_KEYDOWN, event.type);
  EXPECT_EQ(oKeycode, event.key.keysym.scancode);
  EXPECT_EQ(XBMCK_o, event.key.keysym.sym);
  EXPECT_EQ(XBMCK_o, event.key.keysym.unicode);
}

TYPED_TEST_P(InputEventQueueWestonTest, RepeatAfter1000Ms)
{
  typedef InputEventsWestonTest Base;
  
  const unsigned int oKeycode = LookupKeycodeForKeysym(*Base::keymap,
                                                       XBMCK_o);

  Base::xbmcWayland->GiveSurfaceKeyboardFocus(Base::surface->GetWlSurface());
  Base::xbmcWayland->SendKeyToKeyboard(Base::surface->GetWlSurface(),
                                       oKeycode,
                                       WL_KEYBOARD_KEY_STATE_PRESSED);
  Base::WaitForSynchronize();
  ::usleep(1100000); // 1100ms
  Base::xbmcWayland->SendKeyToKeyboard(Base::surface->GetWlSurface(),
                                       oKeycode,
                                       WL_KEYBOARD_KEY_STATE_RELEASED);
  Base::WaitForSynchronize();
  
  /* Throw away first key down */
  XBMC_Event event(Base::listener.FetchLastEvent());
  
  /* Synthetic key up should be generated */
  event = Base::listener.FetchLastEvent();
  EXPECT_EQ(XBMC_KEYUP, event.type);
  EXPECT_EQ(oKeycode, event.key.keysym.scancode);
  
  /* Synthetic key down should be generated */
  event = Base::listener.FetchLastEvent();
  EXPECT_EQ(XBMC_KEYDOWN, event.type);
  EXPECT_EQ(oKeycode, event.key.keysym.scancode);
}

TYPED_TEST_P(InputEventQueueWestonTest, NoRepeatAfterRelease)
{
  typedef InputEventsWestonTest Base;
  
  const unsigned int oKeycode = LookupKeycodeForKeysym(*Base::keymap,
                                                       XBMCK_o);

  Base::xbmcWayland->GiveSurfaceKeyboardFocus(Base::surface->GetWlSurface());
  Base::xbmcWayland->SendKeyToKeyboard(Base::surface->GetWlSurface(),
                                       oKeycode,
                                       WL_KEYBOARD_KEY_STATE_PRESSED);
  Base::WaitForSynchronize();
  ::usleep(1100000); // 1100ms
  Base::xbmcWayland->SendKeyToKeyboard(Base::surface->GetWlSurface(),
                                       oKeycode,
                                       WL_KEYBOARD_KEY_STATE_RELEASED);
  Base::WaitForSynchronize();

  /* Drain any residual events */
  bool eventsPending = true;
  while (eventsPending)
  {
    try
    {
      Base::listener.FetchLastEvent();
    }
    catch (std::logic_error &err)
    {
      eventsPending = false;
    }
  }
  
  /* Sleep-wait again */
  ::usleep(1100000); // 1100ms
  Base::WaitForSynchronize();
  
  /* Should not be any more events */
  EXPECT_THROW({ Base::listener.FetchLastEvent(); }, std::logic_error);
}

TYPED_TEST_P(InputEventQueueWestonTest, Modifiers)
{
  typedef InputEventsWestonTest Base;
  
  const unsigned int oKeycode = LookupKeycodeForKeysym(*Base::keymap,
                                                       XBMCK_o);
  const unsigned int leftShiftIndex =
    LookupModifierIndexForModifier(*Base::keymap, XBMCKMOD_LSHIFT);

  Base::xbmcWayland->GiveSurfaceKeyboardFocus(Base::surface->GetWlSurface());
  Base::xbmcWayland->SendModifiersToKeyboard(Base::surface->GetWlSurface(),
                                             1 << leftShiftIndex,
                                             0,
                                             0,
                                             0);
  Base::xbmcWayland->SendKeyToKeyboard(Base::surface->GetWlSurface(),
                                       oKeycode,
                                       WL_KEYBOARD_KEY_STATE_PRESSED);
  Base::WaitForSynchronize();
  
  XBMC_Event event(Base::listener.FetchLastEvent());
  EXPECT_EQ(XBMC_KEYDOWN, event.type);
  EXPECT_EQ(oKeycode, event.key.keysym.scancode);
  EXPECT_TRUE((XBMCKMOD_LSHIFT & event.key.keysym.mod) != 0);
}

REGISTER_TYPED_TEST_CASE_P(InputEventQueueWestonTest,
                           Construction,
                           MotionEvent,
                           ButtonEvent,
                           AxisEvent,
                           KeyEvent,
                           RepeatAfter1000Ms,
                           NoRepeatAfterRelease,
                           Modifiers);

typedef ::testing::Types<SingleThreadedEventQueue,
#if (WAYLAND_VERSION_NUMBER >= WAYLAND_VERSION_CHECK(1, 1, 90))
                         xw::version_11::EventQueueStrategy,
                         xw::version_12::EventQueueStrategy> EventQueueTypes;
#else
                         xw::version_11::EventQueueStrategy> EventQueueTypes;
#endif

INSTANTIATE_TYPED_TEST_CASE_P(EventQueues,
                              InputEventQueueWestonTest,
                              EventQueueTypes);
