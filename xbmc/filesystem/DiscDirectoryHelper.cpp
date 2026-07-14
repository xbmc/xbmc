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
#include "ServiceBroker.h"
#include "URL.h"
#include "dialogs/GUIDialogBusy.h"
#include "dialogs/GUIDialogOK.h"
#include "dialogs/GUIDialogSimpleMenu.h"
#include "resources/LocalizeStrings.h"
#include "resources/ResourcesComponent.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "threads/IRunnable.h"
#include "utils/RegExp.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"
#include "video/VideoDatabase.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <iterator>
#include <map>
#include <memory>
#include <numeric>
#include <ranges>
#include <set>
#include <string>
#include <utility>
#include <vector>

using namespace XFILE;
using namespace std::chrono_literals;

using PlaylistMapEntry = std::pair<unsigned int, PlaylistInformation>;
using PlaylistVector = std::vector<std::pair<unsigned int, PlaylistInformation>>;
using PlaylistVectorEntry = std::pair<unsigned int, PlaylistInformation>;

namespace
{
class CGetDirectoryItems : public IRunnable
{
public:
  CGetDirectoryItems(std::string path, CFileItemList& items, CDirectory::CHints hints)
    : m_path(std::move(path)),
      m_items(&items),
      m_hints(std::move(hints))
  {
  }
  void Run() override { m_result = CDirectory::GetDirectory(m_path, *m_items, m_hints); }
  bool m_result{false};

private:
  std::string m_path;
  CFileItemList* m_items;
  CDirectory::CHints m_hints;
};
} // namespace

CDiscDirectoryHelper::CDiscDirectoryHelper()
{
  m_playAllPlaylists.clear();
  m_playAllPlaylistsMap.clear();
  m_groups.clear();
  m_candidatePlaylists.clear();
  m_candidateSpecials.clear();
  m_nthLongestPlaylists.clear();

  m_minEpisodeDuration = std::chrono::milliseconds(CServiceBroker::GetSettingsComponent()
                                                       ->GetAdvancedSettings()
                                                       ->m_minimumEpisodePlaylistDuration *
                                                   1000);
}

void CDiscDirectoryHelper::InitialiseEpisodePlaylistSearch(int episodeIndex,
                                                           const Episodes& episodesOnDisc)
{
  // Need to differentiate between specials and episodes
  m_allEpisodes = episodeIndex == -1 ? AllEpisodes::ALL : AllEpisodes::SINGLE;
  m_isSpecial = m_allEpisodes == AllEpisodes::SINGLE && episodesOnDisc[episodeIndex].iSeason == 0
                    ? IsSpecial::SPECIAL
                    : IsSpecial::EPISODE;

  m_numEpisodes = static_cast<unsigned int>(
      std::ranges::count_if(episodesOnDisc, [](const Episode& e) { return e.iSeason > 0; }));
  m_numSpecials = static_cast<unsigned int>(episodesOnDisc.size()) - m_numEpisodes;

  // If we are looking for a special then we want to find all episodes - to exclude them
  if (m_isSpecial == IsSpecial::SPECIAL && m_numEpisodes > 0)
    m_allEpisodes = AllEpisodes::ALL;

  CLog::LogFC(LOGDEBUG, LOGBLURAY, "*** Episode Search Start ***");

  if (m_allEpisodes == AllEpisodes::SINGLE)
  {
    CLog::LogFC(LOGDEBUG, LOGBLURAY, "Looking for season {} episode {} duration {}",
                episodesOnDisc[episodeIndex].iSeason, episodesOnDisc[episodeIndex].iEpisode,
                episodesOnDisc[episodeIndex].duration);
  }
  else
    CLog::LogFC(LOGDEBUG, LOGBLURAY, "Looking for all episodes on disc");

  // List episodes expected on disc
  for (const auto& e : episodesOnDisc)
  {
    CLog::LogFC(LOGDEBUG, LOGBLURAY, "Expected on disc - season {} episode {} duration {}",
                e.iSeason, e.iEpisode, e.duration);
  }
}

namespace
{
bool IsPotentialPlayAllPlaylist(const PlaylistInformation& playlistInformation,
                                unsigned int numEpisodes)
{
  return playlistInformation.clips.size() >= numEpisodes &&
         playlistInformation.clips.size() <= numEpisodes + 2;
}

bool ClipQualifies(const ClipInfo& clipInformation,
                   unsigned int clip,
                   const PlaylistInformation& playlistInformation,
                   unsigned int& playAllPlaylistEpisodesStartOffset,
                   bool& allowBeginningOrEnd,
                   bool allowBeginningAndEnd,
                   std::chrono::milliseconds minEpisodeDuration)
{
  // If clip doesn't appear in another playlist (ie. clip should appear in both the play all playlist and the individual episode)
  // or clip is too short this is not a Play All playlist
  // BUT we allow first and/or last clip to be shorter or single (ie start intro/end credits)
  const bool isShort{clipInformation.duration < minEpisodeDuration};
  const bool inSinglePlaylist{clipInformation.playlists.size() == 1};
  if (!isShort && !inSinglePlaylist)
    return true;

  const bool canBeAtBeginningOrEnd{allowBeginningOrEnd || allowBeginningAndEnd};
  if (isShort && canBeAtBeginningOrEnd)
  {
    if (clip == playlistInformation.clips.front())
    {
      playAllPlaylistEpisodesStartOffset = 1;
      allowBeginningOrEnd =
          false; // If allowBeginningOrEnd true and short clip found at beginning, cannot have one at end
      return true;
    }

    if (clip == playlistInformation.clips.back())
      return true;
  }

  return false;
}

bool IsValidSingleEpisodePlaylist(const PlaylistInformation& singleEpisodePlaylistInformation,
                                  unsigned int clip,
                                  const ClipMap& clips,
                                  std::chrono::milliseconds minEpisodeDuration)
{
  // See if potential single episode playlist contains too many clips
  // If there are 3 clips then expect the middle clip to be the main episode clip
  // If there are numEpisodes clips this could be another play all playlist
  if (singleEpisodePlaylistInformation.clips.size() == 1)
    return singleEpisodePlaylistInformation.clips[0] == clip;

  if (singleEpisodePlaylistInformation.clips.size() == 2)
  {
    if (singleEpisodePlaylistInformation.clips[0] == clip)
      return clips.at(singleEpisodePlaylistInformation.clips[1]).duration <
             minEpisodeDuration; // Allow short clip at end
    if (singleEpisodePlaylistInformation.clips[1] == clip)
      return clips.at(singleEpisodePlaylistInformation.clips[0]).duration <
             minEpisodeDuration; // Allow short clip at start
    return false;
  }

  if (singleEpisodePlaylistInformation.clips.size() == 3)
  {
    if (singleEpisodePlaylistInformation.clips[1] != clip)
      return false; // Must be in the middle between short intro and ending clips
    return clips.at(singleEpisodePlaylistInformation.clips[0]).duration < minEpisodeDuration &&
           clips.at(singleEpisodePlaylistInformation.clips[2]).duration < minEpisodeDuration;
  }

  return false;
}

bool CheckClip(const PlaylistMap& playlists,
               const ClipMap& clips,
               unsigned int playlistNumber,
               const ClipInfo& clipInformation,
               unsigned int clip,
               std::chrono::milliseconds minEpisodeDuration,
               std::vector<unsigned int>& playAllPlaylistMap)
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
    if (!IsValidSingleEpisodePlaylist(it->second, clip, clips, minEpisodeDuration))
      return false;

    playAllPlaylistMap.emplace_back(singleEpisodePlaylist);
  }

  return true;
}

