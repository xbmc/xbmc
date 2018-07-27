/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "InputProcessorTouch.h"

#include "input/touch/generic/GenericTouchInputHandler.h"

using namespace KODI::WINDOWING::WAYLAND;

CInputProcessorTouch::CInputProcessorTouch(wayland::touch_t const& touch, wayland::surface_t const& surface)
: m_touch{touch}, m_surface{surface}
{
  m_touch.on_down() = [this](std::uint32_t, std::uint32_t time, wayland::surface_t surface, std::int32_t id, double x, double y)
  {
    if (surface != m_surface)
    {
      return;
    }

    // Find free Kodi pointer number
    int kodiPointer{-1};
    // Not optimal, but irrelevant for the small number of iterations
    for (int testPointer{0}; testPointer < CGenericTouchInputHandler::MAX_POINTERS; testPointer++)
    {
      if (std::all_of(m_touchPoints.cbegin(), m_touchPoints.cend(),
                      [=](decltype(m_touchPoints)::value_type const& pair)
                      {
                        return (pair.second.kodiPointerNumber != testPointer);
                      }))
      {
        kodiPointer = testPointer;
        break;
      }
    }

    if (kodiPointer != -1)
    {
      auto it = m_touchPoints.emplace(std::piecewise_construct, std::forward_as_tuple(id), std::forward_as_tuple(time, kodiPointer, x * m_coordinateScale, y * m_coordinateScale, 0.0f)).first;
      SendTouchPointEvent(TouchInputDown, it->second);
    }
  };
  m_touch.on_up() = [this](std::uint32_t, std::uint32_t time, std::int32_t id)
  {
    auto it = m_touchPoints.find(id);
    if (it != m_touchPoints.end())
    {
      auto& point = it->second;
      point.lastEventTime = time;
      SendTouchPointEvent(TouchInputUp, point);
      m_touchPoints.erase(it);
    }
  };
  m_touch.on_motion() = [this](std::uint32_t time, std::int32_t id, double x, double y)
  {
    auto it = m_touchPoints.find(id);
    if (it != m_touchPoints.end())
    {
      auto& point = it->second;
      point.x = x * m_coordinateScale;
      point.y = y * m_coordinateScale;
      point.lastEventTime = time;
      SendTouchPointEvent(TouchInputMove, point);
    }
  };
  m_touch.on_cancel() = [this]()
  {
    AbortTouches();
  };
  m_touch.on_shape() = [this](std::int32_t id, double major, double minor)
  {
    auto it = m_touchPoints.find(id);
    if (it != m_touchPoints.end())
    {
      auto& point = it->second;
      // Kodi only supports size without shape, so use average of both axes
      point.size = ((major + minor) / 2.0) * m_coordinateScale;
      UpdateTouchPoint(point);
    }
  };
}

CInputProcessorTouch::~CInputProcessorTouch() noexcept
{
  AbortTouches();
}

void CInputProcessorTouch::AbortTouches()
{
  // TouchInputAbort aborts for all pointers, so it does not matter which is specified
  if (!m_touchPoints.empty())
  {
    SendTouchPointEvent(TouchInputAbort, m_touchPoints.begin()->second);
  }
  m_touchPoints.clear();
}

void CInputProcessorTouch::SendTouchPointEvent(TouchInput event, const TouchPoint& point)
{
  if (event == TouchInputMove)
  {
    for (auto const& point : m_touchPoints)
    {
      // Contrary to the docs, this must be called before HandleTouchInput or the
      // position will not be updated and gesture detection will not work
      UpdateTouchPoint(point.second);
    }
  }
  CGenericTouchInputHandler::GetInstance().HandleTouchInput(event, point.x, point.y, point.lastEventTime * INT64_C(1000000), point.kodiPointerNumber, point.size);
}

void CInputProcessorTouch::UpdateTouchPoint(const TouchPoint& point)
{
  CGenericTouchInputHandler::GetInstance().UpdateTouchPointer(point.kodiPointerNumber, point.x, point.y, point.lastEventTime * INT64_C(1000000), point.size);
}
