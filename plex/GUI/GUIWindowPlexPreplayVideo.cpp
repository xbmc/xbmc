
#include "GUIDialogPlexAudioSubtitlePicker.h"

#include "GUIWindowPlexPreplayVideo.h"
#include "plex/PlexTypes.h"
#include "guilib/GUIWindow.h"
#include "filesystem/Directory.h"

#include "FileItem.h"
#include "guilib/GUILabelControl.h"
#include "ApplicationMessenger.h"
#include "PlexApplication.h"
#include "PlexContentPlayerMixin.h"

CGUIWindowPlexPreplayVideo::CGUIWindowPlexPreplayVideo(void)
 : CGUIMediaWindow(WINDOW_PLEX_PREPLAY_VIDEO, "PlexPreplayVideo.xml")
{
}

CGUIWindowPlexPreplayVideo::~CGUIWindowPlexPreplayVideo()
{
}

bool
CGUIWindowPlexPreplayVideo::OnMessage(CGUIMessage &message)
{
  bool ret = CGUIMediaWindow::OnMessage(message);

  if (message.GetMessage() == GUI_MSG_WINDOW_INIT)
  {
    if (m_vecItems->GetContent() == "movies")
      m_vecItems->SetContent("movie");
    if (m_vecItems->GetContent() == "episodes")
      m_vecItems->SetContent("episode");
    
    g_plexApplication.m_preplayItem = m_vecItems->Get(0);
  }
  else if (message.GetMessage() == GUI_MSG_WINDOW_DEINIT)
  {
    g_plexApplication.m_preplayItem.reset();
  }

  return ret;
}

bool
CGUIWindowPlexPreplayVideo::OnAction(const CAction &action)
{
  if (action.GetID() == ACTION_PLAYER_PLAY)
  {
    PlexContentPlayerMixin::PlayPlexItem(g_plexApplication.m_preplayItem);
    return true;
  }
  else if (action.GetID() == ACTION_TOGGLE_WATCHED)
  {
    if (m_vecItems->Get(0)->GetOverlayImageID() == CGUIListItem::ICON_OVERLAY_WATCHED)
      m_vecItems->Get(0)->MarkAsUnWatched();
    else
      m_vecItems->Get(0)->MarkAsWatched();
  }
  
  return CGUIMediaWindow::OnAction(action);
}

CFileItemPtr
CGUIWindowPlexPreplayVideo::GetCurrentListItem(int offset)
{
  if (offset == 0 && m_vecItems->Size() > 0)
    return m_vecItems->Get(0);
  
  return CFileItemPtr();
}
