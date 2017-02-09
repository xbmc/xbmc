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
#pragma once

#include "addons/kodi-addon-dev-kit/include/kodi/kodi_peripheral_types.h"
#include "addons/kodi-addon-dev-kit/include/kodi/kodi_peripheral_utils.hpp"
#include "input/joysticks/DriverPrimitive.h"
#include "input/joysticks/JoystickTypes.h"
#include "peripherals/PeripheralTypes.h"

#include <vector>

namespace PERIPHERALS
{
  class CPeripheralAddonTranslator
  {
  public:
    static const char* TranslateError(PERIPHERAL_ERROR error);

    static PeripheralType  TranslateType(PERIPHERAL_TYPE type);
    static PERIPHERAL_TYPE TranslateType(PeripheralType type);

    static KODI::JOYSTICK::CDriverPrimitive TranslatePrimitive(const ADDON::DriverPrimitive& primitive);
    static ADDON::DriverPrimitive     TranslatePrimitive(const KODI::JOYSTICK::CDriverPrimitive& primitive);

    static std::vector<KODI::JOYSTICK::CDriverPrimitive> TranslatePrimitives(const std::vector<ADDON::DriverPrimitive>& primitives);
    static std::vector<ADDON::DriverPrimitive>     TranslatePrimitives(const std::vector<KODI::JOYSTICK::CDriverPrimitive>& primitives);

    static KODI::JOYSTICK::HAT_DIRECTION       TranslateHatDirection(JOYSTICK_DRIVER_HAT_DIRECTION dir);
    static JOYSTICK_DRIVER_HAT_DIRECTION TranslateHatDirection(KODI::JOYSTICK::HAT_DIRECTION dir);

    static KODI::JOYSTICK::HAT_STATE TranslateHatState(JOYSTICK_STATE_HAT state);

    static KODI::JOYSTICK::SEMIAXIS_DIRECTION       TranslateSemiAxisDirection(JOYSTICK_DRIVER_SEMIAXIS_DIRECTION dir);
    static JOYSTICK_DRIVER_SEMIAXIS_DIRECTION TranslateSemiAxisDirection(KODI::JOYSTICK::SEMIAXIS_DIRECTION dir);

    static KODI::JOYSTICK::FEATURE_TYPE TranslateFeatureType(JOYSTICK_FEATURE_TYPE type);
    static JOYSTICK_FEATURE_TYPE  TranslateFeatureType(KODI::JOYSTICK::FEATURE_TYPE type);

    static ADDON::DriverPrimitive Opposite(const ADDON::DriverPrimitive& semiaxis);
  };
}
