#include "stdafx.h"
#include "GUIWindowVideoPlayList.h"
#include "PlayListFactory.h"
#include "Util.h"
#include "PlayListM3U.h"
#include "Application.h"
#include "PlayListPlayer.h"
#include "GUIThumbnailPanel.h"
#include "GUIListControl.h"
#include "GUIDialogContextMenu.h"

#define CONTROL_BTNVIEWASICONS     2 
#define CONTROL_BTNSORTBY          3
#define CONTROL_BTNSORTASC         4
#define CONTROL_LIST              50
#define CONTROL_LABELFILES        12

#define CONTROL_BTNSHUFFLE    20
#define CONTROL_BTNSAVE      21
#define CONTROL_BTNCLEAR     22

#define CONTROL_BTNPLAY      23
#define CONTROL_BTNNEXT      24
#define CONTROL_BTNPREVIOUS    25

#define CONTROL_BTNREPEAT     26
#define CONTROL_BTNREPEATONE   27
#define CONTROL_BTNRANDOMIZE  28

CGUIWindowVideoPlaylist::CGUIWindowVideoPlaylist()
: CGUIWindowVideoBase(WINDOW_VIDEO_PLAYLIST, "MyVideoPlaylist.xml")
{
  iPos = -1;
}

CGUIWindowVideoPlaylist::~CGUIWindowVideoPlaylist()
{
}

bool CGUIWindowVideoPlaylist::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_DEINIT:
    {
      OutputDebugString("deinit guiwindowvideoplaylist!\n");
      m_iSelectedItem = m_viewControl.GetSelectedItem();
      m_iLastControl = GetFocusedControl();

      iPos = -1;
    }
    break;

  case GUI_MSG_WINDOW_INIT:
    {
      OutputDebugString("init guiwindowvideoplaylist!\n");
      CGUIWindow::OnMessage(message);

      Update("");

      if (m_viewControl.HasControl(m_iLastControl) && m_vecItems.Size() > 0)
      {
        m_viewControl.SetFocused();
      }

      if (m_iSelectedItem > -1)
      {
        m_viewControl.SetSelectedItem(m_iSelectedItem);
      }

      if (g_playlistPlayer.Repeated(PLAYLIST_VIDEO))
      {
        CONTROL_SELECT(CONTROL_BTNREPEAT);
      }

      if (g_playlistPlayer.RepeatedOne(PLAYLIST_VIDEO))
      {
        CONTROL_SELECT(CONTROL_BTNREPEATONE);
      }

      if (g_playlistPlayer.ShuffledPlay(PLAYLIST_VIDEO))
      {
        CONTROL_SELECT(CONTROL_BTNRANDOMIZE);
      }

      if (g_playlistPlayer.GetPlaylist(PLAYLIST_VIDEO).IsShuffled())
      {
        CONTROL_SELECT(CONTROL_BTNSHUFFLE);
      }
      else
      {
        CONTROL_DESELECT(CONTROL_BTNSHUFFLE);
      }

      if (g_application.IsPlayingVideo() && g_playlistPlayer.GetCurrentPlaylist() == PLAYLIST_VIDEO)
      {
        int iSong = g_playlistPlayer.GetCurrentSong();
        if (iSong >= 0 && iSong <= (int)m_vecItems.Size())
        {
          m_viewControl.SetSelectedItem(iSong);
        }
      }
      return true;
    }
    break;

  case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();
      if (iControl == CONTROL_BTNRANDOMIZE)
      {
        g_stSettings.m_bMyVideoPlaylistShuffle = !g_playlistPlayer.ShuffledPlay(PLAYLIST_VIDEO);
        g_settings.Save();
        g_playlistPlayer.ShufflePlay(PLAYLIST_VIDEO, g_stSettings.m_bMyVideoPlaylistShuffle);
      }
      else if (iControl == CONTROL_BTNSHUFFLE)
      {
        ShufflePlayList();
      }
      else if (iControl == CONTROL_BTNSAVE)
      {
        SavePlayList();
      }
      else if (iControl == CONTROL_BTNCLEAR)
      {
        ClearPlayList();
      }
      else if (iControl == CONTROL_BTNPLAY)
      {
        g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_VIDEO);
        g_playlistPlayer.Reset();
        g_playlistPlayer.Play(m_viewControl.GetSelectedItem());
        UpdateButtons();
      }
      else if (iControl == CONTROL_BTNNEXT)
      {
        g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_VIDEO);
        g_playlistPlayer.PlayNext();
      }
      else if (iControl == CONTROL_BTNPREVIOUS)
      {
        g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_VIDEO);
        g_playlistPlayer.PlayPrevious();
      }
      else if (iControl == CONTROL_BTNREPEAT)
      {
        g_stSettings.m_bMyVideoPlaylistRepeat = !g_stSettings.m_bMyVideoPlaylistRepeat;
        g_settings.Save();
        g_playlistPlayer.Repeat(PLAYLIST_VIDEO, g_stSettings.m_bMyVideoPlaylistRepeat);
      }
      else if (iControl == CONTROL_BTNREPEATONE)
      {
        static bool bRepeatOne = false;
        bRepeatOne = !bRepeatOne;
        g_playlistPlayer.RepeatOne(PLAYLIST_VIDEO, bRepeatOne);
      }
      else if (m_viewControl.HasControl(iControl))  // list/thumb control
      {
        int iAction = message.GetParam1();
        int iItem = m_viewControl.GetSelectedItem();
        if (iAction == ACTION_DELETE_ITEM || iAction == ACTION_MOUSE_MIDDLE_CLICK)
        {
          RemovePlayListItem(iItem);
        }
      }
    }
    break;
  }
  return CGUIWindowVideoBase::OnMessage(message);
}

