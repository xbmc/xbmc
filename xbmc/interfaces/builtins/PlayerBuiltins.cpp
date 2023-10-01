/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PlayerBuiltins.h"

#include "FileItem.h"
#include "GUIUserMessages.h"
#include "PartyModeManager.h"
#include "PlayListPlayer.h"
#include "SeekHandler.h"
#include "ServiceBroker.h"
#include "Util.h"
#include "application/Application.h"
#include "application/ApplicationComponents.h"
#include "application/ApplicationPlayer.h"
#include "application/ApplicationPowerHandling.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "music/MusicUtils.h"
#include "playlists/PlayList.h"
#include "pvr/PVRManager.h"
#include "pvr/channels/PVRChannel.h"
#include "pvr/guilib/PVRGUIActionsChannels.h"
#include "pvr/recordings/PVRRecording.h"
#include "settings/MediaSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "storage/MediaManager.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"
#include "video/PlayerController.h"
#include "video/VideoUtils.h"
#include "video/guilib/VideoSelectActionProcessor.h"

#include <math.h>

#ifdef HAS_OPTICAL_DRIVE
#include "Autorun.h"
#endif

/*! \brief Clear current playlist
 *  \param params (ignored)
 */
static int ClearPlaylist(const std::vector<std::string>& params)
{
  CServiceBroker::GetPlaylistPlayer().Clear();

  return 0;
}

/*! \brief Start a playlist from a given offset.
 *  \param params The parameters.
 *  \details params[0] = Position in playlist or playlist type.
 *           params[1] = Position in playlist if params[0] is playlist type (optional).
 */
static int PlayOffset(const std::vector<std::string>& params)
{
  // playlist.playoffset(offset)
  // playlist.playoffset(music|video,offset)
  std::string strPos = params[0];
  std::string paramlow(params[0]);
  StringUtils::ToLower(paramlow);
  if (params.size() > 1)
  {
    // ignore any other parameters if present
    std::string strPlaylist = params[0];
    strPos = params[1];

    PLAYLIST::Id playlistId = PLAYLIST::TYPE_NONE;
    if (paramlow == "music")
      playlistId = PLAYLIST::TYPE_MUSIC;
    else if (paramlow == "video")
      playlistId = PLAYLIST::TYPE_VIDEO;

    // unknown playlist
    if (playlistId == PLAYLIST::TYPE_NONE)
    {
      CLog::Log(LOGERROR, "Playlist.PlayOffset called with unknown playlist: {}", strPlaylist);
      return false;
    }

    // user wants to play the 'other' playlist
    if (playlistId != CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist())
    {
      g_application.StopPlaying();
      CServiceBroker::GetPlaylistPlayer().Reset();
      CServiceBroker::GetPlaylistPlayer().SetCurrentPlaylist(playlistId);
    }
  }
  // play the desired offset
  int pos = atol(strPos.c_str());

  const auto& components = CServiceBroker::GetAppComponents();
  const auto appPlayer = components.GetComponent<CApplicationPlayer>();
  // playlist is already playing
  if (appPlayer->IsPlaying())
    CServiceBroker::GetPlaylistPlayer().PlayNext(pos);
  // we start playing the 'other' playlist so we need to use play to initialize the player state
  else
    CServiceBroker::GetPlaylistPlayer().Play(pos, "");

  return 0;
}

/*! \brief Control player.
 *  \param params The parameters
 *  \details params[0] = Control to execute.
 *           params[1] = "notify" to notify user (optional, certain controls).
 */
