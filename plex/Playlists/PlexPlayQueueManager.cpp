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
#include "Client/PlexTimelineManager.h"
#include "dialogs/GUIDialogYesNo.h"
#include "LocalizeStrings.h"
#include "Application.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "URIUtils.h"

using namespace PLAYLIST;

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CPlexPlayQueueManager::create(const CFileItem& container, const CStdString& uri,
                                   const CPlexPlayQueueOptions& options)
{
  // look for existing PQ of same type
  CPlexPlayQueuePtr pq = getPlayQueueOfType(PlexUtils::GetMediaTypeFromItem(container));
  
  if (pq && pq->getVersion() > 1 && options.showPrompts)
  {
    CFileItemList list;
    if (pq->get(list))
    {
      if (list.HasProperty("playQueueLastAddedItemID"))
      {
        // Give user a warning since this will clear the current
        // play queue
        bool canceled;
        if (!CGUIDialogYesNo::ShowAndGetInput(g_localizeStrings.Get(52604), g_localizeStrings.Get(52605),
                                              g_localizeStrings.Get(52606), "", canceled) || canceled)
          return false;
      }
    }
  }

  // create a new pq
  pq = getImpl(container);
  if (pq)
  {
    m_playQueues[PlexUtils::GetMediaTypeFromItem(container)] = pq;
    return pq->create(container, uri, options);
  }

  return false;
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
void CPlexPlayQueueManager::playId(ePlexMediaType type, int id)
{
  if (!getPlayQueueOfType(type))
    return;
  playQueueUpdated(type, true, id);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CPlexPlayQueueManager::clear()
{
  if (PlexUtils::IsPlayingPlaylist())
    CApplicationMessenger::Get().MediaStop();

  m_playQueues.clear();
 
  CGUIMessage msg(GUI_MSG_PLEX_PLAYQUEUE_UPDATED, PLEX_PLAYQUEUE_MANAGER, 0);
  g_windowManager.SendThreadMessage(msg);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CPlexPlayQueueManager::clear(ePlexMediaType type)
{
  if (PlexUtils::IsPlayingPlaylist())
    CApplicationMessenger::Get().MediaStop();
  
  m_playQueues.erase(type);
    
  // TODO : Save here by type
  CGUIMessage msg(GUI_MSG_PLEX_PLAYQUEUE_UPDATED, PLEX_PLAYQUEUE_MANAGER, 0);
  g_windowManager.SendThreadMessage(msg);
  
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CPlexPlayQueueManager::playQueueUpdated(const ePlexMediaType& type, bool startPlaying, int id)
{
  CPlexPlayQueuePtr pq = getPlayQueueOfType(type);
  if (!pq)
    return;

  CFileItemPtr playlistItem;
  int playlist = getPlaylistFromType(type);
  if (playlist != PLAYLIST_NONE)
  {
    CPlayList pl = g_playlistPlayer.GetPlaylist(type);
    if (pl.size() > 0)
      playlistItem = pl[0];
  }

  int pqID = pq->getID();

  CFileItemList list;
  if (!pq->get(list))
    return;

  int selectedId = id;
  bool hasChanged = false;

  if (g_playlistPlayer.GetCurrentPlaylist() == playlist && playlistItem &&
      playlistItem->GetProperty("playQueueID").asInteger() == pqID)
  {
    hasChanged = reconcilePlayQueueChanges(type, list);
  }
  else
  {
    if (selectedId == -1)
      selectedId = list.GetProperty("playQueueSelectedItemID").asInteger(0);

    g_playlistPlayer.ClearPlaylist(playlist);
    g_playlistPlayer.Add(playlist, list);
    hasChanged = true;

    CLog::Log(LOGDEBUG,
              "CPlexPlayQueueManager::PlayQueueUpdated now playing PlayQueue of type %d",
              type);
  }

  pq->setVersion(list.GetProperty("playQueueVersion").asInteger());

  // no more items in that PQ, just remove it
  if (!list.Size())
    m_playQueues.erase(type);
  
  if (hasChanged)
  {
    CGUIMessage msg(GUI_MSG_PLEX_PLAYQUEUE_UPDATED, PLEX_PLAYQUEUE_MANAGER, 0);
    g_windowManager.SendThreadMessage(msg);

    g_plexApplication.timelineManager->RefreshSubscribers();
  }

  if (startPlaying)
  {
    g_application.StopPlaying();
    g_playlistPlayer.SetCurrentPlaylist(playlist);
    g_playlistPlayer.SetRepeat(playlist, PLAYLIST::REPEAT_NONE);
    if (selectedId == -1)
      g_playlistPlayer.Play(0);
    else
      g_playlistPlayer.PlaySongId(selectedId);
  }
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

  CStdString librarySectionUUID;
  if (!item.HasProperty("librarySectionUUID"))
  {
    if (!item.HasProperty("extraType"))
    {
      CLog::Log(LOGWARNING,
                "CPlexPlayQueueManager::getURIFromItem item %s doesn't have a section UUID",
                item.GetPath().c_str());
      librarySectionUUID = "whatever";
    }
    else
      librarySectionUUID = "extras";
  }
  else
    librarySectionUUID = item.GetProperty("librarySectionUUID").asString();

  CStdString realURI;
  if (uri.empty())
  {
    realURI = (CStdString)item.GetProperty("unprocessed_key").asString();
    CURL::Encode(realURI);
  }
  else
  {
    realURI = uri;
  }

  CStdString ret;
  ret.Format("library://%s/%s/%s", librarySectionUUID, itemDirStr,
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
      listItemId = PlexUtils::GetItemListID(playlist[playlistCursor]);

    if (listCursor >= list.Size())
    {
      if (list.Size() != playlist.size())
        hasChanged = true;
      break;
    }

    int serverItemId = PlexUtils::GetItemListID(list.Get(listCursor));

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
bool CPlexPlayQueueManager::getPlayQueue(ePlexMediaType type, CFileItemList& list)
{
  CPlexPlayQueuePtr pq = getPlayQueueOfType(type);
  if (pq)
    return pq->get(list);
  return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
EPlexDirectoryType CPlexPlayQueueManager::getPlayQueueDirType(ePlexMediaType type) const
{
  const CFileItemList* list;
  
  CPlexPlayQueuePtr pq = getPlayQueueOfType(type);
  if (pq)
  {
    list = pq->get();
    if (list)
      return list->GetPlexDirectoryType();
  }

  return PLEX_DIR_TYPE_UNKNOWN;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CPlexPlayQueueManager::loadPlayQueue(const CPlexServerPtr& server,
                                          const std::string& playQueueID,
                                          const CPlexPlayQueueOptions& options)
{
  if (!server || playQueueID.empty())
    return false;

  CPlexPlayQueuePtr pq = CPlexPlayQueuePtr(new CPlexPlayQueueServer(server));
  pq->get(playQueueID, options);
  m_playQueues[pq->getType()] = pq;
  return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
CPlexPlayQueuePtr CPlexPlayQueueManager::getImpl(const CFileItem& container)
{
  CPlexServerPtr server = g_plexApplication.serverManager->FindFromItem(container);
  if (server)
  {
    CLog::Log(LOGDEBUG, "CPlexPlayQueueManager::getImpl identifier: %s server: %s, filename: %s",
              container.GetProperty("identifier").asString().c_str(),
              server->toString().c_str(),
              URIUtils::GetFileName(container.GetPath()).c_str());

    if ((boost::starts_with(container.GetAsUrl().GetFileName(), "playlists") ||
        (container.GetProperty("identifier").asString() == "com.plexapp.plugins.library" &&
         URIUtils::GetFileName(container.GetPath()) != "folder")) &&
        CPlexPlayQueueServer::isSupported(server))
    {
      CLog::Log(LOGDEBUG, "CPlexPlayQueueManager::getImpl selecting PlexPlayQueueServer");
      return CPlexPlayQueuePtr(new CPlexPlayQueueServer(server));
    }

    CLog::Log(LOGDEBUG, "CPlexPlayQueueManager::getImpl selecting PlexPlayQueueLocal");
    return CPlexPlayQueuePtr(new CPlexPlayQueueLocal(server));
  }

  CLog::Log(LOGDEBUG, "CPlexPlayQueueManager::getImpl can't select implementation");
  return CPlexPlayQueuePtr();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CPlexPlayQueueManager::addItem(const CFileItemPtr &item, bool next)
{
  CPlexPlayQueuePtr pq = getPlayQueueOfType(PlexUtils::GetMediaTypeFromItem(item));
  if (pq)
  {
    return pq->addItem(item, next);
  }
  return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CPlexPlayQueueManager::removeItem(const CFileItemPtr &item)
{
  CPlexPlayQueuePtr pq = getPlayQueueOfType(PlexUtils::GetMediaTypeFromItem(item));
  if (pq)
  {
    pq->removeItem(item);
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
int CPlexPlayQueueManager::getID(ePlexMediaType type)
{
  CPlexPlayQueuePtr pq = getPlayQueueOfType(type);
  if (pq)
    return pq->getID();
  return -1;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CPlexPlayQueueManager::refresh(ePlexMediaType type)
{
  CPlexPlayQueuePtr pq = getPlayQueueOfType(type);
  if (pq)
    return pq->refresh();
  return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CPlexPlayQueueManager::QueueItem(const CFileItemPtr& item, bool next)
{
  if (!item)
    return;

  bool isItemAudio = (PlexUtils::GetMediaTypeFromItem(*item)==PLEX_MEDIA_TYPE_MUSIC);
  bool isItemVideo = (PlexUtils::GetMediaTypeFromItem(*item)==PLEX_MEDIA_TYPE_VIDEO);

  bool success = false;
  CPlexPlayQueuePtr pq = g_plexApplication.playQueueManager->getPlayQueueOfType(PlexUtils::GetMediaTypeFromItem(item));
 
  if (!pq)
  {
    CPlexPlayQueueOptions options;
    options.startPlaying = false;
    success = create(*item, "", options);

    if ((g_application.IsPlayingAudio() && isItemVideo) ||
        (g_application.IsPlayingVideo() && isItemAudio))
      CApplicationMessenger::Get().MediaStop();
  }
  else
  {
    success = addItem(item, next);
  }

  if (success)
  {
    CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info,
                                          "Item Queued", "The item was added the current queue..",
                                          2500L, false);
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CPlexPlayQueueManager::moveItem(const CFileItemPtr &item, const CFileItemPtr& afteritem)
{
  CPlexPlayQueuePtr pq = getPlayQueueOfType(PlexUtils::GetMediaTypeFromItem(item));
  if (pq)
  {
    return pq->moveItem(item, afteritem);
  }
  return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CPlexPlayQueueManager::moveItem(const CFileItemPtr &item, int offset)
{
  CPlexPlayQueuePtr pq = getPlayQueueOfType(PlexUtils::GetMediaTypeFromItem(item));

  if (pq && item)
  {
     CFileItemListPtr list = CFileItemListPtr(new CFileItemList);

     if (pq->get(*list))
     {
       int itemIndex = list->IndexOfItem(item->GetPath());
       int targetpos = ((((itemIndex + offset) % list->Size()) + list->Size()) % list->Size());

       if (targetpos < itemIndex)
         targetpos--;

       return pq->moveItem(item, list->Get(targetpos));

     }
  }
  return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
CPlexPlayQueuePtr CPlexPlayQueueManager::getPlayQueueOfType(ePlexMediaType type) const
{
  PlayQueueMap::const_iterator it = m_playQueues.find(type);
  if (it != m_playQueues.end())
    return it->second;
  
  return CPlexPlayQueuePtr();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
CPlexPlayQueuePtr CPlexPlayQueueManager::getPlayingPlayQueue() const
{
  if (PlexUtils::IsPlayingPlaylist() && g_application.CurrentFileItemPtr())
  {
    ePlexMediaType type = PlexUtils::GetMediaTypeFromItem(g_application.CurrentFileItemPtr());
    return getPlayQueueOfType(type);
  }
  
  return CPlexPlayQueuePtr();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
CPlexPlayQueuePtr CPlexPlayQueueManager::getPlayQueueFromID(int id) const
{
  for (PlayQueueMap::const_iterator it = m_playQueues.begin(); it != m_playQueues.end(); ++it)
  {
    if (it->second->getID() == id)
      return it->second;
  }
  
  return CPlexPlayQueuePtr();
}

