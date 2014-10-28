#include "GUIWindowPlexPlayQueue.h"
#include "PlexApplication.h"
#include "PlexPlayQueueManager.h"
#include "music/tags/MusicInfoTag.h"
#include "GUIUserMessages.h"
#include "Application.h"
#include "GUIPlexDefaultActionHandler.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CGUIWindowPlexPlayQueue::OnSelect(int iItem)
{
  CFileItemPtr item = m_vecItems->Get(iItem);
  if (!item)
    return false;

  if (isPQ())
  {
    g_plexApplication.playQueueManager->playId(PlexUtils::GetMediaTypeFromItem(item), PlexUtils::GetItemListID(item));
  }
  else
  {
    CPlexPlayQueueOptions options;
    options.startPlaying = true;
    options.startItemKey = item->GetProperty("key").asString();
    g_plexApplication.playQueueManager->create(*m_vecItems, "", options);
  }

  return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CGUIWindowPlexPlayQueue::isPQ() const
{
  if (m_vecItems->GetPath() == "plexserver://playqueue" ||
      m_vecItems->GetPath() == "plexserver://playqueue/")
    return true;
  return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CGUIWindowPlexPlayQueue::isPlayList() const
{
  CURL url(m_vecItems->GetPath());

  if (boost::starts_with(url.GetFileName(), "playlists"))
    return true;
  else
    return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CGUIWindowPlexPlayQueue::isItemPlaying(CFileItemPtr item)
{
  int playingID = -1;

  if (PlexUtils::IsPlayingPlaylist() && g_application.CurrentFileItemPtr())
  {
    if (g_application.CurrentFileItemPtr()->HasMusicInfoTag())
      playingID = PlexUtils::GetItemListID(g_application.CurrentFileItemPtr());

    if (item->HasMusicInfoTag())
      if ((playingID > 0) && (playingID == PlexUtils::GetItemListID(item)))
        return true;
  }

  return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CGUIWindowPlexPlayQueue::GetContextButtons(int itemNumber, CContextButtons& buttons)
{
  CFileItemPtr item = m_vecItems->Get(itemNumber);
  if (!item)
    return;

  g_plexApplication.defaultActionHandler->GetContextButtons(WINDOW_PLEX_PLAY_QUEUE, item, m_vecItems, buttons);

  if (!g_application.IsPlaying())
    buttons.Add(CONTEXT_BUTTON_EDIT, 52608);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CGUIWindowPlexPlayQueue::Update(const CStdString& strDirectory, bool updateFilterPath)
{
  CStdString dirPath = strDirectory;
  if (strDirectory.empty())
    dirPath = "plexserver://playqueue/";

  CStdString plexEditMode = m_vecItems->GetProperty("PlexEditMode").asString();

  // retrieve PQ itemID from selection
  CStdString key;
  int selectedItemID = m_viewControl.GetSelectedItem();
  if (selectedItemID >= 0)
    key = m_vecItems->Get(selectedItemID)->GetProperty("key").asInteger();

  if (CGUIPlexMediaWindow::Update(dirPath, updateFilterPath))
  {
    if (m_vecItems->Size() == 0)
    {
      OnBack(ACTION_NAV_BACK);
      return true;
    }

    // restore EditMode now that we have updated the list
    m_vecItems->SetProperty("PlexEditMode", plexEditMode);

    // restore selection if any
    if (!key.IsEmpty())
    {
      // try to restore selection based on PQ itemID
      for (int i = 0; i < m_vecItems->Size(); i++)
      {
        if ((m_vecItems->Get(i)->GetProperty("key").asString() == key) &&
            (i != selectedItemID))
        {
          m_viewControl.SetSelectedItem(i);
          break;
        }
      }
    }
    else if (PlexUtils::IsPlayingPlaylist() && g_application.CurrentFileItemPtr())
      m_viewControl.SetSelectedItem(g_application.CurrentFileItemPtr()->GetPath());

    // since we call to plexserver://playqueue we need to rewrite that to the real
    // current path.
    m_startDirectory = CURL(m_vecItems->GetPath()).GetUrlWithoutOptions();
    return true;
  }
  return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CGUIWindowPlexPlayQueue::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
    case GUI_MSG_PLEX_PLAYQUEUE_UPDATED:
    {
      if (CGUIPlexDefaultActionHandler::IsPlayQueueContainer(m_vecItems))
        Update(m_vecItems->GetPath(), false);
      return true;
    }

    case GUI_MSG_WINDOW_INIT:
      break;

    case GUI_MSG_WINDOW_DEINIT:
      m_vecItems->SetProperty("PlexEditMode", "");
      break;
  }

  return CGUIPlexMediaWindow::OnMessage(message);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CGUIWindowPlexPlayQueue::OnContextButton(int itemNumber, CONTEXT_BUTTON button)
{
  CFileItemPtr item = m_vecItems->Get(itemNumber);
  if (!item)
    return false;

  g_plexApplication.defaultActionHandler->OnAction(WINDOW_PLEX_PLAY_QUEUE, button, item, m_vecItems);
  
  switch (button)
  {
    case CONTEXT_BUTTON_EDIT:
    {
      // toggle edit mode
       if (!g_application.IsPlaying())
        m_vecItems->SetProperty("PlexEditMode",
                                m_vecItems->GetProperty("PlexEditMode").asString() == "1" ? "" : "1");
      break;
    }
    default:
      break;
  }
  return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CGUIWindowPlexPlayQueue::OnAction(const CAction &action)
{

  // Long OK press, we wanna handle PQ EditMode
  if (action.GetID() == ACTION_SHOW_GUI)
  {
    OnContextButton(0, CONTEXT_BUTTON_EDIT);
    return true;
  }

  // move directly PQ/PL items without requiring editmode
  if ((action.GetID() == ACTION_MOVE_ITEM_UP) || (action.GetID() == ACTION_MOVE_ITEM_DOWN))
  {
    int iSelected = m_viewControl.GetSelectedItem();
    if (iSelected >= 0 && iSelected < (int)m_vecItems->Size())
    {
      if (isPlayList())
      {
        int afterID;
        if (action.GetID() == ACTION_MOVE_ITEM_UP)
          afterID = iSelected + 1;
        else
          afterID = iSelected - 2;
        
        if (afterID < 0)
          afterID += m_vecItems->Size();
        
        g_plexApplication.mediaServerClient->movePlayListItem(m_vecItems->Get(iSelected), m_vecItems->Get(afterID));
      }
      else
      {
        g_plexApplication.playQueueManager->moveItem(m_vecItems->Get(iSelected),
                                                   action.GetID() == ACTION_MOVE_ITEM_UP ? 1 : -1);
      }
      return true;
    }
  }

  // record selected item before processing
  int oldSelectedID = m_viewControl.GetSelectedItem();

  if (g_plexApplication.defaultActionHandler->OnAction(WINDOW_PLEX_PLAY_QUEUE, action, m_vecItems->Get(m_viewControl.GetSelectedItem()), m_vecItems))
    return true;
  
  bool ret = CGUIPlexMediaWindow::OnAction(action);

  // handle cursor move if we are in editmode for PQ
  if (!m_vecItems->GetProperty("PlexEditMode").asString().empty())
  {
    switch (action.GetID())
    {
      case ACTION_MOVE_LEFT:
      case ACTION_MOVE_RIGHT:
      case ACTION_MOVE_UP:
      case ACTION_MOVE_DOWN:
        // Move the PQ item to the new selection position, but keep selection on old one
        int newSelectedID = m_viewControl.GetSelectedItem();
        m_viewControl.SetSelectedItem(oldSelectedID);

        if (oldSelectedID != newSelectedID)
        {
          if (isPlayList())
          {
            CFileItemPtr after;
            
            if (newSelectedID > oldSelectedID)
              after = m_vecItems->Get(newSelectedID);
            else
            {
              if (newSelectedID > 0)
                after = m_vecItems->Get(newSelectedID - 1);
            }
          
            g_plexApplication.mediaServerClient->movePlayListItem(m_vecItems->Get(oldSelectedID), after);
          }
          else
          {
            g_plexApplication.playQueueManager->moveItem(m_vecItems->Get(oldSelectedID),
                                                       newSelectedID - oldSelectedID);
          }
        }
        break;
    }

  }

  return ret;
}