static int PlayerControl(const std::vector<std::string>& params)
{
  auto& components = CServiceBroker::GetAppComponents();
  const auto appPower = components.GetComponent<CApplicationPowerHandling>();
  appPower->ResetScreenSaver();
  appPower->WakeUpScreenSaverAndDPMS();

  std::string paramlow(params[0]);
  StringUtils::ToLower(paramlow);

  const auto appPlayer = components.GetComponent<CApplicationPlayer>();

  if (paramlow ==  "play")
  { // play/pause
    // either resume playing, or pause
    if (appPlayer->IsPlaying())
    {
      if (appPlayer->GetPlaySpeed() != 1)
        appPlayer->SetPlaySpeed(1);
      else
        appPlayer->Pause();
    }
  }
  else if (paramlow == "stop")
  {
    g_application.StopPlaying();
  }
  else if (StringUtils::StartsWithNoCase(params[0], "frameadvance"))
  {
    std::string strFrames;
    if (params[0].size() == 12)
      CLog::Log(LOGERROR, "PlayerControl(frameadvance(n)) called with no argument");
    else if (params[0].size() < 15) // arg must be at least "(N)"
      CLog::Log(LOGERROR, "PlayerControl(frameadvance(n)) called with invalid argument: \"{}\"",
                params[0].substr(13));
    else

    strFrames = params[0].substr(13);
    StringUtils::TrimRight(strFrames, ")");
    float frames = (float) atof(strFrames.c_str());
    appPlayer->FrameAdvance(frames);
  }
  else if (paramlow =="rewind" || paramlow == "forward")
  {
    if (appPlayer->IsPlaying() && !appPlayer->IsPaused())
    {
      float playSpeed = appPlayer->GetPlaySpeed();

      if (paramlow == "rewind" && playSpeed == 1) // Enables Rewinding
        playSpeed *= -2;
      else if (paramlow == "rewind" && playSpeed > 1) //goes down a notch if you're FFing
        playSpeed /= 2;
      else if (paramlow == "forward" && playSpeed < 1) //goes up a notch if you're RWing
      {
        playSpeed /= 2;
        if (playSpeed == -1)
          playSpeed = 1;
      }
      else
        playSpeed *= 2;

      if (playSpeed > 32 || playSpeed < -32)
        playSpeed = 1;

      appPlayer->SetPlaySpeed(playSpeed);
    }
  }
  else if (paramlow =="tempoup" || paramlow == "tempodown")
  {
    if (appPlayer->SupportsTempo() && appPlayer->IsPlaying() && !appPlayer->IsPaused())
    {
      float playTempo = appPlayer->GetPlayTempo();
      if (paramlow == "tempodown")
          playTempo -= 0.1f;
      else if (paramlow == "tempoup")
          playTempo += 0.1f;

      appPlayer->SetTempo(playTempo);
    }
  }
  else if (StringUtils::StartsWithNoCase(params[0], "tempo"))
  {
    if (params[0].size() == 5)
      CLog::Log(LOGERROR, "PlayerControl(tempo(n)) called with no argument");
    else if (params[0].size() < 8) // arg must be at least "(N)"
      CLog::Log(LOGERROR, "PlayerControl(tempo(n)) called with invalid argument: \"{}\"",
                params[0].substr(6));
    else
    {
      if (appPlayer->SupportsTempo() && appPlayer->IsPlaying() && !appPlayer->IsPaused())
      {
        std::string strTempo = params[0].substr(6);
        StringUtils::TrimRight(strTempo, ")");
        float playTempo = strtof(strTempo.c_str(), nullptr);

        appPlayer->SetTempo(playTempo);
      }
    }
  }
  else if (paramlow == "next")
  {
    g_application.OnAction(CAction(ACTION_NEXT_ITEM));
  }
  else if (paramlow == "previous")
  {
    g_application.OnAction(CAction(ACTION_PREV_ITEM));
  }
  else if (paramlow == "bigskipbackward")
  {
    if (appPlayer->IsPlaying())
      appPlayer->Seek(false, true);
  }
  else if (paramlow == "bigskipforward")
  {
    if (appPlayer->IsPlaying())
      appPlayer->Seek(true, true);
  }
  else if (paramlow == "smallskipbackward")
  {
    if (appPlayer->IsPlaying())
      appPlayer->Seek(false, false);
  }
  else if (paramlow == "smallskipforward")
  {
    if (appPlayer->IsPlaying())
      appPlayer->Seek(true, false);
  }
  else if (StringUtils::StartsWithNoCase(params[0], "seekpercentage"))
  {
    std::string offset;
    if (params[0].size() == 14)
      CLog::Log(LOGERROR,"PlayerControl(seekpercentage(n)) called with no argument");
    else if (params[0].size() < 17) // arg must be at least "(N)"
      CLog::Log(LOGERROR, "PlayerControl(seekpercentage(n)) called with invalid argument: \"{}\"",
                params[0].substr(14));
    else
    {
      // Don't bother checking the argument: an invalid arg will do seek(0)
      offset = params[0].substr(15);
      StringUtils::TrimRight(offset, ")");
      float offsetpercent = (float) atof(offset.c_str());
      if (offsetpercent < 0 || offsetpercent > 100)
        CLog::Log(LOGERROR, "PlayerControl(seekpercentage(n)) argument, {:f}, must be 0-100",
                  offsetpercent);
      else if (appPlayer->IsPlaying())
        g_application.SeekPercentage(offsetpercent);
    }
  }
  else if (paramlow == "showvideomenu")
  {
    if (appPlayer->IsPlaying())
      appPlayer->OnAction(CAction(ACTION_SHOW_VIDEOMENU));
  }
  else if (StringUtils::StartsWithNoCase(params[0], "partymode"))
  {
    std::string strXspPath;
    //empty param=music, "music"=music, "video"=video, else xsp path
    PartyModeContext context = PARTYMODECONTEXT_MUSIC;
    if (params[0].size() > 9)
    {
      if (params[0].size() == 16 && StringUtils::EndsWithNoCase(params[0], "video)"))
        context = PARTYMODECONTEXT_VIDEO;
      else if (params[0].size() != 16 || !StringUtils::EndsWithNoCase(params[0], "music)"))
      {
        strXspPath = params[0].substr(10);
        StringUtils::TrimRight(strXspPath, ")");
        context = PARTYMODECONTEXT_UNKNOWN;
      }
    }
    if (g_partyModeManager.IsEnabled())
      g_partyModeManager.Disable();
    else
      g_partyModeManager.Enable(context, strXspPath);
  }
  else if (paramlow == "random" || paramlow == "randomoff" || paramlow == "randomon")
  {
    // get current playlist
    PLAYLIST::Id playlistId = CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist();

    // reverse the current setting
    bool shuffled = CServiceBroker::GetPlaylistPlayer().IsShuffled(playlistId);
    if ((shuffled && paramlow == "randomon") || (!shuffled && paramlow == "randomoff"))
      return 0;

    // check to see if we should notify the user
    bool notify = (params.size() == 2 && StringUtils::EqualsNoCase(params[1], "notify"));
    CServiceBroker::GetPlaylistPlayer().SetShuffle(playlistId, !shuffled, notify);

    // save settings for now playing windows
    switch (playlistId)
    {
      case PLAYLIST::TYPE_MUSIC:
        CMediaSettings::GetInstance().SetMusicPlaylistShuffled(
            CServiceBroker::GetPlaylistPlayer().IsShuffled(playlistId));
        CServiceBroker::GetSettingsComponent()->GetSettings()->Save();
        break;
      case PLAYLIST::TYPE_VIDEO:
        CMediaSettings::GetInstance().SetVideoPlaylistShuffled(
            CServiceBroker::GetPlaylistPlayer().IsShuffled(playlistId));
        CServiceBroker::GetSettingsComponent()->GetSettings()->Save();
      default:
        break;
    }

    // send message
    CGUIMessage msg(GUI_MSG_PLAYLISTPLAYER_RANDOM, 0, 0, playlistId,
                    CServiceBroker::GetPlaylistPlayer().IsShuffled(playlistId));
    CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg);
  }
  else if (StringUtils::StartsWithNoCase(params[0], "repeat"))
  {
    // get current playlist
    PLAYLIST::Id playlistId = CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist();
    PLAYLIST::RepeatState prevRepeatState =
        CServiceBroker::GetPlaylistPlayer().GetRepeat(playlistId);

    std::string paramlow(params[0]);
    StringUtils::ToLower(paramlow);

    PLAYLIST::RepeatState repeatState;
    if (paramlow == "repeatall")
      repeatState = PLAYLIST::RepeatState::ALL;
    else if (paramlow == "repeatone")
      repeatState = PLAYLIST::RepeatState::ONE;
    else if (paramlow == "repeatoff")
      repeatState = PLAYLIST::RepeatState::NONE;
    else if (prevRepeatState == PLAYLIST::RepeatState::NONE)
      repeatState = PLAYLIST::RepeatState::ALL;
    else if (prevRepeatState == PLAYLIST::RepeatState::ALL)
      repeatState = PLAYLIST::RepeatState::ONE;
    else
      repeatState = PLAYLIST::RepeatState::NONE;

    if (repeatState == prevRepeatState)
      return 0;

    // check to see if we should notify the user
    bool notify = (params.size() == 2 && StringUtils::EqualsNoCase(params[1], "notify"));
    CServiceBroker::GetPlaylistPlayer().SetRepeat(playlistId, repeatState, notify);

    // save settings for now playing windows
    switch (playlistId)
    {
      case PLAYLIST::TYPE_MUSIC:
        CMediaSettings::GetInstance().SetMusicPlaylistRepeat(repeatState ==
                                                             PLAYLIST::RepeatState::ALL);
        CServiceBroker::GetSettingsComponent()->GetSettings()->Save();
        break;
      case PLAYLIST::TYPE_VIDEO:
        CMediaSettings::GetInstance().SetVideoPlaylistRepeat(repeatState ==
                                                             PLAYLIST::RepeatState::ALL);
        CServiceBroker::GetSettingsComponent()->GetSettings()->Save();
    }

    // send messages so now playing window can get updated
    CGUIMessage msg(GUI_MSG_PLAYLISTPLAYER_REPEAT, 0, 0, playlistId, static_cast<int>(repeatState));
    CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg);
  }
  else if (StringUtils::StartsWithNoCase(params[0], "resumelivetv"))
  {
    CFileItem& fileItem(g_application.CurrentFileItem());
    std::shared_ptr<PVR::CPVRChannel> channel = fileItem.HasPVRRecordingInfoTag() ? fileItem.GetPVRRecordingInfoTag()->Channel() : std::shared_ptr<PVR::CPVRChannel>();

    if (channel)
    {
      const std::shared_ptr<PVR::CPVRChannelGroupMember> groupMember =
          CServiceBroker::GetPVRManager().Get<PVR::GUI::Channels>().GetChannelGroupMember(channel);
      if (!groupMember)
      {
        CLog::Log(LOGERROR, "ResumeLiveTv could not obtain channel group member for channel: {}",
                  channel->ChannelName());
        return false;
      }

      CFileItem playItem(groupMember);
      if (!g_application.PlayMedia(
              playItem, "", channel->IsRadio() ? PLAYLIST::TYPE_MUSIC : PLAYLIST::TYPE_VIDEO))
      {
        CLog::Log(LOGERROR, "ResumeLiveTv could not play channel: {}", channel->ChannelName());
        return false;
      }
    }
  }
  else if (paramlow == "reset")
  {
    g_application.OnAction(CAction(ACTION_PLAYER_RESET));
  }

  return 0;
}

