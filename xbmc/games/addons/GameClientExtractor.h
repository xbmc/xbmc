/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>

namespace KODI
{
namespace GAME
{
/*!
 * \ingroup games
 *
 * \brief Copies a game out of its archive for a game client that reads only
 *        local files
 *
 * Copies go beside the folders that hold each game's saves and are kept, because
 * a disk-based game saves onto its own disks.
 */
class CGameClientExtractor
{
public:
  /*!
   * \param supportsFolders Whether the client boots a whole folder, and so
   *        needs every file beside the one it runs
   * \param supportsPlaylists Whether the client takes an .m3u playlist of disks
   */
  CGameClientExtractor(bool supportsFolders, bool supportsPlaylists);

  /*!
   * \brief Copy a game out of its archive
   *
   * \return The copy's local path, or empty if it could not be made
   */
  std::string Extract(const std::string& archivedPath) const;

private:
  /*!
   * \brief Copy a game, and any other disks of it, out of its archive
   */
  std::string ExtractGame(const std::string& archivedPath) const;

  /*!
   * \brief Unpack the whole archive a game is in
   */
  static std::string ExtractArchive(const std::string& archivedPath);

  static bool CopyTree(const std::string& from, const std::string& to);

  const bool m_supportsFolders;
  const bool m_supportsPlaylists;
};
} // namespace GAME
} // namespace KODI
