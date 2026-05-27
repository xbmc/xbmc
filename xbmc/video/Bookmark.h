/*
 *  Copyright (C) 2005-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <chrono>
#include <string>
#include <vector>

class CBookmark;
typedef std::vector<CBookmark> VECBOOKMARKS;

class CBookmark
{
public:
  enum EType
  {
    STANDARD = 0,
    RESUME = 1,
    EPISODE = 2
  } type;

  CBookmark();
  void Reset();

  /*! \brief returns true if this bookmark has been set.
   \return true if totalTimeInSeconds is positive.
   */
  bool IsSet() const;

  /*! \brief returns true if this bookmark is part way through the video file
   \return true if both totalTimeInSeconds and timeInSeconds are positive.
   */
  bool IsPartWay() const;

  /*! \brief returns true if this bookmark has a stored serialized player state
   \return true if playerState is not empty.
   */
  bool HasSavedPlayerState() const;

  /*!
   * \brief Retrieve bookmarks for the file
   * \param[in] filePath file
   * \param[in,out] bookmarks list of bookmarks
   * \param[in] types types of bookmark to retrieve
   * \return true for success, false otherwise
   */
  static bool GetBookmarksForFile(const std::string& filePath,
                                  VECBOOKMARKS& bookmarks,
                                  std::vector<EType> types);

  /*!
   * \brief Create a list of the start timestamps of the standard bookmarks from the provided list
   * \param[in] bookmarks list of bookmarks
   * \return list of bookmarks' start timestamp
   */
  static std::vector<std::chrono::milliseconds> BookmarksToPositions(const VECBOOKMARKS& bookmarks);
  /*!
   * \brief Add a bookmark to the provided list
   * \param[in] bookmark the bookmark
   * \param[in] positions list of bookmark start timestamps
   */
  static void AddToPositions(const CBookmark& bookmark,
                             std::vector<std::chrono::milliseconds>& positions);

  double timeInSeconds;
  double totalTimeInSeconds;
  long partNumber;
  std::string thumbNailImage;
  std::string playerState;
  std::string player;
  long seasonNumber;
  long episodeNumber;
};
