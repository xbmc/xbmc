/*
 *  Copyright (C) 2014-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "JoystickTypes.h"

#include <string>
#include <vector>

/// \ingroup joystick
/// \{

namespace KODI
{
namespace JOYSTICK
{

inline HAT_DIRECTION& operator|=(HAT_DIRECTION& lhs, HAT_DIRECTION rhs)
{
  return lhs = static_cast<HAT_DIRECTION>(static_cast<int>(lhs) | static_cast<int>(rhs));
}

inline HAT_STATE& operator|=(HAT_STATE& lhs, HAT_STATE rhs)
{
  return lhs = static_cast<HAT_STATE>(static_cast<int>(lhs) | static_cast<int>(rhs));
}

inline bool operator&(HAT_STATE lhs, HAT_DIRECTION rhs)
{
  return (static_cast<int>(lhs) & static_cast<int>(rhs)) ? true : false;
}

inline SEMIAXIS_DIRECTION operator*(SEMIAXIS_DIRECTION lhs, int rhs)
{
  return static_cast<SEMIAXIS_DIRECTION>(static_cast<int>(lhs) * rhs);
}

inline float operator*(float lhs, SEMIAXIS_DIRECTION rhs)
{
  return lhs * static_cast<int>(rhs);
}

class CJoystickUtils
{
public:
  /*!
   * \brief Create a key name used to index an action in the keymap
   *
   * \param feature  The feature name
   *
   * \return A valid name for a key in the joystick keymap
   */
  static std::string MakeKeyName(const FeatureName& feature);

  /*!
   * \brief Create a key name used to index an action in the keymap
   *
   * \param feature  The feature name
   * \param dir      The direction for analog sticks
   *
   * \return A valid name for a key in the joystick keymap
   */
  static std::string MakeKeyName(const FeatureName& feature, ANALOG_STICK_DIRECTION dir);

  /*!
   * \brief Create a key name used to index an action in the keymap
   *
   * \param feature  The feature name
   * \param dir      The direction for a wheel to turn
   *
   * \return A valid name for a key in the joystick keymap
   */
  static std::string MakeKeyName(const FeatureName& feature, WHEEL_DIRECTION dir);

  /*!
   * \brief Create a key name used to index an action in the keymap
   *
   * \param feature  The feature name
   * \param dir      The direction for a throttle to move
   *
   * \return A valid name for a key in the joystick keymap
   */
  static std::string MakeKeyName(const FeatureName& feature, THROTTLE_DIRECTION dir);

  /*!
   * \brief Return a vector of the four cardinal directions
   */
  static const std::vector<ANALOG_STICK_DIRECTION>& GetAnalogStickDirections();

  /*!
   * \brief Return a vector of the two wheel directions
   */
  static const std::vector<WHEEL_DIRECTION>& GetWheelDirections();

  /*!
   * \brief Return a vector of the two throttle directions
   */
  static const std::vector<THROTTLE_DIRECTION>& GetThrottleDirections();
};

} // namespace JOYSTICK
} // namespace KODI

/// \}
