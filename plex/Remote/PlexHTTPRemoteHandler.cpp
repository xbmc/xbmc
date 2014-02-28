//
//  PlexHTTPRemoteHandler.cpp
//  Plex Home Theater
//
//  Created by Tobias Hieta on 2013-08-16.
//
//

#include "PlexHTTPRemoteHandler.h"
#include "network/WebServer.h"
#include <boost/algorithm/string.hpp>

#include "Client/PlexServerManager.h"
#include "FileSystem/PlexDirectory.h"
#include "ApplicationMessenger.h"
#include "Application.h"
#include "GUIUserMessages.h"
#include "guilib/GUIWindowManager.h"
#include "dialogs/GUIDialogKeyboardGeneric.h"

#include "utils/log.h"
#include "interfaces/Builtins.h"
#include "PlexRemoteSubscriberManager.h"
#include "Client/PlexTimelineManager.h"

#include <boost/asio/detail/socket_ops.hpp>
#include "PlexApplication.h"

#include "settings/GUISettings.h"
#include "guilib/GUIEditControl.h"
#include "GUIAudioManager.h"

#include "PlayList.h"
#include "Settings.h"

#include "GUIWindowSlideShow.h"

#define LEGACY 1

////////////////////////////////////////////////////////////////////////////////////////
bool CPlexHTTPRemoteHandler::CheckHTTPRequest(const HTTPRequest &request)
{
  return boost::starts_with(request.url, "/player") || boost::starts_with(request.url, "/resources");;
}

////////////////////////////////////////////////////////////////////////////////////////
int CPlexHTTPRemoteHandler::HandleHTTPRequest(const HTTPRequest &request)
{
  /* defaults */
  m_responseType = HTTPMemoryDownloadNoFreeCopy;
  m_responseCode = MHD_HTTP_OK;

  ArgMap argumentMap;
  ArgMap headerMap;
  CStdString path(request.url);
  
  CWebServer::GetRequestHeaderValues(request.connection, MHD_GET_ARGUMENT_KIND, argumentMap);
  CWebServer::GetRequestHeaderValues(request.connection, MHD_HEADER_KIND, headerMap);

  /* first see if we need to handle CORS requests - Access-Control-Allow-Origin needs to be
   * available for all requests, the other headers is only needed on a OPTIONS call */
  m_responseHeaderFields.insert(std::pair<std::string, std::string>("Access-Control-Allow-Origin", "*"));
  m_responseHeaderFields.insert(std::pair<std::string, std::string>("X-Plex-Client-Identifier", g_guiSettings.GetString("system.uuid")));
  if (request.method == OPTIONS &&
      headerMap.find("Access-Control-Request-Method") != headerMap.end())
  {
    m_responseHeaderFields.insert(std::pair<std::string, std::string>("Content-Type", "text/plain"));
    m_responseHeaderFields.insert(std::pair<std::string, std::string>("Access-Control-Allow-Methods", "POST, GET, OPTIONS, DELETE, PUT, HEAD"));
    m_responseHeaderFields.insert(std::pair<std::string, std::string>("Access-Control-Max-Age", "1209600"));
    m_responseHeaderFields.insert(std::pair<std::string, std::string>("Connection", "close"));

    if (headerMap.find("Access-Control-Request-Headers") != headerMap.end())
      m_responseHeaderFields.insert(std::pair<std::string, std::string>("Access-Control-Allow-Headers", headerMap["Access-Control-Request-Headers"]));

    return MHD_YES;
  }

  m_responseHeaderFields.insert(std::pair<std::string, std::string>("Content-Type", "text/xml"));
  setStandardResponse();

  updateCommandID(request, argumentMap);

  CLog::Log(LOGDEBUG, "CPlexHTTPRemoteHandler::HandleHTTPRequest handling %s", request.url.c_str());

  if (boost::starts_with(path, "/resources"))
    resources();
  else if (path.Equals("/player/playback/playMedia") ||
      /* LEGACY */ path.Equals("/player/application/playMedia"))
    playMedia(argumentMap);
  else if (path.Equals("/player/playback/stepForward") ||
           path.Equals("/player/playback/stepBack"))
    stepFunction(request.url, argumentMap);
  else if (path.Equals("/player/playback/skipNext"))
    skipNext(argumentMap);
  else if (path.Equals("/player/playback/skipPrevious"))
    skipPrevious(argumentMap);
  else if (path.Equals("/player/playback/stop"))
    stop(argumentMap);
  else if (path.Equals("/player/playback/seekTo"))
    seekTo(argumentMap);
  else if (path.Equals("/player/playback/skipTo"))
    skipTo(argumentMap);
  else if (path.Equals("/player/playback/setParameters"))
    set(argumentMap);
  else if (path.Equals("/player/playback/setStreams"))
    setStreams(argumentMap);
  else if (path.Equals("/player/playback/pause"))
    pausePlay(argumentMap);
  else if (path.Equals("/player/playback/play"))
    pausePlay(argumentMap);
  else if (boost::starts_with(path, "/player/navigation"))
    navigation(path, argumentMap);
  else if (path.Equals("/player/timeline/subscribe"))
    subscribe(request, argumentMap);
  else if (path.Equals("/player/timeline/unsubscribe"))
    unsubscribe(request, argumentMap);
  else if (path.Equals("/player/timeline/poll"))
    poll(request, argumentMap);
  else if (path.Equals("/player/application/setText"))
    sendString(argumentMap);


#ifdef LEGACY
  else if (path.Equals("/player/application/sendString"))
    sendString(argumentMap);
  else if (path.Equals("/player/application/sendVirtualKey") ||
           path.Equals("/player/application/sendKey"))
    sendVKey(argumentMap);
  else if (path.Equals("/player/playback/bigStepForward") ||
           path.Equals("/player/playback/bigStepBack"))
    stepFunction(request.url, argumentMap);
#endif

  else
  {
    setStandardResponse(500, "Nope, not implemented, sorry!");
  }

  m_data = PlexUtils::GetXMLString(m_xmlOutput);

  return MHD_YES;
}

