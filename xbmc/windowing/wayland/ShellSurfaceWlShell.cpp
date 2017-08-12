/*
 *      Copyright (C) 2017 Team XBMC
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

#include "Registry.h"
#include "ShellSurfaceWlShell.h"

#include <cmath>

using namespace KODI::WINDOWING::WAYLAND;
using namespace std::placeholders;

CShellSurfaceWlShell::CShellSurfaceWlShell(IShellSurfaceHandler& handler, CConnection& connection, const wayland::surface_t& surface, std::string title, std::string class_)
: m_handler{handler}
{
  {
    CRegistry registry{connection};
    registry.RequestSingleton(m_shell, 1, 1);
    registry.Bind();
  }

  m_shellSurface = m_shell.get_shell_surface(surface);

  m_surfaceState.set(STATE_ACTIVATED);
  m_shellSurface.set_class(class_);
  m_shellSurface.set_title(title);
  m_shellSurface.on_ping() = [this](std::uint32_t serial)
  {
    m_shellSurface.pong(serial);
  };
  m_shellSurface.on_configure() = [this](wayland::shell_surface_resize, std::int32_t width, std::int32_t height)
  {
    // wl_shell does not have serials
    m_handler.OnConfigure(0, {width, height}, m_surfaceState);
  };
}

void CShellSurfaceWlShell::AckConfigure(std::uint32_t)
{
}

void CShellSurfaceWlShell::Initialize()
{
  // Nothing to do here - constructor already handles it
  // This is not a problem since the constructor is guaranteed not to call
  // handler functions since the event loop is not running.
}

void CShellSurfaceWlShell::SetFullScreen(const wayland::output_t& output, float refreshRate)
{
  m_shellSurface.set_fullscreen(wayland::shell_surface_fullscreen_method::driver, std::round(refreshRate * 1000.0f), output);
  m_surfaceState.set(STATE_FULLSCREEN);
}

void CShellSurfaceWlShell::SetWindowed()
{
  m_shellSurface.set_toplevel();
  m_surfaceState.reset(STATE_FULLSCREEN);
}

void CShellSurfaceWlShell::SetMaximized()
{
  m_shellSurface.set_maximized(wayland::output_t());
  m_surfaceState.set(STATE_MAXIMIZED);
}

void CShellSurfaceWlShell::UnsetMaximized()
{
  m_surfaceState.reset(STATE_MAXIMIZED);
}

void CShellSurfaceWlShell::SetMinimized()
{
}

void CShellSurfaceWlShell::SetWindowGeometry(CRectInt)
{
}

void CShellSurfaceWlShell::StartMove(const wayland::seat_t& seat, std::uint32_t serial)
{
  m_shellSurface.move(seat, serial);
}

void CShellSurfaceWlShell::StartResize(const wayland::seat_t& seat, std::uint32_t serial, wayland::shell_surface_resize edge)
{
  m_shellSurface.resize(seat, serial, edge);
}

void CShellSurfaceWlShell::ShowShellContextMenu(const wayland::seat_t&, std::uint32_t, CPointInt)
{
}
