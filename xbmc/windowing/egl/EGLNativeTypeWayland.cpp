/*
 *      Copyright (C) 2011-2013 Team XBMC
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
#include <sstream>
#include <iostream>
#include <stdexcept>

#include <boost/noncopyable.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>

#include <cstdlib>

#include <wayland-client.h>

#include "../DllWaylandClient.h"
#include "../DllWaylandEgl.h"
#include "../DllXKBCommon.h"
#include "../WaylandProtocol.h"

#include "system.h"
#include "guilib/gui3d.h"
#include "utils/log.h"
#include "EGLNativeTypeWayland.h"
#include "../WinEvents.h"

#if defined(HAVE_WAYLAND)
namespace
{
class IWaylandRegistration
{
public:

  virtual ~IWaylandRegistration() {};

  virtual bool OnCompositorAvailable(struct wl_compositor *) = 0;
  virtual bool OnShellAvailable(struct wl_shell *) = 0;
  virtual bool OnSeatAvailable(struct wl_seat *) = 0;
  virtual bool OnShmAvailable(struct wl_shm *) = 0;
  virtual bool OnOutputAvailable(struct wl_output *) = 0;
};
}

namespace xbmc
{
namespace wayland
{
class Callback :
  boost::noncopyable
{
public:

  typedef boost::function<void(uint32_t)> Func;

  Callback(IDllWaylandClient &clientLibrary,
           struct wl_callback *callback,
           const Func &func);
  ~Callback();

  struct wl_callback * GetWlCallback();

  static const struct wl_callback_listener m_listener;

  static void OnCallback(void *,
                         struct wl_callback *,
                         uint32_t);

private:

  IDllWaylandClient &m_clientLibrary;
  struct wl_callback *m_callback;
  Func m_func;
};

const wl_callback_listener Callback::m_listener =
{
  Callback::OnCallback
};

class Region :
  boost::noncopyable
{
public:

  Region(IDllWaylandClient &clientLibrary,
         struct wl_region *);
  ~Region();
  
  struct wl_region * GetWlRegion();

  void AddRectangle(int32_t x,
                    int32_t y,
                    int32_t width,
                    int32_t height);

private:

  IDllWaylandClient &m_clientLibrary;
  struct wl_region *m_region;
};

class Display :
  boost::noncopyable
{
  public:

    Display(IDllWaylandClient &clientLibrary);
    ~Display();

    struct wl_display * GetWlDisplay();
    struct wl_display ** GetWlDisplayPtr();
    struct wl_callback * Sync();

  private:

    IDllWaylandClient &m_clientLibrary;
    struct wl_display *m_display;
};

class Registry :
  boost::noncopyable
{
public:

  Registry(IDllWaylandClient &clientLibrary,
           struct wl_display   *display,
           IWaylandRegistration &registration);
  ~Registry();

  struct wl_registry * GetWlRegistry();

  static void HandleGlobalCallback(void *, struct wl_registry *,
                                   uint32_t, const char *, uint32_t);
  static void HandleRemoveGlobalCallback(void *, struct wl_registry *,
                                         uint32_t name);

  static const std::string CompositorName;
  static const std::string ShellName;
  static const std::string SeatName;
  static const std::string ShmName;
  static const std::string OutputName;

private:

  static const struct wl_registry_listener m_listener;

  IDllWaylandClient &m_clientLibrary;
  struct wl_registry *m_registry;
  IWaylandRegistration &m_registration;

  void HandleGlobal(uint32_t, const char *, uint32_t);
  void HandleRemoveGlobal(uint32_t);
};

const std::string Registry::CompositorName("wl_compositor");
const std::string Registry::ShellName("wl_shell");
const std::string Registry::SeatName("wl_seat");
const std::string Registry::ShmName("wl_shm");
const std::string Registry::OutputName("wl_output");

const struct wl_registry_listener Registry::m_listener =
{
  Registry::HandleGlobalCallback,
  Registry::HandleRemoveGlobalCallback
};

class Compositor :
  boost::noncopyable
{
public:

  Compositor(IDllWaylandClient &clientLibrary,
             struct wl_compositor *compositor);
  ~Compositor();

  struct wl_compositor * GetWlCompositor();
  struct wl_surface * CreateSurface();
  struct wl_region * CreateRegion();

private:

  IDllWaylandClient &m_clientLibrary;
  struct wl_compositor *m_compositor;
};

class Shell :
  boost::noncopyable
{
public:

  Shell(IDllWaylandClient &clientLibrary,
        struct wl_shell *shell);
  ~Shell();

  struct wl_shell * GetWlShell();
  struct wl_shell_surface * CreateShellSurface(struct wl_surface *);

private:

  IDllWaylandClient &m_clientLibrary;
  struct wl_shell *m_shell;
};

struct Output :
  boost::noncopyable
{
public:

  Output(IDllWaylandClient &,
         struct wl_output *);
  ~Output();

  struct ModeGeometry
  {
    int32_t width;
    int32_t height;
    int32_t refresh;
  };

  struct PhysicalGeometry
  {
    int32_t x;
    int32_t y;
    int32_t physicalWidth;
    int32_t physicalHeight;
    enum wl_output_subpixel subpixelArrangement;
    enum wl_output_transform outputTransformation;
  };

  struct wl_output * GetWlOutput();

  const ModeGeometry & CurrentMode();
  const ModeGeometry & PreferredMode();

  const std::vector <ModeGeometry> & AllModes();

  const PhysicalGeometry & Geometry();
  uint32_t ScaleFactor();

  static void GeometryCallback(void *,
                               struct wl_output *,
                               int32_t,
                               int32_t,
                               int32_t,
                               int32_t,
                               int32_t,
                               const char *,
                               const char *,
                               int32_t);
  static void ModeCallback(void *,
                           struct wl_output *,
                           uint32_t,
                           int32_t,
                           int32_t,
                           int32_t);
  static void ScaleCallback(void *,
                            struct wl_output *,
                            int32_t);
  static void DoneCallback(void *,
                           struct wl_output *);

private:

  static const wl_output_listener m_listener;

  void Geometry(int32_t x,
                int32_t y,
                int32_t physicalWidth,
                int32_t physicalHeight,
                int32_t subpixel,
                const char *make,
                const char *model,
                int32_t transform);
  void Mode(uint32_t flags,
            int32_t width,
            int32_t height,
            int32_t refresh);
  void Scale(int32_t);
  void Done();

  IDllWaylandClient &m_clientLibrary;

  struct wl_output *m_output;

  PhysicalGeometry m_geometry;
  std::vector<ModeGeometry> m_modes;

  uint32_t m_scaleFactor;

  ModeGeometry *m_current;
  ModeGeometry *m_preferred;
};

const wl_output_listener Output::m_listener = 
{
  Output::GeometryCallback,
  Output::ModeCallback,
  Output::DoneCallback,
  Output::ScaleCallback
};

class Surface :
  boost::noncopyable
{
public:

  Surface(IDllWaylandClient &clientLibrary,
          struct wl_surface *surface);
  ~Surface();

  struct wl_surface * GetWlSurface();
  struct wl_callback * CreateFrameCallback();
  void SetOpaqueRegion(struct wl_region *region);
  void Commit();

private:

  IDllWaylandClient &m_clientLibrary;
  struct wl_surface *m_surface;
};

class ShellSurface :
  boost::noncopyable
{
public:

  ShellSurface(IDllWaylandClient &clientLibrary,
               struct wl_shell_surface *shellSurface);
  ~ShellSurface();

  struct wl_shell_surface * GetWlShellSurface();
  void SetFullscreen(enum wl_shell_surface_fullscreen_method method,
                     uint32_t framerate,
                     struct wl_output *output);

  static const wl_shell_surface_listener m_listener;

  static void HandlePingCallback(void *,
                                 struct wl_shell_surface *,
                                 uint32_t);
  static void HandleConfigureCallback(void *,
                                      struct wl_shell_surface *,
                                      uint32_t,
                                      int32_t,
                                      int32_t);
  static void HandlePopupDoneCallback(void *,
                                      struct wl_shell_surface *);

private:

  void HandlePing(uint32_t serial);
  void HandleConfigure(uint32_t edges,
                       int32_t width,
                       int32_t height);
  void HandlePopupDone();

  IDllWaylandClient &m_clientLibrary;
  struct wl_shell_surface *m_shellSurface;
};

const wl_shell_surface_listener ShellSurface::m_listener =
{
  ShellSurface::HandlePingCallback,
  ShellSurface::HandleConfigureCallback,
  ShellSurface::HandlePopupDoneCallback
};

class OpenGLSurface :
  boost::noncopyable
{
public:

  OpenGLSurface(IDllWaylandEGL &eglLibrary,
                struct wl_surface *surface,
                int32_t width,
                int32_t height);
  ~OpenGLSurface();

  struct wl_egl_window * GetWlEglWindow();
  struct wl_egl_window ** GetWlEglWindowPtr();
  void Resize(int width, int height);

private:

  IDllWaylandEGL &m_eglLibrary;
  struct wl_egl_window *m_eglWindow;
};
}
}

namespace xw = xbmc::wayland;

xw::Region::Region(IDllWaylandClient &clientLibrary,
                   struct wl_region *region) :
  m_clientLibrary(clientLibrary),
  m_region(region)
{
}

xw::Region::~Region()
{
  protocol::CallMethodOnWaylandObject(m_clientLibrary,
                                      m_region,
                                      WL_REGION_DESTROY);
  protocol::DestroyWaylandObject(m_clientLibrary,
                                 m_region);
}

struct wl_region *
xw::Region::GetWlRegion()
{
  return m_region;
}

void
xw::Region::AddRectangle(int32_t x,
                         int32_t y,
                         int32_t width,
                         int32_t height)
{
  protocol::CallMethodOnWaylandObject(m_clientLibrary,
                                      m_region,
                                      WL_REGION_ADD,
                                      x,
                                      y,
                                      width,
                                      height);
}

xw::Callback::Callback(IDllWaylandClient &clientLibrary,
                       struct wl_callback *callback,
                       const Func &func) :
  m_clientLibrary(clientLibrary),
  m_callback(callback),
  m_func(func)
{
  protocol::AddListenerOnWaylandObject(m_clientLibrary,
                                       m_callback,
                                       &m_listener,
                                       reinterpret_cast<void *>(this));
}

xw::Callback::~Callback()
{
  protocol::DestroyWaylandObject(m_clientLibrary,
                                 m_callback);
}

struct wl_callback *
xw::Callback::GetWlCallback()
{
  return m_callback;
}

void
xw::Callback::OnCallback(void *data,
                         struct wl_callback *callback,
                         uint32_t time)
{
  static_cast<Callback *>(data)->m_func(time);
}

xw::Display::Display(IDllWaylandClient &clientLibrary) :
  m_clientLibrary(clientLibrary),
  m_display(m_clientLibrary.wl_display_connect(NULL))
{
  if (!m_display)
  {
    std::stringstream ss;
    ss << "Failed to connect to "
       << getenv("WAYLAND_DISPLAY");
    throw std::runtime_error(ss.str());
  }
}

xw::Display::~Display()
{
  m_clientLibrary.wl_display_flush(m_display);
  m_clientLibrary.wl_display_disconnect(m_display);
}

struct wl_display *
xw::Display::GetWlDisplay()
{
  return m_display;
}

struct wl_display **
xw::Display::GetWlDisplayPtr()
{
  return &m_display;
}

struct wl_callback *
xw::Display::Sync()
{
  struct wl_callback *callback =
      protocol::CreateWaylandObject<struct wl_callback *,
                                    struct wl_display *> (m_clientLibrary,
                                                          m_display,
                                                          m_clientLibrary.Get_wl_callback_interface());
  protocol::CallMethodOnWaylandObject(m_clientLibrary,
                                      m_display,
                                      WL_DISPLAY_SYNC,
                                      callback);
  return callback;
}

xw::Registry::Registry(IDllWaylandClient &clientLibrary,
                       struct wl_display *display,
                       IWaylandRegistration &registration) :
  m_clientLibrary(clientLibrary),
  m_registry(protocol::CreateWaylandObject<struct wl_registry *,
                                           struct wl_display *> (m_clientLibrary,
                                                                 display,
                                                                 m_clientLibrary.Get_wl_registry_interface())),
  m_registration(registration)
{
  protocol::CallMethodOnWaylandObject(m_clientLibrary,
                                      display,
                                      WL_DISPLAY_GET_REGISTRY,
                                      m_registry);
  protocol::AddListenerOnWaylandObject(m_clientLibrary,
                                       m_registry,
                                       &m_listener,
                                       reinterpret_cast<void *>(this));
}

xw::Registry::~Registry()
{
  protocol::DestroyWaylandObject(m_clientLibrary, m_registry);
}

void
xw::Registry::HandleGlobal(uint32_t name,
                           const char *interface,
                           uint32_t version)
{
  if (interface == CompositorName)
  {
    struct wl_compositor *compositor =
      static_cast<struct wl_compositor *>(protocol::CreateWaylandObject<struct wl_compositor *,
                                                                        struct wl_registry *>(m_clientLibrary,
                                                                                              m_registry,
                                                                                              m_clientLibrary.Get_wl_compositor_interface()));
    protocol::CallMethodOnWaylandObject(m_clientLibrary,
                                        m_registry,
                                        WL_REGISTRY_BIND,
                                        name,
                                        reinterpret_cast<struct wl_interface *>(m_clientLibrary.Get_wl_compositor_interface())->name,
                                        1,
                                        compositor);
    m_registration.OnCompositorAvailable(compositor);
  }
  else if (interface == ShellName)
  {
    struct wl_shell *shell =
      static_cast<struct wl_shell *>(protocol::CreateWaylandObject<struct wl_shell *,
                                                                   struct wl_registry *>(m_clientLibrary,
                                                                                         m_registry,
                                                                                         m_clientLibrary.Get_wl_shell_interface()));
    protocol::CallMethodOnWaylandObject(m_clientLibrary,
                                        m_registry,
                                        WL_REGISTRY_BIND,
                                        name,
                                        reinterpret_cast<struct wl_interface *>(m_clientLibrary.Get_wl_shell_interface())->name,
                                        1,
                                        shell);
    m_registration.OnShellAvailable(shell);
  }
  else if (interface == SeatName)
  {
    struct wl_seat *seat =
      static_cast<struct wl_seat *>(protocol::CreateWaylandObject<struct wl_seat *,
                                                                  struct wl_registry *>(m_clientLibrary,
                                                                                        m_registry,
                                                                                        m_clientLibrary.Get_wl_seat_interface()));
    protocol::CallMethodOnWaylandObject(m_clientLibrary,
                                        m_registry,
                                        WL_REGISTRY_BIND,
                                        name,
                                        reinterpret_cast<struct wl_interface *>(m_clientLibrary.Get_wl_seat_interface())->name,
                                        1,
                                        seat);
    m_registration.OnSeatAvailable(seat);
  }
  else if (interface == ShmName)
  {
  }
  else if (interface == OutputName)
  {
    struct wl_output *output =
      static_cast<struct wl_output *>(protocol::CreateWaylandObject<struct wl_output *,
                                                                    struct wl_registry *>(m_clientLibrary,
                                                                                          m_registry,
                                                                                          m_clientLibrary.Get_wl_output_interface()));
    protocol::CallMethodOnWaylandObject(m_clientLibrary,
                                        m_registry,
                                        WL_REGISTRY_BIND,
                                        name,
                                        reinterpret_cast<struct wl_interface *>(m_clientLibrary.Get_wl_output_interface())->name,
                                        1,
                                        output);
    m_registration.OnOutputAvailable(output);
  }
}

void
xw::Registry::HandleRemoveGlobal(uint32_t name)
{
}

void
xw::Registry::HandleGlobalCallback(void *data,
                                   struct wl_registry *registry,
                                   uint32_t name,
                                   const char *interface,
                                   uint32_t version)
{
  static_cast<Registry *>(data)->HandleGlobal(name, interface, version);
}

void
xw::Registry::HandleRemoveGlobalCallback(void *data,
                                         struct wl_registry *registry,
                                         uint32_t name)
{
  static_cast<Registry *>(data)->HandleRemoveGlobal(name);
}

xw::Compositor::Compositor(IDllWaylandClient &clientLibrary,
                           struct wl_compositor *compositor) :
  m_clientLibrary(clientLibrary),
  m_compositor(compositor)
{
}

xw::Compositor::~Compositor()
{
  protocol::DestroyWaylandObject(m_clientLibrary,
                                 m_compositor);
}

struct wl_compositor *
xw::Compositor::GetWlCompositor()
{
  return m_compositor;
}

struct wl_surface *
xw::Compositor::CreateSurface()
{
  struct wl_surface *surface =
    protocol::CreateWaylandObject<struct wl_surface *,
                                  struct wl_compositor *>(m_clientLibrary,
                                                          m_compositor,
                                                          m_clientLibrary.Get_wl_surface_interface());
  protocol::CallMethodOnWaylandObject(m_clientLibrary,
                                      m_compositor,
                                      WL_COMPOSITOR_CREATE_SURFACE,
                                      surface);
  return surface;
}

struct wl_region *
xw::Compositor::CreateRegion()
{
  struct wl_region *region =
    protocol::CreateWaylandObject<struct wl_region *,
                                  struct wl_compositor *>(m_clientLibrary,
                                                          m_compositor,
                                                          m_clientLibrary.Get_wl_region_interface ());
  protocol::CallMethodOnWaylandObject(m_clientLibrary,
                                      m_compositor,
                                      WL_COMPOSITOR_CREATE_REGION,
                                      region);
  return region;
}

xw::Shell::Shell(IDllWaylandClient &clientLibrary,
                 struct wl_shell *shell) :
  m_clientLibrary(clientLibrary),
  m_shell(shell)
{
}

xw::Shell::~Shell()
{
  protocol::DestroyWaylandObject(m_clientLibrary,
                                 m_shell);
}

struct wl_shell *
xw::Shell::GetWlShell()
{
  return m_shell;
}

struct wl_shell_surface *
xw::Shell::CreateShellSurface(struct wl_surface *surface)
{
  struct wl_shell_surface *shellSurface =
    protocol::CreateWaylandObject<struct wl_shell_surface *,
                                  struct wl_shell *>(m_clientLibrary,
                                                     m_shell,
                                                     m_clientLibrary.Get_wl_shell_surface_interface ());
  protocol::CallMethodOnWaylandObject(m_clientLibrary,
                                      m_shell,
                                      WL_SHELL_GET_SHELL_SURFACE,
                                      shellSurface,
                                      surface);
  return shellSurface;
}

xw::Output::Output(IDllWaylandClient &clientLibrary,
                   struct wl_output *output) :
  m_clientLibrary(clientLibrary),
  m_output(output),
  m_scaleFactor(1.0),
  m_current(NULL),
  m_preferred(NULL)
{
  protocol::AddListenerOnWaylandObject(m_clientLibrary,
                                       m_output,
                                       &m_listener,
                                       reinterpret_cast<void *>(this));
}

xw::Output::~Output()
{
  protocol::DestroyWaylandObject(m_clientLibrary,
                                 m_output);
}

struct wl_output *
xw::Output::GetWlOutput()
{
  return m_output;
}

const xw::Output::ModeGeometry &
xw::Output::CurrentMode()
{
  if (!m_current)
    throw std::logic_error("No current mode has been set by the server"
                           " yet");
  
  return *m_current;
}

const xw::Output::ModeGeometry &
xw::Output::PreferredMode()
{
  if (!m_preferred)
    throw std::logic_error("No preferred mode has been set by the "
                           " server yet");

  return *m_preferred;
}

const std::vector <xw::Output::ModeGeometry> &
xw::Output::AllModes()
{
  return m_modes;
}

const xw::Output::PhysicalGeometry &
xw::Output::Geometry()
{
  return m_geometry;
}

uint32_t
xw::Output::ScaleFactor()
{
  return m_scaleFactor;
}

void
xw::Output::GeometryCallback(void *data,
                             struct wl_output *output,
                             int32_t x,
                             int32_t y,
                             int32_t physicalWidth,
                             int32_t physicalHeight,
                             int32_t subpixelArrangement,
                             const char *make,
                             const char *model,
                             int32_t transform)
{
  return static_cast<xw::Output *>(data)->Geometry(x,
                                                   y,
                                                   physicalWidth,
                                                   physicalHeight,
                                                   subpixelArrangement,
                                                   make,
                                                   model,
                                                   transform);
}

void
xw::Output::ModeCallback(void *data,
                         struct wl_output *output,
                         uint32_t flags,
                         int32_t width,
                         int32_t height,
                         int32_t refresh)
{
  return static_cast<xw::Output *>(data)->Mode(flags,
                                               width,
                                               height,
                                               refresh);
}

void
xw::Output::DoneCallback(void *data,
                         struct wl_output *output)
{
  return static_cast<xw::Output *>(data)->Done();
}

void
xw::Output::ScaleCallback(void *data,
                          struct wl_output *output,
                          int32_t factor)
{
  return static_cast<xw::Output *>(data)->Scale(factor);
}

void
xw::Output::Geometry(int32_t x,
                     int32_t y,
                     int32_t physicalWidth,
                     int32_t physicalHeight,
                     int32_t subpixelArrangement,
                     const char *make,
                     const char *model,
                     int32_t transform)
{
  m_geometry.x = x;
  m_geometry.y = y;
  m_geometry.physicalWidth = physicalWidth;
  m_geometry.physicalHeight = physicalHeight;
  m_geometry.subpixelArrangement =
    static_cast<enum wl_output_subpixel>(subpixelArrangement);
  m_geometry.outputTransformation =
    static_cast<enum wl_output_transform>(transform);
}

void
xw::Output::Mode(uint32_t flags,
                 int32_t width,
                 int32_t height,
                 int32_t refresh)
{
  xw::Output::ModeGeometry *update = NULL;
  
  for (std::vector<ModeGeometry>::iterator it = m_modes.begin();
       it != m_modes.end();
       ++it)
  { 
    if (it->width == width &&
        it->height == height &&
        it->refresh == refresh)
    {
      update = &(*it);
      break;
    }
  }
  
  enum wl_output_mode outputFlags =
    static_cast<enum wl_output_mode>(flags);
  
  if (!update)
  {
    /* New output created */
    m_modes.push_back(ModeGeometry());
    ModeGeometry &next(m_modes.back());
    
    next.width = width;
    next.height = height;
    next.refresh = refresh;
    
    update = &next;
  }
  
  if (outputFlags & WL_OUTPUT_MODE_CURRENT)
    m_current = update;
  if (outputFlags & WL_OUTPUT_MODE_PREFERRED)
    m_preferred = update;
}

