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

#include "input/joysticks/JoystickTypes.h"

namespace KODI
{
namespace JOYSTICK
{
  /*!
   * \ingroup joystick
   * \brief Interface for handling keymap keys
   *
   * Keys can be mapped to analog actions (e.g. "AnalogSeekForward") or digital
   * actions (e.g. "Up").
   */
  class IKeyHandler
  {
  public:
    virtual ~IKeyHandler() = default;

    /*!
     * \brief Return true if the key is "pressed" (has a magnitude greater
     *        than 0.5)
     *
     * \return True if the key is "pressed", false otherwise
     */
    virtual bool IsPressed() const = 0;

    /*!
     * \brief A key mapped to a digital feature has been pressed or released
     *
     * \param bPressed   true if the key's button/axis is activated, false if deactivated
     * \param holdTimeMs The held time in ms for pressed buttons, or 0 for released
     *
     * \return True if the key is mapped to an action, false otherwise
     */
    virtual bool OnDigitalMotion(bool bPressed, unsigned int holdTimeMs) = 0;

    /*!
     * \brief Callback for keys mapped to analog features
     *
     * \param magnitude     The amount of the analog action
     * \param motionTimeMs  The time since the magnitude was 0
     *
     * \return True if the key is mapped to an action, false otherwise
     */
    virtual bool OnAnalogMotion(float magnitude, unsigned int motionTimeMs) = 0;
  };
}
}
