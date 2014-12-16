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

#include <sstream>
#include <vector>

#include <boost/scoped_ptr.hpp>
#include <boost/noncopyable.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>

#include <unistd.h>
#include <signal.h>
#include <errno.h>

extern "C"
{
/* Work around usage of a reserved keyword
 * https://bugs.freedesktop.org/show_bug.cgi?id=63485 */
#define private cprivate
#include <weston/compositor.h>
#undef private
#include <wayland-server.h>
#include "xbmc_wayland_test_server_protocol.h"
}

namespace xbmc
{
namespace test
{
namespace wayland
{
class Listener :
  boost::noncopyable
{
public:

  typedef boost::function<void()> Delegate;

  Listener(const Delegate &);
  void BindTo(struct wl_signal *);

private:

  static void MainCallback(struct wl_listener *listener, void *data);

  void Callback();

  struct wl_listener m_listener;
  Delegate m_delegate;
};
}

namespace weston
{
class Compositor :
  boost::noncopyable
{
public:

  Compositor(struct weston_compositor *);
  ~Compositor();

  struct wl_display * Display();

  struct weston_surface * TopSurface();
  struct weston_mode * LastMode();
  void OnEachMode(const boost::function<void(struct weston_mode *)> &);
  struct wl_resource * PointerResource(struct wl_client *client);
  struct wl_resource * KeyboardResource(struct wl_client *client);
  struct weston_surface * Surface(struct wl_resource *client);
  struct weston_keyboard * Keyboard();

private:

  static void Unload(Compositor *compositor);
  static int Ready(void *);

  struct weston_compositor *m_compositor;
  struct wl_event_source *m_readySource;
  wayland::Listener m_destroyListener;
  struct weston_seat * Seat();
  struct weston_output * FirstOutput();
};
}

namespace wayland
{
class XBMCWayland :
  boost::noncopyable
{
public:

  ~XBMCWayland();
  
  struct wl_resource * GetResource();

  /* Effectively a factory function for XBMCWayland. Creates an
   * instantiation for a client */
  static void BindToClient(struct wl_client *client,
                           void *data,
                           uint32_t version,
                           uint32_t id);

private:

  /* Constructor is private as this object may only be constructed
   * by a client binding to its interface */
  XBMCWayland(struct wl_client *client,
              uint32_t id,
              weston::Compositor &compositor);

  static void UnbindFromClientCallback(struct wl_resource *);

  static const struct xbmc_wayland_interface m_listener;
  
  static void AddModeCallback(struct wl_client *,
                              struct wl_resource *,
                              int32_t,
                              int32_t,
                              uint32_t,
                              uint32_t);
  static void MovePointerToOnSurfaceCallback(struct wl_client *,
                                             struct wl_resource *,
                                             struct wl_resource *,
                                             wl_fixed_t x,
                                             wl_fixed_t y);
  static void SendButtonToSurfaceCallback(struct wl_client *,
                                          struct wl_resource *,
                                          struct wl_resource *,
                                          uint32_t,
                                          uint32_t);
  static void SendAxisToSurfaceCallback(struct wl_client *,
                                        struct wl_resource *,
                                        struct wl_resource *,
                                        uint32_t,
                                        wl_fixed_t);
  static void SendKeyToKeyboardCallback(struct wl_client *,
                                        struct wl_resource *,
                                        struct wl_resource *,
                                        uint32_t,
                                        uint32_t);
  static void SendModifiersToKeyboardCallback(struct wl_client *,
                                              struct wl_resource *,
                                              struct wl_resource *,
                                              uint32_t,
                                              uint32_t,
                                              uint32_t,
                                              uint32_t);
  static void GiveSurfaceKeyboardFocusCallback(struct wl_client *,
                                               struct wl_resource *,
                                               struct wl_resource *);
  static void PingSurfaceCallback(struct wl_client *,
                                  struct wl_resource *,
                                  struct wl_resource *,
                                  uint32_t timestamp);

  void AddMode(struct wl_client *client,
               struct wl_resource *resource,
               int32_t width,
               int32_t height,
               uint32_t refresh,
               uint32_t flags);
  void MovePointerToOnSurface(struct wl_client *client,
                              struct wl_resource *resource,
                              struct wl_resource *surface,
                              wl_fixed_t x,
                              wl_fixed_t y);
  void SendButtonToSurface(struct wl_client *client,
                           struct wl_resource *resource,
                           struct wl_resource *surface,
                           uint32_t button,
                           uint32_t state);
  void SendAxisToSurface(struct wl_client *client,
                         struct wl_resource *resource,
                         struct wl_resource *surface,
                         uint32_t axis,
                         wl_fixed_t value);
  void SendKeyToKeyboard(struct wl_client *client,
                         struct wl_resource *resource,
                         struct wl_resource *surfaceResource,
                         uint32_t key,
                         uint32_t state);
  void SendModifiersToKeyboard(struct wl_client *client,
                               struct wl_resource *resource,
                               struct wl_resource *surfaceResource,
                               uint32_t depressed,
                               uint32_t latched,
                               uint32_t locked,
                               uint32_t group);
  void GiveSurfaceKeyboardFocus(struct wl_client *clent,
                                struct wl_resource *resource,
                                struct wl_resource *surfaceResource);
  void PingSurface(struct wl_client *client,
                   struct wl_resource *resource,
                   struct wl_resource *surfaceResource,
                   uint32_t timestamp);

  weston::Compositor &m_compositor;
  struct wl_resource *m_clientXBMCWaylandResource;
  
  std::vector<struct weston_mode> m_additionalModes;
};

const struct xbmc_wayland_interface XBMCWayland::m_listener =
{
  XBMCWayland::AddModeCallback,
  XBMCWayland::MovePointerToOnSurfaceCallback,
  XBMCWayland::SendButtonToSurfaceCallback,
  XBMCWayland::SendAxisToSurfaceCallback,
  XBMCWayland::SendKeyToKeyboardCallback,
  XBMCWayland::SendModifiersToKeyboardCallback,
  XBMCWayland::GiveSurfaceKeyboardFocusCallback,
  XBMCWayland::PingSurfaceCallback
};
}
}
}

