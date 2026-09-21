/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>

namespace KODI::GAME
{
/*!
 * \ingroup games
 *
 * \brief Filename helpers for cheat discovery and remembered pack selections
 */
class CCheatUtils
{
public:
  /*!
   * \brief Derive the filename used for exact cheat-pack matching
   *
   * \param gamePath The game file's local path or VFS URL
   *
   * \return The game filename with its extension replaced by .cht, preserving
   *         the rest of the filename, including region and revision tags
   */
  static std::string GetCheatFileName(const std::string& gamePath);

  /*!
   * \brief Derive the filename for the game's remembered cheat-pack selection
   *
   * \param gamePath The game file's local path or VFS URL
   *
   * \return The sanitized game filename, including its extension, followed by
   *         an underscore, the full path's eight-digit hexadecimal CRC32, and .xml
   */
  static std::string GetSelectionFileName(const std::string& gamePath);
};
} // namespace KODI::GAME
