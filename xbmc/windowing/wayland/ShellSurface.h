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

#pragma once

#include <bitset>
#include <cstdint>

#include <wayland-client.hpp>

#include "utils/Geometry.h"

namespace KODI
{
namespace WINDOWING
{
namespace WAYLAND
{

class IShellSurfaceHandler;

/**
 * Abstraction for shell surfaces to support multiple protocols
 * such as wl_shell (for compatibility) and xdg_shell (for features)
 *
 * The interface itself is modeled after xdg_shell, so see there for the meaning
 * of e.g. the surface states
 */
class IShellSurface
{
public:
  // Not enum class since it must be used like a bitfield
  enum State
  {
    STATE_MAXIMIZED = 0,
    STATE_FULLSCREEN,
    STATE_RESIZING,
    STATE_ACTIVATED,
    STATE_COUNT
  };
  using StateBitset = std::bitset<STATE_COUNT>;
  static std::string StateToString(StateBitset state);

  /**
   * Initialize shell surface
   *
   * The event loop thread MUST NOT be running when this function is called.
   * The difference to the constructor is that in this function callbacks may
   * already be called.
   */
  virtual void Initialize() = 0;

  virtual void SetFullScreen(wayland::output_t const& output, float refreshRate) = 0;
  virtual void SetWindowed() = 0;
  virtual void SetMaximized() = 0;
  virtual void UnsetMaximized() = 0;
  virtual void SetMinimized() = 0;
  virtual void SetWindowGeometry(CRectInt geometry) = 0;

  virtual void AckConfigure(std::uint32_t serial) = 0;

  virtual void StartMove(wayland::seat_t const& seat, std::uint32_t serial) = 0;
  virtual void StartResize(wayland::seat_t const& seat, std::uint32_t serial, wayland::shell_surface_resize edge) = 0;
  virtual void ShowShellContextMenu(wayland::seat_t const& seat, std::uint32_t serial, CPointInt position) = 0;

  virtual ~IShellSurface() = default;

protected:
  IShellSurface() noexcept = default;

private:
  IShellSurface(IShellSurface const& other) = delete;
  IShellSurface& operator=(IShellSurface const& other) = delete;
};

class IShellSurfaceHandler
{
public:
  virtual void OnConfigure(std::uint32_t serial, CSizeInt size, IShellSurface::StateBitset state) = 0;
  virtual void OnClose() = 0;

  virtual ~IShellSurfaceHandler() = default;
};

}
}
}
