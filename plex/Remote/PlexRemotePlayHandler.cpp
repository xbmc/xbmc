#include "PlexRemotePlayHandler.h"
#include "PlexApplication.h"
#include "Client/PlexServerManager.h"
#include "Application.h"
#include "ApplicationMessenger.h"
#include "FileSystem/PlexDirectory.h"

////////////////////////////////////////////////////////////////////////////////////////
CPlexRemoteResponse CPlexRemotePlayHandler::handle(const CStdString& url, const ArgMap &arguments)
{
  CPlexServerPtr server;
  CStdString key;
  std::string containerPath;

  server = CPlexHTTPRemoteHandler::getServerFromArguments(arguments);
  if (!server)
  {
    CLog::Log(LOGERROR, "CPlexHTTPRemoteHandler::playMedia didn't get a valid server!");
    return CPlexRemoteResponse(500, "Did not find a server!");
  }

  CLog::Log(LOGDEBUG, "CPlexHTTPRemoteHandler::playMedia got a valid server %s", server->toString().c_str());

  if (arguments.find("key") == arguments.end())
  {
    CLog::Log(LOGWARNING, "CPlexHTTPRemoteHandler::playMedia need a key argument!");
    return CPlexRemoteResponse(500, "Need key argument");
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

  /* fetch the container */
  CFileItemList list;
  XFILE::CPlexDirectory dir;
  if (!dir.GetDirectory(itemURL.Get(), list))
  {
    CLog::Log(LOGWARNING, "CPlexHTTPRemoteHandler::playMedia can't fetch container %s", itemURL.Get().c_str());
    return CPlexRemoteResponse(500, "Could not find that item");
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
    return CPlexRemoteResponse(500, "Could not find that item");
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
  g_application.WakeUpScreenSaverAndDPMS();
  g_application.ResetSystemIdleTimer();

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

    CLog::Log(LOGDEBUG, "PlexHTTPRemoteHandler::playMedia photo slideshow with start %s", list.Get(idx)->GetPath().c_str());
    CApplicationMessenger::Get().PictureSlideShow(itemURL.Get(), false, list.Get(idx)->GetPath());
  }
  else
    CApplicationMessenger::Get().PlayFile(*item);

  return CPlexRemoteResponse();
}
