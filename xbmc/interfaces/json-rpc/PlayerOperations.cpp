/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PlayerOperations.h"

#include "AudioLibrary.h"
#include "FileItem.h"
#include "FileItemList.h"
#include "GUIInfoManager.h"
#include "GUIUserMessages.h"
#include "PartyModeManager.h"
#include "PlayListPlayer.h"
#include "SeekHandler.h"
#include "ServiceBroker.h"
#include "Util.h"
#include "VideoLibrary.h"
#include "application/Application.h"
#include "application/ApplicationComponents.h"
#include "application/ApplicationPlayer.h"
#include "application/ApplicationPowerHandling.h"
#include "cores/playercorefactory/PlayerCoreFactory.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"
#include "interfaces/builtins/Builtins.h"
#include "messaging/ApplicationMessenger.h"
#include "music/MusicDatabase.h"
#include "music/MusicFileItemClassify.h"
#include "pictures/SlideShowDelegator.h"
#include "pvr/PVRManager.h"
#include "pvr/PVRPlaybackState.h"
#include "pvr/channels/PVRChannel.h"
#include "pvr/channels/PVRChannelGroupMember.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "pvr/epg/EpgContainer.h"
#include "pvr/epg/EpgInfoTag.h"
#include "pvr/guilib/PVRGUIActionsChannels.h"
#include "pvr/guilib/PVRGUIActionsPlayback.h"
#include "pvr/recordings/PVRRecordings.h"
#include "settings/AdvancedSettings.h"
#include "settings/DisplaySettings.h"
#include "settings/MediaSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/MathUtils.h"
#include "utils/PlayerUtils.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "video/VideoDatabase.h"

#include <map>
#include <tuple>

using namespace KODI;
using namespace JSONRPC;
using namespace PVR;

namespace
{

void AppendAudioStreamFlagsAsBooleans(CVariant& list, StreamFlags flags)
{
  list["isdefault"] = ((flags & StreamFlags::FLAG_DEFAULT) != 0);
  list["isoriginal"] = ((flags & StreamFlags::FLAG_ORIGINAL) != 0);
  list["isimpaired"] = ((flags & StreamFlags::FLAG_VISUAL_IMPAIRED) != 0);
}

void AppendSubtitleStreamFlagsAsBooleans(CVariant& list, StreamFlags flags)
{
  list["isdefault"] = ((flags & StreamFlags::FLAG_DEFAULT) != 0);
  list["isforced"] = ((flags & StreamFlags::FLAG_FORCED) != 0);
  list["isimpaired"] = ((flags & StreamFlags::FLAG_HEARING_IMPAIRED) != 0);
}

} // namespace

JSONRPC_STATUS CPlayerOperations::GetActivePlayers(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  int activePlayers = GetActivePlayers();
  result = CVariant(CVariant::VariantTypeArray);

  std::string strPlayerType = "internal";
  if (activePlayers & External)
    strPlayerType = "external";
  else if (activePlayers & Remote)
    strPlayerType = "remote";

  if (activePlayers & Video)
  {
    CVariant video = CVariant(CVariant::VariantTypeObject);
    video["playerid"] = GetPlaylist(Video);
    video["type"] = "video";
    video["playertype"] = strPlayerType;
    result.append(video);
  }
  if (activePlayers & Audio)
  {
    CVariant audio = CVariant(CVariant::VariantTypeObject);
    audio["playerid"] = GetPlaylist(Audio);
    audio["type"] = "audio";
    audio["playertype"] = strPlayerType;
    result.append(audio);
  }
  if (activePlayers & Picture)
  {
    CVariant picture = CVariant(CVariant::VariantTypeObject);
    picture["playerid"] = GetPlaylist(Picture);
    picture["type"] = "picture";
    picture["playertype"] = "internal";
    result.append(picture);
  }

  return OK;
}

JSONRPC_STATUS CPlayerOperations::GetPlayers(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  const CPlayerCoreFactory &playerCoreFactory = CServiceBroker::GetPlayerCoreFactory();

  std::string media = parameterObject["media"].asString();
  result = CVariant(CVariant::VariantTypeArray);
  std::vector<std::string> players;

  if (media == "all")
  {
    playerCoreFactory.GetPlayers(players);
  }
  else
  {
    bool video = false;
    if (media == "video")
      video = true;
    playerCoreFactory.GetPlayers(players, true, video);
  }

  for (const auto& playername : players)
  {
    CVariant player(CVariant::VariantTypeObject);
    player["name"] = playername;

    player["playsvideo"] = playerCoreFactory.PlaysVideo(playername);
    player["playsaudio"] = playerCoreFactory.PlaysAudio(playername);
    player["type"] = playerCoreFactory.GetPlayerType(playername);

    result.push_back(player);
  }

  return OK;
}

JSONRPC_STATUS CPlayerOperations::GetProperties(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  PlayerType player = GetPlayer(parameterObject["playerid"]);

  CVariant properties = CVariant(CVariant::VariantTypeObject);
  for (unsigned int index = 0; index < parameterObject["properties"].size(); index++)
  {
    std::string propertyName = parameterObject["properties"][index].asString();
    CVariant property;
    JSONRPC_STATUS ret;
    if ((ret = GetPropertyValue(player, propertyName, property)) != OK)
      return ret;

    properties[propertyName] = property;
  }

  result = properties;

  return OK;
}

JSONRPC_STATUS CPlayerOperations::GetItem(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  PlayerType player = GetPlayer(parameterObject["playerid"]);
  CFileItemPtr fileItem;

  switch (player)
  {
    case Video:
    case Audio:
    {
      fileItem = std::make_shared<CFileItem>(g_application.CurrentFileItem());
      if (IsPVRChannel())
        break;

      if (player == Video)
      {
        if (!CVideoLibrary::FillFileItem(fileItem->GetPath(), fileItem, parameterObject))
        {
          // Fallback to item details held by GUI but ensure path unchanged
          //! @todo  remove this once there is no route to playback that updates
          // GUI item without also updating app item e.g. start playback of a
          // non-library item via JSON
          const CVideoInfoTag *currentVideoTag = CServiceBroker::GetGUI()->GetInfoManager().GetCurrentMovieTag();
          if (currentVideoTag != NULL)
          {
            std::string originalLabel = fileItem->GetLabel();
            std::string originalPath = fileItem->GetPath();
            fileItem->SetFromVideoInfoTag(*currentVideoTag);
            if (fileItem->GetLabel().empty())
              fileItem->SetLabel(originalLabel);
            fileItem->SetPath(originalPath);   // Ensure path unchanged
          }
        }

        bool additionalInfo = false;
        for (CVariant::const_iterator_array itr = parameterObject["properties"].begin_array();
             itr != parameterObject["properties"].end_array(); ++itr)
        {
          std::string fieldValue = itr->asString();
          if (fieldValue == "cast" || fieldValue == "set" || fieldValue == "setid" || fieldValue == "showlink" || fieldValue == "resume" ||
             (fieldValue == "streamdetails" && !fileItem->GetVideoInfoTag()->m_streamDetails.HasItems()))
            additionalInfo = true;
        }

        CVideoDatabase videodatabase;
        if ((additionalInfo) &&
            videodatabase.Open())
        {
          switch (fileItem->GetVideoContentType())
          {
            case VideoDbContentType::MOVIES:
              videodatabase.GetMovieInfo("", *(fileItem->GetVideoInfoTag()),
                                         fileItem->GetVideoInfoTag()->m_iDbId,
                                         fileItem->GetVideoInfoTag()->GetAssetInfo().GetId());
              break;

            case VideoDbContentType::MUSICVIDEOS:
              videodatabase.GetMusicVideoInfo("", *(fileItem->GetVideoInfoTag()),
                                              fileItem->GetVideoInfoTag()->m_iDbId);
              break;

            case VideoDbContentType::EPISODES:
              videodatabase.GetEpisodeInfo("", *(fileItem->GetVideoInfoTag()),
                                           fileItem->GetVideoInfoTag()->m_iDbId);
              break;

            case VideoDbContentType::TVSHOWS:
            case VideoDbContentType::MOVIE_SETS:
            default:
              break;
          }

          videodatabase.Close();
        }
      }
      else // Audio
      {
        if (!CAudioLibrary::FillFileItem(fileItem->GetPath(), fileItem, parameterObject))
        {
          // Fallback to item details held by GUI but ensure path unchanged
          //! @todo  remove this once there is no route to playback that updates
          // GUI item without also updating app item e.g. start playback of a
          // non-library item via JSON
          const MUSIC_INFO::CMusicInfoTag* currentMusicTag =
              CServiceBroker::GetGUI()->GetInfoManager().GetCurrentSongTag();
          if (currentMusicTag != NULL)
          {
            std::string originalLabel = fileItem->GetLabel();
            std::string originalPath = fileItem->GetPath();
            fileItem->SetFromMusicInfoTag(*currentMusicTag);
            if (fileItem->GetLabel().empty())
              fileItem->SetLabel(originalLabel);
            fileItem->SetPath(originalPath); // Ensure path unchanged
          }
        }

        if (MUSIC::IsMusicDb(*fileItem))
        {
          CMusicDatabase musicdb;
          CFileItemList items;
          items.Add(fileItem);
          CAudioLibrary::GetAdditionalSongDetails(parameterObject, items, musicdb);
        }
      }
      break;
    }

    case Picture:
    {
      CSlideShowDelegator& slideShow = CServiceBroker::GetSlideShowDelegator();
      CFileItemList slides;
      slideShow.GetSlideShowContents(slides);
      fileItem = slides[slideShow.CurrentSlide() - 1];
      break;
    }

    case None:
    default:
      return FailedToExecute;
  }

  HandleFileItem("id", !IsPVRChannel(), "item", fileItem, parameterObject, parameterObject["properties"], result, false);
  return OK;
}

