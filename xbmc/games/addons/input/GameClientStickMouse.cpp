/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GameClientStickMouse.h"

#include "ServiceBroker.h"
#include "games/GameServices.h"
#include "games/GameSettings.h"
#include "games/addons/GameClient.h"
#include "games/addons/input/GameClientInput.h"
#include "games/controllers/ControllerIDs.h"

#include <algorithm>
#include <cmath>

using namespace KODI;
using namespace GAME;

namespace
{
constexpr auto STICK = "rightstick";
constexpr auto LEFT_BUTTON_TRIGGER = "righttrigger";
constexpr auto RIGHT_BUTTON_TRIGGER = "lefttrigger";

// Pointer speed at full deflection, in mouse units per second. The speed goes
// with the square of the deflection, for fine control near the centre.
constexpr float MAX_SPEED = 600.0f;

// A trigger that is only up or down arrives as a value that starts low and
// rises, so a light pull is a press
constexpr float PRESS_THRESHOLD = 0.25f;
constexpr float RELEASE_THRESHOLD = 0.15f;

// The stick is reported once per input frame while it's pushed
constexpr std::chrono::milliseconds FIRST_STEP{16};
constexpr std::chrono::milliseconds MAX_STEP{50};
} // namespace

CGameClientStickMouse::CGameClientStickMouse(CGameClient& gameClient) : m_gameClient(gameClient)
{
}

CGameClientStickMouse::~CGameClientStickMouse()
{
  ReleaseButtons();
}

void CGameClientStickMouse::ReleaseButtons()
{
  SetButton("left", 0.0f, m_leftPressed);
  SetButton("right", 0.0f, m_rightPressed);
}

std::string CGameClientStickMouse::ControllerID() const
{
  return DEFAULT_CONTROLLER_ID;
}

bool CGameClientStickMouse::HasFeature(const JOYSTICK::FeatureName& feature) const
{
  return feature == STICK || feature == LEFT_BUTTON_TRIGGER || feature == RIGHT_BUTTON_TRIGGER;
}

bool CGameClientStickMouse::AcceptsInput(const JOYSTICK::FeatureName& feature) const
{
  return HasFeature(feature) &&
         CServiceBroker::GetGameServices().GameSettings().RightStickMouse() &&
         m_gameClient.Input().AcceptsInput() && m_gameClient.Input().CanMoveMouse();
}

bool CGameClientStickMouse::OnButtonPress(const JOYSTICK::FeatureName& feature, bool bPressed)
{
  return OnButtonMotion(feature, bPressed ? 1.0f : 0.0f, 0);
}

bool CGameClientStickMouse::OnButtonMotion(const JOYSTICK::FeatureName& feature,
                                           float magnitude,
                                           unsigned int motionTimeMs)
{
  // Input that isn't accepted can still let go of a button, but not press one
  if (!AcceptsInput(feature))
    magnitude = 0.0f;

  if (feature == LEFT_BUTTON_TRIGGER)
    SetButton("left", magnitude, m_leftPressed);
  else if (feature == RIGHT_BUTTON_TRIGGER)
    SetButton("right", magnitude, m_rightPressed);
  else
    return false;

  return true;
}

bool CGameClientStickMouse::OnAnalogStickMotion(const JOYSTICK::FeatureName& feature,
                                                float x,
                                                float y,
                                                unsigned int motionTimeMs)
{
  if (feature != STICK)
    return false;

  if ((x == 0.0f && y == 0.0f) || !AcceptsInput(feature))
  {
    m_lastMotion = {};
    m_remainderX = 0.0f;
    m_remainderY = 0.0f;
    return true;
  }

  const auto now = std::chrono::steady_clock::now();
  std::chrono::duration<float> step = FIRST_STEP;
  if (m_lastMotion != std::chrono::steady_clock::time_point{})
    step = std::min<std::chrono::duration<float>>(now - m_lastMotion, MAX_STEP);
  m_lastMotion = now;

  // A stick can report past full deflection on its diagonals, so the
  // direction is kept to unit length there
  const float length = std::hypot(x, y);
  const float deflection = std::min(length, 1.0f);
  const float distance = MAX_SPEED * deflection * step.count();
  const float scale = length > 1.0f ? distance / length : distance;

  // The stick's y axis points up, the mouse's down
  const float moveX = x * scale + m_remainderX;
  const float moveY = -y * scale + m_remainderY;

  const int stepX = static_cast<int>(moveX);
  const int stepY = static_cast<int>(moveY);
  m_remainderX = moveX - static_cast<float>(stepX);
  m_remainderY = moveY - static_cast<float>(stepY);

  if (stepX != 0 || stepY != 0)
    m_gameClient.Input().MoveMouse(stepX, stepY);

  return true;
}

void CGameClientStickMouse::SetButton(const std::string& button, float magnitude, bool& pressed)
{
  std::lock_guard<std::mutex> lock(m_buttonMutex);

  const bool press = magnitude >= (pressed ? RELEASE_THRESHOLD : PRESS_THRESHOLD);
  if (press != pressed)
  {
    pressed = press;
    m_gameClient.Input().PressMouseButton(button, press);
  }
}
