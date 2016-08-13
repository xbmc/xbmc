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

#include "JoystickTypes.h"

namespace JOYSTICK
{
  /*!
   * \brief Interface for handling keymap keys
   *
   * Keys can be mapped to analog actions (e.g. "AnalogSeekForward") or digital
   * actions (e.g. "Up").
   */
  class IKeymapHandler
  {
  public:
    virtual ~IKeymapHandler(void) { }

    /*!
     * \brief Get the type of action mapped to the specified key ID
     *
     * \param keyId  The key ID from Key.h
     *
     * \return The type of action mapped to keyId, or INPUT_TYPE::UNKNOWN if
     *         no action is mapped to the specified key
     */
    virtual INPUT_TYPE GetInputType(unsigned int keyId) const = 0;

    /*!
     * \brief A key mapped to a digital action has been pressed or released
     *
     * \param keyId      The key ID from Key.h
     * \param bPressed   true if the key's button/axis is activated, false if deactivated
     * \param holdTimeMs The held time in ms for pressed buttons, or 0 for released
     */
    virtual void OnDigitalKey(unsigned int keyId, bool bPressed, unsigned int holdTimeMs = 0) = 0;

    /*!
     * \brief Callback for keys mapped to analog actions
     *
     * \param keyId      The button key ID from Key.h
     * \param magnitude  The amount of the analog action
     *
     * If keyId is not mapped to an analog action, no action need be taken
     */
    virtual void OnAnalogKey(unsigned int buttonKeyId, float magnitude) = 0;
  };
}
