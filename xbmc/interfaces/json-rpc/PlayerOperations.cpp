/*
 *      Copyright (C) 2005-2010 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "PlayerOperations.h"
#include "Application.h"
#include "Util.h"
#include "PlayListPlayer.h"
#include "playlists/PlayList.h"
#include "guilib/GUIWindowManager.h"
#include "GUIUserMessages.h"
#include "pictures/GUIWindowSlideShow.h"
#include "interfaces/Builtins.h"
#include "PartyModeManager.h"
#include "ApplicationMessenger.h"
#include "FileItem.h"
#include "VideoLibrary.h"
#include "video/VideoDatabase.h"
#include "AudioLibrary.h"

using namespace JSONRPC;
using namespace PLAYLIST;

JSON_STATUS CPlayerOperations::GetActivePlayers(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  int activePlayers = GetActivePlayers();
  result = CVariant(CVariant::VariantTypeArray);

  if (activePlayers & Video)
  {
    CVariant video = CVariant(CVariant::VariantTypeObject);
    video["playerid"] = GetPlaylist(Video);
    video["type"] = "video";
    result.append(video);
  }
  if (activePlayers & Audio)
  {
    CVariant audio = CVariant(CVariant::VariantTypeObject);
    audio["playerid"] = GetPlaylist(Audio);
    audio["type"] = "audio";
    result.append(audio);
  }
  if (activePlayers & Picture)
  {
    CVariant picture = CVariant(CVariant::VariantTypeObject);
    picture["playerid"] = GetPlaylist(Picture);
    picture["type"] = "picture";
    result.append(picture);
  }

  return OK;
}

JSON_STATUS CPlayerOperations::GetProperties(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  PlayerType player = GetPlayer(parameterObject["playerid"]);

  CVariant properties = CVariant(CVariant::VariantTypeObject);
  for (unsigned int index = 0; index < parameterObject["properties"].size(); index++)
  {
    CStdString propertyName = parameterObject["properties"][index].asString();
    CVariant property;
    JSON_STATUS ret;
    if ((ret = GetPropertyValue(player, propertyName, property)) != OK)
      return ret;

    properties[propertyName] = property;
  }

  result = properties;

  return OK;
}

JSON_STATUS CPlayerOperations::GetItem(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  PlayerType player = GetPlayer(parameterObject["playerid"]);
  CFileItemPtr fileItem;

  switch (player)
  {
    case Video:
    case Audio:
    {
      if (g_application.CurrentFileItem().GetLabel().empty())
      {
        CFileItem tmpItem;
        if (player == Video)
          CVideoLibrary::FillFileItem(g_application.CurrentFile(), tmpItem);
        else
          CAudioLibrary::FillFileItem(g_application.CurrentFile(), tmpItem);

        fileItem = CFileItemPtr(new CFileItem(tmpItem));
      }
      else
        fileItem = CFileItemPtr(new CFileItem(g_application.CurrentFileItem()));

      if (player == Video)
      {
        bool additionalInfo = false;
        for (CVariant::const_iterator_array itr = parameterObject["properties"].begin_array(); itr != parameterObject["properties"].end_array(); itr++)
        {
          CStdString fieldValue = itr->asString();
          if (fieldValue == "cast" || fieldValue == "set" || fieldValue == "setid" || fieldValue == "showlink" || fieldValue == "resume")
            additionalInfo = true;
        }

        if (additionalInfo)
        {
          CVideoDatabase videodatabase;
          if (videodatabase.Open())
          {
            switch (fileItem->GetVideoContentType())
            {
              case VIDEODB_CONTENT_MOVIES:
                videodatabase.GetMovieInfo("", *(fileItem->GetVideoInfoTag()), fileItem->GetVideoInfoTag()->m_iDbId);
                break;

              case VIDEODB_CONTENT_MUSICVIDEOS:
                videodatabase.GetMusicVideoInfo("", *(fileItem->GetVideoInfoTag()), fileItem->GetVideoInfoTag()->m_iDbId);
                break;

              case VIDEODB_CONTENT_EPISODES:
                videodatabase.GetEpisodeInfo("", *(fileItem->GetVideoInfoTag()), fileItem->GetVideoInfoTag()->m_iDbId);
                break;

              case VIDEODB_CONTENT_TVSHOWS:
              case VIDEODB_CONTENT_MOVIE_SETS:
              default:
                break;
            }

            videodatabase.Close();
          }
        }
      }
      break;
    }

    case Picture:
    {
      CGUIWindowSlideShow *slideshow = (CGUIWindowSlideShow*)g_windowManager.GetWindow(WINDOW_SLIDESHOW);
      if (!slideshow)
        return FailedToExecute;

      CFileItemList slides;
      slideshow->GetSlideShowContents(slides);
      fileItem = slides[slideshow->CurrentSlide() - 1];
      break;
    }

    case None:
    default:
      return FailedToExecute;
  }

  HandleFileItem("id", true, "item", fileItem, parameterObject, parameterObject["properties"], result, false);
  return OK;
}

JSON_STATUS CPlayerOperations::PlayPause(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  CGUIWindowSlideShow *slideshow = NULL;
  switch (GetPlayer(parameterObject["playerid"]))
  {
    case Video:
    case Audio:
      CBuiltins::Execute("playercontrol(play)");
      result["speed"] = g_application.IsPaused() ? 0 : g_application.GetPlaySpeed();
      return OK;

    case Picture:
      SendSlideshowAction(ACTION_PAUSE);
      slideshow = (CGUIWindowSlideShow*)g_windowManager.GetWindow(WINDOW_SLIDESHOW);
      if (slideshow && slideshow->IsPlaying() && !slideshow->IsPaused())
        result["speed"] = slideshow->GetDirection();
      else
        result["speed"] = 0;
      return OK;

    case None:
    default:
      return FailedToExecute;
  }
}

JSON_STATUS CPlayerOperations::Stop(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  switch (GetPlayer(parameterObject["playerid"]))
  {
    case Video:
    case Audio:
      g_application.getApplicationMessenger().SendAction(CAction(ACTION_STOP));
      return ACK;

    case Picture:
      SendSlideshowAction(ACTION_STOP);
      return ACK;

    case None:
    default:
      return FailedToExecute;
  }
}

JSON_STATUS CPlayerOperations::SetSpeed(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  int speed;
  switch (GetPlayer(parameterObject["playerid"]))
  {
    case Video:
    case Audio:
      if (parameterObject["speed"].isInteger())
      {
        speed = (int)parameterObject["speed"].asInteger();
        if (speed != 0)
        {
          // If the player is paused we first need to unpause
          if (g_application.IsPaused())
            g_application.m_pPlayer->Pause();
          g_application.SetPlaySpeed(speed);
        }
        else
          g_application.m_pPlayer->Pause();
      }
      else if (parameterObject["speed"].isString())
      {
        speed = g_application.GetPlaySpeed();
        if (parameterObject["speed"].asString().compare("increment") == 0)
          CBuiltins::Execute("playercontrol(forward)");
        else
          CBuiltins::Execute("playercontrol(rewind)");
      }
      else
        return InvalidParams;

      result["speed"] = g_application.IsPaused() ? 0 : g_application.GetPlaySpeed();
      return OK;

    case Picture:
    case None:
    default:
      return FailedToExecute;
  }
}

JSON_STATUS CPlayerOperations::Seek(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  PlayerType player = GetPlayer(parameterObject["playerid"]);
  switch (player)
  {
    case Video:
    case Audio:
      if (parameterObject["value"].isObject())
        g_application.SeekTime(((parameterObject["value"]["hours"].asInteger() * 60) + parameterObject["value"]["minutes"].asInteger()) * 60 + 
          parameterObject["value"]["seconds"].asInteger() + ((double)parameterObject["value"]["milliseconds"].asInteger() / 1000.0));
      else if (IsType(parameterObject["value"], NumberValue))
        g_application.SeekPercentage(parameterObject["value"].asFloat());
      else if (parameterObject["value"].isString())
      {
        CStdString step = parameterObject["value"].asString();
        if (step.Equals("smallforward"))
          CBuiltins::Execute("playercontrol(smallskipforward)");
        else if (step.Equals("smallbackward"))
          CBuiltins::Execute("playercontrol(smallskipbackward)");
        else if (step.Equals("bigforward"))
          CBuiltins::Execute("playercontrol(bigskipforward)");
        else if (step.Equals("bigbackward"))
          CBuiltins::Execute("playercontrol(bigskipbackward)");
        else
          return InvalidParams;
      }
      else
        return InvalidParams;

      GetPropertyValue(player, "percentage", result["percentage"]);
      GetPropertyValue(player, "time", result["time"]);
      GetPropertyValue(player, "totaltime", result["totaltime"]);
      return OK;

    case Picture:
    case None:
    default:
      return FailedToExecute;
  }
}

JSON_STATUS CPlayerOperations::MoveLeft(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  switch (GetPlayer(parameterObject["playerid"]))
  {
    case Picture:
      SendSlideshowAction(ACTION_MOVE_LEFT);
      return ACK;

    case Video:
    case Audio:
    case None:
    default:
      return FailedToExecute;
  }
}

JSON_STATUS CPlayerOperations::MoveRight(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  switch (GetPlayer(parameterObject["playerid"]))
  {
    case Picture:
      SendSlideshowAction(ACTION_MOVE_RIGHT);
      return ACK;

    case Video:
    case Audio:
    case None:
    default:
      return FailedToExecute;
  }
}

JSON_STATUS CPlayerOperations::MoveDown(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  switch (GetPlayer(parameterObject["playerid"]))
  {
    case Picture:
      SendSlideshowAction(ACTION_MOVE_DOWN);
      return ACK;

    case Video:
    case Audio:
    case None:
    default:
      return FailedToExecute;
  }
}

JSON_STATUS CPlayerOperations::MoveUp(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  switch (GetPlayer(parameterObject["playerid"]))
  {
    case Picture:
      SendSlideshowAction(ACTION_MOVE_UP);
      return ACK;

    case Video:
    case Audio:
    case None:
    default:
      return FailedToExecute;
  }
}

JSON_STATUS CPlayerOperations::ZoomOut(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  switch (GetPlayer(parameterObject["playerid"]))
  {
    case Picture:
      SendSlideshowAction(ACTION_ZOOM_OUT);
      return ACK;

    case Video:
    case Audio:
    case None:
    default:
      return FailedToExecute;
  }
}

JSON_STATUS CPlayerOperations::ZoomIn(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  switch (GetPlayer(parameterObject["playerid"]))
  {
    case Picture:
      SendSlideshowAction(ACTION_ZOOM_IN);
      return ACK;

    case Video:
    case Audio:
    case None:
    default:
      return FailedToExecute;
  }
}

JSON_STATUS CPlayerOperations::Zoom(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  switch (GetPlayer(parameterObject["playerid"]))
  {
    case Picture:
      SendSlideshowAction(ACTION_ZOOM_LEVEL_NORMAL + ((int)parameterObject["value"].asInteger() - 1));
      return ACK;

    case Video:
    case Audio:
    case None:
    default:
      return FailedToExecute;
  }
}

JSON_STATUS CPlayerOperations::Rotate(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  switch (GetPlayer(parameterObject["playerid"]))
  {
    case Picture:
      SendSlideshowAction(ACTION_ROTATE_PICTURE);
      return ACK;

    case Video:
    case Audio:
    case None:
    default:
      return FailedToExecute;
  }
}

JSON_STATUS CPlayerOperations::Open(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  if (parameterObject["item"].isObject() && parameterObject["item"].isMember("playlistid"))
  {
    int playlistid = (int)parameterObject["item"]["playlistid"].asInteger();
    switch (playlistid)
    {
      case PLAYLIST_MUSIC:
        if (g_playlistPlayer.GetCurrentPlaylist() != playlistid)
          g_playlistPlayer.SetCurrentPlaylist(playlistid);

        g_application.getApplicationMessenger().PlayListPlayerPlay((int)parameterObject["item"]["position"].asInteger());
        OnPlaylistChanged();
        break;

      case PLAYLIST_VIDEO:
        g_application.getApplicationMessenger().MediaPlay(playlistid, (int)parameterObject["item"]["position"].asInteger());
        OnPlaylistChanged();
        break;

      case PLAYLIST_PICTURE:
        return StartSlideshow();
        break;
    }

    return ACK;
  }
  else if (parameterObject["item"].isObject() && parameterObject["item"].isMember("path"))
  {
    CStdString exec = "slideShow(";

    exec += parameterObject["item"]["path"].asString();

    if (parameterObject["item"]["random"].asBoolean())
      exec += ", random";
    else
      exec += ", notrandom";

    if (parameterObject["item"]["recursive"].asBoolean())
      exec += ", recursive";

    exec += ")";
    ThreadMessage msg = { TMSG_EXECUTE_BUILT_IN, (DWORD)0, (DWORD)0, exec };
    g_application.getApplicationMessenger().SendMessage(msg);

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
        CGUIWindowSlideShow *slideshow = (CGUIWindowSlideShow*)g_windowManager.GetWindow(WINDOW_SLIDESHOW);
        if (!slideshow)
          return FailedToExecute;

        SendSlideshowAction(ACTION_STOP);
        slideshow->Reset();
        for (int index = 0; index < list.Size(); index++)
          slideshow->Add(list[index].get());

        return StartSlideshow();
      }
      else
        g_application.getApplicationMessenger().MediaPlay(list);

      return ACK;
    }
    else
      return InvalidParams;
  }

  return InvalidParams;
}

JSON_STATUS CPlayerOperations::GoPrevious(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  switch (GetPlayer(parameterObject["playerid"]))
  {
    case Video:
    case Audio:
      g_application.getApplicationMessenger().SendAction(CAction(ACTION_PREV_ITEM));
      return ACK;

    case Picture:
      SendSlideshowAction(ACTION_PREV_PICTURE);
      return ACK;

    case None:
    default:
      return FailedToExecute;
  }
}

JSON_STATUS CPlayerOperations::GoNext(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  switch (GetPlayer(parameterObject["playerid"]))
  {
    case Video:
    case Audio:
      g_application.getApplicationMessenger().SendAction(CAction(ACTION_NEXT_ITEM));
      return ACK;

    case Picture:
      SendSlideshowAction(ACTION_NEXT_PICTURE);
      return ACK;

    case None:
    default:
      return FailedToExecute;
  }
}

JSON_STATUS CPlayerOperations::GoTo(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  int position = (int)parameterObject["position"].asInteger();
  switch (GetPlayer(parameterObject["playerid"]))
  {
    case Video:
    case Audio:
      g_application.getApplicationMessenger().PlayListPlayerPlay(position);
      break;

    case Picture:
    case None:
    default:
      return FailedToExecute;
  }

  OnPlaylistChanged();
  return ACK;
}

JSON_STATUS CPlayerOperations::Shuffle(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  CGUIWindowSlideShow *slideshow = NULL;
  switch (GetPlayer(parameterObject["playerid"]))
  {
    case Video:
    case Audio:
      g_application.getApplicationMessenger().PlayListPlayerShuffle(GetPlaylist(GetPlayer(parameterObject["playerid"])), true);
      OnPlaylistChanged();
      break;

    case Picture:
      slideshow = (CGUIWindowSlideShow*)g_windowManager.GetWindow(WINDOW_SLIDESHOW);
      if (slideshow && !slideshow->IsShuffled())
        slideshow->Shuffle();
      else if (!slideshow)
        return FailedToExecute;
      break;

    default:
      return FailedToExecute;
  }
  return ACK;
}

JSON_STATUS CPlayerOperations::UnShuffle(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  switch (GetPlayer(parameterObject["playerid"]))
  {
    case Video:
    case Audio:
      g_application.getApplicationMessenger().PlayListPlayerShuffle(GetPlaylist(GetPlayer(parameterObject["playerid"])), false);
      OnPlaylistChanged();
      break;

    case Picture:
    default:
      return FailedToExecute;
  }
  return ACK;
}

JSON_STATUS CPlayerOperations::Repeat(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  REPEAT_STATE state = REPEAT_NONE;
  std::string strState = parameterObject["state"].asString();
  
  switch (GetPlayer(parameterObject["playerid"]))
  {
    case Video:
    case Audio:
      if (strState.compare("one") == 0)
        state = REPEAT_ONE;
      else if (strState.compare("all") == 0)
        state = REPEAT_ALL;

      g_application.getApplicationMessenger().PlayListPlayerRepeat(GetPlaylist(GetPlayer(parameterObject["playerid"])), state);
      OnPlaylistChanged();
      break;

    case Picture:
    default:
      return FailedToExecute;
  }

  return ACK;
}

JSON_STATUS CPlayerOperations::SetAudioStream(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  switch (GetPlayer(parameterObject["playerid"]))
  {
    case Video:
      if (g_application.m_pPlayer)
      {
        int index = -1;
        if (parameterObject["stream"].isString())
        {
          std::string action = parameterObject["stream"].asString();
          if (action.compare("previous") == 0)
          {
            index = g_application.m_pPlayer->GetAudioStream() - 1;
            if (index < 0)
              index = g_application.m_pPlayer->GetAudioStreamCount() - 1;
          }
          else if (action.compare("next") == 0)
          {
            index = g_application.m_pPlayer->GetAudioStream() + 1;
            if (index >= g_application.m_pPlayer->GetAudioStreamCount())
              index = 0;
          }
          else
            return InvalidParams;
        }
        else if (parameterObject["stream"].isInteger())
          index = (int)parameterObject["stream"].asInteger();

        if (index < 0 || g_application.m_pPlayer->GetAudioStreamCount() <= index)
          return InvalidParams;

        g_application.m_pPlayer->SetAudioStream(index);
      }
      else
        return FailedToExecute;
      break;
      
    case Audio:
    case Picture:
    default:
      return FailedToExecute;
  }

  return ACK;
}

JSON_STATUS CPlayerOperations::SetSubtitle(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  switch (GetPlayer(parameterObject["playerid"]))
  {
    case Video:
      if (g_application.m_pPlayer)
      {
        int index = -1;
        if (parameterObject["subtitle"].isString())
        {
          std::string action = parameterObject["subtitle"].asString();
          if (action.compare("previous") == 0)
          {
            index = g_application.m_pPlayer->GetSubtitle() - 1;
            if (index < 0)
              index = g_application.m_pPlayer->GetSubtitleCount() - 1;
          }
          else if (action.compare("next") == 0)
          {
            index = g_application.m_pPlayer->GetSubtitle() + 1;
            if (index >= g_application.m_pPlayer->GetSubtitleCount())
              index = 0;
          }
          else if (action.compare("off") == 0)
          {
            g_application.m_pPlayer->SetSubtitleVisible(false);
            return ACK;
          }
          else if (action.compare("on") == 0)
          {
            g_application.m_pPlayer->SetSubtitleVisible(true);
            return ACK;
          }
          else
            return InvalidParams;
        }
        else if (parameterObject["subtitle"].isInteger())
          index = (int)parameterObject["subtitle"].asInteger();

        if (index < 0 || g_application.m_pPlayer->GetSubtitleCount() <= index)
          return InvalidParams;

        g_application.m_pPlayer->SetSubtitle(index);
      }
      else
        return FailedToExecute;
      break;
      
    case Audio:
    case Picture:
    default:
      return FailedToExecute;
  }

  return ACK;
}

int CPlayerOperations::GetActivePlayers()
{
  int activePlayers = 0;

  if (g_application.IsPlayingVideo())
    activePlayers |= Video;
  if (g_application.IsPlayingAudio())
    activePlayers |= Audio;
  if (g_windowManager.IsWindowActive(WINDOW_SLIDESHOW))
    activePlayers |= Picture;

  return activePlayers;
}

PlayerType CPlayerOperations::GetPlayer(const CVariant &player)
{
  int activePlayers = GetActivePlayers();
  int playerID;

  switch ((int)player.asInteger())
  {
    case PLAYLIST_VIDEO:
      playerID = Video;
      break;

    case PLAYLIST_MUSIC:
      playerID = Audio;
      break;

    case PLAYLIST_PICTURE:
      playerID = Picture;
      break;

    default:
      playerID = PlayerImplicit;
      break;
  }

  int choosenPlayer = playerID & activePlayers;

  // Implicit order
  if (choosenPlayer & Video)
    return Video;
  else if (choosenPlayer & Audio)
    return Audio;
  else if (choosenPlayer & Picture)
    return Picture;
  else
    return None;
}

int CPlayerOperations::GetPlaylist(PlayerType player)
{
  switch (player)
  {
    case Video:
      return PLAYLIST_VIDEO;

    case Audio:
      return PLAYLIST_MUSIC;

    case Picture:
      return PLAYLIST_PICTURE;

    default:
      return PLAYLIST_NONE;
  }
}

JSON_STATUS CPlayerOperations::StartSlideshow()
{
  CGUIWindowSlideShow *slideshow = (CGUIWindowSlideShow*)g_windowManager.GetWindow(WINDOW_SLIDESHOW);
  if (!slideshow || slideshow->NumSlides() <= 0)
    return FailedToExecute;

  if (g_application.IsPlayingVideo())
    g_application.StopPlaying();

  g_graphicsContext.Lock();

  g_application.WakeUpScreenSaverAndDPMS();
  slideshow->StartSlideShow();

  if (g_windowManager.GetActiveWindow() != WINDOW_SLIDESHOW)
    g_windowManager.ActivateWindow(WINDOW_SLIDESHOW);

  g_graphicsContext.Unlock();

  return ACK;
}

void CPlayerOperations::SendSlideshowAction(int actionID)
{
  g_application.getApplicationMessenger().SendAction(CAction(actionID), WINDOW_SLIDESHOW);
}

void CPlayerOperations::OnPlaylistChanged()
{
  CGUIMessage msg(GUI_MSG_PLAYLIST_CHANGED, 0, 0);
  g_windowManager.SendThreadMessage(msg);
}

JSON_STATUS CPlayerOperations::GetPropertyValue(PlayerType player, const CStdString &property, CVariant &result)
{
  if (player == None)
    return FailedToExecute;

  int playlist = GetPlaylist(player);

  if (property.Equals("type"))
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
  else if (property.Equals("partymode"))
  {
    switch (player)
    {
      case Video:
      case Audio:
        result = g_partyModeManager.IsEnabled();
        break;

      case Picture:
        result = false;
        break;

      default:
        return FailedToExecute;
    }
  }
  else if (property.Equals("speed"))
  {
    CGUIWindowSlideShow *slideshow = NULL;
    switch (player)
    {
      case Video:
      case Audio:
        result = g_application.IsPaused() ? 0 : g_application.GetPlaySpeed();
        break;

      case Picture:
        slideshow = (CGUIWindowSlideShow*)g_windowManager.GetWindow(WINDOW_SLIDESHOW);
        if (slideshow && slideshow->IsPlaying() && !slideshow->IsPaused())
          result = slideshow->GetDirection();
        else
          result = 0;
        break;

      default:
        return FailedToExecute;
    }
  }
  else if (property.Equals("time"))
  {
    switch (player)
    {
      case Video:
      case Audio:
        MillisecondsToTimeObject((int)(g_application.GetTime() * 1000.0), result);
        break;

      case Picture:
        MillisecondsToTimeObject(0, result);
        break;

      default:
        return FailedToExecute;
    }
  }
  else if (property.Equals("percentage"))
  {
    CGUIWindowSlideShow *slideshow = NULL;
    switch (player)
    {
      case Video:
      case Audio:
        result = g_application.GetPercentage();
        break;

      case Picture:
        slideshow = (CGUIWindowSlideShow*)g_windowManager.GetWindow(WINDOW_SLIDESHOW);
        if (slideshow && slideshow->NumSlides() > 0)
          result = (double)slideshow->CurrentSlide() / slideshow->NumSlides();
        else
          result = 0.0;
        break;

      default:
        return FailedToExecute;
    }
  }
  else if (property.Equals("totaltime"))
  {
    switch (player)
    {
      case Video:
      case Audio:
        MillisecondsToTimeObject((int)(g_application.GetTotalTime() * 1000.0), result);
        break;

      case Picture:
        MillisecondsToTimeObject(0, result);
        break;

      default:
        return FailedToExecute;
    }
  }
  else if (property.Equals("playlistid"))
  {
    result = playlist;
  }
  else if (property.Equals("position"))
  {
    CGUIWindowSlideShow *slideshow = NULL;
    switch (player)
    {
      case Video:
      case Audio:
        if (g_playlistPlayer.GetCurrentPlaylist() == playlist)
          result = g_playlistPlayer.GetCurrentSong();
        else
          result = -1;
        break;

      case Picture:
        slideshow = (CGUIWindowSlideShow*)g_windowManager.GetWindow(WINDOW_SLIDESHOW);
        if (slideshow && slideshow->IsPlaying())
          result = slideshow->CurrentSlide() - 1;
        else
          result = -1;
        break;

      default:
        result = -1;
        break;
    }
  }
  else if (property.Equals("repeat"))
  {
    switch (player)
    {
      case Video:
      case Audio:
        switch (g_playlistPlayer.GetRepeat(playlist))
        {
        case REPEAT_ONE:
          result = "one";
          break;
        case REPEAT_ALL:
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
  else if (property.Equals("shuffled"))
  {
    CGUIWindowSlideShow *slideshow = NULL;
    switch (player)
    {
      case Video:
      case Audio:
        result = g_playlistPlayer.IsShuffled(playlist);
        break;

      case Picture:
        slideshow = (CGUIWindowSlideShow*)g_windowManager.GetWindow(WINDOW_SLIDESHOW);
        if (slideshow && slideshow->IsPlaying())
          result = slideshow->IsShuffled();
        else
          result = -1;
        break;

      default:
        result = -1;
        break;
    }
  }
  else if (property.Equals("canseek"))
  {
    switch (player)
    {
      case Video:
      case Audio:
        if (g_application.m_pPlayer)
          result = g_application.m_pPlayer->CanSeek();
        else
          result = false;
        break;

      case Picture:
      default:
        result = false;
        break;
    }
  }
  else if (property.Equals("canchangespeed"))
  {
    switch (player)
    {
      case Video:
      case Audio:
        result = true;
        break;

      case Picture:
      default:
        result = false;
        break;
    }
  }
  else if (property.Equals("canmove"))
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
  else if (property.Equals("canzoom"))
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
  else if (property.Equals("canrotate"))
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
  else if (property.Equals("canshuffle"))
  {
    switch (player)
    {
      case Video:
      case Audio:
      case Picture:
        result = true;
        break;

      default:
        result = false;
        break;
    }
  }
  else if (property.Equals("canrepeat"))
  {
    switch (player)
    {
      case Video:
      case Audio:
        result = true;
        break;

      case Picture:
      default:
        result = false;
        break;
    }
  }
  else if (property.Equals("currentaudiostream"))
  {
    switch (player)
    {
      case Video:
      case Audio:
        if (g_application.m_pPlayer)
        {
          result = CVariant(CVariant::VariantTypeObject);
          int index = g_application.m_pPlayer->GetAudioStream();
          if (index >= 0)
          {
            result["index"] = index;
            CStdString value;
            g_application.m_pPlayer->GetAudioStreamName(index, value);
            result["name"] = value;
            value.Empty();
            g_application.m_pPlayer->GetAudioStreamLanguage(index, value);
            result["language"] = value;
          }
          result["codec"] = g_application.m_pPlayer->GetAudioCodecName();
          result["bitrate"] = g_application.m_pPlayer->GetAudioBitrate();
          result["channels"] = g_application.m_pPlayer->GetChannels();
        }
        else
          result = CVariant(CVariant::VariantTypeNull);
        break;
        
      case Picture:
      default:
        result = CVariant(CVariant::VariantTypeNull);
        break;
    }
  }
  else if (property.Equals("audiostreams"))
  {
    result = CVariant(CVariant::VariantTypeArray);
    switch (player)
    {
      case Video:
        if (g_application.m_pPlayer)
        {
          for (int index = 0; index < g_application.m_pPlayer->GetAudioStreamCount(); index++)
          {
            CVariant audioStream(CVariant::VariantTypeObject);
            audioStream["index"] = index;
            CStdString value;
            g_application.m_pPlayer->GetAudioStreamName(index, value);
            audioStream["name"] = value;
            value.Empty();
            g_application.m_pPlayer->GetAudioStreamLanguage(index, value);
            audioStream["language"] = value;

            result.append(audioStream);
          }
        }
        break;
        
      case Audio:
      case Picture:
      default:
        break;
    }
  }
  else if (property.Equals("subtitleenabled"))
  {
    switch (player)
    {
      case Video:
        if (g_application.m_pPlayer)
          result = g_application.m_pPlayer->GetSubtitleVisible();
        break;
        
      case Audio:
      case Picture:
      default:
        result = false;
        break;
    }
  }
  else if (property.Equals("currentsubtitle"))
  {
    switch (player)
    {
      case Video:
        if (g_application.m_pPlayer)
        {
          result = CVariant(CVariant::VariantTypeObject);
          int index = g_application.m_pPlayer->GetSubtitle();
          if (index >= 0)
          {
            result["index"] = index;
            CStdString value;
            g_application.m_pPlayer->GetSubtitleName(index, value);
            result["name"] = value;
            value.Empty();
            g_application.m_pPlayer->GetSubtitleLanguage(index, value);
            result["language"] = value;
          }
        }
        else
          result = CVariant(CVariant::VariantTypeNull);
        break;
        
      case Audio:
      case Picture:
      default:
        result = CVariant(CVariant::VariantTypeNull);
        break;
    }
  }
  else if (property.Equals("subtitles"))
  {
    result = CVariant(CVariant::VariantTypeArray);
    switch (player)
    {
      case Video:
        if (g_application.m_pPlayer)
        {
          for (int index = 0; index < g_application.m_pPlayer->GetSubtitleCount(); index++)
          {
            CVariant subtitle(CVariant::VariantTypeObject);
            subtitle["index"] = index;
            CStdString value;
            g_application.m_pPlayer->GetSubtitleName(index, value);
            subtitle["name"] = value;
            value.Empty();
            g_application.m_pPlayer->GetSubtitleLanguage(index, value);
            subtitle["language"] = value;

            result.append(subtitle);
          }
        }
        break;
        
      case Audio:
      case Picture:
      default:
        break;
    }
  }
  else
    return InvalidParams;

  return OK;
}
