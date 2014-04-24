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
void CGUIWindowPlexPlayQueue::GetContextButtons(int itemNumber, CContextButtons &buttons)
{
  CFileItemPtr item = m_vecItems->Get(itemNumber);
  if (!item)
    return;

  if (PlexUtils::IsPlayingPlaylist())
    buttons.Add(CONTEXT_BUTTON_NOW_PLAYING, 13350);
  buttons.Add(CONTEXT_BUTTON_REMOVE_SOURCE, 1210);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CGUIWindowPlexPlayQueue::OnMessage(CGUIMessage &message)
{
  switch (message.GetMessage())
  {
    case GUI_MSG_PLEX_PLAYQUEUE_UPDATED:
    {
      Update(m_vecItems->GetPath(), false, false);
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
