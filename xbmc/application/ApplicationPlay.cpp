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
#include "Util.h"
#include "cores/AudioEngine/Interfaces/AE.h"
#include "cores/playercorefactory/PlayerCoreFactory.h"
#include "filesystem/DirectoryFactory.h"
#include "filesystem/DiscDirectoryHelper.h"
#include "music/MusicFileItemClassify.h"
#include "playlists/PlayList.h"
#include "playlists/PlayListFileItemClassify.h"
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
      options.state = bookmark.playerState;

      // Only apply starttime for non-local/LAN items where EDL parser won't run
      if (!URIUtils::IsLocalOrLAN(item.GetDynPath()))
        options.starttime = bookmark.timeInSeconds;

      return true;
    }
  }
  return false;
}
} // namespace

void CApplicationPlay::GetOptionsAndUpdateItem()
{
  if (m_item.HasProperty("StartPercent"))
  {
    m_options.startpercent = m_item.GetProperty("StartPercent").asDouble();
    m_item.SetStartOffset(0);
  }
  m_options.starttime = CUtil::ConvertMilliSecsToSecs(m_item.GetStartOffset());

  if (VIDEO::IsVideo(m_item))
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
        else if (m_options.starttime == 0.0)
          GetEpisodeBookmark(m_item, m_options, db);
      }

      // No resume data found from any source — clear the stale resume request
      // so downstream code (e.g. DVDInputStreamBluray::Open) doesn't attempt
      // to resume from a non-existent state.
      if (m_options.starttime == 0.0)
        m_item.SetStartOffset(0);
    }
    else
      GetEpisodeBookmark(m_item, m_options, db);

    db.Close();
  }
}

namespace
{
MenuDecision GetMenuDecisions(const CFileItem& item,
                              const std::string& player,
                              const CPlayerOptions& options)
{
  using enum MenuDecision;
  const CPlayerCoreFactory& playerCoreFactory{CServiceBroker::GetPlayerCoreFactory()};
  const std::string defaultPlayer{player.empty() ? playerCoreFactory.GetDefaultPlayer(item)
                                                 : player};

  // No video selection when using external or remote players (they handle it if supported)
  const bool isExternalPlayer{playerCoreFactory.IsExternalPlayer(defaultPlayer)};
  const bool isRemotePlayer{playerCoreFactory.IsRemotePlayer(defaultPlayer)};
  if (isExternalPlayer || isRemotePlayer)
    return NO_ACTION;

  // See if disc image is a Blu-ray (as an image could be a DVD as well) or if the path is a BDMV folder
  const bool isBluray{::UTILS::DISCS::IsBlurayDiscImage(item) ||
                      URIUtils::IsBDFile(item.GetDynPath())};

  // If already a bluray:// path then playlist selection may not be needed
  // Unless overridden by context menu 'Choose Playlist' or 'Simple Menu' in settings
  const bool isBlurayPath{URIUtils::IsBlurayPath(item.GetDynPath())};

  const int playbackSetting{CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(
      CSettings::SETTING_DISC_PLAYBACK)};
  const bool atStart{options.startpercent == 0.0 && options.starttime == 0.0};

  // Show Disc menu
  // This takes priority over the simple menu
  const bool useDiscMenuSetting{playbackSetting == BD_PLAYBACK_DISC_MENU};
  if ((isBluray || isBlurayPath) && atStart && useDiscMenuSetting)
    return SHOW_DISC_MENU;

  // Select main title
  const bool useMainTitleSetting{playbackSetting == BD_PLAYBACK_MAIN_TITLE};
  if ((isBluray || isBlurayPath) && atStart && useMainTitleSetting)
    return GET_MAIN_TITLE;

  // See if choose (new) playlist has been selected from context menu of if simple menu should always be used
  const bool forceSelectionAlways{item.GetProperty("force_playlist_selection").asBoolean(false)};
  const bool forceSelectionAtStart{playbackSetting == BD_PLAYBACK_SIMPLE_MENU};
  const bool mainTitle{playbackSetting == BD_PLAYBACK_MAIN_TITLE};

  // Show simple menu:
  // If we don't have a playlist (ie. ISO or BDMV) and we're at the start
  if (isBluray && atStart && !mainTitle)
    return SHOW_SIMPLE_MENU;

  // If we already have a playlist but Choose Playlist has been selected on the context menu
  if (forceSelectionAlways && isBlurayPath)
    return SHOW_SIMPLE_MENU;
  // If we already have a playlist but we're at the start and simple menu is enabled in settings
  if (forceSelectionAtStart && isBlurayPath && atStart)
    return SHOW_SIMPLE_MENU;

  return NO_ACTION;
}
} // namespace