void
xw::Output::Done()
{
}

void
xw::Output::Scale(int32_t factor)
{
  m_scaleFactor = factor;
}

xw::Surface::Surface(IDllWaylandClient &clientLibrary,
                     struct wl_surface *surface) :
  m_clientLibrary(clientLibrary),
  m_surface(surface)
{
}

xw::Surface::~Surface()
{
  protocol::CallMethodOnWaylandObject(m_clientLibrary,
                                      m_surface,
                                      WL_SURFACE_DESTROY);
  protocol::DestroyWaylandObject(m_clientLibrary,
                                 m_surface);
}

struct wl_surface *
xw::Surface::GetWlSurface()
{
  return m_surface;
}

struct wl_callback *
xw::Surface::CreateFrameCallback()
{
  struct wl_callback *callback =
    protocol::CreateWaylandObject<struct wl_callback *,
                                  struct wl_surface *>(m_clientLibrary,
                                                       m_surface,
                                                       m_clientLibrary.Get_wl_callback_interface());
  protocol::CallMethodOnWaylandObject(m_clientLibrary,
                                      m_surface,
                                      WL_SURFACE_FRAME, callback);
  return callback;
}

void
xw::Surface::SetOpaqueRegion(struct wl_region *region)
{
  protocol::CallMethodOnWaylandObject(m_clientLibrary,
                                      m_surface,
                                      WL_SURFACE_SET_OPAQUE_REGION,
                                      region);
}