bool ProcessPlaylistClips(const ClipMap& clips,
                          const PlaylistMap& playlists,
                          unsigned int numEpisodes,
                          unsigned int playlistNumber,
                          unsigned int& playAllPlaylistEpisodesStartOffset,
                          std::chrono::milliseconds minEpisodeDuration,
                          const PlaylistInformation& playlistInformation,
                          std::map<unsigned int, std::vector<unsigned int>>& playAllPlaylistClipMap)
{
  bool allowBeginningOrEnd{playlistInformation.clips.size() == numEpisodes + 1};
  const bool allowBeginningAndEnd{playlistInformation.clips.size() == numEpisodes + 2};

  // Loop through each clip in potential play all playlist (numbering between numEpisodes and numEpisodes+2)
  for (unsigned int clip : playlistInformation.clips)
  {
    const auto& it{clips.find(clip)};
    if (it == clips.end())
      return false;

    // See if the clips qualify (ie. small clips (in addition to numEpisode clips) at start or end)
    const ClipInfo& clipInformation{it->second};
    if (!ClipQualifies(clipInformation, clip, playlistInformation,
                       playAllPlaylistEpisodesStartOffset, allowBeginningOrEnd,
                       allowBeginningAndEnd, minEpisodeDuration))
      return false;

    // See if the playlists associated with the clip are valid as a single episodes
    std::vector<unsigned int> playAllPlaylistMap;
    if (!CheckClip(playlists, clips, playlistNumber, clipInformation, clip, minEpisodeDuration,
                   playAllPlaylistMap))
      return false;

    playAllPlaylistClipMap[clip] = std::move(playAllPlaylistMap);
  }

  return true;
}

bool CheckDurationsWithinTolerance(std::chrono::milliseconds episodeDuration,
                                   std::chrono::milliseconds playlistDuration,
                                   int durationTolerancePercent = DURATION_TOLERANCE_PERCENT)
{
  const auto tolerance{(episodeDuration * durationTolerancePercent) / 100};
  return episodeDuration > 0ms && std::chrono::abs(playlistDuration - episodeDuration) <= tolerance;
}

std::chrono::milliseconds GetAverageEpisodeDuration(const Episodes& episodesOnDisc)
{
  auto nonZeroEpisodes{episodesOnDisc |
                       std::views::filter([](const Episode& e) { return e.duration > 0; })};
  if (nonZeroEpisodes.empty())
    return 0ms;

  const double mean{std::accumulate(nonZeroEpisodes.begin(), nonZeroEpisodes.end(), 0.0,
                                    [](double s, const Episode& e) { return s + e.duration; }) /
                    static_cast<double>(std::ranges::distance(nonZeroEpisodes))};

  const auto [sum, count]{std::accumulate(
      nonZeroEpisodes.begin(), nonZeroEpisodes.end(), std::pair{0.0, 0},
      [=](auto acc, const Episode& e)
      {
        return std::abs(e.duration - mean) <=
                       mean * static_cast<double>(DURATION_TOLERANCE_PERCENT) / 100.0
                   ? std::pair{acc.first + e.duration, acc.second + 1}
                   : acc;
      })};

  return std::chrono::milliseconds(count ? static_cast<long long>(sum / count) : 0) * 1000;
}
} // namespace

