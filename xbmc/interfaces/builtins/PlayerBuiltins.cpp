/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PlayerBuiltins.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "GUIPassword.h"
#include "PartyMode.h"
#include "SeekHandler.h"
#include "ServiceBroker.h"
#include "Util.h"
#include "application/Application.h"
#include "application/ApplicationPlayLists.h"
#include "application/ApplicationPlayer.h"
#include "application/ApplicationPowerHandling.h"
#include "application/PlayListsGUIListener.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "guilib/GUIAudioManager.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"
#include "music/MusicFileItemClassify.h"
#include "music/MusicUtils.h"
#include "playlists/PlayList.h"
#include "playlists/PlayListFileItemClassify.h"
#include "pvr/PVRManager.h"
#include "pvr/channels/PVRChannel.h"
#include "pvr/guilib/PVRGUIActionsChannels.h"
#include "pvr/recordings/PVRRecording.h"
#include "resources/LocalizeStrings.h"
#include "resources/ResourcesComponent.h"
#include "settings/MediaSettings.h"
#include "storage/MediaManager.h"
#include "utils/ItemProperties.h"
#include "utils/PlayerUtils.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"
#include "video/PlayerController.h"
#include "video/VideoFileItemClassify.h"
#include "video/VideoUtils.h"
#include "video/guilib/VideoGUIUtils.h"
#include "video/guilib/VideoPlayActionProcessor.h"

#include <algorithm>
#include <math.h>
#include <string_view>

#ifdef HAS_OPTICAL_DRIVE
#include "Autorun.h"
#endif

using namespace KODI;

namespace
{
//! Toast what changed in a playlist's shuffle or repeat, as PlayerControl(...,notify) asks.
void NotifyPlayOrder(const CApplicationPlayLists& playLists,
                     PLAYLIST::Type type,
                     bool wasShuffled,
                     PLAYLIST::Repeat wasRepeat)
{
  constexpr int STRING_PLAYLIST = 559;
  constexpr int STRING_SHUFFLE = 191;
  constexpr int STRING_ALL = 593;
  constexpr int STRING_OFF = 591;
  const auto localize = [](int code) -> const std::string&
  { return CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(code); };

  if (const bool shuffled = playLists.IsShuffled(type); shuffled != wasShuffled)
    CGUIDialogKaiToast::QueueNotification(
        CGUIDialogKaiToast::Info, localize(STRING_PLAYLIST),
        StringUtils::Format("{}: {}", localize(STRING_SHUFFLE),
                            localize(shuffled ? STRING_ALL : STRING_OFF)));

  if (const PLAYLIST::Repeat repeat = playLists.GetRepeat(type); repeat != wasRepeat)
    CGUIDialogKaiToast::QueueNotification(
        CGUIDialogKaiToast::Info, localize(STRING_PLAYLIST),
        localize(CApplicationPlayLists::RepeatLabel(
            repeat, CApplicationPlayLists::RepeatWording::WithSetting)));
}
} // namespace

/*! \brief Clear both playlists
 *  \param params (ignored)
 */
static int ClearPlaylist(const std::vector<std::string>& params)
{
  CServiceBroker::GetPlayLists()->ClearPlayLists();

  return 0;
}

/*! \brief Move through a playlist, or start it.
 *  \param params The parameters.
 *  \details params[0] = The number of entries to move from the current one in play order (negative
 *                       goes back), or the playlist type.
 *           params[1] = The same number, if params[0] is the playlist type (optional). When nothing
 *                       is playing it is the position to start at.
 */