void
xw::Surface::Commit()
{
  protocol::CallMethodOnWaylandObject(m_clientLibrary,
                                      m_surface,
                                      WL_SURFACE_COMMIT);
}

xw::ShellSurface::ShellSurface(IDllWaylandClient &clientLibrary,
                               struct wl_shell_surface *shell_surface) :
  m_clientLibrary(clientLibrary),
  m_shellSurface(shell_surface)
{
  protocol::AddListenerOnWaylandObject(m_clientLibrary,
                                       m_shellSurface,
                                       &m_listener,
                                       reinterpret_cast<void *>(this));
}

xw::ShellSurface::~ShellSurface()
{
  protocol::DestroyWaylandObject(m_clientLibrary, m_shellSurface);
}

struct wl_shell_surface *
xw::ShellSurface::GetWlShellSurface()
{
  return m_shellSurface;
}

void
xw::ShellSurface::SetFullscreen(enum wl_shell_surface_fullscreen_method method,
                                uint32_t framerate,
                                struct wl_output *output)
{
  protocol::CallMethodOnWaylandObject(m_clientLibrary,
                                      m_shellSurface,
                                      WL_SHELL_SURFACE_SET_FULLSCREEN,
                                      method,
                                      framerate,
                                      output);
}

void
xw::ShellSurface::HandlePingCallback(void *data,
                                     struct wl_shell_surface *shell_surface,
                                     uint32_t serial)
{
  return static_cast<ShellSurface *>(data)->HandlePing(serial);
}