JSONRPC_STATUS CPlayerOperations::PlayPause(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  switch (GetPlayer(parameterObject["playerid"]))
  {
    case Video:
    case Audio:
    {
      auto& components = CServiceBroker::GetAppComponents();
      const auto appPlayer = components.GetComponent<CApplicationPlayer>();
      if (!appPlayer->CanPause())
        return FailedToExecute;

      if (parameterObject["play"].isString())
        CBuiltins::GetInstance().Execute("playercontrol(play)");
      else
      {
        if (parameterObject["play"].asBoolean())
        {
          if (appPlayer->IsPausedPlayback())
            CServiceBroker::GetAppMessenger()->SendMsg(TMSG_MEDIA_PAUSE);
          else if (appPlayer->GetPlaySpeed() != 1)
            appPlayer->SetPlaySpeed(1);
        }
        else if (!appPlayer->IsPausedPlayback())
          CServiceBroker::GetAppMessenger()->SendMsg(TMSG_MEDIA_PAUSE);
      }
      result["speed"] = appPlayer->IsPausedPlayback() ? 0 : (int)lrint(appPlayer->GetPlaySpeed());
      return OK;
    }
    case Picture:
    {
      CSlideShowDelegator& slideShow = CServiceBroker::GetSlideShowDelegator();
      if (slideShow.IsPlaying() && (parameterObject["play"].isString() ||
                                    (parameterObject["play"].isBoolean() &&
                                     parameterObject["play"].asBoolean() == slideShow.IsPaused())))
        SendSlideshowAction(ACTION_PAUSE);

      if (slideShow.IsPlaying() && !slideShow.IsPaused())
        result["speed"] = slideShow.GetDirection();
      else
        result["speed"] = 0;
      return OK;
    }
    case None:
    default:
      return FailedToExecute;
  }
}

JSONRPC_STATUS CPlayerOperations::Stop(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  switch (GetPlayer(parameterObject["playerid"]))
  {
    case Video:
    case Audio:
      CServiceBroker::GetAppMessenger()->PostMsg(
          TMSG_MEDIA_STOP, static_cast<int>(parameterObject["playerid"].asInteger()));
      return ACK;

    case Picture:
      SendSlideshowAction(ACTION_STOP);
      return ACK;

    case None:
    default:
      return FailedToExecute;
  }
}

JSONRPC_STATUS CPlayerOperations::GetAudioDelay(const std::string& method,
                                                ITransportLayer* transport,
                                                IClient* client,
                                                const CVariant& parameterObject,
                                                CVariant& result)
{
  auto& components = CServiceBroker::GetAppComponents();
  const auto appPlayer = components.GetComponent<CApplicationPlayer>();
  result["offset"] = appPlayer->GetVideoSettings().m_AudioDelay;
  return OK;
}

JSONRPC_STATUS CPlayerOperations::SetAudioDelay(const std::string& method,
                                                ITransportLayer* transport,
                                                IClient* client,
                                                const CVariant& parameterObject,
                                                CVariant& result)
{
  switch (GetPlayer(parameterObject["playerid"]))
  {
    case Video:
    {
      auto& components = CServiceBroker::GetAppComponents();
      const auto appPlayer = components.GetComponent<CApplicationPlayer>();
      float videoAudioDelayRange =
          CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_videoAudioDelayRange;

      if (parameterObject["offset"].isDouble())
      {
        float offset = static_cast<float>(parameterObject["offset"].asDouble());
        offset = MathUtils::RoundF(offset, AUDIO_DELAY_STEP);
        if (offset > videoAudioDelayRange)
          offset = videoAudioDelayRange;
        else if (offset < -videoAudioDelayRange)
          offset = -videoAudioDelayRange;

        appPlayer->SetAVDelay(offset);
      }
      else if (parameterObject["offset"].isString())
      {
        CVideoSettings vs = appPlayer->GetVideoSettings();
        if (parameterObject["offset"].asString().compare("increment") == 0)
        {
          vs.m_AudioDelay += AUDIO_DELAY_STEP;
          if (vs.m_AudioDelay > videoAudioDelayRange)
            vs.m_AudioDelay = videoAudioDelayRange;
          appPlayer->SetAVDelay(vs.m_AudioDelay);
        }
        else
        {
          vs.m_AudioDelay -= AUDIO_DELAY_STEP;
          if (vs.m_AudioDelay < -videoAudioDelayRange)
            vs.m_AudioDelay = -videoAudioDelayRange;
          appPlayer->SetAVDelay(vs.m_AudioDelay);
        }
      }
      else
        return InvalidParams;

      result["offset"] = appPlayer->GetVideoSettings().m_AudioDelay;
      return OK;
    }
    case Audio:
    case Picture:
    case None:
    default:
      return FailedToExecute;
  }
}

JSONRPC_STATUS CPlayerOperations::SetSpeed(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  switch (GetPlayer(parameterObject["playerid"]))
  {
    case Video:
    case Audio:
    {
      auto& components = CServiceBroker::GetAppComponents();
      const auto appPlayer = components.GetComponent<CApplicationPlayer>();
      if (parameterObject["speed"].isInteger())
      {
        int speed = (int)parameterObject["speed"].asInteger();
        if (speed != 0)
        {
          // If the player is paused we first need to unpause
          if (appPlayer->IsPausedPlayback())
            appPlayer->Pause();
          appPlayer->SetPlaySpeed(speed);
        }
        else
          appPlayer->Pause();
      }
      else if (parameterObject["speed"].isString())
      {
        if (parameterObject["speed"].asString().compare("increment") == 0)
          CBuiltins::GetInstance().Execute("playercontrol(forward)");
        else
          CBuiltins::GetInstance().Execute("playercontrol(rewind)");
      }
      else
        return InvalidParams;

      result["speed"] = appPlayer->IsPausedPlayback() ? 0 : (int)lrint(appPlayer->GetPlaySpeed());
      return OK;
    }

    case Picture:
    case None:
    default:
      return FailedToExecute;
  }
}

