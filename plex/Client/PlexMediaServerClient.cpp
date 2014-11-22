//
//  PlexMediaServerClient.cpp
//  Plex Home Theater
//
//  Created by Tobias Hieta on 2013-06-11.
//
//

#include <boost/asio.hpp>

#include "PlexMediaServerClient.h"

#include <boost/lexical_cast.hpp>
#include <boost/foreach.hpp>
#include <string>

#include "PlexFile.h"
#include "video/VideoInfoTag.h"
#include "music/tags/MusicInfoTag.h"
#include "Client/PlexTranscoderClient.h"
#include "PlexJobs.h"
#include "guilib/GUIWindowManager.h"
#include "GUIUserMessages.h"
#include "guilib/Key.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "guilib/LocalizeStrings.h"

#include "Application.h"
#include "settings/AdvancedSettings.h"
#include "utils/log.h"
#include "PlexApplication.h"
#include "PlexServerManager.h"
#include "GUISettings.h"
#include "StringUtils.h"
#include "URIUtils.h"
#include "PlexServer.h"
#include "PlexFile.h"
#include "NetworkInterface.h"
#include "PlexPlayQueueManager.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
CURL CPlexMediaServerClient::GetItemURL(CFileItemPtr item)
{
  CPlexServerPtr server = g_plexApplication.serverManager->FindFromItem(item);
  if (server)
    return server->BuildPlexURL("/");

  CURL u(item->GetPath());
  if (u.GetHostName().empty())
  {
    CLog::Log(LOGDEBUG, "CPlexMediaServerClient::GetItemURL failed to find a viable URL!");
    return CURL();
  }

  return u;
}