void
xw::ShellSurface::HandleConfigureCallback(void *data,
                                          struct wl_shell_surface *shell_surface,
                                          uint32_t edges,
                                          int32_t width,
                                          int32_t height)
{
  return static_cast<ShellSurface *>(data)->HandleConfigure(edges,
                                                            width,
                                                            height);
}

void
xw::ShellSurface::HandlePopupDoneCallback(void *data,
                                          struct wl_shell_surface *shell_surface)
{
  return static_cast<ShellSurface *>(data)->HandlePopupDone();
}

void
xw::ShellSurface::HandlePing(uint32_t serial)
{
  protocol::CallMethodOnWaylandObject(m_clientLibrary,
                                      m_shellSurface,
                                      WL_SHELL_SURFACE_PONG,
                                      serial);
}

void
xw::ShellSurface::HandleConfigure(uint32_t edges,
                                  int32_t width,
                                  int32_t height)
{
}

void
xw::ShellSurface::HandlePopupDone()
{
}

xw::OpenGLSurface::OpenGLSurface(IDllWaylandEGL &eglLibrary,
                                 struct wl_surface *surface,
                                 int width,
                                 int height) :
  m_eglLibrary(eglLibrary),
  m_eglWindow(m_eglLibrary.wl_egl_window_create(surface,
                                                width,
                                                height))
{
}

