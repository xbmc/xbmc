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

#include "windowing/DllWaylandClient.h"
#include "windowing/WaylandProtocol.h"
#include "ShellSurface.h"

namespace xw = xbmc::wayland;

const wl_shell_surface_listener xw::ShellSurface::m_listener =
{
  ShellSurface::HandlePingCallback,
  ShellSurface::HandleConfigureCallback,
  ShellSurface::HandlePopupDoneCallback
};

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
