/*
 *      Copyright (C) 2017 Team XBMC
 *      http://xbmc.org
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

#include "InputProcessorPointer.h"

#include <cmath>

#include <linux/input-event-codes.h>

#include "input/MouseStat.h"

using namespace KODI::WINDOWING::WAYLAND;

namespace
{

int WaylandToXbmcButton(std::uint32_t button)
{
  // Wayland button is evdev code
  switch (button)
  {
    case BTN_LEFT:
      return XBMC_BUTTON_LEFT;
    case BTN_MIDDLE:
      return XBMC_BUTTON_MIDDLE;
    case BTN_RIGHT:
      return XBMC_BUTTON_RIGHT;
    default:
      return -1;
  }
}

}

CInputProcessorPointer::CInputProcessorPointer(wayland::pointer_t const& pointer, wayland::surface_t const& surface, IInputHandlerPointer& handler)
: m_pointer{pointer}, m_surface{surface}, m_handler{handler}
{
  m_pointer.on_enter() = [this](std::uint32_t serial, wayland::surface_t surface, double surfaceX, double surfaceY)
  {
    if (surface == m_surface)
    {
      m_pointerOnSurface = true;
      m_handler.OnPointerEnter(m_pointer, serial);
      SetMousePosFromSurface({surfaceX, surfaceY});
      SendMouseMotion();
    }
  };
  m_pointer.on_leave() = [this](std::uint32_t serial, wayland::surface_t surface)
  {
    if (m_pointerOnSurface)
    {
      m_handler.OnPointerLeave();
      m_pointerOnSurface = false;
    }
  };
  m_pointer.on_motion() = [this](std::uint32_t time, double surfaceX, double surfaceY)
  {
    if (m_pointerOnSurface)
    {
      SetMousePosFromSurface({surfaceX, surfaceY});
      SendMouseMotion();
    }
  };
  m_pointer.on_button() = [this](std::uint32_t serial, std::uint32_t time, std::uint32_t button, wayland::pointer_button_state state)
  {
    if (m_pointerOnSurface)
    {
      int xbmcButton = WaylandToXbmcButton(button);
      if (xbmcButton < 0)
      {
        // Button is unmapped
        return;
      }

      bool pressed = (state == wayland::pointer_button_state::pressed);
      SendMouseButton(xbmcButton, pressed);
    }
  };
  m_pointer.on_axis() = [this](std::uint32_t, wayland::pointer_axis, double value)
  {
    if (m_pointerOnSurface)
    {
      // For axis events we only care about the vector direction
      // and not the scalar magnitude. Every axis event callback
      // generates one scroll button event for XBMC

      // Negative is up
      auto xbmcButton = static_cast<unsigned char> ((value < 0.0) ? XBMC_BUTTON_WHEELUP : XBMC_BUTTON_WHEELDOWN);
      // Simulate a single click of the wheel-equivalent "button"
      SendMouseButton(xbmcButton, true);
      SendMouseButton(xbmcButton, false);
    }
  };

  // Wayland groups pointer events, but right now there is no benefit in
  // treating them in groups. The main use case for doing so seems to be
  // multi-axis (i.e. diagnoal) scrolling, but we do not support this anyway.
  /*m_pointer.on_frame() = [this]()
  {

  };*/
}

std::uint16_t CInputProcessorPointer::ConvertMouseCoordinate(double coord) const
{
  return static_cast<std::uint16_t> (std::round(coord * m_coordinateScale));
}

void CInputProcessorPointer::SetMousePosFromSurface(CPointGen<double> position)
{
  m_pointerPosition = {ConvertMouseCoordinate(position.x), ConvertMouseCoordinate(position.y)};
}

void CInputProcessorPointer::SendMouseMotion()
{
  XBMC_Event event{XBMC_MOUSEMOTION};
  event.motion = {m_pointerPosition.x, m_pointerPosition.y};
  m_handler.OnPointerEvent(event);
}

void CInputProcessorPointer::SendMouseButton(unsigned char button, bool pressed)
{
  XBMC_Event event{static_cast<unsigned char> (pressed ? XBMC_MOUSEBUTTONDOWN : XBMC_MOUSEBUTTONUP)};
  event.button = {button, m_pointerPosition.x, m_pointerPosition.y};
  m_handler.OnPointerEvent(event);
}