////////////////////////////////////////////////////////////////////////////////////////
void* CPlexHTTPRemoteHandler::GetHTTPResponseData() const
{
  return (void*)m_data.c_str();
}

////////////////////////////////////////////////////////////////////////////////////////
size_t CPlexHTTPRemoteHandler::GetHTTPResonseDataLength() const
{
  return m_data.size();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CPlexHTTPRemoteHandler::updateCommandID(const HTTPRequest &request, const ArgMap &arguments)
{
  if (arguments.find("commandID") == arguments.end())
  {
    CLog::Log(LOGWARNING, "CPlexHTTPRemoteHandler::updateCommandID no commandID sent to this request!");
    return;
  }

  int commandID = -1;
  try { commandID = boost::lexical_cast<int>(arguments.find("commandID")->second); }
  catch (boost::bad_lexical_cast) { return; }

  std::string uuid = CWebServer::GetRequestHeaderValue(request.connection, MHD_HEADER_KIND, "X-Plex-Client-Identifier");
  if (uuid.empty())
  {
    CLog::Log(LOGWARNING, "CPlexHTTPRemoteHandler::updateCommandID subscriber didn't set X-Plex-Client-Identifier");
    setStandardResponse(500, "When commandID is set you also need to specify X-Plex-Client-Identifier");
    return;
  }

  CPlexRemoteSubscriberPtr sub = g_plexApplication.remoteSubscriberManager->findSubscriberByUUID(uuid);
  if (sub)
    sub->setCommandID(commandID);
}

////////////////////////////////////////////////////////////////////////////////////////
void CPlexHTTPRemoteHandler::playMedia(const ArgMap &arguments)
{
  CPlexServerPtr server;
  CStdString key;
  std::string containerPath;
  
  /* Protocol v2 requires machineIndentifier. */
  if (arguments.find("machineIdentifier") != arguments.end())
  {
    std::string uuid = arguments.find("machineIdentifier")->second;
    server = g_plexApplication.serverManager->FindByUUID(uuid);
    if (!server)
    {
      CLog::Log(LOGWARNING, "CPlexHTTPRemoteHandler::playMedia could not find server %s", uuid.c_str());
      setStandardResponse(500, "Could not find specified server");
      return;
    }
  }
  else
  {
    CLog::Log(LOGWARNING, "CPlexHTTPRemoteHandler::playMedia playMedia command REQUIRES a machineIdentifier for the server!");
    setStandardResponse(500, "Required argument machineIdentifier not present");
    return;
  }

  CLog::Log(LOGDEBUG, "CPlexHTTPRemoteHandler::playMedia got a valid server %s", server->toString().c_str());

  if (arguments.find("key") == arguments.end())
  {
    CLog::Log(LOGWARNING, "CPlexHTTPRemoteHandler::playMedia need a key argument!");
    setStandardResponse(500, "Client failed to send key argument");
    return;
  }

  key = arguments.find("key")->second;

  if (containerPath.empty() && arguments.find("containerKey") != arguments.end())
    containerPath = arguments.find("containerKey")->second;
  else if (containerPath.empty())
    containerPath = key;

  // iOS 3.3.1 hacking here, iOS client sends the full absolute path as key
  // We only need the end part of it
  {
    CURL keyURL(key);
    CStdString options = keyURL.GetOptions();
    CURL::Decode(options);

    if (!keyURL.Get().empty() && keyURL.GetProtocol() == "http")
      key = "/" + keyURL.GetFileName() + options;

    if (containerPath == keyURL.Get())
      containerPath = key;
  }

  CURL itemURL = server->BuildPlexURL(containerPath);
  if (arguments.find("protocol") != arguments.end())
  {
    if (arguments.find("protocol")->second == "https")
    {
      if (itemURL.GetProtocol() == "plexserver")
        itemURL.SetProtocolOption("ssl", "1");
    }
  }
  
  /* fetch the container */
  CFileItemList list;
  XFILE::CPlexDirectory dir;
  if (!dir.GetDirectory(itemURL.Get(), list))
  {
    CLog::Log(LOGWARNING, "CPlexHTTPRemoteHandler::playMedia can't fetch container %s", itemURL.Get().c_str());
    setStandardResponse(500, "Could not find that item");
    return;
  }
  
  CFileItemPtr item;
  int idx = 0;
  for (int i = 0; i < list.Size(); i ++)
  {
    CFileItemPtr it = list.Get(i);
    CStdString itemKey = it->GetProperty("unprocessed_key").asString();
    CStdString decodedKey(itemKey);
    CURL::Decode(decodedKey);

    CLog::Log(LOGDEBUG, "CPlexHTTPRemoteHandler::playMedia compare %s|%s = %s", itemKey.c_str(), decodedKey.c_str(), key.c_str());
    if (itemKey == key || decodedKey == key)
    {
      CLog::Log(LOGDEBUG, "CPlexHTTPRemoteHandler::playMedia found media (%s) at index %d", key.c_str(), idx);
      item = it;
      idx = i;
      break;
    }
  }

  if (!item)
  {
    CLog::Log(LOGWARNING, "CPlexHTTPRemoteHandler::playMedia couldn't find %s in %s", key.c_str(), itemURL.Get().c_str());
    setStandardResponse(500, "Could not find that item");
    return;
  }

  item->m_lStartOffset = 0;
  std::string offset;
  if (arguments.find("viewOffset") != arguments.end())
    offset = arguments.find("viewOffset")->second;
  else if (arguments.find("offset") != arguments.end())
    offset = arguments.find("offset")->second;

  if (!offset.empty())
  {
    int offint = 0;

    try { offint = boost::lexical_cast<int>(offset); }
    catch (boost::bad_lexical_cast) { }

    item->SetProperty("viewOffset", offint);
    if (item->GetPlexDirectoryType() == PLEX_DIR_TYPE_TRACK)
      item->m_lStartOffset = (offint / 1000) * 75;
    else
      item->m_lStartOffset = offint != 0 ? STARTOFFSET_RESUME : 0;
  }

  /* make sure that the playlist player doesn't reset our position */
  item->SetProperty("forceStartOffset", true);
  
  if (item->GetPlexDirectoryType() == PLEX_DIR_TYPE_TRACK)
  {
    if (g_application.IsPlaying() || g_application.IsPaused())
      CApplicationMessenger::Get().MediaStop(true);
    
    if (g_playlistPlayer.IsShuffled(PLAYLIST_MUSIC))
      CApplicationMessenger::Get().PlayListPlayerShuffle(PLAYLIST_MUSIC, false);
    
    CApplicationMessenger::Get().PlayListPlayerClear(PLAYLIST_MUSIC);
    CApplicationMessenger::Get().PlayListPlayerAdd(PLAYLIST_MUSIC, list);
    CApplicationMessenger::Get().MediaPlay(PLAYLIST_MUSIC, idx);
  }
  else if (item->GetPlexDirectoryType() == PLEX_DIR_TYPE_PHOTO)
  {
    /* if we are playing music, we don't need to stop */
    if (g_application.IsPlayingVideo())
      CApplicationMessenger::Get().MediaStop(true);
    
    g_application.WakeUpScreenSaverAndDPMS();
    CLog::Log(LOGDEBUG, "PlexHTTPRemoteHandler::playMedia photo slideshow with start %s", list.Get(idx)->GetPath().c_str());
    CApplicationMessenger::Get().PictureSlideShow(itemURL.Get(), false, list.Get(idx)->GetPath());
  }
  else
    CApplicationMessenger::Get().PlayFile(*item);
}

////////////////////////////////////////////////////////////////////////////////////////
void CPlexHTTPRemoteHandler::stepFunction(const CStdString &url, const ArgMap &arguments)
{
  
  if (!g_application.IsPlaying())
    return;
  
  if (url.Equals("/player/playback/bigStepForward"))
    CApplicationMessenger::Get().ExecBuiltIn("playercontrol(bigskipforward)");
  else if (url.Equals("/player/playback/bigStepBack"))
    CApplicationMessenger::Get().ExecBuiltIn("playercontrol(bigskipbackward)");
  else if (url.Equals("/player/playback/stepForward"))
    CApplicationMessenger::Get().ExecBuiltIn("playercontrol(smallskipforward)");
  else if (url.Equals("/player/playback/stepBack"))
    CApplicationMessenger::Get().ExecBuiltIn("playercontrol(smallskipbackward)");
}

////////////////////////////////////////////////////////////////////////////////////////
void CPlexHTTPRemoteHandler::skipNext(const ArgMap &arguments)
{
  CStdString type="video";
  if (arguments.find("type") != arguments.end())
    type = arguments.find("type")->second;

  if (type == "video" || type == "music")
    /* WINDOW_INVALID gets AppMessenger to send to send the action the application instead */
    CApplicationMessenger::Get().SendAction(CAction(ACTION_NEXT_ITEM), WINDOW_INVALID);
  else if (type == "photo")
    CApplicationMessenger::Get().SendAction(CAction(ACTION_NEXT_PICTURE), WINDOW_SLIDESHOW);
}

////////////////////////////////////////////////////////////////////////////////////////
void CPlexHTTPRemoteHandler::skipPrevious(const ArgMap &arguments)
{
  CStdString type="video";
  if (arguments.find("type") != arguments.end())
    type = arguments.find("type")->second;

  if (type == "video" || type == "music")
    /* WINDOW_INVALID gets AppMessenger to send to send the action the application instead */
    CApplicationMessenger::Get().SendAction(CAction(ACTION_PREV_ITEM), WINDOW_INVALID);
  else if (type == "photo")
    CApplicationMessenger::Get().SendAction(CAction(ACTION_PREV_PICTURE), WINDOW_SLIDESHOW);
}

////////////////////////////////////////////////////////////////////////////////////////
void CPlexHTTPRemoteHandler::pausePlay(const ArgMap &arguments)
{
  CStdString type="video";
  if (arguments.find("type") != arguments.end())
    type = arguments.find("type")->second;

  if (type == "video" || type == "music")
    CApplicationMessenger::Get().MediaPause();
  else if (type == "photo")
    CApplicationMessenger::Get().SendAction(CAction(ACTION_PAUSE), WINDOW_SLIDESHOW);
}

////////////////////////////////////////////////////////////////////////////////////////
void CPlexHTTPRemoteHandler::stop(const ArgMap &arguments)
{
  CStdString type="video";
  if (arguments.find("type") != arguments.end())
    type = arguments.find("type")->second;

  if (type == "video" || type == "music")
    CApplicationMessenger::Get().MediaStop();
  else if (type == "photo")
    CApplicationMessenger::Get().SendAction(CAction(ACTION_STOP), WINDOW_SLIDESHOW);
}

////////////////////////////////////////////////////////////////////////////////////////
void CPlexHTTPRemoteHandler::seekTo(const ArgMap &arguments)
{
  int64_t seekTo;
  
  if (arguments.find("offset") != arguments.end())
  {
    try {
      seekTo = boost::lexical_cast<int64_t>(arguments.find("offset")->second);
    } catch (...) {
      CLog::Log(LOGWARNING, "CPlexHTTPRemoteHandler::seekTo failed to convert offset into a int64_t");
      setStandardResponse(500, "offset is not a integer?");
      return;
    }
  }
  else
    return;
  
  if (g_application.IsPlaying() && g_application.m_pPlayer)
    g_application.m_pPlayer->SeekTime(seekTo);
}

////////////////////////////////////////////////////////////////////////////////////////
void CPlexHTTPRemoteHandler::navigation(const CStdString &url, const ArgMap &arguments)
{
  int action = ACTION_NONE;
  int activeWindow = g_windowManager.GetFocusedWindow();
  
  CStdString navigation = url.Mid(19, url.length() - 19);
  
  if (navigation.Equals("moveRight"))
    action = ACTION_MOVE_RIGHT;
  else if (navigation.Equals("moveLeft"))
    action = ACTION_MOVE_LEFT;
  else if (navigation.Equals("moveDown"))
    action = ACTION_MOVE_DOWN;
  else if (navigation.Equals("moveUp"))
    action = ACTION_MOVE_UP;
  else if (navigation.Equals("select"))
    action = ACTION_SELECT_ITEM;
  else if (navigation.Equals("music"))
  {
    if (g_application.IsPlayingAudio() && activeWindow != WINDOW_VISUALISATION)
      action = ACTION_SHOW_GUI;
  }
  else if (navigation.Equals("home"))
  {
    std::vector<CStdString> args;
    g_application.WakeUpScreenSaverAndDPMS();
    CApplicationMessenger::Get().ActivateWindow(WINDOW_HOME, args, false);
    return;
  }
  else if (navigation.Equals("back"))
  {
    if (g_application.IsPlayingFullScreenVideo() &&
        (activeWindow != WINDOW_DIALOG_AUDIO_OSD_SETTINGS &&
         activeWindow != WINDOW_DIALOG_VIDEO_OSD_SETTINGS &&
         activeWindow != WINDOW_DIALOG_PLEX_AUDIO_PICKER &&
         activeWindow != WINDOW_DIALOG_PLEX_SUBTITLE_PICKER))
      action = ACTION_STOP;
    else
      action = ACTION_NAV_BACK;
  }


#ifdef LEGACY
  else if (navigation.Equals("contextMenu"))
    action = ACTION_CONTEXT_MENU;
  else if (navigation.Equals("toggleOSD"))
    action = ACTION_SHOW_OSD;
  else if (navigation.Equals("pageUp"))
    action = ACTION_PAGE_UP;
  else if (navigation.Equals("pageDown"))
    action = ACTION_PAGE_DOWN;
  else if (navigation.Equals("nextLetter"))
    action = ACTION_NEXT_LETTER;
  else if (navigation.Equals("previousLetter"))
    action = ACTION_PREV_LETTER;
#endif
  
  if (action != ACTION_NONE)
  {
    CAction actionId(action);

    g_application.WakeUpScreenSaverAndDPMS();

    g_application.ResetSystemIdleTimer();

    if (!g_application.IsPlaying())
      g_audioManager.PlayActionSound(actionId);

    CApplicationMessenger::Get().SendAction(actionId, WINDOW_INVALID, false);

  }
}

////////////////////////////////////////////////////////////////////////////////////////
void CPlexHTTPRemoteHandler::set(const ArgMap &arguments)
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
    try { mute = boost::lexical_cast<bool>(arguments.find("mute")->second); }
    catch (boost::bad_lexical_cast) { return; }

    if (g_application.IsMuted() && !mute)
      g_application.ToggleMute();
    else if (!g_application.IsMuted() && mute)
      g_application.ToggleMute();
  }
}

