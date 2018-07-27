/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
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
