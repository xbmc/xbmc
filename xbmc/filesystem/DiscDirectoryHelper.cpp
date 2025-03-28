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
#include <iterator>
#include <ranges>
#include <set>
#include <tuple>

using namespace XFILE;

using PlaylistMapEntry = std::pair<unsigned int, PlaylistInfo>;
using PlaylistVector = std::vector<std::pair<unsigned int, PlaylistInfo>>;
using PlaylistVectorEntry = std::pair<unsigned int, PlaylistInfo>;

CDiscDirectoryHelper::CDiscDirectoryHelper()
{
  m_allEpisodes = AllEpisodes::SINGLE;
  m_isSpecial = IsSpecial::EPISODE;
  m_numEpisodes = 0;
  m_numSpecials = 0;

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
  if (m_numEpisodes > 1 && playlists.size() >= m_numEpisodes)
  {
    for (const auto& playlist : playlists)
    {
      const auto& [playlistNumber, playlistInformation] = playlist;

      // Find playlists that have a clip count = number of episodes on disc (+1/2) (1)
      if (playlistInformation.clips.size() >= m_numEpisodes &&
          playlistInformation.clips.size() <= m_numEpisodes + 2)
      {
        bool allClipsQualify{true};
        bool allowBeginningOrEnd{playlistInformation.clips.size() == m_numEpisodes + 1};
        const bool allowBeginningAndEnd{playlistInformation.clips.size() == m_numEpisodes + 2};

        // Map of clips in the play all playlist and the potential single episode playlists containing them
        std::map<unsigned int, std::vector<unsigned int>> playAllPlaylistClipMap;

        // For each clip in the playlist
        for (unsigned int clip : playlistInformation.clips)
        {
          if (!clips.contains(clip))
          {
            CLog::LogF(LOGERROR, "Clip {} missing in clip map", clip);
            return;
          }
          // Map of potential single episode playlists containing the clip
          std::vector<unsigned int> playAllPlaylistMap;

          // If clip doesn't appear in another playlist or clip is too short this is not a Play All playlist (2)(3)
          // BUT we allow first and/or last clip to be shorter (ie start intro/end credits) IF there are enough clips
          const ClipInfo& clipInformation{clips.find(clip)->second};
          bool skipClip{false};
          if (clipInformation.playlists.size() < 2 ||
              clipInformation.duration < MIN_EPISODE_DURATION)
          {
            if ((allowBeginningOrEnd || allowBeginningAndEnd) &&
                clip == playlistInformation.clips.front())
            {
              // First clip
              allowBeginningOrEnd = false; // Cannot have a short clip at end as well
              skipClip = true;
            }
            else if ((allowBeginningOrEnd || allowBeginningAndEnd) &&
                     clip == playlistInformation.clips.back())
            {
              // Last clip
              skipClip = true;
            }
            else
            {
              allClipsQualify = false;
              break;
            }
          }

          // Now check the other playlists to ensure they contain the clip and have at most only one other clip before or after (4)
          // (eg. starting recap/ending credits etc..)
          if (!skipClip)
          {
            for (const auto& singleEpisodePlaylist : clipInformation.playlists)
            {
              if (singleEpisodePlaylist !=
                  playlistNumber) // Exclude potential play all playlist we are currently examining
              {
                if (!playlists.contains(singleEpisodePlaylist))
                {
                  CLog::LogF(LOGERROR, "Single episode playlist {} missing in playlist map",
                             singleEpisodePlaylist);
                  return;
                }
                // Find the potential single episode playlist information
                const auto& singleEpisodePlaylistInformation{
                    playlists.find(singleEpisodePlaylist)->second};

                // See if potential single episode playlist contains too many clips
                // If there are 3 clips then expect the middle clip to be the main episode clip
                if (singleEpisodePlaylistInformation.clips.size() > 3 ||
                    (singleEpisodePlaylistInformation.clips.size() == 3 &&
                     singleEpisodePlaylistInformation.clips[1] != clip))
                {
                  allClipsQualify = false;
                  break;
                }
                playAllPlaylistMap.emplace_back(singleEpisodePlaylist);
              }
            }
          }
          playAllPlaylistClipMap[clip] = playAllPlaylistMap;
        }

        // Found potential play all playlist
        if (allClipsQualify)
        {
          CLog::LogF(LOGDEBUG, "Potential play all playlist {}", playlistNumber);
          m_playAllPlaylists.emplace_back(playlistNumber);
          m_playAllPlaylistsMap[playlistNumber] = playAllPlaylistClipMap;
        }
      }
    }
    if (m_playAllPlaylists.empty())
      CLog::LogF(LOGDEBUG, "No play all playlists found");
  }
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
                         return playlistInformation.duration >= MIN_EPISODE_DURATION &&
                                std::ranges::none_of(m_playAllPlaylists, [&playlist](const auto a)
                                                     { return playlist == a; });
                       });

  // Find groups
  if (m_numEpisodes > 1)
  {
    m_groups.emplace_back(1, longPlaylists.begin()->first);
    std::for_each(++longPlaylists.begin(), longPlaylists.end(),
                  [&](const PlaylistMapEntry& p)
                  {
                    const unsigned int playlist = p.first;
                    if (m_groups.back().back() == playlist - 1)
                      m_groups.back().emplace_back(playlist);
                    else
                      m_groups.emplace_back(1, playlist); // New group
                  });

    // See if any groups have at least numEpisode playlists
    if (std::erase_if(m_groups, [&](const std::vector<unsigned int>& group)
                      { return group.size() < m_numEpisodes; }) == 0)
    {
      // See if there are exactly numEpisode playlists remaining and no specials, in which case make a group
      // Assumption has to be playlists match episodes in ascending order
      if (m_numSpecials == 0 && longPlaylists.size() == m_numEpisodes)
      {
        m_groups.clear();
        m_groups.emplace_back(1, longPlaylists.begin()->first);
        std::for_each(std::next(longPlaylists.begin()), longPlaylists.end(),
                      [&](const PlaylistMapEntry& p) { m_groups.back().emplace_back(p.first); });
      }
    }

    if (m_groups.empty())
      CLog::LogF(LOGDEBUG, "No playlist groups found");
    else
      for (const auto& group : m_groups)
        CLog::LogF(LOGDEBUG, "Playlist group found from {} to {}", group.front(), group.back());
  }
  else
  {
    CLog::LogF(LOGDEBUG, "No group search as single episode or specials only");
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
  // 2a) Using the longest playlists that are consecutive, using the playlist in the nth position.
  //
  // 2b) For single episode discs, look for the longest playlist.
  //    There are some discs where there are extras that are longer than the episode itself, in n this case
  //    we look for the playlist with a common starting number (eg. 1, 800 etc..)

  bool foundEpisode{false};
  bool findIdenticalEpisodes{false};

  if (m_allEpisodes == AllEpisodes::ALL)
    CLog::LogF(LOGDEBUG, "Looking for all episodes on disc");

  // Method 1 - Play all playlist
  if (m_playAllPlaylists.size() == 1)
  {
    CLog::LogF(LOGDEBUG, "Using Play All playlist method");

    // Get the playlist
    const unsigned int playAllPlaylist{m_playAllPlaylists[0]};

    if (!playlists.contains(playAllPlaylist))
    {
      CLog::LogF(LOGERROR, "Play all playlist {} missing in playlist map", playAllPlaylist);
      return;
    }

    // Get the potential single episodes from the map
    const auto& [playlist, playlistInformation] = *playlists.find(playAllPlaylist);
    CLog::LogF(LOGDEBUG, "Using candidate play all playlist {} duration {}", playlist,
               playlistInformation.duration);

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
        const auto& singleEpisodePlaylists{
            m_playAllPlaylistsMap[playAllPlaylist].find(clip)->second};

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
                     singleEpisodePlaylistInformation.duration);

          m_candidatePlaylists.insert(
              {singleEpisodePlaylist,
               i + m_numSpecials}); // Also save episodeIndex for all episodes
        }
      }
      ++i;
    }
    foundEpisode = true;
    findIdenticalEpisodes = true;
  }

  // Method 2 - Look for the longest playlists in groups, taking the nth playlist
  if (!foundEpisode)
  {
    CLog::LogF(LOGDEBUG, "Using longest playlists in groups method");

    if (m_numEpisodes == 1)
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
          playlists_length |
          std::views::filter([](const PlaylistVectorEntry& p)
                             { return p.second.duration >= MIN_EPISODE_DURATION; })};

      // Look for common starting playlists
      constexpr std::array<unsigned int, 5> commonStartingPlaylists = {801, 800, 1, 811, 0};
      const auto& playlistsView{playlists | std::views::keys};
      const auto commonPlaylist{std::ranges::find_first_of(playlistsView, commonStartingPlaylists)};

      if (std::ranges::distance(episodeLengthPlaylists) == 1)
      {
        // If only one long playlist, then assume it's that
        const unsigned int playlist{episodeLengthPlaylists.begin()->first};
        CLog::LogF(LOGDEBUG, "Single Episode - found using single long playlist method");
        CLog::LogF(LOGDEBUG, "Candidate playlist {}", playlist);
        m_candidatePlaylists.insert({playlist, episodeIndex});
      }
      else if (commonPlaylist != playlistsView.end())
      {
        // Found a common playlist, so assume it's that
        CLog::LogF(LOGDEBUG, "Single Episode - found using common playlist method");
        CLog::LogF(LOGDEBUG, "Candidate playlist {}", *commonPlaylist);
        m_candidatePlaylists.insert({*commonPlaylist, episodeIndex});
      }
    }
    else
    {
      // Method 2a - More than one episode on disc

      // Use groups and find nth playlist (or all for all episodes) in group
      // Groups are already contain at least numEpisodes playlists of minimum duration
      if (!m_groups.empty())
      {
        for (const auto& group : m_groups)
        {
          if (group.size() == m_numEpisodes)
          {
            for (unsigned int i = 0; i < m_numEpisodes; ++i)
            {
              if (m_allEpisodes == AllEpisodes::ALL ||
                  i == episodeIndex - m_numSpecials) // Specials before episodes in episodesOnDisc
              {
                m_candidatePlaylists.insert(
                    {group[i], i + m_numSpecials}); // Also save episodeIndex for all episodes
                CLog::LogF(LOGDEBUG, "Candidate playlist {}", group[i]);
              }
            }
          }
        }
        findIdenticalEpisodes = true;
      }
    }
  }

  // Now deal with the possibility there may be more than one playlist (per episode)
  // For this, see which is closest in duration to the desired episode duration (from the scraper)
  // If we are looking for a special then leave all potential episode playlists in array
  if (m_candidatePlaylists.size() > 1 && m_isSpecial == IsSpecial::EPISODE)
  {
    // Rebuild candidatePlaylists
    const auto oldCandidatePlaylists{m_candidatePlaylists};
    m_candidatePlaylists.clear();

    // Loop through each episode (in case of all episodes)
    // Generate set of indexes of episode entry in episodesOnDisc
    std::set<unsigned int> indexes;
    for (unsigned int index : oldCandidatePlaylists | std::views::values)
      indexes.insert(index);

    for (unsigned int currentEpisodeIndex : indexes)
    {
      unsigned int duration{episodesOnDisc[currentEpisodeIndex].GetDuration()};
      if (duration == 0)
        duration = std::numeric_limits<
            int>::max(); // If episode length not known, ensure the longest playlist selected

      // Create vector of candidate playlists, duration difference and chapters
      std::vector<CandidatePlaylistsDurationInformation> candidatePlaylistsDuration;
      candidatePlaylistsDuration.reserve(oldCandidatePlaylists.size());
      for (const auto& [playlist, index] : oldCandidatePlaylists)
      {
        if (index == currentEpisodeIndex)
        {
          if (!playlists.contains(playlist))
          {
            CLog::LogF(LOGERROR, "Playlist {} missing in playlist map", playlist);
            return;
          }
          const auto& playlistInformation{playlists.find(playlist)->second};
          candidatePlaylistsDuration.emplace_back(CandidatePlaylistsDurationInformation{
              .playlist = playlist,
              .durationDelta = abs(static_cast<int>(playlistInformation.duration - duration)),
              .chapters = static_cast<unsigned int>(playlistInformation.chapters.size())});
        }
      }

      // Sort descending based on number of chapters and ascending by relative difference in duration
      std::ranges::sort(candidatePlaylistsDuration,
                        [](const auto& p, const auto& q)
                        {
                          if (p.chapters == q.chapters)
                            return p.durationDelta < q.durationDelta;
                          return p.chapters > q.chapters;
                        });

      // Keep the playlist with most chapters and closest duration
      const unsigned int playlist{candidatePlaylistsDuration[0].playlist};
      m_candidatePlaylists.insert({playlist, currentEpisodeIndex});

      CLog::LogF(LOGDEBUG,
                 "Remaining candidate playlist (closest in duration) is {} for episode index {}",
                 playlist, currentEpisodeIndex);
    }
  }

  // candidatePlaylists should now (ideally) contain one candidate title for the episode or all episodes
  // Now look at durations of found playlist and add identical (in case language options)
  if (findIdenticalEpisodes && !m_candidatePlaylists.empty())
  {
    for (const auto& [candidatePlaylist, candidatePlaylistEpisodeIndex] : m_candidatePlaylists)
    {
      if (!playlists.contains(candidatePlaylist))
      {
        CLog::LogF(LOGERROR, "Candidate playlist {} missing in playlist map", candidatePlaylist);
        return;
      }
      // Get candidatePlaylist clips and duration
      const auto& [candidatePlaylistNumber, candidatePlaylistInformation] =
          *playlists.find(candidatePlaylist);

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
          m_candidatePlaylists.insert({playlist, candidatePlaylistEpisodeIndex});
        }
      }
    }
  }
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
    std::erase_if(playlistsLength,
                  [&](const PlaylistVectorEntry& playlist)
                  {
                    const auto isShort{playlist.second.duration < MIN_SPECIAL_DURATION};
                    const auto isEpisode{
                        std::ranges::any_of(m_candidatePlaylists | std::views::elements<0>,
                                            [&playlist](const auto& candidatePlaylist)
                                            { return playlist.first == candidatePlaylist; })};
                    const auto isPlayAll{std::ranges::any_of(
                        m_playAllPlaylists, [&playlist](const unsigned int playAllPlaylist)
                        { return playlist.first == playAllPlaylist; })};

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
      m_candidateSpecials.emplace_back(playlist);
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
  const int duration{static_cast<int>(it->second.duration)};

  // Get languages
  const std::string langs{it->second.languages};

  CURL path{url};
  std::string buf{StringUtils::Format("BDMV/PLAYLIST/{:05}.mpls", playlist)};
  path.SetFileName(buf);
  item->SetPath(path.Get());

  CVideoInfoTag* itemTag{item->GetVideoInfoTag()};
  itemTag->SetDuration(duration);
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
  item->m_strTitle = buf;
  item->SetLabel(buf);

  item->SetLabel2(StringUtils::Format(
      g_localizeStrings.Get(25005) /* Title: {0:d} */ + " - {1:s}: {2:s}\n\r{3:s}: {4:s}", playlist,
      g_localizeStrings.Get(180) /* Duration */, StringUtils::SecondsToTimeString(duration),
      g_localizeStrings.Get(24026) /* Languages */, langs));
  item->m_dwSize = 0;
  item->SetArt("icon", "DefaultVideo.png");
}

void CDiscDirectoryHelper::EndPlaylistSearch()
{
  CLog::LogF(LOGDEBUG, "*** Episode Search End ***");
}

void CDiscDirectoryHelper::PopulateFileItems(const CURL& url,
                                             CFileItemList& items,
                                             int episodeIndex,
                                             const std::vector<CVideoInfoTag>& episodesOnDisc,
                                             const PlaylistMap& playlists)
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
    std::vector<SortedPlaylistsInformation> sortedPlaylists;
    sortedPlaylists.reserve(m_candidatePlaylists.size());
    for (const auto& [playlist, index] : m_candidatePlaylists)
    {
      if (!playlists.contains(playlist))
      {
        CLog::LogF(LOGERROR, "Playlist {} missing in playlist map", playlist);
        return;
      }
      sortedPlaylists.emplace_back(
          SortedPlaylistsInformation{.playlist = playlist,
                                     .index = index,
                                     .languages = playlists.find(playlist)->second.languages});
    }
    std::ranges::sort(sortedPlaylists,
                      [](const auto& i, const auto& j)
                      {
                        if (i.index == j.index)
                        {
                          if (i.languages.size() == j.languages.size())
                            return i.playlist < j.playlist;
                          return i.languages.size() > j.languages.size();
                        }
                        return i.index < j.index;
                      });

    for (const auto& [playlist, index, languages] : sortedPlaylists)
    {
      const auto newItem{std::make_shared<CFileItem>("", false)};
      GenerateItem(url, newItem, playlist, playlists, episodesOnDisc[index], IsSpecial::EPISODE);
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
