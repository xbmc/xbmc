/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>

class CFileItemList;
class CVideoDatabase;

namespace KODI::VIDEO
{
class CVideoInfoScannerArt;

/*!
 \brief Adds the video extras a scan finds to the movies they belong to.

 One class belongs to a scan, and lives as long as it does.
 */
class CVideoInfoScannerExtras
{
public:
  CVideoInfoScannerExtras(CVideoDatabase& database, const CVideoInfoScannerArt& art);

  /*!
   \brief Add the videos in an extras folder, and in the folders below it, as extras of the movie
   listed beside it.
   \param items the listing holding the extras folder
   \param path the extras folder
   \return false if no movie was found for the extras, true otherwise
   */
  bool AddVideoExtras(const CFileItemList& items, const std::string& path);

  /*!
   \brief Add the videos in an extras folder, and in the folders below it, as extras of a movie.
   \param dbId the movie
   \param path the extras folder
   */
  void AddVideoExtras(int dbId, const std::string& path);

private:
  CVideoDatabase& m_database;
  const CVideoInfoScannerArt& m_art;
};
} // namespace KODI::VIDEO