void CDiscDirectoryHelper::StorePlayAllPlaylist(
    unsigned int playlistNumber,
    unsigned int playAllPlaylistEpisodesStartOffset,
    const PlaylistInformation& playlistInformation,
    const std::map<unsigned int, std::vector<unsigned int>>& playAllPlaylistClipMap)
{
  CLog::LogFC(LOGDEBUG, LOGBLURAY, "Potential play all playlist {}", playlistNumber);
  m_playAllPlaylists.insert(CandidatePlaylistInformation{
      .playlist = playlistNumber,
      .playAllPlaylistEpisodesStartOffset = playAllPlaylistEpisodesStartOffset,
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
  //   3) Each clip (bar the last) will be at least MIN_EPISODE_DURATION long
  //   4) Each potential individual episode playlist containing a clip from the potential play all playlist
  //      will have at most one other clip before/after

  // Only look for play all playlists if enough playlists and more than one episode on disc
  if (m_numEpisodes < 2 || playlists.size() < m_numEpisodes)
    return;

  for (const auto& [playlistNumber, playlistInformation] : playlists)
  {
    if (!IsPotentialPlayAllPlaylist(playlistInformation, m_numEpisodes))
      continue;

    std::map<unsigned int, std::vector<unsigned int>> playAllPlaylistClipMap;
    unsigned int playAllPlaylistEpisodesStartOffset{0};
    if (ProcessPlaylistClips(clips, playlists, m_numEpisodes, playlistNumber,
                             playAllPlaylistEpisodesStartOffset, m_minEpisodeDuration,
                             playlistInformation, playAllPlaylistClipMap))
    {
      StorePlayAllPlaylist(playlistNumber, playAllPlaylistEpisodesStartOffset, playlistInformation,
                           playAllPlaylistClipMap);
    }
  }

  if (m_playAllPlaylists.empty())
    CLog::LogFC(LOGDEBUG, LOGBLURAY, "No play all playlists found");
}

void CDiscDirectoryHelper::FindGroups(const PlaylistMap& playlists, const Episodes& episodesOnDisc)
{
  // Look for groups of playlists - consecutively numbered playlists where the number of playlists
  //   is at least the number of episodes on disc
  // First generate map of all playlists >= minEpisodeDuration and not a play all playlist
  // Then generate array of all consecutive groups of playlists

  if (m_numEpisodes < 2)
  {
    CLog::LogFC(LOGDEBUG, LOGBLURAY, "No group search as single episode or specials only");
    return;
  }

  // Get all playlists(s) >= minEpisodeDuration and not a play all playlist
  PlaylistMap longPlaylists;
  std::ranges::copy_if(
      playlists, std::inserter(longPlaylists, longPlaylists.end()),
      [&](const PlaylistMapEntry& p)
      {
        const auto& [playlist, playlistInformation] = p;

        const auto playAllPlaylistNumbers{
            m_playAllPlaylists | std::views::transform(&CandidatePlaylistInformation::playlist)};

        return playlistInformation.duration >= m_minEpisodeDuration &&
               std::ranges::find(playAllPlaylistNumbers, playlist) == playAllPlaylistNumbers.end();
      });

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

  // Remove any groups that have less than m_numEpisodes playlists
  std::erase_if(m_groups, [this](const std::vector<CandidatePlaylistInformation>& group)
                { return group.size() < m_numEpisodes; });

  // See if there are exactly numEpisode playlists and no specials, in which case make a group.
  // Assumption has to be playlists match episodes in ascending order
  if (m_groups.empty() && m_numSpecials == 0)
  {
    CLog::LogFC(LOGDEBUG, LOGBLURAY, "Looking exact number of non-consecutive playlists");
    // Remove playlists whose durations are not within 20% of episodes' average
    const auto episodeDuration{GetAverageEpisodeDuration(episodesOnDisc)};
    if (episodeDuration > 0ms)
    {
      std::erase_if(longPlaylists, [episodeDuration](const PlaylistMapEntry& p)
                    { return !CheckDurationsWithinTolerance(episodeDuration, p.second.duration); });
    }

    // See if exact number remaining
    if (longPlaylists.size() == m_numEpisodes)
    {
      std::vector<CandidatePlaylistInformation> group;
      std::ranges::transform(longPlaylists, std::back_inserter(group),
                             [](const auto& PlaylistInformation) -> CandidatePlaylistInformation
                             {
                               const auto& [playlist, playlistInformation] = PlaylistInformation;
                               return {.playlist = playlist,
                                       .duration = playlistInformation.duration,
                                       .clips = {},
                                       .languages = {}};
                             });
      m_groups.emplace_back(std::move(group));
    }
  }

  // Make a group of the numEpisodes longest playlists (sorted by playlist)
  // This assumes the episodes occur in ascending playlist order
  // Exclude any playlists that might be play all playlists
  // Also exclude duplicate durations (added back later if needed)
  // Don't add to m_groups yet as it is only to be used if no other groups are found
  if (playlists.size() >= m_numEpisodes)
  {
    std::set<int64_t> seenDurations;
    std::vector<CandidatePlaylistInformation> candidates;
    candidates.reserve(playlists.size());
    std::ranges::copy_if(std::views::values(playlists) |
                             std::views::transform(
                                 [](const PlaylistInformation& p)
                                 {
                                   return CandidatePlaylistInformation{.playlist = p.playlist,
                                                                       .duration = p.duration,
                                                                       .clips = p.clips,
                                                                       .languages = p.languages};
                                 }) |
                             std::views::filter([this](const CandidatePlaylistInformation& cpi)
                                                { return !m_playAllPlaylists.contains(cpi); }),
                         std::back_inserter(candidates),
                         [&seenDurations](const CandidatePlaylistInformation& c) noexcept
                         { return seenDurations.insert(c.duration.count()).second; });

    m_nthLongestPlaylists.clear();
    m_nthLongestPlaylists.resize(m_numEpisodes);
    const auto sortResult = std::ranges::partial_sort_copy(
        candidates, m_nthLongestPlaylists,
        [](const CandidatePlaylistInformation& a, const CandidatePlaylistInformation& b)
        { return a.duration > b.duration; });

    // Fewer than m_numEpisodes playlists remain after excluding play-all/duplicate playlists
    if (sortResult.out != m_nthLongestPlaylists.end())
      m_nthLongestPlaylists.clear();
    else
      std::ranges::sort(m_nthLongestPlaylists, [&](const CandidatePlaylistInformation& a,
                                                   const CandidatePlaylistInformation& b)
                        { return a.playlist < b.playlist; });
  }

  if (m_groups.empty())
    CLog::LogFC(LOGDEBUG, LOGBLURAY, "No playlist groups found");
  else
    for (const auto& group : m_groups)
      CLog::LogFC(LOGDEBUG, LOGBLURAY, "Playlist group found from {} to {}", group.front().playlist,
                  group.back().playlist);
}

void CDiscDirectoryHelper::UsePlayAllPlaylistMethod(int episodeIndex, const PlaylistMap& playlists)
{
  CLog::LogFC(LOGDEBUG, LOGBLURAY, "Using Play All playlist method");

  // Get the playlist
  const auto& playlistInformation{*m_playAllPlaylists.begin()};
  const unsigned int playAllPlaylist{playlistInformation.playlist};
  CLog::LogFC(LOGDEBUG, LOGBLURAY, "Using candidate play all playlist {} duration {}",
              playAllPlaylist, static_cast<int>(playlistInformation.duration.count() / 1000));

  // Find the clip for the episode(s)
  const int episodeOffset{
      episodeIndex - static_cast<int>(m_numSpecials) + // Specials before episodes in episodesOnDisc
      static_cast<int>(
          playlistInformation
              .playAllPlaylistEpisodesStartOffset)}; // Adjust if a short clip at start of play-all playlist
  unsigned int i{0};
  for (const auto& clip : playlistInformation.clips)
  {
    if (m_allEpisodes == AllEpisodes::ALL || std::cmp_equal(i, episodeOffset))
    {
      const auto& it{m_playAllPlaylistsMap.find(playAllPlaylist)};
      if (it == m_playAllPlaylistsMap.end() || !it->second.contains(clip))
      {
        CLog::LogF(LOGERROR, "Clip {} missing in play all playlist map", clip);
        return;
      }

      CLog::LogFC(LOGDEBUG, LOGBLURAY, "Clip is {}", clip);

      // Find playlist(s) with that clip from map populated earlier
      const auto& singleEpisodePlaylists{it->second.find(clip)->second};

      for (const auto& singleEpisodePlaylist : singleEpisodePlaylists)
      {
        if (!playlists.contains(singleEpisodePlaylist))
        {
          CLog::LogF(LOGERROR, "Single episode playlist {} missing in playlist map",
                     singleEpisodePlaylist);
          return;
        }
        // Get playlist information
        const PlaylistInformation& singleEpisodePlaylistInformation{
            playlists.find(singleEpisodePlaylist)->second};

        CLog::LogFC(LOGDEBUG, LOGBLURAY, "Candidate playlist {} duration {}", singleEpisodePlaylist,
                    static_cast<int>(singleEpisodePlaylistInformation.duration.count() / 1000));

        m_candidatePlaylists.try_emplace(
            singleEpisodePlaylist,
            CandidatePlaylistInformation{
                .playlist = singleEpisodePlaylist,
                .index = i + m_numSpecials - playlistInformation.playAllPlaylistEpisodesStartOffset,
                .duration = singleEpisodePlaylistInformation.duration,
                .clips = singleEpisodePlaylistInformation.clips,
                .languages = singleEpisodePlaylistInformation.languages});
      }
    }
    ++i;
  }
}

void CDiscDirectoryHelper::UseLongOrCommonMethodForSingleEpisode(int episodeIndex,
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

  // See how many unique (different length) episode length(>= minEpisodeDuration) playlists there are
  auto episodeLengthPlaylists{
      playlists_length | std::views::filter([this](const PlaylistVectorEntry& p)
                                            { return p.second.duration >= m_minEpisodeDuration; })};

  // Look for common starting playlists
  constexpr std::array<unsigned int, 5> commonStartingPlaylists = {801, 800, 1, 811, 0};
  auto playlistsView{episodeLengthPlaylists | std::views::keys};
  const auto commonPlaylist{std::ranges::find_first_of(playlistsView, commonStartingPlaylists)};

  if (std::ranges::distance(episodeLengthPlaylists) == 1)
  {
    // If only one long playlist, then assume it's that
    const auto& [playlist, playlistInformation] = *episodeLengthPlaylists.begin();
    CLog::LogFC(LOGDEBUG, LOGBLURAY, "Single Episode - found using single long playlist method");
    CLog::LogFC(LOGDEBUG, LOGBLURAY, "Candidate playlist {}", playlist);
    m_candidatePlaylists.try_emplace(
        playlist, CandidatePlaylistInformation{
                      .playlist = playlist,
                      .index = m_allEpisodes == AllEpisodes::ALL ? m_numSpecials : episodeIndex,
                      .duration = playlistInformation.duration,
                      .clips = playlistInformation.clips,
                      .languages = playlistInformation.languages});
  }
  else if (commonPlaylist != playlistsView.end())
  {
    // Found a common playlist, so assume it's that
    const auto& playlistInformation{playlists.at(*commonPlaylist)};
    CLog::LogFC(LOGDEBUG, LOGBLURAY, "Single Episode - found using common playlist method");
    CLog::LogFC(LOGDEBUG, LOGBLURAY, "Candidate playlist {}", *commonPlaylist);
    m_candidatePlaylists.try_emplace(
        *commonPlaylist,
        CandidatePlaylistInformation{.playlist = *commonPlaylist,
                                     .index = m_allEpisodes == AllEpisodes::ALL ? m_numSpecials
                                                                                : episodeIndex,
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
    auto CandidatePlaylistInformationNotDuplicate{
        [&seenDurations](const CandidatePlaylistInformation& c) noexcept
        { return seenDurations.insert(c.duration.count()).second; }};

    std::ranges::copy_if(playlistGroup, std::back_inserter(uniqueGroups.emplace_back()),
                         CandidatePlaylistInformationNotDuplicate);
  }
  return uniqueGroups;
}

void CDiscDirectoryHelper::GetPlaylistsFromGroup(
    int episodeIndex, const std::vector<CandidatePlaylistInformation>& group)
{
  const int episodeOffset{episodeIndex - static_cast<int>(m_numSpecials)};
  for (unsigned int i = 0; i < m_numEpisodes; ++i)
  {
    if (m_allEpisodes != AllEpisodes::ALL &&
        std::cmp_not_equal(i, episodeOffset)) // Specials before episodes in episodesOnDisc
      continue;

    m_candidatePlaylists.try_emplace(group[i].playlist,
                                     CandidatePlaylistInformation{.playlist = group[i].playlist,
                                                                  .index = i + m_numSpecials,
                                                                  .duration = group[i].duration,
                                                                  .clips = group[i].clips,
                                                                  .languages = group[i].languages});
    CLog::LogFC(LOGDEBUG, LOGBLURAY, "Candidate playlist {}", group[i].playlist);
  }
}

bool CDiscDirectoryHelper::CheckGroupDurations(
    const std::vector<CandidatePlaylistInformation>& group,
    const Episodes& episodesOnDisc,
    int durationTolerancePercent) const
{
  return std::ranges::all_of(std::views::iota(0u, m_numEpisodes),
                             [&](const unsigned i)
                             {
                               const auto& episode = episodesOnDisc[i + m_numSpecials];
                               return CheckDurationsWithinTolerance(episode.duration * 1000ms,
                                                                    group[i].duration,
                                                                    durationTolerancePercent);
                             });
}

bool CDiscDirectoryHelper::CheckGroupDurations(
    const std::vector<CandidatePlaylistInformation>& groupA,
    const std::vector<CandidatePlaylistInformation>& groupB,
    int durationTolerancePercent) const
{
  return std::ranges::all_of(std::views::iota(0u, m_numEpisodes),
                             [&](const unsigned i)
                             {
                               return CheckDurationsWithinTolerance(groupA[i].duration,
                                                                    groupB[i].duration,
                                                                    durationTolerancePercent);
                             });
}

// Decide if the group identified is likely to contain the episodes
// We might be in a position where we have a group of shorter playlists but the actual episode (longer)
// playlists are not in a group
bool CDiscDirectoryHelper::CheckGroup(const std::vector<CandidatePlaylistInformation>& group,
                                      const Episodes& episodesOnDisc) const
{
  // If we have episode durations in episodesOnDisc
  if (std::ranges::all_of(episodesOnDisc | std::views::drop(m_numSpecials),
                          [](const Episode& e) { return e.duration > 0; }))
  {
    // First check they are within tolerance of the durations in the group
    // This makes it likely the group represents the episodes
    if (CheckGroupDurations(group, episodesOnDisc))
      return true;

    // Now see if the durations of the longest playlists are within tolerance of the expected episodes on disc
    // If so, this probably isn't a valid group
    if (!m_nthLongestPlaylists.empty() &&
        CheckGroupDurations(m_nthLongestPlaylists, episodesOnDisc))
      return false;
  }
  else
  {
    // If we don't have all durations (often the case as they are loaded sequentially by the scraper)
    if (!m_nthLongestPlaylists.empty())
    {
      // Compare the longest playlist group to this group
      // Use a more relaxed tolerance (arbitrarily chosen)
      constexpr int RELAXED_DURATION_TOLERANCE_PERCENT = 40;
      if (!CheckGroupDurations(m_nthLongestPlaylists, group, RELAXED_DURATION_TOLERANCE_PERCENT))
        return false; // This probably isn't a valid group
    }
  }

  return true;
}

void CDiscDirectoryHelper::UseGroupMethod(int episodeIndex,
                                          const Episodes& episodesOnDisc,
                                          const PlaylistMap& playlists)
{
  // Method 2ai - More than one episode on disc

  // Use groups and find nth playlist (or all for all episodes) in group
  // Groups are already contain at least numEpisodes playlists of minimum duration
  // Firstly look just at groups that contain exactly numEpisodes playlists
  // Having removed duplicates
  CLog::LogFC(LOGDEBUG, LOGBLURAY, "Using group method - exact number of playlists");

  const std::vector groups{GetGroupsWithoutDuplicates(m_groups)};
  for (const auto& group : groups)
  {
    if (group.size() != m_numEpisodes)
      continue;

    if (!CheckGroup(group, episodesOnDisc))
      continue;

    GetPlaylistsFromGroup(episodeIndex, group);
  }

  if (m_candidatePlaylists.empty())
  {
    // Now look for groups that contain same/more than numEpisodes playlists (with duplicates)
    // Check that the first numEpisodes playlists have a duration within 20% of the desired episode
    // Exclude episodes with 0 duration as this will skew results
    CLog::LogFC(LOGDEBUG, LOGBLURAY, "Using group method - relaxed number of playlists");
    std::chrono::milliseconds episodeDuration;
    if (m_allEpisodes == AllEpisodes::ALL)
      episodeDuration = GetAverageEpisodeDuration(episodesOnDisc);
    else
      episodeDuration = episodesOnDisc[episodeIndex].duration * 1000ms;

    if (episodeDuration > 0ms)
    {
      for (const auto& group : m_groups)
      {
        if (group.size() < m_numEpisodes)
          continue;

        // Check duration
        if (!std::ranges::all_of(
                group | std::views::take(m_numEpisodes) |
                    std::views::transform(&CandidatePlaylistInformation::duration),
                [episodeDuration](const std::chrono::milliseconds playlistDuration)
                { return CheckDurationsWithinTolerance(episodeDuration, playlistDuration); }))
          continue;

        GetPlaylistsFromGroup(episodeIndex, group);
      }
    }
  }

  if (m_candidatePlaylists.empty() && !m_nthLongestPlaylists.empty())
  {
    // Now use the longest playlists
    // These should not be sequential (as they would have been handled above)
    if (std::ranges::adjacent_find(
            m_nthLongestPlaylists,
            [](const CandidatePlaylistInformation& a, const CandidatePlaylistInformation& b)
            { return b.playlist != a.playlist + 1; }) != m_nthLongestPlaylists.end())
    {
      // Now ensure there are no other playlists of similar length to the longest
      // Get the next longest playlist
      std::vector<PlaylistInformation> tmp;
      tmp.reserve(playlists.size());
      std::ranges::copy(
          playlists | std::views::values |
              std::views::filter([this](const PlaylistInformation& p)
                                 { return !m_playAllPlaylists.contains(p.playlist); }),
          std::back_inserter(tmp));

      // If there is no playlist beyond the m_nthLongestPlaylists ones, there is nothing
      // to compare against, so there cannot be a next playlist of similar length
      bool nextPlaylistIsClose{false};
      if (tmp.size() > m_nthLongestPlaylists.size())
      {
        std::ranges::nth_element(tmp, tmp.begin() + (m_nthLongestPlaylists.size()),
                                 [](const PlaylistInformation& a, const PlaylistInformation& b)
                                 { return a.duration > b.duration; });

        const auto lengthNext{tmp[m_nthLongestPlaylists.size()].duration};
        const auto length{std::ranges::min_element(m_nthLongestPlaylists, {},
                                                   &CandidatePlaylistInformation::duration)
                              ->duration};
        nextPlaylistIsClose = CheckDurationsWithinTolerance(length, lengthNext);
      }

      // If the next longest playlist is close to those in nthLongestPlaylists then
      // we cannot be certain the nthLongestPlaylists are the episodes
      if (!nextPlaylistIsClose)
        if (CheckGroup(m_nthLongestPlaylists, episodesOnDisc))
          GetPlaylistsFromGroup(episodeIndex, m_nthLongestPlaylists);
    }
  }

  if (m_candidatePlaylists.empty())
    CLog::LogFC(LOGDEBUG, LOGBLURAY, "No candidate playlists found");
}

namespace
{
int CalculateMultiple(std::chrono::milliseconds duration,
                      std::chrono::milliseconds averageShortest,
                      double multiplePercent)
{
  if (averageShortest == 0ms)
    return 0;

  double multiple{static_cast<double>(duration.count()) /
                  static_cast<double>(averageShortest.count())};
  int integerMultiple{static_cast<int>(std::round(multiple))};
  if (abs(multiple - integerMultiple) < multiplePercent / 100.0)
    return integerMultiple;

  return 0;
}
} // namespace

std::chrono::milliseconds CDiscDirectoryHelper::CalculateAverageOfShortEpisodes(
    const std::vector<CandidatePlaylistInformation>& group)
{
  constexpr double SHORTEST_PERCENT{20.0};

  auto nonZero{group | std::views::filter([](const CandidatePlaylistInformation& p)
                                          { return p.duration > 0ms; })};
  if (nonZero.empty())
    return 0ms;

  // Find shortest playlist in group
  const std::chrono::milliseconds shortest = std::ranges::min(
      nonZero | std::views::transform(&CandidatePlaylistInformation::duration), {});

  // Then calculate the average of shortest (within 20% of the shortest) playlists
  const auto threshold = shortest * (1 + (SHORTEST_PERCENT / 100));
  std::vector<std::chrono::milliseconds> groupDurations;
  for (const auto& p : nonZero)
  {
    if (p.duration < threshold)
      groupDurations.push_back(p.duration);
  }

  if (groupDurations.empty())
    return 0ms;

  const std::chrono::milliseconds averageShortest{
      std::accumulate(groupDurations.begin(), groupDurations.end(), 0ms) /
      static_cast<long long>(groupDurations.size())};

  return averageShortest;
}

void CDiscDirectoryHelper::UseGroupsWithMultiplesMethod(int episodeIndex,
                                                        const Episodes& episodesOnDisc)
{
  // No groups of numEpisodes length so see if there could be double episode playlists
  // Assume more than one playlist
  CLog::LogFC(LOGDEBUG, LOGBLURAY, "Using groups with multiples method");
  for (auto& group : GetGroupsWithoutDuplicates(m_allGroups))
  {
    // Calculate multiples
    const std::chrono::milliseconds averageShortest{CalculateAverageOfShortEpisodes(group)};

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
    const int episodeOffset{episodeIndex - static_cast<int>(m_numSpecials)};
    unsigned int index{
        m_numSpecials}; // Start at numSpecials as specials (S00) are before episodes in episodesOnDisc
    for (const auto& playlist : group)
    {
      auto playlistInformation{playlist};
      for (int i = 0; i < playlist.multiple; ++i)
      {
        if (m_allEpisodes == AllEpisodes::ALL || std::cmp_equal(index, episodeOffset))
        {
          playlistInformation.index = index;
          m_candidatePlaylists.try_emplace(playlist.playlist, playlistInformation);
          CLog::LogFC(LOGDEBUG, LOGBLURAY, "Candidate playlist {} for episode {}",
                      playlist.playlist, episodesOnDisc[index].iEpisode);
        }
        ++index;
      }
    }
    break;
  }

  if (m_candidatePlaylists.empty())
    CLog::LogFC(LOGDEBUG, LOGBLURAY, "No candidate playlists found");
}

void CDiscDirectoryHelper::ChooseSingleBestPlaylist(const Episodes& episodesOnDisc)
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
    std::chrono::milliseconds duration{episodesOnDisc[currentEpisodeIndex].duration * 1000};

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
    if (filteredCandidatePlaylists.empty())
    {
      CLog::LogFC(LOGDEBUG, LOGBLURAY, "No candidate playlists found for episode index {}",
                  currentEpisodeIndex);
      return;
    }
    const unsigned int playlist{filteredCandidatePlaylists[0].playlist};
    m_candidatePlaylists.try_emplace(playlist, filteredCandidatePlaylists[0]);

    CLog::LogFC(LOGDEBUG, LOGBLURAY,
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
        CLog::LogFC(LOGDEBUG, LOGBLURAY,
                    "Adding playlist {} as same duration and clips as playlist {}", playlist,
                    candidatePlaylist);
        m_candidatePlaylists.try_emplace(playlist, candidatePlaylistInformation);
      }
    }
  }
}

void CDiscDirectoryHelper::FindCandidatePlaylists(const Episodes& episodesOnDisc,
                                                  int episodeIndex,
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
    CLog::LogFC(LOGDEBUG, LOGBLURAY, "Looking for all episodes on disc");

  m_candidatePlaylists.clear();
  m_candidateSpecials.clear();

  if (m_playAllPlaylists.size() == 1)
    UsePlayAllPlaylistMethod(episodeIndex, playlists);
  else if (m_numEpisodes == 1)
    UseLongOrCommonMethodForSingleEpisode(episodeIndex, playlists);
  else if (!m_groups.empty())
    UseGroupMethod(episodeIndex, episodesOnDisc, playlists);

  if (m_candidatePlaylists.empty() && !m_allGroups.empty() && m_numEpisodes > 1)
    UseGroupsWithMultiplesMethod(episodeIndex, episodesOnDisc);

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

  // Take the remaining playlists as specials
  // If more than one candidate, we don't know which the special is, so include them all
  if (playlistsLength.size() >= m_numSpecials)
  {
    for (unsigned int playlist : playlistsLength | std::views::keys)
      m_candidateSpecials.emplace(playlist);
  }
}

namespace
{
std::shared_ptr<CFileItem> GenerateEpisodeItem(const CURL& url,
                                               unsigned int playlist,
                                               const PlaylistInformation& information,
                                               const Episode& episode,
                                               bool isSpecial)
{
  CURL path{url};
  std::string buf{StringUtils::Format("BDMV/PLAYLIST/{:05}.mpls", playlist)};
  path.SetFileName(buf);
  const auto item{std::make_shared<CFileItem>(path.Get(), false)};

  // Get clips
  const std::chrono::milliseconds duration{information.duration};

  // Get languages
  const std::string langs{information.languages};

  CVideoInfoTag* itemTag{item->GetVideoInfoTag()};
  itemTag->SetDuration(static_cast<int>(duration.count() / 1000));
  item->SetProperty("bluray_playlist", playlist);

  // Get episode title
  const std::string& title{episode.strTitle};
  if (isSpecial)
  {
    if (title.empty())
      buf = CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(21350); /* Special */
    else
      /* Special xx - title */
      buf = StringUtils::Format(
          CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(21348), episode.iEpisode,
          episode.strTitle);
  }
  else
    /* Episode xx - title */
    buf =
        StringUtils::Format(CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(21349),
                            episode.iEpisode, episode.strTitle);
  item->SetTitle(buf);
  item->SetLabel(buf);

  item->SetLabel2(StringUtils::Format(
      CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(25005) /* Title: {0:d} */ +
          " - {1:s}: {2:s}\r\n{3:s}: {4:s}",
      playlist,
      CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(180) /* Duration */,
      StringUtils::SecondsToTimeString(static_cast<int>(duration.count() / 1000)),
      CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(24026) /* Languages */,
      langs));
  item->SetSize(0);
  item->SetArt("icon", "DefaultVideo.png");

  return item;
}
} // namespace

void CDiscDirectoryHelper::EndEpisodePlaylistSearch()
{
  CLog::LogFC(LOGDEBUG, LOGBLURAY, "*** Episode Search End ***");
}

void CDiscDirectoryHelper::PopulateEpisodeFileItems(const CURL& url,
                                                    CFileItemList& items,
                                                    const CFileItemList& allTitles,
                                                    int episodeIndex,
                                                    const Episodes& episodesOnDisc,
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
      if (!playlists.contains(playlist.playlist))
      {
        CLog::LogF(LOGERROR, "Playlist {} missing in playlist map", playlist.playlist);
        continue;
      }
      const auto& information{playlists.find(playlist.playlist)->second};
      const auto newItem{GenerateEpisodeItem(url, playlist.playlist, information,
                                             episodesOnDisc[playlist.index], false)}; // Episode
      if (!newItem)
      {
        CLog::LogFC(LOGDEBUG, LOGBLURAY, "Failed to generate FileItem for playlist {}",
                    playlist.playlist);
        continue;
      }

      if (const auto detailsItem{allTitles.Get(newItem->GetPath())}; detailsItem)
        newItem->GetVideoInfoTag()->m_streamDetails =
            detailsItem->GetVideoInfoTag()->m_streamDetails;
      else
        CLog::LogFC(LOGDEBUG, LOGBLURAY, "Failed to find streamdetails for playlist {}",
                    playlist.playlist);

      items.Add(newItem);
    }
  }

  if (m_isSpecial == IsSpecial::SPECIAL)
  {
    for (const auto& playlist : m_candidateSpecials)
    {
      if (!playlists.contains(playlist))
      {
        CLog::LogF(LOGERROR, "Playlist {} missing in playlist map", playlist);
        continue;
      }
      const auto& information{playlists.find(playlist)->second};

      Episode episode;
      if (m_isSpecial == IsSpecial::SPECIAL && m_candidateSpecials.size() == 1)
        episode = episodesOnDisc[episodeIndex];

      const auto newItem{GenerateEpisodeItem(url, playlist, information, episode, true)}; // Special
      if (!newItem)
      {
        CLog::LogFC(LOGDEBUG, LOGBLURAY, "Failed to generate FileItem for playlist {}", playlist);
        continue;
      }

      if (const auto detailsItem{allTitles.Get(newItem->GetPath())}; detailsItem)
        newItem->GetVideoInfoTag()->m_streamDetails =
            detailsItem->GetVideoInfoTag()->m_streamDetails;
      else
        CLog::LogFC(LOGDEBUG, LOGBLURAY, "Failed to find streamdetails for playlist {}", playlist);

      items.Add(newItem);
    }
  }
}

