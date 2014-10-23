  #include "PlexPlayQueueServer.h"
#include "PlexUtils.h"
#include "PlexJobs.h"
#include "playlists/PlayList.h"
#include "PlayListPlayer.h"
#include "dialogs/GUIDialogOK.h"
#include "Client/PlexServerManager.h"
#include "PlexApplication.h"
#include "ApplicationMessenger.h"
#include "GUISettings.h"

using namespace PLAYLIST;

///////////////////////////////////////////////////////////////////////////////////////////////////
CURL CPlexPlayQueueServer::getPlayQueueURL(ePlexMediaType type, const std::string& uri, const std::string &playlistID,
                                           const std::string& key, bool shuffle, bool continuous,
                                           int limit, bool next)
{
  CURL u = m_server->BuildPlexURL("/playQueues");
  std::string typeStr = PlexUtils::GetMediaTypeString(type);

  if (typeStr == "music")
  {
    // here we expect audio for some reason
    typeStr = "audio";
  }

  u.SetOption("type", typeStr);
    
  if (!uri.empty())
    u.SetOption("uri", uri);

  if (!playlistID.empty())
    u.SetOption("playlistID", playlistID);
  
  if (!key.empty())
  {
    CStdString keyStr = key;

    CURL keyURL(key);
    if (keyURL.GetProtocol() == "plexserver")
    {
      // This means we have the full path and needs to extract just the
      // filename part of it for the server to understand us.
      keyStr = "/" + keyURL.GetFileName();
    }
    u.SetOption("key", keyStr);
  }
  if (shuffle)
    u.SetOption("shuffle", "1");
  if (continuous)
    u.SetOption("continuous", "1");
  if (limit > 0)
    u.SetOption("limit", boost::lexical_cast<std::string>(limit));
  u.SetOption("next", next ? "1" : "0");

  return u;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CPlexPlayQueueServer::sendRequest(const CURL& url, const CStdString& verb,
                                       const CPlexPlayQueueOptions& options)
{
  CURL u(url);

  // This is the window size, let's add it here so we know that it's on all requests
  u.SetOption("window", "50");
  CPlexPlayQueueFetchJob* job = new CPlexPlayQueueFetchJob(u, options);
  job->m_caller = shared_from_this();
  if (!verb.empty())
    job->m_dir.SetHTTPVerb(verb);

  return g_plexApplication.busy.blockWaitingForJob(job, this);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CPlexPlayQueueServer::create(const CFileItem& container, const CStdString& uri,
                                  const CPlexPlayQueueOptions &options)
{

  ePlexMediaType type = PlexUtils::GetMediaTypeFromItem(container);
  if (type == PLEX_MEDIA_TYPE_UNKNOWN)
    return false;

  setType(type);
  
  CStdString realUri(uri);
  CStdString playlistID;
  if (realUri.empty())
  {
    // calculate URI from the container item
    realUri = CPlexPlayQueueManager::getURIFromItem(container);
  }
  
  // calculate URI from the container item
  if (container.GetPlexDirectoryType() == PLEX_DIR_TYPE_PLAYLIST)
  {
    realUri = "";
    playlistID = container.GetProperty("ratingkey").asString();
  }

  CURL u = getPlayQueueURL(type, realUri, playlistID, options.startItemKey, options.shuffle, false, 0, false);

  if (u.Get().empty())
    return false;

  // add any creation URL option
  u.AddOptions(options.urlOptions);

  // add trailers option count to creation url if required
  if (container.GetPlexDirectoryType() == PLEX_DIR_TYPE_MOVIE)
  {
    int trailerCount = g_guiSettings.GetInt("videoplayer.playtrailercount");
    if ((trailerCount) && ((container.GetProperty("viewOffset").asInteger() == 0) || (options.forceTrailers)))
      u.SetOption("extrasPrefixCount", boost::lexical_cast<std::string>(trailerCount));
  }

  return sendRequest(u, "POST", options);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CPlexPlayQueueServer::refresh()
{
  int id = getID();

  CLog::Log(LOGDEBUG, "CPlexPlayQueueServer::refresh refreshing playQueue %d", id);

  CStdString path;
  path.Format("/playQueues/%d", id);

  sendRequest(m_server->BuildPlexURL(path), "", CPlexPlayQueueOptions(false, false));
  return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CPlexPlayQueueServer::get(CFileItemList& list)
{
  CSingleLock lk(m_mapLock);
  if (m_list)
  {
    list.Copy(*m_list);
    return true;
  }

  return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CPlexPlayQueueServer::removeItem(const CFileItemPtr& item)
{
  if (!item || !item->HasProperty("playQueueItemID"))
    return;

  CStdString path;
  path.Format("/playQueues/%d/items/%d", (int)item->GetProperty("playQueueID").asInteger(),
              (int)item->GetProperty("playQueueItemID").asInteger());
  CURL u = m_server->BuildPlexURL(path);

  sendRequest(u, "DELETE", CPlexPlayQueueOptions(false, false));
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CPlexPlayQueueServer::addItem(const CFileItemPtr& item, bool next)
{
  CStdString uri = CPlexPlayQueueManager::getURIFromItem(*item);
  CPlexServerPtr server = g_plexApplication.serverManager->FindFromItem(item);

  if (server)
  {
    CURL u = getPlayQueueURL(PlexUtils::GetMediaTypeFromItem(item), uri, "", "", false, false, 0, next);
    CStdString path;
    path.Format("/playQueues/%d", getID());

    u.SetFileName(path);

    if (u.Get().empty())
      return false;

    return sendRequest(u, "PUT", CPlexPlayQueueOptions(false, false));
  }
  return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
int CPlexPlayQueueServer::getID()
{
  CSingleLock lk(m_mapLock);
  if (m_list)
    return m_list->GetProperty("playQueueID").asInteger();
  return -1;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
int CPlexPlayQueueServer::getPlaylistID()
{
  CSingleLock lk(m_mapLock);
  if (m_list)
    return m_list->GetProperty("playQueuePlaylistID").asInteger();
  return -1;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
CStdString CPlexPlayQueueServer::getPlaylistTitle()
{
  if (m_list)
    return m_list->GetProperty("playQueuePlaylistTitle").asString();
  return "";
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CPlexPlayQueueServer::get(const CStdString &playQueueID, const CPlexPlayQueueOptions &options)
{
  CStdString path;
  path.Format("/playQueues/%s", playQueueID);
  sendRequest(m_server->BuildPlexURL(path), "", options);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
void CPlexPlayQueueServer::OnJobComplete(unsigned int jobID, bool success, CJob* job)
{
  CPlexPlayQueueFetchJob* fj = static_cast<CPlexPlayQueueFetchJob*>(job);
  if (fj && success)
  {
    ePlexMediaType type = PlexUtils::GetMediaTypeFromItem(fj->m_items);
    m_Type = type;
    int playlist = CPlexPlayQueueManager::getPlaylistFromType(type);

    // if we are removing items an that there is no more, clear the list and send update message
    if ((fj->m_dir.getHTTPVerb() == "DELETE") && (!fj->m_items.Size()))
    {
      m_list->Clear();
      CApplicationMessenger::Get().PlexUpdatePlayQueue(getType(), fj->m_options.startPlaying);
      return;
    }
    
    if (playlist == PLAYLIST_NONE)
    {
      CLog::Log(LOGERROR, "CPlexPlayQueueServer::OnJobComplete : The response from the server did not make sense (PlayList Type is Unknown)");
      return;
    }

    if (fj->m_items.Size() == 0)
    {
      CGUIDialogOK::ShowAndGetInput("Error creating the Play Queue",
                                    "The server responded with a empty Play Queue", "", "");
      return;
    }

    CLog::Log(LOGDEBUG, "CPlexPlayQueueServer::OnJobComplete got playQueue (%lld) of size %d and type %d",
              fj->m_items.GetProperty("playQueueID").asInteger(), fj->m_items.Size(), type);

    CFileItemListPtr pqCopy = CFileItemListPtr(new CFileItemList);
    pqCopy->Assign(fj->m_items);
    pqCopy->SetPath("plexserver://playqueue/");

    {
      CSingleLock lk(m_mapLock);
      m_list = pqCopy;
      if (m_list->Get(0) && m_list->Get(m_list->Size()-1))
        CLog::Log(LOGDEBUG, "CPlexPlayQueueServer::OnJobComplete new list %s => %s",
                  m_list->Get(0)->GetLabel().c_str(), m_list->Get(m_list->Size() - 1)->GetLabel().c_str());
    }

    if (!fj->m_options.showPrompts && m_list->Get(0))
    {
      m_list->Get(0)->SetProperty("avoidPrompts", true);
      PlexUtils::SetItemResumeOffset(m_list->Get(0), fj->m_options.resumeOffset);
    }
    
    // we remove also prompts for trailers
    for (int i=0; i<m_list->Size(); i++)
    {
      if (m_list->Get(i)->HasProperty("extraType"))
        m_list->Get(i)->SetProperty("avoidPrompts", true);

      m_list->Get(i)->SetProperty("playQueueVersion", m_list->GetProperty("playQueueVersion").asString());
    }

    CApplicationMessenger::Get().PlexUpdatePlayQueue(type, fj->m_options.startPlaying);
  }
  else
  {
    CGUIDialogOK::ShowAndGetInput("Error creating the Play Queue",
                                  "The server was unable to create the Play Queue", "", "");
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CPlexPlayQueueServer::moveItem(const CFileItemPtr& item, const CFileItemPtr& afteritem)
{

  if (!item || !item->HasProperty("playQueueItemID") ||
      (item->GetProperty("playQueueID").asInteger() != getID()))
    return false;

  // define insert pos
  CStdString insertID = "";
  if (afteritem)
  {
    if (!afteritem->HasProperty("playQueueItemID") ||
        (afteritem->GetProperty("playQueueID").asInteger() != getID()))
      return false;
    else
      insertID = afteritem->GetProperty("playQueueItemID").asString();
  }

  // move the item in PMS
  CStdString path;
  path.Format("/playQueues/%d/items/%d/move", (int)item->GetProperty("playQueueID").asInteger(),
              (int)item->GetProperty("playQueueItemID").asInteger());
  CURL u = m_server->BuildPlexURL(path);

  if (!insertID.IsEmpty())
    u.SetOption("after", insertID);

  sendRequest(u, "PUT", CPlexPlayQueueOptions(false, false));

  return true;
}

