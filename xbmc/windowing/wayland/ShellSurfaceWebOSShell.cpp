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

namespace
{
/*
  WebOS always is 1920x1080
*/
constexpr int WEBOS_UI_WIDTH = 1920;
constexpr int WEBOS_UI_HEIGHT = 1080;
} // unnamed namespace

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

  m_webos_shellSurface.on_state_changed() = [this](std::uint32_t state) {
    switch (state)
    {
      case static_cast<uint32_t>(webos_shell_surface_state::fullscreen):
        CLog::Log(LOGDEBUG, "webOS notification - Changed to full screen");
        m_handler.OnConfigure(0, {WEBOS_UI_WIDTH, WEBOS_UI_HEIGHT}, m_surfaceState);
        break;
      case static_cast<uint32_t>(webos_shell_surface_state::minimized):
        CLog::Log(LOGDEBUG, "webOS notification - Changed to minimized");
        break;
    }
  };

  m_webos_shellSurface.on_close() = [this]() {
    CLog::Log(LOGDEBUG, "webOS notification - Close notification received");
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

  m_surfaceState.set(STATE_ACTIVATED);
  m_shellSurface.set_class(className);
  m_shellSurface.set_title(title);
}

void CShellSurfaceWebOSShell::SetFullScreen(const wayland::output_t& output, float refreshRate)
{
  m_shellSurface.set_fullscreen(wayland::shell_surface_fullscreen_method::driver,
                                std::round(refreshRate * 1000.0f), output);
  m_surfaceState.set(STATE_FULLSCREEN);
}

void CShellSurfaceWebOSShell::SetWindowed()
{
  m_shellSurface.set_toplevel();
  m_surfaceState.reset(STATE_FULLSCREEN);
}

void CShellSurfaceWebOSShell::SetMaximized()
{
  m_shellSurface.set_maximized(wayland::output_t());
  m_surfaceState.set(STATE_MAXIMIZED);
}

void CShellSurfaceWebOSShell::UnsetMaximized()
{
  m_surfaceState.reset(STATE_MAXIMIZED);
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