bool CGUIWindowVideoPlaylist::OnAction(const CAction &action)
{
  if (action.wID == ACTION_PARENT_DIR)
  {
    // Playlist has no parent dirs
    return true;
  }
  if (action.wID == ACTION_SHOW_PLAYLIST)
  {
    m_gWindowManager.PreviousWindow();
    return true;
  }
  if ((action.wID == ACTION_MOVE_ITEM_UP) || (action.wID == ACTION_MOVE_ITEM_DOWN))
  {
    int iItem = -1;
    int iFocusedControl = GetFocusedControl();
    if (m_viewControl.HasControl(iFocusedControl))
      iItem = m_viewControl.GetSelectedItem();
    OnMove(iItem, action.wID);
    return true;
  }
  return CGUIWindowVideoBase::OnAction(action);
}

bool CGUIWindowVideoPlaylist::MoveCurrentPlayListItem(int iItem, int iAction, bool bUpdate /* = true */)
{
  int iSelected = iItem;
  int iNew = iSelected;
  if (iAction == ACTION_MOVE_ITEM_UP)
    iNew--;
  else
    iNew++;

  // is the currently playing item affected?
  bool bFixCurrentSong = false;
  if ((g_playlistPlayer.GetCurrentPlaylist() == PLAYLIST_VIDEO) && (g_application.IsPlayingVideo()) &&
    ((g_playlistPlayer.GetCurrentSong() == iSelected) || (g_playlistPlayer.GetCurrentSong() == iNew)))
    bFixCurrentSong = true;

  CPlayList& playlist = g_playlistPlayer.GetPlaylist(PLAYLIST_VIDEO);
  if (playlist.Swap(iSelected, iNew))
  {
    // Correct the current playing song in playlistplayer
    if (bFixCurrentSong)
    {
      int iCurrentSong = g_playlistPlayer.GetCurrentSong();
      if (iSelected == iCurrentSong)
        iCurrentSong = iNew;
      else if (iNew == iCurrentSong)
        iCurrentSong = iSelected;
      g_playlistPlayer.SetCurrentSong(iCurrentSong);
      m_vecItems[iCurrentSong]->Select(true);
    }

    if (bUpdate)
      Update(m_vecItems.m_strPath);
    return true;
  }

  return false;
}


