/*
 *  Copyright (C) 2017-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

class CKey;

namespace KODI
{
namespace KEYMAP
{
/*!
 * \ingroup keymap
 *
 * \brief Interface for translating keyboard keys to action IDs
 */
class IKeyboardActionMap
{
public:
  virtual ~IKeyboardActionMap() = default;

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
} // namespace KEYMAP
} // namespace KODI