JSONRPC_STATUS CPlayerOperations::SetTempo(const std::string& method,
                                           ITransportLayer* transport,
                                           IClient* client,
                                           const CVariant& parameterObject,
                                           CVariant& result)
{
  switch (GetPlayer(parameterObject["playerid"]))
  {
    case Video:
    case Audio:
    {
      auto& components = CServiceBroker::GetAppComponents();
      const auto appPlayer = components.GetComponent<CApplicationPlayer>();
      if (!appPlayer->SupportsTempo() || appPlayer->IsPausedPlayback())
        return FailedToExecute;

      if (parameterObject["tempo"].isDouble())
        appPlayer->SetTempo(parameterObject["tempo"].asFloat());
      else if (parameterObject["tempo"].isString())
      {
        if (parameterObject["tempo"].asString().compare("increment") == 0)
          CPlayerUtils::AdvanceTempoStep(appPlayer, TempoStepChange::INCREASE);
        else
          CPlayerUtils::AdvanceTempoStep(appPlayer, TempoStepChange::DECREASE);
      }
      else
        return InvalidParams;

      result["tempo"] = appPlayer->GetPlayTempo();
      return OK;
    }

    case Picture:
    case None:
    default:
      return FailedToExecute;
  }
}

JSONRPC_STATUS CPlayerOperations::Seek(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  PlayerType player = GetPlayer(parameterObject["playerid"]);
  switch (player)
  {
    case Video:
    case Audio:
    {
      auto& components = CServiceBroker::GetAppComponents();
      const auto appPlayer = components.GetComponent<CApplicationPlayer>();
      if (!appPlayer->CanSeek())
        return FailedToExecute;

      const CVariant& value = parameterObject["value"];
      if (value.isMember("percentage"))
        g_application.SeekPercentage(value["percentage"].asFloat());
      else if (value.isMember("step"))
      {
        std::string step = value["step"].asString();
        if (step == "smallforward")
          CBuiltins::GetInstance().Execute("playercontrol(smallskipforward)");
        else if (step == "smallbackward")
          CBuiltins::GetInstance().Execute("playercontrol(smallskipbackward)");
        else if (step == "bigforward")
          CBuiltins::GetInstance().Execute("playercontrol(bigskipforward)");
        else if (step == "bigbackward")
          CBuiltins::GetInstance().Execute("playercontrol(bigskipbackward)");
        else
          return InvalidParams;
      }
      else if (value.isMember("seconds"))
        appPlayer->GetSeekHandler().SeekSeconds(static_cast<int>(value["seconds"].asInteger()));
      else if (value.isMember("time"))
        g_application.SeekTime(ParseTimeInSeconds(value["time"]));
      else
        return InvalidParams;

      GetPropertyValue(player, "percentage", result["percentage"]);
      GetPropertyValue(player, "time", result["time"]);
      GetPropertyValue(player, "totaltime", result["totaltime"]);
      return OK;
    }

    case Picture:
    case None:
    default:
      return FailedToExecute;
  }
}

JSONRPC_STATUS CPlayerOperations::Move(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  std::string direction = parameterObject["direction"].asString();
  switch (GetPlayer(parameterObject["playerid"]))
  {
    case Picture:
      if (direction == "left")
        SendSlideshowAction(ACTION_MOVE_LEFT);
      else if (direction == "right")
        SendSlideshowAction(ACTION_MOVE_RIGHT);
      else if (direction == "up")
        SendSlideshowAction(ACTION_MOVE_UP);
      else if (direction == "down")
        SendSlideshowAction(ACTION_MOVE_DOWN);
      else
        return InvalidParams;

      return ACK;

    case Video:
    case Audio:
      if (direction == "left" || direction == "up")
        CServiceBroker::GetAppMessenger()->SendMsg(
            TMSG_GUI_ACTION, WINDOW_INVALID, -1, static_cast<void*>(new CAction(ACTION_PREV_ITEM)));
      else if (direction == "right" || direction == "down")
        CServiceBroker::GetAppMessenger()->SendMsg(
            TMSG_GUI_ACTION, WINDOW_INVALID, -1, static_cast<void*>(new CAction(ACTION_NEXT_ITEM)));
      else
        return InvalidParams;

      return ACK;

    case None:
    default:
      return FailedToExecute;
  }
}

JSONRPC_STATUS CPlayerOperations::Zoom(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  CVariant zoom = parameterObject["zoom"];
  switch (GetPlayer(parameterObject["playerid"]))
  {
    case Picture:
      if (zoom.isInteger())
        SendSlideshowAction(ACTION_ZOOM_LEVEL_NORMAL + ((int)zoom.asInteger() - 1));
      else if (zoom.isString())
      {
        std::string strZoom = zoom.asString();
        if (strZoom == "in")
          SendSlideshowAction(ACTION_ZOOM_IN);
        else if (strZoom == "out")
          SendSlideshowAction(ACTION_ZOOM_OUT);
        else
          return InvalidParams;
      }
      else
        return InvalidParams;

      return ACK;

    case Video:
    case Audio:
    case None:
    default:
      return FailedToExecute;
  }
}

// Matching pairs of values from JSON type "Player.ViewMode" and C++ enum ViewMode
// Additions to enum ViewMode need to be added here and in the JSON type
std::map<std::string, ViewMode> viewModes =
{
  {"normal", ViewModeNormal},
  {"zoom", ViewModeZoom},
  {"stretch4x3", ViewModeStretch4x3},
  {"widezoom", ViewModeWideZoom, },
  {"stretch16x9", ViewModeStretch16x9},
  {"original", ViewModeOriginal},
  {"stretch16x9nonlin", ViewModeStretch16x9Nonlin},
  {"zoom120width", ViewModeZoom120Width},
  {"zoom110width", ViewModeZoom110Width}
};

std::string GetStringFromViewMode(ViewMode viewMode)
{
  std::string result = "custom";

  auto it = find_if(viewModes.begin(), viewModes.end(), [viewMode](const std::pair<std::string, ViewMode> & p)
  {
    return p.second == viewMode;
  });

  if (it != viewModes.end())
  {
    std::pair<std::string, ViewMode> value = *it;
    result = value.first;
  }

  return result;
}

void GetNewValueForViewModeParameter(const CVariant &parameter, float stepSize, float minValue, float maxValue, float &result)
{
  if (parameter.isDouble())
  {
    result = parameter.asDouble();
  }
  else if (parameter.isString())
  {
    if (parameter == "decrease")
    {
      stepSize *= -1;
    }

    result += stepSize;
  }

  result = std::max(minValue, std::min(result, maxValue));
}

JSONRPC_STATUS CPlayerOperations::SetViewMode(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  JSONRPC_STATUS jsonStatus = InvalidParams;
  // init with current values from settings
  auto& components = CServiceBroker::GetAppComponents();
  const auto appPlayer = components.GetComponent<CApplicationPlayer>();

  CVideoSettings vs = appPlayer->GetVideoSettings();
  ViewMode mode = ViewModeNormal;

  CVariant viewMode = parameterObject["viewmode"];
  if (viewMode.isString())
  {
    std::string modestr = viewMode.asString();
    if (viewModes.find(modestr) != viewModes.end())
    {
      mode = viewModes[modestr];
      jsonStatus = ACK;
    }
  }
  else if (viewMode.isObject())
  {
    mode = ViewModeCustom;
    CVariant zoom = viewMode["zoom"];
    CVariant pixelRatio = viewMode["pixelratio"];
    CVariant verticalShift = viewMode["verticalshift"];
    CVariant stretch = viewMode["nonlinearstretch"];

    if (!zoom.isNull())
    {
      GetNewValueForViewModeParameter(zoom, 0.01f, 0.5f, 2.f, vs.m_CustomZoomAmount);
      jsonStatus = ACK;
    }

    if (!pixelRatio.isNull())
    {
      GetNewValueForViewModeParameter(pixelRatio, 0.01f, 0.5f, 2.f, vs.m_CustomPixelRatio);
      jsonStatus = ACK;
    }

    if (!verticalShift.isNull())
    {
      GetNewValueForViewModeParameter(verticalShift, -0.01f, -2.f, 2.f, vs.m_CustomVerticalShift);
      jsonStatus = ACK;
    }

    if (stretch.isBoolean())
    {
      vs.m_CustomNonLinStretch = stretch.asBoolean();
      jsonStatus = ACK;
    }
  }

  if (jsonStatus == ACK)
  {
    appPlayer->SetRenderViewMode(static_cast<int>(mode), vs.m_CustomZoomAmount,
                                 vs.m_CustomPixelRatio, vs.m_CustomVerticalShift,
                                 vs.m_CustomNonLinStretch);
  }

  return jsonStatus;
}

