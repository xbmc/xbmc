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
#include <iomanip>
#include <iterator>
#include <ranges>
#include <set>
#include <sstream>
#include <tuple>

void CDiscDirectoryHelper::GetEpisodeTitles(const CURL& url,
                                            CFileItemList& items,
                                            int episodeIndex,
                                            const std::vector<CVideoInfoTag>& episodesOnDisc,
                                            const ClipMap& clips,
                                            const PlaylistMap& playlists)
{
  // Need to differentiate between specials and episodes
  bool allEpisodes{episodeIndex == -1};
  bool isSpecial{!allEpisodes && episodesOnDisc[episodeIndex].m_iSeason == 0};

  const unsigned int numEpisodes{static_cast<unsigned int>(std::ranges::count_if(
      episodesOnDisc, [](const CVideoInfoTag& e) { return e.m_iSeason > 0; }))};
  const unsigned int numSpecials{static_cast<unsigned int>(episodesOnDisc.size()) - numEpisodes};

  // If we are looking for a special then we want to find all episodes - to exclude them
  if (isSpecial && numEpisodes > 0)
    allEpisodes = true;

  CLog::LogF(LOGDEBUG, "*** Episode Search Start ***");

  if (!allEpisodes)
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

  // Look for a potential play all playlist (gives episode order)
  //
  // Assumptions
  //   1) Playlist clip count = number of episodes on disc (+2 for potential separate intro/end credits)
  //   2) Each clip will be in at least one other playlist (the individual episode playlist)
  //   3) Each clip (bar the last) will be at least MIN_EPISODE_LENGTH long
  //   4) Each potential individual episode playlist containing a clip from the potential play all playlist
  //      will have at most one other clip before/after

  std::vector<unsigned int> playAllPlaylists;
  std::map<unsigned int, std::map<unsigned int, std::vector<unsigned int>>> playAllPlaylistsMap;

  // Only look for play all playlists if enough playlists and more than one episode on disc
  if (numEpisodes > 1 && playlists.size() >= numEpisodes)
  {
    for (const auto& playlist : playlists)
    {
      const auto& [playlistNumber, playlistInformation] = playlist;

      // Find playlists that have a clip count = number of episodes on disc (+1/2) (1)
      if (playlistInformation.clips.size() >= numEpisodes &&
          playlistInformation.clips.size() <= numEpisodes + 2)
      {
        bool allClipsQualify{true};
        bool allowBeginningOrEnd{playlistInformation.clips.size() == numEpisodes + 1};
        const bool allowBeginningAndEnd{playlistInformation.clips.size() == numEpisodes + 2};

        // Map of clips in the play all playlist and the potential single episode playlists containing them
        std::map<unsigned int, std::vector<unsigned int>> playAllPlaylistClipMap;

        // For each clip in the playlist
        for (const auto& clip : playlistInformation.clips)
        {
          // Map of potential single episode playlists containing the clip
          std::vector<unsigned int> playAllPlaylistMap;

          // If clip doesn't appear in another playlist or clip is too short this is not a Play All playlist (2)(3)
          // BUT we allow first and/or last clip to be shorter (ie start intro/end credits) IF there are enough clips
          const ClipInformation& clipInformation{clips.find(clip)->second};
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
          playAllPlaylists.emplace_back(playlistNumber);
          playAllPlaylistsMap[playlistNumber] = playAllPlaylistClipMap;
        }
      }
    }
    if (playAllPlaylists.empty())
      CLog::LogF(LOGDEBUG, "No play all playlists found");
  }

  // Look for groups of playlists - consecutively numbered playlists where the number of playlists
  //   is at least the number of episodes on disc
  // First generate map of all playlists >= MIN_EPISODE_LENGTH and not a play all playlist
  // Then generate array of all consecutive groups of playlists

  // Get all playlists(s) >= MIN_EPISODE_LENGTH and not a play all playlist
  PlaylistMap longPlaylists;
  std::ranges::copy_if(playlists, std::inserter(longPlaylists, longPlaylists.end()),
                       [&playAllPlaylists](const PlaylistMapEntry& p)
                       {
                         const auto& [playlist, playlistInformation] = p;
                         return playlistInformation.duration >= MIN_EPISODE_DURATION &&
                                std::ranges::none_of(playAllPlaylists, [&playlist](const auto a)
                                                     { return playlist == a; });
                       });

  // Find groups
  std::vector<std::vector<unsigned int>> groups;
  if (numEpisodes > 1)
  {
    groups.emplace_back(1, longPlaylists.begin()->first);
    std::for_each(++longPlaylists.begin(), longPlaylists.end(),
                  [&groups](const PlaylistMapEntry& p)
                  {
                    const unsigned int playlist = p.first;
                    if (groups.back().back() == playlist - 1)
                      groups.back().emplace_back(playlist);
                    else
                      groups.emplace_back(1, playlist); // New group
                  });

    // Remove any groups containing fewer than numEpisodes playlists
    std::erase_if(groups, [&](const std::vector<unsigned int>& group)
                  { return group.size() < numEpisodes; });

    if (groups.empty())
      CLog::LogF(LOGDEBUG, "No playlist groups found");
    else
      for (const auto& group : groups)
        CLog::LogF(LOGDEBUG, "Playlist group found from {} to {}", group.front(), group.back());
  }
  else
  {
    CLog::LogF(LOGDEBUG, "No group search as single episode or specials only");
  }

  // At this stage we have a number of ways of trying to determine the correct playlist for an episode
  //
  // 1) Using a 'Play All' playlist, if present.
  //
  // 2a) Using the longest playlists that are consecutive, using the playlist in the nth position.
  //
  // 2b) For single episode discs, look for the longest playlist.
  //    There are some discs where there are extras that are longer than the episode itself, in n this case
  //    we look for the playlist with a common starting number (eg. 1, 800 etc..)
  //
  // Specials
  //
  // These are more difficult - as there may only be one per disc and we can't make assumptions about playlists.
  // So have to look on basis of duration alone.
  //

  std::map<unsigned int, unsigned int> candidatePlaylists;
  bool foundEpisode{false};
  bool findIdenticalEpisodes{false};
  std::vector<unsigned int> commonStartingPlaylists = {801, 800, 1, 811, 0};

  if (allEpisodes)
    CLog::LogF(LOGDEBUG, "Looking for all episodes on disc");

  // Method 1 - Play all playlist
  if (playAllPlaylists.size() == 1)
  {
    CLog::LogF(LOGDEBUG, "Using Play All playlist method");

    // Get the playlist
    const unsigned int playAllPlaylist{playAllPlaylists[0]};

    // Get the potential single episodes from the map
    const auto& [playlist, playlistInformation] = *playlists.find(playAllPlaylist);
    CLog::LogF(LOGDEBUG, "Using candidate play all playlist {} duration {}", playlist,
               playlistInformation.duration);

    // Find the clip for the episode(s)
    for (unsigned int i{0}; const auto& clip : playlistInformation.clips)
    {
      if (allEpisodes || i == static_cast<unsigned int>(episodeIndex))
      {
        CLog::LogF(LOGDEBUG, "Clip is {}", clip);

        // Find playlist(s) with that clip from map populated earlier
        const auto& singleEpisodePlaylists{playAllPlaylistsMap[playAllPlaylist].find(clip)->second};

        for (const auto& singleEpisodePlaylist : singleEpisodePlaylists)
        {
          // Get playlist information
          const PlaylistInformation& singleEpisodePlaylistInformation{
              playlists.find(singleEpisodePlaylist)->second};

          CLog::LogF(LOGDEBUG, "Candidate playlist {} duration {}", singleEpisodePlaylist,
                     singleEpisodePlaylistInformation.duration);

          candidatePlaylists.insert(
              {singleEpisodePlaylist, i}); // Also save episodeIndex for all episodes
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

    if (numEpisodes == 1)
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
      const auto [first, last] = std::ranges::unique(
          playlists_length, [](const PlaylistVectorEntry& i, const PlaylistVectorEntry& j)
          { return i.second.duration == j.second.duration; });
      playlists_length.erase(first, last);

      // See how many unique (different length) episode length(>= MIN_EPISODE_LENGTH) playlists there are
      unsigned int episodeLengthPlaylist{0};
      const auto episodeLengthPlaylists{
          std::ranges::count_if(playlists_length,
                                [&episodeLengthPlaylist](const PlaylistVectorEntry& p)
                                {
                                  const auto& [playlist, playlistInformation] = p;
                                  if (playlistInformation.duration >= MIN_EPISODE_DURATION)
                                  {
                                    episodeLengthPlaylist = playlist;
                                    return true;
                                  }
                                  return false;
                                })};

      // Look for common starting playlists (1, 800, 801, 811)
      unsigned int commonPlaylist{0};
      const bool foundCommonPlaylist{std::ranges::any_of(
          playlists,
          [&commonStartingPlaylists, &commonPlaylist](const PlaylistMapEntry& p)
          {
            const unsigned int playlist = p.first;
            return std::ranges::any_of(commonStartingPlaylists,
                                       [playlist, &commonPlaylist](const unsigned int c)
                                       {
                                         if (c == playlist)
                                         {
                                           commonPlaylist = playlist;
                                           return true;
                                         }
                                         return false;
                                       });
          })};

      if (episodeLengthPlaylists == 1)
      {
        // If only one long playlist, then assume it's that
        CLog::LogF(LOGDEBUG, "Single Episode - found using single long playlist method");
        CLog::LogF(LOGDEBUG, "Candidate playlist {}", episodeLengthPlaylist);
        candidatePlaylists.insert({episodeLengthPlaylist, episodeIndex});
        foundEpisode = true;
      }
      else if (foundCommonPlaylist)
      {
        // Found a common playlist, so assume it's that
        CLog::LogF(LOGDEBUG, "Single Episode - found using common playlist method");
        CLog::LogF(LOGDEBUG, "Candidate playlist {}", commonPlaylist);
        candidatePlaylists.insert({commonPlaylist, episodeIndex});
        foundEpisode = true;
      }
    }
    else
    {
      // Method 2a - More than one episode on disc

      // Use groups and find nth playlist (or all for all episodes) in group
      // Groups are already contain at least numEpisodes playlists of minimum duration
      if (!groups.empty())
      {
        for (const auto& group : groups)
        {
          for (unsigned int i = 0; i < numEpisodes; ++i)
          {
            if (allEpisodes || i == static_cast<unsigned int>(episodeIndex))
            {
              candidatePlaylists.insert({group[i], i}); // Also save episodeIndex for all episodes
              CLog::LogF(LOGDEBUG, "Candidate playlist {}", group[i]);
            }
          }
        }
        foundEpisode = true;
        findIdenticalEpisodes = true;
      }
    }
  }

  // Now deal with the possibility there may be more than one playlist (per episode)
  // For this, see which is closest in duration to the desired episode duration (from the scraper)
  if (candidatePlaylists.size() > 1)
  {
    // Rebuild candidatePlaylists
    const auto oldCandidatePlaylists = candidatePlaylists;
    candidatePlaylists.clear();

    // Loop through each episode (in case of all episodes)
    // Generate set of indexes of episode entry in episodesOnDisc
    std::set<unsigned int> indexes;
    for (const unsigned int index : oldCandidatePlaylists | std::views::values)
      indexes.insert(index);

    for (const unsigned int currentEpisodeIndex : indexes)
    {
      unsigned int duration{episodesOnDisc[currentEpisodeIndex].GetDuration()};
      if (duration == 0)
        duration = INT_MAX; // If episode length not known, ensure the longest playlist selected

      // Create vector of candidate playlists and duration difference
      std::vector<std::pair<unsigned int, unsigned int>> candidatePlaylistsDuration;
      candidatePlaylistsDuration.reserve(oldCandidatePlaylists.size());
      for (const auto& [playlist, index] : oldCandidatePlaylists)
        if (index == currentEpisodeIndex)
        {
          candidatePlaylistsDuration.emplace_back(
              playlist,
              abs(static_cast<int>(playlists.find(playlist)->second.duration - duration)));
        }

      // Sort descending based on relative difference in duration
      std::ranges::sort(candidatePlaylistsDuration,
                        [](const auto& i, const auto& j)
                        {
                          const auto& [i_playlist, i_delta] = i;
                          const auto& [j_playlist, j_delta] = j;
                          return i_delta < j_delta;
                        });

      // Keep the playlist with closest duration
      const unsigned int playlist{std::get<0>(candidatePlaylistsDuration[0])};
      candidatePlaylists.insert({playlist, currentEpisodeIndex});

      CLog::LogF(LOGDEBUG,
                 "Remaining candidate playlist (closest in duration) is {} for episode index {}",
                 playlist, episodeIndex);
    }
  }

  // candidatePlaylists should now (ideally) contain one candidate title for the episode or all episodes
  // Now look at durations of found playlist and add identical (in case language options)
  if (findIdenticalEpisodes && !candidatePlaylists.empty())
  {
    for (const auto& [candidatePlaylist, candidatePlaylistEpisodeIndex] : candidatePlaylists)
    {
      // Get candidatePlaylist clips and duration
      const auto& [candidatePlaylistNumber, candidatePlaylistInformation] =
          *playlists.find(candidatePlaylist);

      // Find all other playlists of same duration with same clips
      for (const auto& [playlist, playlistInformation] : playlists)
      {
        if (playlist != candidatePlaylist &&
            candidatePlaylistInformation.duration == playlistInformation.duration &&
            candidatePlaylistInformation.clips == playlistInformation.clips &&
            candidatePlaylistInformation.languages != playlistInformation.languages)
        {
          CLog::LogF(LOGDEBUG, "Adding playlist {} as same duration and clips as playlist {}",
                     playlist, candidatePlaylist);
          candidatePlaylists.insert({playlist, candidatePlaylistEpisodeIndex});
        }
      }
    }
  }

  // Find specials
  std::vector<unsigned int> candidateSpecials;
  if (numSpecials > 0)
  {
    // Remove episodes and short playlists
    PlaylistVector playlists_length;
    playlists_length.reserve(playlists.size());
    playlists_length.assign(playlists.begin(), playlists.end());
    if (numEpisodes > 0)
    {
      std::erase_if(playlists_length,
                    [&candidatePlaylists, &playAllPlaylists](const PlaylistVectorEntry& playlist)
                    {
                      return playlist.second.duration < MIN_SPECIAL_DURATION ||
                             std::ranges::any_of(candidatePlaylists,
                                                 [&playlist](const auto& candidatePlaylist)
                                                 {
                                                   const unsigned int candidatePlaylistNumber{
                                                       std::get<0>(candidatePlaylist)};
                                                   return playlist.first == candidatePlaylistNumber;
                                                 }) ||
                             std::ranges::any_of(playAllPlaylists,
                                                 [&playlist](const unsigned int playAllPlaylist)
                                                 { return playlist.first == playAllPlaylist; });
                    });
    }

    // Sort playlists by length
    std::ranges::sort(playlists_length,
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
    if (playlists_length.size() >= numSpecials)
      for (unsigned int playlist : playlists_length | std::views::keys)
        candidateSpecials.emplace_back(playlist);
  }

  CLog::LogF(LOGDEBUG, "*** Episode Search End ***");

  // Now populate CFileItemList to return
  //
  // For singe episodes - relevant playlists are in candidatePlaylists. Show then all
  // For all episodes   - relevant playlists are in candidatePlaylists and specials in candidateSpecials
  //                    - show all episodes and specials (with latter entitled 'Special')
  // For single special - relevant playlists are in candidateSpecials.
  //                    - if there is a single playlist then label that with special's title
  //                      otherwise label all playlists as 'Special' as cannot determine which it is

  CFileItemList newItems;
  if (!isSpecial)
  {
    // Sort by index, number of languages and playlist
    std::vector<std::tuple<unsigned int, unsigned int, std::string>> sortedPlaylists;
    sortedPlaylists.reserve(candidatePlaylists.size());
    for (const auto& [playlist, index] : candidatePlaylists)
      sortedPlaylists.emplace_back(playlist, index, playlists.find(playlist)->second.languages);
    std::ranges::sort(sortedPlaylists,
                      [](const auto& i, const auto& j)
                      {
                        const auto& [i_playlist, i_index, i_languages] = i;
                        const auto& [j_playlist, j_index, j_languages] = j;
                        if (i_index == j_index)
                        {
                          if (i_languages.size() == j_languages.size())
                            return i_playlist < j_playlist;
                          return i_languages.size() > j_languages.size();
                        }
                        return i_index < j_index;
                      });

    for (const auto& [playlist, index, languages] : sortedPlaylists)
    {
      const auto newItem{std::make_shared<CFileItem>("", false)};
      GenerateItem(url, newItem, playlist, playlists, episodesOnDisc[index]);
      items.Add(newItem);
    }
  }
  if (isSpecial || allEpisodes)
  {
    for (const auto& playlist : candidateSpecials)
    {
      const auto newItem{std::make_shared<CFileItem>("", false)};
      CVideoInfoTag tag;
      if (isSpecial && candidateSpecials.size() == 1)
        tag = episodesOnDisc[episodeIndex];
      GenerateItem(url, newItem, playlist, playlists, tag, true);
      items.Add(newItem);
    }
  }
}

void CDiscDirectoryHelper::GenerateItem(const CURL& url,
                                        const std::shared_ptr<CFileItem>& item,
                                        unsigned int playlist,
                                        const PlaylistMap& playlists,
                                        const CVideoInfoTag& tag,
                                        bool isSpecial /* = false */)
{
  // Get clips
  const auto& it{playlists.find(playlist)};
  const int duration{static_cast<int>(it->second.duration)};

  // Get languages
  const std::string langs{it->second.languages};

  CURL path{url};
  std::string buf{StringUtils::Format("BDMV/PLAYLIST/{:05}.mpls", playlist)};
  path.SetFileName(buf);
  item->SetPath(path.Get());

  item->GetVideoInfoTag()->SetDuration(duration);
  item->GetVideoInfoTag()->m_iTrack = static_cast<int>(playlist);

  // Get episode title
  const std::string& title{tag.GetTitle()};
  if (isSpecial)
    if (title.empty())
      buf = g_localizeStrings.Get(21348); /* Special */
    else
      buf = StringUtils::Format("{0:s} {1:d} - {2:s}", g_localizeStrings.Get(21348) /* Special */,
                                tag.m_iEpisode, tag.GetTitle());
  else
    buf = StringUtils::Format("{0:s} {1:d} - {2:s}", g_localizeStrings.Get(20359) /* Episode */,
                              tag.m_iEpisode, tag.GetTitle());
  item->m_strTitle = buf;
  item->SetLabel(buf);

  item->SetLabel2(StringUtils::Format(
      g_localizeStrings.Get(25005) /* Title: {0:d} */ + " - {1:s}: {2:s}\n\r{3:s}: {4:s}", playlist,
      g_localizeStrings.Get(180) /* Duration */, StringUtils::SecondsToTimeString(duration),
      g_localizeStrings.Get(24026) /* Languages */, langs));
  item->m_dwSize = 0;
  item->SetArt("icon", "DefaultVideo.png");
}

void CDiscDirectoryHelper::AddRootOptions(const CURL& url, CFileItemList& items, bool addMenuOption)
{
  CURL path{url};
  path.SetFileName(URIUtils::AddFileToFolder("root", "titles"));

  auto item{std::make_shared<CFileItem>(path.Get(), true)};
  item->SetLabel(g_localizeStrings.Get(25002) /* All titles */);
  item->SetArt("icon", "DefaultVideoPlaylists.png");
  items.Add(item);

  if (addMenuOption)
  {
    path.SetFileName("menu");
    item = {std::make_shared<CFileItem>(path.Get(), false)};
    item->SetLabel(g_localizeStrings.Get(25003) /* Menu */);
    item->SetArt("icon", "DefaultProgram.png");
    items.Add(item);
  }
}

std::string CDiscDirectoryHelper::HexToString(std::span<const uint8_t> buf, int count)
{
  std::stringstream ss;
  ss << std::hex << std::setw(count) << std::setfill('0');
  std::ranges::for_each(buf, [&](auto x) { ss << static_cast<int>(x); });
  return ss.str();
}

std::string CDiscDirectoryHelper::GetEpisodesLabel(CFileItem& newItem, const CFileItem& item)
{
  std::string label;

  // Get episodes on disc
  CVideoDatabase database;
  if (!database.Open())
  {
    CLog::LogF(LOGERROR, "Failed to open video database");
    return label;
  }

  std::vector<CVideoInfoTag> episodes;
  database.GetEpisodesByFileId(item.GetVideoInfoTag()->m_iFileId, episodes);
  if (!episodes.empty())
  {
    bool specials{false};
    int startEpisode{INT_MAX};
    int endEpisode{-1};
    for (const auto& episode : episodes)
    {
      if (episode.m_iSeason > 0 && episode.m_iEpisode < startEpisode)
        startEpisode = episode.m_iEpisode;
      if (episode.m_iSeason > 0 && episode.m_iEpisode > endEpisode)
        endEpisode = episode.m_iEpisode;
      if (episode.m_iSeason == 0)
        specials = true;
    }

    if (startEpisode == endEpisode && startEpisode != -1)
      label = StringUtils::Format(g_localizeStrings.Get(21349) /* Episode n */, startEpisode);
    else if (startEpisode < endEpisode)
    {
      label = StringUtils::Format(g_localizeStrings.Get(21350) /* Episodes m-n*/, startEpisode,
                                  endEpisode);

      // Get description of plot as more generic for multiple episodes
      newItem.GetVideoInfoTag()->m_strPlot =
          database.GetPlotByShowId(item.GetVideoInfoTag()->m_iIdShow);
    }

    if (specials)
    {
      if (!label.empty())
        label += g_localizeStrings.Get(21351); // and Specials
      else
        label = g_localizeStrings.Get(21352); // Specials
    }
  }
  return label;
}

std::vector<CVideoInfoTag> CDiscDirectoryHelper::GetEpisodesOnDisc(const CURL& url)
{
  CVideoDatabase database;
  if (!database.Open())
  {
    CLog::LogF(LOGERROR, "Failed to open video database");
    return {};
  }
  const std::string basePath{URIUtils::GetBlurayFile(url.Get())};
  std::vector<CVideoInfoTag> episodesOnDisc{};
  database.GetEpisodesByFile(basePath, episodesOnDisc);
  return episodesOnDisc;
}