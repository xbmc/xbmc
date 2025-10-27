/*
 *  Copyright (C) 2005-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */
#include "DiscDirectoryHelper.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "URL.h"
#include "guilib/LocalizeStrings.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"
#include "video/VideoDatabase.h"
#include "video/VideoInfoTag.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <iterator>
#include <numeric>
#include <ranges>
#include <set>
#include <string>

using namespace XFILE;
using namespace std::chrono_literals;

using PlaylistMapEntry = std::pair<unsigned int, PlaylistInfo>;
using PlaylistVector = std::vector<std::pair<unsigned int, PlaylistInfo>>;
using PlaylistVectorEntry = std::pair<unsigned int, PlaylistInfo>;

CDiscDirectoryHelper::CDiscDirectoryHelper()
{
  m_playAllPlaylists.clear();
  m_playAllPlaylistsMap.clear();
  m_groups.clear();
  m_candidatePlaylists.clear();
  m_candidateSpecials.clear();
}

void CDiscDirectoryHelper::InitialisePlaylistSearch(
    int episodeIndex, const std::vector<CVideoInfoTag>& episodesOnDisc)
{
  // Need to differentiate between specials and episodes
  m_allEpisodes = episodeIndex == -1 ? AllEpisodes::ALL : AllEpisodes::SINGLE;
  m_isSpecial = m_allEpisodes == AllEpisodes::SINGLE && episodesOnDisc[episodeIndex].m_iSeason == 0
                    ? IsSpecial::SPECIAL
                    : IsSpecial::EPISODE;

  m_numEpisodes = static_cast<unsigned int>(std::ranges::count_if(
      episodesOnDisc, [](const CVideoInfoTag& e) { return e.m_iSeason > 0; }));
  m_numSpecials = static_cast<unsigned int>(episodesOnDisc.size()) - m_numEpisodes;

  // If we are looking for a special then we want to find all episodes - to exclude them
  if (m_isSpecial == IsSpecial::SPECIAL && m_numEpisodes > 0)
    m_allEpisodes = AllEpisodes::ALL;

  CLog::LogF(LOGDEBUG, "*** Episode Search Start ***");

  if (m_allEpisodes == AllEpisodes::SINGLE)
  {
    CLog::LogF(LOGDEBUG, "Looking for season {} episode {} duration {}",
               episodesOnDisc[episodeIndex].m_iSeason, episodesOnDisc[episodeIndex].m_iEpisode,
               episodesOnDisc[episodeIndex].GetDuration());
  }
  else
    CLog::LogF(LOGDEBUG, "Looking for all episodes on disc");

  // List episodes expected on disc
  for (const auto& e : episodesOnDisc)
  {
    CLog::LogF(LOGDEBUG, "Expected on disc - season {} episode {} duration {}", e.m_iSeason,
               e.m_iEpisode, e.GetDuration());
  }
}

bool CDiscDirectoryHelper::IsPotentialPlayAllPlaylist(const PlaylistInfo& playlistInformation) const
{
  return playlistInformation.clips.size() >= m_numEpisodes &&
         playlistInformation.clips.size() <= m_numEpisodes + 2;
}

bool CDiscDirectoryHelper::ClipQualifies(const ClipInfo& clipInformation,
                                         unsigned int clip,
                                         const PlaylistInfo& playlistInformation,
                                         bool& allowBeginningOrEnd,
                                         bool allowBeginningAndEnd)
{
  // If clip doesn't appear in another playlist or clip is too short this is not a Play All playlist (2)(3)
  // BUT we allow first and/or last clip to be shorter or single (ie start intro/end credits) IF there are enough clips
  if (clipInformation.playlists.size() == 1 || clipInformation.duration < MIN_EPISODE_DURATION)
  {
    if ((allowBeginningOrEnd || allowBeginningAndEnd) && clip == playlistInformation.clips.front())
    {
      // First clip
      allowBeginningOrEnd = false; // Cannot have a short clip at end as well
      return true;
    }
    if ((allowBeginningOrEnd || allowBeginningAndEnd) && clip == playlistInformation.clips.back())
    {
      // Last clip
      return true;
    }
    return false;
  }
  return true;
}

bool CDiscDirectoryHelper::IsValidSingleEpisodePlaylist(
    const PlaylistInfo& singleEpisodePlaylistInformation, unsigned int clip) const
{
  // See if potential single episode playlist contains too many clips
  // If there are 3 clips then expect the middle clip to be the main episode clip
  // If there are numEpisodes clips this could be another play all playlist
  return singleEpisodePlaylistInformation.clips.size() < 3 ||
         (singleEpisodePlaylistInformation.clips.size() == 3 &&
          singleEpisodePlaylistInformation.clips[1] == clip) ||
         singleEpisodePlaylistInformation.clips.size() == m_numEpisodes;
}

