/*
 *  Copyright (C) 2005-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <map>
#include <string>
#include <utility>
#include <vector>

class CFileItem;
class CFileItemList;
class CURL;
class CVideoInfoTag;

namespace XFILE
{

static constexpr unsigned int MIN_EPISODE_DURATION = 10 * 60; // 10 minutes
static constexpr unsigned int MAX_EPISODE_DIFFERENCE = 30; // 30 seconds
static constexpr unsigned int MIN_SPECIAL_DURATION = 5 * 60; // 5 minutes
static constexpr unsigned int MAIN_TITLE_LENGTH_PERCENT = 70;

struct DiscStreamInfo
{
  unsigned int coding{0};
  unsigned int format{0};
  unsigned int rate{0};
  unsigned int aspect{0};
  std::string lang;
};

struct PlaylistInfo
{
  unsigned int playlist;
  unsigned int duration{0};
  std::vector<unsigned int> clips;
  std::map<unsigned int, unsigned int> clipDuration;
  std::vector<unsigned int> chapters;
  std::vector<DiscStreamInfo> videoStreams;
  std::vector<DiscStreamInfo> audioStreams;
  std::vector<DiscStreamInfo> pgStreams;
  std::string languages;
};

struct ClipInfo
{
  unsigned int duration{0};
  std::vector<unsigned int> playlists;
};

using PlaylistMap = std::map<unsigned int, PlaylistInfo>;
using ClipMap = std::map<unsigned int, ClipInfo>;

enum class GetTitles : uint8_t
{
  GET_TITLES_ONE = 0,
  GET_TITLES_MAIN,
  GET_TITLES_ALL
};

enum class SortTitles : uint8_t
{
  SORT_TITLES_NONE = 0,
  SORT_TITLES_EPISODE,
  SORT_TITLES_MOVIE
};

enum class AddMenuOption : bool
{
  NO_MENU,
  ADD_MENU
};

class CDiscDirectoryHelper
{
public:
  /*!
   * \brief Populates a CFileItemList with the playlist(s) corresponding to the given episode.
   * \param url bluray:// episode url
   * \param items CFileItemList to populate
   * \param episodeIndex index into episodesOnDisc
   * \param episodesOnDisc vector array of CVideoInfoTags - one for each episode on the disc (populated by GetEpisodesOnDisc)
   * \param clips map of clips on disc (populated in CBlurayDirectory)
   * \param playlists map of playlists on disc (populated in CBlurayDirectory)
   * \return true if at least one playlist is found, otherwise false
   */
  static bool GetEpisodePlaylists(const CURL& url,
                                  CFileItemList& items,
                                  int episodeIndex,
                                  const std::vector<CVideoInfoTag>& episodesOnDisc,
                                  const ClipMap& clips,
                                  const PlaylistMap& playlists);

  /*!
   * \brief Populates a vector array of CVideoInfoTags with information for the episodes on a bluray disc
   * \param url bluray:// episode url
   * \return vector array of CVideoInfoTags containing episode information
   */
  static std::vector<CVideoInfoTag> GetEpisodesOnDisc(const CURL& url);

  /*!
   * \brief Add All Titles and, if appropriate, Menu options to the CFileItemList
   * \param url bluray:// episode url
   * \param items CFileItemList to populate
   * \param addMenuOption Bluray disc has menu, so add Menu Option
   */
  static void AddRootOptions(const CURL& url, CFileItemList& items, AddMenuOption addMenuOption);

  /*!
   * \brief Generate label for episode range (eg. Episodes 1-4) and update plot
   * \param newItem the CFileItem being updated
   * \param item The source CFileItem
   * \return label
   */
  static std::string GetEpisodesLabel(CFileItem& newItem, const CFileItem& item);
};
} // namespace XFILE
