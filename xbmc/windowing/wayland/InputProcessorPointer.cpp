/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "InputProcessorPointer.h"

#include "input/mouse/MouseStat.h"

#include <cmath>

#include <linux/input.h>

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

CInputProcessorPointer::CInputProcessorPointer(wayland::surface_t const& surface, IInputHandlerPointer& handler)
: m_surface{surface}, m_handler{handler}
{
}

void CInputProcessorPointer::OnPointerEnter(CSeat* seat,
                                            std::uint32_t serial,
                                            const wayland::surface_t& surface,
                                            double surfaceX,
                                            double surfaceY)
{
  if (surface == m_surface)
  {
    m_pointerOnSurface = true;
    m_handler.OnPointerEnter(seat->GetGlobalName(), serial);
    SetMousePosFromSurface({surfaceX, surfaceY});
    SendMouseMotion();
  }
}

void CInputProcessorPointer::OnPointerLeave(CSeat* seat,
                                            std::uint32_t serial,
                                            const wayland::surface_t& surface)
{
  if (m_pointerOnSurface)
  {
    m_handler.OnPointerLeave();
    m_pointerOnSurface = false;
  }
}

void CInputProcessorPointer::OnPointerMotion(CSeat* seat, std::uint32_t time, double surfaceX, double surfaceY)
{
  if (m_pointerOnSurface)
  {
    SetMousePosFromSurface({surfaceX, surfaceY});
    SendMouseMotion();
  }
}

void CInputProcessorPointer::OnPointerButton(CSeat* seat, std::uint32_t serial, std::uint32_t time, std::uint32_t button, wayland::pointer_button_state state)
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
}

void CInputProcessorPointer::OnPointerAxis(CSeat* seat, std::uint32_t time, wayland::pointer_axis axis, double value)
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
  XBMC_Event event{};
  event.type = XBMC_MOUSEMOTION;
  event.motion = {m_pointerPosition.x, m_pointerPosition.y};
  m_handler.OnPointerEvent(event);
}

void CInputProcessorPointer::SendMouseButton(unsigned char button, bool pressed)
{
  XBMC_Event event{};
  event.type = pressed ? XBMC_MOUSEBUTTONDOWN : XBMC_MOUSEBUTTONUP;
  event.button = {button, m_pointerPosition.x, m_pointerPosition.y};
  m_handler.OnPointerEvent(event);
}