bool CDiscDirectoryHelper::GetEpisodePlaylists(
    const CURL& url,
    CFileItemList& items,
    const CFileItemList& allTitles, // FileItem for each playlist including stream details
    int episodeIndex,
    const Episodes& episodesOnDiscUnsorted,
    const ClipMap& clips,
    const PlaylistMap& playlists)
{
  items.Clear();

  // Checks
  if (playlists.empty() || clips.empty() || episodeIndex < -1 ||
      std::cmp_greater_equal(episodeIndex, episodesOnDiscUnsorted.size()))
    return false;

  // Sort (subsequent routines assume that specials (season 0) are before episodes)
  auto episodesOnDisc{episodesOnDiscUnsorted};
  std::ranges::sort(episodesOnDisc, std::ranges::less{}, [](const Episode& e)
                    { return std::tie(e.iSeason, e.iEpisode, e.iSubepisode); });

  InitialiseEpisodePlaylistSearch(episodeIndex, episodesOnDisc);
  FindPlayAllPlaylists(clips, playlists);
  FindGroups(playlists, episodesOnDisc);
  FindCandidatePlaylists(episodesOnDisc, episodeIndex, playlists);
  FindSpecials(playlists);
  EndEpisodePlaylistSearch();
  PopulateEpisodeFileItems(url, items, allTitles, episodeIndex, episodesOnDisc, playlists);

  return !items.IsEmpty();
}

