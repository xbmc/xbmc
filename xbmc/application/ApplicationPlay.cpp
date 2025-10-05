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
#include "cores/AudioEngine/Interfaces/AE.h"
#include "cores/playercorefactory/PlayerCoreFactory.h"
#include "dialogs/GUIDialogSimpleMenu.h"
#include "filesystem/DirectoryFactory.h"
#include "music/MusicFileItemClassify.h"
#include "playlists/PlayList.h"
#include "playlists/PlayListFileItemClassify.h"
#include "settings/AdvancedSettings.h"
#include "settings/DiscSettings.h"
#include "settings/MediaSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "storage/MediaManager.h"
#include "utils/DiscsUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"
#include "video/Bookmark.h"
#include "video/VideoDatabase.h"
#include "video/VideoFileItemClassify.h"
#include "video/VideoInfoTag.h"

#include <cstdint>
#include <memory>
#include <utility>

using namespace KODI;
using namespace XFILE;

bool CApplicationPlay::ResolvePath()
{
  // Get bluray:// path for resolution if present
  if (m_item.HasVideoInfoTag() && m_item.GetVideoInfoTag()->GetPath().starts_with("bluray://"))
    m_item.SetDynPath(m_item.GetVideoInfoTag()->m_strFileNameAndPath);

  // Translate/Resolve the url if needed - recursively, but only limited times.
  std::string lastDynPath{m_item.GetDynPath()};
  size_t itemResolveAttempt{0};
  while (itemResolveAttempt < MAX_ITEM_RESOLVE_ATTEMPTS)
  {
    itemResolveAttempt++;

    if (const std::unique_ptr<IDirectory> dir{CDirectoryFactory::Create(m_item)};
        dir && !dir->Resolve(m_item))
    {
      CLog::LogF(LOGERROR, "Error resolving item. Item '{}' is not playable.", m_item.GetDynPath());
      return false;
    }

    std::string newDynPath{m_item.GetDynPath()};
    if (newDynPath == lastDynPath)
      break; // done

    lastDynPath = std::move(newDynPath);
  }
  return true;
}

bool CApplicationPlay::ResolveStack()
{
  if (!m_stackHelper.InitializeStack(m_item))
    return false;

  // Particularly inefficient on startup as, if times are not saved in the database, each video
  // is opened in turn to determine its length. A faster calculation of video time would improve
  // this substantially.
  const auto startOffset{m_stackHelper.InitializeStackStartPartAndOffset(m_item)};
  if (!startOffset.has_value())
  {
    CLog::LogF(LOGERROR, "Failed to obtain start offset for stack {}. Aborting playback.",
               m_item.GetDynPath());
    return false;
  }
  const std::string savedPlayerState{m_item.GetProperty("savedplayerstate").asString("")};

  // Replace stack:// FileItem with the individual stack part FileItem
  m_item = m_stackHelper.GetCurrentStackPartFileItem();

  m_item.SetStartOffset(startOffset.value());
  if (!savedPlayerState.empty())
    m_item.SetProperty("savedplayerstate", savedPlayerState);

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

void CApplicationPlay::GetOptionsAndUpdateItem(bool restart)
{
  if (m_item.HasProperty("StartPercent"))
  {
    m_options.startpercent = m_item.GetProperty("StartPercent").asDouble();
    m_item.SetStartOffset(0);
  }
  m_options.starttime = CUtil::ConvertMilliSecsToSecs(m_item.GetStartOffset());

  if (restart && m_item.HasVideoInfoTag())
    m_options.state = m_item.GetVideoInfoTag()->GetResumePoint().playerState;

  if (VIDEO::IsVideo(m_item) && (!restart || m_stackHelper.IsPlayingISOStack()))
  {
    if (m_item.HasProperty("savedplayerstate"))
    {
      // savedplayerstate is set in CPowerManager on sleep
      m_options.starttime = CUtil::ConvertMilliSecsToSecs(m_item.GetStartOffset());
      m_options.state = m_item.GetProperty("savedplayerstate").asString();
      m_item.ClearProperty("savedplayerstate");
      return;
    }

    CVideoDatabase db;
    if (!db.Open())
      return;

    // Get path
    std::string path{m_item.GetPath()};
    if (m_item.HasVideoInfoTag())
    {
      const std::string videoInfoTagPath{m_item.GetVideoInfoTag()->m_strFileNameAndPath};
      // removable:// may be embedded in bluray:// path
      if (CURL::Decode(videoInfoTagPath).find("removable://") != std::string::npos ||
          VIDEO::IsVideoDb(m_item))
        path = videoInfoTagPath;
    }
    else if (m_item.HasProperty("original_listitem_url") &&
             URIUtils::IsPlugin(m_item.GetProperty("original_listitem_url").asString()))
    {
      path = m_item.GetProperty("original_listitem_url").asString();
    }

    // Note that we need to load the tag from database also if the item already has a tag,
    // because for example the (full) video info for strm files will be loaded here.
    db.LoadVideoInfo(path, *m_item.GetVideoInfoTag());

    if (m_item.GetStartOffset() == STARTOFFSET_RESUME)
    {
      m_options.starttime = 0.0;

      // See if resume point is set in the item
      if (m_item.IsResumePointSet())
      {
        m_options.starttime = m_item.GetCurrentResumeTime();
        if (m_item.HasVideoInfoTag())
          m_options.state = m_item.GetVideoInfoTag()->GetResumePoint().playerState;
      }
      else
      {
        // See if there is resume point in the database
        CBookmark bookmark;
        if (db.GetResumeBookMark(path, bookmark))
        {
          m_options.starttime = bookmark.timeInSeconds;
          m_options.state = bookmark.playerState;
        }
        else if (m_options.starttime == 0.0 && m_item.HasVideoInfoTag())
          GetEpisodeBookmark(m_item, m_options, db);
      }
    }
    else if (m_item.HasVideoInfoTag())
      GetEpisodeBookmark(m_item, m_options, db);

    db.Close();
  }
}

bool CApplicationPlay::GetPlaylistIfDisc()
{
  // See if disc image is a Blu-ray (as an image could be a DVD as well) or if the path is a BDMV folder
  const bool isBluray{::UTILS::DISCS::IsBlurayDiscImage(m_item) ||
                      URIUtils::IsBDFile(m_item.GetDynPath())};

  // See if choose (new) playlist has been selected from context menu
  const bool forceSelection{m_item.GetProperty("force_playlist_selection").asBoolean(false)};
  const bool forceBlurayPlaylistSelection{forceSelection &&
                                          URIUtils::IsBlurayPath(m_item.GetDynPath())};

  if ((isBluray && !(m_options.startpercent > 0.0 || m_options.starttime > 0.0)) ||
      forceBlurayPlaylistSelection)
  {
    const bool isSimpleMenuAllowed{
        [this, forceSelection]()
        {
          const CPlayerCoreFactory& playerCoreFactory{CServiceBroker::GetPlayerCoreFactory()};
          const std::string defaultPlayer{
              m_player.empty() ? playerCoreFactory.GetDefaultPlayer(m_item) : m_player};

          // No video selection when using external or remote players (they handle it if supported)
          const bool isExternalPlayer{playerCoreFactory.IsExternalPlayer(defaultPlayer)};
          const bool isRemotePlayer{playerCoreFactory.IsRemotePlayer(defaultPlayer)};

          // Check if simple menu is enabled or if we are forced to select a playlist
          const bool isSimpleMenu{forceSelection ||
                                  CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(
                                      CSettings::SETTING_DISC_PLAYBACK) == BD_PLAYBACK_SIMPLE_MENU};

          return !isExternalPlayer && !isRemotePlayer && isSimpleMenu;
        }()};

    if (isSimpleMenuAllowed)
    {
      if (!CGUIDialogSimpleMenu::ShowPlaylistSelection(m_item))
        return false;

      // Reset any resume state as new playlist chosen
      m_options.starttime = m_options.startpercent = 0.0;
      m_options.state = {};
      m_item.ClearProperty("force_playlist_selection");
    }
  }

  return true;
}

namespace
{
enum class PlayMediaType : uint8_t
{
  MUSIC_PLAYLIST,
  VIDEO,
  VIDEO_PLAYLIST
};

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
    default:
      break;
  }

  return false; // should never reach here
}
} // namespace

