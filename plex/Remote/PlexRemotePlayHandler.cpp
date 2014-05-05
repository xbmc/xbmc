#include "PlexRemotePlayHandler.h"
#include "PlexApplication.h"
#include "Client/PlexServerManager.h"
#include "Application.h"
#include "ApplicationMessenger.h"
#include "FileSystem/PlexDirectory.h"
#include "PlayLists/PlexPlayQueueManager.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CPlexRemotePlayHandler::getKeyAndContainerUrl(const ArgMap& arguments, std::string& key, std::string& containerKey)
{
  std::string containerPath;
  std::string keyPath = arguments.find("key")->second;

  if (arguments.find("key") == arguments.end())
  {
    CLog::Log(LOGWARNING, "CPlexHTTPRemoteHandler::playMedia need a key argument!");
    return false;
  }

  if (arguments.find("containerKey") != arguments.end())
    containerPath = arguments.find("containerKey")->second;

  // iOS 3.3.1 hacking here, iOS client sends the full absolute path as key
  // We only need the end part of it
  if (boost::starts_with(keyPath, "http://"))
  {
    CURL keyURL(keyPath);
    CStdString options = keyURL.GetOptions();
    CURL::Decode(options);
    keyPath = "/" + keyURL.GetFileName() + options;

    if (containerPath == keyURL.Get())
      containerPath = keyPath;
  }

  key = keyPath;
  containerKey = containerPath;

  return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CPlexRemotePlayHandler::getContainer(const CURL& dirURL, CFileItemList& list)
{
  XFILE::CPlexDirectory dir;
  if (!dir.GetDirectory(dirURL.Get(), list))
  {
    CLog::Log(LOGWARNING, "CPlexHTTPRemoteHandler::playMedia can't fetch container %s", dirURL.Get().c_str());
    return false;
  }
  return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
CFileItemPtr CPlexRemotePlayHandler::getItemFromContainer(const std::string& key, const CFileItemList& list, int& idx)
{
  CFileItemPtr item;
  idx = 0;

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

  return item;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CPlexRemotePlayHandler::setStartPosition(const CFileItemPtr& item, const ArgMap& arguments)
{
  item->m_lStartOffset = 0;
  std::string offset;
  if (arguments.find("viewOffset") != arguments.end())
    offset = arguments.find("viewOffset")->second;
  else if (arguments.find("offset") != arguments.end())
    offset = arguments.find("offset")->second;

  if (!offset.empty())
  {
    int offint = -1;

    try { offint = boost::lexical_cast<int>(offset); }
    catch (boost::bad_lexical_cast) { }

    if (offint != -1)
    {
      item->SetProperty("viewOffset", offint);
      if (item->GetPlexDirectoryType() == PLEX_DIR_TYPE_TRACK)
        item->m_lStartOffset = (offint / 1000) * 75;
      else
        item->m_lStartOffset = offint != 0 ? STARTOFFSET_RESUME : 0;
    }
    else if (item->HasProperty("viewOffset"))
    {
      item->m_lStartOffset = STARTOFFSET_RESUME;
    }
  }

  /* make sure that the playlist player doesn't reset our position */
  item->SetProperty("forceStartOffset", true);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
CPlexRemoteResponse CPlexRemotePlayHandler::playPlayQueue(const CPlexServerPtr& server,
                                                          const CStdString& playQueueUrl)
{

  // chop of /playQueues/ and all arguments
  std::string playQueueId = playQueueUrl.substr(12);
  int i = playQueueId.find("?");
  if (i != std::string::npos)
    playQueueId = playQueueId.substr(0, i);

  CLog::Log(LOGDEBUG, "CPlexRemotePlayHandler::playPlayQueue asked to play a playQueue: %s",
            playQueueId.c_str());

  g_plexApplication.playQueueManager->loadPlayQueue(server, playQueueId);
  return CPlexRemoteResponse();
}

////////////////////////////////////////////////////////////////////////////////////////
CPlexRemoteResponse CPlexRemotePlayHandler::handle(const CStdString& url, const ArgMap &arguments)
{
  CPlexServerPtr server;

  server = CPlexHTTPRemoteHandler::getServerFromArguments(arguments);
  if (!server)
  {
    CLog::Log(LOGERROR, "CPlexHTTPRemoteHandler::playMedia didn't get a valid server!");
    return CPlexRemoteResponse(500, "Did not find a server!");
  }

  CLog::Log(LOGDEBUG, "CPlexHTTPRemoteHandler::playMedia got a valid server %s", server->toString().c_str());

  std::string key, containerPath;

  if (!getKeyAndContainerUrl(arguments, key, containerPath))
    return CPlexRemoteResponse(500, "Could not parse key argument");

  CURL dirURL;
  if (!containerPath.empty())
  {
    if (boost::starts_with(containerPath, "/playQueues"))
      return playPlayQueue(server, containerPath);
    else
      dirURL = server->BuildPlexURL(containerPath);
  }
  else
  {
    dirURL = server->BuildPlexURL(key);
  }


  CFileItemList list;
  if (!getContainer(dirURL, list))
    return CPlexRemoteResponse(500, "could not load container");

  int idx;
  CFileItemPtr item = getItemFromContainer(key, list, idx);
  if (!item)
  {
    CLog::Log(LOGWARNING, "CPlexHTTPRemoteHandler::playMedia couldn't find %s in %s", key.c_str(), dirURL.Get().c_str());
    return CPlexRemoteResponse(500, "Could not find that item");
  }

  setStartPosition(item, arguments);

  g_application.WakeUpScreenSaverAndDPMS();
  g_application.ResetSystemIdleTimer();

  if (item->GetPlexDirectoryType() == PLEX_DIR_TYPE_TRACK && !containerPath.empty())
  {
    g_plexApplication.playQueueManager->create(list, "",
                                               item->GetProperty("unprocessed_key").asString(),
                                               false);
  }
  else if (item->GetPlexDirectoryType() == PLEX_DIR_TYPE_PHOTO)
  {
    /* if we are playing music, we don't need to stop */
    if (g_application.IsPlayingVideo())
      CApplicationMessenger::Get().MediaStop(true);

    CLog::Log(LOGDEBUG, "PlexHTTPRemoteHandler::playMedia photo slideshow with start %s", list.Get(idx)->GetPath().c_str());
    CApplicationMessenger::Get().PictureSlideShow(dirURL.Get(), false, list.Get(idx)->GetPath());
  }
  else
    CApplicationMessenger::Get().PlayFile(*item);

  return CPlexRemoteResponse();
}