xw::OpenGLSurface::~OpenGLSurface()
{
  m_eglLibrary.wl_egl_window_destroy(m_eglWindow);
}

struct wl_egl_window *
xw::OpenGLSurface::GetWlEglWindow()
{
  return m_eglWindow;
}

struct wl_egl_window **
xw::OpenGLSurface::GetWlEglWindowPtr()
{
  return &m_eglWindow;
}

void
xw::OpenGLSurface::Resize(int width, int height)
{
  m_eglLibrary.wl_egl_window_resize(m_eglWindow,
                                    width,
                                    height,
                                    0,
                                    0);
}

class CEGLNativeTypeWayland::Private :
  public IWaylandRegistration
{
public:

  boost::scoped_ptr<xw::Display> m_display;
  boost::scoped_ptr<xw::Registry> m_registry;
  boost::scoped_ptr<xw::Compositor> m_compositor;
  boost::scoped_ptr<xw::Shell> m_shell;

  boost::scoped_ptr<xw::Surface> m_surface;
  boost::scoped_ptr<xw::ShellSurface> m_shellSurface;
  boost::scoped_ptr<xw::OpenGLSurface> m_glSurface;
  boost::scoped_ptr<xw::Callback> m_frameCallback;
  std::vector<boost::shared_ptr <xw::Output> > m_outputs;

  DllWaylandClient m_clientLibrary;
  DllWaylandEGL m_eglLibrary;
  DllXKBCommon m_xkbCommonLibrary;

  void AddFrameCallback();
  void WaitForSynchronize();

private:

  bool synchronized;
  boost::scoped_ptr<xw::Callback> synchronizeCallback;
  
  void Synchronize();

  bool OnCompositorAvailable(struct wl_compositor *);
  bool OnShellAvailable(struct wl_shell *);
  bool OnSeatAvailable(struct wl_seat *);
  bool OnShmAvailable(struct wl_shm *);
  bool OnOutputAvailable(struct wl_output *);

  void OnFrameCallback(uint32_t);
};
#else
class CEGLNativeTypeWayland::Private
{
};
#endif

