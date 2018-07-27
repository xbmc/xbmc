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
 * Shell surface implementation for stable xdg_shell
 */
class CShellSurfaceXdgShell : public IShellSurface
{
public:
  /**
   * Construct xdg_shell toplevel object for given surface
   *
   * \param handler the shell surface handler
   * \param display the wl_display global (for initial roundtrip)
   * \param shell the xdg_wm_base global
   * \param surface surface to make shell surface for
   * \param title title of the surfae
   * \param class_ class of the surface, which should match the name of the
   *               .desktop file of the application
   */
  CShellSurfaceXdgShell(IShellSurfaceHandler& handler, wayland::display_t& display, wayland::xdg_wm_base_t const& shell, wayland::surface_t const& surface, std::string const& title, std::string const& class_);
  virtual ~CShellSurfaceXdgShell() noexcept;

  static CShellSurfaceXdgShell* TryCreate(IShellSurfaceHandler& handler, CConnection& connection, wayland::surface_t const& surface, std::string const& title, std::string const& class_);

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
  wayland::xdg_wm_base_t m_shell;
  wayland::surface_t m_surface;
  wayland::xdg_surface_t m_xdgSurface;
  wayland::xdg_toplevel_t m_xdgToplevel;

  CSizeInt m_configuredSize;
  StateBitset m_configuredState;
};

}
}
}
