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
