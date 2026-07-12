/*
 *  Copyright (C) 2005-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "Directory.h"
#include "video/Episode.h"
#include "video/VideoInfoTag.h"

#include <chrono>
#include <cstdint>
#include <map>
#include <set>
#include <string>
#include <vector>

class CFileItem;
class CFileItemList;
class CURL;
class CVideoInfoTag;

namespace XFILE
{
using namespace std::chrono_literals;

static constexpr int ALL_PLAYLISTS{-1};

enum class GetTitle : int
{
  SINGLE = -1,
  MAIN = -2,
  EPISODES = -3,
  ALL = -4
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

enum class MenuDecision : uint8_t
{
  NO_ACTION,
  SILENT,
  SHOW_SIMPLE_MENU,
  SHOW_DISC_MENU,
  GET_MAIN_TITLE
};

enum class ENCODING_TYPE : uint8_t
{
  // Video
  VIDEO_MPEG2 = 0x02,
  VIDEO_VC1 = 0xea,
  VIDEO_H264 = 0x1b,
  VIDEO_H264_MVC = 0x20,
  VIDEO_HEVC = 0x24,

  // Audio
  AUDIO_LPCM = 0x80,
  AUDIO_AC3 = 0x81,
  AUDIO_DTS = 0x82,
  AUDIO_TRUHD = 0x83,
  AUDIO_AC3PLUS = 0x84,
  AUDIO_DTSHD = 0x85,
  AUDIO_DTSHD_MASTER = 0x86,
  AUDIO_AC3PLUS_SECONDARY = 0xa1,
  AUDIO_DTSHD_SECONDARY = 0xa2,

  // Other
  SUB_PG = 0x90,
  SUB_IG = 0x91,
  SUB_TEXT = 0x92,
};

enum class ASPECT_RATIO : uint8_t
{
  RATIO_4_3 = 2,
  RATIO_16_9 = 3
};

struct PlaylistInformation
{
  unsigned int playlist{0};
  std::chrono::milliseconds duration{0ms};
  std::vector<unsigned int> clips;
  std::map<unsigned int, std::chrono::milliseconds> clipDuration;
  std::vector<std::chrono::milliseconds> chapters;
  std::vector<VideoStreamInfo> videoStreams;
  std::vector<AudioStreamInfo> audioStreams;
  std::vector<SubtitleStreamInfo> pgStreams;
  std::string languages;

  void clear()
  {
    playlist = 0;
    duration = 0ms;
    clips.clear();
    clipDuration.clear();
    chapters.clear();
    videoStreams.clear();
    audioStreams.clear();
    pgStreams.clear();
    languages.clear();
  }
};

struct ClipInfo
{
  std::chrono::milliseconds duration{0ms};
  std::vector<unsigned int> playlists;
};

using PlaylistMap = std::map<unsigned int, PlaylistInformation>;
using ClipMap = std::map<unsigned int, ClipInfo>;
using Episode = KODI::VIDEO::EPISODE;
using Episodes = std::vector<KODI::VIDEO::EPISODE>;

// Episodes
static constexpr std::chrono::milliseconds MAX_EPISODE_DIFFERENCE{30 * 1000}; // 30 seconds
static constexpr std::chrono::milliseconds MIN_SPECIAL_DURATION{5 * 60 * 1000}; // 5 minutes
static constexpr int DURATION_TOLERANCE_PERCENT{20};

// Movies
static constexpr std::chrono::milliseconds MIN_MOVIE_DURATION{30 * 60 * 1000}; // 30 minutes
static constexpr int MAIN_TITLE_LENGTH_PERCENT{70};

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

  struct CandidatePlaylistInformation
  {
    unsigned int playlist{0};
    unsigned int index{0};
    unsigned int playAllPlaylistEpisodesStartOffset{0};
    std::chrono::milliseconds duration{0ms};
    std::chrono::milliseconds durationDelta{0ms};
    int multiple{0};
    unsigned int chapters{0};
    std::vector<unsigned int> clips;
    std::string languages;

    // Used for inserting into a set where playlist is the key
    auto operator<=>(const CandidatePlaylistInformation& rhs) const noexcept
    {
      return playlist <=> rhs.playlist;
    }
  };

public:
  CDiscDirectoryHelper();
  CDiscDirectoryHelper(const CDiscDirectoryHelper&) = delete;
  CDiscDirectoryHelper& operator=(const CDiscDirectoryHelper&) = delete;

  /*!
   * \brief Populates a CFileItemList with the playlist(s) corresponding to the given episode.
   * \param url bluray:// episode url
   * \param items CFileItemList to populate
   * \param allTitles CFileItemList of all titles on the disc (populated by CBlurayDirectory). Used for streamdetails.
   * \param episodeIndex index into episodesOnDisc
   * \param episodesOnDisc vector array of CVideoInfoTags - one for each episode on the disc (populated by GetEpisodesOnDisc)
   * \param clips map of clips on disc (populated in CBlurayDirectory)
   * \param playlists map of playlists on disc (populated in CBlurayDirectory)
   * \return true if at least one playlist is found, otherwise false
   */
  bool GetEpisodePlaylists(const CURL& url,
                           CFileItemList& items,
                           const CFileItemList& allTitles,
                           int episodeIndex,
                           const Episodes& episodesOnDisc,
                           const ClipMap& clips,
                           const PlaylistMap& playlists);

  /*!
   * \brief Populates a CFileItemList with the playlist(s) corresponding to the given episode.
   * \param url bluray:// episode url
   * \param items CFileItemList to populate
   * \param allTitles CFileItemList of all titles on the disc (populated by CBlurayDirectory). Used for streamdetails.
   * \param job determines whether to get all valid episode playlists or the most likely ones
   * \param episodesOnDisc vector array of CVideoInfoTags - one for each episode on the disc (populated by GetEpisodesOnDisc)
   * \param clips map of clips on disc (populated in CBlurayDirectory)
   * \param playlists map of playlists on disc (populated in CBlurayDirectory)
   * \return true if at least one playlist is found, otherwise false
   */
  bool GetAllEpisodePlaylists(const CURL& url,
                              CFileItemList& items,
                              const CFileItemList& allTitles,
                              GetTitle job,
                              const Episodes& episodesOnDisc,
                              const ClipMap& clips,
                              const PlaylistMap& playlists);

  /*!
   * \brief Populates a CFileItemList with the playlist(s) corresponding to the main movie.
   * \param url bluray:// url
   * \param items CFileItemList to populate
   * \param allTitles CFileItemList of all titles on the disc (populated by CBlurayDirectory). Used for streamdetails.
   * \param mainPlaylist the main playlist number (if known, from disc.inf), otherwise -1
   * \param job determines whether to get all possible movie playlists (ie. multiple versions) or just one (for initial library scan)
   * \param clips map of clips on disc (populated in CBlurayDirectory)
   * \param playlistMap map of playlists on disc (populated in CBlurayDirectory)
   * \return true if at least one playlist is found, otherwise false
   */
  bool GetMoviePlaylists(const CURL& url,
                         CFileItemList& items,
                         const CFileItemList& allTitles,
                         int mainPlaylist,
                         GetTitle job,
                         const ClipMap& clips,
                         const PlaylistMap& playlistMap);

  /*!
   * \brief Populates a vector array of CVideoInfoTags with information for the episodes on a bluray disc
   * \param url bluray:// episode url
   * \return vector array of CVideoInfoTags containing episode information
   */
  static std::vector<CVideoInfoTag> GetEpisodesOnDisc(const CURL& url);

  enum class AllTitles : bool
  {
    EPISODES,
    MOVIES
  };

  /*!
   * \brief Add All Titles and, if appropriate, Menu options to the CFileItemList
   * \param url bluray:// episode url
   * \param items CFileItemList to populate
   * \param allTitlesType Determines whether to add All Episodes or All Movies option
   * \param addMenuOption Bluray disc has menu, so add Menu Option
   */
  static void AddRootOptions(const CURL& url,
                             CFileItemList& items,
                             AllTitles allTitlesType,
                             AddMenuOption addMenuOption);

  /*!
   * \brief Either shows simple menu to select playlist, chooses main feature (movie/episode) playlists or returns if disc menu will be used later.
   * \param item FileItem containing details of desired movie/episode. This is updated with the selected playlist.
   * \param playback Determines if the simple dialog should be shown or the main title selected (if possible).
   * \return true if a playlist was selected or if the disc menu will be used later, false if the user cancelled.
   */
  static bool GetOrShowPlaylistSelection(CFileItem& item, MenuDecision playback);

