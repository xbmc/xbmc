#include "PlexPlayQueueManager.h"
#include "PlexPlayQueueServer.h"
#include "URL.h"
#include "PlayListPlayer.h"
#include "playlists/PlayList.h"
#include "ApplicationMessenger.h"
#include "FileItem.h"
#include "PlexApplication.h"
#include "Client/PlexServerManager.h"
#include "PlexPlayQueueLocal.h"
#include "settings/GUISettings.h"
#include "guilib/GUIWindowManager.h"
#include "music/tags/MusicInfoTag.h"

using namespace PLAYLIST;

///////////////////////////////////////////////////////////////////////////////////////////////////
void CPlexPlayQueueManager::create(const CFileItem& container, const CStdString& uri,
                                   const CStdString& startItemKey, bool shuffle)
{
  IPlexPlayQueueBasePtr impl = getImpl(container);
  if (impl)
  {
    m_currentImpl = impl;
    m_currentImpl->create(container, uri, startItemKey, shuffle);
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
int CPlexPlayQueueManager::getPlaylistFromType(ePlexMediaType type)
{
  switch (type)
  {
    case PLEX_MEDIA_TYPE_MUSIC:
      return PLAYLIST_MUSIC;
    case PLEX_MEDIA_TYPE_VIDEO:
      return PLAYLIST_VIDEO;
    default:
      return PLAYLIST_NONE;
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CPlexPlayQueueManager::playCurrentId(int id)
{
  if (!m_currentImpl)
    return;
  ePlexMediaType type = getCurrentPlayQueueType();
  playQueueUpdated(type, true, id);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CPlexPlayQueueManager::playQueueUpdated(const ePlexMediaType& type, bool startPlaying, int id)
{
  if (!m_currentImpl)
    return;

  CFileItemPtr playlistItem;
  int playlist = getPlaylistFromType(type);
  if (playlist != PLAYLIST_NONE)
  {
    CPlayList pl = g_playlistPlayer.GetPlaylist(type);
    if (pl.size() > 0)
      playlistItem = pl[0];
  }

  int pqID = m_currentImpl->getCurrentID();

  CFileItemList list;
  if (!m_currentImpl->getCurrent(list))
    return;

  int selectedOffset;
  bool hasChanged = false;

  if (g_playlistPlayer.GetCurrentPlaylist() == playlist && playlistItem &&
      playlistItem->GetProperty("playQueueID").asInteger() == pqID)
  {
    hasChanged = reconcilePlayQueueChanges(type, list);
  }
  else
  {
    selectedOffset = list.GetProperty("playQueueSelectedItemOffset").asInteger(0);
    //CApplicationMessenger::Get().MediaStop(true);
    g_playlistPlayer.ClearPlaylist(playlist);
    g_playlistPlayer.Add(playlist, list);
    hasChanged = true;

    saveCurrentPlayQueue(m_currentImpl->server(), list);

    CLog::Log(LOGDEBUG,
              "CPlexPlayQueueManager::PlayQueueUpdated now playing PlayQueue of type %d",
              type);
  }

  if (hasChanged)
  {
    CGUIMessage msg(GUI_MSG_PLEX_PLAYQUEUE_UPDATED, PLEX_PLAYQUEUE_MANAGER, 0);
    g_windowManager.SendThreadMessage(msg);
  }

  if (startPlaying)
  {
    g_playlistPlayer.SetCurrentPlaylist(playlist);
    if (id == -1)
      g_playlistPlayer.Play(selectedOffset);
    else
      g_playlistPlayer.PlaySongId(id);
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CPlexPlayQueueManager::saveCurrentPlayQueue(const CPlexServerPtr& server,
                                                 const CFileItemList& list)
{
  m_playQueueType = PlexUtils::GetMediaTypeFromItem(list);

  int playQueueID = list.GetProperty("playQueueID").asInteger();
  if (playQueueID > 0)
  {
    CStdString path;
    path.Format("%d", playQueueID);

    g_guiSettings.SetString("system.mostrecentplayqueue", server->BuildPlexURL(path).Get());
  }
  else
    // reset the saved state - we don't want it to show an old playlist
    g_guiSettings.SetString("system.mostrecentplayqueue", "");
}

///////////////////////////////////////////////////////////////////////////////////////////////////
CStdString CPlexPlayQueueManager::getURIFromItem(const CFileItem& item, const CStdString& uri)
{
  if (item.GetPath().empty())
    return "";

  CURL u(item.GetPath());

  if (u.GetProtocol() != "plexserver")
    return "";

  CStdString itemDirStr = "item";
  if (item.m_bIsFolder)
    itemDirStr = "directory";

  if (!item.HasProperty("librarySectionUUID"))
  {
    CLog::Log(LOGWARNING,
              "CPlexPlayQueueManager::getURIFromItem item %s doesn't have a section UUID",
              item.GetPath().c_str());
    return "";
  }

  CStdString realURI =
  uri.empty() ? (CStdString)item.GetProperty("unprocessed_key").asString() : uri;
  CURL::Encode(realURI);

  CStdString ret;
  ret.Format("library://%s/%s/%s", item.GetProperty("librarySectionUUID").asString(), itemDirStr,
             realURI);

  return ret;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CPlexPlayQueueManager::reconcilePlayQueueChanges(int playlistType, const CFileItemList& list)
{
  bool firstCommon = false;
  int listCursor = 0;
  int playlistCursor = 0;
  bool hasChanged = false;

  CPlayList playlist(g_playlistPlayer.GetPlaylist(playlistType));

  while (true)
  {
    int listItemId = -1;
    if (playlistCursor < playlist.size())
      listItemId = playlist[playlistCursor]->GetMusicInfoTag()->GetDatabaseId();

    if (listCursor >= list.Size())
      break;

    int serverItemId = list.Get(listCursor)->GetMusicInfoTag()->GetDatabaseId();

    if (!firstCommon && listItemId != -1)
    {
      if (listItemId == serverItemId)
      {
        firstCommon = true;
        listCursor++;
      }
      else
      {
        // remove the item at in the playlist.
        g_playlistPlayer.Remove(playlistType, playlistCursor);

        // we also need to remove it from the list we are iterating to make sure the
        // offsets match up
        playlist.Remove(playlistCursor);

        hasChanged = true;
        CLog::Log(LOGDEBUG,
                  "CPlexPlayQueueManager::reconcilePlayQueueChanges removing item %d from playlist %d",
                  playlistCursor, playlistType);
        continue;
      }
    }
    else if (listItemId == -1)
    {
      CLog::Log(LOGDEBUG,
                "CPlexPlayQueueManager::reconcilePlayQueueChanges adding item %d to playlist %d",
                serverItemId, playlistType);

      g_playlistPlayer.Add(playlistType, list.Get(listCursor));
      // add it to the local copy just to make sure they match up.
      playlist.Add(list.Get(listCursor));

      hasChanged = true;
      listCursor++;
    }
    else if (listItemId == serverItemId)
    {
      // we will update the current item with new properties
      // because for example index might change after a update.
      playlist[playlistCursor]->ClearProperties();
      playlist[playlistCursor]->AppendProperties(*(list.Get(listCursor)));

      listCursor++;
    }
    else
    {
      g_playlistPlayer.Remove(playlistType, playlistCursor);
      playlist.Remove(playlistCursor);
      hasChanged = true;
      continue;
    }

    playlistCursor++;
  }

  return hasChanged;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CPlexPlayQueueManager::getCurrentPlayQueue(CFileItemList& list)
{
  if (m_currentImpl)
    return m_currentImpl->getCurrent(list);
  return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CPlexPlayQueueManager::loadSavedPlayQueue()
{
  CURL playQueueURL(g_guiSettings.GetString("system.mostrecentplayqueue"));
  CPlexServerPtr server = g_plexApplication.serverManager->FindByUUID(playQueueURL.GetHostName());
  if (server && !m_currentImpl)
  {
    m_currentImpl = IPlexPlayQueueBasePtr(new CPlexPlayQueueServer(server));
    m_currentImpl->get(playQueueURL.GetFileName());
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
IPlexPlayQueueBasePtr CPlexPlayQueueManager::getImpl(const CFileItem& container)
{
  CPlexServerPtr server = g_plexApplication.serverManager->FindFromItem(container);
  if (server)
  {
    CLog::Log(LOGDEBUG, "CPlexPlayQueueManager::getImpl identifier: %s server: %s",
              container.GetProperty("identifier").asString().c_str(),
              server->toString().c_str());

    if (container.GetProperty("identifier").asString() == "com.plexapp.plugins.library" &&
        CPlexPlayQueueServer::isSupported(server))
    {
      CLog::Log(LOGDEBUG, "CPlexPlayQueueManager::getImpl selecting PlexPlayQueueServer");
      return IPlexPlayQueueBasePtr(new CPlexPlayQueueServer(server));
    }

    CLog::Log(LOGDEBUG, "CPlexPlayQueueManager::getImpl selecting PlexPlayQueueLocal");
    return IPlexPlayQueueBasePtr(new CPlexPlayQueueLocal(server));
  }

  CLog::Log(LOGDEBUG, "CPlexPlayQueueManager::getImpl can't select implementation");
  return IPlexPlayQueueBasePtr();
}
