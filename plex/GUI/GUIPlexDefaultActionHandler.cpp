
#include "GUIPlexDefaultActionHandler.h"
#include "PlexExtraDataLoader.h"
#include "Application.h"
#include "PlexApplication.h"
#include "PlayLists/PlexPlayQueueManager.h"
#include "guilib/GUIWindowManager.h"
#include <boost/foreach.hpp>
#include "dialogs/GUIDialogYesNo.h"
#include "GUIBaseContainer.h"
#include "Client/PlexServerManager.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
CGUIPlexDefaultActionHandler::CGUIPlexDefaultActionHandler()
{
  ACTION_SETTING* action;

  action = new ACTION_SETTING(ACTION_PLEX_NOW_PLAYING);
  action->WindowSettings[WINDOW_HOME].contextMenuVisisble = true;
  action->WindowSettings[WINDOW_PLEX_PLAY_QUEUE].contextMenuVisisble = true;
  action->WindowSettings[WINDOW_VIDEO_NAV].contextMenuVisisble = true;
  m_ActionSettings.push_back(*action);
  
  action = new ACTION_SETTING(ACTION_PLEX_PLAY_TRAILER);
  action->WindowSettings[WINDOW_HOME].contextMenuVisisble = false;
  action->WindowSettings[WINDOW_PLEX_PLAY_QUEUE].contextMenuVisisble = false;
  action->WindowSettings[WINDOW_VIDEO_NAV].contextMenuVisisble = false;
  m_ActionSettings.push_back(*action);

  action = new ACTION_SETTING(ACTION_QUEUE_ITEM);
  action->WindowSettings[WINDOW_HOME].contextMenuVisisble = true;
  action->WindowSettings[WINDOW_VIDEO_NAV].contextMenuVisisble = true;
  m_ActionSettings.push_back(*action);

  action = new ACTION_SETTING(ACTION_PLEX_PQ_ADDUPTONEXT);
  action->WindowSettings[WINDOW_HOME].contextMenuVisisble = true;
  action->WindowSettings[WINDOW_VIDEO_NAV].contextMenuVisisble = true;
  m_ActionSettings.push_back(*action);

  action = new ACTION_SETTING(ACTION_PLEX_PQ_CLEAR);
  action->WindowSettings[WINDOW_HOME].contextMenuVisisble = true;
  action->WindowSettings[WINDOW_PLEX_PLAY_QUEUE].contextMenuVisisble = true;
  m_ActionSettings.push_back(*action);

  action = new ACTION_SETTING(ACTION_DELETE_ITEM);
  action->WindowSettings[WINDOW_HOME].contextMenuVisisble = true;
  action->WindowSettings[WINDOW_PLEX_PLAY_QUEUE].contextMenuVisisble = true;
  action->WindowSettings[WINDOW_VIDEO_NAV].contextMenuVisisble = true;
  m_ActionSettings.push_back(*action);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CGUIPlexDefaultActionHandler::GetContextButtons(int windowID, CFileItemPtr item, CContextButtons& buttons)
{
  // check if the action is supported
  for (ActionsSettingListIterator it = m_ActionSettings.begin(); it != m_ActionSettings.end(); ++it)
  {
    ActionWindowSettingsMapIterator itwin = it->WindowSettings.find(windowID);
    if ((itwin != it->WindowSettings.end()) && (itwin->second.contextMenuVisisble))
    {
      GetContextButtonsForAction(it->actionID, item, buttons);
    }
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CGUIPlexDefaultActionHandler::OnAction(int windowID, CAction action, CFileItemPtr item)
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
    // actions that require an item
    switch (actionID)
    {
      case ACTION_PLEX_PLAY_TRAILER:

        if (item->GetPlexDirectoryType() == PLEX_DIR_TYPE_MOVIE)
        {
          CPlexExtraDataLoader loader;

          if (loader.getDataForItem(item))
          {
            if (loader.getItems()->Size())
            {
              /// we dont want quality selection menu for trailers
              CFileItem trailerItem(*loader.getItems()->Get(0));
              trailerItem.SetProperty("avoidPrompts", true);
              g_application.PlayFile(trailerItem, true);
            }
          }
        }
        return true;
        break;

      case ACTION_PLEX_PQ_CLEAR:
        if (item->HasProperty("playQueueItemID"))
        {
          g_plexApplication.playQueueManager->clear();
          return true;
        }
        break;

      case ACTION_DELETE_ITEM:
        // if we are on a PQ item, remove it from PQ
        if (item->HasProperty("playQueueItemID"))
        {
          g_plexApplication.playQueueManager->removeItem(item);
          return true;
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
        g_plexApplication.playQueueManager->QueueItem(item, true);
        return true;
        break;

      case ACTION_PLEX_PQ_ADDUPTONEXT:
        g_plexApplication.playQueueManager->QueueItem(item, false);
        return true;
        break;
    }
  }

  // other actions that dont need an itemp.
  switch (actionID)
  {
    case ACTION_PLEX_NOW_PLAYING:
      m_navHelper.navigateToNowPlaying();
      return true;
      break;
  }

  return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CGUIPlexDefaultActionHandler::GetContextButtonsForAction(int actionID, CFileItemPtr item,
                                                              CContextButtons& buttons)
{
  switch (actionID)
  {
    case ACTION_PLEX_NOW_PLAYING:
      if (g_application.IsPlaying())
        buttons.Add(actionID, 13350);
      break;

    case ACTION_PLEX_PQ_CLEAR:
      if (item->HasProperty("playQueueItemID"))
        buttons.Add(actionID, 192);
      break;

    case ACTION_DELETE_ITEM:
      if (item->HasProperty("playQueueItemID"))
      {
        buttons.Add(actionID, 1210);
      }
      else
      {
        EPlexDirectoryType dirType = item->GetPlexDirectoryType();

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
      CFileItemList pqlist;
      g_plexApplication.playQueueManager->getCurrentPlayQueue(pqlist);

      if (pqlist.Size())
        buttons.Add(actionID, 52602);
      else
        buttons.Add(actionID, 52607);
      break;
    }

    case ACTION_PLEX_PQ_ADDUPTONEXT:
    {
      ePlexMediaType itemType = PlexUtils::GetMediaTypeFromItem(item);
      if (g_plexApplication.playQueueManager->getCurrentPlayQueueType() == itemType)
        buttons.Add(actionID, 52603);
      break;
    }
  }
}
