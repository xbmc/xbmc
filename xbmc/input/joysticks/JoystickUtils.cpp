/*
 *      Copyright (C) 2017-present Team Kodi
 *      This file is part of Kodi - https://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "JoystickUtils.h"
#include "JoystickTranslator.h"
#include "utils/StringUtils.h"

using namespace KODI;
using namespace JOYSTICK;

std::string CJoystickUtils::MakeKeyName(const FeatureName &feature)
{
  return feature;
}

std::string CJoystickUtils::MakeKeyName(const FeatureName &feature, ANALOG_STICK_DIRECTION dir)
{
  std::string keyName = feature;

  if (dir != ANALOG_STICK_DIRECTION::NONE)
    keyName += CJoystickTranslator::TranslateAnalogStickDirection(dir);

  return keyName;
}

std::string CJoystickUtils::MakeKeyName(const FeatureName &feature, WHEEL_DIRECTION dir)
{
  ANALOG_STICK_DIRECTION stickDir = ANALOG_STICK_DIRECTION::NONE;

  switch (dir)
  {
  case WHEEL_DIRECTION::LEFT:
    stickDir = ANALOG_STICK_DIRECTION::LEFT;
    break;
  case WHEEL_DIRECTION::RIGHT:
    stickDir = ANALOG_STICK_DIRECTION::RIGHT;
    break;
  default:
    break;
  }

  return MakeKeyName(feature, stickDir);
}

std::string CJoystickUtils::MakeKeyName(const FeatureName &feature, THROTTLE_DIRECTION dir)
{
  ANALOG_STICK_DIRECTION stickDir = ANALOG_STICK_DIRECTION::NONE;

  switch (dir)
  {
    case THROTTLE_DIRECTION::UP:
      stickDir = ANALOG_STICK_DIRECTION::UP;
      break;
    case THROTTLE_DIRECTION::DOWN:
      stickDir = ANALOG_STICK_DIRECTION::DOWN;
      break;
    default:
      break;
  }

  return MakeKeyName(feature, stickDir);
}

const std::vector<ANALOG_STICK_DIRECTION> &CJoystickUtils::GetAnalogStickDirections()
{
  static std::vector<ANALOG_STICK_DIRECTION> directions;
  if (directions.empty())
  {
    directions.push_back(ANALOG_STICK_DIRECTION::UP);
    directions.push_back(ANALOG_STICK_DIRECTION::DOWN);
    directions.push_back(ANALOG_STICK_DIRECTION::RIGHT);
    directions.push_back(ANALOG_STICK_DIRECTION::LEFT);
  }
  return directions;
}

const std::vector<WHEEL_DIRECTION> &CJoystickUtils::GetWheelDirections()
{
  static std::vector<WHEEL_DIRECTION> directions;
  if (directions.empty())
  {
    directions.push_back(WHEEL_DIRECTION::RIGHT);
    directions.push_back(WHEEL_DIRECTION::LEFT);
  }
  return directions;
}

const std::vector<THROTTLE_DIRECTION> &CJoystickUtils::GetThrottleDirections()
{
  static std::vector<THROTTLE_DIRECTION> directions;
  if (directions.empty())
  {
    directions.push_back(THROTTLE_DIRECTION::UP);
    directions.push_back(THROTTLE_DIRECTION::DOWN);
  }
  return directions;
}
