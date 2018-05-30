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

#include "JoystickTypes.h"
#include "input/mouse/MouseTypes.h"

namespace KODI
{
namespace JOYSTICK
{
  class CDriverPrimitive;

  /*!
   * \brief Joystick translation utilities
   */
  class CJoystickTranslator
  {
  public:
    /*!
     * \brief Translate a hat state to a string representation
     *
     * \param state The hat state
     *
     * \return A capitalized string representation, or "RELEASED" if the hat is centered.
     */
    static const char* HatStateToString(HAT_STATE state);

    /*!
     * \brief Translate an analog stick direction to a lower-case string
     *
     * \param dir The analog stick direction
     *
     * \return A lower-case string representation, or "" if the direction is invalid
     */
    static const char* TranslateAnalogStickDirection(ANALOG_STICK_DIRECTION dir);

    /*!
     * \brief Translate an analog stick direction string to an enum value
     *
     * \param dir The analog stick direction
     *
     * \return The translated direction, or ANALOG_STICK_DIRECTION::UNKNOWN if unknown
     */
    static ANALOG_STICK_DIRECTION TranslateAnalogStickDirection(const std::string &dir);

    /*!
     * \brief Translate a wheel direction to a lower-case string
     *
     * \param dir The wheel direction
     *
     * \return A lower-case string representation, or "" if the direction is invalid
     */
    static const char* TranslateWheelDirection(WHEEL_DIRECTION dir);

    /*!
     * \brief Translate a wheel direction string to an enum value
     *
     * \param dir The wheel direction
     *
     * \return The translated direction, or WHEEL_DIRECTION::UNKNOWN if unknown
     */
    static WHEEL_DIRECTION TranslateWheelDirection(const std::string &dir);

    /*!
     * \brief Translate a throttle direction to a lower-case string
     *
     * \param dir The analog stick direction
     *
     * \return A lower-case string representation, or "" if the direction is invalid
     */
    static const char* TranslateThrottleDirection(THROTTLE_DIRECTION dir);

    /*!
     * \brief Translate a throttle direction string to an enum value
     *
     * \param dir The throttle direction
     *
     * \return The translated direction, or THROTTLE_DIRECTION::UNKNOWN if unknown
     */
    static THROTTLE_DIRECTION TranslateThrottleDirection(const std::string &dir);

    /*!
     * \brief Get the semi-axis direction containing the specified position
     *
     * \param position The position of the axis
     *
     * \return POSITIVE, NEGATIVE, or UNKNOWN if position is 0
     */
    static SEMIAXIS_DIRECTION PositionToSemiAxisDirection(float position);

    /*!
     * \brief Get the wheel direction containing the specified position
     *
     * \param position The position of the axis
     *
     * \return LEFT, RIGHT, or UNKNOWN if position is 0
     */
    static WHEEL_DIRECTION PositionToWheelDirection(float position);

    /*!
     * \brief Get the throttle direction containing the specified position
     *
     * \param position The position of the axis
     *
     * \return UP, DOWN, or UNKNOWN if position is 0
     */
    static THROTTLE_DIRECTION PositionToThrottleDirection(float position);

    /*!
     * \brief Get the localized name of the primitive
     *
     * \param primitive The primitive, currently only buttons and axes are supported
     *
     * \return A title for the primitive, e.g. "Button 0" or "Axis 1"
     */
    static std::string GetPrimitiveName(const CDriverPrimitive& primitive);
  };
}
}
