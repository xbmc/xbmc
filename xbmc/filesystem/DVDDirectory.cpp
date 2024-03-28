/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DVDDirectory.h"

#include "FileItem.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "cores/VideoPlayer/DVDInputStreams/DVDInputStreamNavigator.h"
#include "filesystem/Directory.h"
#include "guilib/LocalizeStrings.h"
#include "storage/MediaManager.h"
#include "utils/log.h"
#include "utils/URIUtils.h"

using namespace XFILE;

bool CDVDDirectory::Resolve(CFileItem& item) const
{
  const CURL url{item.GetDynPath()};
  if (url.GetProtocol() != "dvd")
  {
    return false;
  }

  item.SetDynPath(CServiceBroker::GetMediaManager().TranslateDevicePath(""));
  return true;
}

static constexpr unsigned int MIN_EPISODE_LENGTH = 15 * 60; // 15 minutes
static constexpr unsigned int MIN_CHAPTERS_PER_EPISODE = 3;
static constexpr unsigned int PLAYLIST_CHAPTER_OFFSET =
    2; //First two entries in playlist array are playlist number and duration. Remaining are chapter durations

void CDVDDirectory::GetEpisodeTitles(const CFileItem& episode,
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
  const bool allEpisodes = episode.m_allEpisodes;

  for (unsigned int i = 0; i < episodesOnDisc.size(); ++i)
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
        if (!allEpisodes)
          isSpecial = true;
      }

      // Remove from episode list
      episodesOnDisc.erase(episodesOnDisc.begin() + i);
      --i;
    }
  }

  const unsigned int numEpisodes = episodesOnDisc.size();

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
  //
  // Asumptions
  //   1) There will be consistency in chapter length between the play all playlist and episode playlist
  //
  // Consider chapter duration array - exclude first and last as they are more likely
  // to be episode specific (intro/trailers)
  // Look for titles that contain chapter duration array as a subset

  // Adjust array from absolute chapter offsets to lengths
  std::vector<std::vector<unsigned int>> playlists_length(playlists);
  for (unsigned int i = 0; i < playlists.size(); ++i)
    for (unsigned int j = playlists[i].size() - 2; j >= PLAYLIST_CHAPTER_OFFSET; --j)
      playlists_length[i][j + 1] -= playlists_length[i][j];

  // Remove zero length titles
  playlists_length.erase(std::remove_if(playlists_length.begin(), playlists_length.end(),
                                        [](const std::vector<unsigned int>& i)
                                        { return i[1] == 0; }),
                         playlists_length.end());

  // Remove zero length chapters from end
  for (auto playlist : playlists_length)
    while (playlist.back() == 0)
      playlist.pop_back();

  // Start with title with most chapters
  std::sort(playlists_length.begin(), playlists_length.end(),
            [](const std::vector<unsigned int>& i, const std::vector<unsigned int>& j)
            {
              if (i.size() == j.size())
                return i[1] > j[1]; // Duration
              return i.size() > j.size();
            });

  // playAllPlaylists[n][x,y] - n is the index
  //                            x is title
  //                            y is vector of pairs - first is title, second is chapter position in x's chapters
  std::vector<std::pair<unsigned int, std::vector<std::pair<unsigned int, unsigned int>>>>
      playAllPlaylists;

  // Only look for groups if enough playlists and more than one episode on disc
  if (playlists.size() >= numEpisodes && numEpisodes > 1)
  {
    // Find chapter matches
    for (unsigned int i = 0; i < playlists_length.size(); ++i)
    {
      unsigned int foundEpisodes{0};
      std::vector<std::pair<unsigned int, unsigned int>> titleOrder;
      for (unsigned int j = 0; j < playlists_length.size(); ++j)
      {
        // Play all playlist must have more chapters
        // Also check minimums
        if (i != j && playlists_length[i].size() > playlists_length[j].size() &&
            playlists_length[j].size() >= PLAYLIST_CHAPTER_OFFSET + MIN_CHAPTERS_PER_EPISODE &&
            playlists_length[j][1] >= MIN_EPISODE_LENGTH)
        {
          const auto& it =
              std::search(std::next(playlists_length[i].begin(), 2), playlists_length[i].end(),
                          std::next(playlists_length[j].begin(), 2), playlists_length[j].end());
          if (it != playlists_length[i].end())
          {
            foundEpisodes += 1;
            titleOrder.emplace_back(playlists_length[j][0],
                                    std::distance(playlists_length[i].begin(), it));

            // Overwrite elements associated with this episode
            // To prevent future false positives
            for (unsigned int k = 0; k < playlists_length[j].size() - PLAYLIST_CHAPTER_OFFSET; ++k)
              playlists_length[i][std::distance(playlists_length[i].begin(), it) + k] = 0;
          }
        }
      }
      CLog::LogF(LOGDEBUG, "Found {} episodes in playlist {}", foundEpisodes,
                 playlists_length[i][0]);
      if (foundEpisodes == numEpisodes)
      {
        CLog::LogF(LOGDEBUG, "Potential play all playlist {}", playlists_length[i][0]);

        playAllPlaylists.emplace_back(playlists_length[i][0], titleOrder);
      }
    }
  }

  std::vector<unsigned int> candidatePlaylists;
  bool foundEpisode = false;

  if (allEpisodes)
    CLog::Log(LOGDEBUG, "Looking for all episodes on disc");

  if (playAllPlaylists.size() == 1 && !isSpecial)
  {
    CLog::Log(LOGDEBUG, "Using Play All playlist method");

    // See where that title was found in the play all playlist
    std::vector<std::pair<unsigned int, unsigned int>> titleOrder{playAllPlaylists[0].second};
    std::sort(titleOrder.begin(), titleOrder.end(),
              [](const std::pair<unsigned int, unsigned int>& i,
                 const std::pair<unsigned int, unsigned int>& j) { return i.second < j.second; });

    // Get nth title number from play all playlist
    candidatePlaylists.emplace_back(titleOrder[episodeOffset].first);

    CLog::Log(LOGDEBUG, "Candidate playlist {}", titleOrder[episodeOffset].first);

    foundEpisode = true;
  }

  // If no play all playlist then look for n longest playlists and assume they are episodes
  if (!foundEpisode && numEpisodes > 1)
  {
    CLog::Log(LOGDEBUG, "Using longest playlists method");
    std::vector<unsigned int> longPlaylists;

    // Sort playlists by length
    std::sort(playlists_length.begin(), playlists_length.end(),
              [](const std::vector<unsigned int>& i, const std::vector<unsigned int>& j)
              {
                if (i[1] == j[1]) // Duration
                  return i[0] < j[0]; // Title
                return i[1] > j[1];
              });

    // See if the first n titles are longer than minimum length
    bool correctLength = true;
    for (unsigned int i = 0; i < numEpisodes; ++i)
      if (playlists_length[i][1] < MIN_EPISODE_LENGTH)
        correctLength = false;

    if (correctLength)
    {
      // If they are sequential then assume they are episodes (in order)
      std::vector<std::vector<unsigned int>> playlists_order{
          playlists_length.begin(), playlists_length.begin() + numEpisodes};
      std::sort(playlists_order.begin(), playlists_order.end(),
                [](const std::vector<unsigned int>& i, const std::vector<unsigned int>& j)
                {
                  return i[0] < j[0]; // Title
                });

      bool sequential = true;
      for (unsigned int i = 0; i < numEpisodes - 1; ++i)
        if (playlists_order[i + 1][0] != playlists_order[i][0] + 1)
          sequential = false;

      if (sequential)
        // Get the nth title
        candidatePlaylists.emplace_back(playlists_order[episodeOffset][0]);
    }
  }

  CLog::LogF(LOGDEBUG, "*** Episode Search End ***");

  // ** Now populate CFileItemList to return
  CFileItemList newItems;
  for (const auto& playlist : candidatePlaylists)
  {
    const auto newItem{std::make_shared<CFileItem>("", false)};

    // Get duration
    const auto& it =
        std::find_if(playlists.begin(), playlists.end(),
                     [&playlist](const std::vector<unsigned int>& x) { return x[0] == playlist; });
    const int duration = it->at(1);

    CURL path(m_url);
    std::string buf{StringUtils::Format("title/{}", playlist)};
    path.SetFileName(buf);
    newItem->SetPath(path.Get());

    newItem->GetVideoInfoTag()->SetDuration(duration);
    newItem->GetVideoInfoTag()->m_iTrack = playlist;

    // Get episode title
    buf = StringUtils::Format("{0:s} {1:d} - {2:s}", g_localizeStrings.Get(20359) /* Episode */,
                              episodesOnDisc[episodeOffset].m_iEpisode,
                              episodesOnDisc[episodeOffset].GetTitle());

    newItem->m_strTitle = buf;
    newItem->SetLabel(buf);
    newItem->SetLabel2(StringUtils::Format(
        g_localizeStrings.Get(25005) /* Title: {0:d} */ + " - {1:s}: {2:s}", playlist,
        g_localizeStrings.Get(180) /* Duration */, StringUtils::SecondsToTimeString(duration)));

    newItem->m_dwSize = 0;
    newItem->SetArt("icon", "DefaultVideo.png");
    items.Add(newItem);
  }
}