bool CDiscDirectoryHelper::CheckClip(const PlaylistMap& playlists,
                                     unsigned int playlistNumber,
                                     const ClipInfo& clipInformation,
                                     unsigned int clip,
                                     std::vector<unsigned int>& playAllPlaylistMap) const
{
  for (const auto& singleEpisodePlaylist : clipInformation.playlists)
  {
    // Exclude potential play all playlist we are currently examining
    if (singleEpisodePlaylist == playlistNumber)
      continue;

    const auto& it{playlists.find(singleEpisodePlaylist)};
    if (it == playlists.end())
      return false;

    // Check the playlist could be a single episode
    if (!IsValidSingleEpisodePlaylist(it->second, clip))
      return false;

    playAllPlaylistMap.emplace_back(singleEpisodePlaylist);
  }

  return true;
}

bool CDiscDirectoryHelper::ProcessPlaylistClips(
    const ClipMap& clips,
    const PlaylistMap& playlists,
    unsigned int playlistNumber,
    const PlaylistInfo& playlistInformation,
    std::map<unsigned int, std::vector<unsigned int>>& playAllPlaylistClipMap) const
{
  bool allowBeginningOrEnd{playlistInformation.clips.size() == m_numEpisodes + 1};
  const bool allowBeginningAndEnd{playlistInformation.clips.size() == m_numEpisodes + 2};

  // Loop through each clip in potential play all playlist (numbering between numEpisodes and numEpisodes+2)
  for (unsigned int clip : playlistInformation.clips)
  {
    const auto& it{clips.find(clip)};
    if (it == clips.end())
      return false;

    // See if the clips qualify (ie. small clips (in addition to numEpisode clips) at start or end)
    const ClipInfo& clipInformation{it->second};
    if (!ClipQualifies(clipInformation, clip, playlistInformation, allowBeginningOrEnd,
                       allowBeginningAndEnd))
      return false;

    // See if the playlists associated with the clip are valid as a single episodes
    std::vector<unsigned int> playAllPlaylistMap;
    if (!CheckClip(playlists, playlistNumber, clipInformation, clip, playAllPlaylistMap))
      return false;

    playAllPlaylistClipMap[clip] = playAllPlaylistMap;
  }

  return true;
}

void CDiscDirectoryHelper::StorePlayAllPlaylist(
    unsigned int playlistNumber,
    const PlaylistInfo& playlistInformation,
    const std::map<unsigned int, std::vector<unsigned int>>& playAllPlaylistClipMap)
{
  CLog::LogF(LOGDEBUG, "Potential play all playlist {}", playlistNumber);
  m_playAllPlaylists.insert(
      CandidatePlaylistInformation{.playlist = playlistNumber,
                                   .duration = playlistInformation.duration,
                                   .clips = playlistInformation.clips,
                                   .languages = playlistInformation.languages});
  m_playAllPlaylistsMap[playlistNumber] = playAllPlaylistClipMap;
}

void CDiscDirectoryHelper::FindPlayAllPlaylists(const ClipMap& clips, const PlaylistMap& playlists)
{
  // Look for a potential play all playlist (gives episode order)
  //
  // Assumptions
  //   1) Playlist clip count = number of episodes on disc (+2 for potential separate intro/end credits)
  //   2) Each clip will be in at least one other playlist (the individual episode playlist)
  //   3) Each clip (bar the last) will be at least MIN_EPISODE_LENGTH long
  //   4) Each potential individual episode playlist containing a clip from the potential play all playlist
  //      will have at most one other clip before/after

  // Only look for play all playlists if enough playlists and more than one episode on disc
  if (m_numEpisodes < 2 || playlists.size() < m_numEpisodes)
    return;

  for (const auto& [playlistNumber, playlistInformation] : playlists)
  {
    if (!IsPotentialPlayAllPlaylist(playlistInformation))
      continue;

    std::map<unsigned int, std::vector<unsigned int>> playAllPlaylistClipMap;
    if (ProcessPlaylistClips(clips, playlists, playlistNumber, playlistInformation,
                             playAllPlaylistClipMap))
    {
      StorePlayAllPlaylist(playlistNumber, playlistInformation, playAllPlaylistClipMap);
    }
  }

  if (m_playAllPlaylists.empty())
    CLog::LogF(LOGDEBUG, "No play all playlists found");
}

