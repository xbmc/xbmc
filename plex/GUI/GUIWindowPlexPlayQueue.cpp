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
      Update(m_vecItems->GetPath(), false);
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
      if (g_plexApplication.playQueueManager->current())
        g_plexApplication.playQueueManager->current()->removeItem(item);
      break;
    }
    default:
      break;
  }
  return true;
}
