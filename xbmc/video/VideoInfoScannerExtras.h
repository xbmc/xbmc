/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>

class CFileItem;
class CFileItemList;
class CVideoDatabase;

namespace KODI::VIDEO
{
class CVideoInfoScannerArt;

/*!
 \brief The extra a movie already has that is the same as one of a disc of it. A movie on more than
 one disc (eg. its 4K and HD editions) may have the same extras on each, named a little differently
 (eg. Gag Reel and Gagreel), and playing all of a kind of extra under a different code.
 \param existing the extras the movie has
 \param extra one of the extras a disc names
 \return the same extra, one of existing, or nullptr where the movie has none
 */
const CFileItem* FindExtraOnAnotherDisc(const CFileItemList& existing, const CFileItem& extra);

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

  /*!
   \brief Add the extras each bluray of a movie names, once the movie is in the library.
   \param item the movie, which may be a stack of discs
   */
  void AddMovieDiscExtras(const CFileItem& item);

  /*!
   \brief Add the video extras folders of a movie whose folder holds its disc structure. Such a
   folder is listed as the disc's file, so its extras folders are not in the listing scanned.
   \param discFolder the folder holding the movie's disc structure
   */
  void AddVideoExtrasBesideDisc(const std::string& discFolder);

private:
  /*!
   \brief Add the extras a bluray names as extras of a movie.
   \param disc path of the disc (index.bdmv, an .iso, or one of its bluray:// playlists)
   \param dbId the movie
   \return true if the disc names extras, whether or not they were already in the library
   */
  bool AddDiscExtras(const std::string& disc, int dbId);

  CVideoDatabase& m_database;
  const CVideoInfoScannerArt& m_art;
};
} // namespace KODI::VIDEO
