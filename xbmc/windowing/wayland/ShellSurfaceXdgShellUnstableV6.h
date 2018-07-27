/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "Connection.h"
#include "ShellSurface.h"

#include <wayland-extra-protocols.hpp>

namespace KODI
{
namespace WINDOWING
{
namespace WAYLAND
{

/**
 * Shell surface implementation for unstable xdg_shell in version 6
 *
 * xdg_shell was accepted as a stable protocol in wayland-protocols, which
 * means this class is deprecated and can be safely removed once the relevant
 * compositors have made the switch.
 */
class CShellSurfaceXdgShellUnstableV6 : public IShellSurface
{
public:
  /**
   * Construct xdg_shell toplevel object for given surface
   *
   * \param handler the shell surface handler
   * \param display the wl_display global (for initial roundtrip)
   * \param shell the zxdg_shell_v6 global
   * \param surface surface to make shell surface for
   * \param title title of the surfae
   * \param class_ class of the surface, which should match the name of the
   *               .desktop file of the application
   */
  CShellSurfaceXdgShellUnstableV6(IShellSurfaceHandler& handler, wayland::display_t& display, wayland::zxdg_shell_v6_t const& shell, wayland::surface_t const& surface, std::string const& title, std::string const& class_);
  virtual ~CShellSurfaceXdgShellUnstableV6() noexcept;

  static CShellSurfaceXdgShellUnstableV6* TryCreate(IShellSurfaceHandler& handler, CConnection& connection, wayland::surface_t const& surface, std::string const& title, std::string const& class_);

  void Initialize() override;

  void SetFullScreen(wayland::output_t const& output, float refreshRate) override;
  void SetWindowed() override;
  void SetMaximized() override;
  void UnsetMaximized() override;
  void SetMinimized() override;
  void SetWindowGeometry(CRectInt geometry) override;
  void AckConfigure(std::uint32_t serial) override;

  void StartMove(const wayland::seat_t& seat, std::uint32_t serial) override;
  void StartResize(const wayland::seat_t& seat, std::uint32_t serial, wayland::shell_surface_resize edge) override;
  void ShowShellContextMenu(const wayland::seat_t& seat, std::uint32_t serial, CPointInt position) override;

private:
  IShellSurfaceHandler& m_handler;
  wayland::display_t& m_display;
  wayland::zxdg_shell_v6_t m_shell;
  wayland::surface_t m_surface;
  wayland::zxdg_surface_v6_t m_xdgSurface;
  wayland::zxdg_toplevel_v6_t m_xdgToplevel;

  CSizeInt m_configuredSize;
  StateBitset m_configuredState;
};

}
}
}