////////////////////////////////////////////////////////////////////////////////////////
void CPlexHTTPRemoteHandler::setVolume(const ArgMap &arguments)
{
  int level;
  
  if (arguments.find("level") == arguments.end())
    return;
  
  level = boost::lexical_cast<int>(arguments.find("level")->second);
  
  int oldVolume = g_application.GetVolume();
  g_application.SetVolume((float)level, true);
  CApplicationMessenger::Get().ShowVolumeBar(oldVolume < level);
}

////////////////////////////////////////////////////////////////////////////////////////
void CPlexHTTPRemoteHandler::sendString(const ArgMap &arguments)
{
  std::string newString;

  /* old school */
  if (arguments.find("text") != arguments.end())
  {
    newString = arguments.find("text")->second;
  }
  else if (g_plexApplication.timelineManager->IsTextFieldFocused())
  {
    if (arguments.find(g_plexApplication.timelineManager->GetCurrentFocusedTextField()) != arguments.end())
      newString = arguments.find(g_plexApplication.timelineManager->GetCurrentFocusedTextField())->second;
  }
  
  g_application.WakeUpScreenSaverAndDPMS();

  int currentWindow = g_windowManager.GetActiveWindow();
  CGUIWindow* win = g_windowManager.GetWindow(currentWindow);
  CGUIControl* ctrl = NULL;

  if (currentWindow == WINDOW_PLEX_SEARCH)
    ctrl = (CGUIControl*)win->GetControl(310);
  else
    ctrl = win->GetFocusedControl();

  if (!ctrl)
    return;

  if (ctrl->GetControlType() != CGUIControl::GUICONTROL_EDIT)
  {
    CLog::Log(LOGWARNING, "CPlexHTTPRemoteHandler::sendString focused control %d is not a edit control", ctrl->GetID());
    setStandardResponse(500, "Current focused control doesn't accept text");
    return;
  }

  /* instead of calling keyb->SetText() we want to send this as a message
   * to avoid any thread locking and contention */
  CGUIMessage msg(GUI_MSG_SET_TEXT, 0, ctrl->GetID());
  msg.SetLabel(newString);
  msg.SetParam1(0);
  
  CApplicationMessenger::Get().SendGUIMessage(msg, g_windowManager.GetFocusedWindow());
}

