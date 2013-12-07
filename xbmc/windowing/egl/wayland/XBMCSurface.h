#pragma once

/*
 *      Copyright (C) 2011-2013 Team XBMC
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
#include <boost/function.hpp>
#include <boost/noncopyable.hpp>

class IDllWaylandClient;
class IDllWaylandEGL;

struct wl_region;

typedef struct wl_egl_window * EGLNativeWindowType;

namespace xbmc
{
namespace wayland
{
class Callback;
class Compositor;
class OpenGLSurface;
class Output;
class Shell;
class ShellSurface;
class Surface;

class XBMCSurface
{
public:

  struct EventInjector
  {
    typedef void (*SetXBMCSurface)(struct wl_surface *);
    
    SetXBMCSurface setXBMCSurface;
  };

  XBMCSurface(IDllWaylandClient &clientLibrary,
              IDllWaylandEGL &eglLibrary,
              const EventInjector &eventInjector,
              Compositor &compositor,
              Shell &shell,
              uint32_t width,
              uint32_t height);
  ~XBMCSurface();

  void Show(Output &output);
  void Resize(uint32_t width, uint32_t height);
  EGLNativeWindowType * EGLNativeWindow() const;

private:

  class Private;
  boost::scoped_ptr<Private> priv;
};
}
}
