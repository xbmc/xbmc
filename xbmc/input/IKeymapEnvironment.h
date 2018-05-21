/*
 *      Copyright (C) 2017-present Team Kodi
 *      This file is part of Kodi - https://kodi.tv
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

/*!
 * \brief Customizes the environment in which keymapping is performed
 *
 * By overriding GetWindowID() and GetFallthrough(), an agent can customize
 * the behavior of the keymap by forcing a window and preventing the use of
 * a fallback window, respectively.
 *
 * An agent can also inform the keymap that it isn't accepting input currently,
 * allowing the input to fall through to the next input handler.
 */
class IKeymapEnvironment
{
public:
  virtual ~IKeymapEnvironment() = default;

  /*!
   * \brief Get the window ID for which actions should be translated
   *
   * \return The window ID
   */
  virtual int GetWindowID() const = 0;

  /*!
   * \brief Set the window ID
   *
   * \param The window ID, used for translating actions
   */
  virtual void SetWindowID(int windowId) = 0;

  /*!
   * \brief Get the fallthrough window to when a key definition is missing
   *
   * \param windowId The window ID
   *
   * \return The window ID, or -1 for no fallthrough
   */
  virtual int GetFallthrough(int windowId) const = 0;

  /*!
   * \brief Specify if the global keymap should be used when the window and
   *        fallback window are undefined
   */
  virtual bool UseGlobalFallthrough() const = 0;

  /*!
   * \brief Specify if the agent should monitor for easter egg presses
   */
  virtual bool UseEasterEgg() const = 0;
};