protected:
  static bool GetDirectoryItems(const std::string& path,
                                CFileItemList& items,
                                const CDirectory::CHints& hints,
                                bool silent = false);

private:
  void InitialiseEpisodePlaylistSearch(int episodeIndex, const Episodes& episodesOnDisc);
  void StorePlayAllPlaylist(
      unsigned int playlistNumber,
      unsigned int playAllPlaylistEpisodesStartOffset,
      const PlaylistInformation& playlistInformation,
      const std::map<unsigned int, std::vector<unsigned int>>& playAllPlaylistClipMap);
  void FindPlayAllPlaylists(const ClipMap& clips, const PlaylistMap& playlists);
  void FindGroups(const PlaylistMap& playlists, const Episodes& episodesOnDisc);
  void UsePlayAllPlaylistMethod(int episodeIndex, const PlaylistMap& playlists);
  void UseLongOrCommonMethodForSingleEpisode(int episodeIndex, const PlaylistMap& playlists);
  static std::vector<std::vector<CandidatePlaylistInformation>> GetGroupsWithoutDuplicates(
      const std::vector<std::vector<CandidatePlaylistInformation>>& groups);
  void GetPlaylistsFromGroup(int episodeIndex,
                             const std::vector<CandidatePlaylistInformation>& group);
  void UseGroupMethod(int episodeIndex,
                      const Episodes& episodesOnDisc,
                      const PlaylistMap& playlists);
  bool CheckGroupDurations(const std::vector<CandidatePlaylistInformation>& group,
                           const Episodes& episodesOnDisc,
                           int durationTolerancePercent = DURATION_TOLERANCE_PERCENT) const;
  bool CheckGroupDurations(const std::vector<CandidatePlaylistInformation>& groupA,
                           const std::vector<CandidatePlaylistInformation>& groupB,
                           int durationTolerancePercent = DURATION_TOLERANCE_PERCENT) const;
  bool CheckGroup(const std::vector<CandidatePlaylistInformation>& group,
                  const Episodes& episodesOnDisc) const;
  static std::chrono::milliseconds CalculateAverageOfShortEpisodes(
      const std::vector<CandidatePlaylistInformation>& group);
  void UseGroupsWithMultiplesMethod(int episodeIndex, const Episodes& episodesOnDisc);
  void ChooseSingleBestPlaylist(const Episodes& episodesOnDisc);
  void AddIdenticalPlaylists(const PlaylistMap& playlists);
  void FindCandidatePlaylists(const Episodes& episodesOnDisc,
                              int episodeIndex,
                              const PlaylistMap& playlists);
  void FindSpecials(const PlaylistMap& playlists);
  static void EndEpisodePlaylistSearch();
  void PopulateEpisodeFileItems(const CURL& url,
                                CFileItemList& items,
                                const CFileItemList& allTitles,
                                int episodeIndex,
                                const Episodes& episodesOnDisc,
                                const PlaylistMap& playlists) const;
  bool FilterAllEpisodesPlaylists(std::vector<PlaylistInformation>& playlists, GetTitle job);

  std::chrono::milliseconds m_minEpisodeDuration{0ms};

  AllEpisodes m_allEpisodes{AllEpisodes::SINGLE};
  IsSpecial m_isSpecial{IsSpecial::EPISODE};
  unsigned int m_numEpisodes{0};
  unsigned int m_numSpecials{0};

  struct Compare
  {
    using is_transparent = void;

    bool operator()(const CandidatePlaylistInformation& a,
                    const CandidatePlaylistInformation& b) const
    {
      return a.playlist < b.playlist;
    }
    bool operator()(const CandidatePlaylistInformation& a, unsigned int id) const
    {
      return a.playlist < id;
    }
    bool operator()(unsigned int id, const CandidatePlaylistInformation& a) const
    {
      return id < a.playlist;
    }
  };

  std::set<CandidatePlaylistInformation, Compare> m_playAllPlaylists;
  std::map<unsigned int, std::map<unsigned int, std::vector<unsigned int>>> m_playAllPlaylistsMap;
  std::vector<std::vector<CandidatePlaylistInformation>> m_groups;
  std::vector<std::vector<CandidatePlaylistInformation>> m_allGroups;
  std::map<unsigned int, CandidatePlaylistInformation> m_candidatePlaylists;
  std::set<unsigned int> m_candidateSpecials;
  std::vector<CandidatePlaylistInformation> m_nthLongestPlaylists;

  static bool GetItems(CFileItemList& items, const std::string& directory, bool silent = false);
};
} // namespace XFILE
