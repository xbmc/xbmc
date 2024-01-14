/*
 *  Copyright (C) 2017-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <set>
#include <string>

namespace KODI
{
namespace KEYMAP
{
/*!
 * \ingroup keymap
 *
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
  virtual bool HotkeysPressed(const std::set<std::string>& keyNames) const = 0;

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
} // namespace KEYMAP
} // namespace KODI