JSONRPC_STATUS CPlayerOperations::GetViewMode(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  const auto& components = CServiceBroker::GetAppComponents();
  const auto appPlayer = components.GetComponent<CApplicationPlayer>();

  int mode = appPlayer->GetVideoSettings().m_ViewMode;

  result["viewmode"] = GetStringFromViewMode(static_cast<ViewMode>(mode));

  result["zoom"] = CDisplaySettings::GetInstance().GetZoomAmount();
  result["pixelratio"] = CDisplaySettings::GetInstance().GetPixelRatio();
  result["verticalshift"] = CDisplaySettings::GetInstance().GetVerticalShift();
  result["nonlinearstretch"] = CDisplaySettings::GetInstance().IsNonLinearStretched();
  return OK;
}

JSONRPC_STATUS CPlayerOperations::Rotate(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  switch (GetPlayer(parameterObject["playerid"]))
  {
    case Picture:
      if (parameterObject["value"].asString().compare("clockwise") == 0)
        SendSlideshowAction(ACTION_ROTATE_PICTURE_CW);
      else
        SendSlideshowAction(ACTION_ROTATE_PICTURE_CCW);
      return ACK;

    case Video:
    case Audio:
    case None:
    default:
      return FailedToExecute;
  }
}

JSONRPC_STATUS CPlayerOperations::Open(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  CVariant options = parameterObject["options"];
  CVariant optionShuffled = options["shuffled"];
  CVariant optionRepeat = options["repeat"];
  CVariant optionResume = options["resume"];
  CVariant optionPlayer = options["playername"];

  if (parameterObject["item"].isMember("playlistid"))
  {
    PLAYLIST::Id playlistid = parameterObject["item"]["playlistid"].asInteger();

    if (playlistid == PLAYLIST::TYPE_MUSIC || playlistid == PLAYLIST::TYPE_VIDEO)
    {
      // Apply the "shuffled" option if available
      if (optionShuffled.isBoolean())
        CServiceBroker::GetPlaylistPlayer().SetShuffle(playlistid, optionShuffled.asBoolean(), false);
      // Apply the "repeat" option if available
      if (!optionRepeat.isNull())
        CServiceBroker::GetPlaylistPlayer().SetRepeat(playlistid, ParseRepeatState(optionRepeat),
                                                      false);
    }

    int playlistStartPosition = (int)parameterObject["item"]["position"].asInteger();

    switch (playlistid)
    {
      case PLAYLIST::TYPE_MUSIC:
      case PLAYLIST::TYPE_VIDEO:
        CServiceBroker::GetAppMessenger()->PostMsg(TMSG_MEDIA_PLAY, playlistid,
                                                   playlistStartPosition);
        break;

      case PLAYLIST::TYPE_PICTURE:
      {
        std::string firstPicturePath;
        if (playlistStartPosition > 0)
        {
          CSlideShowDelegator& slideShow = CServiceBroker::GetSlideShowDelegator();
          CFileItemList list;
          slideShow.GetSlideShowContents(list);
          if (playlistStartPosition < list.Size())
            firstPicturePath = list.Get(playlistStartPosition)->GetPath();
        }

        return StartSlideshow("", false, optionShuffled.isBoolean() && optionShuffled.asBoolean(), firstPicturePath);
        break;
      }
    }

    return ACK;
  }
  else if (parameterObject["item"].isMember("path"))
  {
    bool random = (optionShuffled.isBoolean() && optionShuffled.asBoolean()) ||
                  (!optionShuffled.isBoolean() && parameterObject["item"]["random"].asBoolean());
    return StartSlideshow(parameterObject["item"]["path"].asString(), parameterObject["item"]["recursive"].asBoolean(), random);
  }
  else if (parameterObject["item"].isObject() && parameterObject["item"].isMember("partymode"))
  {
    if (g_partyModeManager.IsEnabled())
      g_partyModeManager.Disable();
    CServiceBroker::GetAppMessenger()->PostMsg(
        TMSG_EXECUTE_BUILT_IN, -1, -1, nullptr,
        "playercontrol(partymode(" + parameterObject["item"]["partymode"].asString() + "))");
    return ACK;
  }
  else if (parameterObject["item"].isMember("broadcastid"))
  {
    const std::shared_ptr<CPVREpgInfoTag> epgTag =
        CServiceBroker::GetPVRManager().EpgContainer().GetTagByDatabaseId(
            static_cast<unsigned int>(parameterObject["item"]["broadcastid"].asInteger()));

    if (!epgTag || !epgTag->IsPlayable())
      return InvalidParams;

    if (!CServiceBroker::GetPVRManager().Get<PVR::GUI::Playback>().PlayEpgTag(CFileItem(epgTag)))
      return FailedToExecute;

    return ACK;
  }
  else if (parameterObject["item"].isMember("channelid"))
  {
    const std::shared_ptr<const CPVRChannelGroupsContainer> channelGroupContainer =
        CServiceBroker::GetPVRManager().ChannelGroups();
    if (!channelGroupContainer)
      return FailedToExecute;

    const std::shared_ptr<const CPVRChannel> channel = channelGroupContainer->GetChannelById(
        static_cast<int>(parameterObject["item"]["channelid"].asInteger()));
    if (!channel)
      return InvalidParams;

    const std::shared_ptr<CPVRChannelGroupMember> groupMember =
        CServiceBroker::GetPVRManager().Get<PVR::GUI::Channels>().GetChannelGroupMember(channel);
    if (!groupMember)
      return InvalidParams;

    if (!CServiceBroker::GetPVRManager().Get<PVR::GUI::Playback>().PlayMedia(
            CFileItem(groupMember)))
      return FailedToExecute;

    return ACK;
  }
  else if (parameterObject["item"].isMember("recordingid"))
  {
    const std::shared_ptr<const CPVRRecordings> recordingsContainer =
        CServiceBroker::GetPVRManager().Recordings();
    if (!recordingsContainer)
      return FailedToExecute;

    const std::shared_ptr<CPVRRecording> recording = recordingsContainer->GetById(static_cast<int>(parameterObject["item"]["recordingid"].asInteger()));
    if (!recording)
      return InvalidParams;

    if (!CServiceBroker::GetPVRManager().Get<PVR::GUI::Playback>().PlayMedia(CFileItem(recording)))
      return FailedToExecute;

    return ACK;
  }
  else
  {
    CFileItemList list;
    if (FillFileItemList(parameterObject["item"], list) && list.Size() > 0)
    {
      bool slideshow = true;
      for (int index = 0; index < list.Size(); index++)
      {
        if (!list[index]->IsPicture())
        {
          slideshow = false;
          break;
        }
      }

      if (slideshow)
      {
        //! @todo: This should be a delegator method instead of going via GUI!
        //! look into triggering stop from Reset() itself!
        SendSlideshowAction(ACTION_STOP);
        CSlideShowDelegator& slideShow = CServiceBroker::GetSlideShowDelegator();
        slideShow.Reset();
        for (int index = 0; index < list.Size(); index++)
          slideShow.Add(list[index].get());

        return StartSlideshow("", false, optionShuffled.isBoolean() && optionShuffled.asBoolean());
      }
      else if (list.Size() == 1 && (URIUtils::IsPVRChannel(list[0]->GetPath()) ||
                                    URIUtils::IsPVRRecording(list[0]->GetPath())))
      {
        if (!CServiceBroker::GetPVRManager().Get<PVR::GUI::Playback>().PlayMedia(*list[0]))
          return FailedToExecute;
      }
      else
      {
        std::string playername;
        // Handle the "playerid" option
        if (!optionPlayer.isNull())
        {
          if (optionPlayer.isString())
          {
            playername = optionPlayer.asString();

            if (playername != "default")
            {
              const CPlayerCoreFactory &playerCoreFactory = CServiceBroker::GetPlayerCoreFactory();

              // check if the there's actually a player with the given name
              if (playerCoreFactory.GetPlayerType(playername).empty())
                return InvalidParams;

              // check if the player can handle at least the first item in the list
              std::vector<std::string> possiblePlayers;
              playerCoreFactory.GetPlayers(*list.Get(0).get(), possiblePlayers);

              bool match = false;
              for (const auto& entry : possiblePlayers)
              {
                if (StringUtils::EqualsNoCase(entry, playername))
                {
                  match = true;
                  break;
                }
              }
              if (!match)
                return InvalidParams;
            }
          }
          else
            return InvalidParams;
        }

        // Handle "shuffled" option
        if (optionShuffled.isBoolean())
          list.SetProperty("shuffled", optionShuffled);
        // Handle "repeat" option
        if (!optionRepeat.isNull())
          list.SetProperty("repeat", static_cast<int>(ParseRepeatState(optionRepeat)));
        // Handle "resume" option
        if (list.Size() == 1)
        {
          if (optionResume.isBoolean() && optionResume.asBoolean())
            list[0]->SetStartOffset(STARTOFFSET_RESUME);
          else if (optionResume.isDouble())
            list[0]->SetProperty("StartPercent", optionResume);
          else if (optionResume.isObject())
            list[0]->SetStartOffset(
                CUtil::ConvertSecsToMilliSecs(ParseTimeInSeconds(optionResume)));
        }

        auto l = new CFileItemList(); //don't delete
        l->Copy(list);
        CServiceBroker::GetAppMessenger()->PostMsg(TMSG_MEDIA_PLAY, -1, -1, static_cast<void*>(l),
                                                   playername);
      }

      return ACK;
    }
    else
      return InvalidParams;
  }

  return InvalidParams;
}

