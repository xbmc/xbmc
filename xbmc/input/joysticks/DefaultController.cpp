/*
 *      Copyright (C) 2017 Team Kodi
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

#include "DefaultController.h"
#include "JoystickEasterEgg.h"
#include "JoystickIDs.h"
#include "input/Key.h"

using namespace KODI;
using namespace JOYSTICK;

CDefaultController::CDefaultController(void)
{
  // initialize CDefaultJoystick
  m_easterEgg.reset(new CJoystickEasterEgg(GetControllerID()));
}

std::string CDefaultController::GetControllerID(void) const
{
  return DEFAULT_CONTROLLER_ID;
}

unsigned int CDefaultController::GetKeyID(const FeatureName& feature, ANALOG_STICK_DIRECTION dir /* = ANALOG_STICK_DIRECTION::UNKNOWN */) const
{
  if      (feature == "a")             return KEY_JOYSTICK_BUTTON_A;
  else if (feature == "b")             return KEY_JOYSTICK_BUTTON_B;
  else if (feature == "x")             return KEY_JOYSTICK_BUTTON_X;
  else if (feature == "y")             return KEY_JOYSTICK_BUTTON_Y;
  else if (feature == "start")         return KEY_JOYSTICK_BUTTON_START;
  else if (feature == "back")          return KEY_JOYSTICK_BUTTON_BACK;
  else if (feature == "guide")         return KEY_JOYSTICK_BUTTON_GUIDE;
  else if (feature == "leftbumper")    return KEY_JOYSTICK_BUTTON_LEFT_SHOULDER;
  else if (feature == "rightbumper")   return KEY_JOYSTICK_BUTTON_RIGHT_SHOULDER;
  else if (feature == "leftthumb")     return KEY_JOYSTICK_BUTTON_LEFT_STICK_BUTTON;
  else if (feature == "rightthumb")    return KEY_JOYSTICK_BUTTON_RIGHT_STICK_BUTTON;
  else if (feature == "up")            return KEY_JOYSTICK_BUTTON_DPAD_UP;
  else if (feature == "down")          return KEY_JOYSTICK_BUTTON_DPAD_DOWN;
  else if (feature == "right")         return KEY_JOYSTICK_BUTTON_DPAD_RIGHT;
  else if (feature == "left")          return KEY_JOYSTICK_BUTTON_DPAD_LEFT;
  else if (feature == "lefttrigger")   return KEY_JOYSTICK_BUTTON_LEFT_TRIGGER;
  else if (feature == "righttrigger")  return KEY_JOYSTICK_BUTTON_RIGHT_TRIGGER;
  else if (feature == "leftstick")
  {
    switch (dir)
    {
      case ANALOG_STICK_DIRECTION::UP:     return KEY_JOYSTICK_BUTTON_LEFT_THUMB_STICK_UP;
      case ANALOG_STICK_DIRECTION::DOWN:   return KEY_JOYSTICK_BUTTON_LEFT_THUMB_STICK_DOWN;
      case ANALOG_STICK_DIRECTION::RIGHT:  return KEY_JOYSTICK_BUTTON_LEFT_THUMB_STICK_RIGHT;
      case ANALOG_STICK_DIRECTION::LEFT:   return KEY_JOYSTICK_BUTTON_LEFT_THUMB_STICK_LEFT;
      default:
        break;
    }
  }
  else if (feature == "rightstick")
  {
    switch (dir)
    {
      case ANALOG_STICK_DIRECTION::UP:     return KEY_JOYSTICK_BUTTON_RIGHT_THUMB_STICK_UP;
      case ANALOG_STICK_DIRECTION::DOWN:   return KEY_JOYSTICK_BUTTON_RIGHT_THUMB_STICK_DOWN;
      case ANALOG_STICK_DIRECTION::RIGHT:  return KEY_JOYSTICK_BUTTON_RIGHT_THUMB_STICK_RIGHT;
      case ANALOG_STICK_DIRECTION::LEFT:   return KEY_JOYSTICK_BUTTON_RIGHT_THUMB_STICK_LEFT;
      default:
        break;
    }
  }
  else if (feature == "accelerometer") return 0; //! @todo implement

  return 0;
}