namespace xtw = xbmc::test::wayland;
namespace xtwc = xbmc::test::weston;

xtw::XBMCWayland::XBMCWayland(struct wl_client *client,
                              uint32_t id,
                              xtwc::Compositor &compositor) :
  m_compositor(compositor)
{
  m_clientXBMCWaylandResource =
    static_cast<struct wl_resource *>(wl_resource_create(client,
                                                         &xbmc_wayland_interface,
                                                         1,
                                                         id));
  wl_resource_set_implementation (m_clientXBMCWaylandResource,
                                  &m_listener,
                                  this,
                                  XBMCWayland::UnbindFromClientCallback);
}

xtw::XBMCWayland::~XBMCWayland()
{
  /* Remove all but the first output if we added any */
  for (std::vector<struct weston_mode>::iterator it = m_additionalModes.begin();
       it != m_additionalModes.end();
       ++it)
  {
    wl_list_remove(&it->link);
  }
}

void
xtw::XBMCWayland::UnbindFromClientCallback(struct wl_resource *r)
{
  delete static_cast<XBMCWayland *>(wl_resource_get_user_data(r));
}

void
xtw::XBMCWayland::BindToClient(struct wl_client *client,
                               void *data,
                               uint32_t version,
                               uint32_t id)
{
  xtwc::Compositor *compositor = static_cast<xtwc::Compositor *>(data);
  
  /* This looks funky - however the constructor will handle registering
   * the destructor function with wl_registry so that it gets destroyed
   * at the right time */
  new XBMCWayland(client, id, *compositor);
}

void
xtw::XBMCWayland::AddModeCallback(struct wl_client *c,
                                  struct wl_resource *r,
                                  int32_t w,
                                  int32_t h,
                                  uint32_t re,
                                  uint32_t f)
{
  static_cast<XBMCWayland *>(wl_resource_get_user_data(r))->AddMode(c, r, w, h, re, f);
}

void
xtw::XBMCWayland::MovePointerToOnSurfaceCallback(struct wl_client *c,
                                                 struct wl_resource *r,
                                                 struct wl_resource *s,
                                                 wl_fixed_t x,
                                                 wl_fixed_t y)
{
  static_cast<XBMCWayland *>(wl_resource_get_user_data(r))->MovePointerToOnSurface(c, r, s, x, y);
}

void
xtw::XBMCWayland::SendButtonToSurfaceCallback(struct wl_client *c,
                                              struct wl_resource *r,
                                              struct wl_resource *s,
                                              uint32_t b,
                                              uint32_t st)
{
  static_cast<XBMCWayland *>(wl_resource_get_user_data(r))->SendButtonToSurface(c, r, s, b, st);
}

void
xtw::XBMCWayland::SendAxisToSurfaceCallback(struct wl_client *c,
                                            struct wl_resource *r,
                                            struct wl_resource *s,
                                            uint32_t b,
                                            wl_fixed_t v)
{
  static_cast<XBMCWayland *>(wl_resource_get_user_data(r))->SendAxisToSurface(c, r, s, b, v);
}

