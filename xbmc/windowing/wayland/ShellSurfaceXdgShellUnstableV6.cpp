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

#include "ShellSurfaceXdgShellUnstableV6.h"

#include "messaging/ApplicationMessenger.h"
#include "Registry.h"

using namespace KODI::WINDOWING::WAYLAND;

namespace
{

IShellSurface::State ConvertStateFlag(wayland::zxdg_toplevel_v6_state flag)
{
  switch(flag)
  {
    case wayland::zxdg_toplevel_v6_state::activated:
      return IShellSurface::STATE_ACTIVATED;
    case wayland::zxdg_toplevel_v6_state::fullscreen:
      return IShellSurface::STATE_FULLSCREEN;
    case wayland::zxdg_toplevel_v6_state::maximized:
      return IShellSurface::STATE_MAXIMIZED;
    case wayland::zxdg_toplevel_v6_state::resizing:
      return IShellSurface::STATE_RESIZING;
    default:
      throw std::runtime_error(std::string("Unknown xdg_toplevel state flag ") + std::to_string(static_cast<std::underlying_type<decltype(flag)>::type> (flag)));
  }
}

}

CShellSurfaceXdgShellUnstableV6* CShellSurfaceXdgShellUnstableV6::TryCreate(IShellSurfaceHandler& handler, CConnection& connection, const wayland::surface_t& surface, std::string const& title, std::string const& class_)
{
  wayland::zxdg_shell_v6_t shell;
  CRegistry registry{connection};
  registry.RequestSingleton(shell, 1, 1, false);
  registry.Bind();

  if (shell)
  {
    return new CShellSurfaceXdgShellUnstableV6(handler, connection.GetDisplay(), shell, surface, title, class_);
  }
  else
  {
    return nullptr;
  }
}

CShellSurfaceXdgShellUnstableV6::CShellSurfaceXdgShellUnstableV6(IShellSurfaceHandler& handler, wayland::display_t& display, const wayland::zxdg_shell_v6_t& shell, const wayland::surface_t& surface, std::string const& title, std::string const& app_id)
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
  m_xdgToplevel.on_configure() = [this](std::int32_t width, std::int32_t height, std::vector<wayland::zxdg_toplevel_v6_state> states)
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

void CShellSurfaceXdgShellUnstableV6::Initialize()
{
  // Commit surface to confirm role
  // Don't do it in constructor since SetFullScreen might be called before
  m_surface.commit();
  // Make sure we get the initial configure before continuing
  m_display.roundtrip();
}

void CShellSurfaceXdgShellUnstableV6::AckConfigure(std::uint32_t serial)
{
  m_xdgSurface.ack_configure(serial);
}

CShellSurfaceXdgShellUnstableV6::~CShellSurfaceXdgShellUnstableV6() noexcept
{
  // xdg_shell is picky: must destroy toplevel role before surface
  m_xdgToplevel.proxy_release();
  m_xdgSurface.proxy_release();
}

void CShellSurfaceXdgShellUnstableV6::SetFullScreen(const wayland::output_t& output, float)
{
  // xdg_shell does not support refresh rate setting at the moment
  m_xdgToplevel.set_fullscreen(output);
}

void CShellSurfaceXdgShellUnstableV6::SetWindowed()
{
  m_xdgToplevel.unset_fullscreen();
}

void CShellSurfaceXdgShellUnstableV6::SetMaximized()
{
  m_xdgToplevel.set_maximized();
}

void CShellSurfaceXdgShellUnstableV6::UnsetMaximized()
{
  m_xdgToplevel.unset_maximized();
}

void CShellSurfaceXdgShellUnstableV6::SetMinimized()
{
  m_xdgToplevel.set_minimized();
}

void CShellSurfaceXdgShellUnstableV6::SetWindowGeometry(CRectInt geometry)
{
  m_xdgSurface.set_window_geometry(geometry.x1, geometry.y1, geometry.Width(), geometry.Height());
}

void CShellSurfaceXdgShellUnstableV6::StartMove(const wayland::seat_t& seat, std::uint32_t serial)
{
  m_xdgToplevel.move(seat, serial);
}

void CShellSurfaceXdgShellUnstableV6::StartResize(const wayland::seat_t& seat, std::uint32_t serial, wayland::shell_surface_resize edge)
{
  // wl_shell shell_surface_resize is identical to xdg_shell resize_edge
  m_xdgToplevel.resize(seat, serial, static_cast<std::uint32_t> (edge));
}

void CShellSurfaceXdgShellUnstableV6::ShowShellContextMenu(const wayland::seat_t& seat, std::uint32_t serial, CPointInt position)
{
  m_xdgToplevel.show_window_menu(seat, serial, position.x, position.y);
}