CEGLNativeTypeWayland::CEGLNativeTypeWayland() :
  priv(new Private())
{
}

CEGLNativeTypeWayland::~CEGLNativeTypeWayland()
{
} 

#if defined(HAVE_WAYLAND)
bool CEGLNativeTypeWayland::Private::OnCompositorAvailable(struct wl_compositor *c)
{
  m_compositor.reset(new xw::Compositor(m_clientLibrary, c));
  return true;
}

bool CEGLNativeTypeWayland::Private::OnShellAvailable(struct wl_shell *s)
{
  m_shell.reset(new xw::Shell(m_clientLibrary, s));
  return true;
}

bool CEGLNativeTypeWayland::Private::OnSeatAvailable(struct wl_seat *s)
{
  CWinEventsWayland::SetWaylandSeat(m_clientLibrary,
                                    m_xkbCommonLibrary,
                                    s);
  return true;
}

bool CEGLNativeTypeWayland::Private::OnShmAvailable(struct wl_shm *s)
{
  return true;
}

bool CEGLNativeTypeWayland::Private::OnOutputAvailable(struct wl_output *o)
{
  m_outputs.push_back(boost::shared_ptr<xw::Output>(new xw::Output(m_clientLibrary,
                                                                   o)));
  WaitForSynchronize();
  return true;
}

void CEGLNativeTypeWayland::Private::WaitForSynchronize()
{
  boost::function<void(uint32_t)> func(boost::bind(&Private::Synchronize,
                                                   this));
  
  synchronized = false;
  synchronizeCallback.reset(new xw::Callback(m_clientLibrary,
                                             m_display->Sync(),
                                             func));
  while (!synchronized)
    CWinEvents::MessagePump();
}

