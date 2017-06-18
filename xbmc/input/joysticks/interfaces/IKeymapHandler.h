/*
 *      Copyright (C) 2017 Team Kodi
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

#include <string>

namespace KODI
{
namespace JOYSTICK
{
  /*!
   * \ingroup joystick
   * \brief Interface for a class working with a keymap
   */
  class IKeymapHandler
  {
  public:
    virtual ~IKeymapHandler() = default;
    
    /*!
     * \brief Get the pressed state of the key
     *
     * \param keyName The key name
     *
     * \return True if the key is pressed, false otherwise
     */
    virtual bool IsPressed(const std::string& keyName) const = 0;

    /*!
     * \brief Get the key name of the last button pressed
     *
     * \return The key name of the last-pressed button, or empty if no button
     *         is pressed
     */
    virtual std::string GetLastPressed() const = 0;

    /*!
     * \brief Called when a key has emitted an action after bring pressed
     *
     * \param keyName the key name that emitted the action
     */
    virtual void OnPress(const std::string& keyName) = 0;
  };
}
}
