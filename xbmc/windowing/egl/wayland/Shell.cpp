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

#ifndef WAYLAND_WINDOWING_DLLWAYLANDCLIENT_H_INCLUDED
#define WAYLAND_WINDOWING_DLLWAYLANDCLIENT_H_INCLUDED
#include "windowing/DllWaylandClient.h"
#endif

#ifndef WAYLAND_WINDOWING_WAYLANDPROTOCOL_H_INCLUDED
#define WAYLAND_WINDOWING_WAYLANDPROTOCOL_H_INCLUDED
#include "windowing/WaylandProtocol.h"
#endif

#ifndef WAYLAND_SHELL_H_INCLUDED
#define WAYLAND_SHELL_H_INCLUDED
#include "Shell.h"
#endif


namespace xw = xbmc::wayland;

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
