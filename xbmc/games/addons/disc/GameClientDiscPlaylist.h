/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>
#include <string_view>

namespace KODI
{
namespace GAME
{

/*!
 * \brief Shared path helpers for disc-state playlist persistence formats (XML/M3U)
 */
class CGameClientDiscPlaylist
{
public:
  /*!
   * \brief Build the path of a game's disc state file
   *
   * \param gamePath The game the disc state belongs to
   * \param extension The file's extension, with its leading dot
   */
  static std::string GetStateFilePath(const std::string& gamePath, std::string_view extension);
};

} // namespace GAME
} // namespace KODI
