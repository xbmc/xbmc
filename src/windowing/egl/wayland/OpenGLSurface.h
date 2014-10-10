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
#include <boost/noncopyable.hpp>

class IDllWaylandEGL;

struct wl_surface;
struct wl_egl_window;

typedef struct wl_egl_window * EGLNativeWindowType;

namespace xbmc
{
namespace wayland
{
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
  EGLNativeWindowType * GetEGLNativeWindow();
  void Resize(int width, int height);

private:

  IDllWaylandEGL &m_eglLibrary;
  struct wl_egl_window *m_eglWindow;
};
}
}