JSONRPC_STATUS CPlayerOperations::GoTo(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  CVariant to = parameterObject["to"];
  switch (GetPlayer(parameterObject["playerid"]))
  {
    case Video:
    case Audio:
      if (to.isString())
      {
        std::string strTo = to.asString();
        int actionID;
        if (strTo == "previous")
          actionID = ACTION_PREV_ITEM;
        else if (strTo == "next")
          actionID = ACTION_NEXT_ITEM;
        else
          return InvalidParams;

        CServiceBroker::GetAppMessenger()->SendMsg(TMSG_GUI_ACTION, WINDOW_INVALID, -1,
                                                   static_cast<void*>(new CAction(actionID)));
      }
      else if (to.isInteger())
      {
        if (IsPVRChannel())
          CServiceBroker::GetAppMessenger()->SendMsg(
              TMSG_GUI_ACTION, WINDOW_INVALID, -1,
              static_cast<void*>(
                  new CAction(ACTION_CHANNEL_SWITCH, static_cast<float>(to.asInteger()))));
        else
          CServiceBroker::GetAppMessenger()->SendMsg(TMSG_PLAYLISTPLAYER_PLAY,
                                                     static_cast<int>(to.asInteger()));
      }
      else
        return InvalidParams;
      break;

    case Picture:
      if (to.isString())
      {
        std::string strTo = to.asString();
        int actionID;
        if (strTo == "previous")
          actionID = ACTION_PREV_PICTURE;
        else if (strTo == "next")
          actionID = ACTION_NEXT_PICTURE;
        else
          return InvalidParams;

        SendSlideshowAction(actionID);
      }
      else
        return FailedToExecute;
      break;

    case None:
    default:
      return FailedToExecute;
  }

  return ACK;
}

JSONRPC_STATUS CPlayerOperations::SetShuffle(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  CVariant shuffle = parameterObject["shuffle"];
  switch (GetPlayer(parameterObject["playerid"]))
  {
    case Video:
    case Audio:
    {
      if (IsPVRChannel())
        return FailedToExecute;

      PLAYLIST::Id playlistid = GetPlaylist(GetPlayer(parameterObject["playerid"]));
      if (CServiceBroker::GetPlaylistPlayer().IsShuffled(playlistid))
      {
        if ((shuffle.isBoolean() && !shuffle.asBoolean()) ||
            (shuffle.isString() && shuffle.asString() == "toggle"))
        {
          CServiceBroker::GetAppMessenger()->SendMsg(TMSG_PLAYLISTPLAYER_SHUFFLE, playlistid, 0);
        }
      }
      else
      {
        if ((shuffle.isBoolean() && shuffle.asBoolean()) ||
            (shuffle.isString() && shuffle.asString() == "toggle"))
        {
          CServiceBroker::GetAppMessenger()->SendMsg(TMSG_PLAYLISTPLAYER_SHUFFLE, playlistid, 1);
        }
      }
      break;
    }

    case Picture:
    {
      CSlideShowDelegator& slideShow = CServiceBroker::GetSlideShowDelegator();
      if (slideShow.NumSlides() < 0)
      {
        return FailedToExecute;
      }

      if (slideShow.IsShuffled())
      {
        if ((shuffle.isBoolean() && !shuffle.asBoolean()) ||
            (shuffle.isString() && shuffle.asString() == "toggle"))
          return FailedToExecute;
      }
      else
      {
        if ((shuffle.isBoolean() && shuffle.asBoolean()) ||
            (shuffle.isString() && shuffle.asString() == "toggle"))
          slideShow.Shuffle();
      }
      break;
    }
    default:
      return FailedToExecute;
  }
  return ACK;
}

JSONRPC_STATUS CPlayerOperations::SetRepeat(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  switch (GetPlayer(parameterObject["playerid"]))
  {
    case Video:
    case Audio:
    {
      if (IsPVRChannel())
        return FailedToExecute;

      PLAYLIST::RepeatState repeat = PLAYLIST::RepeatState::NONE;
      PLAYLIST::Id playlistid = GetPlaylist(GetPlayer(parameterObject["playerid"]));
      if (parameterObject["repeat"].asString() == "cycle")
      {
        PLAYLIST::RepeatState repeatPrev =
            CServiceBroker::GetPlaylistPlayer().GetRepeat(playlistid);
        if (repeatPrev == PLAYLIST::RepeatState::NONE)
          repeat = PLAYLIST::RepeatState::ALL;
        else if (repeatPrev == PLAYLIST::RepeatState::ALL)
          repeat = PLAYLIST::RepeatState::ONE;
        else
          repeat = PLAYLIST::RepeatState::NONE;
      }
      else
        repeat = ParseRepeatState(parameterObject["repeat"]);

      CServiceBroker::GetAppMessenger()->SendMsg(TMSG_PLAYLISTPLAYER_REPEAT, playlistid,
                                                 static_cast<int>(repeat));
      break;
    }

    case Picture:
    default:
      return FailedToExecute;
  }

  return ACK;
}

JSONRPC_STATUS CPlayerOperations::SetPartymode(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  PlayerType player = GetPlayer(parameterObject["playerid"]);
  switch (player)
  {
    case Video:
    case Audio:
    {
      if (IsPVRChannel())
        return FailedToExecute;

      bool change = false;
      PartyModeContext context = PARTYMODECONTEXT_UNKNOWN;
      std::string strContext;
      if (player == Video)
      {
        context = PARTYMODECONTEXT_VIDEO;
        strContext = "video";
      }
      else if (player == Audio)
      {
        context = PARTYMODECONTEXT_MUSIC;
        strContext = "music";
      }

      bool toggle = parameterObject["partymode"].isString();
      if (g_partyModeManager.IsEnabled())
      {
        if (g_partyModeManager.GetType() != context)
          return InvalidParams;

        if (toggle || parameterObject["partymode"].asBoolean() == false)
          change = true;
      }
      else
      {
        if (toggle || parameterObject["partymode"].asBoolean() == true)
          change = true;
      }

      if (change)
        CServiceBroker::GetAppMessenger()->PostMsg(TMSG_EXECUTE_BUILT_IN, -1, -1, nullptr,
                                                   "playercontrol(partymode(" + strContext + "))");
      break;
    }

    case Picture:
    default:
      return FailedToExecute;
  }

  return ACK;
}

