//
//  GUIWindowPlexPlaylistSelection.cpp
//  Plex Home Theater
//
//  Created by Tobias Hieta on 06/08/14.
//
//

#include "GUIWindowPlexPlaylistSelection.h"
#include "interfaces/Builtins.h"

CGUIWindowPlexPlaylistSelection::CGUIWindowPlexPlaylistSelection()
  : CGUIMediaWindow(WINDOW_PLEX_PLAYLIST_SELECTION, "PlexPlaylistSelection.xml")
{
}

bool CGUIWindowPlexPlaylistSelection::OnSelect(int iItem)
{
  if (iItem < 0 || iItem >= (int)m_vecItems->Size())
    return true;
  CFileItemPtr pItem = m_vecItems->Get(iItem);
  
  CStdString action = "XBMC.ActivateWindow(PlexPlayQueue," + pItem->GetPath() + ",return)";
  
  CBuiltins::Execute(action);
  return true;
}