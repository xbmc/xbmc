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

#include "input/joysticks/JoystickTypes.h"

#include <string>

class IKeymapEnvironment;

/*!
  * \brief Interface for mapping buttons to Kodi actions
  *
  * \sa CJoystickUtils::MakeKeyName
  */
class IKeymap
{
public:
  virtual ~IKeymap() = default;

  /*!
   * \brief The controller ID
   *
   * This is required because key names are specific to each controller
   */
  virtual std::string ControllerID() const = 0;

  /*!
   * \brief Access properties of the keymapping environment
   */
  virtual const IKeymapEnvironment *Environment() const = 0;

  /*!
   * \brief Get the actions for a given key name
   *
   * \param keyName   The key name created by CJoystickUtils::MakeKeyName()
   *
   * \return A list of actions associated with the given key
   */
  virtual const KODI::JOYSTICK::KeymapActionGroup& GetActions(const std::string& keyName) const = 0;
};

/*!
  * \brief Interface for mapping buttons to Kodi actions for specific windows
  *
  * \sa CJoystickUtils::MakeKeyName
  */
class IWindowKeymap
{
public:
  virtual ~IWindowKeymap() = default;

  /*!
   * \brief The controller ID
   *
   * This is required because key names are specific to each controller
   */
  virtual std::string ControllerID() const = 0;

  /*!
   * \brief Add an action to the keymap for a given key name and window ID
   *
   * \param windowId  The window ID to look up
   * \param keyName   The key name created by CJoystickUtils::MakeKeyName()
   * \param action    The action to map
   */
  virtual void MapAction(int windowId, const std::string &keyName, KODI::JOYSTICK::KeymapAction action) = 0;

  /*!
   * \brief Get the actions for a given key name and window ID
   *
   * \param windowId  The window ID to look up
   * \param keyName   The key name created by CJoystickUtils::MakeKeyName()
   *
   * \return A list of actions associated with the given key for the given window
   */
  virtual const KODI::JOYSTICK::KeymapActionGroup& GetActions(int windowId, const std::string& keyName) const = 0;
};
