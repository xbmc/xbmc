/*
 *      Copyright (C) 2017-present Team Kodi
 *      This file is part of Kodi - https://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
 *
 */
#pragma once

#include <cstdint>

#include <wayland-client-protocol.hpp>

#include "input/XBMC_keysym.h"
#include "utils/Geometry.h"
#include "windowing/XBMC_events.h"

namespace KODI
{
namespace WINDOWING
{
namespace WAYLAND
{

class IInputHandlerPointer
{
public:
  virtual void OnPointerEnter(wayland::pointer_t& pointer, std::uint32_t serial) {};
  virtual void OnPointerLeave() {};
  virtual void OnPointerEvent(XBMC_Event& event) = 0;
  virtual ~IInputHandlerPointer() = default;
};

class CInputProcessorPointer
{
public:
  CInputProcessorPointer(wayland::pointer_t const& pointer, wayland::surface_t const& surface, IInputHandlerPointer& handler);
  void SetCoordinateScale(std::int32_t scale) { m_coordinateScale = scale; }

private:
  CInputProcessorPointer(CInputProcessorPointer const& other) = delete;
  CInputProcessorPointer& operator=(CInputProcessorPointer const& other) = delete;

  std::uint16_t ConvertMouseCoordinate(double coord) const;
  void SetMousePosFromSurface(CPointGen<double> position);
  void SendMouseMotion();
  void SendMouseButton(unsigned char button, bool pressed);

  wayland::pointer_t m_pointer;
  wayland::surface_t m_surface;
  IInputHandlerPointer& m_handler;

  bool m_pointerOnSurface{};

  // Pointer position in *scaled* coordinates
  CPointGen<std::uint16_t> m_pointerPosition;
  std::int32_t m_coordinateScale{1};
};

}
}
}