void CEGLNativeTypeWayland::Private::Synchronize()
{
  synchronized = true;
  synchronizeCallback.reset();
}
#endif

bool CEGLNativeTypeWayland::CheckCompatibility()
{
#if defined(HAVE_WAYLAND)
  if (!getenv("WAYLAND_DISPLAY"))
  {
    CLog::Log(LOGWARNING, "%s:, WAYLAND_DISPLAY is not set",
              __FUNCTION__);
    return false;
  }
  
  /* FIXME:
   * There appears to be a bug in DllDynamic::CanLoad() which causes
   * it to always return false. We are just loading the library 
   * directly at CheckCompatibility time now */
  const struct LibraryStatus
  {
    const char *library;
    bool status;
  } libraryStatus[] =
  {
    { priv->m_clientLibrary.GetFile().c_str(), priv->m_clientLibrary.Load() },
    { priv->m_eglLibrary.GetFile().c_str(), priv->m_eglLibrary.Load() },
    { priv->m_xkbCommonLibrary.GetFile().c_str(), priv->m_xkbCommonLibrary.Load() }
  };
  
  const size_t libraryStatusSize = sizeof(libraryStatus) /
                                   sizeof(libraryStatus[0]);
  bool loadFailure = false;

  for (size_t i = 0; i < libraryStatusSize; ++i)
  {
    if (!libraryStatus[i].status)
    {
      CLog::Log(LOGWARNING, "%s: Unable to load library: %s\n",
                __FUNCTION__, libraryStatus[i].library);
      loadFailure = true;
    }
  }

  if (loadFailure)
  {
    priv->m_clientLibrary.Unload();
    priv->m_eglLibrary.Unload();
    priv->m_xkbCommonLibrary.Unload();
    return false;
  }

  return true;
#else
  return false
#endif
}

void CEGLNativeTypeWayland::Initialize()
{
}

void CEGLNativeTypeWayland::Destroy()
{
#if defined(HAVE_WAYLAND)
  priv->m_registry.reset();
  priv->m_display.reset();

  priv->m_clientLibrary.Unload();
  priv->m_eglLibrary.Unload();
  priv->m_xkbCommonLibrary.Unload();
#endif
}

bool CEGLNativeTypeWayland::CreateNativeDisplay()
{
#if defined(HAVE_WAYLAND)
  try
  {
    priv->m_display.reset(new xw::Display(priv->m_clientLibrary));
  }
  catch (const std::runtime_error &err)
  {
    CLog::Log(LOGERROR, "%s: %s", __FUNCTION__, err.what());
    return false;
  }
  
  CWinEventsWayland::SetWaylandDisplay(priv->m_clientLibrary,
                                       priv->m_display->GetWlDisplay());

  priv->m_registry.reset(new xw::Registry(priv->m_clientLibrary,
                                          priv->m_display->GetWlDisplay(),
                                          *priv));
  priv->WaitForSynchronize();
  
  if (priv->m_outputs.empty())
  {
    CLog::Log(LOGERROR, "%s: compositor did not provide at least one "
              "output", __FUNCTION__);
    return false;
  }

  return true;
#else
  return false;
#endif
}

bool CEGLNativeTypeWayland::CreateNativeWindow()
{
#if defined(HAVE_WAYLAND)
  struct wl_surface *wls = priv->m_compositor->CreateSurface();
  xw::Surface *s = new xw::Surface(priv->m_clientLibrary, wls);
  priv->m_surface.reset(s);

  xw::ShellSurface *ss =
    new xw::ShellSurface(priv->m_clientLibrary,
                         priv->m_shell->CreateShellSurface(wls));
  priv->m_shellSurface.reset(ss);

  /* Supporting only the first output device at the moment */
  const xw::Output::ModeGeometry &current(priv->m_outputs[0]->CurrentMode());

  xw::OpenGLSurface *os = new xw::OpenGLSurface(priv->m_eglLibrary,
                                                wls,
                                                current.width,
                                                current.height);
  priv->m_glSurface.reset(os);
  
  xw::Region region(priv->m_clientLibrary,
                    priv->m_compositor->CreateRegion());
  
  region.AddRectangle(0, 0, current.width, current.height);
  
  priv->m_surface->SetOpaqueRegion(region.GetWlRegion());
  priv->m_surface->Commit();

  priv->AddFrameCallback();
  priv->WaitForSynchronize();

  CWinEventsWayland::SetXBMCSurface(wls);

  return true;
#else
  return false;
#endif
}

bool CEGLNativeTypeWayland::GetNativeDisplay(XBNativeDisplayType **nativeDisplay) const
{
#if defined(HAVE_WAYLAND)
  /* We need to return a pointer to the wl_display * (eg wl_display **),
   * as EGLWrapper needs to dereference our return value to get the
   * actual display and not its first member */
  *nativeDisplay =
      reinterpret_cast <XBNativeDisplayType *>(priv->m_display.get()->GetWlDisplayPtr());
  return true;
#else
  return false;
#endif
}