void CGUIWindowVideoPlaylist::ClearPlayList()
{
  ClearFileItems();
  g_playlistPlayer.GetPlaylist(PLAYLIST_VIDEO).Clear();
  if (g_playlistPlayer.GetCurrentPlaylist() == PLAYLIST_VIDEO)
  {
    g_playlistPlayer.Reset();
    g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_NONE);
  }
  UpdateListControl();
  UpdateButtons();
  SET_CONTROL_FOCUS(CONTROL_BTNVIEWASICONS, 0);
}

void CGUIWindowVideoPlaylist::UpdateListControl()
{
  for (int i = 0; i < (int)m_vecItems.Size(); i++)
  {
    CFileItem* pItem = m_vecItems[i];

    // Format label for listcontrol
    // and set thumb/icon for item
    OnFileItemFormatLabel(pItem);
  }

  SortItems(m_vecItems);

  m_viewControl.SetItems(m_vecItems);
}

void CGUIWindowVideoPlaylist::OnFileItemFormatLabel(CFileItem* pItem)
{
  pItem->SetThumb();
  pItem->FillInDefaultIcon();
  // Remove extension from title if it exists
  pItem->SetLabel(CUtil::GetTitleFromPath(pItem->GetLabel()));
}

void CGUIWindowVideoPlaylist::UpdateButtons()
{
  // Update playlist buttons
  if (m_vecItems.Size() )
  {
    CONTROL_ENABLE(CONTROL_BTNCLEAR);
    CONTROL_ENABLE(CONTROL_BTNSAVE);
    CONTROL_ENABLE(CONTROL_BTNPLAY);
    CONTROL_ENABLE(CONTROL_BTNSHUFFLE);
    CONTROL_ENABLE(CONTROL_BTNRANDOMIZE);
    CONTROL_ENABLE(CONTROL_BTNREPEAT);
    CONTROL_ENABLE(CONTROL_BTNREPEATONE);

    if (g_application.IsPlayingVideo() && g_playlistPlayer.GetCurrentPlaylist() == PLAYLIST_VIDEO)
    {
      CONTROL_ENABLE(CONTROL_BTNNEXT);
      CONTROL_ENABLE(CONTROL_BTNPREVIOUS);
    }
    else
    {
      CONTROL_DISABLE(CONTROL_BTNNEXT);
      CONTROL_DISABLE(CONTROL_BTNPREVIOUS);
    }
  }
  else
  {
    CONTROL_DISABLE(CONTROL_BTNCLEAR);
    CONTROL_DISABLE(CONTROL_BTNSAVE);
    CONTROL_DISABLE(CONTROL_BTNSHUFFLE);
    CONTROL_DISABLE(CONTROL_BTNRANDOMIZE);
    CONTROL_DISABLE(CONTROL_BTNPLAY);
    CONTROL_DISABLE(CONTROL_BTNNEXT);
    CONTROL_DISABLE(CONTROL_BTNPREVIOUS);
    CONTROL_DISABLE(CONTROL_BTNREPEAT);
    CONTROL_DISABLE(CONTROL_BTNREPEATONE);
  }

  CGUIMediaWindow::UpdateButtons();

  // Update Repeat/Repeat One button
  if (g_playlistPlayer.Repeated(PLAYLIST_VIDEO))
  {
    CONTROL_SELECT(CONTROL_BTNREPEAT);
  }

  if (g_playlistPlayer.RepeatedOne(PLAYLIST_VIDEO))
  {
    CONTROL_SELECT(CONTROL_BTNREPEATONE);
  }

}