static int PlayOffset(const std::vector<std::string>& params)
{
  const auto playLists = CServiceBroker::GetPlayLists();
  std::optional<PLAYLIST::Type> type = playLists->GetPlayingType();
  std::string offset = params[0];
  if (params.size() > 1)
  {
    type = PLAYLIST::TypeFromName(params[0]);
    if (!type)
    {
      CLog::Log(LOGERROR, "Playlist.PlayOffset called with unknown playlist: {}", params[0]);
      return -1;
    }
    offset = params[1];
  }

  if (!type)
  {
    CLog::Log(LOGERROR, "Playlist.PlayOffset({}) names no playlist and none is playing", offset);
    return -1;
  }
  APPLICATION::NotifyIfNothingThere(playLists->PlayOffset(*type, std::atoi(offset.c_str())),
                                    APPLICATION::Direction::Forward);

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
    {
      strFrames = params[0].substr(13);
      StringUtils::TrimRight(strFrames, ")");
      appPlayer->FrameAdvance(strtof(strFrames.c_str(), nullptr));
    }
  }
  else if (paramlow =="rewind" || paramlow == "forward")
  {
    const bool wasPaused = appPlayer->IsPaused();
    // FF/RW while paused is only supported for video outside of menus
    if (appPlayer->IsPlaying() &&
        (!wasPaused ||
         (appPlayer->IsPlayingVideo() && !appPlayer->IsPlayingGame() && !appPlayer->IsInMenu())))
    {
      float playSpeed = appPlayer->GetPlaySpeed();
      if (playSpeed == 0)
        playSpeed = 1;

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

      CGUIComponent* gui = CServiceBroker::GetGUI();
      if (wasPaused && gui)
        gui->GetAudioManager().Enable(appPlayer->IsPaused());
    }
  }
  else if (paramlow == "tempoup" || paramlow == "tempodown")
  {
    if (appPlayer->SupportsTempo() && appPlayer->IsPlaying() && !appPlayer->IsPaused())
    {
      if (paramlow == "tempodown")
        CPlayerUtils::AdvanceTempoStep(*appPlayer, TempoStepChange::DECREASE);
      else if (paramlow == "tempoup")
        CPlayerUtils::AdvanceTempoStep(*appPlayer, TempoStepChange::INCREASE);
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
    // partymode, partymode(music), partymode(video) or partymode(<smart playlist path>)
    std::string argument = params[0].size() > 10 ? params[0].substr(10) : "";
    StringUtils::TrimRight(argument, ")");
    if (PARTYMODE::IsRunning())
      PARTYMODE::Stop();
    else if (argument.empty())
      PARTYMODE::Start(PLAYLIST::Audio);
    else if (const std::optional<PLAYLIST::Type> type = PLAYLIST::TypeFromName(argument); type)
      PARTYMODE::Start(*type);
    else
      PARTYMODE::Start(argument);
  }
  else if (paramlow.starts_with("random") || paramlow.starts_with("repeat"))
  {
    const auto playLists = CServiceBroker::GetPlayLists();
    const std::optional<PLAYLIST::Type> type = playLists->GetPlayingType();
    if (!type)
      return 0;

    const bool notify = params.size() == 2 && StringUtils::EqualsNoCase(params[1], "notify");
    const bool shuffled = playLists->IsShuffled(*type);
    const PLAYLIST::Repeat repeated = playLists->GetRepeat(*type);
    // what follows the verb: on or off for random, off, one or all for repeat; anything else
    // toggles random or cycles repeat
    const std::string_view state = std::string_view{paramlow}.substr(6);
    constexpr auto persist = CApplicationPlayLists::Persist::Yes;

    if (paramlow.starts_with("random"))
    {
      if (state == "on" || state == "off")
        playLists->SetShuffle(*type, state == "on", persist);
      else
        playLists->ToggleShuffle(*type, persist);
    }
    else if (const std::optional<PLAYLIST::Repeat> repeat = PLAYLIST::RepeatFromName(state))
    {
      playLists->SetRepeat(*type, *repeat, persist);
    }
    else
    {
      playLists->CycleRepeat(*type, persist);
    }

    if (notify)
      NotifyPlayOrder(*playLists, *type, shuffled, repeated);
  }
  else if (StringUtils::StartsWithNoCase(params[0], "resumelivetv"))
  {
    const std::shared_ptr<const CFileItem> fileItem = g_application.CurrentFileItemPtr();
    std::shared_ptr<PVR::CPVRChannel> channel = fileItem->HasPVRRecordingInfoTag() ? fileItem->GetPVRRecordingInfoTag()->Channel() : std::shared_ptr<PVR::CPVRChannel>();

    if (channel)
    {
      const std::shared_ptr<PVR::CPVRChannelGroupMember> groupMember =
          CServiceBroker::GetPVRManager().Get<PVR::GUI::Channels>().GetChannelGroupMember(channel);
      if (!groupMember)
      {
        CLog::Log(LOGERROR, "ResumeLiveTv could not obtain channel group member for channel: {}",
                  channel->ChannelName());
        return -1;
      }

      CFileItem playItem(groupMember);
      if (!g_application.PlayMedia(playItem))
      {
        CLog::Log(LOGERROR, "ResumeLiveTv could not play channel: {}", channel->ChannelName());
        return -1;
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
 *  \details params[0] = "restart" to play from the beginning instead of resuming (optional).
 */
static int PlayDVD(const std::vector<std::string>& params)
{
#ifdef HAS_OPTICAL_DRIVE
  bool restart = false;
  if (!params.empty() && StringUtils::EqualsNoCase(params[0], "restart"))
    restart = true;
  MEDIA_DETECT::PlayDiscOptions options(
      {.bypassSettings = true, .startFromBeginning = restart, .forceSelection = false});
  MEDIA_DETECT::CAutorun::PlayDisc(CServiceBroker::GetMediaManager().GetDiscPath(), options);
#endif

  return 0;
}

/*! \brief Play currently inserted Bluray, allowing the user to choose the playlist.
 *  \param params Not used here (but needed for builtin interface).
 */
static int PlayPlaylist(const std::vector<std::string>& /*params*/)
{
#ifdef HAS_OPTICAL_DRIVE
  MEDIA_DETECT::PlayDiscOptions options(
      {.bypassSettings = true, .startFromBeginning = false, .forceSelection = true});
  MEDIA_DETECT::CAutorun::PlayDisc(CServiceBroker::GetMediaManager().GetDiscPath(), options);
#endif

  return 0;
}

namespace
{
void GetItemsForPlayList(const std::shared_ptr<CFileItem>& item, CFileItemList& queuedItems)
{
  if (VIDEO::UTILS::IsItemPlayable(*item))
    VIDEO::UTILS::GetItemsForPlayList(item, queuedItems,
                                      ContentUtils::PlayMode::CHECK_AUTO_PLAY_NEXT_ITEM);
  else if (MUSIC_UTILS::IsItemPlayable(*item))
    MUSIC_UTILS::GetItemsForPlayList(item, queuedItems);
}

int PlayOrQueueMedia(const std::vector<std::string>& params,
                     bool forcePlay,
                     const std::shared_ptr<CGUIListItem>& itemIn)
{
  g_application.LeavePlaybackWindow();

  // reset screensaver
  auto& components = CServiceBroker::GetAppComponents();
  const auto appPower = components.GetComponent<CApplicationPowerHandling>();
  appPower->ResetScreenSaver();
  appPower->WakeUpScreenSaverAndDPMS();

  CFileItem item;
  if (itemIn && itemIn->IsFileItem())
  {
    item = *std::static_pointer_cast<CFileItem>(itemIn);
  }
  else
  {
    item = {params[0], URIUtils::HasSlashAtEnd(params[0], true)};

    // at this point the item instance has only the path and the folder flag set. We
    // need some extended item properties to process resume successfully. Load them.
    item.LoadDetails();
  }

  if ((VIDEO::IsVideo(item) && !g_passwordManager.IsVideoUnlocked()) ||
      (MUSIC::IsAudio(item) && !g_passwordManager.IsMusicUnlocked()))
  {
    CLog::LogF(LOGERROR, "MasterCode or MediaSource-code is wrong: {} will not be played.",
               item.GetPath());
    return -1;
  }

  // ask if we need to check guisettings to resume
  bool askToResume = true;
  int playOffset = 0;
  bool hasPlayOffset = false;
  bool playNext = false;
  std::optional<PLAYLIST::Type> namedType;
  for (unsigned int i = 1 ; i < params.size() ; i++)
  {
    if (StringUtils::EqualsNoCase(params[i], "isdir"))
      item.SetFolder(true);
    else if (params[i] == "1") // set fullscreen or windowed
      CMediaSettings::GetInstance().SetMediaStartWindowed(true);
    else if (StringUtils::EqualsNoCase(params[i], "resume"))
    {
      // force the item to resume (if applicable)
      if (VIDEO::UTILS::GetItemResumeInformation(item).isResumable)
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
      hasPlayOffset = true;
    }
    else if (StringUtils::StartsWithNoCase(params[i], "playlist_type_hint="))
    {
      // the hint is a Python playlist id: 0 is music, 1 video
      const std::string_view hint = std::string_view{params[i]}.substr(19);
      if (hint == "0")
        namedType = PLAYLIST::Audio;
      else if (hint == "1")
        namedType = PLAYLIST::Video;
    }
    else if (StringUtils::EqualsNoCase(params[i], "playnext"))
    {
      // If app player is currently playing, the queued media shall be played next.
      playNext = true;
    }
  }

  if (!item.IsFolder() && item.IsPlugin())
    item.SetProperty(ITEM::PROPERTY::IS_PLAYABLE, true);

  if (forcePlay && askToResume)
  {
    const VIDEO::GUILIB::Action action =
        VIDEO::GUILIB::CVideoPlayActionProcessor::ChoosePlayOrResume(item);
    if (action == VIDEO::GUILIB::ACTION_RESUME)
    {
      item.SetStartOffset(STARTOFFSET_RESUME);
    }
    else if (action != VIDEO::GUILIB::ACTION_PLAY_FROM_BEGINNING)
    {
      // The Resume dialog was closed without any choice
      return 0;
    }
  }

  if (!forcePlay /* queue */ || item.IsFolder() || PLAYLIST::IsPlayList(item))
  {
    CFileItemList items;
    GetItemsForPlayList(std::make_shared<CFileItem>(item), items);
    if (!items.IsEmpty()) // fall through on non expandable playlist
    {
      const bool containsMusic =
          std::ranges::any_of(items, [](const auto& i)
      { return !VIDEO::IsVideo(*i); });
      const PLAYLIST::Type type = namedType.value_or(PLAYLIST::TypeFor(items, item));
      const auto playLists = CServiceBroker::GetPlayLists();

      if (forcePlay)
        return playLists->PlayItems(type, items,
                                    hasPlayOffset ? std::optional<int>(playOffset) : std::nullopt)
                   ? 0
                   : -1;

      const int first = playLists->Queue(type, items,
                                         playNext ? CApplicationPlayLists::Placement::Next
                                                  : CApplicationPlayLists::Placement::End);
      if (first < 0)
      {
        CLog::LogF(LOGERROR, "Unable to queue item '{}'", item.GetPath());
        return -1;
      }

      // video does not auto play on queue like music
      if (containsMusic && !components.GetComponent<CApplicationPlayer>()->IsPlaying() &&
          !playLists->PlayFrom(type, hasPlayOffset ? playOffset : first))
        return -1;
      return 0;
    }
  }

  if (!forcePlay)
  {
    CLog::LogF(LOGERROR, "Unable to queue item '{}'", item.GetPath());
    return -1;
  }

  return g_application.PlayMedia(item, "", namedType,
                                 hasPlayOffset ? std::optional<int>(playOffset) : std::nullopt)
             ? 0
             : -1;
}

/*! \brief Start playback of media.
 *  \param params The parameters.
 *  \details params[0] = URL to media to play (optional).
 *           params[1,...] = "isdir" if media is a directory (optional).
 *           params[1,...] = "1" to start playback without switching to fullscreen (optional).
 *           params[1,...] = "resume" to force resuming (optional).
 *           params[1,...] = "noresume" to force not resuming (optional).
 *           params[1,...] = "playoffset=<offset>" to start playback from a given position in a playlist (optional).
 *           params[1,...] = "playlist_type_hint=<id>" to set the playlist type if a playlist file (e.g. STRM) is played (optional),
 *                           <id> is 0 for the music playlist or 1 for video; without it, the playlist is chosen from what the file holds.
 */
int PlayMedia(const std::vector<std::string>& params)
{
  return PlayOrQueueMedia(params, true, nullptr);
}

int PlayMediaEx(const std::vector<std::string>& params, const std::shared_ptr<CGUIListItem>& item)
{
  return PlayOrQueueMedia(params, true, item);
}

/*! \brief Queue media in the video or music playlist, according to type of media items. If both audio and video items are contained, queue to video
 *  playlist. Start playback at requested position if player is not playing.
 *  \param params The parameters.
 *  \details params[0] = URL of media to queue.
 *           params[1,...] = "isdir" if media is a directory (optional).
 *           params[1,...] = "1" to start playback without switching to fullscreen (optional).
 *           params[1,...] = "resume" to force resuming (optional).
 *           params[1,...] = "noresume" to force not resuming (optional).
 *           params[1,...] = "playoffset=<offset>" to start playback from a given position in a playlist (optional).
 *           params[1,...] = "playlist_type_hint=<id>" to set the playlist type if a playlist file (e.g. STRM) is played (optional),
 *                           <id> is 0 for the music playlist or 1 for video; without it, the playlist is chosen from what the file holds.
 *           params[1,...] = "playnext" if player is currently playing, to play the media right after the currently playing item. If player is not
 *                           playing, append the media to its playlist (optional).
 */
int QueueMedia(const std::vector<std::string>& params)
{
  return PlayOrQueueMedia(params, false, nullptr);
}

int QueueMediaEx(const std::vector<std::string>& params, const std::shared_ptr<CGUIListItem>& item)
{
  return PlayOrQueueMedia(params, false, item);
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
///     <b>`PlayDisc(param)`</b>\n
///     <b>`PlayDVD(param)`</b>(deprecated)
///     ,
///     Plays the inserted disc\, like CD\, DVD or Blu-ray\, in the disc drive.
///     @param[in] param                 "restart" to play from the beginning instead of resuming (optional)
///   }
///   \table_row2_l{
///     <b>`PlayPlaylist`</b>
///     ,
///     Play the disc in the drive\, asking which playlist to use rather than
///     starting the main title. Intended for Blu-ray discs.
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
///     Clear the video and music playlists
///     @param[in]                       (ignored)
///   }
///   \table_row2_l{
///     <b>`Playlist.PlayOffset(positionType[\,position])`</b>
///     ,
///     Move a number of entries through the playlist in play order, or start it
///     @param[in] positionType          The number of entries to move (negative goes back)\, or the playlist type.
///     @param[in] position              The number of entries\, if the first parameter is the playlist type (optional).
///                                      When nothing is playing it is the position to start at.
///   }
///   \table_row2_l{
///     <b>`PlayMedia(media[\,isdir][\,1]\,[playoffset=xx])`</b>
///     ,
///     Plays the given media. This can be a playlist\, music\, or video file\, directory\,
///     plugin\, disc image stack\, video file stack or an URL. The optional parameter `\,isdir` can
///     be used for playing a directory. `\,1` will start the media without switching to fullscreen.
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
///     <b>`SubtitleShiftUp([save])`</b>
///     ,
///     Shift the subtitle position up.
///     @param[in] save                  "save" to keep the change permanently (optional)
///   }
///   \table_row2_l{
///     <b>`SubtitleShiftDown([save])`</b>
///     ,
///     Shift the subtitle position down.
///     @param[in] save                  "save" to keep the change permanently (optional)
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
///     plugin\, disc image stack\, video file stack or an URL. The optional parameter `\,isdir` can
///     be used for playing a directory. `\,1` will start the media without switching to fullscreen.
///     If media is a playlist or a disc image stack or a video file stack\, you can use
///     playoffset=xx where xx is the position to start playback from.
///     @param[in] media                 URL of media to queue.
///     @param[in] isdir                 Set `isdir` if media is a directory (optional).
///     @param[in] 1                     Set `1` to start playback without switching to fullscreen (optional).
///     @param[in] resume                Set `resume` to force resuming (optional).
///     @param[in] noresume              Set `noresume` to force not resuming (optional).
///     @param[in] playoffset            Set `playoffset=<offset>` to start playback from a given position in a playlist or stack (optional).
///     @param[in] playnext              Set `playnext` to play the media right after the currently playing item\, if player is currently
///     playing. If player is not playing\, append the media to its playlist (optional).
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
           {"playplaylist",        {"Plays a playlist on the Blu-ray in the disc drive.", 0, PlayPlaylist}},
           {"playlist.clear",      {"Clear the video and music playlists", 0, ClearPlaylist}},
           {"playlist.playoffset", {"Start playing from a particular offset in the playlist", 1, PlayOffset}},
           {"playercontrol",       {"Control the music or video player", 1, PlayerControl}},
           {"playmedia",           {"Play the specified media file (or playlist)", 1, PlayMedia, PlayMediaEx}},
           {"queuemedia",          {"Queue the specified media in video or music playlist", 1, QueueMedia, QueueMediaEx}},
           {"playwith",            {"Play the selected item with the specified core", 1, PlayWith}},
           {"seek",                {"Performs a seek in seconds on the current playing media file", 1, Seek}},
           {"subtitleshiftup",     {"Shift up the subtitle position", 0, SubtitleShiftUp}},
           {"subtitleshiftdown",   {"Shift down the subtitle position", 0, SubtitleShiftDown}},
         };
}
// clang-format on
