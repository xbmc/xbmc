/*
 *  Copyright (C) 2005-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

class CFileItem;
class CFileItemList;
class CURL;
class CVideoInfoTag;

namespace XFILE
{

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

struct DiscStreamInfo
{
  bool operator==(const DiscStreamInfo&) const = default;

  unsigned int coding{0};
  unsigned int format{0};
  unsigned int rate{0};
  unsigned int aspect{0};
  std::string lang;
};

struct PlaylistInfo
{
  unsigned int playlist{0};
  unsigned int duration{0}; // seconds
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

static constexpr unsigned int MIN_EPISODE_DURATION = 10 * 60; // 10 minutes
static constexpr unsigned int MAX_EPISODE_DIFFERENCE = 30; // 30 seconds
static constexpr unsigned int MIN_SPECIAL_DURATION = 5 * 60; // 5 minutes
static constexpr unsigned int MAIN_TITLE_LENGTH_PERCENT = 70;

class CDiscDirectoryHelper
{
  enum class IsSpecial : bool
  {
    SPECIAL,
    EPISODE
  };

  enum class AllEpisodes : bool
  {
    SINGLE,
    ALL
  };

  struct CandidatePlaylistsDurationInformation
  {
    unsigned int playlist{0};
    int durationDelta{0};
    unsigned int chapters{0};
  };

  struct SortedPlaylistsInformation
  {
    unsigned int playlist{0};
    unsigned int index{0};
    std::string languages;
  };

public:
  CDiscDirectoryHelper();

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
  bool GetEpisodePlaylists(const CURL& url,
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

private:
  void InitialisePlaylistSearch(int episodeIndex, const std::vector<CVideoInfoTag>& episodesOnDisc);
  void FindPlayAllPlaylists(const ClipMap& clips, const PlaylistMap& playlists);
  void FindGroups(const PlaylistMap& playlists);
  void FindCandidatePlaylists(const std::vector<CVideoInfoTag>& episodesOnDisc,
                              unsigned int episodeIndex,
                              const PlaylistMap& playlists);
  void FindSpecials(const PlaylistMap& playlists);
  static void GenerateItem(const CURL& url,
                           const std::shared_ptr<CFileItem>& item,
                           unsigned int playlist,
                           const PlaylistMap& playlists,
                           const CVideoInfoTag& tag,
                           IsSpecial isSpecial);
  void EndPlaylistSearch();
  void PopulateFileItems(const CURL& url,
                         CFileItemList& items,
                         int episodeIndex,
                         const std::vector<CVideoInfoTag>& episodesOnDisc,
                         const PlaylistMap& playlists);

  AllEpisodes m_allEpisodes;
  IsSpecial m_isSpecial;
  unsigned int m_numEpisodes;
  unsigned int m_numSpecials;

  std::vector<unsigned int> m_playAllPlaylists;
  std::map<unsigned int, std::map<unsigned int, std::vector<unsigned int>>> m_playAllPlaylistsMap;
  std::vector<std::vector<unsigned int>> m_groups;
  std::map<unsigned int, unsigned int> m_candidatePlaylists;
  std::vector<unsigned int> m_candidateSpecials;
};
} // namespace XFILE