////////////////////////////////////////////////////////////////////////////////////////
void CPlexMediaServerClient::OnJobComplete(unsigned int jobID, bool success, CJob *job)
{
  if (stricmp(job->GetType(), "mediaServerClientJob") == 0)
  {
    CPlexMediaServerClientJob *clientJob = static_cast<CPlexMediaServerClientJob*>(job);

    if (success && clientJob->m_msg.GetMessage() != 0)
    {
      /* give us a small breathing room to make sure PMS is up-to-date before reloading */
      Sleep(500);
      g_windowManager.SendThreadMessage(clientJob->m_msg);
    }
    else if (!success && clientJob->m_errorMsg)
      CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Error, g_localizeStrings.Get(257),
                                            g_localizeStrings.Get(clientJob->m_errorMsg));
  }
  else if (stricmp(job->GetType(), "mediaServerClientTimelineJob") == 0)
  {
    CPlexMediaServerClientTimelineJob* tljob = static_cast<CPlexMediaServerClientTimelineJob*>(job);
    if (tljob && success)
    {
      CFileItemPtr item = tljob->m_item;
      if (item->HasProperty("playQueueID"))
      {
        ePlexMediaType type = PlexUtils::GetMediaTypeFromItem(item);
        int time = boost::lexical_cast<int>(tljob->m_url.GetOption("time"));
        if (g_plexApplication.playQueueManager->getID(type) == item->GetProperty("playQueueID").asInteger() && time < 10)
          g_plexApplication.playQueueManager->refresh(type);
      }
    }
  }

  CJobQueue::OnJobComplete(jobID, success, job);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CPlexMediaServerClient::share(const CFileItemPtr &item, const CStdString &network, const CStdString &message)
{
  CStdString fnameStr;
  fnameStr.Format("pms/social/networks/%s/share", network);

  CURL u;
  u.SetProtocol("plexserver");
  u.SetHostName("myplex");
  u.SetFileName(fnameStr);
  u.SetOption("url", item->GetProperty("url").asString());
  u.SetOption("message", message);

  AddJob(new CPlexMediaServerClientJob(u, "POST"));
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CPlexMediaServerClient::publishDevice()
{
  CStdString path;
  path.Format("devices/%s", g_guiSettings.GetString("system.uuid"));

  CPlexServerPtr myPlexServer = g_plexApplication.serverManager->FindByUUID("myplex");
  if (!myPlexServer)
    return;

  CURL u = myPlexServer->BuildURL(path);

  std::vector<NetworkInterface> allInterfaces;
  NetworkInterface::GetCachedList(allInterfaces);
  CStdStringArray interfaceOptions;

  BOOST_FOREACH(NetworkInterface& xface, allInterfaces)
  {
    if (xface.loopback() == false && boost::starts_with(xface.name(), "v") == false)
    {
      // Avoid APIPA addresses.
      if (boost::starts_with(xface.address(), "169.254.") == false)
      {
        CURL deviceAddr;
        deviceAddr.SetProtocol("http");
        deviceAddr.SetHostName(xface.address());
        deviceAddr.SetPort(boost::lexical_cast<int>(g_guiSettings.GetString("services.webserverport")));
        CLog::Log(LOGDEBUG, "CPlexMediaServerClient::publishDevice Adding interface: %s for publishing", deviceAddr.Get().c_str());

        CStdString opt;
        opt.Format("%s=%s", CURL::Encode("Connection[][uri]"), CURL::Encode(deviceAddr.Get()));

        interfaceOptions.push_back(opt);
      }
    }
  }

  CStdString optList;
  StringUtils::JoinString(interfaceOptions, "&", optList);

  CStdString url = u.Get() + "&" + optList;

  CLog::Log(LOGDEBUG, "CPlexMediaServerClient::publishDevice Going to call url: %s", url.c_str());

  AddJob(new CPlexMediaServerClientJob(url, "PUT"));
}

////////////////////////////////////////////////////////////////////////////////////////
void CPlexMediaServerClient::SelectStream(const CFileItemPtr &item,
                                          int partID,
                                          int subtitleStreamID,
                                          int audioStreamID)
{
  CURL u = GetItemURL(item);
  
  u.SetFileName("/library/parts/" + boost::lexical_cast<std::string>(partID));
  if (subtitleStreamID != -1)
    u.SetOption("subtitleStreamID", boost::lexical_cast<std::string>(subtitleStreamID));
  if (audioStreamID != -1)
    u.SetOption("audioStreamID", boost::lexical_cast<std::string>(audioStreamID));
  
  AddJob(new CPlexMediaServerClientJob(u, "PUT"));
}

////////////////////////////////////////////////////////////////////////////////////////
void CPlexMediaServerClient::SetItemWatchStatus(const CFileItemPtr &item, bool watched, bool sendMessage)
{
  CURL u = GetItemURL(item);

  std::string action = watched ? "scrobble" : "unscrobble";

  if (u.GetHostName() == "node" || u.GetHostName() == "myplex")
  {
    u.SetHostName("myplex");
    u.SetFileName("/pms/:/" + action);
    u.SetOption("identifier", PLEX_IDENTIFIER_MYPLEX);
  }
  else
  {
    u.SetFileName("/:/" + action);
    u.SetOption("identifier", PLEX_IDENTIFIER_LIBRARY);
  }

  u.SetOption("key", item->GetProperty("ratingKey").asString());

  if (!sendMessage)
  {
    AddJob(new CPlexMediaServerClientJob(u));
    return;
  }

  CGUIMessage msg(GUI_MSG_UPDATE, PLEX_SERVER_MANAGER, g_windowManager.GetActiveWindow(), 0, 0);
  AddJob(new CPlexMediaServerClientJob(u, "GET", msg));
}

////////////////////////////////////////////////////////////////////////////////////////
void CPlexMediaServerClient::SetItemRating(const CFileItemPtr &item, float rating)
{
  CURL u = GetItemURL(item);
  
  u.SetFileName(GetPrefix(item) + "rate");
  u.SetOption("key", item->GetProperty("ratingKey").asString());
  u.SetOption("identifier", item->GetProperty("identifier").asString());
  u.SetOption("rating", boost::lexical_cast<std::string>(rating));
  
  AddJob(new CPlexMediaServerClientJob(u));
}

////////////////////////////////////////////////////////////////////////////////////////
void CPlexMediaServerClient::SendServerTimeline(const CFileItemPtr &item, const CUrlOptions &options)
{
  CURL u = GetItemURL(item);

  if (u.GetHostName() == "node")
    u.SetHostName("myplex");

  u.SetFileName(":/timeline");
  u.AddOptions(options);

  AddJob(new CPlexMediaServerClientTimelineJob(u, item));
}

////////////////////////////////////////////////////////////////////////////////////////
void CPlexMediaServerClient::SendSubscriberTimeline(const CURL &url, const CStdString &postData)
{
  CPlexMediaServerClientJob *job = new CPlexMediaServerClientJob(url, "POST");
  job->m_postData = postData;

  AddJob(job);
}

////////////////////////////////////////////////////////////////////////////////////////
void CPlexMediaServerClient::SetViewMode(CFileItemPtr item, int viewMode, int sortMode, int sortAsc)
{
  CURL u = GetItemURL(item);
  u.SetFileName("/:/viewChange");
  u.SetOption("identifier", item->GetProperty("identifier").asString());
  u.SetOption("viewGroup", item->GetProperty("viewGroup").asString());

  u.SetOption("viewMode", boost::lexical_cast<CStdString>(viewMode));
  u.SetOption("sortMode", boost::lexical_cast<CStdString>(sortMode));
  u.SetOption("sortAsc", boost::lexical_cast<CStdString>(sortAsc));

  AddJob(new CPlexMediaServerClientJob(u));
}

////////////////////////////////////////////////////////////////////////////////////////
void CPlexMediaServerClient::StopTranscodeSession(CPlexServerPtr server)
{
  CURL u = server->BuildPlexURL("/video/:/transcode/universal/stop");
  u.SetOption("session", CPlexTranscoderClient::GetCurrentSession());
  AddJob(new CPlexMediaServerClientJob(u));
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CPlexMediaServerClient::SendTranscoderPing(CPlexServerPtr server)
{
  CURL u = server->BuildPlexURL("/video/:/transcode/universal/ping");
  u.SetOption("session", CPlexTranscoderClient::GetCurrentSession());
  AddJob(new CPlexMediaServerClientJob(u));
}

////////////////////////////////////////////////////////////////////////////////////////
void CPlexMediaServerClient::deleteItem(const CFileItemPtr &item)
{
  deleteItemFromPath(item->GetPath());
}

////////////////////////////////////////////////////////////////////////////////////////
void CPlexMediaServerClient::deleteItemFromPath(const CStdString path)
{
  CGUIMessage msg(GUI_MSG_UPDATE, 0, 0, 0, 0);
  AddJob(new CPlexMediaServerClientJob(path, "DELETE", msg, 16205));
}

////////////////////////////////////////////////////////////////////////////////////////
void CPlexMediaServerClient::movePlayListItem(CFileItemPtr item, CFileItemPtr after)
{
  CURL url(item->GetPath());
  url.SetFileName(item->GetProperty("containerKey").asString() + "/" +
                  item->GetProperty("playlistItemID").asString() + "/move");

  if (after)
    url.SetOption("after", after->GetProperty("playlistItemID").asString());
  else
    url.SetOption("after", "0");

  CGUIMessage msg(GUI_MSG_UPDATE, 0, 0, 0, 0);
  AddJob(new CPlexMediaServerClientJob(url.Get(), "PUT", msg, 16205));
}

////////////////////////////////////////////////////////////////////////////////////////
bool CPlexMediaServerClient::addItemToPlayList(CPlexServerPtr server, CFileItemPtr item, CStdString playlistID, bool block)
{
  CURL url = server->BuildPlexURL("/playlists/" + playlistID + "/items");
  
  CStdString uri = CPlexPlayQueueManager::getURIFromItem(*item);
  url.SetOption("uri", uri);
  
  CGUIMessage msg(GUI_MSG_UPDATE, 0, 0, 0, 0);
  CPlexMediaServerClientJob *job = new CPlexMediaServerClientJob(url.Get(), "PUT", msg);
  
  if (block)
  {
    return g_plexApplication.busy.blockWaitingForJob(job, NULL);
  }
  else
  {
    AddJob(job);
    return true;
  }
}

////////////////////////////////////////////////////////////////////////////////////////
bool CPlexMediaServerClient::createPlayList(CPlexServerPtr server, CStdString name, CFileItemPtr item, bool smart, bool block)
{
  CURL url = server->BuildPlexURL("/playlists");

  url.SetOption("title", name);

  ePlexMediaType type = PlexUtils::GetMediaTypeFromItem(item);
  if (type == PLEX_MEDIA_TYPE_MUSIC)
    url.SetOption("type", "audio");
  else if (type == PLEX_MEDIA_TYPE_VIDEO)
    url.SetOption("type", "video");
  else
  {
    CLog::Log(LOGERROR, "CPlexMediaServerClient : type %d is not supported for creating playlists", type);
    return false;
  }

  CStdString uri = CPlexPlayQueueManager::getURIFromItem(*item);
  url.SetOption("uri", uri);

  url.SetOption("smart", smart ? "1" : "0");

  CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE, g_windowManager.GetActiveWindow());
  CPlexMediaServerClientJob *job = new CPlexMediaServerClientJob(url.Get(), "POST", msg, 52615);
  if (block)
  {
    return g_plexApplication.busy.blockWaitingForJob(job, this);
  }
  else
  {
    AddJob(job);
    return true;
  }
}

////////////////////////////////////////////////////////////////////////////////////////
CFileItemListPtr CPlexMediaServerClient::getPlayLists()
{
  CPlexDirectoryFetchJob *job = new CPlexDirectoryFetchJob(CURL("plexserver://playlists/"));

  CFileItemListPtr playlists;
  if (g_plexApplication.busy.blockWaitingForJob(job, NULL, &playlists))
  {
    return playlists;
  }

  return CFileItemListPtr();
}
