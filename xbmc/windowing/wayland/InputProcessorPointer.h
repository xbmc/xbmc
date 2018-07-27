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
