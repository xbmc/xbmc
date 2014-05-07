#include "GUIWindowPlexPlayQueue.h"
#include "PlexApplication.h"
#include "PlexPlayQueueManager.h"
#include "music/tags/MusicInfoTag.h"
#include "GUIUserMessages.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CGUIWindowPlexPlayQueue::OnSelect(int iItem)
{
  CFileItemPtr item = m_vecItems->Get(iItem);
  if (!item)
    return false;

  if (item->HasMusicInfoTag())
  {
    g_plexApplication.playQueueManager->playCurrentId(item->GetMusicInfoTag()->GetDatabaseId());
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

  if (PlexUtils::IsPlayingPlaylist())
    buttons.Add(CONTEXT_BUTTON_NOW_PLAYING, 13350);
  buttons.Add(CONTEXT_BUTTON_REMOVE_SOURCE, 1210);
  buttons.Add(CONTEXT_BUTTON_CLEAR, 192);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CGUIWindowPlexPlayQueue::Update(const CStdString& strDirectory, bool updateFilterPath)
{
  if (CGUIPlexMediaWindow::Update(strDirectory, updateFilterPath))
  {
    if (PlexUtils::IsPlayingPlaylist() && g_application.CurrentFileItemPtr())
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
      Update("plexserver://playqueue/", false);
      return true;
    }
  }

  return CGUIPlexMediaWindow::OnMessage(message);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CGUIWindowPlexPlayQueue::OnContextButton(int itemNumber, CONTEXT_BUTTON button)
{
  CFileItemPtr item = m_vecItems->Get(itemNumber);
  if (!item)
    return false;

  switch (button)
  {
    case CONTEXT_BUTTON_NOW_PLAYING:
    {
      CPlexNavigationHelper::navigateToNowPlaying();
      break;
    }
    case CONTEXT_BUTTON_REMOVE_SOURCE:
    {
      g_plexApplication.playQueueManager->removeItem(item);
      break;
    }
    case CONTEXT_BUTTON_CLEAR:
    {
      g_plexApplication.playQueueManager->clear();
      OnBack(ACTION_NAV_BACK);
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
  if (action.GetID() == ACTION_DELETE_ITEM)
  {
    int i = m_viewControl.GetSelectedItem();
    CFileItemPtr item = m_vecItems->Get(i);
    if (item)
      g_plexApplication.playQueueManager->removeItem(item);
    return true;
  }

  return CGUIPlexMediaWindow::OnAction(action);
}
