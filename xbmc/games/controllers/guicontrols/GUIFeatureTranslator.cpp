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

#include "GUIFeatureTranslator.h"

using namespace KODI;
using namespace GAME;

BUTTON_TYPE CGUIFeatureTranslator::GetButtonType(JOYSTICK::FEATURE_TYPE featureType)
{
  switch (featureType)
  {
  case JOYSTICK::FEATURE_TYPE::SCALAR:
    return BUTTON_TYPE::BUTTON;

  case JOYSTICK::FEATURE_TYPE::ANALOG_STICK:
    return BUTTON_TYPE::ANALOG_STICK;

  //! @todo Create button control for relative pointers. Use analog sticks for
  //  now, as four directions are sufficient for relative pointers.
  case JOYSTICK::FEATURE_TYPE::RELPOINTER:
    return BUTTON_TYPE::ANALOG_STICK;

  default:
    break;
  }

  return BUTTON_TYPE::UNKNOWN;
}
