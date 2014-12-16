#pragma once

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

#if (defined HAVE_CONFIG_H) && (!defined TARGET_WINDOWS)
  #include "config.h"
#endif
#include "utils/log.h"
#include "DynamicDll.h"

struct wl_surface;
struct wl_egl_window;

class IDllWaylandEGL
{
public:
  virtual ~IDllWaylandEGL() {}
  virtual struct wl_egl_window * wl_egl_window_create(struct wl_surface *,
                                                      int width,
                                                      int height) = 0;
  virtual void wl_egl_window_destroy(struct wl_egl_window *) = 0;
  virtual void wl_egl_window_resize(struct wl_egl_window *,
                                    int width, int height,
                                    int dx, int dy) = 0;
};

class DllWaylandEGL : public DllDynamic, public IDllWaylandEGL
{
  DECLARE_DLL_WRAPPER(DllWaylandEGL, DLL_PATH_WAYLAND_EGL)
  
  DEFINE_METHOD3(struct wl_egl_window *,
                 wl_egl_window_create,
                 (struct wl_surface *p1, int p2, int p3));
  DEFINE_METHOD1(void, wl_egl_window_destroy, (struct wl_egl_window *p1));
  DEFINE_METHOD5(void,
                 wl_egl_window_resize,
                 (struct wl_egl_window *p1,
                  int p2,
                  int p3,
                  int p4,
                  int p5));
  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD(wl_egl_window_create)
    RESOLVE_METHOD(wl_egl_window_destroy)
    RESOLVE_METHOD(wl_egl_window_resize)
  END_METHOD_RESOLVE()
};