bool CGUIWindowVideoPlaylist::GetDirectory(const CStdString &strDirectory, CFileItemList &items)
{
  if (items.Size())
  {
    items.Clear(); // will clean up everything
  }

  CPlayList& playlist = g_playlistPlayer.GetPlaylist(PLAYLIST_VIDEO);
  /* copy playlist from general playlist*/
  int iCurrentSong = -1;
  if (g_playlistPlayer.GetCurrentPlaylist() == PLAYLIST_VIDEO)
    iCurrentSong = g_playlistPlayer.GetCurrentSong();

  CStdString strPath, strFileName;
  for (int i = 0; i < playlist.size(); ++i)
  {
    const CPlayList::CPlayListItem& item = playlist[i];

    CStdString strFileName = item.GetFileName();
    //CStdString strPath;
    //CUtil::GetDirectory( strFileName, strPath);
    //m_Pathes.insert(strPath);

    CFileItem *pItem = new CFileItem(item.GetDescription());
    pItem->m_strPath = strFileName;
    pItem->m_bIsFolder = false;
    pItem->m_bIsShareOrDrive = false;

    if (item.GetDuration())
    {
      int nDuration = item.GetDuration();
      if (nDuration > 0)
      {
        CStdString str;
        CUtil::SecondsToHMSString(nDuration, str);
        pItem->SetLabel2(str);
      }
      else
        pItem->SetLabel2("");
    }
    items.Add(pItem);
  }

  return true;
}

bool CGUIWindowVideoPlaylist::Update(const CStdString &strDirectory)
{
  // get selected item
  int iItem = m_viewControl.GetSelectedItem();
  CStdString strSelectedItem = "";
  if (iItem >= 0 && iItem < (int)m_vecItems.Size())
  {
    CFileItem* pItem = m_vecItems[iItem];
    if (!pItem->IsParentFolder())
    {
      GetDirectoryHistoryString(pItem, strSelectedItem);
    }
  }

  ClearFileItems();

  m_history.SetSelectedItem(strSelectedItem, m_vecItems.m_strPath);
  m_vecItems.m_strPath = strDirectory;

  GetDirectory(m_vecItems.m_strPath, m_vecItems);

  m_guiState.reset(CGUIViewState::GetViewState(GetID(), m_vecItems));
  UpdateListControl();
  UpdateButtons();

  strSelectedItem = m_history.GetSelectedItem(m_vecItems.m_strPath);

  int iCurrentSong = -1;
  // Search current playlist item
  CStdString strCurrentDirectory = m_vecItems.m_strPath;
  if (CUtil::HasSlashAtEnd(strCurrentDirectory))
    strCurrentDirectory.Delete(strCurrentDirectory.size() - 1);
  // Is this window responsible for the current playlist?
  if (m_guiState.get() && g_playlistPlayer.GetCurrentPlaylist()==m_guiState->GetPlaylist() && strCurrentDirectory==m_guiState->GetPlaylistDirectory())
  {
    iCurrentSong = g_playlistPlayer.GetCurrentSong();
  }

  bool bSelectedFound = false;
  for (int i = 0; i < (int)m_vecItems.Size(); ++i)
  {
    CFileItem* pItem = m_vecItems[i];

    // Update selected item
    if (!bSelectedFound)
    {
      CStdString strHistory;
      GetDirectoryHistoryString(pItem, strHistory);
      if (strHistory == strSelectedItem)
      {
        m_viewControl.SetSelectedItem(i);
        bSelectedFound = true;
      }
    }

    // synchronize playlist with current directory
    if (i == iCurrentSong)
    {
      pItem->Select(true);
    }

  }

  return true;
}

void CGUIWindowVideoPlaylist::OnPlayMedia(int iItem)
{
  if ( iItem < 0 || iItem >= (int)m_vecItems.Size() ) return;
  CFileItem* pItem = m_vecItems[iItem];
  CStdString strPath = pItem->m_strPath;
  g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_VIDEO);
  g_playlistPlayer.Reset();
  g_playlistPlayer.Play( iItem );
}

void CGUIWindowVideoPlaylist::RemovePlayListItem(int iItem)
{
  // The current playing song can't be removed
  if (g_playlistPlayer.GetCurrentPlaylist() == PLAYLIST_VIDEO && g_application.IsPlayingVideo()
      && g_playlistPlayer.GetCurrentSong() == iItem)
    return ;

  g_playlistPlayer.GetPlaylist(PLAYLIST_VIDEO).Remove(iItem);

  // Correct the current playing song in playlistplayer
  if (g_playlistPlayer.GetCurrentPlaylist() == PLAYLIST_VIDEO && g_application.IsPlayingVideo())
  {
    int iCurrentSong = g_playlistPlayer.GetCurrentSong();
    if (iItem <= iCurrentSong)
    {
      iCurrentSong--;
      g_playlistPlayer.SetCurrentSong(iCurrentSong);
      m_vecItems[iCurrentSong]->Select(true);
    }
  }

  Update(m_vecItems.m_strPath);

  if (m_vecItems.Size() <= 0)
  {
    SET_CONTROL_FOCUS(CONTROL_BTNVIEWASICONS, 0);
  }
  else
  {
    m_viewControl.SetSelectedItem(iItem - 1);
  }
}

