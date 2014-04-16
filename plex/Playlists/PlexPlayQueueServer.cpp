#include "PlexPlayQueueServer.h"
#include "PlexUtils.h"
#include "PlexJobs.h"
#include "playlists/PlayList.h"
#include "PlayListPlayer.h"
#include "dialogs/GUIDialogOK.h"
#include "Client/PlexServerManager.h"
#include "PlexApplication.h"
#include "ApplicationMessenger.h"

using namespace PLAYLIST;

///////////////////////////////////////////////////////////////////////////////////////////////////
CURL CPlexPlayQueueServer::getPlayQueueURL(ePlexMediaType type, const std::string& uri,
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
  u.SetOption("uri", uri);

  if (!key.empty())
    u.SetOption("key", key);
  if (shuffle)
    u.SetOption("shuffle", "1");
  if (continuous)
    u.SetOption("continuous", "1");
  if (limit > 0)
    u.SetOption("limit", boost::lexical_cast<std::string>(limit));
  if (next)
    u.SetOption("next", "1");

  return u;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CPlexPlayQueueServer::create(const CFileItem& container, const CStdString& uri,
                                  const CStdString& startItemKey, bool shuffle)
{

  ePlexMediaType type = PlexUtils::GetMediaTypeFromItem(container);
  if (type == PLEX_MEDIA_TYPE_UNKNOWN)
    return;

  CStdString realUri(uri);
  if (realUri.empty())
  {
    // calculate URI from the container item
    realUri = CPlexPlayQueueManager::getURIFromItem(container);
  }

  CURL u = getPlayQueueURL(type, realUri, startItemKey, shuffle, false, 0, false);

  if (u.Get().empty())
    return;

  CPlexPlayQueueFetchJob* job = new CPlexPlayQueueFetchJob(u, true);
  job->m_dir.SetHTTPVerb("POST");
  job->m_caller = shared_from_this();
  CJobManager::GetInstance().AddJob(job, this);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CPlexPlayQueueServer::refreshCurrent()
{
  int id = getCurrentID();

  CStdString path;
  path.Format("/playQueues/%d", id);

  CURL u = m_server->BuildPlexURL(path);
  CPlexPlayQueueFetchJob* job = new CPlexPlayQueueFetchJob(u);
  job->m_caller = shared_from_this();
  CJobManager::GetInstance().AddJob(job, this);
  return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CPlexPlayQueueServer::getCurrent(CFileItemList& list)
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
  path.Format("/playQueues/%d/items/%d", item->GetProperty("playQueueID").asInteger(),
              item->GetProperty("playQueueItemID").asInteger());
  CURL u = m_server->BuildPlexURL(path);

  CPlexPlayQueueFetchJob* job = new CPlexPlayQueueFetchJob(u);
  job->m_dir.SetHTTPVerb("DELETE");
  job->m_caller = shared_from_this();
  CJobManager::GetInstance().AddJob(job, this);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CPlexPlayQueueServer::addItem(const CFileItemPtr& item)
{
  CStdString uri = CPlexPlayQueueManager::getURIFromItem(*item);
  CPlexServerPtr server = g_plexApplication.serverManager->FindFromItem(item);

  if (server)
  {
    CURL u = getPlayQueueURL(PlexUtils::GetMediaTypeFromItem(item), uri, "", false, false);

    if (u.Get().empty())
      return;

    CPlexPlayQueueFetchJob* job = new CPlexPlayQueueFetchJob(u, true);
    job->m_dir.SetHTTPVerb("PUT");
    job->m_caller = shared_from_this();
    CJobManager::GetInstance().AddJob(job, this);
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
int CPlexPlayQueueServer::getCurrentID()
{
  CSingleLock lk(m_mapLock);
  if (m_list)
    return m_list->GetProperty("playQueueID").asInteger();
  return -1;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CPlexPlayQueueServer::get(const CStdString &playQueueID)
{
  CStdString path;
  path.Format("/playQueues/%s", playQueueID);
  CPlexPlayQueueFetchJob* job = new CPlexPlayQueueFetchJob(m_server->BuildPlexURL(path), false);
  job->m_caller = shared_from_this();
  CJobManager::GetInstance().AddJob(job, this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
void CPlexPlayQueueServer::OnJobComplete(unsigned int jobID, bool success, CJob* job)
{
  CPlexPlayQueueFetchJob* fj = static_cast<CPlexPlayQueueFetchJob*>(job);
  if (fj && success)
  {
    ePlexMediaType type = PlexUtils::GetMediaTypeFromItem(fj->m_items);
    int playlist = CPlexPlayQueueManager::getPlaylistFromType(type);

    if (playlist == PLAYLIST_NONE)
    {
      CGUIDialogOK::ShowAndGetInput("Error creating the Play Queue",
                                    "The reponse from the server did not make sense.", "", "");
      return;
    }

    if (fj->m_items.Size() == 0)
    {
      CGUIDialogOK::ShowAndGetInput("Error creating the Play Queue",
                                    "The server responded with a empty Play Queue", "", "");
      return;
    }

    CLog::Log(LOGDEBUG, "CPlexPlayQueueServer::OnJobComplete got playQueue of size %d and type %d",
              fj->m_items.Size(), playlist);

    CFileItemListPtr pqCopy = CFileItemListPtr(new CFileItemList);
    pqCopy->Assign(fj->m_items);

    {
      CSingleLock lk(m_mapLock);
      m_list = pqCopy;
    }
    CApplicationMessenger::Get().PlexUpdatePlayQueue(type, fj->m_startPlaying);
  }
  else
  {
    CGUIDialogOK::ShowAndGetInput("Error creating the Play Queue",
                                  "The server was unable to create the Play Queue", "", "");
  }
}
