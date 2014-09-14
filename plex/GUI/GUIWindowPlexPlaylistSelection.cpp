//
//  GUIWindowPlexPlaylistSelection.cpp
//  Plex Home Theater
//
//  Created by Tobias Hieta on 06/08/14.
//
//

#include "GUIWindowPlexPlaylistSelection.h"
#include "interfaces/Builtins.h"
#include "PlexApplication.h"
#include "GUIPlexDefaultActionHandler.h"
#include "GUIDialogPlexError.h"
#include "LocalizeStrings.h"

CGUIWindowPlexPlaylistSelection::CGUIWindowPlexPlaylistSelection()
  : CGUIMediaWindow(WINDOW_PLEX_PLAYLIST_SELECTION, "PlexPlaylistSelection.xml")
{
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CGUIWindowPlexPlaylistSelection::OnSelect(int iItem)
{
  if (iItem < 0 || iItem >= (int)m_vecItems->Size())
    return true;
  CFileItemPtr pItem = m_vecItems->Get(iItem);
  
  if (pItem->GetProperty("leafCount").asInteger())
  {
    CStdString action = "XBMC.ActivateWindow(PlexPlayQueue," + pItem->GetPath() + ",return)";
    
    CBuiltins::Execute(action);
  }
  else
    CGUIDialogPlexError::ShowError(g_localizeStrings.Get(52610), g_localizeStrings.Get(52611), "", "");
    
  return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CGUIWindowPlexPlaylistSelection::GetContextButtons(int itemNumber, CContextButtons &buttons)
{
  CFileItemPtr item = m_vecItems->Get(itemNumber);
  if (!item)
    return;
  
  g_plexApplication.defaultActionHandler->GetContextButtons(WINDOW_PLEX_PLAYLIST_SELECTION, item, m_vecItems, buttons);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CGUIWindowPlexPlaylistSelection::OnContextButton(int itemNumber, CONTEXT_BUTTON button)
{
  CFileItemPtr item = m_vecItems->Get(itemNumber);
  if (!item)
    return false;
  
  return g_plexApplication.defaultActionHandler->OnAction(WINDOW_PLEX_PLAYLIST_SELECTION, button, item, m_vecItems);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CGUIWindowPlexPlaylistSelection::OnAction(const CAction &action)
{
  if (g_plexApplication.defaultActionHandler->OnAction(WINDOW_PLEX_PLAY_QUEUE, action, m_vecItems->Get(m_viewControl.GetSelectedItem()), m_vecItems))
    return true;
  
  return CGUIMediaWindow::OnAction(action);
}
