/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "Seat.h"
#include "input/keyboard/XBMC_keysym.h"
#include "utils/Geometry.h"
#include "windowing/XBMC_events.h"

#include <cstdint>

#include <wayland-client-protocol.hpp>

namespace KODI
{
namespace WINDOWING
{
namespace WAYLAND
{

class IInputHandlerPointer
{
public:
  virtual void OnPointerEnter(std::uint32_t seatGlobalName, std::uint32_t serial) {}
  virtual void OnPointerLeave() {}
  virtual void OnPointerEvent(XBMC_Event& event) = 0;
protected:
  ~IInputHandlerPointer() = default;
};

class CInputProcessorPointer final : public IRawInputHandlerPointer
{
public:
  CInputProcessorPointer(wayland::surface_t const& surface, IInputHandlerPointer& handler);
  void SetCoordinateScale(std::int32_t scale) { m_coordinateScale = scale; }

  void OnPointerEnter(CSeat* seat,
                      std::uint32_t serial,
                      const wayland::surface_t& surface,
                      double surfaceX,
                      double surfaceY) override;
  void OnPointerLeave(CSeat* seat,
                      std::uint32_t serial,
                      const wayland::surface_t& surface) override;
  void OnPointerMotion(CSeat* seat, std::uint32_t time, double surfaceX, double surfaceY) override;
  void OnPointerButton(CSeat* seat, std::uint32_t serial, std::uint32_t time, std::uint32_t button, wayland::pointer_button_state state) override;
  void OnPointerAxis(CSeat* seat, std::uint32_t time, wayland::pointer_axis axis, double value) override;

private:
  CInputProcessorPointer(CInputProcessorPointer const& other) = delete;
  CInputProcessorPointer& operator=(CInputProcessorPointer const& other) = delete;

  std::uint16_t ConvertMouseCoordinate(double coord) const;
  void SetMousePosFromSurface(CPointGen<double> position);
  void SendMouseMotion();
  void SendMouseButton(unsigned char button, bool pressed);

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