void
xtw::XBMCWayland::SendModifiersToKeyboardCallback(struct wl_client *c,
                                                  struct wl_resource *r,
                                                  struct wl_resource *s,
                                                  uint32_t d,
                                                  uint32_t la,
                                                  uint32_t lo,
                                                  uint32_t g)
{
  static_cast<XBMCWayland *>(wl_resource_get_user_data(r))->SendModifiersToKeyboard(c, r, s, d, la, lo, g);
}

void
xtw::XBMCWayland::SendKeyToKeyboardCallback(struct wl_client *c,
                                            struct wl_resource *r,
                                            struct wl_resource *s,
                                            uint32_t k,
                                            uint32_t st)
{
  static_cast<XBMCWayland *>(wl_resource_get_user_data(r))->SendKeyToKeyboard(c, r, s, k, st);
}

void
xtw::XBMCWayland::GiveSurfaceKeyboardFocusCallback(struct wl_client *c,
                                                   struct wl_resource *r,
                                                   struct wl_resource *s)
{
  static_cast<XBMCWayland *>(wl_resource_get_user_data(r))->GiveSurfaceKeyboardFocus(c, r, s);
}

void
xtw::XBMCWayland::PingSurfaceCallback(struct wl_client *c,
                                      struct wl_resource *r,
                                      struct wl_resource *s,
                                      uint32_t t)
{
  static_cast<XBMCWayland *>(wl_resource_get_user_data(r))->PingSurface(c, r, s, t);
}

namespace
{
void ClearFlagsOnOtherModes(struct weston_mode *mode,
                            uint32_t flags,
                            struct weston_mode *skip)
{
  if (mode == skip)
    return;
  
  mode->flags &= ~flags;
}
}

void
xtw::XBMCWayland::AddMode(struct wl_client *client,
                          struct wl_resource *resource,
                          int32_t width,
                          int32_t height,
                          uint32_t refresh,
                          uint32_t flags)
{
  const struct weston_mode mode =
  {
    flags,
    width,
    height,
    refresh
  };
  
  m_additionalModes.push_back(mode);
  struct weston_mode *lastMode = m_compositor.LastMode();
  wl_list_insert(&lastMode->link, &m_additionalModes.back().link);
  
  /* Clear flags from all other outputs that may have the same flags
   * as this one */
  m_compositor.OnEachMode(boost::bind(ClearFlagsOnOtherModes,
                                      _1,
                                      flags,
                                      &m_additionalModes.back()));
}

namespace
{
void GetSerialAndTime(struct wl_display *display,
                      uint32_t &serial,
                      uint32_t &time)
{
  serial = wl_display_next_serial(display);
  
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  
  time = ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}
}

void
xtw::XBMCWayland::MovePointerToOnSurface(struct wl_client *client,
                                         struct wl_resource *resource,
                                         struct wl_resource *surface,
                                         wl_fixed_t x,
                                         wl_fixed_t y)
{
  struct wl_client *surfaceClient = wl_resource_get_client(surface);
  struct wl_resource *pointer = m_compositor.PointerResource(surfaceClient);
  struct wl_display *display = wl_client_get_display(surfaceClient);
  uint32_t serial, time;

  GetSerialAndTime(display, serial, time);
  
  wl_pointer_send_motion(pointer, time, x, y);
}

void
xtw::XBMCWayland::SendButtonToSurface(struct wl_client *client,
                                      struct wl_resource *resource,
                                      struct wl_resource *surface,
                                      uint32_t button,
                                      uint32_t state)
{
  struct wl_client *surfaceClient = wl_resource_get_client(surface);
  struct wl_resource *pointer = m_compositor.PointerResource(surfaceClient);
  struct wl_display *display = wl_client_get_display(surfaceClient);
  uint32_t serial, time;
  
  GetSerialAndTime(display, serial, time);
  
  wl_pointer_send_button(pointer, serial, time, button, state);
}

void
xtw::XBMCWayland::SendAxisToSurface(struct wl_client *client,
                                    struct wl_resource *resource,
                                    struct wl_resource *surface,
                                    uint32_t axis,
                                    wl_fixed_t value)
{
  struct wl_client *surfaceClient = wl_resource_get_client(surface);
  struct wl_resource *pointer = m_compositor.PointerResource(surfaceClient);
  struct wl_display *display = wl_client_get_display(surfaceClient);
  uint32_t serial, time;
  
  GetSerialAndTime(display, serial, time);
  
  wl_pointer_send_axis(pointer, time, axis, value);
}