/*! \brief Play currently inserted DVD.
 *  \param params The parameters.
 *  \details params[0] = "restart" to restart from resume point (optional).
 */
static int PlayDVD(const std::vector<std::string>& params)
{
#ifdef HAS_OPTICAL_DRIVE
  bool restart = false;
  if (!params.empty() && StringUtils::EqualsNoCase(params[0], "restart"))
    restart = true;
  MEDIA_DETECT::CAutorun::PlayDisc(CServiceBroker::GetMediaManager().GetDiscPath(), true, restart);
#endif

  return 0;
}

namespace
{
void GetItemsForPlayList(const std::shared_ptr<CFileItem>& item, CFileItemList& queuedItems)
{
  if (VIDEO_UTILS::IsItemPlayable(*item))
    VIDEO_UTILS::GetItemsForPlayList(item, queuedItems);
  else if (MUSIC_UTILS::IsItemPlayable(*item))
    MUSIC_UTILS::GetItemsForPlayList(item, queuedItems);
  else
    CLog::LogF(LOGERROR, "Unable to get playlist items for {}", item->GetPath());
}

int PlayOrQueueMedia(const std::vector<std::string>& params, bool forcePlay)
{
  // restore to previous window if needed
  if( CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() == WINDOW_SLIDESHOW ||
      CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() == WINDOW_FULLSCREEN_VIDEO ||
      CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() == WINDOW_FULLSCREEN_GAME ||
      CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() == WINDOW_VISUALISATION )
    CServiceBroker::GetGUI()->GetWindowManager().PreviousWindow();

  // reset screensaver
  auto& components = CServiceBroker::GetAppComponents();
  const auto appPower = components.GetComponent<CApplicationPowerHandling>();
  appPower->ResetScreenSaver();
  appPower->WakeUpScreenSaverAndDPMS();

  CFileItem item(params[0], URIUtils::HasSlashAtEnd(params[0], true));

  // at this point the item instance has only the path and the folder flag set. We
  // need some extended item properties to process resume successfully. Load them.
  item.LoadDetails();

  // ask if we need to check guisettings to resume
  bool askToResume = true;
  int playOffset = 0;
  bool hasPlayOffset = false;
  bool playNext = true;
  for (unsigned int i = 1 ; i < params.size() ; i++)
  {
    if (StringUtils::EqualsNoCase(params[i], "isdir"))
      item.m_bIsFolder = true;
    else if (params[i] == "1") // set fullscreen or windowed
      CMediaSettings::GetInstance().SetMediaStartWindowed(true);
    else if (StringUtils::EqualsNoCase(params[i], "resume"))
    {
      // force the item to resume (if applicable)
      if (VIDEO_UTILS::GetItemResumeInformation(item).isResumable)
        item.SetStartOffset(STARTOFFSET_RESUME);
      else
        item.SetStartOffset(0);

      askToResume = false;
    }
    else if (StringUtils::EqualsNoCase(params[i], "noresume"))
    {
      // force the item to start at the beginning
      item.SetStartOffset(0);
      askToResume = false;
    }
    else if (StringUtils::StartsWithNoCase(params[i], "playoffset="))
    {
      playOffset = atoi(params[i].substr(11).c_str()) - 1;
      item.SetProperty("playlist_starting_track", playOffset);
      hasPlayOffset = true;
    }
    else if (StringUtils::StartsWithNoCase(params[i], "playlist_type_hint="))
    {
      // Set the playlist type for the playlist file (e.g. STRM)
      int playlistTypeHint = std::stoi(params[i].substr(19));
      item.SetProperty("playlist_type_hint", playlistTypeHint);
    }
    else if (StringUtils::EqualsNoCase(params[i], "playnext"))
    {
      // If app player is currently playing, the queued media shall be played next.
      playNext = true;
    }
  }

  if (!item.m_bIsFolder && item.IsPlugin())
    item.SetProperty("IsPlayable", true);

  if (askToResume)
  {
    const VIDEO::GUILIB::SelectAction action =
        VIDEO::GUILIB::CVideoSelectActionProcessorBase::ChoosePlayOrResume(item);
    if (action == VIDEO::GUILIB::SELECT_ACTION_RESUME)
    {
      item.SetStartOffset(STARTOFFSET_RESUME);
    }
    else if (action != VIDEO::GUILIB::SELECT_ACTION_PLAY)
    {
      // The Resume dialog was closed without any choice
      return false;
    }
    item.SetProperty("check_resume", false);
  }

  if (item.IsStack())
  {
    const VIDEO_UTILS::ResumeInformation resumeInfo =
        VIDEO_UTILS::GetStackPartResumeInformation(item, playOffset + 1);

    if (item.GetStartOffset() == STARTOFFSET_RESUME)
      item.SetStartOffset(resumeInfo.startOffset);

    item.m_lStartPartNumber = resumeInfo.partNumber;
  }
  else if (!forcePlay /* queue */ || item.m_bIsFolder || item.IsPlayList())
  {
    CFileItemList items;
    GetItemsForPlayList(std::make_shared<CFileItem>(item), items);
    if (!items.IsEmpty()) // fall through on non expandable playlist
    {
      bool containsMusic = false;
      bool containsVideo = false;
      for (const auto& i : items)
      {
        const bool isVideo = i->IsVideo();
        containsMusic |= !isVideo;
        containsVideo |= isVideo;

        if (containsMusic && containsVideo)
          break;
      }

      PLAYLIST::Id playlistId = containsVideo ? PLAYLIST::TYPE_VIDEO : PLAYLIST::TYPE_MUSIC;
      // Mixed playlist item played by music player, mixed content folder has music removed
      if (containsMusic && containsVideo)
      {
        if (item.IsPlayList())
          playlistId = PLAYLIST::TYPE_MUSIC;
        else
        {
          for (int i = items.Size() - 1; i >= 0; i--) //remove music entries
          {
            if (!items[i]->IsVideo())
              items.Remove(i);
          }
        }
      }

      auto& playlistPlayer = CServiceBroker::GetPlaylistPlayer();

      // Play vs. Queue (+Play)
      if (forcePlay)
      {
        playlistPlayer.ClearPlaylist(playlistId);
        playlistPlayer.Reset();
        playlistPlayer.Add(playlistId, items);
        playlistPlayer.SetCurrentPlaylist(playlistId);
        playlistPlayer.Play(playOffset, "");
      }
      else
      {
        const int oldSize = playlistPlayer.GetPlaylist(playlistId).size();

        const auto appPlayer = components.GetComponent<CApplicationPlayer>();
        if (playNext)
        {
          if (appPlayer->IsPlaying())
            playlistPlayer.Insert(playlistId, items, playlistPlayer.GetCurrentSong() + 1);
          else
            playlistPlayer.Add(playlistId, items);
        }
        else
        {
          playlistPlayer.Add(playlistId, items);
        }

        if (items.Size() && !appPlayer->IsPlaying())
        {
          playlistPlayer.SetCurrentPlaylist(playlistId);

          if (containsMusic)
          {
            // video does not auto play on queue like music
            playlistPlayer.Play(hasPlayOffset ? playOffset : oldSize, "");
          }
        }
      }

      return 0;
    }
  }

  if (forcePlay)
  {
    if (item.HasVideoInfoTag() && item.GetStartOffset() == STARTOFFSET_RESUME)
    {
      const CBookmark bookmark = item.GetVideoInfoTag()->GetResumePoint();
      if (bookmark.IsSet())
        item.SetStartOffset(CUtil::ConvertSecsToMilliSecs(bookmark.timeInSeconds));
    }

    if ((item.IsAudio() || item.IsVideo()) && !item.IsSmartPlayList())
    {
      if (!item.HasProperty("playlist_type_hint"))
        item.SetProperty("playlist_type_hint", PLAYLIST::TYPE_MUSIC);

      CServiceBroker::GetPlaylistPlayer().Play(std::make_shared<CFileItem>(item), "");
    }
    else
    {
      g_application.PlayMedia(item, "", PLAYLIST::TYPE_NONE);
    }
  }

  return 0;
}

/*! \brief Start playback of media.
 *  \param params The parameters.
 *  \details params[0] = URL to media to play (optional).
 *           params[1,...] = "isdir" if media is a directory (optional).
 *           params[1,...] = "1" to start playback in fullscreen (optional).
 *           params[1,...] = "resume" to force resuming (optional).
 *           params[1,...] = "noresume" to force not resuming (optional).
 *           params[1,...] = "playoffset=<offset>" to start playback from a given position in a playlist (optional).
 *           params[1,...] = "playlist_type_hint=<id>" to set the playlist type if a playlist file (e.g. STRM) is played (optional),
 *                           for <id> value refer to PLAYLIST::TYPE_MUSIC / PLAYLIST::TYPE_VIDEO values, if not set will fallback to music playlist.
 */
int PlayMedia(const std::vector<std::string>& params)
{
  return PlayOrQueueMedia(params, true);
}

/*! \brief Queue media in the video or music playlist, according to type of media items. If both audio and video items are contained, queue to video
 *  playlist. Start playback at requested position if player is not playing.
 *  \param params The parameters.
 *  \details params[0] = URL of media to queue.
 *           params[1,...] = "isdir" if media is a directory (optional).
 *           params[1,...] = "1" to start playback in fullscreen (optional).
 *           params[1,...] = "resume" to force resuming (optional).
 *           params[1,...] = "noresume" to force not resuming (optional).
 *           params[1,...] = "playoffset=<offset>" to start playback from a given position in a playlist (optional).
 *           params[1,...] = "playlist_type_hint=<id>" to set the playlist type if a playlist file (e.g. STRM) is played (optional),
 *                           for <id> value refer to PLAYLIST::TYPE_MUSIC / PLAYLIST::TYPE_VIDEO values, if not set will fallback to music playlist.
 *           params[1,...] = "playnext" if player is currently playing, to play the media right after the currently playing item. If player is not
 *                           playing, append media to current playlist (optional).
 */
int QueueMedia(const std::vector<std::string>& params)
{
  return PlayOrQueueMedia(params, false);
}

} // unnamed namespace