void CDVDDirectory::GetRoot(CFileItemList& items)
{
  GetTitles(true, items);

  AddRootOptions(items);
}

void CDVDDirectory::GetRoot(CFileItemList& items,
                            const CFileItem& episode,
                            const std::vector<CVideoInfoTag>& episodesOnDisc)
{
  CDVDInputStreamNavigator dvd{nullptr, episode};
  if (dvd.Open())
  {
    // Get playlist, clip and language information
    std::vector<std::vector<unsigned int>> clips;
    std::vector<std::vector<unsigned int>> playlists;
    std::map<unsigned int, std::string> playlist_langs;

    dvd.GetPlaylistInfo(clips, playlists, playlist_langs);

    // Get episode playlists
    GetEpisodeTitles(episode, items, episodesOnDisc, clips, playlists, playlist_langs);

    if (!items.IsEmpty())
      AddRootOptions(items);

    dvd.Close();
  }
}

void CDVDDirectory::AddRootOptions(CFileItemList& items) const
{
  CURL path(m_url);
  path.SetFileName(URIUtils::AddFileToFolder(m_url.GetFileName(), "titles"));

  auto item{std::make_shared<CFileItem>(path.Get(), true)};
  item->SetLabel(g_localizeStrings.Get(25002) /* All titles */);
  item->SetArt("icon", "DefaultVideoPlaylists.png");
  items.Add(item);

  path.SetFileName("menu");
  item = {std::make_shared<CFileItem>(path.Get(), false)};
  item->SetLabel(g_localizeStrings.Get(25015) /* Menu */);
  item->SetArt("icon", "DefaultProgram.png");
  items.Add(item);
}

