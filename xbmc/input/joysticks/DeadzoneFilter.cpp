/*
 *  Copyright (C) 2016-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DeadzoneFilter.h"

#include "JoystickIDs.h"
#include "games/controllers/ControllerIDs.h"
#include "input/joysticks/interfaces/IButtonMap.h"
#include "peripherals/devices/Peripheral.h"
#include "utils/log.h"

#include <cmath>
#include <vector>

using namespace KODI;
using namespace JOYSTICK;

//! Allowed noise for detecting discrete D-pads (value of 0.007 when centered
//! has been observed)
#define AXIS_EPSILON 0.01f

// Settings for analog sticks
#define SETTING_LEFT_STICK_DEADZONE "left_stick_deadzone"
#define SETTING_RIGHT_STICK_DEADZONE "right_stick_deadzone"

CDeadzoneFilter::CDeadzoneFilter(IButtonMap* buttonMap, PERIPHERALS::CPeripheral* peripheral)
  : m_buttonMap(buttonMap), m_peripheral(peripheral)
{
  if (m_buttonMap->ControllerID() != DEFAULT_CONTROLLER_ID)
    CLog::Log(LOGERROR, "ERROR: Must use default controller profile instead of {}",
              m_buttonMap->ControllerID());
}

float CDeadzoneFilter::FilterAxis(unsigned int axisIndex, float axisValue)
{
  float deadzone = 0.0f;

  bool bSuccess =
      GetDeadzone(axisIndex, deadzone, DEFAULT_LEFT_STICK_NAME, SETTING_LEFT_STICK_DEADZONE) ||
      GetDeadzone(axisIndex, deadzone, DEFAULT_RIGHT_STICK_NAME, SETTING_RIGHT_STICK_DEADZONE);

  if (bSuccess)
    return ApplyDeadzone(axisValue, deadzone);

  // Always filter noise about the center
  if (std::abs(axisValue) < AXIS_EPSILON)
    axisValue = 0.0f;

  return axisValue;
}

bool CDeadzoneFilter::GetDeadzone(unsigned int axisIndex,
                                  float& deadzone,
                                  const char* featureName,
                                  const char* settingName)
{
  std::vector<ANALOG_STICK_DIRECTION> dirs = {
      ANALOG_STICK_DIRECTION::UP,
      ANALOG_STICK_DIRECTION::RIGHT,
      ANALOG_STICK_DIRECTION::DOWN,
      ANALOG_STICK_DIRECTION::LEFT,
  };

  CDriverPrimitive primitive;
  for (auto dir : dirs)
  {
    if (m_buttonMap->GetAnalogStick(featureName, dir, primitive))
    {
      if (primitive.Type() == PRIMITIVE_TYPE::SEMIAXIS && primitive.Index() == axisIndex)
      {
        deadzone = m_peripheral->GetSettingFloat(settingName);
        return true;
      }
    }
  }

  return false;
}

float CDeadzoneFilter::ApplyDeadzone(float value, float deadzone)
{
  if (deadzone < 0.0f || deadzone >= 1.0f)
    return 0.0f;

  if (value > deadzone)
    return (value - deadzone) / (1.0f - deadzone);
  else if (value < -deadzone)
    return (value + deadzone) / (1.0f - deadzone);

  return 0.0f;
}