void
xtw::XBMCWayland::SendKeyToKeyboard(struct wl_client *client,
                                    struct wl_resource *resource,
                                    struct wl_resource *surface,
                                    uint32_t key,
                                    uint32_t state)
{
  struct wl_client *surfaceClient = wl_resource_get_client(surface);
  struct wl_resource *keyboard = m_compositor.KeyboardResource(surfaceClient);
  struct wl_display *display = wl_client_get_display(surfaceClient);
  uint32_t serial, time;

  GetSerialAndTime(display, serial, time);
  
  wl_keyboard_send_key(keyboard, serial, time, key, state);
}

void
xtw::XBMCWayland::SendModifiersToKeyboard(struct wl_client *client,
                                          struct wl_resource *resource,
                                          struct wl_resource *surface,
                                          uint32_t depressed,
                                          uint32_t latched,
                                          uint32_t locked,
                                          uint32_t group)
{
  struct wl_client *surfaceClient = wl_resource_get_client(surface);
  struct wl_resource *keyboard = m_compositor.KeyboardResource(surfaceClient);
  struct wl_display *display = wl_client_get_display(surfaceClient);
  uint32_t serial = wl_display_next_serial(display);
  
  wl_keyboard_send_modifiers(keyboard,
                             serial,
                             depressed,
                             latched,
                             locked,
                             group);
}

void
xtw::XBMCWayland::GiveSurfaceKeyboardFocus(struct wl_client *client,
                                           struct wl_resource *resource,
                                           struct wl_resource *surface)
{
  struct weston_surface *westonSurface = m_compositor.Surface(surface);
  struct weston_keyboard *keyboard = m_compositor.Keyboard();
  weston_keyboard_set_focus(keyboard, westonSurface);
}

void
xtw::XBMCWayland::PingSurface(struct wl_client *client,
                              struct wl_resource *resource,
                              struct wl_resource *surface,
                              uint32_t timestamp)
{
}

xtw::Listener::Listener(const Delegate &delegate) :
  m_delegate(delegate)
{
  m_listener.notify = Listener::MainCallback;
}

void
xtw::Listener::MainCallback(struct wl_listener *listener, void *data)
{
  static_cast<Listener *>(data)->Callback();
}

void
xtw::Listener::Callback()
{
  m_delegate();
}

void
xtw::Listener::BindTo(struct wl_signal *s)
{
  wl_signal_add(s, &m_listener);
}

xtwc::Compositor::Compositor(struct weston_compositor *c) :
  m_compositor(c),
  /* This is a workaround for a race condition where the registry
   * might not be ready if we send SIGUSR2 right away - so we
   * put it on the event loop to happen after the first poll() */
  m_readySource(wl_event_loop_add_timer(wl_display_get_event_loop(Display()),
                                        Compositor::Ready,
                                        this)),
  m_destroyListener(boost::bind(Compositor::Unload, this))
{
  /* Dispatch ASAP */
  wl_event_source_timer_update(m_readySource, 1);
  m_destroyListener.BindTo(&c->destroy_signal);
  
  /* The parent process should have set the SIGUSR2 handler to
   * SIG_IGN, throw if it hasn't */
  if (signal(SIGUSR2, SIG_IGN) != SIG_IGN)
  {
    std::stringstream ss;
    throw std::runtime_error("Parent process is not handling SIGUSR2");
  }
}

int
xtwc::Compositor::Ready(void *data)
{
  xtwc::Compositor *compositor = static_cast<xtwc::Compositor *>(data);

  if (kill(getppid(), SIGUSR2) == -1)
  {
    std::stringstream ss;
    ss << "kill: "
       << strerror(errno);
    throw std::runtime_error(ss.str());
  }
  
  wl_event_source_remove(compositor->m_readySource);
  compositor->m_readySource = NULL;
  
  /* Initialize the fake keyboard and pointer on our seat. This is
   * effectively manipulating the backend into doing something it
   * shouldn't, but we're in control here */
  struct weston_seat *seat = compositor->Seat();
  weston_seat_init_pointer(seat);
  
  struct xkb_keymap *keymap =
    xkb_keymap_new_from_names(compositor->m_compositor->xkb_context,
                              &compositor->m_compositor->xkb_names,
                              static_cast<enum xkb_keymap_compile_flags>(0));
  if (!keymap)
    throw std::runtime_error("Failed to compile keymap\n");

  weston_seat_init_keyboard(seat, keymap);
  xkb_keymap_unref(keymap);
  return 1;
}

