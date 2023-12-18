/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */
#include "BlurayDirectory.h"

#include "File.h"
#include "FileItem.h"
#include "LangInfo.h"
#include "URL.h"
#include "filesystem/BlurayCallback.h"
#include "filesystem/Directory.h"
#include "guilib/LocalizeStrings.h"
#include "utils/LangCodeExpander.h"
#include "utils/RegExp.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"
#include "video/VideoInfoTag.h"

#include <array>
#include <cassert>
#include <climits>
#include <memory>
#include <stdlib.h>
#include <string>

#include <libbluray/bluray-version.h>
#include <libbluray/bluray.h>
#include <libbluray/filesystem.h>
#include <libbluray/log_control.h>

namespace XFILE
{

#define MAIN_TITLE_LENGTH_PERCENT 70 /** Minimum length of main titles, based on longest title */

CBlurayDirectory::~CBlurayDirectory()
{
  Dispose();
}

void CBlurayDirectory::Dispose()
{
  if (m_bd)
  {
    bd_close(m_bd);
    m_bd = nullptr;
  }
}

std::string CBlurayDirectory::GetBlurayTitle()
{
  return GetDiscInfoString(DiscInfo::TITLE);
}

std::string CBlurayDirectory::GetBlurayID()
{
  return GetDiscInfoString(DiscInfo::ID);
}

std::string CBlurayDirectory::GetDiscInfoString(DiscInfo info)
{
  switch (info)
  {
    case XFILE::CBlurayDirectory::DiscInfo::TITLE:
    {
      if (!m_blurayInitialized)
        return "";
      const BLURAY_DISC_INFO* disc_info = bd_get_disc_info(m_bd);
      if (!disc_info || !disc_info->bluray_detected)
        return "";

      std::string title = "";

#if (BLURAY_VERSION > BLURAY_VERSION_CODE(1, 0, 0))
      title = disc_info->disc_name ? disc_info->disc_name : "";
#endif

      return title;
    }
    case XFILE::CBlurayDirectory::DiscInfo::ID:
    {
      if (!m_blurayInitialized)
        return "";

      const BLURAY_DISC_INFO* disc_info = bd_get_disc_info(m_bd);
      if (!disc_info || !disc_info->bluray_detected)
        return "";

      std::string id = "";

#if (BLURAY_VERSION > BLURAY_VERSION_CODE(1, 0, 0))
      id = disc_info->udf_volume_id ? disc_info->udf_volume_id : "";

      if (id.empty())
      {
        id = HexToString(disc_info->disc_id, 20);
      }
#endif

      return id;
    }
    default:
      break;
  }

  return "";
}

static constexpr unsigned int MIN_EPISODE_LENGTH = 15 * 60; // 15 minutes
static constexpr unsigned int MAX_EPISODE_DIFFERENCE = 30; // 30 seconds
static constexpr unsigned int MIN_SPECIAL_LENGTH = 5 * 60; // 5 minutes
static constexpr unsigned int PLAYLIST_CLIP_OFFSET =
    2; //First two entries in playlist array are playlist number and duration. Remaining entries are clip(s)
static constexpr unsigned int CLIP_PLAYLIST_OFFSET =
    2; // First two entries in clip array are clip number and duration. Remaining entries are playlist(s)

void CBlurayDirectory::GetPlaylistInfo(std::vector<std::vector<unsigned int>>& clips,
    std::vector<std::vector<unsigned int>>& playlists,
    std::map<unsigned int, std::string>& playlist_langs)
{
  // Get all titles on disc
  // Sort by playlist for grouping later

  CFileItemList allTitles;
  GetTitles(false, allTitles);
  SortDescription sorting;
  sorting.sortBy = SortByFile;
  allTitles.Sort(sorting);

  // Get information on all titles
  // Including relationship between clips and playlists
  // List all playlists

  CLog::LogF(LOGDEBUG, "Playlist information");
  for (const auto& title : allTitles)
  {
    CURL url(title->GetDynPath());
    std::string filename = URIUtils::GetFileName(url.GetFileName());
    unsigned int playlist;

    if (sscanf(filename.c_str(), "%05u.mpls", &playlist) == 1)
    {
      BLURAY_TITLE_INFO* titleInfo = bd_get_playlist_info(m_bd, playlist, 0);
      if (!titleInfo)
      {
        CLog::Log(LOGDEBUG, "Unable to get playlist {}", playlist);
      }
      else
      {
        // Save playlist
        auto pl = std::vector<unsigned int>{playlist};

        // Save playlist duration
        pl.emplace_back(titleInfo->duration / 90000);

        // Get clips
        std::string clipsStr;
        for (int i = 0; i < static_cast<int>(titleInfo->clip_count); ++i)
        {
          unsigned int clip;
          sscanf(titleInfo->clips[i].clip_id, "%u", &clip);

          // Add clip to playlist
          pl.emplace_back(clip);

          // Add/extend clip information
          const auto& it =
              std::find_if(clips.begin(), clips.end(),
                           [&](const std::vector<unsigned int>& x) { return x[0] == clip; });
          if (it == clips.end())
          {
            // First reference to clip
            unsigned int duration =
                ((titleInfo->clips[i].out_time - titleInfo->clips[i].in_time) / 90000);
            clips.emplace_back(std::vector<unsigned int>{clip, duration, playlist});
          }
          else
            // Additional reference to clip
            it->emplace_back(playlist);

          std::string c(reinterpret_cast<char const*>(titleInfo->clips[i].clip_id));
          clipsStr += c + ',';
        }
        if (!clipsStr.empty())
          clipsStr.pop_back();

        playlists.emplace_back(pl);

        // Get languages
        std::string langs;
        for (int i = 0; i < titleInfo->clips[0].audio_stream_count; ++i)
        {
          std::string l(reinterpret_cast<char const*>(titleInfo->clips[0].audio_streams[i].lang));
          langs += l + '/';
        }
        if (!langs.empty())
          langs.pop_back();

        playlist_langs[playlist] = langs;

        CLog::Log(LOGDEBUG, "Playlist {}, Duration {}, Langs {}, Clips {} ", playlist,
                  title->GetVideoInfoTag()->GetDuration(), langs, clipsStr);

        bd_free_title_info(titleInfo);
      }
    }
  }

  // Sort and list clip info

  std::sort(clips.begin(), clips.end(),
            [&](std::vector<unsigned int> i, std::vector<unsigned int> j)
            { return (i[0] < j[0]); });
  for (const auto& c : clips)
  {
    std::string ps(StringUtils::Format("Clip {0:d} duration {1:d} - playlists ", c[0], c[1]));
    for (int i = CLIP_PLAYLIST_OFFSET; i < static_cast<int>(c.size()); ++i)
    {
      ps += StringUtils::Format("{0:d},", c[i]);
    }
    ps.pop_back();
    CLog::Log(LOGDEBUG, ps);
  }
}

void CBlurayDirectory::GetEpisodeTitles(const CFileItem& episode,
                                        CFileItemList& items,
                                        std::vector<CVideoInfoTag> episodesOnDisc,
                                        const std::vector<std::vector<unsigned int>>& clips,
                                        const std::vector<std::vector<unsigned int>>& playlists,
                                        std::map<unsigned int, std::string>& playlist_langs)
{
  // Find our episode on disc
  // Need to differentiate between specials and episodes
  std::vector<CVideoInfoTag> specialsOnDisc;
  bool isSpecial = false;
  unsigned int episodeOffset = 0;

  for (int i = 0; i < static_cast<int>(episodesOnDisc.size()); ++i)
  {
    if (episodesOnDisc[i].m_iSeason > 0 &&
        episodesOnDisc[i].m_iSeason == episode.GetVideoInfoTag()->m_iSeason &&
        episodesOnDisc[i].m_iEpisode == episode.GetVideoInfoTag()->m_iEpisode)
    {
      // Episode found
      episodeOffset = i;
    }
    else if (episodesOnDisc[i].m_iSeason == 0)
    {
      // Special
      specialsOnDisc.emplace_back(episodesOnDisc[i]);

      if (episode.GetVideoInfoTag()->m_iSeason == 0 &&
          episodesOnDisc[i].m_iEpisode == episode.GetVideoInfoTag()->m_iEpisode)
      {
        // Sepcial found
        episodeOffset = specialsOnDisc.size() - 1;
        isSpecial = true;
      }

      // Remove from episode list
      episodesOnDisc.erase(episodesOnDisc.begin() + i);
      --i;
    }
  }

  const unsigned int numEpisodes = episodesOnDisc.size();
  const unsigned int episodeDuration = episode.GetVideoInfoTag()->GetDuration();

  CLog::LogF(LOGDEBUG, "*** Episode Search Start ***");

  CLog::Log(LOGDEBUG, "Looking for season {} episode {} duration {}",
            episode.GetVideoInfoTag()->m_iSeason, episode.GetVideoInfoTag()->m_iEpisode,
            episode.GetVideoInfoTag()->GetDuration());

  // List episodes expected on disc
  for (const auto& e : episodesOnDisc)
  {
    CLog::Log(LOGDEBUG, "On disc - season {} episode {} duration {}", e.m_iSeason, e.m_iEpisode,
              e.GetDuration());
  }
  for (const auto& e : specialsOnDisc)
  {
    CLog::Log(LOGDEBUG, "On disc - special - season {} episode {} duration {}", e.m_iSeason,
              e.m_iEpisode, e.GetDuration());
  }

  // Look for a potential play all playlist (can give episode order)
  // Asumptions
  //   1) PLaylist clip count = number of episodes on disc
  //   2) Each clip will be in at least one other playlist
  //   3) Each clip will be at least MIN_EPISODE_LENGTH long
  std::vector<unsigned int> playAllPlaylists;

  for (const auto& playlist : playlists)
  {
    // Find playlists that have a clip count = number of episodes on disc
    if (playlist.size() == numEpisodes + PLAYLIST_CLIP_OFFSET)
    {
      bool allClipsQualify = true;
      for (int i = PLAYLIST_CLIP_OFFSET; i < static_cast<int>(playlist.size()); ++i)
      {
        // Get clip information
        const auto& it =
            std::find_if(clips.begin(), clips.end(),
                         [&](const std::vector<unsigned int>& x) { return x[0] == playlist[i]; });
        const unsigned int duration = it->at(1);

        if (it->size() < (CLIP_PLAYLIST_OFFSET + 2) || duration < MIN_EPISODE_LENGTH)
        {
          // If clip only appears in one playlist or is too short - this is not a Play All playlist
          allClipsQualify = false;
          break;
        }
      }
      if (allClipsQualify)
      {
        CLog::Log(LOGDEBUG, "Potential play all playlist {}", playlist[0]);
        playAllPlaylists.emplace_back(playlist[0]);
      }
    }
  }

  // Look for groups of playlists
  unsigned int length = 1;
  std::vector<std::pair<int, int>> groups;
  const unsigned int n = playlists.size();

  // Only look for groups if enough playlists and more than one episode on disc
  if (n >= numEpisodes && numEpisodes > 1)
  {
    for (unsigned int i = 1; i <= n; ++i)
    {
      const int curPlaylist = (i == n) ? 0 : playlists[i][0]; // Ensure last group is recognised
      const int prevPlaylist = playlists[i - 1][0];
      if (i == n || (curPlaylist - prevPlaylist) != 1)
      {
        if (length == numEpisodes)
        {
          // Found a group of consecutive playlists of at least the number of episodes on disc
          const int firstPlaylist = playlists[i - length][0];
          groups.emplace_back(std::pair<int, int>(firstPlaylist, prevPlaylist));
          CLog::Log(LOGDEBUG, "Playlist group found from {} to {}", firstPlaylist, prevPlaylist);
        }
        length = 1;
      }
      else
      {
        length++;
      }
    }
  }

  // At this stage we have four ways of trying to determine the correct playlist for an episode
  //
  // 1) Using a 'Play All' playlist
  //    - Caveat: there may not be one
  //
  // 2) Using the longest (non-'Play All') playlists that are consecutive
  //    - Caveat: assumes play-all detection has worked and playlists are consecutive (hopefully reasonable assumptions)
  //
  // 3) Playlists that are +/- 30sec of the episode length that is passed to us
  //    - Caveat: the episode length may be wrong - either from scraper/NFO or from bug in Kodi that overwrites episode streamdetails on same disc
  //
  // 4) Using position in a group of adjacent playlists (see below)
  //    - Cavet: won't work for single episode discs
  //
  // Order of preference:
  //
  // 1) Use 'Play All' playlist - take the nth clip and find a playlist that plays that clip
  //
  // 2) Take the nth playlist of a consecutive run of longest playlists
  //
  // 3) Refine a group using relative position of playlist found on basis of length
  //    - this tries to account for episodes that have the wrong duration passed
  //    eg. if we have a disc with episodes 4,5,6 we would expect episode 5 to be the middle playlist of a group of 3 consecutive playlists (eg. 801, 802, 803)
  //
  // 4) Playlist based on episode length alone (assumes no groups found or 2) and 3) failed)
  //
  // 5) Look at groups found (ignoring length) and pick the relevant playlist - eg. the middle one for episode 5 using the exampe above
  //    - Only looks at playlists > MIN_EPISODE_LENGTH
  //
  // SPECIALS
  //
  // These are more difficult - as there may only be one per disc and we can't make assumptions about playlists.
  // So have to look on basis of duration alone.
  //

  std::vector<unsigned int> candidatePlaylists;
  bool foundEpisode = false;
  bool findIdenticalEpisodes = false;

  if (playAllPlaylists.size() == 1 && !isSpecial)
  {
    CLog::Log(LOGDEBUG, "Using Play All playlist method");

    // Get the relevant clip
    const auto& it = std::find_if(playlists.begin(), playlists.end(),
                                  [&](const std::vector<unsigned int>& x)
                                  { return x[0] == playAllPlaylists[0]; });
    const unsigned int clip = it->at(PLAYLIST_CLIP_OFFSET + episodeOffset);
    CLog::Log(LOGDEBUG, "Clip is {}", clip);

    // Find playlist with just that clip
    const auto& it2 = std::find_if(
        playlists.begin(), playlists.end(),
        [&](const std::vector<unsigned int>& x)
        { return (x.size() == (PLAYLIST_CLIP_OFFSET + 1) && x[PLAYLIST_CLIP_OFFSET] == clip); });
    const unsigned int playlist = it2->at(0);
    const unsigned int duration = it2->at(1);

    CLog::Log(LOGDEBUG, "Candidate playlist {} duration {}", playlist, duration);

    candidatePlaylists.emplace_back(playlist);

    foundEpisode = true;
    findIdenticalEpisodes = true;
  }

  // Look for the longest (non-'Play All') playlists

  if (!foundEpisode)
  {
    CLog::Log(LOGDEBUG, "Using longest playlists method");
    std::vector<unsigned int> longPlaylists;

    // Sort playlists by length
    std::vector<std::vector<unsigned int>> playlists_length(playlists);
    std::sort(playlists_length.begin(), playlists_length.end(),
              [&](std::vector<unsigned int> i, std::vector<unsigned int> j)
              { return (i[1] > j[1]); });

    // Remove duplicate lengths
    for (int i = 0; i < static_cast<int>(playlists_length.size()) - 1; ++i)
    {
      if (playlists_length[i][1] == playlists_length[i + 1][1])
      {
        playlists_length.erase(playlists_length.begin() + (i + 1));
        --i;
      }
    }

    unsigned int foundPlaylists = 0;
    for (const auto& playlist : playlists_length)
    {
      // Check not a 'Play All' playlist
      const auto& it = std::find_if(playAllPlaylists.begin(), playAllPlaylists.end(),
                                    [&](const unsigned int& x) { return x == playlist[0]; });
      if (it == playAllPlaylists.end() && playlist[1] >= MIN_EPISODE_LENGTH &&
          foundPlaylists < numEpisodes)
      {
        foundPlaylists += 1;
        longPlaylists.emplace_back(playlist[0]);
        CLog::Log(LOGDEBUG, "Long playlist {} duration {}", playlist[0], playlist[1]);
      }
    }

    if (foundPlaylists > 0 && foundPlaylists == numEpisodes && !isSpecial)
    {
      // Sort found playlists
      std::sort(longPlaylists.begin(), longPlaylists.end(),
                [&](unsigned int i, unsigned int j) { return (i < j); });

      // Ensure sequential
      if (longPlaylists[0] + numEpisodes - 1 == longPlaylists[numEpisodes - 1])
      {
        // Now select the nth one
        candidatePlaylists.emplace_back(longPlaylists[episodeOffset]);

        CLog::Log(LOGDEBUG, "Candidate playlist {}", longPlaylists[episodeOffset]);

        foundEpisode = true;
        findIdenticalEpisodes = true;
      }
    }

    if (isSpecial)
    {
      // Assume specials are longest playlists that are not (assumed) episodes or play-all lists
      for (const auto& playlist : playlists_length)
      {
        // Check not a 'Play All' playlist
        const auto& it = std::find_if(playAllPlaylists.begin(), playAllPlaylists.end(),
                                      [&](const unsigned int& x) { return x == playlist[0]; });

        if (it == playAllPlaylists.end() && playlist[1] >= MIN_SPECIAL_LENGTH &&
            std::count(longPlaylists.begin(), longPlaylists.end(), playlist[0]) == 0)
        {
          // This will only work if one special on disc (otherwise no way of knowing lengths)
          if (specialsOnDisc.size() == 1)
          {
            candidatePlaylists.emplace_back(playlist[0]);

            CLog::Log(LOGDEBUG, "Candidate special playlist {}", playlist[0]);

            foundEpisode = true;
          }
        }
      }
    }
  }

  // See if we can find titles of similar length (+/- MAX_EPISODE_DIFFERENCE sec) to the desired episode or special

  if (!foundEpisode)
  {
    CLog::Log(LOGDEBUG, "Using episode length method");
    for (const auto& playlist : playlists)
    {
      const unsigned int titleDuration = playlist[1];
      if (episodeDuration > (titleDuration - MAX_EPISODE_DIFFERENCE) &&
          episodeDuration < (titleDuration + MAX_EPISODE_DIFFERENCE))
      {
        // episode candidate found (on basis of duration)
        candidatePlaylists.emplace_back(playlist[0]);

        CLog::Log(LOGDEBUG, "Candidate playlist {} - actual duration {}, desired duration {}",
                  playlist[0], titleDuration, episodeDuration);
      }
    }

    if (!groups.empty() && !isSpecial)
    {
      // Found candidate groupings of playlists matching the number of episodes on disc
      // This assumes that the episodes have sequential playlist numbers

      // Firstly, cross-referece with duration based approach above
      // ie. look for episodes of correct approx duration in correct position in group of playlists
      // (titleCandidates already contains playlists of approx duration)

      CLog::Log(LOGDEBUG, "Refining candidate titles using groups");
      for (int i = 0; i < static_cast<int>(candidatePlaylists.size()); ++i)
      {
        const unsigned int playlist = candidatePlaylists[i];
        bool remove = true;
        for (const auto& group : groups)
        {
          if (playlist == group.first + episodeOffset)
          {
            CLog::Log(LOGDEBUG, "Candidate {} kept as in position {} in group", playlist,
                      episodeOffset + 1);
            remove = false;
            break;
          }
        }
        if (remove)
        {
          CLog::Log(LOGDEBUG, "Removed candidate {} as not in position {} in group", playlist,
                    episodeOffset + 1);
          candidatePlaylists.erase(candidatePlaylists.begin() + i);
          --i;
        }
      }
    }

    // candidatePlaylists now contains playlists of the approx duration, refined by group position (if there were groups)
    // If found nothing then it could be duration is wrong (ie from scraper)
    // In which case favour groups starting with common start points (eg. 1, 800, 801)

    if (candidatePlaylists.empty() && !isSpecial)
    {
      CLog::Log(LOGDEBUG, "Using find playlist using candidate groups alone method");
      std::vector<unsigned int> candidateGroups = {801, 800, 1};

      for (const auto& candidateGroup : candidateGroups)
      {
        bool foundFirst = false;
        for (const auto& playlist : playlists)
        {
          if (playlist[0] == candidateGroup)
          {
            // Need to make sure the beginning of the candidate group is present
            // Otherwise a group starting at 802 and containig 803 would be found, whereas the intention would be a group starting with 800
            // (or whatever candidateGroup is)
            CLog::Log(LOGDEBUG, "Potential candidate group start at playlist {}", candidateGroup);
            foundFirst = true;
          }

          unsigned int duration = playlist[1];
          if (foundFirst && playlist[0] == candidateGroup + episodeOffset &&
              duration >= MIN_EPISODE_LENGTH)
          {
            CLog::Log(LOGDEBUG, "Candidate playlist {} duration {}", candidateGroup + episodeOffset,
                      duration);
            candidatePlaylists.emplace_back(playlist[0]);
            findIdenticalEpisodes = true;
          }
        }
      }
    }
  }

  // candidatePlaylists should now (ideally) contain one or more candidate titles for the episode
  // Now look at durations of found playlist and add identical (in case language options)
  // Note this has already happend with the episode duration method

  if (findIdenticalEpisodes && candidatePlaylists.size() == 1)
  {
    // Find candidatePlaylist duration
    const auto& it = std::find_if(playlists.begin(), playlists.end(),
                                  [&](const std::vector<unsigned int>& x)
                                  { return x[0] == candidatePlaylists[0]; });
    const unsigned int candidatePlaylistDuration = it->at(1);

    // Look for other playlists of same duration
    for (const auto& playlist : playlists)
    {
      if (candidatePlaylists[0] != playlist[0] && candidatePlaylistDuration == playlist[1])
      {
        CLog::Log(LOGDEBUG, "Adding playlist {} as same duration as playlist {}", playlist[0],
                  candidatePlaylists[0]);
        candidatePlaylists.emplace_back(playlist[0]);
      }
    }
  }

  // Remove duplicates (ie. those that play exactly the same clip with same languages)

  if (candidatePlaylists.size() > 1)
  {
    for (int i = 0; i < static_cast<int>(candidatePlaylists.size()) - 1; ++i)
    {
      const auto& it = std::find_if(playlists.begin(), playlists.end(),
                                    [&](const std::vector<unsigned int>& x)
                                    { return x[0] == candidatePlaylists[i]; });

      for (int j = i + 1; j < static_cast<int>(candidatePlaylists.size()); ++j)
      {
        const auto& it2 = std::find_if(playlists.begin(), playlists.end(),
                                       [&](const std::vector<unsigned int>& x)
                                       { return x[0] == candidatePlaylists[j]; });

        if (std::equal(it->begin() + PLAYLIST_CLIP_OFFSET, it->end(),
                       it2->begin() + PLAYLIST_CLIP_OFFSET))
        {
          // Clips are the same so check languages
          if (playlist_langs[candidatePlaylists[i]] == playlist_langs[candidatePlaylists[j]])
          {
            // Remove duplicate
            CLog::Log(LOGDEBUG, "Removing duplicate playlist {}", candidatePlaylists[j]);
            candidatePlaylists.erase(candidatePlaylists.begin() + j);
            --j;
          }
        }
      }
    }
  }

  CLog::LogF(LOGDEBUG, "*** Episode Search End ***");

  // ** Now populate CFileItemList to return
  CFileItemList newItems;
  for (const auto& playlist : candidatePlaylists)
  {
    const auto newItem{std::make_shared<CFileItem>("", false)};

    // Get clips
    const auto& it2 =
        std::find_if(playlists.begin(), playlists.end(),
                     [&](const std::vector<unsigned int>& x) { return x[0] == playlist; });
    const int duration = it2->at(1);

    std::string clips;
    for (int i = CLIP_PLAYLIST_OFFSET; i < static_cast<int>(it2->size()); ++i)
    {
      clips += std::to_string(it2->at(i)) + ',';
    }
    if (!clips.empty())
      clips.pop_back();

    // Get languages
    std::string langs = playlist_langs[playlist];

    std::string buf;
    CURL path(m_url);
    buf = StringUtils::Format("BDMV/PLAYLIST/{:05}.mpls", playlist);
    path.SetFileName(buf);
    newItem->SetPath(path.Get());

    newItem->GetVideoInfoTag()->SetDuration(duration);
    newItem->GetVideoInfoTag()->m_iTrack = playlist;
    buf = StringUtils::Format("{0:s} {1:d} - {2:s}", g_localizeStrings.Get(20359) /* Episode */,
                              episode.GetVideoInfoTag()->m_iEpisode,
                              episode.GetVideoInfoTag()->m_strTitle);
    newItem->m_strTitle = buf;
    newItem->SetLabel(buf);
    newItem->SetLabel2(StringUtils::Format(
        g_localizeStrings.Get(25005) /* Title: {0:d} */ + " - {1:s}: {2:s}\n\r{3:s}: {4:s}", playlist,
        g_localizeStrings.Get(180) /* Duration */, StringUtils::SecondsToTimeString(duration),
        g_localizeStrings.Get(24026) /* Languages */, langs));
    newItem->m_dwSize = 0;
    newItem->SetArt("icon", "DefaultVideo.png");
    items.Add(newItem);
  }
}

std::shared_ptr<CFileItem> CBlurayDirectory::GetTitle(const BLURAY_TITLE_INFO* title,
                                                      const std::string& label)
{
  std::string buf;
  std::string chap;
  CFileItemPtr item(new CFileItem("", false));
  CURL path(m_url);
  buf = StringUtils::Format("BDMV/PLAYLIST/{:05}.mpls", title->playlist);
  path.SetFileName(buf);
  item->SetPath(path.Get());
  int duration = (int)(title->duration / 90000);
  item->GetVideoInfoTag()->SetDuration(duration);
  item->GetVideoInfoTag()->m_iTrack = title->playlist;
  buf = StringUtils::Format(label, title->playlist);
  item->m_strTitle = buf;
  item->SetLabel(buf);
  chap = StringUtils::Format(g_localizeStrings.Get(25007), title->chapter_count,
                             StringUtils::SecondsToTimeString(duration));
  item->SetLabel2(chap);
  item->m_dwSize = 0;
  item->SetArt("icon", "DefaultVideo.png");
  for (unsigned int i = 0; i < title->clip_count; ++i)
    item->m_dwSize += title->clips[i].pkt_count * 192;

  return item;
}

void CBlurayDirectory::GetTitles(bool main, CFileItemList &items)
{
  std::vector<BLURAY_TITLE_INFO*> titleList;
  uint64_t minDuration = 0;

  // Searching for a user provided list of playlists.
  if (main)
    titleList = GetUserPlaylists();

  if (!main || titleList.empty())
  {
    uint32_t numTitles = bd_get_titles(m_bd, TITLES_RELEVANT, 0);

    for (uint32_t i = 0; i < numTitles; i++)
    {
      BLURAY_TITLE_INFO* t = bd_get_title_info(m_bd, i, 0);

      if (!t)
      {
        CLog::Log(LOGDEBUG, "CBlurayDirectory - unable to get title {}", i);
        continue;
      }

      if (main && t->duration > minDuration)
          minDuration = t->duration;

      titleList.emplace_back(t);
    }
  }

  minDuration = minDuration * MAIN_TITLE_LENGTH_PERCENT / 100;

  for (auto& title : titleList)
  {
    if (title->duration < minDuration)
      continue;

    items.Add(GetTitle(title, main ? g_localizeStrings.Get(25004) /* Main Title */ : g_localizeStrings.Get(25005) /* Title */));
    bd_free_title_info(title);
  }
}

void CBlurayDirectory::GetRoot(CFileItemList& items)
{
  GetTitles(true, items);

  AddRootOptions(items);
}

void CBlurayDirectory::GetRoot(CFileItemList& items,
                               const CFileItem& episode,
                               const std::vector<CVideoInfoTag>& episodesOnDisc)
{
  // Get playlist, clip and language information
  std::vector<std::vector<unsigned int>> clips;
  std::vector<std::vector<unsigned int>> playlists;
  std::map<unsigned int, std::string> playlist_langs;

  GetPlaylistInfo(clips, playlists, playlist_langs);

  // Get episode playlists
  GetEpisodeTitles(episode, items, episodesOnDisc, clips, playlists, playlist_langs);

  if (!items.IsEmpty())
    AddRootOptions(items);
}

void CBlurayDirectory::AddRootOptions(CFileItemList& items) const
{
  CURL path(m_url);
  path.SetFileName(URIUtils::AddFileToFolder(m_url.GetFileName(), "titles"));

  auto item{std::make_shared<CFileItem>(path.Get(), true)};
  item->SetLabel(g_localizeStrings.Get(25002) /* All titles */);
  item->SetArt("icon", "DefaultVideoPlaylists.png");
  items.Add(item);

  const BLURAY_DISC_INFO* disc_info = bd_get_disc_info(m_bd);
  if (disc_info && disc_info->no_menu_support)
  {
    CLog::Log(LOGDEBUG, "CBlurayDirectory::GetRoot - no menu support, skipping menu entry");
    return;
  }

  path.SetFileName("menu");
  item = {std::make_shared<CFileItem>(path.Get(), false)};
  item->SetLabel(g_localizeStrings.Get(25003) /* Menus */);
  item->SetArt("icon", "DefaultProgram.png");
  items.Add(item);
}

bool CBlurayDirectory::GetDirectory(const CURL& url, CFileItemList &items)
{
  Dispose();
  m_url = url;
  std::string root = m_url.GetHostName();
  std::string file = m_url.GetFileName();
  URIUtils::RemoveSlashAtEnd(file);
  URIUtils::RemoveSlashAtEnd(root);

  if (!InitializeBluray(root))
    return false;

  if(file == "root")
    GetRoot(items);
  else if(file == "root/titles")
    GetTitles(false, items);
  else
  {
    CURL url2 = GetUnderlyingCURL(url);
    CDirectory::CHints hints;
    hints.flags = m_flags;
    if (!CDirectory::GetDirectory(url2, items, hints))
      return false;
  }

  items.AddSortMethod(SortByTrackNumber,  554, LABEL_MASKS("%L", "%D", "%L", ""));    // FileName, Duration | Foldername, empty
  items.AddSortMethod(SortBySize,         553, LABEL_MASKS("%L", "%I", "%L", "%I"));  // FileName, Size | Foldername, Size

  return true;
}

bool CBlurayDirectory::GetEpisodeDirectory(const CURL& url,
                                           const CFileItem& episode,
                                           CFileItemList& items,
                                           const std::vector<CVideoInfoTag>& episodesOnDisc)
{
  Dispose();
  m_url = url;
  std::string root = m_url.GetHostName();
  URIUtils::RemoveSlashAtEnd(root);

  if (!InitializeBluray(root))
    return false;

  GetRoot(items, episode, episodesOnDisc);

  items.AddSortMethod(SortByTrackNumber, 554,
                      LABEL_MASKS("%L", "%D", "%L", "")); // FileName, Duration | Foldername, empty
  items.AddSortMethod(SortBySize, 553,
                      LABEL_MASKS("%L", "%I", "%L", "%I")); // FileName, Size | Foldername, Size

  return true;
}

CURL CBlurayDirectory::GetUnderlyingCURL(const CURL& url)
{
  assert(url.IsProtocol("bluray"));
  std::string host = url.GetHostName();
  const std::string& filename = url.GetFileName();
  return CURL(host.append(filename));
}

bool CBlurayDirectory::InitializeBluray(const std::string &root)
{
  bd_set_debug_handler(CBlurayCallback::bluray_logger);
  bd_set_debug_mask(DBG_CRIT | DBG_BLURAY | DBG_NAV);

  m_bd = bd_init();

  if (!m_bd)
  {
    CLog::Log(LOGERROR, "CBlurayDirectory::InitializeBluray - failed to initialize libbluray");
    return false;
  }

  std::string langCode;
  g_LangCodeExpander.ConvertToISO6392T(g_langInfo.GetDVDMenuLanguage(), langCode);
  bd_set_player_setting_str(m_bd, BLURAY_PLAYER_SETTING_MENU_LANG, langCode.c_str());

  if (!bd_open_files(m_bd, const_cast<std::string*>(&root), CBlurayCallback::dir_open, CBlurayCallback::file_open))
  {
    CLog::Log(LOGERROR, "CBlurayDirectory::InitializeBluray - failed to open {}",
              CURL::GetRedacted(root));
    return false;
  }
  m_blurayInitialized = true;

  return true;
}

std::string CBlurayDirectory::HexToString(const uint8_t *buf, int count)
{
  std::array<char, 42> tmp;

  for (int i = 0; i < count; i++)
  {
    sprintf(tmp.data() + (i * 2), "%02x", buf[i]);
  }

  return std::string(std::begin(tmp), std::end(tmp));
}

std::vector<BLURAY_TITLE_INFO*> CBlurayDirectory::GetUserPlaylists()
{
  std::string root = m_url.GetHostName();
  std::string discInfPath = URIUtils::AddFileToFolder(root, "disc.inf");
  std::vector<BLURAY_TITLE_INFO*> userTitles;
  CFile file;
  char buffer[1025];

  if (file.Open(discInfPath))
  {
    CLog::Log(LOGDEBUG, "CBlurayDirectory::GetTitles - disc.inf found");

    CRegExp pl(true);
    if (!pl.RegComp("(\\d+)"))
    {
      file.Close();
      return userTitles;
    }

    uint8_t maxLines = 100;
    while ((maxLines > 0) && file.ReadString(buffer, 1024))
    {
      maxLines--;
      if (StringUtils::StartsWithNoCase(buffer, "playlists"))
      {
        int pos = 0;
        while ((pos = pl.RegFind(buffer, static_cast<unsigned int>(pos))) >= 0)
        {
          std::string playlist = pl.GetMatch(0);
          uint32_t len = static_cast<uint32_t>(playlist.length());

          if (len <= 5)
          {
            unsigned long int plNum = strtoul(playlist.c_str(), nullptr, 10);

            BLURAY_TITLE_INFO* t = bd_get_playlist_info(m_bd, static_cast<uint32_t>(plNum), 0);
            if (t)
              userTitles.emplace_back(t);
          }

          if (static_cast<int64_t>(pos) + static_cast<int64_t>(len) > INT_MAX)
            break;
          else
            pos += len;
        }
      }
    }
    file.Close();
  }
  return userTitles;
}

} /* namespace XFILE */