////////////////////////////////////////////////////////////////////////////////////////
void CPlexHTTPRemoteHandler::sendVKey(const ArgMap &arguments)
{
  if (arguments.find("code") == arguments.end())
    return;
  
  int code = boost::lexical_cast<int>(arguments.find("code")->second);
  CLog::Log(LOGDEBUG, "CPlexHTTPRemoteHandler::sendVKey got code %d", code);
  int action;
  
  switch (code)
  {
    case 8:
      action = ACTION_BACKSPACE;
      break;
    case 13:
      action = ACTION_SELECT_ITEM;
      break;
    default:
      action = ACTION_NONE;
  }
  
  if (action != ACTION_NONE)
  {
    g_application.WakeUpScreenSaverAndDPMS();
    CApplicationMessenger::Get().SendAction(CAction(action), g_windowManager.GetFocusedWindow(), false);
  }
}

////////////////////////////////////////////////////////////////////////////////////////
void CPlexHTTPRemoteHandler::subscribe(const HTTPRequest &request, const ArgMap &arguments)
{
  CPlexRemoteSubscriberPtr sub = getSubFromRequest(request, arguments);
  if (sub && g_plexApplication.remoteSubscriberManager && g_plexApplication.timelineManager)
  {
    g_plexApplication.remoteSubscriberManager->addSubscriber(sub);
    g_plexApplication.timelineManager->SendTimelineToSubscriber(sub);
  }
}

