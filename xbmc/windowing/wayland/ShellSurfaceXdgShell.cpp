/*
 *      Copyright (C) 2017 Team XBMC
 *      http://kodi.tv
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

#include "ShellSurfaceXdgShell.h"

#include "messaging/ApplicationMessenger.h"
#include "Registry.h"

using namespace KODI::WINDOWING::WAYLAND;

namespace
{

IShellSurface::State ConvertStateFlag(wayland::xdg_toplevel_state flag)
{
  switch(flag)
  {
    case wayland::xdg_toplevel_state::activated:
      return IShellSurface::STATE_ACTIVATED;
    case wayland::xdg_toplevel_state::fullscreen:
      return IShellSurface::STATE_FULLSCREEN;
    case wayland::xdg_toplevel_state::maximized:
      return IShellSurface::STATE_MAXIMIZED;
    case wayland::xdg_toplevel_state::resizing:
      return IShellSurface::STATE_RESIZING;
    default:
      throw std::runtime_error(std::string("Unknown xdg_toplevel state flag ") + std::to_string(static_cast<std::underlying_type<decltype(flag)>::type> (flag)));
  }
}

}

CShellSurfaceXdgShell* CShellSurfaceXdgShell::TryCreate(IShellSurfaceHandler& handler, CConnection& connection, const wayland::surface_t& surface, std::string const& title, std::string const& class_)
{
  wayland::xdg_wm_base_t shell;
  CRegistry registry{connection};
  registry.RequestSingleton(shell, 1, 1, false);
  registry.Bind();

  if (shell)
  {
    return new CShellSurfaceXdgShell(handler, connection.GetDisplay(), shell, surface, title, class_);
  }
  else
  {
    return nullptr;
  }
}

CShellSurfaceXdgShell::CShellSurfaceXdgShell(IShellSurfaceHandler& handler, wayland::display_t& display, const wayland::xdg_wm_base_t& shell, const wayland::surface_t& surface, std::string const& title, std::string const& app_id)
: m_handler{handler}, m_display{display}, m_shell{shell}, m_surface{surface}, m_xdgSurface{m_shell.get_xdg_surface(m_surface)}, m_xdgToplevel{m_xdgSurface.get_toplevel()}
{
  m_shell.on_ping() = [this](std::uint32_t serial)
  {
    m_shell.pong(serial);
  };
  m_xdgSurface.on_configure() = [this](std::uint32_t serial)
  {
    m_handler.OnConfigure(serial, m_configuredSize, m_configuredState);
  };
  m_xdgToplevel.on_close() = [this]()
  {
    m_handler.OnClose();
  };
  m_xdgToplevel.on_configure() = [this](std::int32_t width, std::int32_t height, std::vector<wayland::xdg_toplevel_state> states)
  {
    m_configuredSize.Set(width, height);
    m_configuredState.reset();
    for (auto state : states)
    {
      m_configuredState.set(ConvertStateFlag(state));
    }
  };
  m_xdgToplevel.set_app_id(app_id);
  m_xdgToplevel.set_title(title);
  // Set sensible minimum size
  m_xdgToplevel.set_min_size(300, 200);
}

void CShellSurfaceXdgShell::Initialize()
{
  // Commit surface to confirm role
  // Don't do it in constructor since SetFullScreen might be called before
  m_surface.commit();
  // Make sure we get the initial configure before continuing
  m_display.roundtrip();
}

void CShellSurfaceXdgShell::AckConfigure(std::uint32_t serial)
{
  m_xdgSurface.ack_configure(serial);
}

CShellSurfaceXdgShell::~CShellSurfaceXdgShell() noexcept
{
  // xdg_shell is picky: must destroy toplevel role before surface
  m_xdgToplevel.proxy_release();
  m_xdgSurface.proxy_release();
}

void CShellSurfaceXdgShell::SetFullScreen(const wayland::output_t& output, float)
{
  // xdg_shell does not support refresh rate setting at the moment
  m_xdgToplevel.set_fullscreen(output);
}

void CShellSurfaceXdgShell::SetWindowed()
{
  m_xdgToplevel.unset_fullscreen();
}

void CShellSurfaceXdgShell::SetMaximized()
{
  m_xdgToplevel.set_maximized();
}

void CShellSurfaceXdgShell::UnsetMaximized()
{
  m_xdgToplevel.unset_maximized();
}

void CShellSurfaceXdgShell::SetMinimized()
{
  m_xdgToplevel.set_minimized();
}

void CShellSurfaceXdgShell::SetWindowGeometry(CRectInt geometry)
{
  m_xdgSurface.set_window_geometry(geometry.x1, geometry.y1, geometry.Width(), geometry.Height());
}

void CShellSurfaceXdgShell::StartMove(const wayland::seat_t& seat, std::uint32_t serial)
{
  m_xdgToplevel.move(seat, serial);
}

void CShellSurfaceXdgShell::StartResize(const wayland::seat_t& seat, std::uint32_t serial, wayland::shell_surface_resize edge)
{
  // wl_shell shell_surface_resize is identical to xdg_shell resize_edge
  m_xdgToplevel.resize(seat, serial, static_cast<std::uint32_t> (edge));
}

void CShellSurfaceXdgShell::ShowShellContextMenu(const wayland::seat_t& seat, std::uint32_t serial, CPointInt position)
{
  m_xdgToplevel.show_window_menu(seat, serial, position.x, position.y);
}
