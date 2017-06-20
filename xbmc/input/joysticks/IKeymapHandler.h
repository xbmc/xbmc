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
  class IKeymapHandler
  {
  public:
    virtual ~IKeymapHandler() = default;

    /*!
     * \brief Get the action ID mapped to the specified key ID
     *
     * \param keyId  The key ID from Key.h
     * \param windowId   The window ID from WindowIDs.h
     * \param bFallthrough Use a key from an underlying window or global keymap
     *
     * \return The action ID, or ACTION_NONE if no action is mapped to the
     *         specified key
     */
    virtual unsigned int GetActionID(unsigned int keyId, int windowId, bool bFallthrough) const = 0;

    /*!
     * \brief Get the time required to hold the button before calling OnDigitalKey()
     *
     * \param keyId      The key ID from Key.h
     * \param windowId   The window ID from WindowIDs.h
     * \param bFallthrough Use a key from an underlying window or global keymap
     *
     * \return The hold time, in ms
     */
    virtual unsigned int GetHoldTimeMs(unsigned int keyId, int windowId, bool bFallthrough) const = 0;

    /*!
     * \brief A key mapped to a digital action has been pressed or released
     *
     * \param keyId      The key ID from Key.h
     * \param windowId   The window ID from WindowIDs.h
     * \param bFallthrough Use a key from an underlying window or global keymap
     * \param bPressed   true if the key's button/axis is activated, false if deactivated
     * \param holdTimeMs The held time in ms for pressed buttons, or 0 for released
     */
    virtual void OnDigitalKey(unsigned int keyId, int windowId, bool bPressed, bool bFallthrough, unsigned int holdTimeMs = 0) = 0;

    /*!
     * \brief Callback for keys mapped to analog actions
     *
     * \param keyId      The button key ID from Key.h
     * \param windowId   The window ID from WindowIDs.h
     * \param bFallthrough Use a key from an underlying window or global keymap
     * \param magnitude  The amount of the analog action
     * \param motionTimeMs The time elapsed since the magnitude was 0
     *
     * If keyId is not mapped to an analog action, no action need be taken
     */
    virtual void OnAnalogKey(unsigned int keyId, int windowId, bool bFallthrough, float magnitude, unsigned int motionTimeMs) = 0;
  };
}
}
