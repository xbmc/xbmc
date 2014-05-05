#include "PlexRemotePlaybackHandler.h"
#include "ApplicationMessenger.h"
#include "Application.h"
#include "PlexApplication.h"
#include "client/PlexServerManager.h"
#include "settings/GUISettings.h"
#include "settings/Settings.h"
#include "guilib/GUIWindowManager.h"
#include "pictures/GUIWindowSlideShow.h"
#include "Playlists/PlexPlayQueueManager.h"

#include <boost/lexical_cast.hpp>

///////////////////////////////////////////////////////////////////////////////////////////////////
CPlexRemoteResponse CPlexRemotePlaybackHandler::handle(const CStdString &url, const ArgMap &arguments)
{
  if (url.Equals("/player/playback/stepForward") ||
           url.Equals("/player/playback/stepBack"))
    return stepFunction(url, arguments);
  else if (url.Equals("/player/playback/skipNext"))
    return skipNext(arguments);
  else if (url.Equals("/player/playback/skipPrevious"))
    return skipPrevious(arguments);
  else if (url.Equals("/player/playback/stop"))
    return stop(arguments);
  else if (url.Equals("/player/playback/seekTo"))
    return seekTo(arguments);
  else if (url.Equals("/player/playback/skipTo"))
    return skipTo(arguments);
  else if (url.Equals("/player/playback/setParameters"))
    return set(arguments);
  else if (url.Equals("/player/playback/setStreams"))
    return setStreams(arguments);
  else if (url.Equals("/player/playback/pause"))
    return pausePlay(arguments);
  else if (url.Equals("/player/playback/play"))
    return pausePlay(arguments);
  else if (url.Equals("/player/playback/refreshPlayQueue"))
    return refreshPlayQueue(arguments);

  return CPlexRemoteResponse();
}

////////////////////////////////////////////////////////////////////////////////////////
CPlexRemoteResponse CPlexRemotePlaybackHandler::stepFunction(const CStdString &url, const ArgMap &arguments)
{
  if (!g_application.IsPlaying())
    return CPlexRemoteResponse();

  if (url.Equals("/player/playback/bigStepForward"))
    CApplicationMessenger::Get().ExecBuiltIn("playercontrol(bigskipforward)");
  else if (url.Equals("/player/playback/bigStepBack"))
    CApplicationMessenger::Get().ExecBuiltIn("playercontrol(bigskipbackward)");
  else if (url.Equals("/player/playback/stepForward"))
    CApplicationMessenger::Get().ExecBuiltIn("playercontrol(smallskipforward)");
  else if (url.Equals("/player/playback/stepBack"))
    CApplicationMessenger::Get().ExecBuiltIn("playercontrol(smallskipbackward)");

  return CPlexRemoteResponse();
}

////////////////////////////////////////////////////////////////////////////////////////
CPlexRemoteResponse CPlexRemotePlaybackHandler::skipNext(const ArgMap &arguments)
{
  CStdString type="video";
  if (arguments.find("type") != arguments.end())
    type = arguments.find("type")->second;

  if (type == "video" || type == "music")
    /* WINDOW_INVALID gets AppMessenger to send to send the action the application instead */
    CApplicationMessenger::Get().SendAction(CAction(ACTION_NEXT_ITEM), WINDOW_INVALID);
  else if (type == "photo")
    CApplicationMessenger::Get().SendAction(CAction(ACTION_NEXT_PICTURE), WINDOW_SLIDESHOW);

  return CPlexRemoteResponse();
}

////////////////////////////////////////////////////////////////////////////////////////
CPlexRemoteResponse CPlexRemotePlaybackHandler::skipPrevious(const ArgMap &arguments)
{
  CStdString type="video";
  if (arguments.find("type") != arguments.end())
    type = arguments.find("type")->second;

  if (type == "video" || type == "music")
    /* WINDOW_INVALID gets AppMessenger to send to send the action the application instead */
    CApplicationMessenger::Get().SendAction(CAction(ACTION_PREV_ITEM), WINDOW_INVALID);
  else if (type == "photo")
    CApplicationMessenger::Get().SendAction(CAction(ACTION_PREV_PICTURE), WINDOW_SLIDESHOW);
  return CPlexRemoteResponse();
}

////////////////////////////////////////////////////////////////////////////////////////
CPlexRemoteResponse CPlexRemotePlaybackHandler::pausePlay(const ArgMap &arguments)
{
  CStdString type="video";
  if (arguments.find("type") != arguments.end())
    type = arguments.find("type")->second;

  if (type == "video" || type == "music")
    CApplicationMessenger::Get().MediaPause();
  else if (type == "photo")
    CApplicationMessenger::Get().SendAction(CAction(ACTION_PAUSE), WINDOW_SLIDESHOW);
  return CPlexRemoteResponse();
}