JSONRPC_STATUS CPlayerOperations::SetAudioStream(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  switch (GetPlayer(parameterObject["playerid"]))
  {
    case Video:
    {
      auto& components = CServiceBroker::GetAppComponents();
      const auto appPlayer = components.GetComponent<CApplicationPlayer>();
      if (appPlayer->HasPlayer())
      {
        int index = -1;
        if (parameterObject["stream"].isString())
        {
          std::string action = parameterObject["stream"].asString();
          if (action.compare("previous") == 0)
          {
            index = appPlayer->GetAudioStream() - 1;
            if (index < 0)
              index = appPlayer->GetAudioStreamCount() - 1;
          }
          else if (action.compare("next") == 0)
          {
            index = appPlayer->GetAudioStream() + 1;
            if (index >= appPlayer->GetAudioStreamCount())
              index = 0;
          }
          else
            return InvalidParams;
        }
        else if (parameterObject["stream"].isInteger())
          index = (int)parameterObject["stream"].asInteger();

        if (index < 0 || appPlayer->GetAudioStreamCount() <= index)
          return InvalidParams;

        appPlayer->SetAudioStream(index);
      }
      else
        return FailedToExecute;
      break;
    }

    case Audio:
    case Picture:
    default:
      return FailedToExecute;
  }

  return ACK;
}

JSONRPC_STATUS CPlayerOperations::AddSubtitle(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  if (GetPlayer(parameterObject["playerid"]) != Video)
    return FailedToExecute;

  auto& components = CServiceBroker::GetAppComponents();
  const auto appPlayer = components.GetComponent<CApplicationPlayer>();

  if (!appPlayer->HasPlayer())
    return FailedToExecute;

  if (!parameterObject["subtitle"].isString())
    return FailedToExecute;

  std::string sub = parameterObject["subtitle"].asString();
  appPlayer->AddSubtitle(sub);
  return ACK;
}

JSONRPC_STATUS CPlayerOperations::SetSubtitle(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  switch (GetPlayer(parameterObject["playerid"]))
  {
    case Video:
    {
      auto& components = CServiceBroker::GetAppComponents();
      const auto appPlayer = components.GetComponent<CApplicationPlayer>();
      if (appPlayer->HasPlayer())
      {
        int index = -1;
        if (parameterObject["subtitle"].isString())
        {
          std::string action = parameterObject["subtitle"].asString();
          if (action.compare("previous") == 0)
          {
            index = appPlayer->GetSubtitle() - 1;
            if (index < 0)
              index = appPlayer->GetSubtitleCount() - 1;
          }
          else if (action.compare("next") == 0)
          {
            index = appPlayer->GetSubtitle() + 1;
            if (index >= appPlayer->GetSubtitleCount())
              index = 0;
          }
          else if (action.compare("off") == 0)
          {
            appPlayer->SetSubtitleVisible(false);
            return ACK;
          }
          else if (action.compare("on") == 0)
          {
            appPlayer->SetSubtitleVisible(true);
            return ACK;
          }
          else
            return InvalidParams;
        }
        else if (parameterObject["subtitle"].isInteger())
          index = (int)parameterObject["subtitle"].asInteger();

        if (index < 0 || appPlayer->GetSubtitleCount() <= index)
          return InvalidParams;

        appPlayer->SetSubtitle(index);

        // Check if we need to enable subtitles to be displayed
        if (parameterObject["enable"].asBoolean() && !appPlayer->GetSubtitleVisible())
          appPlayer->SetSubtitleVisible(true);
      }
      else
        return FailedToExecute;
      break;
    }

    case Audio:
    case Picture:
    default:
      return FailedToExecute;
  }

  return ACK;
}

JSONRPC_STATUS CPlayerOperations::SetVideoStream(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  switch (GetPlayer(parameterObject["playerid"]))
  {
  case Video:
  {
    auto& components = CServiceBroker::GetAppComponents();
    const auto appPlayer = components.GetComponent<CApplicationPlayer>();
    int streamCount = appPlayer->GetVideoStreamCount();
    if (streamCount > 0)
    {
      int index = appPlayer->GetVideoStream();
      if (parameterObject["stream"].isString())
      {
        std::string action = parameterObject["stream"].asString();
        if (action.compare("previous") == 0)
        {
          index--;
          if (index < 0)
            index = streamCount - 1;
        }
        else if (action.compare("next") == 0)
        {
          index++;
          if (index >= streamCount)
            index = 0;
        }
        else
          return InvalidParams;
      }
      else if (parameterObject["stream"].isInteger())
        index = (int)parameterObject["stream"].asInteger();

      if (index < 0 || streamCount <= index)
        return InvalidParams;

      appPlayer->SetVideoStream(index);
    }
    else
      return FailedToExecute;
    break;
  }
  case Audio:
  case Picture:
  default:
    return FailedToExecute;
  }

  return ACK;
}

int CPlayerOperations::GetActivePlayers()
{
  const auto& components = CServiceBroker::GetAppComponents();
  const auto appPlayer = components.GetComponent<CApplicationPlayer>();

  int activePlayers = 0;
  if (appPlayer->IsPlayingVideo() ||
      CServiceBroker::GetPVRManager().PlaybackState()->IsPlayingTV() ||
      CServiceBroker::GetPVRManager().PlaybackState()->IsPlayingRecording())
    activePlayers |= Video;
  if (appPlayer->IsPlayingAudio() ||
      CServiceBroker::GetPVRManager().PlaybackState()->IsPlayingRadio())
    activePlayers |= Audio;
  if (CServiceBroker::GetGUI()->GetWindowManager().IsWindowActive(WINDOW_SLIDESHOW))
    activePlayers |= Picture;
  if (appPlayer->IsExternalPlaying())
    activePlayers |= External;
  if (appPlayer->IsRemotePlaying())
    activePlayers |= Remote;

  return activePlayers;
}

PlayerType CPlayerOperations::GetPlayer(const CVariant &player)
{
  PLAYLIST::Id playerPlaylistId = player.asInteger();
  PlayerType playerID;

  switch (playerPlaylistId)
  {
    case PLAYLIST::TYPE_VIDEO:
      playerID = Video;
      break;

    case PLAYLIST::TYPE_MUSIC:
      playerID = Audio;
      break;

    case PLAYLIST::TYPE_PICTURE:
      playerID = Picture;
      break;

    default:
      playerID = None;
      break;
  }

  if (GetPlaylist(playerID) == playerPlaylistId)
    return playerID;
  else
    return None;
}

PLAYLIST::Id CPlayerOperations::GetPlaylist(PlayerType player)
{
  PLAYLIST::Id playlistId = CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist();
  if (playlistId == PLAYLIST::TYPE_NONE) // No active playlist, try guessing
  {
    const auto& components = CServiceBroker::GetAppComponents();
    const auto appPlayer = components.GetComponent<CApplicationPlayer>();
    playlistId = appPlayer->GetPreferredPlaylist();
  }

  switch (player)
  {
    case Video:
      return playlistId == PLAYLIST::TYPE_NONE ? PLAYLIST::TYPE_VIDEO : playlistId;

    case Audio:
      return playlistId == PLAYLIST::TYPE_NONE ? PLAYLIST::TYPE_MUSIC : playlistId;

    case Picture:
      return PLAYLIST::TYPE_PICTURE;

    default:
      return playlistId;
  }
}

