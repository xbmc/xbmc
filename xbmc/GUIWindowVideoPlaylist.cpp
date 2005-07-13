#include "stdafx.h"
#include "GUIWindowVideoPlayList.h"
#include "PlayListFactory.h"
#include "Util.h"
#include "PlayListM3U.h"
#include "Application.h"
#include "PlayListPlayer.h"
#include "GUIThumbnailPanel.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define CONTROL_BTNVIEWASICONS  2
#define CONTROL_BTNSORTBY     3
#define CONTROL_BTNSORTASC    4

#define CONTROL_BTNSHUFFLE    20
#define CONTROL_BTNSAVE      21
#define CONTROL_BTNCLEAR     22

#define CONTROL_BTNPLAY      23
#define CONTROL_BTNNEXT      24
#define CONTROL_BTNPREVIOUS    25

#define CONTROL_BTNREPEAT     26
#define CONTROL_BTNREPEATONE   27

#define CONTROL_LABELFILES        12

#define CONTROL_LIST       50
#define CONTROL_THUMBS      51

static int m_nTempPlayListWindow = 0;
static CStdString m_strTempPlayListDirectory = "";


CGUIWindowVideoPlaylist::CGUIWindowVideoPlaylist()
{
  m_Directory.m_strPath = "";
}

CGUIWindowVideoPlaylist::~CGUIWindowVideoPlaylist()
{
}

