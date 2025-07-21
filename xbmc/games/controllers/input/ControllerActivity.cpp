/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ControllerActivity.h"

#include "application/Application.h"
#include "input/InputTranslator.h"

using namespace std::chrono_literals;

using namespace KODI;
using namespace GAME;

#include <algorithm>
#include <cstdlib>

namespace
{
// Chosen small for high responsiveness
constexpr auto MOUSE_MOTION_TIMEOUT = 50ms;
} // namespace

float CControllerActivity::GetActivation() const
{
  // De-activate mouse if an event hasn't been sent since last motion and
  // no mouse button is pressed
  if (!m_mousePointers.empty() && m_activeMouseButtons.empty())
  {
    const bool anyPointerActive =
        std::ranges::any_of(m_mousePointers, [](const auto& it)
                            { return it.second.active && !it.second.timer.IsTimePast(); });
    if (!anyPointerActive)
      return 0.0f;
  }

  return m_lastActivation;
}

void CControllerActivity::ClearButtonState()
{
  m_lastActivation = 0.0f;
  m_currentActivation = 0.0f;
  m_activeKey.clear();
  m_mousePointers.clear();
  m_activeMouseButtons.clear();
  m_bKeyPressed = false;
}

void CControllerActivity::OnButtonPress(bool pressed)
{
  if (pressed)
    m_currentActivation = 1.0f;
}

void CControllerActivity::OnButtonMotion(float magnitude)
{
  m_currentActivation = std::max(magnitude, m_currentActivation);
}

void CControllerActivity::OnAnalogStickMotion(float x, float y)
{
  m_currentActivation = std::max(std::abs(x), m_currentActivation);
  m_currentActivation = std::max(std::abs(y), m_currentActivation);
}

void CControllerActivity::OnWheelMotion(float position)
{
  m_currentActivation = std::max(std::abs(position), m_currentActivation);
}

void CControllerActivity::OnThrottleMotion(float position)
{
  m_currentActivation = std::max(std::abs(position), m_currentActivation);
}

void CControllerActivity::OnKeyPress(const KEYBOARD::KeyName& key)
{
  // Skip the first key press, as it is usually a modifier key
  if (!m_bKeyPressed)
  {
    m_bKeyPressed = true;
    return;
  }

  // We only store a single key to avoid "stuck" keys, as any key release will
  // clear the current activation
  m_activeKey = key;
}

void CControllerActivity::OnKeyRelease(const KEYBOARD::KeyName& key)
{
  // Clear the current activation to avoid "stuck" keys
  m_activeKey.clear();
}

void CControllerActivity::OnMouseMotion(const MOUSE::PointerName& relpointer,
                                        int /*differenceX*/,
                                        int /*differenceY*/)
{
  auto& pointer = m_mousePointers[relpointer];
  pointer.active = true;
  pointer.timer.Set(MOUSE_MOTION_TIMEOUT);
}

void CControllerActivity::OnMouseButtonPress(const MOUSE::ButtonName& button)
{
  m_activeMouseButtons.insert(button);
}

void CControllerActivity::OnMouseButtonRelease(const MOUSE::ButtonName& button)
{
  m_activeMouseButtons.erase(button);
}

void CControllerActivity::OnInputFrame()
{
  if (g_application.IsAppFocused())
  {
    // Process pressed keys
    if (!m_activeKey.empty())
      m_currentActivation = 1.0f;

    // Process pressed mouse buttons
    if (!m_activeMouseButtons.empty())
      m_currentActivation = 1.0f;

    // Process mouse motion
    for (const auto& it : m_mousePointers)
    {
      const MousePointer& pointer = it.second;
      if (pointer.active && !pointer.timer.IsTimePast())
      {
        m_currentActivation = 1.0f;
        break;
      }
    }
  }

  // Process activation
  m_lastActivation = m_currentActivation;
  m_currentActivation = 0.0f;
}