////////////////////////////////////////////////////////////////////////////////////////
CPlexRemoteResponse CPlexRemotePlaybackHandler::stop(const ArgMap &arguments)
{
  CStdString type="video";
  if (arguments.find("type") != arguments.end())
    type = arguments.find("type")->second;

  if (type == "video" || type == "music")
    CApplicationMessenger::Get().MediaStop();
  else if (type == "photo")
    CApplicationMessenger::Get().SendAction(CAction(ACTION_STOP), WINDOW_SLIDESHOW);
  return CPlexRemoteResponse();
}

////////////////////////////////////////////////////////////////////////////////////////
CPlexRemoteResponse CPlexRemotePlaybackHandler::seekTo(const ArgMap &arguments)
{
  int64_t seekTo;

  if (arguments.find("offset") != arguments.end())
  {
    try
    {
      seekTo = boost::lexical_cast<int64_t>(arguments.find("offset")->second);
    }
    catch (...)
    {
      CLog::Log(LOGWARNING, "CPlexRemotePlaybackHandler::seekTo failed to convert offset into a int64_t");
      return CPlexRemoteResponse(500, "offset is not a integer?");
    }
  }
  else
    return CPlexRemoteResponse(500, "missing offset argument!");

  if (g_application.IsPlaying() && g_application.m_pPlayer)
    g_application.m_pPlayer->SeekTime(seekTo);

  return CPlexRemoteResponse();
}