struct weston_output *
xtwc::Compositor::FirstOutput()
{
  struct weston_output *output;
  
  output = wl_container_of(m_compositor->output_list.prev,
                           output,
                           link);

  return output;
}

struct wl_display *
xtwc::Compositor::Display()
{
  return m_compositor->wl_display;
}

struct weston_seat *
xtwc::Compositor::Seat()
{
  /* Since it is impossible to get a weston_seat from a weston_compositor
   * and we will need that in order to get access to the weston_pointer
   * and weston_keyboard, we need to use this hack to get access
   * to the seat by casting the weston_compositor to this, which is
   * copied from compositor-headless.c . Since weston_compositor is
   * the the first member of headless_compositor, it is safe to cast
   * from the second to the first */
  struct headless_compositor {
    struct weston_compositor compositor;
    struct weston_seat seat;
  };
  
  /* Before we cast, we should check if we are actually running
   * on the headless compositor. If not, throw an exception so
   * that the user might know what's going on */
  struct weston_output *output = FirstOutput();
  
  if (!output)
    throw std::runtime_error("Compositor does not have an output");

  if (std::string(output->model) != "headless")
  {
    std::stringstream ss;
    ss << "Only the compositor-headless.so backend "
       << "is supported by this extension. "
       << std::endl
       << "The current output model detected was "
       << output->model;
    throw std::logic_error(ss.str());
  }
  
  struct headless_compositor *hc =
    reinterpret_cast<struct headless_compositor *>(m_compositor);

  return &hc->seat;
}

struct weston_surface *
xtwc::Compositor::TopSurface()
{
  struct weston_surface *surface;

  /* The strange semantics of wl_container_of means that we can't
   * return its result directly because it needs to have an
   * instantiation of the type */
  surface = wl_container_of(m_compositor->surface_list.prev,
                            surface,
                            link);
  return surface;
}

void
xtwc::Compositor::OnEachMode(const boost::function<void(struct weston_mode *)> &action)
{
  struct weston_output *output = FirstOutput();
  struct weston_mode *mode;
  
  wl_list_for_each(mode, &output->mode_list, link)
  {
    action(mode);
  }
}

struct weston_mode *
xtwc::Compositor::LastMode()
{
  struct weston_mode *mode;
  struct weston_output *output = FirstOutput();
  mode = wl_container_of(output->mode_list.prev,
                         mode,
                         link);

  return mode;
}

struct wl_resource *
xtwc::Compositor::PointerResource(struct wl_client *client)
{
  struct weston_seat *seat = Seat();
  struct wl_resource *r =
    wl_resource_find_for_client(&seat->pointer->focus_resource_list,
                                client);
  if (!r)
    r =  wl_resource_find_for_client(&seat->pointer->resource_list,
                                     client);

  if (!r)
    throw std::logic_error ("No pointer resource available for this "
                            "client ");
  return r;
}

struct wl_resource *
xtwc::Compositor::KeyboardResource(struct wl_client *client)
{
  struct weston_seat *seat = Seat();
  struct wl_resource *r =
    wl_resource_find_for_client(&seat->keyboard->focus_resource_list,
                                client);
  if (!r)
    r =  wl_resource_find_for_client(&seat->keyboard->resource_list,
                                     client);

  if (!r)
    throw std::logic_error ("No keyboard resource available for this "
                            "client ");
  return r;
}

struct weston_surface *
xtwc::Compositor::Surface(struct wl_resource *surface)
{
  struct weston_surface *ws =
    reinterpret_cast<struct weston_surface *>(wl_resource_get_user_data(surface));
  
  return ws;
}

void
xtwc::Compositor::Unload(xtwc::Compositor *compositor)
{
  delete compositor;
}

struct weston_keyboard *
xtwc::Compositor::Keyboard()
{
  struct weston_seat *seat = Seat();
  return seat->keyboard;
}

xtwc::Compositor::~Compositor()
{
}

extern "C"
{
WL_EXPORT int
module_init(struct weston_compositor *c,
            int *argc,
            char *argv[])
{
  /* Using heap allocated memory directly here is awkward, however
   * weston knows when we need to destroy our resources
   * so we will let it handle it */
  xtwc::Compositor *compositor(new xtwc::Compositor(c));
  /* Register our factory for xbmc_wayland and pass
   * xtwc::Compositor to it when it gets created */
  if (wl_global_create(compositor->Display(),
                       &xbmc_wayland_interface,
                       1,
                       compositor,
                       xtw::XBMCWayland::BindToClient) == NULL)
    return -1;

  return 0;
}
}