JSONRPC_STATUS CPlayerOperations::StartSlideshow(const std::string& path, bool recursive, bool random, const std::string &firstPicturePath /* = "" */)
{
  int flags = 0;
  if (recursive)
    flags |= 1;
  if (random)
    flags |= 2;
  else
    flags |= 4;

  std::vector<std::string> params;
  params.push_back(path);
  if (!firstPicturePath.empty())
    params.push_back(firstPicturePath);

  // Reset screensaver when started from JSON only to avoid potential conflict with slideshow screensavers
  auto& components = CServiceBroker::GetAppComponents();
  const auto appPower = components.GetComponent<CApplicationPowerHandling>();
  appPower->ResetScreenSaver();
  appPower->WakeUpScreenSaverAndDPMS();
  CGUIMessage msg(GUI_MSG_START_SLIDESHOW, 0, 0, flags);
  msg.SetStringParams(params);
  CServiceBroker::GetAppMessenger()->SendGUIMessage(msg, WINDOW_SLIDESHOW);

  return ACK;
}

void CPlayerOperations::SendSlideshowAction(int actionID)
{
  CServiceBroker::GetAppMessenger()->SendMsg(TMSG_GUI_ACTION, WINDOW_SLIDESHOW, -1,
                                             static_cast<void*>(new CAction(actionID)));
}

JSONRPC_STATUS CPlayerOperations::GetPropertyValue(PlayerType player, const std::string &property, CVariant &result)
{
  if (player == None)
    return FailedToExecute;

  PLAYLIST::Id playlistId = GetPlaylist(player);

  if (property == "type")
  {
    switch (player)
    {
      case Video:
        result = "video";
        break;

      case Audio:
        result = "audio";
        break;

      case Picture:
        result = "picture";
        break;

      default:
        return FailedToExecute;
    }
  }
  else if (property == "partymode")
  {
    switch (player)
    {
      case Video:
      case Audio:
        if (IsPVRChannel())
        {
          result = false;
          break;
        }

        result = g_partyModeManager.IsEnabled();
        break;

      case Picture:
        result = false;
        break;

      default:
        return FailedToExecute;
    }
  }
  else if (property == "speed")
  {
    switch (player)
    {
      case Video:
      case Audio:
      {
        const auto& components = CServiceBroker::GetAppComponents();
        const auto appPlayer = components.GetComponent<CApplicationPlayer>();
        result = appPlayer->IsPausedPlayback() ? 0 : (int)lrint(appPlayer->GetPlaySpeed());
        break;
      }
      case Picture:
      {
        CSlideShowDelegator& slideShow = CServiceBroker::GetSlideShowDelegator();
        if (slideShow.IsPlaying() && !slideShow.IsPaused())
          result = slideShow.GetDirection();
        else
          result = 0;
        break;
      }
      default:
        return FailedToExecute;
    }
  }
  else if (property == "time")
  {
    switch (player)
    {
      case Video:
      case Audio:
      {
        int ms = 0;
        if (!IsPVRChannel())
          ms = (int)(g_application.GetTime() * 1000.0);
        else
        {
          std::shared_ptr<CPVREpgInfoTag> epg(GetCurrentEpg());
          if (epg)
            ms = epg->Progress() * 1000;
        }

        MillisecondsToTimeObject(ms, result);
        break;
      }

      case Picture:
        MillisecondsToTimeObject(0, result);
        break;

      default:
        return FailedToExecute;
    }
  }
  else if (property == "percentage")
  {
    switch (player)
    {
      case Video:
      case Audio:
      {
        if (!IsPVRChannel())
          result = g_application.GetPercentage();
        else
        {
          std::shared_ptr<CPVREpgInfoTag> epg(GetCurrentEpg());
          if (epg)
            result = epg->ProgressPercentage();
          else
            result = 0;
        }
        break;
      }
      case Picture:
      {
        CSlideShowDelegator& slideShow = CServiceBroker::GetSlideShowDelegator();
        if (slideShow.NumSlides() > 0)
          result = static_cast<double>(slideShow.CurrentSlide()) / slideShow.NumSlides();
        else
          result = 0.0;
        break;
      }
      default:
        return FailedToExecute;
    }
  }
  else if (property == "cachepercentage")
  {
    switch (player)
    {
      case Video:
      case Audio:
      {
        result = g_application.GetCachePercentage();
        break;
      }

      case Picture:
      {
        result = 0.0;
        break;
      }

      default:
        return FailedToExecute;
    }
  }
  else if (property == "totaltime")
  {
    switch (player)
    {
      case Video:
      case Audio:
      {
        int ms = 0;
        if (!IsPVRChannel())
          ms = (int)(g_application.GetTotalTime() * 1000.0);
        else
        {
          std::shared_ptr<CPVREpgInfoTag> epg(GetCurrentEpg());
          if (epg)
            ms = epg->GetDuration() * 1000;
        }

        MillisecondsToTimeObject(ms, result);
        break;
      }

      case Picture:
        MillisecondsToTimeObject(0, result);
        break;

      default:
        return FailedToExecute;
    }
  }
  else if (property == "playlistid")
  {
    result = playlistId;
  }
  else if (property == "position")
  {
    switch (player)
    {
      case Video:
      case Audio: /* Return the position of current item if there is an active playlist */
      {
        if (!IsPVRChannel() &&
            CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist() == playlistId)
        {
          result = CServiceBroker::GetPlaylistPlayer().GetCurrentItemIdx();
        }
        else
          result = -1;
        break;
      }
      case Picture:
      {
        CSlideShowDelegator& slideShow = CServiceBroker::GetSlideShowDelegator();
        if (slideShow.IsPlaying())
          result = slideShow.CurrentSlide() - 1;
        else
          result = -1;
        break;
      }
      default:
      {
        result = -1;
        break;
      }
    }
  }
  else if (property == "repeat")
  {
    switch (player)
    {
      case Video:
      case Audio:
        if (IsPVRChannel())
        {
          result = "off";
          break;
        }

        switch (CServiceBroker::GetPlaylistPlayer().GetRepeat(playlistId))
        {
          case PLAYLIST::RepeatState::ONE:
            result = "one";
            break;
          case PLAYLIST::RepeatState::ALL:
            result = "all";
            break;
          default:
            result = "off";
            break;
        }
        break;

      case Picture:
      default:
        result = "off";
        break;
    }
  }
  else if (property == "shuffled")
  {
    switch (player)
    {
      case Video:
      case Audio:
      {
        if (IsPVRChannel())
        {
          result = false;
          break;
        }

        result = CServiceBroker::GetPlaylistPlayer().IsShuffled(playlistId);
        break;
      }
      case Picture:
      {
        CSlideShowDelegator& slideShow = CServiceBroker::GetSlideShowDelegator();
        if (slideShow.IsPlaying())
          result = slideShow.IsShuffled();
        else
          result = -1;
        break;
      }
      default:
      {
        result = -1;
        break;
      }
    }
  }
  else if (property == "canseek")
  {
    switch (player)
    {
      case Video:
      case Audio:
      {
        const auto& components = CServiceBroker::GetAppComponents();
        const auto appPlayer = components.GetComponent<CApplicationPlayer>();
        result = appPlayer->CanSeek();
        break;
      }

      case Picture:
      default:
        result = false;
        break;
    }
  }
  else if (property == "canchangespeed")
  {
    switch (player)
    {
      case Video:
      case Audio:
        result = !IsPVRChannel();
        break;

      case Picture:
      default:
        result = false;
        break;
    }
  }
  else if (property == "canmove")
  {
    switch (player)
    {
      case Picture:
        result = true;
        break;

      case Video:
      case Audio:
      default:
        result = false;
        break;
    }
  }
  else if (property == "canzoom")
  {
    switch (player)
    {
      case Picture:
        result = true;
        break;

      case Video:
      case Audio:
      default:
        result = false;
        break;
    }
  }
  else if (property == "canrotate")
  {
    switch (player)
    {
      case Picture:
        result = true;
        break;

      case Video:
      case Audio:
      default:
        result = false;
        break;
    }
  }
  else if (property == "canshuffle")
  {
    switch (player)
    {
      case Video:
      case Audio:
      case Picture:
        result = !IsPVRChannel();
        break;

      default:
        result = false;
        break;
    }
  }
  else if (property == "canrepeat")
  {
    switch (player)
    {
      case Video:
      case Audio:
        result = !IsPVRChannel();
        break;

      case Picture:
      default:
        result = false;
        break;
    }
  }
  else if (property == "currentaudiostream")
  {
    switch (player)
    {
      case Video:
      case Audio:
      {
        auto& components = CServiceBroker::GetAppComponents();
        const auto appPlayer = components.GetComponent<CApplicationPlayer>();
        if (appPlayer->HasPlayer())
        {
          result = CVariant(CVariant::VariantTypeObject);
          int index = appPlayer->GetAudioStream();
          if (index >= 0)
          {
            AudioStreamInfo info;
            appPlayer->GetAudioStreamInfo(index, info);

            result["index"] = index;
            result["name"] = info.name;
            result["language"] = info.language;
            result["codec"] = info.codecName;
            result["bitrate"] = info.bitrate;
            result["channels"] = info.channels;
            result["samplerate"] = info.samplerate;
            AppendAudioStreamFlagsAsBooleans(result, info.flags);
          }
        }
        else
          result = CVariant(CVariant::VariantTypeNull);
        break;
      }

      case Picture:
      default:
        result = CVariant(CVariant::VariantTypeNull);
        break;
    }
  }
  else if (property == "audiostreams")
  {
    result = CVariant(CVariant::VariantTypeArray);
    switch (player)
    {
      case Video:
      case Audio:
      {
        auto& components = CServiceBroker::GetAppComponents();
        const auto appPlayer = components.GetComponent<CApplicationPlayer>();
        if (appPlayer->HasPlayer())
        {
          for (int index = 0; index < appPlayer->GetAudioStreamCount(); index++)
          {
            AudioStreamInfo info;
            appPlayer->GetAudioStreamInfo(index, info);

            CVariant audioStream(CVariant::VariantTypeObject);
            audioStream["index"] = index;
            audioStream["name"] = info.name;
            audioStream["language"] = info.language;
            audioStream["codec"] = info.codecName;
            audioStream["bitrate"] = info.bitrate;
            audioStream["channels"] = info.channels;
            audioStream["samplerate"] = info.samplerate;
            AppendAudioStreamFlagsAsBooleans(audioStream, info.flags);

            result.append(audioStream);
          }
        }
        break;
      }

      case Picture:
      default:
        break;
    }
  }
  else if (property == "currentvideostream")
  {
    switch (player)
    {
    case Video:
    {
      auto& components = CServiceBroker::GetAppComponents();
      const auto appPlayer = components.GetComponent<CApplicationPlayer>();
      int index = appPlayer->GetVideoStream();
      if (index >= 0)
      {
        result = CVariant(CVariant::VariantTypeObject);
        VideoStreamInfo info;
        appPlayer->GetVideoStreamInfo(index, info);

        result["index"] = index;
        result["name"] = info.name;
        result["language"] = info.language;
        result["codec"] = info.codecName;
        result["width"] = info.width;
        result["height"] = info.height;
      }
      else
        result = CVariant(CVariant::VariantTypeNull);
      break;
    }
    case Audio:
    case Picture:
    default:
      result = CVariant(CVariant::VariantTypeNull);
      break;
    }
  }
  else if (property == "videostreams")
  {
    result = CVariant(CVariant::VariantTypeArray);
    switch (player)
    {
    case Video:
    {
      auto& components = CServiceBroker::GetAppComponents();
      const auto appPlayer = components.GetComponent<CApplicationPlayer>();
      int streamCount = appPlayer->GetVideoStreamCount();
      if (streamCount >= 0)
      {
        for (int index = 0; index < streamCount; ++index)
        {
          VideoStreamInfo info;
          appPlayer->GetVideoStreamInfo(index, info);

          CVariant videoStream(CVariant::VariantTypeObject);
          videoStream["index"] = index;
          videoStream["name"] = info.name;
          videoStream["language"] = info.language;
          videoStream["codec"] = info.codecName;
          videoStream["width"] = info.width;
          videoStream["height"] = info.height;

          result.append(videoStream);
        }
      }
      break;
    }
    case Audio:
    case Picture:
    default:
      break;
    }
  }
  else if (property == "subtitleenabled")
  {
    switch (player)
    {
      case Video:
      {
        auto& components = CServiceBroker::GetAppComponents();
        const auto appPlayer = components.GetComponent<CApplicationPlayer>();
        result = appPlayer->GetSubtitleVisible();
        break;
      }

      case Audio:
      case Picture:
      default:
        result = false;
        break;
    }
  }
  else if (property == "currentsubtitle")
  {
    switch (player)
    {
      case Video:
      {
        auto& components = CServiceBroker::GetAppComponents();
        const auto appPlayer = components.GetComponent<CApplicationPlayer>();
        if (appPlayer->HasPlayer())
        {
          result = CVariant(CVariant::VariantTypeObject);
          int index = appPlayer->GetSubtitle();
          if (index >= 0)
          {
            SubtitleStreamInfo info;
            appPlayer->GetSubtitleStreamInfo(index, info);

            result["index"] = index;
            result["name"] = info.name;
            result["language"] = info.language;
            AppendSubtitleStreamFlagsAsBooleans(result, info.flags);
          }
        }
        else
          result = CVariant(CVariant::VariantTypeNull);
        break;
      }

      case Audio:
      case Picture:
      default:
        result = CVariant(CVariant::VariantTypeNull);
        break;
    }
  }
  else if (property == "subtitles")
  {
    result = CVariant(CVariant::VariantTypeArray);
    switch (player)
    {
      case Video:
      {
        auto& components = CServiceBroker::GetAppComponents();
        const auto appPlayer = components.GetComponent<CApplicationPlayer>();
        if (appPlayer->HasPlayer())
        {
          for (int index = 0; index < appPlayer->GetSubtitleCount(); index++)
          {
            SubtitleStreamInfo info;
            appPlayer->GetSubtitleStreamInfo(index, info);

            CVariant subtitle(CVariant::VariantTypeObject);
            subtitle["index"] = index;
            subtitle["name"] = info.name;
            subtitle["language"] = info.language;
            AppendSubtitleStreamFlagsAsBooleans(subtitle, info.flags);

            result.append(subtitle);
          }
        }
        break;
      }

      case Audio:
      case Picture:
      default:
        break;
    }
  }
  else if (property == "live")
    result = IsPVRChannel();
  else
    return InvalidParams;

  return OK;
}