void CGUIWindowVideoPlaylist::ShufflePlayList()
{
  int iPlaylist = PLAYLIST_VIDEO;
  ClearFileItems();
  CPlayList& playlist = g_playlistPlayer.GetPlaylist(iPlaylist);

  CStdString strFileName;
  if (g_application.IsPlayingVideo() && g_playlistPlayer.GetCurrentPlaylist() == iPlaylist)
  {
    const CPlayList::CPlayListItem& item = playlist[g_playlistPlayer.GetCurrentSong()];
    strFileName = item.GetFileName();
  }

  // shuffle or unshuffle?
  playlist.IsShuffled() ? playlist.UnShuffle() : playlist.Shuffle();
  if (g_playlistPlayer.GetCurrentPlaylist() == iPlaylist)
    g_playlistPlayer.Reset();

  if (!strFileName.IsEmpty())
  {
    for (int i = 0; i < playlist.size(); i++)
    {
      const CPlayList::CPlayListItem& item = playlist[i];
      if (item.GetFileName() == strFileName)
        g_playlistPlayer.SetCurrentSong(i);
    }
  }

  Update(m_vecItems.m_strPath);
}

/// \brief Save current playlist to playlist folder
void CGUIWindowVideoPlaylist::SavePlayList()
{
  CStdString strNewFileName;
  if (CGUIDialogKeyboard::ShowAndGetInput(strNewFileName, (CStdStringW)g_localizeStrings.Get(16012), false))
  {
    // need 2 rename it
    CStdString strFolder, strPath;
    CUtil::AddFileToFolder(g_stSettings.m_szPlaylistsDirectory, "video", strFolder);
    CUtil::RemoveIllegalChars( strNewFileName );
    strNewFileName += ".m3u";
    CUtil::AddFileToFolder(strFolder, strNewFileName, strPath);

    CPlayListM3U playlist;
    for (int i = 0; i < m_vecItems.Size(); ++i)
    {
      CFileItem* pItem = m_vecItems[i];
      CPlayList::CPlayListItem newItem;
      newItem.SetFileName(pItem->m_strPath);
      newItem.SetDescription(pItem->GetLabel());
      if (pItem->m_musicInfoTag.Loaded())
        newItem.SetDuration(pItem->m_musicInfoTag.GetDuration());
      else
        newItem.SetDuration(0);
      playlist.Add(newItem);
    }
    CLog::Log(LOGDEBUG, "Saving video playlist: [%s]", strPath.c_str());
    playlist.Save(strPath);
  }
}

