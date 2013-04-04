#include "GUIWindowPlexPreplayVideo.h"
#include "plex/PlexTypes.h"
#include "guilib/GUIWindow.h"
#include "filesystem/Directory.h"
#include "FileItem.h"

using namespace XFILE;

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
  }
  
  return ret;
}

CFileItemPtr
CGUIWindowPlexPreplayVideo::GetCurrentListItem(int offset)
{
  if (offset == 0 && m_vecItems->Size() > 0)
    return m_vecItems->Get(0);
  
  return CFileItemPtr();
}