PLAYLIST::RepeatState CPlayerOperations::ParseRepeatState(const CVariant& repeat)
{
  PLAYLIST::RepeatState state = PLAYLIST::RepeatState::NONE;
  std::string strState = repeat.asString();

  if (strState.compare("one") == 0)
    state = PLAYLIST::RepeatState::ONE;
  else if (strState.compare("all") == 0)
    state = PLAYLIST::RepeatState::ALL;

  return state;
}

double CPlayerOperations::ParseTimeInSeconds(const CVariant &time)
{
  double seconds = 0.0;
  if (time.isMember("hours"))
    seconds += time["hours"].asInteger() * 60 * 60;
  if (time.isMember("minutes"))
    seconds += time["minutes"].asInteger() * 60;
  if (time.isMember("seconds"))
    seconds += time["seconds"].asInteger();
  if (time.isMember("milliseconds"))
    seconds += time["milliseconds"].asDouble() / 1000.0;

  return seconds;
}

bool CPlayerOperations::IsPVRChannel()
{
  const std::shared_ptr<const CPVRPlaybackState> state =
      CServiceBroker::GetPVRManager().PlaybackState();
  return state->IsPlayingTV() || state->IsPlayingRadio();
}

std::shared_ptr<CPVREpgInfoTag> CPlayerOperations::GetCurrentEpg()
{
  const std::shared_ptr<const CPVRPlaybackState> state =
      CServiceBroker::GetPVRManager().PlaybackState();
  if (!state->IsPlayingTV() && !state->IsPlayingRadio())
    return {};

  const std::shared_ptr<const CPVRChannel> currentChannel = state->GetPlayingChannel();
  if (!currentChannel)
    return {};

  return currentChannel->GetEPGNow();
}
