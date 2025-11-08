/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ApplicationPlay.h"

#include "ApplicationStackHelper.h"
#include "FileItem.h"
#include "PlayListPlayer.h"
#include "ServiceBroker.h"
#include "ServiceManager.h"
#include "Util.h"
#include "cores/IPlayer.h"
#include "cores/playercorefactory/PlayerCoreFactory.h"
#include "dialogs/GUIDialogSimpleMenu.h"
#include "filesystem/DirectoryFactory.h"
#include "music/MusicFileItemClassify.h"
#include "playlists/PlayList.h"
#include "settings/AdvancedSettings.h"
#include "settings/DiscSettings.h"
#include "settings/MediaSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/DiscsUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"
#include "video/Bookmark.h"
#include "video/VideoDatabase.h"
#include "video/VideoFileItemClassify.h"
#include "video/VideoInfoTag.h"

#include <memory>
#include <string>
#include <utility>

using namespace KODI;
using namespace XFILE;

bool CApplicationPlay::ResolvePath(CFileItem& item)
{
  // Get bluray:// path for resolution if present
  if (item.HasVideoInfoTag() && item.GetVideoInfoTag()->GetPath().starts_with("bluray://"))
    item.SetDynPath(item.GetVideoInfoTag()->m_strFileNameAndPath);

  // Translate/Resolve the url if needed - recursively, but only limited times.
  std::string lastDynPath{item.GetDynPath()};
  size_t itemResolveAttempt{0};
  while (itemResolveAttempt < MAX_ITEM_RESOLVE_ATTEMPTS)
  {
    itemResolveAttempt++;

    const std::unique_ptr<IDirectory> dir{CDirectoryFactory::Create(item)};
    if (dir && !dir->Resolve(item))
    {
      CLog::LogF(LOGERROR, "Error resolving item. Item '{}' is not playable.", item.GetDynPath());
      return false;
    }

    std::string newDynPath{item.GetDynPath()};
    if (newDynPath == lastDynPath)
      break; // done

    lastDynPath = std::move(newDynPath);
  }
  return true;
}

bool CApplicationPlay::ResolveStack(CFileItem& item,
                                    std::string& player,
                                    const std::shared_ptr<CApplicationStackHelper>& stackHelper,
                                    bool& bRestart)
{
  // Particularly inefficient on startup as, if times are not saved in the database, each video
  // is opened in turn to determine its length. A faster calculation of video time would improve
  // this substantially.
  const auto startOffset{stackHelper->InitializeStackStartPartAndOffset(item)};
  if (!startOffset)
  {
    CLog::LogF(LOGERROR, "Failed to obtain start offset for stack {}. Aborting playback.",
               item.GetDynPath());
    return false;
  }
  const std::string savedPlayerState{item.GetProperty("savedplayerstate").asString("")};

  // Replace stack:// FileItem with the individual stack part FileItem
  item = stackHelper->GetCurrentStackPartFileItem();

  item.SetStartOffset(startOffset.value());
  if (!savedPlayerState.empty())
    item.SetProperty("savedplayerstate", savedPlayerState);

  player.clear();
  bRestart = true;
  return true;
}

namespace
{
bool GetEpisodeBookmark(const CFileItem& item, CPlayerOptions& options, CVideoDatabase& db)
{
  if (item.HasVideoInfoTag())
  {
    const CVideoInfoTag* tag{item.GetVideoInfoTag()};
    if (tag->m_iBookmarkId > 0)
    {
      CBookmark bookmark;
      db.GetBookMarkForEpisode(*tag, bookmark);
      options.starttime = bookmark.timeInSeconds;
      options.state = bookmark.playerState;
      return true;
    }
  }
  return false;
}
} // namespace

void CApplicationPlay::GetOptionsAndUpdateItem(
    CFileItem& item,
    CPlayerOptions& options,
    const std::shared_ptr<CApplicationStackHelper>& stackHelper,
    bool bRestart)
{
  if (item.HasProperty("StartPercent"))
  {
    options.startpercent = item.GetProperty("StartPercent").asDouble();
    item.SetStartOffset(0);
  }
  options.starttime = CUtil::ConvertMilliSecsToSecs(item.GetStartOffset());

  if (bRestart && item.HasVideoInfoTag())
    options.state = item.GetVideoInfoTag()->GetResumePoint().playerState;

  if (VIDEO::IsVideo(item) && (!bRestart || stackHelper->IsPlayingISOStack()))
  {
    if (item.HasProperty("savedplayerstate"))
    {
      // savedplayerstate is set in CPowerManager on sleep
      options.starttime = CUtil::ConvertMilliSecsToSecs(item.GetStartOffset());
      options.state = item.GetProperty("savedplayerstate").asString();
      item.ClearProperty("savedplayerstate");
      return;
    }

    CVideoDatabase db;
    if (!db.Open())
      return;

    // Get path
    std::string path{item.GetPath()};
    if (item.HasVideoInfoTag())
    {
      const std::string videoInfoTagPath{item.GetVideoInfoTag()->m_strFileNameAndPath};
      // removable:// may be embedded in bluray:// path
      if (CURL::Decode(videoInfoTagPath).find("removable://") != std::string::npos ||
          VIDEO::IsVideoDb(item))
        path = videoInfoTagPath;
    }
    else if (item.HasProperty("original_listitem_url") &&
             URIUtils::IsPlugin(item.GetProperty("original_listitem_url").asString()))
    {
      path = item.GetProperty("original_listitem_url").asString();
    }

    // Note that we need to load the tag from database also if the item already has a tag,
    // because for example the (full) video info for strm files will be loaded here.
    db.LoadVideoInfo(path, *item.GetVideoInfoTag());

    if (item.GetStartOffset() == STARTOFFSET_RESUME)
    {
      options.starttime = 0.0;

      // See if resume point is set in the item
      if (item.IsResumePointSet())
      {
        options.starttime = item.GetCurrentResumeTime();
        if (item.HasVideoInfoTag())
          options.state = item.GetVideoInfoTag()->GetResumePoint().playerState;
      }
      else
      {
        // See if there is resume point in the database
        CBookmark bookmark;
        if (db.GetResumeBookMark(path, bookmark))
        {
          options.starttime = bookmark.timeInSeconds;
          options.state = bookmark.playerState;
        }
        else if (options.starttime == 0.0 && item.HasVideoInfoTag())
          GetEpisodeBookmark(item, options, db);
      }
    }
    else if (item.HasVideoInfoTag())
      GetEpisodeBookmark(item, options, db);

    db.Close();
  }
}