void CDiscDirectoryHelper::FindGroups(const PlaylistMap& playlists)
{
  // Look for groups of playlists - consecutively numbered playlists where the number of playlists
  //   is at least the number of episodes on disc
  // First generate map of all playlists >= MIN_EPISODE_DURATION and not a play all playlist
  // Then generate array of all consecutive groups of playlists

  // Get all playlists(s) >= MIN_EPISODE_DURATION and not a play all playlist
  PlaylistMap longPlaylists;
  std::ranges::copy_if(playlists, std::inserter(longPlaylists, longPlaylists.end()),
                       [&](const PlaylistMapEntry& p)
                       {
                         const auto& [playlist, playlistInformation] = p;

                         const auto playAllPlaylistNumbers{
                             m_playAllPlaylists |
                             std::views::transform(&CandidatePlaylistInformation::playlist)};

                         return playlistInformation.duration >= MIN_EPISODE_DURATION &&
                                std::ranges::find(playAllPlaylistNumbers, playlist) ==
                                    playAllPlaylistNumbers.end();
                       });

  // Find groups
  if (m_numEpisodes > 1)
  {
    for (const auto& [playlist, playlistInformation] : longPlaylists)
    {
      CandidatePlaylistInformation groupPlaylist{.playlist = playlist,
                                                 .duration = playlistInformation.duration,
                                                 .clips = {},
                                                 .languages = {}};
      if (!m_groups.empty() && m_groups.back().back().playlist == playlist - 1)
        m_groups.back().emplace_back(groupPlaylist);
      else
        m_groups.emplace_back(std::vector{groupPlaylist}); // New group
    }
    m_allGroups = m_groups;

    // See if any groups have at least numEpisode playlists and there are exactly
    // numEpisode playlists remaining and no specials, in which case make a group.
    // Assumption has to be playlists match episodes in ascending order
    std::erase_if(m_groups, [this](const std::vector<CandidatePlaylistInformation>& group)
                  { return group.size() < m_numEpisodes; });
    if (m_groups.empty() && m_numSpecials == 0 && longPlaylists.size() == m_numEpisodes)
    {
      std::ranges::transform(
          longPlaylists, std::back_inserter(m_groups),
          [](const auto& PlaylistInformation) -> std::vector<CandidatePlaylistInformation>
          {
            const auto& [playlist, playlistInformation] = PlaylistInformation;
            return {{.playlist = playlist,
                     .duration = playlistInformation.duration,
                     .clips = {},
                     .languages = {}}};
          });
    }

    if (m_groups.empty())
      CLog::LogF(LOGDEBUG, "No playlist groups found");
    else
      for (const auto& group : m_groups)
        CLog::LogF(LOGDEBUG, "Playlist group found from {} to {}", group.front().playlist,
                   group.back().playlist);
  }
  else
  {
    CLog::LogF(LOGDEBUG, "No group search as single episode or specials only");
  }
}

void CDiscDirectoryHelper::UsePlayAllPlaylistMethod(unsigned int episodeIndex,
                                                    const PlaylistMap& playlists)
{
  CLog::LogF(LOGDEBUG, "Using Play All playlist method");

  // Get the playlist
  const auto& playlistInformation{*m_playAllPlaylists.begin()};
  const unsigned int playAllPlaylist{playlistInformation.playlist};
  CLog::LogF(LOGDEBUG, "Using candidate play all playlist {} duration {}", playAllPlaylist,
             static_cast<int>(playlistInformation.duration.count() / 1000));

  // Find the clip for the episode(s)
  unsigned int i{0};
  for (const auto& clip : playlistInformation.clips)
  {
    if (m_allEpisodes == AllEpisodes::ALL ||
        i == episodeIndex - m_numSpecials) // Specials before episodes in episodesOnDisc
    {
      if (!m_playAllPlaylistsMap[playAllPlaylist].contains(clip))
      {
        CLog::LogF(LOGERROR, "Clip {} missing in play all playlist map", clip);
        return;
      }

      CLog::LogF(LOGDEBUG, "Clip is {}", clip);

      // Find playlist(s) with that clip from map populated earlier
      const auto& singleEpisodePlaylists{m_playAllPlaylistsMap[playAllPlaylist].find(clip)->second};

      for (const auto& singleEpisodePlaylist : singleEpisodePlaylists)
      {
        if (!playlists.contains(singleEpisodePlaylist))
        {
          CLog::LogF(LOGERROR, "Single episode playlist {} missing in playlist map",
                     singleEpisodePlaylist);
          return;
        }
        // Get playlist information
        const PlaylistInfo& singleEpisodePlaylistInformation{
            playlists.find(singleEpisodePlaylist)->second};

        CLog::LogF(LOGDEBUG, "Candidate playlist {} duration {}", singleEpisodePlaylist,
                   static_cast<int>(singleEpisodePlaylistInformation.duration.count() / 1000));

        m_candidatePlaylists.try_emplace(
            singleEpisodePlaylist,
            CandidatePlaylistInformation{.playlist = singleEpisodePlaylist,
                                         .index = i + m_numSpecials,
                                         .duration = singleEpisodePlaylistInformation.duration,
                                         .clips = singleEpisodePlaylistInformation.clips,
                                         .languages = singleEpisodePlaylistInformation.languages});
      }
    }
    ++i;
  }
}

