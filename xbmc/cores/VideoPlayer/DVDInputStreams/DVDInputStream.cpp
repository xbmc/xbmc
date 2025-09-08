/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DVDInputStream.h"

#include "URL.h"
#include "cores/VideoPlayer/Interface/InputStreamConstants.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

#include <chrono>
#include <functional>
#include <ranges>
#include <string>
#include <vector>

using namespace std::chrono_literals;

CDVDInputStream::CDVDInputStream(DVDStreamType streamType, const CFileItem& fileitem)
{
  m_streamType = streamType;
  m_contentLookup = true;
  m_realtime = fileitem.GetProperty(STREAM_PROPERTY_ISREALTIMESTREAM).asBoolean(false);
  m_item = fileitem;
}

CDVDInputStream::~CDVDInputStream() = default;

bool CDVDInputStream::Open()
{
  m_content = m_item.GetMimeType();
  m_contentLookup = m_item.ContentLookup();
  return true;
}

void CDVDInputStream::Close()
{

}

std::string CDVDInputStream::GetFileName()
{
  CURL url(m_item.GetDynPath());

  url.SetProtocolOptions("");
  return url.Get();
}

CURL CDVDInputStream::GetURL()
{
  return m_item.GetDynURL();
}

void CDVDInputStream::SavePlaylistDetails(std::vector<PlaylistInformation>& playedPlaylists,
                                          std::chrono::steady_clock::time_point startTime,
                                          const PlaylistInformation& currentPlaylistInformation)
{
  // Details for this playlist/title
  const std::chrono::steady_clock::time_point timeNow{std::chrono::steady_clock::now()};
  const auto watchedTime{
      std::chrono::duration_cast<std::chrono::milliseconds>(timeNow - startTime)};

  // See if updating last playlist entry
  if (!playedPlaylists.empty() &&
      playedPlaylists.back().playlist == currentPlaylistInformation.playlist)
  {
    // Update last playlist entry
    auto& lastPlaylist = playedPlaylists.back();
    lastPlaylist.watchedTime += watchedTime;
    lastPlaylist.duration = currentPlaylistInformation.duration;
    lastPlaylist.details = currentPlaylistInformation.details;
    CLog::LogF(LOGDEBUG, "Updated playlist/title {} - watched time {} seconds",
               currentPlaylistInformation.playlist, lastPlaylist.watchedTime.count() / 1000);
  }
  else
  {
    // Update previous playlist with watched time
    if (!playedPlaylists.empty())
    {
      auto& lastPlaylist = playedPlaylists.back();
      lastPlaylist.watchedTime += watchedTime;
      CLog::LogF(LOGDEBUG, "Updated playlist/title {} - watched time {} seconds",
                 lastPlaylist.playlist, lastPlaylist.watchedTime.count() / 1000);
    }
    // New playlist
    playedPlaylists.emplace_back(currentPlaylistInformation);
    CLog::LogF(LOGDEBUG,
               "Playing playlist/title {} - menu {}, duration {} seconds, watched time {} seconds",
               currentPlaylistInformation.playlist, currentPlaylistInformation.inMenu,
               currentPlaylistInformation.duration.count() / 1000,
               currentPlaylistInformation.watchedTime.count() / 1000);
  }
}