////////////////////////////////////////////////////////////////////////////////////////
void CPlexHTTPRemoteHandler::unsubscribe(const HTTPRequest &request, const ArgMap &arguments)
{
  if (g_plexApplication.remoteSubscriberManager)
    g_plexApplication.remoteSubscriberManager->removeSubscriber(getSubFromRequest(request, arguments));
}

////////////////////////////////////////////////////////////////////////////////////////
CPlexRemoteSubscriberPtr CPlexHTTPRemoteHandler::getSubFromRequest(const HTTPRequest &request, const ArgMap &arguments)
{
  std::string uuid, name;
  
  uuid = CWebServer::GetRequestHeaderValue(request.connection, MHD_HEADER_KIND, "X-Plex-Client-Identifier");
  if (uuid.empty())
  {
    CLog::Log(LOGWARNING, "CPlexHTTPRemoteHandler::subscribe subscriber didn't set X-Plex-Client-Identifier");
    setStandardResponse(500, "subscriber didn't set X-Plex-Client-Identifier");
    return CPlexRemoteSubscriberPtr();
  }

  name = CWebServer::GetRequestHeaderValue(request.connection, MHD_HEADER_KIND, "X-Plex-Device-Name");
  if (name.empty())
  {
    CLog::Log(LOGWARNING, "CPlexHTTPRemoteHandler::subscribe subscriber didn't set X-Plex-Device-Name");
    setStandardResponse(500, "subscriber didn't set X-Plex-Device-Name");
    return CPlexRemoteSubscriberPtr();
  }
  
  char ipstr[INET_ADDRSTRLEN];

  struct sockaddr_in *so = (struct sockaddr_in *)MHD_get_connection_info(request.connection, MHD_CONNECTION_INFO_CLIENT_ADDRESS)->client_addr;
  boost::system::error_code ec;
  boost::asio::detail::socket_ops::inet_ntop(AF_INET, &(so->sin_addr), ipstr, INET_ADDRSTRLEN, 0, ec);

  if (!so)
    return CPlexRemoteSubscriberPtr();
  
  int port = 32400;
  int commandID = -1;
  CStdString protocol = "http";

  if (arguments.find("port") != arguments.end())
    port = boost::lexical_cast<int>(arguments.find("port")->second);

  if (arguments.find("commandID") != arguments.end())
    commandID = boost::lexical_cast<int>(arguments.find("commandID")->second);

  if (arguments.find("protocol") != arguments.end())
    protocol = arguments.find("protocol")->second;
  
  CPlexRemoteSubscriberPtr sub = CPlexRemoteSubscriber::NewSubscriber(uuid, ipstr, port, commandID);
  sub->setName(name);
  
  return sub;
}

