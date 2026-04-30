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
   * \brief Root location where RetroPlayer stores per-game disc-state data
   */
  static std::string GetDiscStateDirectory();

  /*!
   * \brief Sanitized base file name derived from the provided game path
   */
  static std::string GetSafeBaseName(const std::string& gamePath);

  /*!
   * \brief Per-game subdirectory name used to isolate state files by source path and CRC
   */
  static std::string GetStateSubdirectory(const std::string& gamePath);

  /*!
   * \brief Build a complete state file path for the given extension (without leading dot)
   */
  static std::string GetStateFilePath(const std::string& gamePath, std::string_view extension);
};

} // namespace GAME
} // namespace KODI