namespace
{
void InitialiseAllEpisodesPlaylistSearch(std::vector<PlaylistInformation>& playlists,
                                         const PlaylistMap& playlistMap)
{
  playlists.reserve(playlistMap.size());
  std::ranges::transform(playlistMap, std::back_inserter(playlists),
                         [](const PlaylistMapEntry& pair) { return pair.second; });
}

std::shared_ptr<CFileItem> GenerateAllEpisodesItem(const CURL& url,
                                                   unsigned int playlist,
                                                   const PlaylistInformation& information)
{
  CURL path{url};
  path.SetFileName(StringUtils::Format("BDMV/PLAYLIST/{:05}.mpls", playlist));
  const auto item{std::make_shared<CFileItem>(path.Get(), false)};

  // Get clips
  const std::chrono::milliseconds duration{information.duration};

  // Get languages
  const std::string langs{information.languages};

  CVideoInfoTag* itemTag{item->GetVideoInfoTag()};
  itemTag->SetDuration(static_cast<int>(duration.count() / 1000));
  item->SetProperty("bluray_playlist", playlist);

  // Get title
  const std::string title{StringUtils::Format(
      CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(25005), playlist)};
  item->SetTitle(title);
  item->SetLabel(title);

  item->SetLabel2(StringUtils::Format(
      CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(25005) /* Title: {0:d} */ +
          " - {1:s}: {2:s}\r\n{3:s}: {4:s}",
      playlist,
      CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(180) /* Duration */,
      StringUtils::SecondsToTimeString(static_cast<int>(duration.count() / 1000)),
      CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(24026) /* Languages */,
      langs));
  item->SetSize(0);
  item->SetArt("icon", "DefaultVideo.png");

  return item;
}

void PopulateAllEpisodesFileItems(const CURL& url,
                                  CFileItemList& items,
                                  const CFileItemList& allTitles,
                                  const std::vector<PlaylistInformation>& playlists,
                                  const PlaylistMap& playlistMap)
{
  // Sort by playlist
  auto sortedPlaylists = playlists;
  std::ranges::sort(sortedPlaylists, {}, &PlaylistInformation::playlist);

  for (const auto& playlist : sortedPlaylists)
  {
    if (!playlistMap.contains(playlist.playlist))
    {
      CLog::LogF(LOGERROR, "Playlist {} missing in playlist map", playlist.playlist);
      continue;
    }
    const auto& information{playlistMap.find(playlist.playlist)->second};
    const auto newItem{GenerateAllEpisodesItem(url, playlist.playlist, information)};
    if (!newItem)
    {
      CLog::LogFC(LOGDEBUG, LOGBLURAY, "Failed to generate FileItem for playlist {}",
                  playlist.playlist);
      continue;
    }

    if (const auto detailsItem{allTitles.Get(newItem->GetPath())}; detailsItem)
      newItem->GetVideoInfoTag()->m_streamDetails = detailsItem->GetVideoInfoTag()->m_streamDetails;
    else
      CLog::LogFC(LOGDEBUG, LOGBLURAY, "Failed to find streamdetails for playlist {}",
                  playlist.playlist);

    items.Add(newItem);
  }
}
} // namespace

