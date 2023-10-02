/*
 *  Copyright (C) 2017-2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ShellSurfaceWebOSShell.h"

#include "CompileInfo.h"
#include "Registry.h"
#include "platform/Environment.h"
#include "utils/log.h"

using namespace KODI::WINDOWING::WAYLAND;
using namespace std::placeholders;

using namespace wayland;

CShellSurfaceWebOSShell::CShellSurfaceWebOSShell(IShellSurfaceHandler& handler,
                                                 CConnection& connection,
                                                 const wayland::surface_t& surface,
                                                 const std::string& title,
                                                 const std::string& className)
  : m_handler{handler}
{
  {
    CRegistry registry{connection};
    registry.RequestSingleton(m_shell, 1, 1);
    registry.RequestSingleton(m_webos_shell, 1, 2);
    registry.Bind();
  }

  m_shellSurface = m_shell.get_shell_surface(surface);

  m_webos_shellSurface = m_webos_shell.get_shell_surface(surface);

  m_webos_shellSurface.on_exposed() = [this](const std::vector<std::int32_t>& rect) {
    if (rect.size() >= 4)
      m_windowSize = {rect[2], rect[3]};
  };

  m_webos_shellSurface.on_state_changed() = [this](std::uint32_t state) {
    switch (static_cast<webos_shell_surface_state>(state))
    {
      case webos_shell_surface_state::fullscreen:
        CLog::Log(LOGDEBUG, "CShellSurfaceWebOSShell: State changed to full screen");
        m_surfaceState.reset();
        m_surfaceState.set(STATE_ACTIVATED);
        m_surfaceState.set(STATE_FULLSCREEN);
        m_handler.OnConfigure(0, m_windowSize, m_surfaceState);
        break;
      case webos_shell_surface_state::maximized:
        CLog::Log(LOGDEBUG, "CShellSurfaceWebOSShell: State changed to maximized");
        m_surfaceState.reset();
        m_surfaceState.set(STATE_ACTIVATED);
        m_surfaceState.set(STATE_MAXIMIZED);
        m_handler.OnConfigure(0, m_windowSize, m_surfaceState);
        break;
      case webos_shell_surface_state::minimized:
        CLog::Log(LOGDEBUG, "CShellSurfaceWebOSShell: State changed to minimized");
        m_surfaceState.reset();
        m_handler.OnConfigure(0, m_windowSize, m_surfaceState);
        break;
      case webos_shell_surface_state::_default:
        CLog::Log(LOGDEBUG, "CShellSurfaceWebOSShell: State changed to default (windowed)");
        m_surfaceState.reset();
        m_surfaceState.set(STATE_ACTIVATED);
        break;
    }
  };

  m_webos_shellSurface.on_close() = [this]() {
    CLog::Log(LOGDEBUG, "CShellSurfaceWebOSShell: Close notification received");
    m_handler.OnClose();
  };

  std::string appId = CEnvironment::getenv("APP_ID");
  if (appId.empty())
  {
    appId = CCompileInfo::GetPackage();
  }

  std::string displayId = CEnvironment::getenv("DISPLAY_ID");
  if (displayId.empty())
  {
    displayId = "0";
  }

  CLog::Log(LOGDEBUG, "Passing appId {} and displayAffinity {} to wl_webos_shell", appId,
            displayId);
  m_webos_shellSurface.set_property("appId", appId);
  m_webos_shellSurface.set_property("displayAffinity", displayId);

  // Allow the back button the LG remote to be passed to Kodi and not intercepted
  m_webos_shellSurface.set_property("_WEBOS_ACCESS_POLICY_KEYS_BACK", "true");

  m_shellSurface.set_class(className);
  m_shellSurface.set_title(title);
}

void CShellSurfaceWebOSShell::SetFullScreen(const wayland::output_t& output, float refreshRate)
{
  m_webos_shellSurface.set_state(static_cast<std::uint32_t>(webos_shell_surface_state::fullscreen));
}

void CShellSurfaceWebOSShell::SetWindowed()
{
  m_shellSurface.set_toplevel();
}

void CShellSurfaceWebOSShell::SetMaximized()
{
  m_webos_shellSurface.set_state(static_cast<std::uint32_t>(webos_shell_surface_state::maximized));
}

void CShellSurfaceWebOSShell::UnsetMaximized()
{
}

void CShellSurfaceWebOSShell::SetMinimized()
{
  m_webos_shellSurface.set_state(static_cast<std::uint32_t>(webos_shell_surface_state::minimized));
}

void CShellSurfaceWebOSShell::StartMove(const wayland::seat_t& seat, std::uint32_t serial)
{
  m_shellSurface.move(seat, serial);
}

void CShellSurfaceWebOSShell::StartResize(const wayland::seat_t& seat,
                                          std::uint32_t serial,
                                          wayland::shell_surface_resize edge)
{
  m_shellSurface.resize(seat, serial, edge);
}
