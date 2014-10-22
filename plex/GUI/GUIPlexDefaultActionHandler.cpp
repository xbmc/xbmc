
#include "GUIPlexDefaultActionHandler.h"
#include "PlexExtraDataLoader.h"
#include "Application.h"
#include "PlexApplication.h"
#include "Playlists/PlexPlayQueueManager.h"
#include "guilib/GUIWindowManager.h"
#include <boost/foreach.hpp>
#include "dialogs/GUIDialogYesNo.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "GUIBaseContainer.h"
#include "Client/PlexServerManager.h"
#include "ApplicationMessenger.h"
#include "VideoInfoTag.h"
#include "GUIMessage.h"
#include "GUI/GUIDialogPlayListSelection.h"
#include "GUI/GUIDialogPlexError.h"
#include "Client/PlexServer.h"
#include "guilib/GUIKeyboardFactory.h"
#include "LocalizeStrings.h"


///////////////////////////////////////////////////////////////////////////////////////////////////
CGUIPlexDefaultActionHandler::CGUIPlexDefaultActionHandler()
{
  ACTION_SETTING* action;
  
  action = new ACTION_SETTING(ACTION_PLAYER_PLAY);
  action->WindowSettings[WINDOW_HOME].contextMenuVisisble = false;
  action->WindowSettings[WINDOW_PLEX_PLAY_QUEUE].contextMenuVisisble = true;
  action->WindowSettings[WINDOW_VIDEO_NAV].contextMenuVisisble = true;
  action->WindowSettings[WINDOW_PLEX_PLAYLIST_SELECTION].contextMenuVisisble = true;
  m_ActionSettings.push_back(*action);
  
  action = new ACTION_SETTING(ACTION_PLEX_PLAY_ALL);
  action->WindowSettings[WINDOW_VIDEO_NAV].contextMenuVisisble = false;
  m_ActionSettings.push_back(*action);
  
  action = new ACTION_SETTING(ACTION_PLEX_SHUFFLE_ALL);
  action->WindowSettings[WINDOW_VIDEO_NAV].contextMenuVisisble = true;
  m_ActionSettings.push_back(*action);

  action = new ACTION_SETTING(ACTION_PLEX_NOW_PLAYING);
  action->WindowSettings[WINDOW_HOME].contextMenuVisisble = true;
  action->WindowSettings[WINDOW_PLEX_PLAY_QUEUE].contextMenuVisisble = true;
  action->WindowSettings[WINDOW_VIDEO_NAV].contextMenuVisisble = true;
  action->WindowSettings[WINDOW_PLEX_PLAYLIST_SELECTION].contextMenuVisisble = true;
  m_ActionSettings.push_back(*action);
  
  action = new ACTION_SETTING(ACTION_PLEX_PLAY_TRAILER);
  action->WindowSettings[WINDOW_HOME].contextMenuVisisble = true;
  action->WindowSettings[WINDOW_PLEX_PLAY_QUEUE].contextMenuVisisble = true;
  action->WindowSettings[WINDOW_VIDEO_NAV].contextMenuVisisble = true;
  m_ActionSettings.push_back(*action);

  action = new ACTION_SETTING(ACTION_QUEUE_ITEM);
  action->WindowSettings[WINDOW_HOME].contextMenuVisisble = true;
  action->WindowSettings[WINDOW_VIDEO_NAV].contextMenuVisisble = true;
  m_ActionSettings.push_back(*action);

  action = new ACTION_SETTING(ACTION_PLEX_PQ_ADDUPTONEXT);
  action->WindowSettings[WINDOW_HOME].contextMenuVisisble = true;
  action->WindowSettings[WINDOW_VIDEO_NAV].contextMenuVisisble = true;
  m_ActionSettings.push_back(*action);
  
  action = new ACTION_SETTING(ACTION_MARK_AS_WATCHED);
  action->WindowSettings[WINDOW_HOME].contextMenuVisisble = true;
  action->WindowSettings[WINDOW_VIDEO_NAV].contextMenuVisisble = true;
  action->WindowSettings[WINDOW_PLEX_PLAY_QUEUE].contextMenuVisisble = true;
  m_ActionSettings.push_back(*action);
  
  action = new ACTION_SETTING(ACTION_MARK_AS_UNWATCHED);
  action->WindowSettings[WINDOW_HOME].contextMenuVisisble = true;
  action->WindowSettings[WINDOW_VIDEO_NAV].contextMenuVisisble = true;
  action->WindowSettings[WINDOW_PLEX_PLAY_QUEUE].contextMenuVisisble = true;
  m_ActionSettings.push_back(*action);
  
  action = new ACTION_SETTING(ACTION_TOGGLE_WATCHED);
  action->WindowSettings[WINDOW_HOME].contextMenuVisisble = false;
  action->WindowSettings[WINDOW_VIDEO_NAV].contextMenuVisisble = false;
  m_ActionSettings.push_back(*action);

  action = new ACTION_SETTING(ACTION_PLEX_PQ_CLEAR);
  action->WindowSettings[WINDOW_HOME].contextMenuVisisble = true;
  action->WindowSettings[WINDOW_PLEX_PLAY_QUEUE].contextMenuVisisble = true;
  m_ActionSettings.push_back(*action);

  action = new ACTION_SETTING(ACTION_DELETE_ITEM);
  action->WindowSettings[WINDOW_HOME].contextMenuVisisble = true;
  action->WindowSettings[WINDOW_PLEX_PLAY_QUEUE].contextMenuVisisble = true;
  action->WindowSettings[WINDOW_VIDEO_NAV].contextMenuVisisble = true;
  action->WindowSettings[WINDOW_PLEX_PLAYLIST_SELECTION].contextMenuVisisble = true;
  m_ActionSettings.push_back(*action);
  
  action = new ACTION_SETTING(ACTION_PLEX_PL_ADDTO);
  action->WindowSettings[WINDOW_HOME].contextMenuVisisble = true;
  action->WindowSettings[WINDOW_VIDEO_NAV].contextMenuVisisble = true;
  m_ActionSettings.push_back(*action);

  action = new ACTION_SETTING(ACTION_PLEX_PL_CREATE);
  action->WindowSettings[WINDOW_HOME].contextMenuVisisble = true;
  action->WindowSettings[WINDOW_VIDEO_NAV].contextMenuVisisble = true;
  m_ActionSettings.push_back(*action);

}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CGUIPlexDefaultActionHandler::GetContextButtons(int windowID, CFileItemPtr item, CFileItemListPtr container, CContextButtons& buttons)
{
  // check if the action is supported
  for (ActionsSettingListIterator it = m_ActionSettings.begin(); it != m_ActionSettings.end(); ++it)
  {
    ActionWindowSettingsMapIterator itwin = it->WindowSettings.find(windowID);
    if ((itwin != it->WindowSettings.end()) && (itwin->second.contextMenuVisisble))
    {
      GetContextButtonsForAction(it->actionID, item, container, buttons);
    }
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CGUIPlexDefaultActionHandler::OnAction(int windowID, CAction action, CFileItemPtr item, CFileItemListPtr container)
{
  CGUIWindow* window = g_windowManager.GetWindow(windowID);
  int actionID = action.GetID();

  // if the action is not known, then just exit
  ACTION_SETTING* setting = NULL;
  for (ActionsSettingListIterator it = m_ActionSettings.begin(); it != m_ActionSettings.end(); ++it)
  {
    if (it->actionID == action.GetID())
    {
      setting = &(*it);
      break;
    }
  }

  if (!setting)
    return false;

  // if the action is known, but not available for the window, then exit
  if (setting->WindowSettings.find(windowID) == setting->WindowSettings.end())
    return false;

  if (item)
  {
    EPlexDirectoryType dirType = item->GetPlexDirectoryType();
  
    // actions that require an item
    switch (actionID)
    {
      case ACTION_PLAYER_PLAY:
        PlayMedia(item, container);
        return true;
        break;
        
      case ACTION_PLEX_PLAY_TRAILER:

        if (item->GetPlexDirectoryType() == PLEX_DIR_TYPE_MOVIE)
        {
          CPlexExtraDataLoader loader;
          if (loader.getDataForItem(item) && loader.getItems()->Size())
          {
            /// we dont want quality selection menu for trailers
            CFileItem trailerItem(*loader.getItems()->Get(0));
            trailerItem.SetProperty("avoidPrompts", true);
            g_application.PlayFile(trailerItem, true);
            
          }
        }
        return true;
        break;

      case ACTION_MARK_AS_WATCHED:
        if (item->IsVideo() && item->IsPlexMediaServer())
        {
          item->MarkAsWatched(true);
          g_windowManager.SendMessage(GUI_MSG_PLEX_ITEM_WATCHEDSTATE_CHANGED, 0, windowID, actionID, 0);
          return true;
        }
        break;
        
      case ACTION_MARK_AS_UNWATCHED:
        if (item->IsVideo() && item->IsPlexMediaServer())
        {
          item->MarkAsUnWatched(true);
          g_windowManager.SendMessage(GUI_MSG_PLEX_ITEM_WATCHEDSTATE_CHANGED, 0, windowID, actionID, 0);
          return true;
        }
        break;
        
      case ACTION_TOGGLE_WATCHED:
        if (item->IsVideo() && item->IsPlexMediaServer())
        {
          if (item->GetVideoInfoTag()->m_playCount == 0)
            return OnAction(windowID, ACTION_MARK_AS_WATCHED, item, container);
          if (item->GetVideoInfoTag()->m_playCount > 0)
            return OnAction(windowID, ACTION_MARK_AS_UNWATCHED, item, container);
          break;
        }
        
      case ACTION_PLEX_PQ_CLEAR:
        if (IsPlayQueueContainer(container) || item->HasProperty("playQueueItemID"))
        {
          g_plexApplication.playQueueManager->clear(PlexUtils::GetMediaTypeFromItem(item));
          return true;
        }
        break;

      case ACTION_DELETE_ITEM:
        // if we are on a PQ item, remove it from PQ
        if (IsPlayQueueContainer(container) || item->HasProperty("playQueueItemID"))
        {
          g_plexApplication.playQueueManager->removeItem(item);
          return true;
        }
        // we're on a playlist, delete it
        else if (dirType == PLEX_DIR_TYPE_PLAYLIST)
        {
          if (!item->GetProperty("smart").asInteger())
          {
            CURL plURL(item->GetPath());
            plURL.SetFileName("/playlists/" + item->GetProperty("ratingKey").asString());
            g_plexApplication.mediaServerClient->deleteItemFromPath(plURL.Get());
          }
        }
        // we're on a playlist item
        else if (IsPlayListContainer(container))
        {
          if (!container->GetProperty("smart").asInteger())
          {
            CURL plURL(item->GetPath());
            plURL.SetFileName("/playlists/" + PlexUtils::GetPlayListIDfromPath(container->GetPath()) +
                              "/items/" + item->GetProperty("playlistItemID").asString());
            g_plexApplication.mediaServerClient->deleteItemFromPath(plURL.Get());
          }
        }
        else
        {
          // we're one a regular item, try to delete it
          // Confirm.
          if (!CGUIDialogYesNo::ShowAndGetInput(750, 125, 0, 0))
            return false;

          g_plexApplication.mediaServerClient->deleteItem(item);

          /* marking as watched and is on the on deck list, we need to remove it then */
          CGUIBaseContainer* container = (CGUIBaseContainer*)window->GetFocusedControl();
          if (container)
          {
            std::vector<CGUIListItemPtr> items = container->GetItems();
            int idx = std::distance(items.begin(), std::find(items.begin(), items.end(), item));
            CGUIMessage msg(GUI_MSG_LIST_REMOVE_ITEM, windowID, window->GetFocusedControlID(),
                            idx + 1, 0);
            window->OnMessage(msg);
            return true;
          }
        }
        break;

      case ACTION_QUEUE_ITEM:
        if (!item->HasProperty("playQueueItemID"))
        {
          g_plexApplication.playQueueManager->QueueItem(item, true);
          return true;
        }
        break;

      case ACTION_PLEX_PQ_ADDUPTONEXT:
        if (!item->HasProperty("playQueueItemID"))
        {
          g_plexApplication.playQueueManager->QueueItem(item, false);
          return true;
        }
        break;
        
      case ACTION_PLEX_PL_ADDTO:
      {
        if (IsItemPlaylistCompatible(item))
        {
          CPlexServerPtr server = g_plexApplication.serverManager->FindFromItem(item);
          
          CGUIDialogPlaylistSelection *plDialog = (CGUIDialogPlaylistSelection *)g_windowManager.GetWindow(WINDOW_DIALOG_PLEX_PLAYLIST_SELECT);
          
          plDialog->filterPlaylist(PlexUtils::GetMediaTypeFromItem(item), server);
          
          plDialog->DoModal();
          if (plDialog->IsConfirmed())
          {
            CFileItemPtr plItem =  plDialog->GetSelectedItem();
            if (plItem)
            {
              CStdString playlistID = plItem->GetProperty("unprocessed_ratingkey").asString();
              
              
              if (server)
              {
                if (g_plexApplication.mediaServerClient->addItemToPlayList(server, item, playlistID, true))
                {
                  CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info, "Playlist addition", "Item was successfully added to the playlist");
                  return true;
                }
                else
                  CLog::Log(LOGERROR, "CGUIPlexDefaultActionHandler : Playlist failure when adding item");
              }
              else
                CLog::Log(LOGERROR, "CGUIPlexDefaultActionHandler : Can't a valid server for selected playlist");
            }
            else
              CLog::Log(LOGERROR, "CGUIPlexDefaultActionHandler : Can't get Playlist selection item");
            
            CGUIDialogPlexError::ShowError("Playlist Error", "The item could not be added to the playlist", "", "");
          }
        }
        break;
      }

      case ACTION_PLEX_PL_CREATE:
      {
        if (IsItemPlaylistCompatible(item))
        {
          CStdString playlistName;
          if (CGUIKeyboardFactory::ShowAndGetInput(playlistName, g_localizeStrings.Get(52614), false))
          {
            CPlexServerPtr server = g_plexApplication.serverManager->FindFromItem(item);

            g_plexApplication.mediaServerClient->createPlayList(server, playlistName, item, false, true);
          }
          return true;
        }
        break;
      }
    }
  }

  // other actions that dont need an itemp.
  switch (actionID)
  {
    case ACTION_PLEX_NOW_PLAYING:
      m_navHelper.navigateToNowPlaying();
      return true;
      break;
      
    case ACTION_PLEX_PLAY_ALL:
      PlayAll(container, false);
      return true;
      break;
      
    case ACTION_PLEX_SHUFFLE_ALL:
      PlayAll(container, true);
      return true;
      break;


      
  }

  return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CGUIPlexDefaultActionHandler::GetContextButtonsForAction(int actionID, CFileItemPtr item,
                                                              CFileItemListPtr container, CContextButtons& buttons)
{
  
  EPlexDirectoryType dirType = item->GetPlexDirectoryType();
  
  switch (actionID)
  {
    case ACTION_PLAYER_PLAY:
      buttons.Add(actionID, 208);
      break;
      
    case ACTION_PLEX_PLAY_TRAILER:
      if (item->GetPlexDirectoryType() == PLEX_DIR_TYPE_MOVIE)
      {
        CPlexExtraDataLoader loader;
        if (loader.getDataForItem(item) && loader.getItems()->Size())
          buttons.Add(actionID, 44550);
      }
      break;
      
    case ACTION_PLEX_NOW_PLAYING:
      if (g_application.IsPlaying())
        buttons.Add(actionID, 13350);
      break;
      
    case ACTION_PLEX_SHUFFLE_ALL:
      if (container->Size())
        buttons.Add(actionID, 52600);
      break;

    case ACTION_MARK_AS_WATCHED:
    {
      if (item->IsVideo() && item->IsPlexMediaServer())
      {
        CStdString viewOffset = item->GetProperty("viewOffset").asString();
        
        if (item->GetVideoInfoTag()->m_playCount == 0 || viewOffset.size() > 0)
          buttons.Add(actionID, 16103);
      }
      break;
    }
      
    case ACTION_MARK_AS_UNWATCHED:
    {
      if (item->IsVideo() && item->IsPlexMediaServer())
      {
        CStdString viewOffset = item->GetProperty("viewOffset").asString();
        
        if (item->GetVideoInfoTag()->m_playCount > 0 || viewOffset.size() > 0)
          buttons.Add(actionID, 16104);
      }
      break;
    }

    case ACTION_PLEX_PQ_CLEAR:
      if (IsPlayQueueContainer(container) || item->HasProperty("playQueueItemID"))
        buttons.Add(actionID, 192);
      break;

    case ACTION_DELETE_ITEM:
      if (IsPlayQueueContainer(container) || item->HasProperty("playQueueItemID"))
      {
        buttons.Add(actionID, 1210);
      }
      else if (dirType == PLEX_DIR_TYPE_PLAYLIST)
      {
        if (!item->GetProperty("smart").asInteger())
          buttons.Add(actionID, 117);
      }
      else if (IsPlayListContainer(container))
      {
        PlexUtils::PrintItemProperties(container);
        
        if (!container->GetProperty("smart").asInteger())
          buttons.Add(actionID, 117);
      }
      else
      {
        if (item->IsPlexMediaServerLibrary() &&
            (item->IsRemoteSharedPlexMediaServerLibrary() == false) &&
            (dirType == PLEX_DIR_TYPE_EPISODE || dirType == PLEX_DIR_TYPE_MOVIE ||
             dirType == PLEX_DIR_TYPE_VIDEO || dirType == PLEX_DIR_TYPE_TRACK))
        {
          CPlexServerPtr server =
          g_plexApplication.serverManager->FindByUUID(item->GetProperty("plexserver").asString());
          if (server && server->SupportsDeletion())
            buttons.Add(actionID, 117);
        }
      }
      break;

    case ACTION_QUEUE_ITEM:
    {
      if (!item->HasProperty("playQueueItemID"))
      {
        CFileItemList pqlist;
        g_plexApplication.playQueueManager->getPlayQueue(PlexUtils::GetMediaTypeFromItem(item), pqlist);
        
        if (pqlist.Size())
          buttons.Add(actionID, 52602);
        else
          buttons.Add(actionID, 52607);
      }
      break;
    }

    case ACTION_PLEX_PQ_ADDUPTONEXT:
    {
      if (!item->HasProperty("playQueueItemID"))
      {
        ePlexMediaType itemType = PlexUtils::GetMediaTypeFromItem(item);
        if (g_plexApplication.playQueueManager->getPlayQueueOfType(itemType))
          buttons.Add(actionID, 52603);
      }
      break;
    }
      
    case ACTION_PLEX_PL_ADDTO:
    {
      if (IsItemPlaylistCompatible(item))
      {
        buttons.Add(actionID, 52612);
      }
      break;
    }

    case ACTION_PLEX_PL_CREATE:
    {
      if (IsItemPlaylistCompatible(item))
      {
        buttons.Add(actionID, 52613);
      }
      break;
    }
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CGUIPlexDefaultActionHandler::PlayMedia(CFileItemPtr item, CFileItemListPtr container)
{
  if (!item)
    return false;

  if (IsPhotoContainer(container))
  {
    if (item->m_bIsFolder)
      CApplicationMessenger::Get().PictureSlideShow(item->GetPath(), false);
    else
      CApplicationMessenger::Get().PictureSlideShow(container->GetPath(), false, item->GetPath());
  }
  else if (IsMusicContainer(container) && !item->m_bIsFolder)
  {
    PlayAll(container, false, item);
  }
  else if (item->HasProperty("playQueueItemID"))
  {
    // we are on a PQ item, play the PQ
    g_plexApplication.playQueueManager->playId(PlexUtils::GetMediaTypeFromItem(item), -1);
  }
  else
  {
    std::string uri = GetFilteredURI(*item);

    g_plexApplication.playQueueManager->create(*item, uri);
  }

  return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
void CGUIPlexDefaultActionHandler::PlayAll(CFileItemListPtr container, bool shuffle,
                                           const CFileItemPtr& fromHere)
{
  if (IsPhotoContainer(container))
  {
    // Photos are handled a bit different
    CApplicationMessenger::Get().PictureSlideShow(container->GetPath(), false,
                                                  fromHere ? fromHere->GetPath() : "", shuffle);
    return;
  }

  CPlexServerPtr server;
  if (container->HasProperty("plexServer"))
    server =
    g_plexApplication.serverManager->FindByUUID(container->GetProperty("plexServer").asString());

  CStdString fromHereKey;
  if (fromHere)
    fromHereKey = fromHere->GetProperty("key").asString();

  // take out the plexserver://plex part from above when passing it down
  CStdString uri = GetFilteredURI(*container);

  CPlexPlayQueueOptions options;
  options.startItemKey = fromHereKey;
  options.startPlaying = true;
  options.shuffle = shuffle;
  options.showPrompts = true;

  g_plexApplication.playQueueManager->create(*container, uri, options);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
std::string CGUIPlexDefaultActionHandler::GetFilteredURI(const CFileItem& item) const
{
  CURL itemUrl(item.GetPath());
  
  itemUrl.SetProtocol("plexserver");
  itemUrl.SetHostName("plex");
  
  if (itemUrl.HasOption("unwatchedLeaves"))
  {
    itemUrl.SetOption("unwatched", itemUrl.GetOption("unwatchedLeaves"));
    itemUrl.RemoveOption("unwatchedLeaves");
  }

  if (item.GetPlexDirectoryType() == PLEX_DIR_TYPE_SHOW ||
      (item.GetPlexDirectoryType() == PLEX_DIR_TYPE_SEASON && item.HasProperty("size")))
  {
    std::string fname = itemUrl.GetFileName();
    boost::replace_last(fname, "/children", "/allLeaves");
    itemUrl.SetFileName(fname);
  }

  // set sourceType
  if (item.m_bIsFolder)
  {
    CStdString sourceType = boost::lexical_cast<CStdString>(PlexUtils::GetFilterType(item));
    itemUrl.SetOption("sourceType", sourceType);
  }

  return CPlexPlayQueueManager::getURIFromItem(item,itemUrl.GetUrlWithoutOptions().substr(17, std::string::npos));
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CGUIPlexDefaultActionHandler::IsPhotoContainer(CFileItemListPtr container)
{
  if (!container)
    return false;
  
  EPlexDirectoryType dirType = container->GetPlexDirectoryType();

  if (dirType == PLEX_DIR_TYPE_CHANNEL && container->Get(0))
    dirType = container->Get(0)->GetPlexDirectoryType();

  return (dirType == PLEX_DIR_TYPE_PHOTOALBUM | dirType == PLEX_DIR_TYPE_PHOTO);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CGUIPlexDefaultActionHandler::IsMusicContainer(CFileItemListPtr container)
{
  if (!container)
    return false;

  EPlexDirectoryType dirType = container->GetPlexDirectoryType();
  if (dirType == PLEX_DIR_TYPE_CHANNEL && container->Get(0))
    dirType = container->Get(0)->GetPlexDirectoryType();
  return (dirType == PLEX_DIR_TYPE_ALBUM || dirType == PLEX_DIR_TYPE_ARTIST ||
          dirType == PLEX_DIR_TYPE_TRACK);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CGUIPlexDefaultActionHandler::IsVideoContainer(CFileItemListPtr container)
{
  if (!container)
    return false;
  
  EPlexDirectoryType dirType = container->GetPlexDirectoryType();
  
  if (dirType == PLEX_DIR_TYPE_CHANNEL && container->Get(0))
    dirType = container->Get(0)->GetPlexDirectoryType();
  
  return (dirType == PLEX_DIR_TYPE_MOVIE    ||
          dirType == PLEX_DIR_TYPE_SHOW     ||
          dirType == PLEX_DIR_TYPE_SEASON   ||
          dirType == PLEX_DIR_TYPE_PLAYLIST ||
          dirType == PLEX_DIR_TYPE_EPISODE  ||
          dirType == PLEX_DIR_TYPE_VIDEO    ||
          dirType == PLEX_DIR_TYPE_CLIP);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CGUIPlexDefaultActionHandler::IsPlayListContainer(CFileItemListPtr container)
{
  if (!container)
    return false;
  
  CURL u(container->GetPath());
  
  if (boost::algorithm::starts_with(u.GetFileName(),"playlists"))
    return true;
  
  return false;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CGUIPlexDefaultActionHandler::IsPlayQueueContainer(CFileItemListPtr container) 
{
  if (!container)
    return false;
  
  CURL u(container->GetPath());
  
  if (u.GetHostName() == "playqueue")
    return true;
  
  return false;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CGUIPlexDefaultActionHandler::IsItemPlaylistCompatible(CFileItemPtr item)
{
  switch (item->GetPlexDirectoryType())
  {
    case PLEX_DIR_TYPE_TRACK:
    case PLEX_DIR_TYPE_MOVIE:
    case PLEX_DIR_TYPE_ALBUM:
    case PLEX_DIR_TYPE_EPISODE:
      return true;
      break;

    default:
      return false;
      break;
  }
}
