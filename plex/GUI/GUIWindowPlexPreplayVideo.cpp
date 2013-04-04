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
#if 0
  if (message.GetMessage() == GUI_MSG_WINDOW_INIT)
  {
    CStdString infoUrl = message.GetStringParam();
    CFileItemList list;
    
    CDirectory::GetDirectory(infoUrl, list);
  }
  
#endif
  return CGUIMediaWindow::OnMessage(message);
}