void CDiscDirectoryHelper::UseLongOrCommonMethodForSingleEpisode(unsigned int episodeIndex,
                                                                 const PlaylistMap& playlists)
{
  // Method 2b - Need to think about the special case of only one episode on disc

  // Sort playlists by length
  PlaylistVector playlists_length;
  playlists_length.reserve(playlists.size());
  playlists_length.assign(playlists.begin(), playlists.end());
  std::ranges::sort(playlists_length,
                    [](const PlaylistVectorEntry& i, const PlaylistVectorEntry& j)
                    {
                      const auto& [i_playlist, i_playlistInformation] = i;
                      const auto& [j_playlist, j_playlistInformation] = j;
                      if (i_playlistInformation.duration == j_playlistInformation.duration)
                        return i_playlist < j_playlist;
                      return i_playlistInformation.duration > j_playlistInformation.duration;
                    });

  // Remove duplicate lengths (each episode may have more than one playlist with different languages)
  const auto& [first, last] = std::ranges::unique(
      playlists_length, {}, [](const PlaylistVectorEntry& i) { return i.second.duration; });
  playlists_length.erase(first, last);

  // See how many unique (different length) episode length(>= MIN_EPISODE_LENGTH) playlists there are
  auto episodeLengthPlaylists{
      playlists_length | std::views::filter([](const PlaylistVectorEntry& p)
                                            { return p.second.duration >= MIN_EPISODE_DURATION; })};

  // Look for common starting playlists
  constexpr std::array<unsigned int, 5> commonStartingPlaylists = {801, 800, 1, 811, 0};
  const auto& playlistsView{playlists | std::views::keys};
  const auto commonPlaylist{std::ranges::find_first_of(playlistsView, commonStartingPlaylists)};

  if (std::ranges::distance(episodeLengthPlaylists) == 1)
  {
    // If only one long playlist, then assume it's that
    const auto& [playlist, playlistInformation] = *episodeLengthPlaylists.begin();
    CLog::LogF(LOGDEBUG, "Single Episode - found using single long playlist method");
    CLog::LogF(LOGDEBUG, "Candidate playlist {}", playlist);
    m_candidatePlaylists.try_emplace(
        playlist, CandidatePlaylistInformation{.playlist = playlist,
                                               .index = episodeIndex,
                                               .duration = playlistInformation.duration,
                                               .clips = playlistInformation.clips,
                                               .languages = playlistInformation.languages});
  }
  else if (commonPlaylist != playlistsView.end())
  {
    // Found a common playlist, so assume it's that
    const auto& playlistInformation{playlists.at(*commonPlaylist)};
    CLog::LogF(LOGDEBUG, "Single Episode - found using common playlist method");
    CLog::LogF(LOGDEBUG, "Candidate playlist {}", *commonPlaylist);
    m_candidatePlaylists.try_emplace(
        *commonPlaylist, CandidatePlaylistInformation{.playlist = *commonPlaylist,
                                                      .index = episodeIndex,
                                                      .duration = playlistInformation.duration,
                                                      .clips = playlistInformation.clips,
                                                      .languages = playlistInformation.languages});
  }
}

std::vector<std::vector<CDiscDirectoryHelper::CandidatePlaylistInformation>> CDiscDirectoryHelper::
    GetGroupsWithoutDuplicates(const std::vector<std::vector<CandidatePlaylistInformation>>& groups)
{
  std::vector<std::vector<CandidatePlaylistInformation>> uniqueGroups;
  for (const auto& playlistGroup : groups)
  {
    std::set<int64_t> seenDurations;
    std::ranges::copy_if(playlistGroup, std::back_inserter(uniqueGroups.emplace_back()),
                         CandidatePlaylistInformationNotDuplicate(seenDurations));
  }
  return uniqueGroups;
}

void CDiscDirectoryHelper::GetPlaylistsFromGroup(
    unsigned int episodeIndex, const std::vector<CandidatePlaylistInformation>& group)
{
  for (unsigned int i = 0; i < m_numEpisodes; ++i)
  {
    if (m_allEpisodes != AllEpisodes::ALL &&
        i != episodeIndex - m_numSpecials) // Specials before episodes in episodesOnDisc
      continue;

    m_candidatePlaylists.try_emplace(group[i].playlist,
                                     CandidatePlaylistInformation{.playlist = group[i].playlist,
                                                                  .index = i + m_numSpecials,
                                                                  .duration = group[i].duration,
                                                                  .clips = group[i].clips,
                                                                  .languages = group[i].languages});
    CLog::LogF(LOGDEBUG, "Candidate playlist {}", group[i].playlist);
  }
}

namespace
{
bool CheckDurationsWithinTolerance(std::chrono::milliseconds episodeDuration,
                                   std::chrono::milliseconds playlistDuration)
{
  constexpr int DURATION_TOLERANCE_PERCENT{20};
  const auto tolerance{(episodeDuration * DURATION_TOLERANCE_PERCENT) / 100};
  return episodeDuration > 0ms && std::chrono::abs(playlistDuration - episodeDuration) <= tolerance;
}
} // namespace

void CDiscDirectoryHelper::UseGroupMethod(unsigned int episodeIndex,
                                          const std::vector<CVideoInfoTag>& episodesOnDisc)
{
  // Method 2ai - More than one episode on disc

  // Use groups and find nth playlist (or all for all episodes) in group
  // Groups are already contain at least numEpisodes playlists of minimum duration
  // Firstly look just at groups that contain exactly numEpisodes playlists
  // Having removed duplicates
  CLog::LogF(LOGDEBUG, "Using group method - exact number of playlists");
  const std::vector groups{GetGroupsWithoutDuplicates(m_groups)};
  for (const auto& group : groups)
  {
    if (group.size() != m_numEpisodes)
      continue;

    GetPlaylistsFromGroup(episodeIndex, group);
  }

  if (m_candidatePlaylists.empty())
  {
    // Now look for groups that contain same/more than numEpisodes playlists (with duplicates)
    // Check that the first numEpisodes playlists have a duration within 20% of the desired episode
    CLog::LogF(LOGDEBUG, "Using group method - relaxed number of playlists");
    const std::chrono::milliseconds episodeDuration{episodesOnDisc[episodeIndex].GetDuration() *
                                                    1000ms};
    for (const auto& group : m_groups)
    {
      if (group.size() < m_numEpisodes)
        continue;

      // Check duration
      if (!std::ranges::all_of(group | std::views::take(m_numEpisodes) |
                                   std::views::transform(&CandidatePlaylistInformation::duration),
                               [episodeDuration](const std::chrono::milliseconds playlistDuration) {
                                 return CheckDurationsWithinTolerance(episodeDuration,
                                                                      playlistDuration);
                               }))
        continue;

      GetPlaylistsFromGroup(episodeIndex, group);
    }
  }

  if (m_candidatePlaylists.empty())
    CLog::LogF(LOGDEBUG, "No candidate playlists found");
}

