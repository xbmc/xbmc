/*
 *      Copyright (C) 2015-2017 Team Kodi
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

#include "addons/kodi-addon-dev-kit/include/kodi/addon-instance/Peripheral.h"
#include "addons/kodi-addon-dev-kit/include/kodi/addon-instance/PeripheralUtils.h"
#include "input/joysticks/DriverPrimitive.h"
#include "input/joysticks/JoystickTypes.h"
#include "input/mouse/MouseTypes.h"
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

    static KODI::JOYSTICK::CDriverPrimitive TranslatePrimitive(const kodi::addon::DriverPrimitive& primitive);
    static kodi::addon::DriverPrimitive     TranslatePrimitive(const KODI::JOYSTICK::CDriverPrimitive& primitive);

    static std::vector<KODI::JOYSTICK::CDriverPrimitive> TranslatePrimitives(const std::vector<kodi::addon::DriverPrimitive>& primitives);
    static std::vector<kodi::addon::DriverPrimitive>     TranslatePrimitives(const std::vector<KODI::JOYSTICK::CDriverPrimitive>& primitives);

    static KODI::JOYSTICK::HAT_DIRECTION TranslateHatDirection(JOYSTICK_DRIVER_HAT_DIRECTION dir);
    static JOYSTICK_DRIVER_HAT_DIRECTION TranslateHatDirection(KODI::JOYSTICK::HAT_DIRECTION dir);

    static KODI::JOYSTICK::HAT_STATE TranslateHatState(JOYSTICK_STATE_HAT state);

    static KODI::JOYSTICK::SEMIAXIS_DIRECTION TranslateSemiAxisDirection(JOYSTICK_DRIVER_SEMIAXIS_DIRECTION dir);
    static JOYSTICK_DRIVER_SEMIAXIS_DIRECTION TranslateSemiAxisDirection(KODI::JOYSTICK::SEMIAXIS_DIRECTION dir);

    static KODI::MOUSE::BUTTON_ID TranslateMouseButton(JOYSTICK_DRIVER_MOUSE_INDEX button);
    static JOYSTICK_DRIVER_MOUSE_INDEX TranslateMouseButton(KODI::MOUSE::BUTTON_ID button);

    static KODI::JOYSTICK::RELATIVE_POINTER_DIRECTION TranslateRelPointerDirection(JOYSTICK_DRIVER_RELPOINTER_DIRECTION dir);
    static JOYSTICK_DRIVER_RELPOINTER_DIRECTION TranslateRelPointerDirection(KODI::JOYSTICK::RELATIVE_POINTER_DIRECTION dir);

    static KODI::JOYSTICK::FEATURE_TYPE TranslateFeatureType(JOYSTICK_FEATURE_TYPE type);
    static JOYSTICK_FEATURE_TYPE        TranslateFeatureType(KODI::JOYSTICK::FEATURE_TYPE type);

    static kodi::addon::DriverPrimitive Opposite(const kodi::addon::DriverPrimitive& semiaxis);
  };
}