////////////////////////////////////////////////////////////////////////////////////////
void CPlexHTTPRemoteHandler::setStreams(const ArgMap &arguments)
{
  if (!g_application.IsPlayingVideo())
    return;

  if (arguments.find("type") != arguments.end())
  {
    if (arguments.find("type")->second != "video")
    {
      CLog::Log(LOGWARNING, "CPlexHTTPRemoteHandler::setStreams only works with type=video");
      setStandardResponse(500, "Can only change streams on videos");
      return;
    }
  }


  CFileItemPtr stream;

  if (arguments.find("audioStreamID") != arguments.end())
  {
    int audioStreamID = boost::lexical_cast<int>(arguments.find("audioStreamID")->second);
    stream = PlexUtils::GetStreamByID(g_application.CurrentFileItemPtr(), PLEX_STREAM_AUDIO, audioStreamID);
    if (!stream)
    {
      CLog::Log(LOGWARNING, "CPlexHTTPRemoteHandler::setStream failed to find audioStream %d", audioStreamID);
      setStandardResponse(500, "Failed to find stream");
      return;
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
        CLog::Log(LOGWARNING, "CPlexHTTPRemoteHandler::setStream failed to find subtitleStream %d", subStreamID);
        setStandardResponse(500, "Failed to find stream");
        return;
      }
      g_application.m_pPlayer->SetSubtitleStreamPlexID(subStreamID);
      g_settings.m_currentVideoSettings.m_SubtitleStream = g_application.m_pPlayer->GetSubtitle();
    }

    g_application.m_pPlayer->SetSubtitleVisible(visible);
    g_settings.m_currentVideoSettings.m_SubtitleOn = visible;

  }

  if (stream)
    PlexUtils::SetSelectedStream(g_application.CurrentFileItemPtr(), stream);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CPlexHTTPRemoteHandler::poll(const HTTPRequest &request, const ArgMap &arguments)
{
  bool wait = false;

  std::string uuid = CWebServer::GetRequestHeaderValue(request.connection, MHD_HEADER_KIND, "X-Plex-Client-Identifier");
  std::string name = CWebServer::GetRequestHeaderValue(request.connection, MHD_HEADER_KIND, "X-Plex-Device-Name");
  int commandID = -1;

  if (arguments.find("commandID") != arguments.end())
  {
    try { commandID = boost::lexical_cast<int>(arguments.find("commandID")->second); }
    catch (boost::bad_lexical_cast) {}
  }

  if (commandID == -1 || uuid.empty())
  {
    CLog::Log(LOGWARNING, "CPlexHTTPRemoteHandler::poll the poller needs to set both X-Plex-Client-Identifier header and commandID arguments.");
    setStandardResponse(500, "You need to specify both x-Plex-Client-Identifier as a header and commandID as a argument");
    return;
  }

  CPlexRemoteSubscriberPtr pollSubscriber = CPlexRemoteSubscriber::NewPollSubscriber(uuid, commandID);
  if (!pollSubscriber)
  {
    setStandardResponse(500, "Could not create a poll subscriber. (internal error)");
    return;
  }

  if (!name.empty())
    pollSubscriber->setName(name);

  if (g_plexApplication.remoteSubscriberManager && g_plexApplication.timelineManager)
    pollSubscriber = g_plexApplication.remoteSubscriberManager->addSubscriber(pollSubscriber);

  if (arguments.find("wait") != arguments.end())
  {
    if (arguments.find("wait")->second == "1" || arguments.find("wait")->second == "true")
      wait = true;
  }

  if (wait)
    m_xmlOutput = g_plexApplication.timelineManager->WaitForTimeline(pollSubscriber);
  else
    m_xmlOutput = g_plexApplication.timelineManager->GetCurrentTimeLinesXML(pollSubscriber);

  m_responseHeaderFields.insert(std::make_pair("Access-Control-Expose-Headers", "X-Plex-Client-Identifier"));
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CPlexHTTPRemoteHandler::skipTo(const ArgMap &arguments)
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
    CLog::Log(LOGWARNING, "CPlexHTTPRemoteHandler::skipTo missing 'key' argument.");
    setStandardResponse(500, "Missing key argument");
    return;
  }

  std::string key = arguments.find("key")->second;

  if (playlistType == PLAYLIST_PICTURE)
  {
    CGUIWindowSlideShow* ss = (CGUIWindowSlideShow*)g_windowManager.GetWindow(WINDOW_SLIDESHOW);
    if (!ss)
    {
      setStandardResponse(500, "Missing slideshow, very internal bad error.");
      return;
    }
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
      setStandardResponse(500, "Can't find that key in the current slideshow!");

    return;
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
}