void CDiscDirectoryHelper::UseTotalMethod(unsigned int episodeIndex,
                                          const std::vector<CVideoInfoTag>& episodesOnDisc,
                                          const PlaylistMap& playlists)
{
  // Method 2aii - More than one episode on disc

  // If no groups but the total number of possible playlists equals the number of episodes then consider
  // these as a group
  CLog::LogF(LOGDEBUG, "Using total playlists method");

  // Check that the playlists have a duration within 20% of the desired episode
  // playlists is a map sorted by playlist number so can use index to get nth episode
  // numSpecials has to be zero (checked before calling this method)
  const bool allLengthsMatch{std::ranges::all_of(
      std::views::iota(0u, m_numEpisodes),
      [&](unsigned int i)
      {
        const std::chrono::milliseconds episodeDuration{episodesOnDisc[i].GetDuration() * 1000ms};
        const auto playlistDuration{std::next(playlists.begin(), i)->second.duration};
        return CheckDurationsWithinTolerance(episodeDuration, playlistDuration);
      })};

  if (!allLengthsMatch || episodeIndex >= playlists.size())
  {
    CLog::LogF(LOGDEBUG, "No candidate playlists found");
    return;
  }

  const auto& playlist{std::next(playlists.begin(), episodeIndex)->second};
  m_candidatePlaylists.try_emplace(playlist.playlist,
                                   CandidatePlaylistInformation{.playlist = playlist.playlist,
                                                                .index = episodeIndex,
                                                                .duration = playlist.duration,
                                                                .clips = playlist.clips,
                                                                .languages = playlist.languages});
  CLog::LogF(LOGDEBUG, "Candidate playlist {}", playlist.playlist);
}

int CDiscDirectoryHelper::CalculateMultiple(std::chrono::milliseconds duration,
                                            std::chrono::milliseconds averageShortest,
                                            double multiplePercent)
{
  double multiple{static_cast<double>(duration.count()) /
                  static_cast<double>(averageShortest.count())};
  int integerMultiple{static_cast<int>(std::round(multiple))};
  if (abs(multiple - integerMultiple) < multiplePercent / 100.0)
    return integerMultiple;
  return 0;
}

void CDiscDirectoryHelper::UseGroupsWithMultiplesMethod(
    unsigned int episodeIndex, const std::vector<CVideoInfoTag>& episodesOnDisc)
{
  // No groups of numEpisodes length so see if there could be double episode playlists
  // Assume more than one playlist
  CLog::LogF(LOGDEBUG, "Using groups with multiples method");
  for (auto& group : GetGroupsWithoutDuplicates(m_allGroups))
  {
    // Calculate multiples
    // Find shortest playlist in group
    const std::chrono::milliseconds shortest = std::ranges::min(
        group | std::views::transform(&CandidatePlaylistInformation::duration), {});

    // Then calculate the average of shortest (within 20% of the shortest) playlists
    constexpr double SHORTEST_PERCENT{20.0};
    auto groupDurations{
        group | std::views::transform(&CandidatePlaylistInformation::duration) |
        std::views::filter([shortest](const std::chrono::milliseconds i)
                           { return i < shortest * (1 + (SHORTEST_PERCENT / 100.0)); })};
    const std::chrono::milliseconds averageShortest{
        std::accumulate(groupDurations.begin(), groupDurations.end(), 0ms) /
        std::ranges::distance(groupDurations)};

    // Multiples of average shortest playlists (within 15% of the average)
    constexpr double MULTIPLE_PERCENT{15.0};
    for (auto& playlist : group)
      playlist.multiple = CalculateMultiple(playlist.duration, averageShortest, MULTIPLE_PERCENT);

    // Check there are no playlists that are not a multiple
    if (std::ranges::any_of(group,
                            [](const CandidatePlaylistInformation& i) { return i.multiple == 0; }))
      continue;

    // Check that multiples add up to numEpisodes
    auto groupMultiples{group | std::views::transform(&CandidatePlaylistInformation::multiple)};
    if (std::accumulate(groupMultiples.begin(), groupMultiples.end(), 0) !=
        static_cast<int>(m_numEpisodes))
      continue;

    // Save candidate episode(s)
    unsigned int index{
        m_numSpecials}; // Start at numSpecials as specials (S00) are before episodes in episodesOnDisc
    for (const auto& playlist : group)
    {
      auto playlistInformation{playlist};
      for (int i = 0; i < playlist.multiple; ++i)
      {
        if (m_allEpisodes == AllEpisodes::ALL || index == episodeIndex - m_numSpecials)
        {
          playlistInformation.index = index;
          m_candidatePlaylists.try_emplace(playlist.playlist, playlistInformation);
          CLog::LogF(LOGDEBUG, "Candidate playlist {} for episode {}", playlist.playlist,
                     episodesOnDisc[index].m_iEpisode);
        }
        ++index;
      }
    }
    break;
  }

  if (m_candidatePlaylists.empty())
    CLog::LogF(LOGDEBUG, "No candidate playlists found");
}

