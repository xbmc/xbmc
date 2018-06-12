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

#include <cstdint>

#include <wayland-client-protocol.hpp>

namespace KODI
{
namespace WINDOWING
{
namespace WAYLAND
{

/**
 * Handler for reacting to events originating in window decorations, such as
 * moving the window by clicking and dragging
 */
class IWindowDecorationHandler
{
public:
  virtual void OnWindowMove(wayland::seat_t const& seat, std::uint32_t serial) = 0;
  virtual void OnWindowResize(wayland::seat_t const& seat, std::uint32_t serial, wayland::shell_surface_resize edge) = 0;
  virtual void OnWindowShowContextMenu(wayland::seat_t const& seat, std::uint32_t serial, CPointInt position) = 0;
  virtual void OnWindowMinimize() = 0;
  virtual void OnWindowMaximize() = 0;
  virtual void OnWindowClose() = 0;

  virtual ~IWindowDecorationHandler() = default;
};

}
}
}