bool CDiscDirectoryHelper::FilterAllEpisodesPlaylists(std::vector<PlaylistInformation>& playlists,
                                                      GetTitle job)
{
  if (job != GetTitle::ALL)
  {
    // Remove all clips less than minEpisodeDuration or play all playlist
    std::erase_if(playlists,
                  [this](const PlaylistInformation& playlist)
                  {
                    return playlist.duration < m_minEpisodeDuration ||
                           m_playAllPlaylists.contains(playlist.playlist);
                  });
  }
  return !playlists.empty();
}

bool CDiscDirectoryHelper::GetAllEpisodePlaylists(
    const CURL& url,
    CFileItemList& items,
    const CFileItemList& allTitles, // FileItem for each playlist including stream details
    GetTitle job,
    const Episodes& episodesOnDiscUnsorted,
    const ClipMap& clips,
    const PlaylistMap& playlistMap)
{
  items.Clear();

  // Checks
  if (playlistMap.empty() || clips.empty())
    return false;

  // Sort (subsequent routines assume that specials (season 0) are before episodes)
  if (!episodesOnDiscUnsorted.empty())
  {
    auto episodesOnDisc{episodesOnDiscUnsorted};
    std::ranges::sort(episodesOnDisc, std::ranges::less{}, [](const Episode& e)
                      { return std::tie(e.iSeason, e.iEpisode, e.iSubepisode); });

    InitialiseEpisodePlaylistSearch(ALL_PLAYLISTS, episodesOnDisc);
    FindPlayAllPlaylists(clips, playlistMap);
  }

  std::vector<PlaylistInformation> playlists;
  InitialiseAllEpisodesPlaylistSearch(playlists, playlistMap);
  if (!FilterAllEpisodesPlaylists(playlists, job))
    return false;
  EndEpisodePlaylistSearch();
  PopulateAllEpisodesFileItems(url, items, allTitles, playlists, playlistMap);

  return !items.IsEmpty();
}

