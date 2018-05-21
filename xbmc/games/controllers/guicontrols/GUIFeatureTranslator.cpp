/*
 *      Copyright (C) 2017-present Team Kodi
 *      http://kodi.tv
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

#include "GUIFeatureTranslator.h"

using namespace KODI;
using namespace GAME;

BUTTON_TYPE CGUIFeatureTranslator::GetButtonType(JOYSTICK::FEATURE_TYPE featureType)
{
  switch (featureType)
  {
  case JOYSTICK::FEATURE_TYPE::SCALAR:
  case JOYSTICK::FEATURE_TYPE::KEY:
    return BUTTON_TYPE::BUTTON;

  case JOYSTICK::FEATURE_TYPE::ANALOG_STICK:
    return BUTTON_TYPE::ANALOG_STICK;

  case JOYSTICK::FEATURE_TYPE::RELPOINTER:
    return BUTTON_TYPE::RELATIVE_POINTER;

  case JOYSTICK::FEATURE_TYPE::WHEEL:
    return BUTTON_TYPE::WHEEL;

  case JOYSTICK::FEATURE_TYPE::THROTTLE:
    return BUTTON_TYPE::THROTTLE;

  default:
    break;
  }

  return BUTTON_TYPE::UNKNOWN;
}