bool CDVDDirectory::GetEpisodeDirectory(const CURL& url,
                                        const CFileItem& episode,
                                        CFileItemList& items,
                                        const std::vector<CVideoInfoTag>& episodesOnDisc)
{
  m_url = url;
  GetRoot(items, episode, episodesOnDisc);

  items.AddSortMethod(SortByTrackNumber, 554,
                      LABEL_MASKS("%L", "%D", "%L", "")); // FileName, Duration | Foldername, empty
  items.AddSortMethod(SortBySize, 553,
                      LABEL_MASKS("%L", "%I", "%L", "%I")); // FileName, Size | Foldername, Size

  return true;
}

bool CDVDDirectory::GetDirectory(const CURL& url, CFileItemList& items)
{
  m_url = url;
  std::string root = m_url.GetHostName();
  std::string file = m_url.GetFileName();
  URIUtils::RemoveSlashAtEnd(file);
  URIUtils::RemoveSlashAtEnd(root);

  if (file == "root")
    GetRoot(items);
  else if (file == "root/titles")
    GetTitles(false, items);
  else
  {
    CURL url2 = GetUnderlyingCURL(url);
    CDirectory::CHints hints;
    hints.flags = m_flags;
    if (!CDirectory::GetDirectory(url2, items, hints))
      return false;
  }

  items.AddSortMethod(SortByTrackNumber, 554,
                      LABEL_MASKS("%L", "%D", "%L", "")); // FileName, Duration | Foldername, empty
  items.AddSortMethod(SortBySize, 553,
                      LABEL_MASKS("%L", "%I", "%L", "%I")); // FileName, Size | Foldername, Size

  return true;
}

static constexpr unsigned int MAIN_TITLE_LENGTH_PERCENT = 70;

void CDVDDirectory::GetTitles(bool main, CFileItemList& items)
{
  CFileItem episode{m_url, false};
  CDVDInputStreamNavigator dvd{nullptr, episode};
  if (dvd.Open())
  {
    // Get playlist, clip and language information
    std::vector<std::vector<unsigned int>> clips;
    std::vector<std::vector<unsigned int>> playlists;
    std::map<unsigned int, std::string> playlist_langs;

    dvd.GetPlaylistInfo(clips, playlists, playlist_langs);

    // Remove zero length titles
    playlists.erase(std::remove_if(playlists.begin(), playlists.end(),
                                   [](const std::vector<unsigned int>& i) { return i[1] == 0; }),
                    playlists.end());

    // Get longest title and calculate minimum title length
    unsigned int minDuration{0};
    if (main)
    {
      for (const auto& playlist : playlists)
        if (playlist[1] > minDuration)
          minDuration = playlist[1];
    }
    minDuration = minDuration * MAIN_TITLE_LENGTH_PERCENT / 100;

    for (const auto& playlist : playlists)
    {
      const unsigned int duration = playlist[1];
      if (duration >= minDuration)
      {
        const auto newItem{std::make_shared<CFileItem>("", false)};

        CURL path(m_url);
        std::string buf{StringUtils::Format("title/{}", playlist[0])};
        path.SetFileName(buf);
        newItem->SetPath(path.Get());

        newItem->GetVideoInfoTag()->SetDuration(duration);
        newItem->GetVideoInfoTag()->m_iTrack = playlist[0];

        // Get episode title
        buf = StringUtils::Format(main ? g_localizeStrings.Get(25004) /* Main Title */
                                       : g_localizeStrings.Get(25005) /* Title */,
                                  playlist[0]);
        newItem->m_strTitle = buf;
        newItem->SetLabel(buf);

        buf = StringUtils::Format(g_localizeStrings.Get(25007),
                                  playlist.size() - PLAYLIST_CHAPTER_OFFSET,
                                  StringUtils::SecondsToTimeString(duration));
        newItem->SetLabel2(buf);

        newItem->m_dwSize = 0;
        newItem->SetArt("icon", "DefaultVideo.png");
        items.Add(newItem);
      }
    }
    dvd.Close();
  }
}

CURL CDVDDirectory::GetUnderlyingCURL(const CURL& url) const
{
  assert(url.IsProtocol("dvd"));
  std::string host = url.GetHostName();
  const std::string& filename = url.GetFileName();
  return CURL(host.append(filename));
}