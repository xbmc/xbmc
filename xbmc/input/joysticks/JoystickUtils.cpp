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

#include "JoystickUtils.h"
#include "JoystickTranslator.h"
#include "utils/StringUtils.h"

using namespace KODI;
using namespace JOYSTICK;

std::string CJoystickUtils::MakeKeyName(const FeatureName &feature, ANALOG_STICK_DIRECTION dir /* = ANALOG_STICK_DIRECTION::UNKNOWN */)
{
  std::string keyName = feature;

  if (dir != ANALOG_STICK_DIRECTION::UNKNOWN)
    keyName += CJoystickTranslator::TranslateDirection(dir);

  return keyName;
}

const std::vector<ANALOG_STICK_DIRECTION> &CJoystickUtils::GetDirections()
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