namespace
{
void InitialiseMoviePlaylistSearch(std::vector<PlaylistInformation>& playlists,
                                   const PlaylistMap& playlistMap)
{
  playlists.reserve(playlistMap.size());
  std::ranges::transform(playlistMap, std::back_inserter(playlists),
                         [](const PlaylistMapEntry& pair) { return pair.second; });

  CLog::LogFC(LOGDEBUG, LOGBLURAY, "*** Movie Search Start ***");
}

bool FilterMoviePlaylists(std::vector<PlaylistInformation>& playlists, GetTitle job)
{
  if (job != GetTitle::ALL)
  {
    // Remove all playlists less than MIN_MOVIE_DURATION
    std::erase_if(playlists, [](const PlaylistInformation& playlist)
                  { return playlist.duration < MIN_MOVIE_DURATION; });
  }
  return !playlists.empty();
}

void GetMainMoviePlaylists(std::vector<PlaylistInformation>& playlists,
                           GetTitle job,
                           int mainPlaylist)
{
  // If any playlists have more than one chapter, discard those without
  if (job != GetTitle::ALL && std::ranges::any_of(playlists, [](const PlaylistInformation& p)
                                                  { return p.chapters.size() > 1; }))
  {
    std::erase_if(playlists,
                  [mainPlaylist](const PlaylistInformation& p) {
                    return p.chapters.size() <= 1 && std::cmp_not_equal(p.playlist, mainPlaylist);
                  });
  }

  if (playlists.empty())
    return;

  if (job == GetTitle::SINGLE && mainPlaylist >= 0)
  {
    const auto it{std::ranges::find(playlists, mainPlaylist, &PlaylistInformation::playlist)};
    if (it != playlists.end())
    {
      playlists = {std::move(*it)};
      return;
    }
  }

  const auto it{std::ranges::max_element(playlists, {}, &PlaylistInformation::duration)};
  if (job == GetTitle::SINGLE)
  {
    // Single longest playlist
    playlists = {std::move(*it)};
    return;
  }

  if (job == GetTitle::MAIN)
  {
    // All playlists with duration of at least 70% of the longest title (to allow multiple editions on same disc)
    const auto minimumDuration{it->duration * MAIN_TITLE_LENGTH_PERCENT / 100};
    std::erase_if(playlists,
                  [minimumDuration, mainPlaylist](const PlaylistInformation& playlist)
                  {
                    return playlist.duration < minimumDuration &&
                           std::cmp_not_equal(playlist.playlist, mainPlaylist);
                  });
  }
}

void EndMoviePlaylistSearch()
{
  CLog::LogFC(LOGDEBUG, LOGBLURAY, "*** Movie Search End ***");
}

std::shared_ptr<CFileItem> GenerateMovieItem(const CURL& url,
                                             unsigned int playlist,
                                             unsigned int mainPlaylist,
                                             const PlaylistInformation& information)
{
  CURL path{url};
  std::string buf{StringUtils::Format("BDMV/PLAYLIST/{:05}.mpls", playlist)};
  path.SetFileName(buf);
  const auto item{std::make_shared<CFileItem>(path.Get(), false)};

  // Get clips
  const std::chrono::milliseconds duration{information.duration};

  // Get languages
  const std::string langs{information.languages};

  CVideoInfoTag* itemTag{item->GetVideoInfoTag()};
  itemTag->SetDuration(static_cast<int>(duration.count() / 1000));
  item->SetProperty("bluray_playlist", playlist);

  buf = StringUtils::Format(
      playlist == mainPlaylist
          ? CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(25004) /* Main Title */
          : CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(25005) /* Title */,
      playlist);
  item->SetTitle(buf);
  item->SetLabel(buf);

  const std::string chap{StringUtils::Format(
      CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(25007),
      information.chapters.size(),
      StringUtils::SecondsToTimeString(static_cast<int>(duration.count() / 1000)))};
  item->SetLabel2(chap);

  item->SetSize(0);
  item->SetArt("icon", "DefaultVideo.png");

  return item;
}

void PopulateMovieFileItems(
    const CURL& url,
    CFileItemList& items,
    int mainPlaylist,
    const CFileItemList& allTitles, // FileItem for each playlist including stream details
    const std::vector<PlaylistInformation>& playlists)
{
  // Sort by duration (putting mainPlaylist first if present)
  auto sortedPlaylists = playlists;
  std::ranges::sort(sortedPlaylists,
                    [mainPlaylist](const PlaylistInformation& a, const PlaylistInformation& b)
                    {
                      const bool aIsMain{std::cmp_equal(a.playlist, mainPlaylist)};
                      const bool bIsMain{std::cmp_equal(b.playlist, mainPlaylist)};

                      if (aIsMain || bIsMain)
                        return aIsMain && !bIsMain;

                      if (a.duration != b.duration)
                        return a.duration > b.duration;

                      return a.playlist < b.playlist;
                    });

  for (const auto& playlist : sortedPlaylists)
  {
    const auto newItem{GenerateMovieItem(url, playlist.playlist, mainPlaylist, playlist)};
    if (!newItem)
    {
      CLog::LogFC(LOGDEBUG, LOGBLURAY, "Failed to generate FileItem for playlist {}",
                  playlist.playlist);
      continue;
    }

    if (const auto detailsItem{allTitles.Get(newItem->GetPath())}; detailsItem)
      newItem->GetVideoInfoTag()->m_streamDetails = detailsItem->GetVideoInfoTag()->m_streamDetails;
    else
      CLog::LogFC(LOGDEBUG, LOGBLURAY, "Failed to find streamdetails for playlist {}",
                  playlist.playlist);

    items.Add(newItem);
  }
}
} // namespace

bool CDiscDirectoryHelper::GetMoviePlaylists(const CURL& url,
                                             CFileItemList& items,
                                             const CFileItemList& allTitles,
                                             int mainPlaylist,
                                             GetTitle job,
                                             const ClipMap& clips,
                                             const PlaylistMap& playlistMap)
{
  items.Clear();

  // Checks
  if (playlistMap.empty() || clips.empty())
    return false;

  std::vector<PlaylistInformation> playlists;
  InitialiseMoviePlaylistSearch(playlists, playlistMap);
  if (!FilterMoviePlaylists(playlists, job))
    return false;
  GetMainMoviePlaylists(playlists, job, mainPlaylist);
  PopulateMovieFileItems(url, items, mainPlaylist, allTitles, playlists);
  EndMoviePlaylistSearch();

  return !items.IsEmpty();
}

