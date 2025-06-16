/*
 *  Copyright (C) 2022-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ControllerState.h"

#include "games/controllers/Controller.h"
#include "games/controllers/input/PhysicalFeature.h"
#include "input/joysticks/JoystickTypes.h"

using namespace KODI;
using namespace GAME;

CControllerState::CControllerState(const CController& controller) : m_controllerId(controller.ID())
{
  for (const auto& feature : controller.Features())
  {
    const std::string& featureName = feature.Name();

    switch (feature.Type())
    {
      case JOYSTICK::FEATURE_TYPE::SCALAR:
        switch (feature.InputType())
        {
          case JOYSTICK::INPUT_TYPE::DIGITAL:
            SetDigitalButton(featureName, false);
            break;
          case JOYSTICK::INPUT_TYPE::ANALOG:
            SetAnalogButton(featureName, 0.0f);
            break;
          default:
            break;
        }
        break;
      case JOYSTICK::FEATURE_TYPE::ANALOG_STICK:
        SetAnalogStick(featureName, {0.0f, 0.0f});
        break;
      case JOYSTICK::FEATURE_TYPE::ACCELEROMETER:
        SetAccelerometer(featureName, {0.0f, 0.0f, 0.0f});
        break;
      case JOYSTICK::FEATURE_TYPE::WHEEL:
        SetWheel(featureName, 0.0f);
        break;
      case JOYSTICK::FEATURE_TYPE::THROTTLE:
        SetThrottle(featureName, 0.0f);
        break;
      default:
        break;
    }
  }
}

CControllerState::DigitalButton CControllerState::GetDigitalButton(
    const std::string& featureName) const
{
  auto it = m_digitalButtons.find(featureName);
  if (it != m_digitalButtons.end())
    return it->second;

  return false;
}

CControllerState::AnalogButton CControllerState::GetAnalogButton(
    const std::string& featureName) const
{
  auto it = m_analogButtons.find(featureName);
  if (it != m_analogButtons.end())
    return it->second;

  return 0.0f;
}

CControllerState::AnalogStick CControllerState::GetAnalogStick(const std::string& featureName) const
{
  auto it = m_analogSticks.find(featureName);
  if (it != m_analogSticks.end())
    return it->second;

  return {};
}

CControllerState::Accelerometer CControllerState::GetAccelerometer(
    const std::string& featureName) const
{
  auto it = m_accelerometers.find(featureName);
  if (it != m_accelerometers.end())
    return it->second;

  return {};
}

CControllerState::Throttle CControllerState::GetThrottle(const std::string& featureName) const
{
  auto it = m_throttles.find(featureName);
  if (it != m_throttles.end())
    return it->second;

  return 0.0f;
}

CControllerState::Wheel CControllerState::GetWheel(const std::string& featureName) const
{
  auto it = m_wheels.find(featureName);
  if (it != m_wheels.end())
    return it->second;

  return 0.0f;
}

void CControllerState::SetDigitalButton(const std::string& featureName, DigitalButton state)
{
  m_digitalButtons[featureName] = state;
}

void CControllerState::SetAnalogButton(const std::string& featureName, AnalogButton state)
{
  m_analogButtons[featureName] = state;
}

void CControllerState::SetAnalogStick(const std::string& featureName, AnalogStick state)
{
  m_analogSticks[featureName] = state;
}

void CControllerState::SetAccelerometer(const std::string& featureName, Accelerometer state)
{
  m_accelerometers[featureName] = state;
}

void CControllerState::SetThrottle(const std::string& featureName, Throttle state)
{
  m_throttles[featureName] = state;
}

void CControllerState::SetWheel(const std::string& featureName, Wheel state)
{
  m_wheels[featureName] = state;
}