void CDiscDirectoryHelper::ChooseSingleBestPlaylist(
    const std::vector<CVideoInfoTag>& episodesOnDisc)
{
  // Rebuild candidatePlaylists
  auto oldCandidatePlaylists{std::move(m_candidatePlaylists)};
  m_candidatePlaylists.clear();

  // Loop through each episode (in case of all episodes)
  // Generate set of indexes of episode entry in episodesOnDisc
  const auto indexView{oldCandidatePlaylists | std::views::values |
                       std::views::transform(&CandidatePlaylistInformation::index)};
  std::set<unsigned int> indexes(indexView.begin(), indexView.end());

  // Loop through each index (episode) and find the playlist with the closest duration
  for (unsigned int currentEpisodeIndex : indexes)
  {
    std::chrono::milliseconds duration{episodesOnDisc[currentEpisodeIndex].GetDuration() * 1000};

    // If episode length not known, ensure the longest playlist selected
    if (duration == 0ms)
      duration = std::chrono::milliseconds::max();

    auto filter{oldCandidatePlaylists |
                std::views::filter([currentEpisodeIndex](const auto& p)
                                   { return p.second.index == currentEpisodeIndex; }) |
                std::views::values};
    auto filteredCandidatePlaylists{std::vector(filter.begin(), filter.end())};

    for (auto& playlistInformation : filteredCandidatePlaylists)
      playlistInformation.durationDelta = abs(playlistInformation.duration - duration);

    // Sort descending based on number of chapters and ascending by relative difference in duration
    std::ranges::sort(
        filteredCandidatePlaylists,
        [](const CandidatePlaylistInformation& p, const CandidatePlaylistInformation& q)
        {
          if (p.chapters == q.chapters)
            return p.durationDelta < q.durationDelta;
          return p.chapters > q.chapters;
        });

    // Keep the playlist with most chapters and closest duration
    const unsigned int playlist{filteredCandidatePlaylists[0].playlist};
    m_candidatePlaylists.try_emplace(playlist, filteredCandidatePlaylists[0]);

    CLog::LogF(LOGDEBUG,
               "Remaining candidate playlist (closest in duration) is {} for episode index {}",
               playlist, currentEpisodeIndex);
  }
}

void CDiscDirectoryHelper::AddIdenticalPlaylists(const PlaylistMap& playlists)
{
  for (const auto& [candidatePlaylist, candidatePlaylistInformation] : m_candidatePlaylists)
  {
    // Find all other playlists of same duration with same clips
    for (const auto& [playlist, playlistInformation] : playlists)
    {
      if (playlist != candidatePlaylist &&
          candidatePlaylistInformation.duration == playlistInformation.duration &&
          candidatePlaylistInformation.languages != playlistInformation.languages &&
          candidatePlaylistInformation.clips == playlistInformation.clips)
      {
        CLog::LogF(LOGDEBUG, "Adding playlist {} as same duration and clips as playlist {}",
                   playlist, candidatePlaylist);
        m_candidatePlaylists.try_emplace(playlist, candidatePlaylistInformation);
      }
    }
  }
}

void CDiscDirectoryHelper::FindCandidatePlaylists(const std::vector<CVideoInfoTag>& episodesOnDisc,
                                                  unsigned int episodeIndex,
                                                  const PlaylistMap& playlists)
{
  // At this stage we have a number of ways of trying to determine the correct playlist for an episode
  //
  // 1) Using a 'Play All' playlist, if present.
  //
  // 2ai) Using the longest playlists that are consecutive, using the playlist in the nth position.
  //      Taking into account that there may be multiple episodes in a single playlist.
  //
  // 2aii) If there are no consecutive playlist groups but the number of possible playlists equals the number of episodes
  //       then consider that a group even if not consecutive.
  //
  // 2b) For single episode discs, look for the longest playlist.
  //     There are some discs where there are extras that are longer than the episode itself, in this case
  //     we look for the playlist with a common starting number (eg. 1, 800 etc..)

  if (m_allEpisodes == AllEpisodes::ALL)
    CLog::LogF(LOGDEBUG, "Looking for all episodes on disc");

  if (m_playAllPlaylists.size() == 1)
    UsePlayAllPlaylistMethod(episodeIndex, playlists);
  else if (m_numEpisodes == 1)
    UseLongOrCommonMethodForSingleEpisode(episodeIndex, playlists);
  else if (!m_groups.empty())
    UseGroupMethod(episodeIndex, episodesOnDisc);

  if (m_candidatePlaylists.empty() && !m_allGroups.empty() && m_numEpisodes > 1)
    UseGroupsWithMultiplesMethod(episodeIndex, episodesOnDisc);

  if (m_candidatePlaylists.empty() && m_numEpisodes > 1 && m_numSpecials == 0 &&
      m_numEpisodes == playlists.size())
    UseTotalMethod(episodeIndex, episodesOnDisc, playlists);

  // Now deal with the possibility there may be more than one playlist (per episode)
  // For this, see which is closest in duration to the desired episode duration (from the scraper)
  // If we are looking for a special then leave all potential episode playlists in array
  if (m_candidatePlaylists.size() > 1 && m_isSpecial == IsSpecial::EPISODE)
    ChooseSingleBestPlaylist(episodesOnDisc);

  // candidatePlaylists should now (ideally) contain one candidate title for the episode or all episodes
  // Now look at durations of found playlist and add identical (in case language options)
  if (!m_candidatePlaylists.empty())
    AddIdenticalPlaylists(playlists);
}

