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
#include <wayland-client.h>
#include <wayland-egl.h>

#include "windowing/DllWaylandEgl.h"
#include "OpenGLSurface.h"

namespace xw = xbmc::wayland;

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

EGLNativeWindowType *
xw::OpenGLSurface::GetEGLNativeWindow()
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