void CPlexHTTPRemoteHandler::resources()
{
  m_xmlOutput.Clear();
  m_xmlOutput.LinkEndChild(new TiXmlDeclaration("1.0", "utf-8", ""));

  TiXmlElement *mediaContainer = new TiXmlElement("MediaContainer");
  m_xmlOutput.LinkEndChild(mediaContainer);

  // title="My Nexus 7" machineIdentifier="x" product="p" platform="p" platformVersion="v" protocolVersion="x" protocolCapabilities="y" deviceClass="z"
  TiXmlElement *player = new TiXmlElement("Player");
  player->SetAttribute("title", g_guiSettings.GetString("services.devicename").c_str());
  player->SetAttribute("protocol", "plex");
  player->SetAttribute("protocolVersion", "1");
  player->SetAttribute("protocolCapabilities", "navigation,playback,timeline");
  player->SetAttribute("machineIdentifier", g_guiSettings.GetString("system.uuid").c_str());
  player->SetAttribute("product", "Plex Home Theater");
  player->SetAttribute("platform", PlexUtils::GetMachinePlatform());
  player->SetAttribute("platformVersion", PlexUtils::GetMachinePlatformVersion());
  player->SetAttribute("deviceClass", "pc");

  mediaContainer->LinkEndChild(player);
}