void CDiscDirectoryHelper::FindSpecials(const PlaylistMap& playlists)
{
  if (m_numSpecials == 0)
    return;

  // Specials
  //
  // These are more difficult - as there may only be one per disc and we can't make assumptions about playlists.
  // So have to look on basis of duration alone.

  // Remove episodes and short playlists
  PlaylistVector playlistsLength;
  playlistsLength.reserve(playlists.size());
  playlistsLength.assign(playlists.begin(), playlists.end());
  if (m_numEpisodes > 0)
  {
    std::erase_if(
        playlistsLength,
        [this](const PlaylistVectorEntry& playlist)
        {
          const auto& [playlistNumber, playlistInformation] = playlist;

          const bool isShort{playlistInformation.duration < MIN_SPECIAL_DURATION};

          const auto candidatePlaylistNumbers{m_candidatePlaylists | std::views::keys};
          const bool isEpisode{std::ranges::find(candidatePlaylistNumbers, playlistNumber) !=
                               candidatePlaylistNumbers.end()};

          const auto playAllPlaylistNumbers{
              m_playAllPlaylists | std::views::transform(&CandidatePlaylistInformation::playlist)};
          const bool isPlayAll{std::ranges::find(playAllPlaylistNumbers, playlistNumber) !=
                               playAllPlaylistNumbers.end()};

          return isShort || isEpisode || isPlayAll;
        });
  }

  // Sort playlists by length
  std::ranges::sort(playlistsLength,
                    [](const PlaylistVectorEntry& i, const PlaylistVectorEntry& j)
                    {
                      const auto& [i_playlist, i_playlistInformation] = i;
                      const auto& [j_playlist, j_playlistInformation] = j;
                      if (i_playlistInformation.duration == j_playlistInformation.duration)
                        return i_playlist < j_playlist;
                      return i_playlistInformation.duration > j_playlistInformation.duration;
                    });

  // Take the longest remaining playlists as specials
  // If more than one candidate, we don't know which the special is, so include them all
  if (playlistsLength.size() >= m_numSpecials)
  {
    for (unsigned int playlist : playlistsLength | std::views::keys)
      m_candidateSpecials.emplace(playlist);
  }
}

void CDiscDirectoryHelper::GenerateItem(const CURL& url,
                                        const std::shared_ptr<CFileItem>& item,
                                        unsigned int playlist,
                                        const PlaylistMap& playlists,
                                        const CVideoInfoTag& tag,
                                        IsSpecial isSpecial)
{
  if (!playlists.contains(playlist))
  {
    CLog::LogF(LOGERROR, "Playlist {} missing in playlist map", playlist);
    return;
  }

  // Get clips
  const auto it{playlists.find(playlist)};
  const std::chrono::milliseconds duration{it->second.duration};

  // Get languages
  const std::string langs{it->second.languages};

  CURL path{url};
  std::string buf{StringUtils::Format("BDMV/PLAYLIST/{:05}.mpls", playlist)};
  path.SetFileName(buf);
  item->SetPath(path.Get());

  CVideoInfoTag* itemTag{item->GetVideoInfoTag()};
  itemTag->SetDuration(static_cast<int>(duration.count()));
  item->SetProperty("bluray_playlist", playlist);

  // Get episode title
  const std::string& title{tag.GetTitle()};
  if (isSpecial == IsSpecial::SPECIAL)
  {
    if (title.empty())
      buf = g_localizeStrings.Get(21350); /* Special */
    else
      /* Special xx - title */
      buf = StringUtils::Format(g_localizeStrings.Get(21348), tag.m_iEpisode, tag.GetTitle());
  }
  else
    /* Episode xx - title */
    buf = StringUtils::Format(g_localizeStrings.Get(21349), tag.m_iEpisode, tag.GetTitle());
  item->SetTitle(buf);
  item->SetLabel(buf);

  item->SetLabel2(StringUtils::Format(
      g_localizeStrings.Get(25005) /* Title: {0:d} */ + " - {1:s}: {2:s}\n\r{3:s}: {4:s}", playlist,
      g_localizeStrings.Get(180) /* Duration */,
      StringUtils::SecondsToTimeString(static_cast<int>(duration.count() / 1000)),
      g_localizeStrings.Get(24026) /* Languages */, langs));
  item->SetSize(0);
  item->SetArt("icon", "DefaultVideo.png");
}

