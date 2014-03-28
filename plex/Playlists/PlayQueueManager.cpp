#include "PlayQueueManager.h"

#include "Client/PlexServer.h"
#include "FileSystem/PlexFile.h"
#include "PlexJobs.h"
#include "PlexTypes.h"
#include "URL.h"

#include "JobManager.h"
#include "PlayList.h"
#include "PlayListPlayer.h"

#include "ApplicationMessenger.h"

#include "dialogs/GUIDialogOK.h"

using namespace PLAYLIST;

///////////////////////////////////////////////////////////////////////////////////////////////////
CPlayQueueManager::CPlayQueueManager()
{
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CPlayQueueManager::getPlayQueue(CPlexServerPtr server, int id)
{
}

///////////////////////////////////////////////////////////////////////////////////////////////////
CStdString CPlayQueueManager::getURIFromItem(const CFileItemPtr& item)
{
  if (!item)
    return "";

  CURL u(item->GetPath());

  if (u.GetProtocol() != "plexserver")
    return "";

  CStdString itemDirStr = "item";
  if (item->m_bIsFolder)
    itemDirStr = "directory";

  if (!item->HasProperty("librarySectionUUID"))
  {
    CLog::Log(LOGWARNING, "CPlayQueueManager::getURIFromItem item %s doesn't have a section UUID", item->GetPath().c_str());
    return "";
  }

  CStdString ret;
  ret.Format("library://%s/%s/%s", item->GetProperty("librarySectionUUID").asString(), itemDirStr,
             CURL::Encode(item->GetProperty("unprocessed_key").asString()));

  return ret;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CPlayQueueManager::createPlayQueueFromItem(const CPlexServerPtr& server,
                                                const CFileItemPtr& item, bool shuffle,
                                                bool continuous, int limit)
{
  if (!server)
    return false;

  ePlexMediaType type = PlexUtils::GetMediaTypeFromItem(item);
  if (type == PLEX_MEDIA_TYPE_UNKNOWN)
    return false;

  std::string containerPath = getURIFromItem(item);
  std::string key;

  if (!item->m_bIsFolder)
    key = item->GetPath();

  return createPlayQueue(server, type, containerPath, key, shuffle, continuous, limit);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
CURL CPlayQueueManager::getCreatePlayQueueURL(const CPlexServerPtr& server, ePlexMediaType type,
                                              const std::string& uri, const std::string& key,
                                              bool shuffle, bool continuous, int limit)
{
  if (!server)
    return CURL();

  CURL u = server->BuildPlexURL("/playQueues");
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

  return u;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CPlayQueueManager::createPlayQueue(const CPlexServerPtr& server, ePlexMediaType type,
                                        const std::string& uri, const std::string& key,
                                        bool shuffle, bool continuous, int limit)
{
  CURL u = getCreatePlayQueueURL(server, type, uri, key, shuffle, continuous, limit);

  if (u.Get().empty())
    return false;

  CPlexDirectoryFetchJob* job = new CPlexDirectoryFetchJob(u);
  job->m_dir.DoPost(true);
  CJobManager::GetInstance().AddJob(job, this);
  return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
int CPlayQueueManager::getPlaylistFromString(const CStdString& typeStr)
{
  if (typeStr.Equals("audio"))
    return PLAYLIST_MUSIC;
  else if (typeStr.Equals("video"))
    return PLAYLIST_VIDEO;
  else if (typeStr.Equals("photo"))
    return PLAYLIST_PICTURE;
  return PLAYLIST_NONE;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CPlayQueueManager::refreshPlayQueue(const CFileItemPtr &item)
{
  if (!item)
    return;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
void CPlayQueueManager::OnJobComplete(unsigned int jobID, bool success, CJob* job)
{
  CPlexDirectoryFetchJob *fj = static_cast<CPlexDirectoryFetchJob*>(job);
  if (fj && success)
  {

    int playlist = getPlaylistFromString(fj->m_url.GetOption("type"));
    int selectedOffset = fj->m_items.GetProperty("playQueueSelectedItemOffset").asInteger();

    if (playlist == PLAYLIST_NONE)
    {
      CGUIDialogOK::ShowAndGetInput("Error creating the Play Queue", "The reponse from the server did not make sense.", "", "");
      return;
    }

    if (fj->m_items.Size() == 0)
    {
      CGUIDialogOK::ShowAndGetInput("Error creating the Play Queue", "The server responded with a empty Play Queue", "", "");
      return;
    }

    CLog::Log(LOGDEBUG, "CPlayQueueManager::OnJobComplete going to play a playlist of size %d and type %d", fj->m_items.Size(), playlist);

    g_playlistPlayer.SetCurrentPlaylist(playlist);
    CApplicationMessenger::Get().PlayListPlayerClear(playlist);
    CApplicationMessenger::Get().PlayListPlayerAdd(playlist, fj->m_items);
    CApplicationMessenger::Get().PlayListPlayerPlay(selectedOffset);
  }
  else
  {
    CGUIDialogOK::ShowAndGetInput("Error creating the Play Queue", "The server was unable to create the Play Queue", "", "");
  }
}
