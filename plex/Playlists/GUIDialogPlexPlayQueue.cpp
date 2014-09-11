#include "GUIDialogPlexPlayQueue.h"
#include "FileSystem/PlexDirectory.h"
#include "PlexApplication.h"
#include "PlexPlayQueueManager.h"
#include "music/tags/MusicInfoTag.h"
#include "Application.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
CGUIDialogPlexPlayQueue::CGUIDialogPlexPlayQueue()
  : CGUIDialogSelect(WINDOW_DIALOG_PLAYER_CONTROLS, "PlayerControls.xml")
{
  m_multiSelection = false;
  m_loadType = KEEP_IN_MEMORY;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CGUIDialogPlexPlayQueue::OnMessage(CGUIMessage& message)
{
  if ((message.GetMessage() == GUI_MSG_WINDOW_INIT) || (message.GetMessage() == GUI_MSG_PLEX_PLAYQUEUE_UPDATED))
    LoadPlayQueue();

  // make sure we refresh upon exit, as we might have edited PQ
  if ((message.GetMessage() == GUI_MSG_WINDOW_DEINIT) && m_vecList->Size())
    g_plexApplication.playQueueManager->refresh(PlexUtils::GetMediaTypeFromItem(m_vecList->Get(0)));

  return CGUIDialogSelect::OnMessage(message);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CGUIDialogPlexPlayQueue::OnAction(const CAction &action)
{
  if (action.GetID() == ACTION_SELECT_ITEM && GetFocusedControlID() == 3)
  {
    ItemSelected();
    return true;
  }

  if (action.GetID() == ACTION_DELETE_ITEM && GetFocusedControlID() == 3)
  {
    // remove current selected item from PQ
    CFileItemPtr selectedItem = m_vecList->Get(m_viewControl.GetSelectedItem());
    if (selectedItem)
    {
      // dont remove currently playing item
      if (!selectedItem->IsSelected())
       g_plexApplication.playQueueManager->removeItem(selectedItem);
    }
    return true;
  }

  return CGUIDialogSelect::OnAction(action);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CGUIDialogPlexPlayQueue::LoadPlayQueue()
{
  XFILE::CPlexDirectory dir;
  CFileItemList list;
  int currentItemId = -1;
  int currentItemIndex = m_viewControl.GetSelectedItem();
  CStdString pqUrl;
  
  if (PlexUtils::IsPlayingPlaylist())
  {
    if (g_application.CurrentFileItemPtr() && g_application.CurrentFileItemPtr()->HasMusicInfoTag())
    {
      currentItemId = PlexUtils::GetItemListID(g_application.CurrentFileItemPtr());
      pqUrl = "plexserver://playqueue/audio";
    }
    else
      pqUrl = "plexserver://playqueue/video";
  }

  // clear items & view control in case we're updating
  m_vecList->Clear();
  m_viewControl.Clear();
  
  if (dir.GetDirectory(pqUrl, list))
  {
    int playingItemIdx = 0;
    for (int i = 0; i < list.Size(); i++)
    {
      CFileItemPtr item = list.Get(i);
      if (item)
      {
        if (item->HasMusicInfoTag() && PlexUtils::GetItemListID(item) == currentItemId)
        {
          playingItemIdx = i;
          item->Select(true);
        }
        else
        {
          item->Select(false);
        }
        Add(item.get());
      }
    }

    // sets again viewControl items in case we're updating
    m_viewControl.SetItems(*m_vecList);

    // if we dont have any current selection, default to playing item
    if (currentItemIndex == -1)
      currentItemIndex = playingItemIdx;

    m_viewControl.SetSelectedItem(currentItemIndex);
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CGUIDialogPlexPlayQueue::ItemSelected()
{
  int iSelected = m_viewControl.GetSelectedItem();
  if (iSelected >= 0 && iSelected < (int)m_vecList->Size())
  {
    CFileItemPtr item(m_vecList->Get(iSelected));
    if (item && item->HasMusicInfoTag())
    {
      // unselect old item
      for (int i = 0 ; i < m_vecList->Size() ; i++)
        m_vecList->Get(i)->Select(false);

      // select the new one
      item->Select(true);
      g_plexApplication.playQueueManager->playId(PlexUtils::GetMediaTypeFromItem(item), PlexUtils::GetItemListID(item));
    }
  }
}
