/*
 *  Copyright (C) 2017-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "input/keymaps/KeymapTypes.h"

#include <string>

namespace KODI
{
namespace KEYMAP
{
class IKeymapEnvironment;

/*!
 * \ingroup keymap
 *
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
  virtual const IKeymapEnvironment* Environment() const = 0;

  /*!
   * \brief Get the actions for a given key name
   *
   * \param keyName   The key name created by CJoystickUtils::MakeKeyName()
   *
   * \return A list of actions associated with the given key
   */
  virtual const KeymapActionGroup& GetActions(const std::string& keyName) const = 0;
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
  virtual void MapAction(int windowId, const std::string& keyName, KeymapAction action) = 0;

  /*!
   * \brief Get the actions for a given key name and window ID
   *
   * \param windowId  The window ID to look up
   * \param keyName   The key name created by CJoystickUtils::MakeKeyName()
   *
   * \return A list of actions associated with the given key for the given window
   */
  virtual const KeymapActionGroup& GetActions(int windowId, const std::string& keyName) const = 0;
};
} // namespace KEYMAP
} // namespace KODI