bool CApplicationPlay::GetPlaylistIfDisc(CFileItem& item,
                                         CPlayerOptions& options,
                                         const std::string& player,
                                         const std::unique_ptr<CServiceManager>& serviceManager)
{
  // See if disc image is a Blu-ray (as an image could be a DVD as well) or if the path is a BDMV folder
  const bool isBluray{::UTILS::DISCS::IsBlurayDiscImage(item) ||
                      URIUtils::IsBDFile(item.GetDynPath())};

  // See if choose (new) playlist has been selected from context menu
  const bool forceSelection{item.GetProperty("force_playlist_selection").asBoolean(false)};
  const bool forceBlurayPlaylistSelection{forceSelection &&
                                          URIUtils::IsBlurayPath(item.GetDynPath())};

  if ((isBluray && !(options.startpercent > 0.0 || options.starttime > 0.0)) ||
      forceBlurayPlaylistSelection)
  {
    const bool isSimpleMenuAllowed{
        [&]()
        {
          const std::string defaultPlayer{
              player.empty() ? serviceManager->GetPlayerCoreFactory().GetDefaultPlayer(item)
                             : player};

          // No video selection when using external or remote players (they handle it if supported)
          const bool isExternalPlayer{
              serviceManager->GetPlayerCoreFactory().IsExternalPlayer(defaultPlayer)};
          const bool isRemotePlayer{
              serviceManager->GetPlayerCoreFactory().IsRemotePlayer(defaultPlayer)};

          // Check if simple menu is enabled or if we are forced to select a playlist
          const bool isSimpleMenu{forceSelection ||
                                  CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(
                                      CSettings::SETTING_DISC_PLAYBACK) == BD_PLAYBACK_SIMPLE_MENU};

          return !isExternalPlayer && !isRemotePlayer && isSimpleMenu;
        }()};

    if (isSimpleMenuAllowed)
    {
      if (!CGUIDialogSimpleMenu::ShowPlaylistSelection(item))
        return false;

      // Reset any resume state as new playlist chosen
      options.starttime = options.startpercent = 0.0;
      options.state = {};
      item.ClearProperty("force_playlist_selection");
    }
  }

  return true;
}

namespace
{
bool ShouldGoFullScreen(PlayMediaType mediaType)
{
  // Determine if we should go fullscreen based on the media type and settings
  if (const bool windowedStart{CMediaSettings::GetInstance().DoesMediaStartWindowed()};
      windowedStart)
    return false;

  const auto settings{CServiceBroker::GetSettingsComponent()};
  const bool fullScreenOnMovieStart{settings->GetAdvancedSettings()->m_fullScreenOnMovieStart};
  const bool hasPlayedFirstFile{CServiceBroker::GetPlaylistPlayer().HasPlayedFirstFile()};

  using enum PlayMediaType;
  switch (mediaType)
  {
    case VIDEO_PLAYLIST:
      return !hasPlayedFirstFile && fullScreenOnMovieStart;
    case MUSIC_PLAYLIST:
      return !hasPlayedFirstFile &&
             settings->GetSettings()->GetBool(CSettings::SETTING_MUSICFILES_SELECTACTION);
    case VIDEO:
      return fullScreenOnMovieStart;
  }

  return false; // should never reach here
}
} // namespace

void CApplicationPlay::DetermineFullScreen(
    const CFileItem& item,
    CPlayerOptions& options,
    const std::shared_ptr<CApplicationStackHelper>& stackHelper)
{
  // Get current playlist info
  const auto playlistId{CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist()};

  // Determine fullscreen status based on media type and playlist
  using enum PlayMediaType;
  if (MUSIC::IsAudio(item) && playlistId == PLAYLIST::Id::TYPE_MUSIC)
    options.fullscreen = ShouldGoFullScreen(MUSIC_PLAYLIST);
  else if (VIDEO::IsVideo(item) && playlistId == PLAYLIST::Id::TYPE_VIDEO &&
           CServiceBroker::GetPlaylistPlayer().GetPlaylist(playlistId).size() > 1)
  {
    options.fullscreen = ShouldGoFullScreen(VIDEO_PLAYLIST);
  }
  else if (stackHelper->IsPlayingRegularStack())
  {
    if (stackHelper->GetCurrentPartNumber() == 0 ||
        stackHelper->GetRegisteredStack(item)->GetStartOffset() != 0)
      options.fullscreen = ShouldGoFullScreen(VIDEO);
    else
      options.fullscreen = false;
  }
  else
    options.fullscreen = ShouldGoFullScreen(VIDEO);
}
