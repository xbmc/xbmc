/*
 *  Copyright (C) 2017-2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "Connection.h"
#include "ShellSurface.h"
#include "wayland-webos-protocols.hpp"

namespace KODI
{
namespace WINDOWING
{
namespace WAYLAND
{

class CShellSurfaceWebOSShell : public IShellSurface
{
public:
  /**
   * Construct wl_shell_surface for given surface
   *
   * \parma handler shell surface handler
   * \param connection connection global
   * \param surface surface to make shell surface for
   * \param title title of the surfae
   * \param className class of the surface, which should match the name of the
   *               .desktop file of the application
   */
  CShellSurfaceWebOSShell(IShellSurfaceHandler& handler,
                          CConnection& connection,
                          wayland::surface_t const& surface,
                          const std::string& title,
                          const std::string& className);

  void Initialize() override{};

  void SetFullScreen(wayland::output_t const& output, float refreshRate) override;
  void SetWindowed() override;
  void SetMaximized() override;
  void UnsetMaximized() override;
  void SetMinimized() override;
  void SetWindowGeometry(CRectInt geometry) override{};
  void AckConfigure(std::uint32_t serial) override{};

  void StartMove(const wayland::seat_t& seat, std::uint32_t serial) override;
  void StartResize(const wayland::seat_t& seat,
                   std::uint32_t serial,
                   wayland::shell_surface_resize edge) override;
  void ShowShellContextMenu(const wayland::seat_t& seat,
                            std::uint32_t serial,
                            CPointInt position) override{};

private:
  IShellSurfaceHandler& m_handler;
  wayland::shell_t m_shell;
  wayland::shell_surface_t m_shellSurface;
  StateBitset m_surfaceState;
  wayland::webos_shell_t m_webos_shell;
  wayland::webos_shell_surface_t m_webos_shellSurface;
  CSizeInt m_windowSize;
};

} // namespace WAYLAND
} // namespace WINDOWING
} // namespace KODI
