/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

class CKey;

namespace KODI
{
namespace KEYBOARD
{
/*!
 * \brief Interface for translating keys to action IDs
 */
class IActionMap
{
public:
  virtual ~IActionMap() = default;

  /*!
   * \brief Get the action ID mapped to the specified key
   *
   * \param key The key to look up
   *
   * \return The action ID from ActionIDs.h, or ACTION_NONE if no action is
   *         mapped to the specified key
   */
  virtual unsigned int GetActionID(const CKey& key) = 0;
};
} // namespace KEYBOARD
} // namespace KODI