void CApplicationPlay::DetermineFullScreen()
{
  // Get current playlist info
  const auto playlistId{CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist()};

  // Determine fullscreen status based on media type and playlist
  using enum PlayMediaType;
  if (MUSIC::IsAudio(m_item) && playlistId == PLAYLIST::Id::TYPE_MUSIC)
    m_options.fullscreen = ShouldGoFullScreen(MUSIC_PLAYLIST);
  else if (VIDEO::IsVideo(m_item) && playlistId == PLAYLIST::Id::TYPE_VIDEO &&
           CServiceBroker::GetPlaylistPlayer().GetPlaylist(playlistId).size() > 1)
  {
    m_options.fullscreen = ShouldGoFullScreen(VIDEO_PLAYLIST);
  }
  else if (m_stackHelper.IsPlayingRegularStack())
  {
    if (m_stackHelper.GetCurrentPartNumber() == 0 ||
        m_stackHelper.GetRegisteredStack(m_item)->GetStartOffset() != 0)
      m_options.fullscreen = ShouldGoFullScreen(VIDEO);
    else
      m_options.fullscreen = false;
  }
  else
    m_options.fullscreen = ShouldGoFullScreen(VIDEO);
}

CApplicationPlay::GatherPlaybackDetailsResult CApplicationPlay::GatherPlaybackDetails(
    const CFileItem& item, std::string player, bool restart)
{
  m_item = item;
  m_player = std::move(player);

  if (m_item.IsStack())
  {
    if (!ResolveStack())
      return GatherPlaybackDetailsResult::RESULT_ERROR;

    m_player.clear();
    restart = true;
  }

  // Ensure the MIME type has been retrieved for http:// and shout:// streams
  if (m_item.GetMimeType().empty())
    m_item.FillInMimeType();

  if (VIDEO::IsDiscStub(m_item))
    return GatherPlaybackDetailsResult::RESULT_SUCCESS; // all done

  if (PLAYLIST::IsPlayList(m_item))
    return GatherPlaybackDetailsResult::RESULT_ERROR; // playlists not supported

  if (!ResolvePath())
    return GatherPlaybackDetailsResult::RESULT_ERROR;

  m_options = {};
  GetOptionsAndUpdateItem(restart);

  if (!GetPlaylistIfDisc())
    return GatherPlaybackDetailsResult::RESULT_NO_PLAYLIST_SELECTED;

  DetermineFullScreen();

  // Stereo streams may have lower quality, i.e. 32bit vs 16 bit
  m_options.preferStereo =
      CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_videoPreferStereoStream &&
      CServiceBroker::GetActiveAE()->HasStereoAudioChannelCount();

  return GatherPlaybackDetailsResult::RESULT_SUCCESS;
}