bool CGUIWindowVideoPlaylist::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_DVDDRIVE_EJECTED_CD:
  case GUI_MSG_DVDDRIVE_CHANGED_CD:
    return true;  // nothing to do
    break;
  case GUI_MSG_PLAYBACK_ENDED:
  case GUI_MSG_PLAYBACK_STOPPED:
  case GUI_MSG_PLAYLISTPLAYER_STOPPED:
    {
      for (int i = 0; i < (int)m_vecItems.Size(); ++i)
      {
        CFileItem* pItem = m_vecItems[i];
        if (pItem && pItem->IsSelected())
        {
          pItem->Select(false);
          break;
        }
      }

      UpdateButtons();
    }
    break;

  case GUI_MSG_PLAYLIST_CHANGED:
    {
      // global playlist changed outside playlist window
      Update("");

      if (m_viewControl.HasControl(m_iLastControl) && m_vecItems.Size() <= 0)
      {
        m_viewControl.SetFocused();
      }

    }
    break;

  case GUI_MSG_WINDOW_DEINIT:
    {
      OutputDebugString("deinit guiwindowvideoplaylist!\n");
      m_iItemSelected = m_viewControl.GetSelectedItem();
      m_iLastControl = GetFocusedControl();
    }
    break;

  case GUI_MSG_WINDOW_INIT:
    {
      OutputDebugString("init guiwindowvideoplaylist!\n");
      CGUIWindow::OnMessage(message);

      LoadViewMode();

      Update("");

      if (m_viewControl.HasControl(m_iLastControl) && m_vecItems.Size() > 0)
      {
        m_viewControl.SetFocused();
      }

      if (m_iItemSelected > -1)
      {
        m_viewControl.SetSelectedItem(m_iItemSelected);
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
        CONTROL_SELECT(CONTROL_BTNSHUFFLE);
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
      if (iControl == CONTROL_BTNSHUFFLE)
      {
        //ShufflePlayList();
        g_stSettings.m_bMyVideoPlaylistShuffle = !g_playlistPlayer.ShuffledPlay(PLAYLIST_VIDEO);
        g_settings.Save();
        g_playlistPlayer.ShufflePlay(PLAYLIST_VIDEO, g_stSettings.m_bMyVideoPlaylistShuffle);
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
        int iItem = m_viewControl.GetSelectedItem();
        int iAction = message.GetParam1();
        if (iItem < 0)
          break;

        if (iAction == ACTION_DELETE_ITEM)
        {
          RemovePlayListItem(iItem);
          return true;
        }
        else if (iAction == ACTION_SELECT_ITEM)
        {
          OnClick(iItem);
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
    OutputDebugString("leave videplaylist!\n");
    m_gWindowManager.PreviousWindow();
    return true;
  }
  if (action.wID == ACTION_MOVE_ITEM_UP)
  {
    MoveCurrentPlayListItem(ACTION_MOVE_ITEM_UP);
    return true;
  }
  if (action.wID == ACTION_MOVE_ITEM_DOWN)
  {
    MoveCurrentPlayListItem(ACTION_MOVE_ITEM_DOWN);
    return true;
  }

  return CGUIWindowVideoBase::OnAction(action);
}

void CGUIWindowVideoPlaylist::MoveCurrentPlayListItem(int iAction)
{
  int iFocusedControl = GetFocusedControl();
  if (m_viewControl.HasControl(iFocusedControl))
  {
    int iSelected = m_viewControl.GetSelectedItem();
    int iNew = iSelected;
    if (iAction == ACTION_MOVE_ITEM_UP)
    {
      iNew--;
    }
    else
    {
      iNew++;
    }
    // The current playing or target song can't be moved
    if (
      (g_playlistPlayer.GetCurrentPlaylist() == PLAYLIST_VIDEO) &&
      (g_application.IsPlayingAudio()) &&
      (
        (g_playlistPlayer.GetCurrentSong() == iSelected) ||
        (g_playlistPlayer.GetCurrentSong() == iNew)
      )
    )
    {
      return ;
    }
    CPlayList& playlist = g_playlistPlayer.GetPlaylist(PLAYLIST_VIDEO);
    if (playlist.Swap(iSelected, iNew))
    {
      Update(m_Directory.m_strPath);
      m_viewControl.SetSelectedItem(iNew);
      return ;
    }
  }
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

  DoSort(m_vecItems);

  m_viewControl.SetItems(m_vecItems);
}

void CGUIWindowVideoPlaylist::OnFileItemFormatLabel(CFileItem* pItem)
{
  pItem->SetThumb();
  pItem->FillInDefaultIcon();
  // Remove extension from title if it exists
  pItem->SetLabel(CUtil::GetTitleFromPath(pItem->GetLabel()));
}

void CGUIWindowVideoPlaylist::DoSort(CFileItemList& items)
{
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
    CONTROL_DISABLE(CONTROL_BTNPLAY);
    CONTROL_DISABLE(CONTROL_BTNNEXT);
    CONTROL_DISABLE(CONTROL_BTNPREVIOUS);
    CONTROL_DISABLE(CONTROL_BTNREPEAT);
    CONTROL_DISABLE(CONTROL_BTNREPEATONE);
  }

  m_viewControl.SetCurrentView(g_stSettings.m_iMyVideoPlaylistViewAsIcons);

  // Update object count label
  int iItems = m_vecItems.Size();
  if (iItems)
  {
    CFileItem* pItem = m_vecItems[0];
    if (pItem->GetLabel() == "..") iItems--;
  }
  WCHAR wszText[20];
  const WCHAR* szText = g_localizeStrings.Get(127).c_str();
  swprintf(wszText, L"%i %s", iItems, szText);

  SET_CONTROL_LABEL(CONTROL_LABELFILES, wszText);

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


void CGUIWindowVideoPlaylist::GetDirectory(const CStdString &strDirectory, CFileItemList &items)
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
}

void CGUIWindowVideoPlaylist::Update(const CStdString &strDirectory)
{
  // get selected item
  int iItem = m_viewControl.GetSelectedItem();
  CStdString strSelectedItem = "";
  if (iItem >= 0 && iItem < (int)m_vecItems.Size())
  {
    CFileItem* pItem = m_vecItems[iItem];
    if (pItem->GetLabel() != "..")
    {
      GetDirectoryHistoryString(pItem, strSelectedItem);
    }
  }

  ClearFileItems();

  m_history.Set(strSelectedItem, m_Directory.m_strPath);
  m_Directory.m_strPath = strDirectory;

  GetDirectory(m_Directory.m_strPath, m_vecItems);

  UpdateListControl();
  UpdateButtons();

  strSelectedItem = m_history.Get(m_Directory.m_strPath);
  int iCurrentSong = -1;
  // Search current playlist item
  if ((m_nTempPlayListWindow == GetID() && m_strTempPlayListDirectory.Find(m_Directory.m_strPath) > -1 && g_application.IsPlayingVideo()
       && g_playlistPlayer.GetCurrentPlaylist() == PLAYLIST_VIDEO_TEMP)
      || (GetID() == WINDOW_VIDEO_PLAYLIST && g_playlistPlayer.GetCurrentPlaylist() == PLAYLIST_VIDEO && g_application.IsPlayingVideo()) )
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

}

void CGUIWindowVideoPlaylist::GetDirectoryHistoryString(const CFileItem* pItem, CStdString& strHistoryString)
{
  strHistoryString = pItem->m_strPath;

  if (CUtil::HasSlashAtEnd(strHistoryString))
    strHistoryString.Delete(strHistoryString.size() - 1);
}

void CGUIWindowVideoPlaylist::OnClick(int iItem)
{
  if ( iItem < 0 || iItem >= (int)m_vecItems.Size() ) return ;
  CFileItem* pItem = m_vecItems[iItem];
  CStdString strPath = pItem->m_strPath;
  g_playlistPlayer.SetCurrentPlaylist( PLAYLIST_VIDEO);
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
    }
  }

  Update(m_Directory.m_strPath);

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
  ClearFileItems();
  CPlayList& playlist = g_playlistPlayer.GetPlaylist(PLAYLIST_VIDEO);

  CStdString strFileName;
  if (g_application.IsPlayingVideo() && g_playlistPlayer.GetCurrentPlaylist() == PLAYLIST_VIDEO)
  {
    const CPlayList::CPlayListItem& item = playlist[g_playlistPlayer.GetCurrentSong()];
    strFileName = item.GetFileName();
  }
  playlist.Shuffle();
  if (g_playlistPlayer.GetCurrentPlaylist() == PLAYLIST_VIDEO)
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

  Update(m_Directory.m_strPath);
}

/// \brief Save current playlist to playlist folder
void CGUIWindowVideoPlaylist::SavePlayList()
{
  CStdString strNewFileName;
  if (CGUIDialogKeyboard::ShowAndGetInput(strNewFileName, false))
  {
    // need 2 rename it
    CStdString strPath = g_stSettings.m_szAlbumDirectory;
    strPath += "\\playlists\\";

    strPath += strNewFileName;
    strPath += ".m3u";
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
    playlist.Save(strPath);
  }
}

void CGUIWindowVideoPlaylist::LoadViewMode()
{
  m_iViewAsIconsRoot = g_stSettings.m_iMyVideoPlaylistViewAsIcons;
}

void CGUIWindowVideoPlaylist::SaveViewMode()
{
  g_stSettings.m_iMyVideoPlaylistViewAsIcons = m_iViewAsIconsRoot;
  g_settings.Save();
}
