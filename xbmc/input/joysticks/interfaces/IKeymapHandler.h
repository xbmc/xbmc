/*
 *      Copyright (C) 2017-present Team Kodi
 *      http://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
 *
 */
#pragma once

#include <set>
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
     * \brief Get the pressed state of the given keys
     *
     * \param keyNames The key names
     *
     * \return True if all keys are pressed or no keys are given, false otherwise
     */
    virtual bool HotkeysPressed(const std::set<std::string> &keyNames) const = 0;

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