void CGUIWindowVideoPlaylist::OnPopupMenu(int iItem)
{
  if ( iItem < 0 || iItem >= m_vecItems.Size() ) return ;
  // calculate our position
  int iPosX = 200;
  int iPosY = 100;
  CGUIListControl *pList = (CGUIListControl *)GetControl(CONTROL_LIST);
  if (pList)
  {
    iPosX = pList->GetXPosition() + pList->GetWidth() / 2;
    iPosY = pList->GetYPosition() + pList->GetHeight() / 2;
  }
  // mark the item
  m_vecItems[iItem]->Select(true);
  // popup the context menu
  CGUIDialogContextMenu *pMenu = (CGUIDialogContextMenu *)m_gWindowManager.GetWindow(WINDOW_DIALOG_CONTEXT_MENU);
  if (!pMenu) return ;
  // load our menu
  pMenu->Initialize();

  // is this playlist playing?
  bool bIsPlaying = false;
  bool bItemIsPlaying = false;
  if ((g_playlistPlayer.GetCurrentPlaylist() == PLAYLIST_VIDEO) && (g_application.IsPlayingVideo()))
  {
    bIsPlaying = true;
    int i = g_playlistPlayer.GetCurrentSong();
    if (iItem == i) bItemIsPlaying = true;
  }
  // add the buttons
  int btn_Move = 0;   // move item
  int btn_MoveTo = 0; // move item here
  int btn_Cancel = 0; // cancel move
  int btn_MoveUp = 0; // move up
  int btn_MoveDn = 0; // move down
  int btn_Delete = 0; // delete

  if (iPos < 0)
  {
    btn_Move = pMenu->AddButton(13251);       // move item
    btn_MoveUp = pMenu->AddButton(13332);     // move up
    if (iItem == 0)
      pMenu->EnableButton(btn_MoveUp, false); // disable if top item
    btn_MoveDn = pMenu->AddButton(13333);     // move down
    if (iItem == (m_vecItems.Size()-1))
      pMenu->EnableButton(btn_MoveDn, false); // disable if bottom item
    btn_Delete = pMenu->AddButton(15015);     // delete
    if (bItemIsPlaying)
      pMenu->EnableButton(btn_Delete, false); // disable if current item
  }
  // after selecting "move item" only two choices
  else
  {
    btn_MoveTo = pMenu->AddButton(13252);         // move item here
    if (iItem == iPos)
      pMenu->EnableButton(btn_MoveTo, false);     // disable the button if its the same position or current item
    btn_Cancel = pMenu->AddButton(13253);         // cancel move
  }
  int btn_Return = pMenu->AddButton(12011);     // return to my videos

  // position it correctly
  pMenu->SetPosition(iPosX - pMenu->GetWidth() / 2, iPosY - pMenu->GetHeight() / 2);
  pMenu->DoModal(GetID());

  int btnid = pMenu->GetButton();
  if (btnid > 0)
  {
    // move item
    if (btnid == btn_Move)
    {
      iPos = iItem;
    }
    // move item here
    else if (btnid == btn_MoveTo && iPos >= 0)
    {
      MoveItem(iPos, iItem);
      iPos = -1;
    }
    // cancel move
    else if (btnid == btn_Cancel)
    {
      iPos = -1;
    }
    // move up
    else if (btnid == btn_MoveUp)
    {
      OnMove(iItem, ACTION_MOVE_ITEM_UP);
    }
    // move down
    else if (btnid == btn_MoveDn)
    {
      OnMove(iItem, ACTION_MOVE_ITEM_DOWN);
    }
    // delete
    else if (btnid == btn_Delete)
    {
      RemovePlayListItem(iItem);
      return;
    }
    // return to my videos
    else if (btnid == btn_Return)
    {
      m_gWindowManager.ActivateWindow(WINDOW_VIDEOS);
      return;
    }
  }
  m_vecItems[iItem]->Select(false);

  // mark the currently playing item
  if (bIsPlaying)
  {
    int i = g_playlistPlayer.GetCurrentSong();
    m_vecItems[i]->Select(true);
  }
}

void CGUIWindowVideoPlaylist::OnMove(int iItem, int iAction)
{
  if (iItem < 0 || iItem >= m_vecItems.Size()) return;
  MoveCurrentPlayListItem(iItem, iAction);
}

void CGUIWindowVideoPlaylist::MoveItem(int iStart, int iDest)
{
  if (iStart < 0 || iStart >= m_vecItems.Size()) return;
  if (iDest < 0 || iDest >= m_vecItems.Size()) return;

  // default to move up
  int iAction = ACTION_MOVE_ITEM_UP;
  int iDirection = -1;
  // are we moving down?
  if (iStart < iDest)
  {
    iAction = ACTION_MOVE_ITEM_DOWN;
    iDirection = 1;
  }

  // keep swapping until you get to the destination or you
  // hit the currently playing song
  int i = iStart;
  while (i != iDest)
  {
    // try to swap adjacent items
    if (MoveCurrentPlayListItem(i, iAction, false))
      i = i + (1 * iDirection);
    // we hit currently playing song, so abort
    else
      break;
  }
  Update(m_vecItems.m_strPath);
}