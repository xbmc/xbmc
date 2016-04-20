/*
 *      Copyright (C) 2014-2016 Team Kodi
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

#include "DefaultJoystick.h"
#include "KeymapHandler.h"
#include "JoystickTranslator.h"
#include "input/Key.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>

#define ANALOG_DIGITAL_THRESHOLD   0.5f

using namespace JOYSTICK;

CDefaultJoystick::CDefaultJoystick(void) :
  m_handler(new CKeymapHandler),
  m_rumbleGenerator(ControllerID())
{
}

CDefaultJoystick::~CDefaultJoystick(void)
{
  delete m_handler;
}

std::string CDefaultJoystick::ControllerID(void) const
{
  return DEFAULT_CONTROLLER_ID;
}

bool CDefaultJoystick::HasFeature(const FeatureName& feature) const
{
  if (GetKeyID(feature) != 0)
    return true;

  // Try analog stick direction
  if (GetKeyID(feature, CARDINAL_DIRECTION::UP) != 0)
    return true;

  return false;
}

INPUT_TYPE CDefaultJoystick::GetInputType(const FeatureName& feature) const
{
  return m_handler->GetInputType(GetKeyID(feature));
}

bool CDefaultJoystick::OnButtonPress(const FeatureName& feature, bool bPressed)
{
  const unsigned int keyId = GetKeyID(feature);

  if (m_handler->GetInputType(keyId) == INPUT_TYPE::DIGITAL)
  {
    m_handler->OnDigitalKey(keyId, bPressed);
    return true;
  }

  return false;
}

bool CDefaultJoystick::OnButtonMotion(const FeatureName& feature, float magnitude)
{
  const unsigned int keyId = GetKeyID(feature);

  if (m_handler->GetInputType(keyId) == INPUT_TYPE::ANALOG)
  {
    m_handler->OnAnalogKey(keyId, magnitude);
    return true;
  }

  return false;
}

bool CDefaultJoystick::OnAnalogStickMotion(const FeatureName& feature, float x, float y)
{
  // Calculate the direction of the stick's position
  const CARDINAL_DIRECTION analogStickDir = CJoystickTranslator::VectorToCardinalDirection(x, y);

  // Calculate the magnitude projected onto that direction
  const float magnitude = std::max(std::abs(x), std::abs(y));

  // Deactivate directions in which the stick is not pointing first
  for (std::vector<CARDINAL_DIRECTION>::const_iterator it = GetDirections().begin(); it != GetDirections().end(); ++it)
  {
    if (*it != analogStickDir)
      DeactivateDirection(feature, *it);
  }

  // Now activate direction the analog stick is pointing
  return ActivateDirection(feature, magnitude, analogStickDir);
}

bool CDefaultJoystick::OnAccelerometerMotion(const FeatureName& feature, float x, float y, float z)
{
  return false; //! @todo implement
}

bool CDefaultJoystick::ActivateDirection(const FeatureName& feature, float magnitude, CARDINAL_DIRECTION dir)
{
  // Calculate the button key ID and input type for the analog stick's direction
  const unsigned int  keyId     = GetKeyID(feature, dir);
  const INPUT_TYPE    inputType = m_handler->GetInputType(keyId);

  if (inputType == INPUT_TYPE::DIGITAL)
  {
    const bool bIsPressed = (magnitude >= ANALOG_DIGITAL_THRESHOLD);
    m_handler->OnDigitalKey(keyId, bIsPressed);
    return true;
  }
  else if (inputType == INPUT_TYPE::ANALOG)
  {
    m_handler->OnAnalogKey(keyId, magnitude);
    return true;
  }

  return false;
}

void CDefaultJoystick::DeactivateDirection(const FeatureName& feature, CARDINAL_DIRECTION dir)
{
  // Calculate the button key ID and input type for this direction
  const unsigned int  keyId     = GetKeyID(feature, dir);
  const INPUT_TYPE    inputType = m_handler->GetInputType(keyId);

  if (inputType == INPUT_TYPE::DIGITAL)
  {
    m_handler->OnDigitalKey(keyId, false);
  }
  else if (inputType == INPUT_TYPE::ANALOG)
  {
    m_handler->OnAnalogKey(keyId, 0.0f);
  }
}

unsigned int CDefaultJoystick::GetKeyID(const FeatureName& feature, CARDINAL_DIRECTION dir /* = CARDINAL_DIRECTION::UNKNOWN */)
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
      case CARDINAL_DIRECTION::UP:     return KEY_JOYSTICK_BUTTON_LEFT_THUMB_STICK_UP;
      case CARDINAL_DIRECTION::DOWN:   return KEY_JOYSTICK_BUTTON_LEFT_THUMB_STICK_DOWN;
      case CARDINAL_DIRECTION::RIGHT:  return KEY_JOYSTICK_BUTTON_LEFT_THUMB_STICK_RIGHT;
      case CARDINAL_DIRECTION::LEFT:   return KEY_JOYSTICK_BUTTON_LEFT_THUMB_STICK_LEFT;
      default:
        break;
    }
  }
  else if (feature == "rightstick")
  {
    switch (dir)
    {
      case CARDINAL_DIRECTION::UP:     return KEY_JOYSTICK_BUTTON_RIGHT_THUMB_STICK_UP;
      case CARDINAL_DIRECTION::DOWN:   return KEY_JOYSTICK_BUTTON_RIGHT_THUMB_STICK_DOWN;
      case CARDINAL_DIRECTION::RIGHT:  return KEY_JOYSTICK_BUTTON_RIGHT_THUMB_STICK_RIGHT;
      case CARDINAL_DIRECTION::LEFT:   return KEY_JOYSTICK_BUTTON_RIGHT_THUMB_STICK_LEFT;
      default:
        break;
    }
  }
  else if (feature == "accelerometer") return 0; //! @todo implement

  return 0;
}

const std::vector<CARDINAL_DIRECTION>& CDefaultJoystick::GetDirections(void)
{
  static std::vector<CARDINAL_DIRECTION> directions;
  if (directions.empty())
  {
    directions.push_back(CARDINAL_DIRECTION::UP);
    directions.push_back(CARDINAL_DIRECTION::DOWN);
    directions.push_back(CARDINAL_DIRECTION::RIGHT);
    directions.push_back(CARDINAL_DIRECTION::LEFT);
  }
  return directions;
}
