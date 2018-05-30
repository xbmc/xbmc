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
#include <map>

#include <wayland-client-protocol.hpp>

#include "input/touch/ITouchInputHandler.h"

namespace KODI
{
namespace WINDOWING
{
namespace WAYLAND
{

/**
 * Touch input processor
 *
 * Events go directly to \ref CGenericTouchInputHandler, so no callbacks here
 */
class CInputProcessorTouch
{
public:
  CInputProcessorTouch(wayland::touch_t const& touch, wayland::surface_t const& surface);
  ~CInputProcessorTouch() noexcept;
  void SetCoordinateScale(std::int32_t scale) { m_coordinateScale = scale; }

private:
  CInputProcessorTouch(CInputProcessorTouch const& other) = delete;
  CInputProcessorTouch& operator=(CInputProcessorTouch const& other) = delete;

  struct TouchPoint
  {
    std::uint32_t lastEventTime;
    /// Pointer number passed to \ref ITouchInputHandler
    std::int32_t kodiPointerNumber;
    /**
     * Last coordinates - needed for TouchInputUp events where Wayland does not
     * send new coordinates but Kodi needs them anyway
     */
    float x, y, size;
    TouchPoint(std::uint32_t initialEventTime, std::int32_t kodiPointerNumber, float x, float y, float size)
    : lastEventTime{initialEventTime}, kodiPointerNumber{kodiPointerNumber}, x{x}, y{y}, size{size}
    {}
  };

  void SendTouchPointEvent(TouchInput event, TouchPoint const& point);
  void UpdateTouchPoint(TouchPoint const& point);
  void AbortTouches();

  wayland::touch_t m_touch;
  wayland::surface_t m_surface;
  std::int32_t m_coordinateScale{1};

  /// Map of wl_touch point id to data
  std::map<std::int32_t, TouchPoint> m_touchPoints;
};

}
}
}
