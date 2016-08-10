/*
 *      Copyright (C) 2015-2016 Team Kodi
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
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "PeripheralAddonTranslator.h"
#include "input/joysticks/JoystickUtils.h"

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

CDriverPrimitive CPeripheralAddonTranslator::TranslatePrimitive(const ADDON::DriverPrimitive& primitive)
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
      retVal = CDriverPrimitive(primitive.DriverIndex(), TranslateHatDirection(primitive.HatDirection()));
      break;
    }
    case JOYSTICK_DRIVER_PRIMITIVE_TYPE_SEMIAXIS:
    {
      retVal = CDriverPrimitive(primitive.DriverIndex(), TranslateSemiAxisDirection(primitive.SemiAxisDirection()));
      break;
    }
    case JOYSTICK_DRIVER_PRIMITIVE_TYPE_MOTOR:
    {
      retVal = CDriverPrimitive(PRIMITIVE_TYPE::MOTOR, primitive.DriverIndex());
      break;
    }
    default:
      break;
  }

  return retVal;
}

ADDON::DriverPrimitive CPeripheralAddonTranslator::TranslatePrimitive(const CDriverPrimitive& primitive)
{
  ADDON::DriverPrimitive retVal;

  switch (primitive.Type())
  {
    case BUTTON:
    {
      retVal = ADDON::DriverPrimitive::CreateButton(primitive.Index());
      break;
    }
    case HAT:
    {
      retVal = ADDON::DriverPrimitive(primitive.Index(), TranslateHatDirection(primitive.HatDirection()));
      break;
    }
    case SEMIAXIS:
    {
      retVal = ADDON::DriverPrimitive(primitive.Index(), TranslateSemiAxisDirection(primitive.SemiAxisDirection()));
      break;
    }
    case MOTOR:
    {
      retVal = ADDON::DriverPrimitive::CreateMotor(primitive.Index());
      break;
    }
    default:
      break;
  }

  return retVal;
}

HAT_DIRECTION CPeripheralAddonTranslator::TranslateHatDirection(JOYSTICK_DRIVER_HAT_DIRECTION dir)
{
  switch (dir)
  {
    case JOYSTICK_DRIVER_HAT_LEFT:   return HAT_DIRECTION::LEFT;
    case JOYSTICK_DRIVER_HAT_RIGHT:  return HAT_DIRECTION::RIGHT;
    case JOYSTICK_DRIVER_HAT_UP:     return HAT_DIRECTION::UP;
    case JOYSTICK_DRIVER_HAT_DOWN:   return HAT_DIRECTION::DOWN;
    default:
      break;
  }
  return HAT_DIRECTION::UNKNOWN;
}

JOYSTICK_DRIVER_HAT_DIRECTION CPeripheralAddonTranslator::TranslateHatDirection(HAT_DIRECTION dir)
{
  switch (dir)
  {
    case HAT_DIRECTION::UP:     return JOYSTICK_DRIVER_HAT_UP;
    case HAT_DIRECTION::DOWN:   return JOYSTICK_DRIVER_HAT_DOWN;
    case HAT_DIRECTION::RIGHT:  return JOYSTICK_DRIVER_HAT_RIGHT;
    case HAT_DIRECTION::LEFT:   return JOYSTICK_DRIVER_HAT_LEFT;
    default:
      break;
  }
  return JOYSTICK_DRIVER_HAT_UNKNOWN;
}

HAT_STATE CPeripheralAddonTranslator::TranslateHatState(JOYSTICK_STATE_HAT state)
{
  HAT_STATE translatedState = HAT_STATE::UNPRESSED;

  if (state & JOYSTICK_STATE_HAT_UP)    translatedState |= HAT_STATE::UP;
  if (state & JOYSTICK_STATE_HAT_DOWN)  translatedState |= HAT_STATE::DOWN;
  if (state & JOYSTICK_STATE_HAT_RIGHT) translatedState |= HAT_STATE::RIGHT;
  if (state & JOYSTICK_STATE_HAT_LEFT)  translatedState |= HAT_STATE::LEFT;

  return translatedState;
}

SEMIAXIS_DIRECTION CPeripheralAddonTranslator::TranslateSemiAxisDirection(JOYSTICK_DRIVER_SEMIAXIS_DIRECTION dir)
{
  switch (dir)
  {
    case JOYSTICK_DRIVER_SEMIAXIS_POSITIVE: return SEMIAXIS_DIRECTION::POSITIVE;
    case JOYSTICK_DRIVER_SEMIAXIS_NEGATIVE: return SEMIAXIS_DIRECTION::NEGATIVE;
    default:
      break;
  }
  return SEMIAXIS_DIRECTION::ZERO;
}

JOYSTICK_DRIVER_SEMIAXIS_DIRECTION CPeripheralAddonTranslator::TranslateSemiAxisDirection(SEMIAXIS_DIRECTION dir)
{
  switch (dir)
  {
    case SEMIAXIS_DIRECTION::POSITIVE: return JOYSTICK_DRIVER_SEMIAXIS_POSITIVE;
    case SEMIAXIS_DIRECTION::NEGATIVE: return JOYSTICK_DRIVER_SEMIAXIS_NEGATIVE;
    default:
      break;
  }
  return JOYSTICK_DRIVER_SEMIAXIS_UNKNOWN;
}

JOYSTICK::FEATURE_TYPE CPeripheralAddonTranslator::TranslateFeatureType(JOYSTICK_FEATURE_TYPE type)
{
  switch (type)
  {
    case JOYSTICK_FEATURE_TYPE_SCALAR:        return JOYSTICK::FEATURE_TYPE::SCALAR;
    case JOYSTICK_FEATURE_TYPE_ANALOG_STICK:  return JOYSTICK::FEATURE_TYPE::ANALOG_STICK;
    case JOYSTICK_FEATURE_TYPE_ACCELEROMETER: return JOYSTICK::FEATURE_TYPE::ACCELEROMETER;
    case JOYSTICK_FEATURE_TYPE_MOTOR:         return JOYSTICK::FEATURE_TYPE::MOTOR;
    default:
      break;
  }
  return JOYSTICK::FEATURE_TYPE::UNKNOWN;
}

JOYSTICK_FEATURE_TYPE CPeripheralAddonTranslator::TranslateFeatureType(JOYSTICK::FEATURE_TYPE type)
{
  switch (type)
  {
    case JOYSTICK::FEATURE_TYPE::SCALAR:        return JOYSTICK_FEATURE_TYPE_SCALAR;
    case JOYSTICK::FEATURE_TYPE::ANALOG_STICK:  return JOYSTICK_FEATURE_TYPE_ANALOG_STICK;
    case JOYSTICK::FEATURE_TYPE::ACCELEROMETER: return JOYSTICK_FEATURE_TYPE_ACCELEROMETER;
    case JOYSTICK::FEATURE_TYPE::MOTOR:         return JOYSTICK_FEATURE_TYPE_MOTOR;
    default:
      break;
  }
  return JOYSTICK_FEATURE_TYPE_UNKNOWN;
}

ADDON::DriverPrimitive CPeripheralAddonTranslator::Opposite(const ADDON::DriverPrimitive& primitive)
{
  return ADDON::DriverPrimitive(primitive.DriverIndex(), primitive.SemiAxisDirection() * -1);
}
