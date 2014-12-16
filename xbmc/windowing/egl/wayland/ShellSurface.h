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
#include <wayland-client.h>

#include <boost/noncopyable.hpp>

class IDllWaylandClient;

namespace xbmc
{
namespace wayland
{
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
}
}