/*! \brief Start playback with a given playback core.
 *  \param params The parameters.
 *  \details params[0] = Name of playback core.
 */
static int PlayWith(const std::vector<std::string>& params)
{
  g_application.OnAction(CAction(ACTION_PLAYER_PLAY, params[0]));

  return 0;
}

/*! \brief Seek in currently playing media.
 *  \param params The parameters.
 *  \details params[0] = Number of seconds to seek.
 */
static int Seek(const std::vector<std::string>& params)
{
  auto& components = CServiceBroker::GetAppComponents();
  const auto appPlayer = components.GetComponent<CApplicationPlayer>();
  if (appPlayer->IsPlaying())
    appPlayer->GetSeekHandler().SeekSeconds(atoi(params[0].c_str()));

  return 0;
}

static int SubtitleShiftUp(const std::vector<std::string>& params)
{
  CAction action{ACTION_SUBTITLE_VSHIFT_UP};
  if (!params.empty() && params[0] == "save")
    action.SetText("save");
  CPlayerController::GetInstance().OnAction(action);
  return 0;
}

static int SubtitleShiftDown(const std::vector<std::string>& params)
{
  CAction action{ACTION_SUBTITLE_VSHIFT_DOWN};
  if (!params.empty() && params[0] == "save")
    action.SetText("save");
  CPlayerController::GetInstance().OnAction(action);
  return 0;
}

