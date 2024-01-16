/*
 *  Copyright (C) 2015-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "addons/kodi-dev-kit/include/kodi/addon-instance/Peripheral.h"
#include "input/joysticks/DriverPrimitive.h"
#include "input/joysticks/JoystickTypes.h"
#include "input/mouse/MouseTypes.h"
#include "peripherals/PeripheralTypes.h"

#include <vector>

namespace PERIPHERALS
{
/*!
 * \ingroup peripherals
 */
class CPeripheralAddonTranslator
{
public:
  static const char* TranslateError(PERIPHERAL_ERROR error);

  static PeripheralType TranslateType(PERIPHERAL_TYPE type);
  static PERIPHERAL_TYPE TranslateType(PeripheralType type);

  static KODI::JOYSTICK::CDriverPrimitive TranslatePrimitive(
      const kodi::addon::DriverPrimitive& primitive);
  static kodi::addon::DriverPrimitive TranslatePrimitive(
      const KODI::JOYSTICK::CDriverPrimitive& primitive);

  static std::vector<KODI::JOYSTICK::CDriverPrimitive> TranslatePrimitives(
      const std::vector<kodi::addon::DriverPrimitive>& primitives);
  static std::vector<kodi::addon::DriverPrimitive> TranslatePrimitives(
      const std::vector<KODI::JOYSTICK::CDriverPrimitive>& primitives);

  static KODI::JOYSTICK::HAT_DIRECTION TranslateHatDirection(JOYSTICK_DRIVER_HAT_DIRECTION dir);
  static JOYSTICK_DRIVER_HAT_DIRECTION TranslateHatDirection(KODI::JOYSTICK::HAT_DIRECTION dir);

  static KODI::JOYSTICK::HAT_STATE TranslateHatState(JOYSTICK_STATE_HAT state);

  static KODI::JOYSTICK::SEMIAXIS_DIRECTION TranslateSemiAxisDirection(
      JOYSTICK_DRIVER_SEMIAXIS_DIRECTION dir);
  static JOYSTICK_DRIVER_SEMIAXIS_DIRECTION TranslateSemiAxisDirection(
      KODI::JOYSTICK::SEMIAXIS_DIRECTION dir);

  static KODI::MOUSE::BUTTON_ID TranslateMouseButton(JOYSTICK_DRIVER_MOUSE_INDEX button);
  static JOYSTICK_DRIVER_MOUSE_INDEX TranslateMouseButton(KODI::MOUSE::BUTTON_ID button);

  static KODI::JOYSTICK::RELATIVE_POINTER_DIRECTION TranslateRelPointerDirection(
      JOYSTICK_DRIVER_RELPOINTER_DIRECTION dir);
  static JOYSTICK_DRIVER_RELPOINTER_DIRECTION TranslateRelPointerDirection(
      KODI::JOYSTICK::RELATIVE_POINTER_DIRECTION dir);

  static KODI::JOYSTICK::FEATURE_TYPE TranslateFeatureType(JOYSTICK_FEATURE_TYPE type);
  static JOYSTICK_FEATURE_TYPE TranslateFeatureType(KODI::JOYSTICK::FEATURE_TYPE type);

  static kodi::addon::DriverPrimitive Opposite(const kodi::addon::DriverPrimitive& semiaxis);
};
} // namespace PERIPHERALS