void CDiscDirectoryHelper::AddRootOptions(const CURL& url,
                                          CFileItemList& items,
                                          AllTitles allTitlesType,
                                          AddMenuOption addMenuOption)
{
  CURL path{url};
  if (allTitlesType == AllTitles::MOVIES)
    path.SetFileName(URIUtils::AddFileToFolder("root", "titles", "all"));
  else if (allTitlesType == AllTitles::EPISODES)
    path.SetFileName(URIUtils::AddFileToFolder("root", "titles", "episodes", "all"));

  auto item{std::make_shared<CFileItem>(path.Get(), true)};
  item->SetLabel(
      CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(25002) /* All titles */);
  item->SetArt("icon", "DefaultVideoPlaylists.png");
  items.Add(item);

  if (addMenuOption == AddMenuOption::ADD_MENU)
  {
    path.SetFileName("menu");
    item = {std::make_shared<CFileItem>(path.Get(), false)};
    item->SetLabel(
        CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(25003) /* Menu */);
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

bool CDiscDirectoryHelper::GetOrShowPlaylistSelection(CFileItem& item, MenuDecision playback)
{
  const bool silent{playback == MenuDecision::SILENT};
  const std::string originalDynPath{
      item.GetDynPath()}; // Overwritten by dialog selection. Needed for screen refresh.

  const std::string directory{
      [&item, &originalDynPath, playback]
      {
        const bool forceSelection{item.GetProperty("force_playlist_selection").asBoolean(false)};

        // All episodes
        if (item.HasProperty("episodes_start"))
          return URIUtils::GetBlurayAllEpisodesPath(originalDynPath);

        // Single episode
        if (item.GetVideoContentType() == VideoDbContentType::EPISODES && !forceSelection)
        {
          const CVideoInfoTag* tag{item.GetVideoInfoTag()};
          return URIUtils::GetBlurayEpisodePath(originalDynPath, tag->m_iSeason, tag->m_iEpisode);
        }

        // Playlists > 70% longest
        using enum MenuDecision;
        using enum VideoDbContentType;
        if (playback == SHOW_SIMPLE_MENU)
        {
          if (item.GetVideoContentType() == EPISODES || item.GetVideoContentType() == TVSHOWS)
          {
            return URIUtils::GetBlurayTitlesPath(originalDynPath, URIUtils::GetAllTitles::LONG,
                                                 URIUtils::AllTitlesOptions::EPISODES);
          }
          else
          {
            return URIUtils::GetBlurayTitlesPath(originalDynPath, URIUtils::GetAllTitles::LONG,
                                                 URIUtils::AllTitlesOptions::MOVIES);
          }
        }

        // Single main title
        return URIUtils::GetBlurayMainTitlePath(originalDynPath);
      }()};

  // Get playlists that are already used (to avoid duplicates in file table)
  std::vector<CVideoDatabase::PlaylistInfo> usedPlaylists{};
  CVideoDatabase database;
  if (!database.Open())
  {
    CLog::LogF(LOGERROR, "Failed to open video database");
    return false;
  }

  // Add duration to bluray:// url as needed for episode determination in CBlurayDirectory
  std::string directoryDuration{directory};
  if (item.HasVideoInfoTag() && item.GetVideoInfoTag()->GetDuration() > 0 &&
      item.GetVideoContentType() == VideoDbContentType::EPISODES)
  {
    CURL dirUrl(directory);
    dirUrl.SetOption("duration", std::to_string(item.GetVideoInfoTag()->GetDuration()));
    directoryDuration = dirUrl.Get();
  }

  // Get items
  CFileItemList items;
  if (!GetItems(items, directoryDuration, silent))
  {
    // No main movie or episode playlist found
    if (silent)
      return false;

    // Not silent so fallback to all titles
    const std::string fallbackDirectory{
        item.GetVideoContentType() == VideoDbContentType::EPISODES ||
                item.GetVideoContentType() == VideoDbContentType::TVSHOWS
            ? URIUtils::GetBlurayTitlesPath(originalDynPath, URIUtils::GetAllTitles::ALL,
                                            URIUtils::AllTitlesOptions::EPISODES)
            : URIUtils::GetBlurayTitlesPath(originalDynPath, URIUtils::GetAllTitles::ALL,
                                            URIUtils::AllTitlesOptions::MOVIES)};
    if (!GetItems(items, fallbackDirectory, silent))
    {
      CGUIDialogOK::ShowAndGetInput(
          CVariant{257},
          CVariant{item.GetVideoContentType() == VideoDbContentType::EPISODES ? 25017 : 25016});
      return false;
    }
  }

  // Select item
  CFileItem selectedItem;
  if (!silent)
  {
    if (playback == MenuDecision::SHOW_SIMPLE_MENU)
    {
      usedPlaylists = database.GetPlaylistsByPath(URIUtils::GetBlurayPlaylistPath(originalDynPath));

      // If replacing existing playlist (FORCE_PLAYLIST_SELECTION), remove it from exclude list
      // as user could choose the same playlist again
      if (item.GetProperty("force_playlist_selection").asBoolean(false))
      {
        CRegExp regex{true, CRegExp::autoUtf8, R"(\/(\d{5}).mpls$)"};
        if (regex.RegFind(originalDynPath) != -1)
        {
          const int playlist{std::stoi(regex.GetMatch(1))};
          std::erase_if(usedPlaylists, [&playlist](const CVideoDatabase::PlaylistInfo& p)
                        { return p.playlist == playlist; });
        }
      }

      // Use simple menu dialog to select playlist
      while (true)
      {
        if (!CGUIDialogSimpleMenu::ShowPlaylistSelection(item, selectedItem, items, usedPlaylists))
          return false;

        // If a non-folder item is selected, we're done
        if (!selectedItem.IsFolder())
          break;

        // Folder selected - retrieve all titles within it
        if (!GetItems(items, selectedItem.GetDynPath(), silent))
          return false;
      }
    }
  }
  else
  {
    // Silent
    if (items.Size() > 1)
    {
      CLog::LogF(LOGERROR, "Unable to automatically determine main playlist for {}", directory);
      return false;
    }
  }

  if (selectedItem.GetPath().empty())
    selectedItem = *items[0]; // Main item

  item.SetDynPath(selectedItem.GetDynPath());
  item.GetVideoInfoTag()->m_streamDetails = selectedItem.GetVideoInfoTag()->m_streamDetails;
  item.SetProperty("original_listitem_url", originalDynPath);

  return true;
}

bool CDiscDirectoryHelper::GetItems(CFileItemList& items, const std::string& directory, bool silent)
{
  items.Clear();
  if (!GetDirectoryItems(directory, items, CDirectory::CHints(), silent))
  {
    CLog::LogF(LOGERROR, "Failed to get play directory for {}", directory);
    return false;
  }

  if (items.IsEmpty())
  {
    CLog::LogF(LOGERROR, "Failed to get any items in {}", directory);
    return false;
  }

  return true;
}

bool CDiscDirectoryHelper::GetDirectoryItems(const std::string& path,
                                             CFileItemList& items,
                                             const CDirectory::CHints& hints,
                                             bool silent)
{
  if (silent)
  {
    // Non-interactive path so skip the busy dialog
    return CDirectory::GetDirectory(path, items, hints);
  }

  CGetDirectoryItems getItems(path, items, hints);
  if (!CGUIDialogBusy::Wait(&getItems, 100, true))
  {
    return false;
  }
  return getItems.m_result;
}