// Note: For new Texts with comma add a "\" before!!! Is used for table text.
//
/// \page page_List_of_built_in_functions
/// \section built_in_functions_12 Player built-in's
///
/// -----------------------------------------------------------------------------
///
/// \table_start
///   \table_h2_l{
///     Function,
///     Description }
///   \table_row2_l{
///     <b>`PlaysDisc(parm)`</b>\n
///     <b>`PlayDVD(param)`</b>(deprecated)
///     ,
///     Plays the inserted disc\, like CD\, DVD or Blu-ray\, in the disc drive.
///     @param[in] param                 "restart" to restart from resume point (optional)
///   }
///   \table_row2_l{
///     <b>`PlayerControl(control[\,param])`</b>
///     ,
///     Allows control of music and videos. <br>
///     <br>
///     | Control                 | Video playback behaviour               | Audio playback behaviour    | Added in    |
///     |:------------------------|:---------------------------------------|:----------------------------|:------------|
///     | Play                    | Play/Pause                             | Play/Pause                  |             |
///     | Stop                    | Stop                                   | Stop                        |             |
///     | Forward                 | Fast Forward                           | Fast Forward                |             |
///     | Rewind                  | Rewind                                 | Rewind                      |             |
///     | Next                    | Next chapter or movie in playlists     | Next track                  |             |
///     | Previous                | Previous chapter or movie in playlists | Previous track              |             |
///     | TempoUp                 | Increases playback speed               | none                        | Kodi v18    |
///     | TempoDown               | Decreases playback speed               | none                        | Kodi v18    |
///     | Tempo(n)                | Sets playback speed to given value     | none                        | Kodi v19    |
///     | BigSkipForward          | Big Skip Forward                       | Big Skip Forward            | Kodi v15    |
///     | BigSkipBackward         | Big Skip Backward                      | Big Skip Backward           | Kodi v15    |
///     | SmallSkipForward        | Small Skip Forward                     | Small Skip Forward          | Kodi v15    |
///     | SmallSkipBackward       | Small Skip Backward                    | Small Skip Backward         | Kodi v15    |
///     | SeekPercentage(n)       | Seeks to given percentage              | Seeks to given percentage   |             |
///     | Random *                | Toggle Random Playback                 | Toggle Random Playback      |             |
///     | RandomOn                | Sets 'Random' to 'on'                  | Sets 'Random' to 'on'       |             |
///     | RandomOff               | Sets 'Random' to 'off'                 | Sets 'Random' to 'off'      |             |
///     | Repeat *                | Cycles through repeat modes            | Cycles through repeat modes |             |
///     | RepeatOne               | Repeats a single video                 | Repeats a single track      |             |
///     | RepeatAll               | Repeat all videos in a list            | Repeats all tracks in a list|             |
///     | RepeatOff               | Sets 'Repeat' to 'off'                 | Sets 'Repeat' to 'off'      |             |
///     | Partymode(music) **     | none                                   | Toggles music partymode     |             |
///     | Partymode(video) **     | Toggles video partymode                | none                        |             |
///     | Partymode(path to .xsp) | Partymode for *.xsp-file               | Partymode for *.xsp-file    |             |
///     | ShowVideoMenu           | Shows the DVD/BR menu if available     | none                        |             |
///     | FrameAdvance(n) ***     | Advance video by _n_ frames            | none                        | Kodi v18    |
///     | SubtitleShiftUp(save)   | Shift up the subtitle position, add "save" to save the change permanently    | none | Kodi v20 |
///     | SubtitleShiftDown(save) | Shift down the subtitle position, add "save" to save the change permanently  | none | Kodi v20 |
///     <br>
///     '*' = For these controls\, the PlayerControl built-in function can make use of the 'notify'-parameter. For example: PlayerControl(random\, notify)
///     <br>
///     '**' = If no argument is given for 'partymode'\, the control  will default to music.
///     <br>
///     '***' = This only works if the player is paused.
///     <br>
///     @param[in] control               Control to execute.
///     @param[in] param                 "notify" to notify user (optional\, certain controls).
///
///     @note 'TempoUp' or 'TempoDown' only works if "Sync playback to display" is enabled.
///     @note 'Next' will behave differently while using video playlists. In those\, chapters will be ignored and the next movie will be played.
///   }
///   \table_row2_l{
///     <b>`Playlist.Clear`</b>
///     ,
///     Clear the current playlist
///     @param[in]                       (ignored)
///   }
///   \table_row2_l{
///     <b>`Playlist.PlayOffset(positionType[\,position])`</b>
///     ,
///     Start playing from a particular offset in the playlist
///     @param[in] positionType          Position in playlist or playlist type.
///     @param[in] position              Position in playlist if params[0] is playlist type (optional).
///   }
///   \table_row2_l{
///     <b>`PlayMedia(media[\,isdir][\,1]\,[playoffset=xx])`</b>
///     ,
///     Plays the given media. This can be a playlist\, music\, or video file\, directory\,
///     plugin\, disc image stack\, video file stack or an URL. The optional parameter `,isdir` can
///     be used for playing a directory. `,1` will start the media without switching to fullscreen.
///     If media is a playlist or a disc image stack or a video file stack\, you can use
///     playoffset=xx where xx is the position to start playback from.
///     @param[in] media                 URL to media to play (optional).
///     @param[in] isdir                 Set `isdir` if media is a directory (optional).
///     @param[in] windowed              Set `1` to start playback without switching to fullscreen (optional).
///     @param[in] resume                Set `resume` to force resuming (optional).
///     @param[in] noresume              Set `noresume` to force not resuming (optional).
///     @param[in] playoffset            Set `playoffset=<offset>` to start playback from a given position in a playlist or stack (optional).
///   }
///   \table_row2_l{
///     <b>`PlayWith(core)`</b>
///     ,
///     Play the selected item with the specified player core.
///     @param[in] core                  Name of playback core.
///   }
///   \table_row2_l{
///     <b>`Seek(seconds)`</b>
///     ,
///     Seeks to the specified relative amount of seconds within the current
///     playing media. A negative value will seek backward and a positive value forward.
///     @param[in] seconds               Number of seconds to seek.
///   }
///   \table_row2_l{
///     <b>`QueueMedia(media[\,isdir][\,1][\,playnext]\,[playoffset=xx])`</b>
///     \anchor Builtin_QueueMedia,
///     Queues the given media. This can be a playlist\, music\, or video file\, directory\,
///     plugin\, disc image stack\, video file stack or an URL. The optional parameter `,isdir` can
///     be used for playing a directory. `,1` will start the media without switching to fullscreen.
///     If media is a playlist or a disc image stack or a video file stack\, you can use
///     playoffset=xx where xx is the position to start playback from.
///     where xx is the position to start playback from.
///     @param[in] media                 URL of media to queue.
///     @param[in] isdir                 Set `isdir` if media is a directory (optional).
///     @param[in] 1                     Set `1` to start playback without switching to fullscreen (optional).
///     @param[in] resume                Set `resume` to force resuming (optional).
///     @param[in] noresume              Set `noresume` to force not resuming (optional).
///     @param[in] playoffset            Set `playoffset=<offset>` to start playback from a given position in a playlist or stack (optional).
///     @param[in] playnext              Set `playnext` to play the media right after the currently playing item, if player is currently
///     playing. If player is not playing, append media to current playlist (optional).
///     <p><hr>
///     @skinning_v20 **[New builtin]** \link Builtin_QueueMedia `QueueMedia(media[\,isdir][\,1][\,playnext]\,[playoffset=xx])`\endlink
///     <p>
///   }
/// \table_end
///

// clang-format off
CBuiltins::CommandMap CPlayerBuiltins::GetOperations() const
{
  return {
           {"playdisc",            {"Plays the inserted disc, like CD, DVD or Blu-ray, in the disc drive.", 0, PlayDVD}},
           {"playdvd",             {"Plays the inserted disc, like CD, DVD or Blu-ray, in the disc drive.", 0, PlayDVD}},
           {"playlist.clear",      {"Clear the current playlist", 0, ClearPlaylist}},
           {"playlist.playoffset", {"Start playing from a particular offset in the playlist", 1, PlayOffset}},
           {"playercontrol",       {"Control the music or video player", 1, PlayerControl}},
           {"playmedia",           {"Play the specified media file (or playlist)", 1, PlayMedia}},
           {"queuemedia",          {"Queue the specified media in video or music playlist", 1, QueueMedia}},
           {"playwith",            {"Play the selected item with the specified core", 1, PlayWith}},
           {"seek",                {"Performs a seek in seconds on the current playing media file", 1, Seek}},
           {"subtitleshiftup",     {"Shift up the subtitle position", 0, SubtitleShiftUp}},
           {"subtitleshiftdown",   {"Shift down the subtitle position", 0, SubtitleShiftDown}},
         };
}
// clang-format on