CDVDInputStream::UpdateState CDVDInputStream::UpdatePlaylistDetails(
    DVDStreamType type,
    std::vector<PlaylistInformation>& playedPlaylists,
    CFileItem& item,
    double time,
    bool& closed)
{
  using enum UpdateState;
  if (playedPlaylists.empty())
  {
    CLog::LogF(LOGDEBUG, "Played playlists list is empty");
    return NOT_PLAYED;
  }

  // List playlists
  for (const auto& playlist : playedPlaylists)
    CLog::LogF(LOGDEBUG,
               "Playlist/title {} - menu {}, duration {} seconds, watched time {} seconds",
               playlist.playlist, playlist.inMenu, playlist.duration.count() / 1000,
               playlist.watchedTime.count() / 1000);

  // Save position of last playlist/title watched
  playedPlaylists.back().position = std::chrono::milliseconds(static_cast<int>(time));

  // With BD-J discs often everything played through the menu is indistinguishable from the menu itself
  // So all playlists could be tagged as inMenu or there may be some pre-menu playlists that aren't
  bool mainTitlePlayed{true};
  const auto& it{std::ranges::find_if(playedPlaylists, &PlaylistInformation::inMenu)};
  if (it != playedPlaylists.end())
  {
    // At least one playlist is menu
    // Now see if any non-menu playlists after first menu playlist
    const auto& it2{std::ranges::find_if(std::next(it), playedPlaylists.end(),
                                         std::not_fn(&PlaylistInformation::inMenu))};

    // If at least one playlist after first menu is not a menu then the main title will not be tagged as a menu
    if (it2 != playedPlaylists.end())
    {
      // Remove all non-menu playlists (if any) before the first menu playlist
      if (it != playedPlaylists.begin())
        playedPlaylists.erase(playedPlaylists.begin(), it);

      // Now remove all menu playlists, as the main item will not be tagged as menu
      std::erase_if(playedPlaylists, std::mem_fn(&PlaylistInformation::inMenu));
    }
    else
    {
      // Last playlist is a menu - see if it occurs more than once
      // (ie. menu -> main item -> back to menu -> stopped)
      // If so, remove that playlist
      const int playlist{playedPlaylists.back().playlist};
      const auto match{[playlist](const PlaylistInformation& p) { return p.playlist == playlist; }};
      if (std::ranges::count_if(playedPlaylists, match) > 1)
        std::erase_if(playedPlaylists, match); // Remove all matching menu playlists
      else
      {
        // If the last playlist a menu, and it's the only menu then assume main title not played
        if (std::next(it) == playedPlaylists.end() &&
            std::ranges::count_if(playedPlaylists, &PlaylistInformation::inMenu) == 1)
          mainTitlePlayed = false;
      }
    }
  }

  // Decide if main title ever played
  // 5 minutes was chosen to exclude short menu clips and trailers/warnings etc.
  static constexpr std::chrono::minutes MIN_PLAYLIST_DURATION{5min};
  if (mainTitlePlayed)
  {
    mainTitlePlayed = [&playedPlaylists]
    {
      if (playedPlaylists.empty())
        return false; // No playlists played
      if (std::ranges::none_of(playedPlaylists, [](const PlaylistInformation& p)
                               { return p.duration > MIN_PLAYLIST_DURATION; }))
        return false; // No playlists with sufficient duration
      return true;
    }();
  }

  if (!mainTitlePlayed)
  {
    item.GetVideoInfoTag()->m_streamDetails.Reset();
    item.SetDynPath("");
    item.SetProperty("no_main_title", true); // Not continuing to play if in stack
    CLog::LogF(LOGDEBUG, "No main title playlist played");
    return NOT_PLAYED;
  }

  // Find the playlist that was played the longest (of those that remain)
  auto filteredPlaylists{playedPlaylists |
                         std::views::filter([](const PlaylistInformation& p)
                                            { return p.duration > MIN_PLAYLIST_DURATION; })};
  const auto& it3{std::ranges::max_element(filteredPlaylists, std::less<>{},
                                           &PlaylistInformation::watchedTime)};

  // Consider if watched main playlist completely
  const int count{
      static_cast<int>(std::ranges::count_if(playedPlaylists, [&it3](const PlaylistInformation& p)
                                             { return p.playlist == it3->playlist; }))};
  constexpr std::chrono::seconds CLOSE_TO_END{5s};
  const bool stoppedBeforeEnd{
      [&]
      {
        const auto& endPlaylist{playedPlaylists.back()};
        if (count > 1)
          return false; // If watched main playlist more than once then assume finished (as some discs repeat automatically)
        if (endPlaylist.playlist != it3->playlist)
          return false; // If the main playlist is not the last playlist watched then assume main playlist watched completely
        if (endPlaylist.position == 0ms ||
            endPlaylist.duration - endPlaylist.position < CLOSE_TO_END)
          return false; // If within 5 seconds of the end then assume watched (as sometimes slight mismatch between time and total time
        return true;
      }()};

  if (stoppedBeforeEnd)
    item.SetProperty("stopped_before_end", true);
  else
    closed = false; // Feed back to VideoPlayer

  item.GetVideoInfoTag()->m_streamDetails = it3->details;
  const int playlist{it3->playlist};
  item.GetVideoInfoTag()->m_iTrack = playlist;
  CLog::LogF(LOGDEBUG, "Main playlist {}", playlist);

  // Update DynPath here as needed to save video settings
  if (type == DVDSTREAM_TYPE_BLURAY)
  {
    const std::string path{item.GetDynPath()};
    item.SetDynPath(URIUtils::GetBlurayPlaylistPath(path, playlist));
  }

  return stoppedBeforeEnd ? NONE : FINISHED;
}
