/*
 *      Copyright (C) 2014-2017 Team Kodi
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
  static std::string MakeKeyName(const FeatureName &feature);

  /*!
   * \brief Create a key name used to index an action in the keymap
   *
   * \param feature  The feature name
   * \param dir      The direction for analog sticks
   *
   * \return A valid name for a key in the joystick keymap
   */
  static std::string MakeKeyName(const FeatureName &feature, ANALOG_STICK_DIRECTION dir);

  /*!
   * \brief Create a key name used to index an action in the keymap
   *
   * \param feature  The feature name
   * \param dir      The direction for a wheel to turn
   *
   * \return A valid name for a key in the joystick keymap
   */
  static std::string MakeKeyName(const FeatureName &feature, WHEEL_DIRECTION dir);

  /*!
   * \brief Create a key name used to index an action in the keymap
   *
   * \param feature  The feature name
   * \param dir      The direction for a throttle to move
   *
   * \return A valid name for a key in the joystick keymap
   */
  static std::string MakeKeyName(const FeatureName &feature, THROTTLE_DIRECTION dir);

  /*!
    * \brief Return a vector of the four cardinal directions
    */
  static const std::vector<ANALOG_STICK_DIRECTION> &GetAnalogStickDirections();

  /*!
   * \brief Return a vector of the two wheel directions
   */
  static const std::vector<WHEEL_DIRECTION> &GetWheelDirections();

  /*!
   * \brief Return a vector of the two throttle directions
   */
  static const std::vector<THROTTLE_DIRECTION> &GetThrottleDirections();
};

}
}

/// \}
