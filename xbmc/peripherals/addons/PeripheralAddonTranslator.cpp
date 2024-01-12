/*
 *  Copyright (C) 2015-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PeripheralAddonTranslator.h"

#include "input/joysticks/JoystickUtils.h"
#include "input/keyboard/KeyboardTranslator.h"

#include <algorithm>
#include <iterator>

using namespace KODI;
using namespace JOYSTICK;
using namespace PERIPHERALS;

// --- Helper function ---------------------------------------------------------

JOYSTICK_DRIVER_SEMIAXIS_DIRECTION operator*(JOYSTICK_DRIVER_SEMIAXIS_DIRECTION dir, int i)
{
  return static_cast<JOYSTICK_DRIVER_SEMIAXIS_DIRECTION>(static_cast<int>(dir) * i);
}

// --- CPeripheralAddonTranslator ----------------------------------------------

const char* CPeripheralAddonTranslator::TranslateError(const PERIPHERAL_ERROR error)
{
  switch (error)
  {
    case PERIPHERAL_NO_ERROR:
      return "no error";
    case PERIPHERAL_ERROR_FAILED:
      return "command failed";
    case PERIPHERAL_ERROR_INVALID_PARAMETERS:
      return "invalid parameters";
    case PERIPHERAL_ERROR_NOT_IMPLEMENTED:
      return "not implemented";
    case PERIPHERAL_ERROR_NOT_CONNECTED:
      return "not connected";
    case PERIPHERAL_ERROR_CONNECTION_FAILED:
      return "connection failed";
    case PERIPHERAL_ERROR_UNKNOWN:
    default:
      return "unknown error";
  }
}

PeripheralType CPeripheralAddonTranslator::TranslateType(PERIPHERAL_TYPE type)
{
  switch (type)
  {
    case PERIPHERAL_TYPE_JOYSTICK:
      return PERIPHERAL_JOYSTICK;
    case PERIPHERAL_TYPE_KEYBOARD:
      return PERIPHERAL_KEYBOARD;
    case PERIPHERAL_TYPE_MOUSE:
      return PERIPHERAL_MOUSE;
    default:
      break;
  }
  return PERIPHERAL_UNKNOWN;
}

PERIPHERAL_TYPE CPeripheralAddonTranslator::TranslateType(PeripheralType type)
{
  switch (type)
  {
    case PERIPHERAL_JOYSTICK:
      return PERIPHERAL_TYPE_JOYSTICK;
    case PERIPHERAL_KEYBOARD:
      return PERIPHERAL_TYPE_KEYBOARD;
    case PERIPHERAL_MOUSE:
      return PERIPHERAL_TYPE_MOUSE;
    default:
      break;
  }
  return PERIPHERAL_TYPE_UNKNOWN;
}

CDriverPrimitive CPeripheralAddonTranslator::TranslatePrimitive(
    const kodi::addon::DriverPrimitive& primitive)
{
  CDriverPrimitive retVal;

  switch (primitive.Type())
  {
    case JOYSTICK_DRIVER_PRIMITIVE_TYPE_BUTTON:
    {
      retVal = CDriverPrimitive(PRIMITIVE_TYPE::BUTTON, primitive.DriverIndex());
      break;
    }
    case JOYSTICK_DRIVER_PRIMITIVE_TYPE_HAT_DIRECTION:
    {
      retVal = CDriverPrimitive(primitive.DriverIndex(),
                                TranslateHatDirection(primitive.HatDirection()));
      break;
    }
    case JOYSTICK_DRIVER_PRIMITIVE_TYPE_SEMIAXIS:
    {
      retVal = CDriverPrimitive(primitive.DriverIndex(), primitive.Center(),
                                TranslateSemiAxisDirection(primitive.SemiAxisDirection()),
                                primitive.Range());
      break;
    }
    case JOYSTICK_DRIVER_PRIMITIVE_TYPE_MOTOR:
    {
      retVal = CDriverPrimitive(PRIMITIVE_TYPE::MOTOR, primitive.DriverIndex());
      break;
    }
    case JOYSTICK_DRIVER_PRIMITIVE_TYPE_KEY:
    {
      KEYBOARD::XBMCKey keycode =
          KEYBOARD::CKeyboardTranslator::TranslateKeysym(primitive.Keycode());
      retVal = CDriverPrimitive(keycode);
      break;
    }
    case JOYSTICK_DRIVER_PRIMITIVE_TYPE_MOUSE_BUTTON:
    {
      retVal = CDriverPrimitive(TranslateMouseButton(primitive.MouseIndex()));
      break;
    }
    case JOYSTICK_DRIVER_PRIMITIVE_TYPE_RELPOINTER_DIRECTION:
    {
      retVal = CDriverPrimitive(TranslateRelPointerDirection(primitive.RelPointerDirection()));
      break;
    }
    default:
      break;
  }

  return retVal;
}

kodi::addon::DriverPrimitive CPeripheralAddonTranslator::TranslatePrimitive(
    const CDriverPrimitive& primitive)
{
  kodi::addon::DriverPrimitive retVal;

  switch (primitive.Type())
  {
    case PRIMITIVE_TYPE::BUTTON:
    {
      retVal = kodi::addon::DriverPrimitive::CreateButton(primitive.Index());
      break;
    }
    case PRIMITIVE_TYPE::HAT:
    {
      retVal = kodi::addon::DriverPrimitive(primitive.Index(),
                                            TranslateHatDirection(primitive.HatDirection()));
      break;
    }
    case PRIMITIVE_TYPE::SEMIAXIS:
    {
      retVal = kodi::addon::DriverPrimitive(
          primitive.Index(), primitive.Center(),
          TranslateSemiAxisDirection(primitive.SemiAxisDirection()), primitive.Range());
      break;
    }
    case PRIMITIVE_TYPE::MOTOR:
    {
      retVal = kodi::addon::DriverPrimitive::CreateMotor(primitive.Index());
      break;
    }
    case PRIMITIVE_TYPE::KEY:
    {
      std::string keysym = KEYBOARD::CKeyboardTranslator::TranslateKeycode(primitive.Keycode());
      retVal = kodi::addon::DriverPrimitive(keysym);
      break;
    }
    case PRIMITIVE_TYPE::MOUSE_BUTTON:
    {
      retVal = kodi::addon::DriverPrimitive::CreateMouseButton(
          TranslateMouseButton(primitive.MouseButton()));
      break;
    }
    case PRIMITIVE_TYPE::RELATIVE_POINTER:
    {
      retVal =
          kodi::addon::DriverPrimitive(TranslateRelPointerDirection(primitive.PointerDirection()));
      break;
    }
    default:
      break;
  }

  return retVal;
}

std::vector<JOYSTICK::CDriverPrimitive> CPeripheralAddonTranslator::TranslatePrimitives(
    const std::vector<kodi::addon::DriverPrimitive>& primitives)
{
  std::vector<JOYSTICK::CDriverPrimitive> ret;
  std::transform(primitives.begin(), primitives.end(), std::back_inserter(ret),
                 [](const kodi::addon::DriverPrimitive& primitive)
                 { return CPeripheralAddonTranslator::TranslatePrimitive(primitive); });
  return ret;
}

std::vector<kodi::addon::DriverPrimitive> CPeripheralAddonTranslator::TranslatePrimitives(
    const std::vector<JOYSTICK::CDriverPrimitive>& primitives)
{
  std::vector<kodi::addon::DriverPrimitive> ret;
  std::transform(primitives.begin(), primitives.end(), std::back_inserter(ret),
                 [](const JOYSTICK::CDriverPrimitive& primitive)
                 { return CPeripheralAddonTranslator::TranslatePrimitive(primitive); });
  return ret;
}

HAT_DIRECTION CPeripheralAddonTranslator::TranslateHatDirection(JOYSTICK_DRIVER_HAT_DIRECTION dir)
{
  switch (dir)
  {
    case JOYSTICK_DRIVER_HAT_LEFT:
      return HAT_DIRECTION::LEFT;
    case JOYSTICK_DRIVER_HAT_RIGHT:
      return HAT_DIRECTION::RIGHT;
    case JOYSTICK_DRIVER_HAT_UP:
      return HAT_DIRECTION::UP;
    case JOYSTICK_DRIVER_HAT_DOWN:
      return HAT_DIRECTION::DOWN;
    default:
      break;
  }
  return HAT_DIRECTION::NONE;
}

JOYSTICK_DRIVER_HAT_DIRECTION CPeripheralAddonTranslator::TranslateHatDirection(HAT_DIRECTION dir)
{
  switch (dir)
  {
    case HAT_DIRECTION::UP:
      return JOYSTICK_DRIVER_HAT_UP;
    case HAT_DIRECTION::DOWN:
      return JOYSTICK_DRIVER_HAT_DOWN;
    case HAT_DIRECTION::RIGHT:
      return JOYSTICK_DRIVER_HAT_RIGHT;
    case HAT_DIRECTION::LEFT:
      return JOYSTICK_DRIVER_HAT_LEFT;
    default:
      break;
  }
  return JOYSTICK_DRIVER_HAT_UNKNOWN;
}

HAT_STATE CPeripheralAddonTranslator::TranslateHatState(JOYSTICK_STATE_HAT state)
{
  HAT_STATE translatedState = HAT_STATE::NONE;

  if (state & JOYSTICK_STATE_HAT_UP)
    translatedState |= HAT_STATE::UP;
  if (state & JOYSTICK_STATE_HAT_DOWN)
    translatedState |= HAT_STATE::DOWN;
  if (state & JOYSTICK_STATE_HAT_RIGHT)
    translatedState |= HAT_STATE::RIGHT;
  if (state & JOYSTICK_STATE_HAT_LEFT)
    translatedState |= HAT_STATE::LEFT;

  return translatedState;
}

SEMIAXIS_DIRECTION CPeripheralAddonTranslator::TranslateSemiAxisDirection(
    JOYSTICK_DRIVER_SEMIAXIS_DIRECTION dir)
{
  switch (dir)
  {
    case JOYSTICK_DRIVER_SEMIAXIS_POSITIVE:
      return SEMIAXIS_DIRECTION::POSITIVE;
    case JOYSTICK_DRIVER_SEMIAXIS_NEGATIVE:
      return SEMIAXIS_DIRECTION::NEGATIVE;
    default:
      break;
  }
  return SEMIAXIS_DIRECTION::ZERO;
}

JOYSTICK_DRIVER_SEMIAXIS_DIRECTION CPeripheralAddonTranslator::TranslateSemiAxisDirection(
    SEMIAXIS_DIRECTION dir)
{
  switch (dir)
  {
    case SEMIAXIS_DIRECTION::POSITIVE:
      return JOYSTICK_DRIVER_SEMIAXIS_POSITIVE;
    case SEMIAXIS_DIRECTION::NEGATIVE:
      return JOYSTICK_DRIVER_SEMIAXIS_NEGATIVE;
    default:
      break;
  }
  return JOYSTICK_DRIVER_SEMIAXIS_UNKNOWN;
}

MOUSE::BUTTON_ID CPeripheralAddonTranslator::TranslateMouseButton(
    JOYSTICK_DRIVER_MOUSE_INDEX button)
{
  switch (button)
  {
    case JOYSTICK_DRIVER_MOUSE_INDEX_LEFT:
      return MOUSE::BUTTON_ID::LEFT;
    case JOYSTICK_DRIVER_MOUSE_INDEX_RIGHT:
      return MOUSE::BUTTON_ID::RIGHT;
    case JOYSTICK_DRIVER_MOUSE_INDEX_MIDDLE:
      return MOUSE::BUTTON_ID::MIDDLE;
    case JOYSTICK_DRIVER_MOUSE_INDEX_BUTTON4:
      return MOUSE::BUTTON_ID::BUTTON4;
    case JOYSTICK_DRIVER_MOUSE_INDEX_BUTTON5:
      return MOUSE::BUTTON_ID::BUTTON5;
    case JOYSTICK_DRIVER_MOUSE_INDEX_WHEEL_UP:
      return MOUSE::BUTTON_ID::WHEEL_UP;
    case JOYSTICK_DRIVER_MOUSE_INDEX_WHEEL_DOWN:
      return MOUSE::BUTTON_ID::WHEEL_DOWN;
    case JOYSTICK_DRIVER_MOUSE_INDEX_HORIZ_WHEEL_LEFT:
      return MOUSE::BUTTON_ID::HORIZ_WHEEL_LEFT;
    case JOYSTICK_DRIVER_MOUSE_INDEX_HORIZ_WHEEL_RIGHT:
      return MOUSE::BUTTON_ID::HORIZ_WHEEL_RIGHT;
    default:
      break;
  }

  return MOUSE::BUTTON_ID::UNKNOWN;
}

JOYSTICK_DRIVER_MOUSE_INDEX CPeripheralAddonTranslator::TranslateMouseButton(
    MOUSE::BUTTON_ID button)
{
  switch (button)
  {
    case MOUSE::BUTTON_ID::LEFT:
      return JOYSTICK_DRIVER_MOUSE_INDEX_LEFT;
    case MOUSE::BUTTON_ID::RIGHT:
      return JOYSTICK_DRIVER_MOUSE_INDEX_RIGHT;
    case MOUSE::BUTTON_ID::MIDDLE:
      return JOYSTICK_DRIVER_MOUSE_INDEX_MIDDLE;
    case MOUSE::BUTTON_ID::BUTTON4:
      return JOYSTICK_DRIVER_MOUSE_INDEX_BUTTON4;
    case MOUSE::BUTTON_ID::BUTTON5:
      return JOYSTICK_DRIVER_MOUSE_INDEX_BUTTON5;
    case MOUSE::BUTTON_ID::WHEEL_UP:
      return JOYSTICK_DRIVER_MOUSE_INDEX_WHEEL_UP;
    case MOUSE::BUTTON_ID::WHEEL_DOWN:
      return JOYSTICK_DRIVER_MOUSE_INDEX_WHEEL_DOWN;
    case MOUSE::BUTTON_ID::HORIZ_WHEEL_LEFT:
      return JOYSTICK_DRIVER_MOUSE_INDEX_HORIZ_WHEEL_LEFT;
    case MOUSE::BUTTON_ID::HORIZ_WHEEL_RIGHT:
      return JOYSTICK_DRIVER_MOUSE_INDEX_HORIZ_WHEEL_RIGHT;
    default:
      break;
  }

  return JOYSTICK_DRIVER_MOUSE_INDEX_UNKNOWN;
}

RELATIVE_POINTER_DIRECTION CPeripheralAddonTranslator::TranslateRelPointerDirection(
    JOYSTICK_DRIVER_RELPOINTER_DIRECTION dir)
{
  switch (dir)
  {
    case JOYSTICK_DRIVER_RELPOINTER_LEFT:
      return RELATIVE_POINTER_DIRECTION::LEFT;
    case JOYSTICK_DRIVER_RELPOINTER_RIGHT:
      return RELATIVE_POINTER_DIRECTION::RIGHT;
    case JOYSTICK_DRIVER_RELPOINTER_UP:
      return RELATIVE_POINTER_DIRECTION::UP;
    case JOYSTICK_DRIVER_RELPOINTER_DOWN:
      return RELATIVE_POINTER_DIRECTION::DOWN;
    default:
      break;
  }

  return RELATIVE_POINTER_DIRECTION::NONE;
}

JOYSTICK_DRIVER_RELPOINTER_DIRECTION CPeripheralAddonTranslator::TranslateRelPointerDirection(
    RELATIVE_POINTER_DIRECTION dir)
{
  switch (dir)
  {
    case RELATIVE_POINTER_DIRECTION::UP:
      return JOYSTICK_DRIVER_RELPOINTER_UP;
    case RELATIVE_POINTER_DIRECTION::DOWN:
      return JOYSTICK_DRIVER_RELPOINTER_DOWN;
    case RELATIVE_POINTER_DIRECTION::RIGHT:
      return JOYSTICK_DRIVER_RELPOINTER_RIGHT;
    case RELATIVE_POINTER_DIRECTION::LEFT:
      return JOYSTICK_DRIVER_RELPOINTER_LEFT;
    default:
      break;
  }
  return JOYSTICK_DRIVER_RELPOINTER_UNKNOWN;
}

JOYSTICK::FEATURE_TYPE CPeripheralAddonTranslator::TranslateFeatureType(JOYSTICK_FEATURE_TYPE type)
{
  switch (type)
  {
    case JOYSTICK_FEATURE_TYPE_SCALAR:
      return JOYSTICK::FEATURE_TYPE::SCALAR;
    case JOYSTICK_FEATURE_TYPE_ANALOG_STICK:
      return JOYSTICK::FEATURE_TYPE::ANALOG_STICK;
    case JOYSTICK_FEATURE_TYPE_ACCELEROMETER:
      return JOYSTICK::FEATURE_TYPE::ACCELEROMETER;
    case JOYSTICK_FEATURE_TYPE_MOTOR:
      return JOYSTICK::FEATURE_TYPE::MOTOR;
    case JOYSTICK_FEATURE_TYPE_RELPOINTER:
      return JOYSTICK::FEATURE_TYPE::RELPOINTER;
    case JOYSTICK_FEATURE_TYPE_ABSPOINTER:
      return JOYSTICK::FEATURE_TYPE::ABSPOINTER;
    case JOYSTICK_FEATURE_TYPE_WHEEL:
      return JOYSTICK::FEATURE_TYPE::WHEEL;
    case JOYSTICK_FEATURE_TYPE_THROTTLE:
      return JOYSTICK::FEATURE_TYPE::THROTTLE;
    case JOYSTICK_FEATURE_TYPE_KEY:
      return JOYSTICK::FEATURE_TYPE::KEY;
    default:
      break;
  }
  return JOYSTICK::FEATURE_TYPE::UNKNOWN;
}

JOYSTICK_FEATURE_TYPE CPeripheralAddonTranslator::TranslateFeatureType(JOYSTICK::FEATURE_TYPE type)
{
  switch (type)
  {
    case JOYSTICK::FEATURE_TYPE::SCALAR:
      return JOYSTICK_FEATURE_TYPE_SCALAR;
    case JOYSTICK::FEATURE_TYPE::ANALOG_STICK:
      return JOYSTICK_FEATURE_TYPE_ANALOG_STICK;
    case JOYSTICK::FEATURE_TYPE::ACCELEROMETER:
      return JOYSTICK_FEATURE_TYPE_ACCELEROMETER;
    case JOYSTICK::FEATURE_TYPE::MOTOR:
      return JOYSTICK_FEATURE_TYPE_MOTOR;
    case JOYSTICK::FEATURE_TYPE::RELPOINTER:
      return JOYSTICK_FEATURE_TYPE_RELPOINTER;
    case JOYSTICK::FEATURE_TYPE::ABSPOINTER:
      return JOYSTICK_FEATURE_TYPE_ABSPOINTER;
    case JOYSTICK::FEATURE_TYPE::WHEEL:
      return JOYSTICK_FEATURE_TYPE_WHEEL;
    case JOYSTICK::FEATURE_TYPE::THROTTLE:
      return JOYSTICK_FEATURE_TYPE_THROTTLE;
    case JOYSTICK::FEATURE_TYPE::KEY:
      return JOYSTICK_FEATURE_TYPE_KEY;
    default:
      break;
  }
  return JOYSTICK_FEATURE_TYPE_UNKNOWN;
}

kodi::addon::DriverPrimitive CPeripheralAddonTranslator::Opposite(
    const kodi::addon::DriverPrimitive& primitive)
{
  return kodi::addon::DriverPrimitive(primitive.DriverIndex(), primitive.Center() * -1,
                                      primitive.SemiAxisDirection() * -1, primitive.Range());
}
