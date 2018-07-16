/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
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