bool CEGLNativeTypeWayland::GetNativeWindow(XBNativeDisplayType **nativeWindow) const
{
#if defined(HAVE_WAYLAND)
  *nativeWindow =
      reinterpret_cast <XBNativeWindowType *>(priv->m_glSurface.get()->GetWlEglWindowPtr());
  return true;
#else
  return false;
#endif
}  

bool CEGLNativeTypeWayland::DestroyNativeDisplay()
{
#if defined(HAVE_WAYLAND)
  CWinEventsWayland::DestroyWaylandSeat();
  CWinEventsWayland::DestroyWaylandDisplay();

  priv->m_shell.reset();
  priv->m_outputs.clear();
  priv->m_compositor.reset();

  priv->m_display.reset();
  return true;
#else
  return false;
#endif
}

bool CEGLNativeTypeWayland::DestroyNativeWindow()
{
#if defined(HAVE_WAYLAND)
  priv->m_glSurface.reset();
  priv->m_shellSurface.reset();
  priv->m_surface.reset();
  priv->m_frameCallback.reset();
  return true;  
#else
  return false;
#endif
}

#if defined(HAVE_WAYLAND)
namespace
{
void ResolutionInfoForMode(const xw::Output::ModeGeometry &mode,
                           RESOLUTION_INFO *res)
{
  res->iWidth = mode.width;
  res->iHeight = mode.height;
  
  /* The refresh rate is given as an integer in the second exponent
   * so we need to divide by 100.0f to get a floating point value */
  res->fRefreshRate = mode.refresh / 100.0f;
  res->dwFlags = D3DPRESENTFLAG_PROGRESSIVE;
  res->iScreen = 0;
  res->bFullScreen = true;
  res->iSubtitles = static_cast<int>(0.965 * res->iHeight);
  res->fPixelRatio = 1.0f;
  res->iScreenWidth = res->iWidth;
  res->iScreenHeight = res->iHeight;
  res->strMode.Format("%dx%d @ %.2fp",
                      res->iScreenWidth,
                      res->iScreenHeight,
                      res->fRefreshRate);
}
}
#endif

bool CEGLNativeTypeWayland::GetNativeResolution(RESOLUTION_INFO *res) const
{
#if defined(HAVE_WAYLAND)
  /* Supporting only the first output device at the moment */
  const xw::Output::ModeGeometry &current(priv->m_outputs[0]->CurrentMode());
  
  ResolutionInfoForMode(current, res);

  return true;
#else
  return false;
#endif
}

bool CEGLNativeTypeWayland::SetNativeResolution(const RESOLUTION_INFO &res)
{
#if defined(HAVE_WAYLAND)
  priv->m_glSurface->Resize(res.iScreenWidth, res.iScreenHeight);
  
  xw::Region region(priv->m_clientLibrary,
                    priv->m_compositor->CreateRegion());
  
  region.AddRectangle(0, 0, res.iScreenWidth, res.iScreenHeight);
  
  priv->m_surface->SetOpaqueRegion(region.GetWlRegion());
  priv->m_surface->Commit();
  return true;
#else
  return false;
#endif
}

bool CEGLNativeTypeWayland::ProbeResolutions(std::vector<RESOLUTION_INFO> &resolutions)
{
#if defined(HAVE_WAYLAND)
  /* Supporting only the first output device at the moment */
  const boost::shared_ptr <xw::Output> &output(priv->m_outputs[0]);
  const std::vector<xw::Output::ModeGeometry> &m_modes(output->AllModes());

  for (std::vector<xw::Output::ModeGeometry>::const_iterator it = m_modes.begin();
       it != m_modes.end();
       ++it)
  {
    resolutions.push_back(RESOLUTION_INFO());
    RESOLUTION_INFO &back(resolutions.back());
    
    ResolutionInfoForMode(*it, &back);
  }

  return true;
#else
  return false;
#endif
}

bool CEGLNativeTypeWayland::GetPreferredResolution(RESOLUTION_INFO *res) const
{
#if defined(HAVE_WAYLAND)
  /* Supporting only the first output device at the moment */
  const xw::Output::ModeGeometry &preferred(priv->m_outputs[0]->PreferredMode());
  ResolutionInfoForMode(preferred, res);
  return true;
#else
  return false;
#endif
}

bool CEGLNativeTypeWayland::ShowWindow(bool show)
{
#if defined(HAVE_WAYLAND)
  if (show)
    priv->m_shellSurface->SetFullscreen(WL_SHELL_SURFACE_FULLSCREEN_METHOD_DRIVER,
                                        0,
                                        priv->m_outputs[0]->GetWlOutput());  else
    return false;

  return true;
#else
  return false;
#endif
}

#if defined(HAVE_WAYLAND)
void CEGLNativeTypeWayland::Private::OnFrameCallback(uint32_t time)
{
  AddFrameCallback();
}

void CEGLNativeTypeWayland::Private::AddFrameCallback()
{
  m_frameCallback.reset(new xw::Callback(m_clientLibrary,
                                         m_surface->CreateFrameCallback(),
                                         boost::bind(&CEGLNativeTypeWayland::Private::OnFrameCallback,
                                                     this,
                                                     _1)));
}
#endif