void CDiscDirectoryHelper::EndPlaylistSearch() const
{
  CLog::LogF(LOGDEBUG, "*** Episode Search End ***");
}

void CDiscDirectoryHelper::PopulateFileItems(const CURL& url,
                                             CFileItemList& items,
                                             int episodeIndex,
                                             const std::vector<CVideoInfoTag>& episodesOnDisc,
                                             const PlaylistMap& playlists) const
{
  // Now populate CFileItemList to return
  //
  // For singe episodes - relevant playlists are in candidatePlaylists. Show then all
  // For all episodes   - relevant playlists are in candidatePlaylists and specials in candidateSpecials
  //                    - show all episodes and specials (with latter entitled 'Special')
  // For single special - relevant playlists are in candidateSpecials.
  //                    - if there is a single playlist then label that with special's title
  //                      otherwise label all playlists as 'Special' as cannot determine which it is

  items.Clear();
  if (m_isSpecial == IsSpecial::EPISODE)
  {
    // Sort by index, number of languages and playlist
    auto filter = m_candidatePlaylists | std::views::values;
    auto sortedPlaylists = std::vector(filter.begin(), filter.end());

    std::ranges::sort(
        sortedPlaylists,
        [](const CandidatePlaylistInformation& i, const CandidatePlaylistInformation& j)
        {
          if (i.index == j.index)
          {
            if (i.languages.size() == j.languages.size())
              return i.playlist < j.playlist;
            return i.languages.size() > j.languages.size();
          }
          return i.index < j.index;
        });

    for (const auto& playlist : sortedPlaylists)
    {
      const auto newItem{std::make_shared<CFileItem>("", false)};
      GenerateItem(url, newItem, playlist.playlist, playlists, episodesOnDisc[playlist.index],
                   IsSpecial::EPISODE);
      items.Add(newItem);
    }
  }

  if (m_isSpecial == IsSpecial::SPECIAL || m_allEpisodes == AllEpisodes::ALL)
  {
    for (const auto& playlist : m_candidateSpecials)
    {
      const auto newItem{std::make_shared<CFileItem>("", false)};
      CVideoInfoTag tag;
      if (m_isSpecial == IsSpecial::SPECIAL && m_candidateSpecials.size() == 1)
        tag = episodesOnDisc[episodeIndex];
      GenerateItem(url, newItem, playlist, playlists, tag, IsSpecial::SPECIAL);
      items.Add(newItem);
    }
  }
}

bool CDiscDirectoryHelper::GetEpisodePlaylists(const CURL& url,
                                               CFileItemList& items,
                                               int episodeIndex,
                                               const std::vector<CVideoInfoTag>& episodesOnDisc,
                                               const ClipMap& clips,
                                               const PlaylistMap& playlists)
{
  InitialisePlaylistSearch(episodeIndex, episodesOnDisc);
  FindPlayAllPlaylists(clips, playlists);
  FindGroups(playlists);
  FindCandidatePlaylists(episodesOnDisc, episodeIndex, playlists);
  FindSpecials(playlists);
  EndPlaylistSearch();
  PopulateFileItems(url, items, episodeIndex, episodesOnDisc, playlists);

  return !items.IsEmpty();
}

void CDiscDirectoryHelper::AddRootOptions(const CURL& url,
                                          CFileItemList& items,
                                          AddMenuOption addMenuOption)
{
  CURL path{url};
  path.SetFileName(URIUtils::AddFileToFolder("root", "titles"));

  auto item{std::make_shared<CFileItem>(path.Get(), true)};
  item->SetLabel(g_localizeStrings.Get(25002) /* All titles */);
  item->SetArt("icon", "DefaultVideoPlaylists.png");
  items.Add(item);

  if (addMenuOption == AddMenuOption::ADD_MENU)
  {
    path.SetFileName("menu");
    item = {std::make_shared<CFileItem>(path.Get(), false)};
    item->SetLabel(g_localizeStrings.Get(25003) /* Menu */);
    item->SetArt("icon", "DefaultProgram.png");
    items.Add(item);
  }
}

std::vector<CVideoInfoTag> CDiscDirectoryHelper::GetEpisodesOnDisc(const CURL& url)
{
  CVideoDatabase database;
  if (!database.Open())
  {
    CLog::LogF(LOGERROR, "Failed to open video database");
    return {};
  }
  std::vector<CVideoInfoTag> episodesOnDisc;
  database.GetEpisodesByBlurayPath(url.Get(), episodesOnDisc);
  std::ranges::sort(episodesOnDisc,
                    [](const CVideoInfoTag& i, const CVideoInfoTag& j)
                    {
                      if (i.m_iSeason == j.m_iSeason)
                        return i.m_iEpisode < j.m_iEpisode;
                      return i.m_iSeason < j.m_iSeason;
                    });
  return episodesOnDisc;
}