///////////////////////////////////////////////////////////////////////////////////////
CPlexRemoteResponse CPlexRemotePlaybackHandler::set(const ArgMap &arguments)
{
  if (arguments.find("volume") != arguments.end())
  {
    ArgMap vmap;
    vmap["level"] = arguments.find("volume")->second;
    setVolume(vmap);
  }

  if (arguments.find("shuffle") != arguments.end())
  {
    int shuffle = boost::lexical_cast<int>(arguments.find("shuffle")->second);
    int playlistType = g_playlistPlayer.GetCurrentPlaylist();
    CApplicationMessenger::Get().PlayListPlayerShuffle(playlistType, shuffle == 1);
  }

  if (arguments.find("repeat") != arguments.end())
  {
    int repeat = boost::lexical_cast<int>(arguments.find("repeat")->second);
    int playlistType = g_playlistPlayer.GetCurrentPlaylist();

    int xbmcRepeat = PLAYLIST::REPEAT_NONE;

    if (repeat==1)
      xbmcRepeat = PLAYLIST::REPEAT_ONE;
    else if (repeat==2)
      xbmcRepeat = PLAYLIST::REPEAT_ALL;

    CApplicationMessenger::Get().PlayListPlayerRepeat(playlistType, xbmcRepeat);
  }

  if (arguments.find("mute") != arguments.end())
  {
    bool mute;
    try
    {
      mute = boost::lexical_cast<bool>(arguments.find("mute")->second);
    }
    catch (boost::bad_lexical_cast)
    {
      return CPlexRemoteResponse(500, "mute is not a integer...");
    }

    if (g_application.IsMuted() && !mute)
      g_application.ToggleMute();
    else if (!g_application.IsMuted() && mute)
      g_application.ToggleMute();
  }

  return CPlexRemoteResponse();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
CPlexRemoteResponse CPlexRemotePlaybackHandler::skipTo(const ArgMap &arguments)
{
  PLAYLIST::CPlayList playlist;
  int playlistType = g_playlistPlayer.GetCurrentPlaylist();

  if (arguments.find("type") != arguments.end())
  {
    std::string type = arguments.find("type")->second;
    if (type == "music")
      playlistType = PLAYLIST_MUSIC;
    else if (type == "video")
      playlistType = PLAYLIST_VIDEO;
    else if (type == "photo")
      playlistType = PLAYLIST_PICTURE;
  }

  if (playlistType != PLAYLIST_PICTURE)
    playlist = g_playlistPlayer.GetPlaylist(playlistType);

  if (arguments.find("key") == arguments.end())
  {
    CLog::Log(LOGWARNING, "CPlexRemotePlaybackHandler::skipTo missing 'key' argument.");
    return CPlexRemoteResponse(500, "Missing key argument");
  }

  std::string key = arguments.find("key")->second;

  if (playlistType == PLAYLIST_PICTURE)
  {
    CGUIWindowSlideShow* ss = (CGUIWindowSlideShow*)g_windowManager.GetWindow(WINDOW_SLIDESHOW);
    if (!ss)
      return CPlexRemoteResponse(500, "Missing slideshow, internal error.");

    CFileItemList list;
    ss->GetSlideShowContents(list);

    bool found = false;

    for (int i = 0; i < list.Size(); i++)
    {
      CFileItemPtr pic = list.Get(i);
      if (pic && (pic->GetProperty("unprocessed_key").asString() == key))
      {
        ss->Select(pic->GetPath());
        found = true;
        break;
      }
    }

    if (!found)
      return CPlexRemoteResponse(500, "Can't find that key in the current slideshow!");

    return CPlexRemoteResponse();
  }

  int idx = -1;
  for (int i = 0; i < playlist.size(); i++)
  {
    CFileItemPtr item = playlist[i];
    if (item->GetProperty("unprocessed_key").asString() == key)
    {
      idx = i;
      break;
    }
  }

  if (idx != -1)
    CApplicationMessenger::Get().MediaPlay(playlistType, idx);

  return CPlexRemoteResponse();
}

////////////////////////////////////////////////////////////////////////////////////////
CPlexRemoteResponse CPlexRemotePlaybackHandler::setVolume(const ArgMap &arguments)
{
  int level;

  if (arguments.find("level") == arguments.end())
    return CPlexRemoteResponse(500, "no level argument supplied...");

  level = boost::lexical_cast<int>(arguments.find("level")->second);

  int oldVolume = g_application.GetVolume();
  g_application.SetVolume((float)level, true);
  CApplicationMessenger::Get().ShowVolumeBar(oldVolume < level);

  return CPlexRemoteResponse();
}

////////////////////////////////////////////////////////////////////////////////////////
CPlexRemoteResponse CPlexRemotePlaybackHandler::setStreams(const ArgMap &arguments)
{
  if (!g_application.IsPlayingVideo())
    return CPlexRemoteResponse();

  if (arguments.find("type") != arguments.end())
  {
    if (arguments.find("type")->second != "video")
    {
      CLog::Log(LOGWARNING, "CPlexRemotePlaybackHandler::setStreams only works with type=video");
      return CPlexRemoteResponse(500, "Can only change streams on videos");
    }
  }

  CFileItemPtr stream;

  if (arguments.find("audioStreamID") != arguments.end())
  {
    int audioStreamID = boost::lexical_cast<int>(arguments.find("audioStreamID")->second);
    stream = PlexUtils::GetStreamByID(g_application.CurrentFileItemPtr(), PLEX_STREAM_AUDIO, audioStreamID);
    if (!stream)
    {
      CLog::Log(LOGWARNING, "CPlexRemotePlaybackHandler::setStream failed to find audioStream %d", audioStreamID);
      return CPlexRemoteResponse(500, "Failed to find stream");
    }
    g_application.m_pPlayer->SetAudioStreamPlexID(audioStreamID);
    g_settings.m_currentVideoSettings.m_AudioStream = g_application.m_pPlayer->GetAudioStream();
  }

  if (arguments.find("subtitleStreamID") != arguments.end())
  {
    int subStreamID = boost::lexical_cast<int>(arguments.find("subtitleStreamID")->second);
    bool visible = subStreamID != 0;

    if (subStreamID == 0)
    {
      stream = CFileItemPtr(new CFileItem);
      stream->SetProperty("streamType", PLEX_STREAM_SUBTITLE);
      stream->SetProperty("id", 0);
    }
    else
    {
      stream = PlexUtils::GetStreamByID(g_application.CurrentFileItemPtr(), PLEX_STREAM_SUBTITLE, subStreamID);
      if (!stream)
      {
        CLog::Log(LOGWARNING, "CPlexRemotePlaybackHandler::setStream failed to find subtitleStream %d", subStreamID);
        return CPlexRemoteResponse(500, "Failed to find stream");
      }
      g_application.m_pPlayer->SetSubtitleStreamPlexID(subStreamID);
      g_settings.m_currentVideoSettings.m_SubtitleStream = g_application.m_pPlayer->GetSubtitle();
    }

    g_application.m_pPlayer->SetSubtitleVisible(visible);
    g_settings.m_currentVideoSettings.m_SubtitleOn = visible;

  }

  if (stream)
    PlexUtils::SetSelectedStream(g_application.CurrentFileItemPtr(), stream);

  return CPlexRemoteResponse();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
CPlexRemoteResponse CPlexRemotePlaybackHandler::refreshPlayQueue(const ArgMap &arguments)
{
  int playQueueId;
  if (arguments.find("playQueueId") == arguments.end())
    return CPlexRemoteResponse(500, "No playQueueId argument!");

  playQueueId = boost::lexical_cast<int>(arguments.find("playQueueId")->second);

  if (g_plexApplication.playQueueManager->current())
  {
    if (g_plexApplication.playQueueManager->current()->getCurrentID() == playQueueId)
      g_plexApplication.playQueueManager->current()->refreshCurrent();
  }
  return CPlexRemoteResponse();
}
