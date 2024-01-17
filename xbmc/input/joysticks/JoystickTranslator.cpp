/*
 *  Copyright (C) 2015-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "JoystickTranslator.h"

#include "guilib/LocalizeStrings.h"
#include "input/joysticks/DriverPrimitive.h"
#include "utils/StringUtils.h"

using namespace KODI;
using namespace JOYSTICK;

const char* CJoystickTranslator::HatStateToString(HAT_STATE state)
{
  switch (state)
  {
    case HAT_STATE::UP:
      return "UP";
    case HAT_STATE::DOWN:
      return "DOWN";
    case HAT_STATE::RIGHT:
      return "RIGHT";
    case HAT_STATE::LEFT:
      return "LEFT";
    case HAT_STATE::RIGHTUP:
      return "UP RIGHT";
    case HAT_STATE::RIGHTDOWN:
      return "DOWN RIGHT";
    case HAT_STATE::LEFTUP:
      return "UP LEFT";
    case HAT_STATE::LEFTDOWN:
      return "DOWN LEFT";
    default:
      break;
  }

  return "RELEASED";
}

const char* CJoystickTranslator::TranslateAnalogStickDirection(ANALOG_STICK_DIRECTION dir)
{
  switch (dir)
  {
    case ANALOG_STICK_DIRECTION::UP:
      return "up";
    case ANALOG_STICK_DIRECTION::DOWN:
      return "down";
    case ANALOG_STICK_DIRECTION::RIGHT:
      return "right";
    case ANALOG_STICK_DIRECTION::LEFT:
      return "left";
    default:
      break;
  }

  return "";
}

ANALOG_STICK_DIRECTION CJoystickTranslator::TranslateAnalogStickDirection(const std::string& dir)
{
  if (dir == "up")
    return ANALOG_STICK_DIRECTION::UP;
  if (dir == "down")
    return ANALOG_STICK_DIRECTION::DOWN;
  if (dir == "right")
    return ANALOG_STICK_DIRECTION::RIGHT;
  if (dir == "left")
    return ANALOG_STICK_DIRECTION::LEFT;

  return ANALOG_STICK_DIRECTION::NONE;
}

const char* CJoystickTranslator::TranslateWheelDirection(WHEEL_DIRECTION dir)
{
  switch (dir)
  {
    case WHEEL_DIRECTION::RIGHT:
      return "right";
    case WHEEL_DIRECTION::LEFT:
      return "left";
    default:
      break;
  }

  return "";
}

WHEEL_DIRECTION CJoystickTranslator::TranslateWheelDirection(const std::string& dir)
{
  if (dir == "right")
    return WHEEL_DIRECTION::RIGHT;
  if (dir == "left")
    return WHEEL_DIRECTION::LEFT;

  return WHEEL_DIRECTION::NONE;
}

const char* CJoystickTranslator::TranslateThrottleDirection(THROTTLE_DIRECTION dir)
{
  switch (dir)
  {
    case THROTTLE_DIRECTION::UP:
      return "up";
    case THROTTLE_DIRECTION::DOWN:
      return "down";
    default:
      break;
  }

  return "";
}

THROTTLE_DIRECTION CJoystickTranslator::TranslateThrottleDirection(const std::string& dir)
{
  if (dir == "up")
    return THROTTLE_DIRECTION::UP;
  if (dir == "down")
    return THROTTLE_DIRECTION::DOWN;

  return THROTTLE_DIRECTION::NONE;
}

SEMIAXIS_DIRECTION CJoystickTranslator::PositionToSemiAxisDirection(float position)
{
  if (position > 0)
    return SEMIAXIS_DIRECTION::POSITIVE;
  else if (position < 0)
    return SEMIAXIS_DIRECTION::NEGATIVE;

  return SEMIAXIS_DIRECTION::ZERO;
}

WHEEL_DIRECTION CJoystickTranslator::PositionToWheelDirection(float position)
{
  if (position > 0.0f)
    return WHEEL_DIRECTION::RIGHT;
  else if (position < 0.0f)
    return WHEEL_DIRECTION::LEFT;

  return WHEEL_DIRECTION::NONE;
}

THROTTLE_DIRECTION CJoystickTranslator::PositionToThrottleDirection(float position)
{
  if (position > 0.0f)
    return THROTTLE_DIRECTION::UP;
  else if (position < 0.0f)
    return THROTTLE_DIRECTION::DOWN;

  return THROTTLE_DIRECTION::NONE;
}

std::string CJoystickTranslator::GetPrimitiveName(const CDriverPrimitive& primitive)
{
  std::string primitiveTemplate;

  switch (primitive.Type())
  {
    case PRIMITIVE_TYPE::BUTTON:
      primitiveTemplate = g_localizeStrings.Get(35015); // "Button %d"
      break;
    case PRIMITIVE_TYPE::SEMIAXIS:
      primitiveTemplate = g_localizeStrings.Get(35016); // "Axis %d"
      break;
    default:
      break;
  }

  return StringUtils::Format(primitiveTemplate, primitive.Index());
}