bool CApplicationPlay::GetPlaylistIfDisc()
{
  using enum MenuDecision;
  switch (MenuDecision menuDecision{GetMenuDecisions(m_item, m_player, m_options)}; menuDecision)
  {
    case SHOW_DISC_MENU:
    {
      // Generate a bluray:// menu path
      m_item.SetDynPath(URIUtils::GetBlurayMenuPath(m_item.GetDynPath()));
      break;
    }
    case SHOW_SIMPLE_MENU:
    case GET_MAIN_TITLE:
    case SILENT:
    {
      // Select playlist, showing simple menu if needed
      if (!CDiscDirectoryHelper::GetOrShowPlaylistSelection(m_item, menuDecision))
        return false; // User cancelled

      // Reset any resume state as new playlist chosen
      m_options.starttime = m_options.startpercent = 0.0;
      m_options.state = {};
      m_item.ClearProperty("force_playlist_selection");
      break;
    }
    case NO_ACTION:
      break;
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
  else
    m_options.fullscreen = ShouldGoFullScreen(VIDEO);
}

CApplicationPlay::GatherPlaybackDetailsResult CApplicationPlay::GatherPlaybackDetails(
    const CFileItem& item, std::string player, bool restart)
{
  m_item = item;
  m_player = std::move(player);

  // Deal with stacks
  // This retrieves the individual stack part FileItem from the stack
  //  and also generates the PlayerOptions for the stack part.
  bool isStack{item.IsStack()};
  if (isStack)
  {
    if (!m_stackHelper.InitializeStack(item))
    {
      CLog::LogF(LOGERROR, "Failed to initialise stack for {}. Aborting playback.",
                 item.GetDynPath());
      return GatherPlaybackDetailsResult::RESULT_ERROR;
    }
    m_stackHelper.GetStackPartAndOptions(m_item, m_options, restart);
  }

  // Ensure the MIME type has been retrieved for http:// and shout:// streams
  if (m_item.GetMimeType().empty())
    m_item.FillInMimeType();

  if (VIDEO::IsDiscStub(m_item))
    return GatherPlaybackDetailsResult::RESULT_SUCCESS; // all done

  if (PLAYLIST::IsPlayList(m_item) && !m_item.IsGame())
    return GatherPlaybackDetailsResult::RESULT_ERROR; // non-game playlists not supported

  if (!ResolvePath())
    return GatherPlaybackDetailsResult::RESULT_ERROR;

  if (!isStack)
  {
    // May be set even if restarting (eg. moving between stack parts)
    m_options = {};
    m_options.starttime = CUtil::ConvertMilliSecsToSecs(item.GetStartOffset());

    GetOptionsAndUpdateItem();
  }

  if (!GetPlaylistIfDisc())
    return GatherPlaybackDetailsResult::
        RESULT_NO_PLAYLIST_SELECTED; // Playlist needed but none selected (ie. user cancelled) so abort playback

  DetermineFullScreen();

  // Stereo streams may have lower quality, i.e. 32bit vs 16 bit
  m_options.preferStereo =
      CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_videoPreferStereoStream &&
      CServiceBroker::GetActiveAE()->HasStereoAudioChannelCount();

  return GatherPlaybackDetailsResult::RESULT_SUCCESS;
}
