/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DVDDirectory.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "cores/VideoPlayer/DVDInputStreams/DVDInputStreamNavigator.h"
#include "filesystem/Directory.h"
#include "guilib/LocalizeStrings.h"
#include "storage/MediaManager.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

using namespace XFILE;

static constexpr unsigned int MIN_EPISODE_LENGTH = 15 * 60; // 15 minutes
static constexpr unsigned int MIN_CHAPTERS_PER_EPISODE = 3;
static constexpr unsigned int MAX_CREDITS_LENGTH = 5 * 60; // 5 minutes
static constexpr int MAX_CHAPTER_LENGTH_DIFFERENCE = 2; // 2 seconds
static constexpr unsigned int PLAYLIST_CHAPTER_OFFSET =
    2; //First two entries in playlist array are playlist number and duration. Remaining are chapter durations

void CDVDDirectory::GetEpisodeTitles(const CFileItem& episode,
                                     CFileItemList& items,
                                     std::vector<CVideoInfoTag> episodesOnDisc,
                                     const std::vector<std::vector<unsigned int>>& clips,
                                     const std::vector<std::vector<unsigned int>>& playlists,
                                     std::map<unsigned int, std::string>& playlist_langs) const
{
  // Find our episode on disc
  // Need to differentiate between specials and episodes
  std::vector<CVideoInfoTag> specialsOnDisc;
  bool isSpecial = false;
  unsigned int episodeOffset = 0;
  const bool allEpisodes = episode.m_multipleTitles;

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
        // Special found
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

  CLog::LogF(LOGDEBUG, "Looking for season {} episode {} duration {}",
             episode.GetVideoInfoTag()->m_iSeason, episode.GetVideoInfoTag()->m_iEpisode,
             episode.GetVideoInfoTag()->GetDuration());

  // List episodes expected on disc
  for (const auto& e : episodesOnDisc)
  {
    CLog::LogF(LOGDEBUG, "On disc - season {} episode {} duration {}", e.m_iSeason, e.m_iEpisode,
               e.GetDuration());
  }
  for (const auto& e : specialsOnDisc)
  {
    CLog::LogF(LOGDEBUG, "On disc - special - season {} episode {} duration {}", e.m_iSeason,
               e.m_iEpisode, e.GetDuration());
  }

  // Look for a potential play all playlist (can give episode order)
  //
  // Assumptions
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
  for (auto& playlist : playlists_length)
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

  // Display
  for (const auto& playlist : playlists_length)
  {
    std::string chapterStr;
    for (uint32_t j = PLAYLIST_CHAPTER_OFFSET; j < playlist.size(); ++j)
      chapterStr += StringUtils::Format("{},", playlist[j]);
    chapterStr.pop_back();
    CLog::LogF(LOGDEBUG, "Sorted - Playlist {}, Duration {}, Chapters lengths {} ", playlist[0],
               playlist[1], chapterStr);
  }

  // playAllPlaylists[n][x,y] - n is the index
  //                            x is title number of the play all playlist
  //                            y is vector of tuple [a,b,c]
  //                              a is the title number of the episode (same as x for chapters)
  //                              b is the chapter position of the start of the episode in x/a
  //                              c the number of chapters in the episode
  std::vector<
      std::pair<unsigned int, std::vector<std::tuple<unsigned int, unsigned int, unsigned int>>>>
      playAllPlaylists;

  // Only look for groups if enough playlists and more than one episode on disc
  if (playlists.size() >= numEpisodes && numEpisodes > 1)
  {
    // Find chapter matches
    // By looking in every playlist to see if within it's chapter lengths it contains all the other playlists
    // chapter lengths
    for (unsigned int i = 0; i < playlists_length.size(); ++i)
    {
      unsigned int foundEpisodes{0};
      std::vector<std::tuple<unsigned int, unsigned int, unsigned int>> titleOrder;
      for (unsigned int j = 0; j < playlists_length.size(); ++j)
      {
        // Play all playlist must have more chapters
        // Also check minimums
        if (i != j && playlists_length[i].size() > playlists_length[j].size() &&
            ((playlists_length[j].size() >= PLAYLIST_CHAPTER_OFFSET + MIN_CHAPTERS_PER_EPISODE &&
              playlists_length[j][1] >= MIN_EPISODE_LENGTH)))
        {
          const auto& it =
              std::search(std::next(playlists_length[i].begin(), 2), playlists_length[i].end(),
                          std::next(playlists_length[j].begin(), 2), playlists_length[j].end(),
                          [](unsigned int k, unsigned int l) {
                            return abs(static_cast<int>(k) - static_cast<int>(l)) <=
                                   MAX_CHAPTER_LENGTH_DIFFERENCE;
                          });
          if (it != playlists_length[i].end())
          {
            foundEpisodes += 1;
            const unsigned int numChapters = playlists_length[j].size() - PLAYLIST_CHAPTER_OFFSET;
            titleOrder.emplace_back(
                playlists_length[j][0],
                static_cast<unsigned int>(std::distance(playlists_length[i].begin(), it)),
                numChapters);

            // Overwrite elements associated with this episode
            // To prevent future false positives
            for (unsigned int k = 0; k < numChapters; ++k)
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

    // If above hasn't found a play all playlist look on basis of playlist positions and a playlist that contains
    // a multiple of numEpisode chapters.
    if (playAllPlaylists.empty())
    {
      if (std::count_if(playlists.begin(), playlists.end(),
                        [numEpisodes](const auto& p) { return p[0] % numEpisodes == 0; }) == 1)
      {
        const auto& it =
            std::find_if(playlists.begin(), playlists.end(),
                         [numEpisodes](const auto& p) { return p[0] % numEpisodes == 0; });
        unsigned int title = playlists[std::distance(playlists.begin(), it)][0];

        std::vector<std::tuple<unsigned int, unsigned int, unsigned int>> titleOrder;
        if (title >= numEpisodes)
        {
          // Look before playlist
          unsigned int before_length{0};
          unsigned int chapters{0};
          for (unsigned int i = title - numEpisodes; i < title; ++i)
          {
            before_length += playlists[i][1];
            const unsigned int numChapters = playlists[i].size() - PLAYLIST_CHAPTER_OFFSET;
            titleOrder.emplace_back(playlists[i][0], chapters + PLAYLIST_CHAPTER_OFFSET,
                                    numChapters);
            chapters += numChapters;
          }

          // see if +/- 30 seconds of expected length and total number of chapters is multiple of number of episodes
          if ((playlists[title][1] >= before_length - 30 &&
               playlists[title][1] <= before_length + 30) &&
              chapters == playlists[title].size() - PLAYLIST_CHAPTER_OFFSET)
          {
            CLog::LogF(LOGDEBUG, "Potential play all playlist {} using relaxed criteria",
                       playlists[title][0]);
            playAllPlaylists.emplace_back(playlists[title][0], titleOrder);
          }
        }
        else if (title + numEpisodes <= playlists.size())
        {
          // Look after playlist
          unsigned int after_length{0};
          unsigned int chapters{0};

          for (unsigned int i = title + 1; i < title + numEpisodes; ++i)
          {
            after_length += playlists[i][1];
            const unsigned int numChapters = playlists[i].size() - PLAYLIST_CHAPTER_OFFSET;
            titleOrder.emplace_back(playlists[i][0], chapters + PLAYLIST_CHAPTER_OFFSET,
                                    numChapters);
            chapters += numChapters;
          }

          // see if +/- 30 seconds of expected length and total number of chapters is multiple of number of episodes
          if ((playlists[title][1] >= after_length - 30 &&
               playlists[title][1] <= after_length + 30) &&
              chapters == playlists[title].size() - PLAYLIST_CHAPTER_OFFSET)
          {
            CLog::LogF(LOGDEBUG, "Potential play all playlist {} using relaxed criteria",
                       playlists[title][0]);
            playAllPlaylists.emplace_back(playlists[title][0], titleOrder);
          }
        }
      }
    }
  }

  // Now see if one long playlist and try and look for patterns in its chapters
  bool chapters{false};
  if (numEpisodes > 1 && playAllPlaylists.empty())
  {
    CLog::LogF(LOGDEBUG, "Looking for chapter based play all playlist");
    for (unsigned int i = 0; i < playlists_length.size(); ++i)
    {
      // If chapters are a multiple of numEpisodes then assume equal division
      std::vector<std::tuple<unsigned int, unsigned int, unsigned int>> titleOrder;
      if ((playlists_length[i].size() - PLAYLIST_CHAPTER_OFFSET) % numEpisodes == 0)
      {
        unsigned int episode_chapter_length =
            (playlists_length[i].size() - PLAYLIST_CHAPTER_OFFSET) / numEpisodes;
        for (unsigned int j = 0; j < playlists_length[i].size() - PLAYLIST_CHAPTER_OFFSET;
             j += episode_chapter_length)
          titleOrder.emplace_back(playlists_length[i][0], j, episode_chapter_length);
        playAllPlaylists.emplace_back(playlists_length[i][0], titleOrder);
        chapters = true;
        CLog::LogF(LOGDEBUG,
                   "Single playlist {} with {} chapters found - assuming {} chapter per episode "
                   "for {} episodes",
                   playlists_length[i][0], playlists_length[i].size() - PLAYLIST_CHAPTER_OFFSET,
                   episode_chapter_length, numEpisodes);
      }
      else
      {
        // First make sure long enough (otherwise main part of episode will get confused with credits)
        if (playlists_length[i][1] >= MIN_EPISODE_LENGTH * numEpisodes)
        {
          // Look for short chapters that could represent beginning/end credits
          std::vector<unsigned int> shortChapters;
          for (unsigned int j = PLAYLIST_CHAPTER_OFFSET; j < playlists_length[i].size(); ++j)
            if (playlists_length[i][j] < MAX_CREDITS_LENGTH)
              shortChapters.emplace_back(j);

          // Except number of short chapters to be a multiple of numEpisodes
          // (ie. opening credits, closing credits, or both for each episode)
          if (!shortChapters.empty())
          {
            // Create an array of differences
            std::vector<unsigned int> differences;
            differences.reserve(shortChapters.size() - 1);
            for (unsigned int j = 0; j < shortChapters.size() - 1; ++j)
              differences.emplace_back(shortChapters[j + 1] - shortChapters[j]);

            // If the first entry in differences is 1 then there is an additional short first chapter
            // Remove it
            unsigned int firstChapterOffset{0};
            if (differences[0] == 1)
            {
              shortChapters.erase(shortChapters.begin());
              differences.erase(differences.begin());
              firstChapterOffset = 1;
            }

            // Likewise at the end
            if (differences.back() == 1)
            {
              shortChapters.pop_back();
              differences.pop_back();
            }

            // Spacing the same (ie. differences is filled with n, n, n, n, ...)
            if (std::adjacent_find(differences.begin(), differences.end(), std::not_equal_to()) ==
                differences.end())
            {
              // Get first short chapter
              unsigned int first{shortChapters[0]};
              unsigned int episode_chapter_length{differences[0]};
              if (first == PLAYLIST_CHAPTER_OFFSET)
                // Assume opening credits followed by episode
                for (unsigned int j = 0; j < shortChapters.size(); ++j)
                  titleOrder.emplace_back(playlists_length[i][0],
                                          (j * episode_chapter_length) + firstChapterOffset,
                                          episode_chapter_length);
              else
              {
                // Assume ending credits
                titleOrder.emplace_back(playlists_length[i][0], firstChapterOffset,
                                        episode_chapter_length);
                for (unsigned int j = 0; j < shortChapters.size() - 1; ++j)
                  titleOrder.emplace_back(playlists_length[i][0],
                                          (j * episode_chapter_length) + firstChapterOffset + 1,
                                          episode_chapter_length);
              }
              playAllPlaylists.emplace_back(playlists_length[i][0], titleOrder);
              chapters = true;
              CLog::LogF(
                  LOGDEBUG, "Single playlist (single credit per episode) {} with {} chapters found",
                  playlists_length[i][0], playlists_length[i].size() - PLAYLIST_CHAPTER_OFFSET);
            }
            else
            {
              // See if spacing is n, 1, n, 1, n, 1, ... , n
              // Which would suggest beginning and closing credits
              bool toggle = false;
              std::vector<unsigned int> episodes;
              std::vector<unsigned int> credits;
              std::partition_copy(differences.begin(), differences.end(),
                                  std::back_inserter(episodes), std::back_inserter(credits),
                                  [&toggle](int) { return toggle = !toggle; });

              if (episodes.size() == numEpisodes &&
                  std::adjacent_find(episodes.begin(), episodes.end(), std::not_equal_to()) ==
                      episodes.end() &&
                  credits.size() == numEpisodes - 1 && credits.front() == 1 &&
                  std::adjacent_find(credits.begin(), credits.end(), std::not_equal_to()) ==
                      credits.end())
              {
                unsigned int episode_chapter_length{episodes[0] + credits[0]};
                for (unsigned int j = 0; j < numEpisodes; ++j)
                  titleOrder.emplace_back(playlists_length[i][0],
                                          (j * episode_chapter_length) + firstChapterOffset,
                                          episode_chapter_length);
                playAllPlaylists.emplace_back(playlists_length[i][0], titleOrder);
                chapters = true;
                CLog::LogF(
                    LOGDEBUG, "Single playlist (two credits per episode) {} with {} chapters found",
                    playlists_length[i][0], playlists_length[i].size() - PLAYLIST_CHAPTER_OFFSET);
              }
            }
          }
        }
      }
    }
  }

  std::vector<std::pair<unsigned int, unsigned int>> candidatePlaylists;
  bool foundEpisode{chapters};

  if (allEpisodes)
    CLog::LogF(LOGDEBUG, "Looking for all episodes on disc");

  if (!foundEpisode && playAllPlaylists.size() == 1 && !isSpecial)
  {
    CLog::LogF(LOGDEBUG, "Using Play All playlist method");

    // See where that title was found in the play all playlist
    std::vector<std::tuple<unsigned int, unsigned int, unsigned int>> titleOrder{
        playAllPlaylists[0].second};
    std::sort(titleOrder.begin(), titleOrder.end(),
              [](const std::tuple<unsigned int, unsigned int, unsigned int>& i,
                 const std::tuple<unsigned int, unsigned int, unsigned int>& j)
              { return std::get<1>(i) < std::get<1>(j); });

    for (unsigned int i = 0; i < numEpisodes; ++i)
    {
      if (allEpisodes || i == episodeOffset)
      {
        // Get ith title number from play all playlist
        candidatePlaylists.emplace_back(std::get<0>(titleOrder[i]), i);

        CLog::LogF(LOGDEBUG, "Candidate playlist {}", std::get<0>(titleOrder[i]));
      }
    }
    foundEpisode = true;
  }

  // If no play all playlist then look for the n longest playlists and assume they are episodes
  // Favour long playlist method over chapter method (otherwise false positives)
  if ((!foundEpisode || (foundEpisode && chapters)) && playlists_length.size() >= numEpisodes)
  {
    CLog::LogF(LOGDEBUG, "Using longest playlists method");
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
      {
        for (unsigned int i = 0; i < numEpisodes; ++i)
        {
          if (allEpisodes || i == episodeOffset)
          {
            // Get ith title number from play all playlist
            candidatePlaylists.emplace_back(playlists_order[i][0], i);

            CLog::LogF(LOGDEBUG, "Candidate playlist {}", playlists_order[i][0]);
          }
        }
        foundEpisode = true;
        chapters = false;
      }
    }
  }

  CLog::LogF(LOGDEBUG, "*** Episode Search End ***");

  // ** Now populate CFileItemList to return
  CFileItemList newItems;

  if (chapters)
  {
    CURL path(m_url);
    for (unsigned int e = 0; e < numEpisodes; ++e)
    {
      if (allEpisodes || e == episodeOffset)
      {
        const auto newItem{std::make_shared<CFileItem>("", false)};

        // playAllPlaylists contains chapter references
        unsigned int playlist = playAllPlaylists[0].first;
        unsigned int chapter =
            std::get<1>(playAllPlaylists[0].second[e]) + 1; // stored as zero based
        unsigned int chapters = std::get<2>(playAllPlaylists[0].second[e]);

        std::string buf;
        if (chapters > 1)
          buf = StringUtils::Format("title/{}/chapters/{}-{}", playlist, chapter,
                                    chapter + chapters - 1);
        else
          buf = StringUtils::Format("title/{}/chapter/{}", playlist, chapter);
        path.SetFileName(buf);
        newItem->SetPath(path.Get());

        // Sum duration of all chapters in episode
        unsigned int duration{0};
        for (unsigned int i = 0; i < std::get<2>(playAllPlaylists[0].second[e]); ++i)
          duration += playlists_length[0][chapter + i + PLAYLIST_CHAPTER_OFFSET - 1];

        newItem->GetVideoInfoTag()->SetDuration(duration);
        newItem->SetProperty("chapter_duration", duration);
        newItem->GetVideoInfoTag()->m_iTrack = 1;

        // Get episode title
        buf = StringUtils::Format("{0:s} {1:d} - {2:s}", g_localizeStrings.Get(20359) /* Episode */,
                                  episodesOnDisc[e].m_iEpisode, episodesOnDisc[e].GetTitle());
        newItem->m_strTitle = buf;
        newItem->SetLabel(buf);
        newItem->SetLabel2(StringUtils::Format(
            g_localizeStrings.Get(21396) /* Chapter */ + ": {0:d} - {1:s}: {2:s}", chapter,
            g_localizeStrings.Get(180) /* Duration */, StringUtils::SecondsToTimeString(duration)));

        newItem->m_dwSize = 0;
        newItem->SetArt("icon", "DefaultVideo.png");
        items.Add(newItem);
      }
    }
  }
  else
  {
    for (const auto& playlist : candidatePlaylists)
    {
      const auto newItem{std::make_shared<CFileItem>("", false)};

      // Get duration
      const auto& it = std::find_if(playlists.begin(), playlists.end(),
                                    [&playlist](const std::vector<unsigned int>& x)
                                    { return x[0] == playlist.first; });
      const int duration = it->at(1);

      CURL path(m_url);
      std::string buf{StringUtils::Format("title/{}", playlist.first)};
      path.SetFileName(buf);
      newItem->SetPath(path.Get());

      newItem->GetVideoInfoTag()->SetDuration(duration);
      newItem->GetVideoInfoTag()->m_iTrack = playlist.first;

      // Get episode title
      buf = StringUtils::Format("{0:s} {1:d} - {2:s}", g_localizeStrings.Get(20359) /* Episode */,
                                episodesOnDisc[playlist.second].m_iEpisode,
                                episodesOnDisc[playlist.second].GetTitle());

      newItem->m_strTitle = buf;
      newItem->SetLabel(buf);
      newItem->SetLabel2(StringUtils::Format(
          g_localizeStrings.Get(25005) /* Title: {0:d} */ + " - {1:s}: {2:s}", playlist.first,
          g_localizeStrings.Get(180) /* Duration */, StringUtils::SecondsToTimeString(duration)));

      newItem->m_dwSize = 0;
      newItem->SetArt("icon", "DefaultVideo.png");
      items.Add(newItem);
    }
  }
}

void CDVDDirectory::GetRoot(CFileItemList& items) const
{
  GetTitles(GET_TITLES_MAIN, items);

  AddRootOptions(items);
}

void CDVDDirectory::GetRoot(CFileItemList& items,
                            const CFileItem& episode,
                            const std::vector<CVideoInfoTag>& episodesOnDisc) const
{
  if (CDVDInputStreamNavigator dvd{nullptr, episode}; dvd.Open())
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
  item->SetLabel(g_localizeStrings.Get(29806) /* Menu */);
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
  std::string file = m_url.GetFileName();
  URIUtils::RemoveSlashAtEnd(file);

  if (file == "root")
    GetRoot(items);
  else if (file == "root/titles")
    GetTitles(GET_TITLES_ALL, items);
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

void CDVDDirectory::GetTitles(const int job, CFileItemList& items) const
{
  const CFileItem episode{m_url, false};
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

    // Get the longest title and calculate minimum title length
    unsigned int minDuration{0};
    unsigned int maxPlaylist{0};
    if (job != GET_TITLES_ALL)
    {
      for (unsigned int i = 0; i < playlists.size(); ++i)
        if (playlists[i][1] > minDuration)
        {
          minDuration = playlists[i][1];
          maxPlaylist = i;
        }
    }
    minDuration = minDuration * MAIN_TITLE_LENGTH_PERCENT / 100;

    for (unsigned int i = 0; i < playlists.size(); ++i)
    {
      const unsigned int duration = playlists[i][1];
      if (job == GET_TITLES_ALL || (job == GET_TITLES_MAIN && duration >= minDuration) ||
          (job == GET_TITLES_ONE && i == maxPlaylist))
      {
        const auto newItem{std::make_shared<CFileItem>("", false)};

        CURL path(m_url);
        const unsigned int playlist{playlists[i][0]};
        std::string buf{StringUtils::Format("title/{}", playlist)};
        path.SetFileName(buf);
        newItem->SetPath(path.Get());

        newItem->GetVideoInfoTag()->SetDuration(duration);
        newItem->GetVideoInfoTag()->m_iTrack = playlist;

        // Get episode title
        buf = StringUtils::Format(g_localizeStrings.Get(25005) /* Title */, playlist);
        newItem->m_strTitle = buf;
        newItem->SetLabel(buf);

        buf = StringUtils::Format(g_localizeStrings.Get(25007),
                                  playlists[i].size() - PLAYLIST_CHAPTER_OFFSET,
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

bool CDVDDirectory::GetMainItem(const CURL& url, CFileItem& main)
{
  m_url = url;
  CFileItemList items;
  GetTitles(GET_TITLES_ONE, items);

  if (items.Size() == 1)
    main = *items[0];

  return true;
}

CURL CDVDDirectory::GetUnderlyingCURL(const CURL& url)
{
  assert(url.IsProtocol("dvd"));
  std::string host = url.GetHostName();
  const std::string& filename = url.GetFileName();
  return CURL(host.append(filename));
}