/*
 *  Copyright (C) 2014-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "input/InputTypes.h"

#include <set>
#include <string>

/// \ingroup joystick
/// \{

namespace KODI
{
namespace JOYSTICK
{
/*!
 * \brief Name of a physical feature belonging to the joystick
 */
using FeatureName = std::string;

/*!
 * \brief Types of features used in the joystick library
 *
 * Available types:
 *
 *   1) scalar[*]
 *   2) analog stick
 *   3) accelerometer
 *   4) rumble motor
 *   5) relative pointer
 *   6) absolute pointer
 *   7) wheel
 *   8) throttle
 *   9) keyboard key
 *
 * [*] All three driver primitives (buttons, hats and axes) have a state that
 *     can be represented using a single scalar value. For this reason,
 *     features that map to a single primitive are called "scalar features".
 */
enum class FEATURE_TYPE
{
  UNKNOWN,
  SCALAR,
  ANALOG_STICK,
  ACCELEROMETER,
  MOTOR,
  RELPOINTER,
  ABSPOINTER,
  WHEEL,
  THROTTLE,
  KEY,
};

/*!
 * \brief Categories of features used in the joystick library
 */
enum class FEATURE_CATEGORY
{
  UNKNOWN,
  FACE,
  SHOULDER,
  TRIGGER,
  ANALOG_STICK,
  ACCELEROMETER,
  HAPTICS,
  MOUSE_BUTTON,
  POINTER,
  LIGHTGUN,
  OFFSCREEN, // Virtual button to shoot light gun offscreen
  KEY, // A keyboard key
  KEYPAD, // A key on a numeric keymap, including star and pound
  HARDWARE, // A button or functionality on the console
  WHEEL,
  JOYSTICK,
  PADDLE,
};

/*!
 * \brief Direction arrows on the hat (directional pad)
 */
using HAT_DIRECTION = INPUT::CARDINAL_DIRECTION;

/*!
 * \brief States in which a hat can be
 */
using HAT_STATE = INPUT::INTERCARDINAL_DIRECTION;

/*!
 * \brief Typedef for analog stick directions
 */
using ANALOG_STICK_DIRECTION = INPUT::CARDINAL_DIRECTION;

/*!
 * \brief Directions of motion for a relative pointer
 */
using RELATIVE_POINTER_DIRECTION = INPUT::CARDINAL_DIRECTION;

/*!
 * \brief Directions in which a semiaxis can point
 */
enum class SEMIAXIS_DIRECTION
{
  NEGATIVE = -1, // semiaxis lies in the interval [-1.0, 0.0]
  ZERO = 0, // semiaxis is unknown or invalid
  POSITIVE = 1, // semiaxis lies in the interval [0.0, 1.0]
};

/*!
 * \brief Directions on a wheel
 */
enum class WHEEL_DIRECTION
{
  NONE,
  RIGHT,
  LEFT,
};

/*!
 * \brief Directions on a throttle
 */
enum class THROTTLE_DIRECTION
{
  NONE,
  UP,
  DOWN,
};

/*!
 * \brief Types of input available for scalar features
 */
enum class INPUT_TYPE
{
  UNKNOWN,
  DIGITAL,
  ANALOG,
};

/*!
 * \brief Type of driver primitive
 */
enum class PRIMITIVE_TYPE
{
  UNKNOWN = 0, // primitive has no type (invalid)
  BUTTON, // a digital button
  HAT, // one of the four direction arrows on a D-pad
  SEMIAXIS, // the positive or negative half of an axis
  MOTOR, // a rumble motor
  KEY, // a keyboard key
  MOUSE_BUTTON, // a mouse button
  RELATIVE_POINTER, // a relative pointer, such as on a mouse
};
} // namespace JOYSTICK
} // namespace KODI

/// \}
