#include "stdafx.h"
#include "GUIWindowMusicPlayList.h"
#include "util.h"
#include "PlayListM3U.h"
#include "application.h"
#include "playlistplayer.h"
#include "GUIListControl.h"
#include "GUIDialogContextMenu.h"


#define CONTROL_BTNVIEWASICONS     2 
#define CONTROL_BTNSORTBY          3
#define CONTROL_BTNSORTASC         4
#define CONTROL_LIST              50
#define CONTROL_THUMBS            51
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


CGUIWindowMusicPlayList::CGUIWindowMusicPlayList(void)
    : CGUIWindowMusicBase(WINDOW_MUSIC_PLAYLIST, "MyMusicPlaylist.xml")
{
  m_tagloader.SetObserver(this);
}

CGUIWindowMusicPlayList::~CGUIWindowMusicPlayList(void)
{
}

bool CGUIWindowMusicPlayList::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_PLAYBACK_STOPPED:
    {
      UpdateButtons();
    }
    break;

  case GUI_MSG_PLAYLIST_CHANGED:
    {
      // global playlist changed outside playlist window
      Update("");

      if (m_viewControl.HasControl(m_iLastControl) && m_vecItems.Size() <= 0)
      {
        m_iLastControl = CONTROL_BTNVIEWASICONS;
        SET_CONTROL_FOCUS(m_iLastControl, 0);
      }

    }
    break;

  case GUI_MSG_WINDOW_INIT:
    {
      // Setup item cache for tagloader
      m_tagloader.UseCacheOnHD("Z:\\MusicPlaylist.fi");

      CGUIWindowMusicBase::OnMessage(message);

      if (g_playlistPlayer.Repeated(PLAYLIST_MUSIC))
      {
        CONTROL_SELECT(CONTROL_BTNREPEAT);
      }
      else
      {
        CONTROL_DESELECT(CONTROL_BTNREPEAT);
      }

      if (g_playlistPlayer.RepeatedOne(PLAYLIST_MUSIC))
      {
        CONTROL_SELECT(CONTROL_BTNREPEATONE);
      }
      else
      {
        CONTROL_DESELECT(CONTROL_BTNREPEATONE);
      }

      if (g_playlistPlayer.ShuffledPlay(PLAYLIST_MUSIC))
      {
        CONTROL_SELECT(CONTROL_BTNRANDOMIZE);
      }
      else
      {
        CONTROL_DESELECT(CONTROL_BTNRANDOMIZE);
      }

      if (g_playlistPlayer.GetPlaylist(PLAYLIST_MUSIC).IsShuffled())
      {
        CONTROL_SELECT(CONTROL_BTNSHUFFLE);
      }
      else
      {
        CONTROL_DESELECT(CONTROL_BTNSHUFFLE);
      }

      if (m_viewControl.HasControl(m_iLastControl) && m_vecItems.Size() <= 0)
      {
        m_iLastControl = CONTROL_BTNVIEWASICONS;
        SET_CONTROL_FOCUS(m_iLastControl, 0);
      }

      if (g_application.IsPlayingAudio() && g_playlistPlayer.GetCurrentPlaylist() == PLAYLIST_MUSIC)
      {
        int iSong = g_playlistPlayer.GetCurrentSong();
        if (iSong >= 0 && iSong <= m_vecItems.Size())
        {
          m_viewControl.SetSelectedItem(iSong);
        }
      }

      return true;
    }
    break;

  case GUI_MSG_WINDOW_DEINIT:
    {
      if (m_tagloader.IsLoading())
        m_tagloader.StopThread();

      // items should not be freed, else the musicdatabase 
      // info is lost
      //for (int i = 0; i < (int)m_vecItems.Size(); ++i)
      //{
      //  CFileItem* pItem = m_vecItems[i];
      //  pItem->FreeMemory();
      //  // Do not clear musicinfo for cue items
      //  if (pItem->m_lEndOffset == 0)
      //    pItem->m_musicInfoTag.Clear();
      //}
    }
    break;

  case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();

      if (iControl == CONTROL_BTNRANDOMIZE)
      {
        g_stSettings.m_bMyMusicPlaylistShuffle = !g_playlistPlayer.ShuffledPlay(PLAYLIST_MUSIC);
        g_settings.Save();
        g_playlistPlayer.ShufflePlay(PLAYLIST_MUSIC, g_stSettings.m_bMyMusicPlaylistShuffle);
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
        if (m_tagloader.IsLoading())
          m_tagloader.StopThread();

        ClearPlayList();
      }
      else if (iControl == CONTROL_BTNPLAY)
      {
        g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_MUSIC);
        g_playlistPlayer.Reset();
        g_playlistPlayer.Play(m_viewControl.GetSelectedItem());
        UpdateButtons();
      }
      else if (iControl == CONTROL_BTNNEXT)
      {
        g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_MUSIC);
        g_playlistPlayer.PlayNext();
      }
      else if (iControl == CONTROL_BTNPREVIOUS)
      {
        g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_MUSIC);
        g_playlistPlayer.PlayPrevious();
      }
      else if (iControl == CONTROL_BTNREPEAT)
      {
        g_stSettings.m_bMyMusicPlaylistRepeat = !g_stSettings.m_bMyMusicPlaylistRepeat;
        g_settings.Save();
        g_playlistPlayer.Repeat(PLAYLIST_MUSIC, g_stSettings.m_bMyMusicPlaylistRepeat);
      }
      else if (iControl == CONTROL_BTNREPEATONE)
      {
        static bool bRepeatOne = false;
        bRepeatOne = !bRepeatOne;
        g_playlistPlayer.RepeatOne(PLAYLIST_MUSIC, bRepeatOne);
      }
      else if (m_viewControl.HasControl(iControl))
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
  return CGUIWindowMusicBase::OnMessage(message);
}

bool CGUIWindowMusicPlayList::OnAction(const CAction &action)
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
  return CGUIWindowMusicBase::OnAction(action);
}

void CGUIWindowMusicPlayList::MoveCurrentPlayListItem(int iItem, int iAction)
{
  int iSelected = iItem;
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
    (g_playlistPlayer.GetCurrentPlaylist() == PLAYLIST_MUSIC) &&
    (g_application.IsPlayingAudio()) &&
    (
      (g_playlistPlayer.GetCurrentSong() == iSelected) ||
      (g_playlistPlayer.GetCurrentSong() == iNew)
    )
  )
  {
    return ;
  }
  CPlayList& playlist = g_playlistPlayer.GetPlaylist(PLAYLIST_MUSIC);
  if (playlist.Swap(iSelected, iNew))
  {
    Update(m_vecItems.m_strPath);
    m_viewControl.SetSelectedItem(iNew);
    return ;
  }
}

bool CGUIWindowMusicPlayList::GetDirectory(const CStdString &strDirectory, CFileItemList &items)
{
  if (items.Size())
  {
    ClearFileItems();
  }

  CPlayList& playlist = g_playlistPlayer.GetPlaylist(PLAYLIST_MUSIC);
  /* copy playlist from general playlist*/
  CStdString strPath, strFileName;
  items.Reserve(playlist.size());

  for (int i = 0; i < playlist.size(); ++i)
  {
    CPlayList::CPlayListItem& item = playlist[i];

    // Commented by JM - don't see the point of this code, as our label is in the least
    // the filename.  Why precede it with a number?  Also, if we have a "proper" label
    // from the EXFINF code, then this will override it.
/*    {
      CStdString strFileName = item.GetFileName();
      CStdString strPath, strFName;
      CUtil::Split( strFileName, strPath, strFName);
      if (CUtil::HasSlashAtEnd(strFileName))
        strFileName.Delete(strFileName.size());

      CStdString strLabel;
      strLabel.Format("%02.2i. %s", i + 1, strFName);
      item.SetLabel(strLabel);
    }*/

    if (item.GetDuration())
    {
      int nDuration = item.GetDuration();
      if (nDuration > 0)
      {
        CStdString str;
        CUtil::SecondsToHMSString(nDuration, str);
        item.SetLabel2(str);
      }
      else
        item.SetLabel2("");
    }
    items.Add(&item);
  }

  // Set default icons first,
  // the tagloader will load the thumbs later
  items.FillInDefaultIcons();

  return true;
}

void CGUIWindowMusicPlayList::SavePlayList()
{
  CStdString strNewFileName;
  if (CGUIDialogKeyboard::ShowAndGetInput(strNewFileName, (CStdStringW)g_localizeStrings.Get(16012), false))
  {
    // need 2 rename it
    CStdString strFolder, strPath;
    strFolder = CUtil::MusicPlaylistsLocation();
    CUtil::RemoveIllegalChars( strNewFileName );
    strNewFileName += ".m3u";
    CUtil::AddFileToFolder(strFolder, strNewFileName, strPath);

    CPlayListM3U playlist;
    for (int i = 0; i < (int)m_vecItems.Size(); ++i)
    {
      CFileItem* pItem = m_vecItems[i];
      CPlayList::CPlayListItem newItem;
      newItem.SetFileName(pItem->m_strPath);
      newItem.SetDescription(pItem->GetLabel());
      if (pItem->m_musicInfoTag.GetDuration())
        newItem.SetDuration(pItem->m_musicInfoTag.GetDuration());
      else
        newItem.SetDuration(0);

      //  Musicdatabase items should contain the real path instead of a musicdb url
      //  otherwise the user can't save and reuse the playlist when the musicdb gets deleted
      if (pItem->IsMusicDb())
        newItem.m_strPath=pItem->m_musicInfoTag.GetURL();

      playlist.Add(newItem);
    }
    CLog::Log(LOGDEBUG, "Saving music playlist: [%s]", strPath.c_str());
    playlist.Save(strPath);
  }
}

void CGUIWindowMusicPlayList::ClearPlayList()
{
  ClearFileItems();
  g_playlistPlayer.GetPlaylist(PLAYLIST_MUSIC).Clear();
  if (g_playlistPlayer.GetCurrentPlaylist() == PLAYLIST_MUSIC)
  {
    g_playlistPlayer.Reset();
    g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_NONE);
  }
  OnSort();
  UpdateButtons();
  SET_CONTROL_FOCUS(CONTROL_BTNVIEWASICONS, 0);
}

void CGUIWindowMusicPlayList::ShufflePlayList()
{
  int iPlaylist = PLAYLIST_MUSIC;
  ClearFileItems();
  CPlayList& playlist = g_playlistPlayer.GetPlaylist(iPlaylist);

  CStdString strFileName;
  if (g_application.IsPlayingAudio() && g_playlistPlayer.GetCurrentPlaylist() == iPlaylist)
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

void CGUIWindowMusicPlayList::RemovePlayListItem(int iItem)
{
  if (iItem < 0 || iItem > m_vecItems.Size()) return;

  // The current playing song can't be removed
  if (g_playlistPlayer.GetCurrentPlaylist() == PLAYLIST_MUSIC && g_application.IsPlayingAudio()
      && g_playlistPlayer.GetCurrentSong() == iItem)
    return ;

  g_playlistPlayer.GetPlaylist(PLAYLIST_MUSIC).Remove(iItem);

  // Correct the current playing song in playlistplayer
  if (g_playlistPlayer.GetCurrentPlaylist() == PLAYLIST_MUSIC && g_application.IsPlayingAudio())
  {
    int iCurrentSong = g_playlistPlayer.GetCurrentSong();
    if (iItem <= iCurrentSong)
    {
      iCurrentSong--;
      g_playlistPlayer.SetCurrentSong(iCurrentSong);
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

void CGUIWindowMusicPlayList::UpdateButtons()
{
  CGUIWindowMusicBase::UpdateButtons();

  // Update playlist buttons
  if (m_vecItems.Size() )
  {
    CONTROL_ENABLE(CONTROL_BTNSHUFFLE);
    CONTROL_ENABLE(CONTROL_BTNRANDOMIZE);
    CONTROL_ENABLE(CONTROL_BTNSAVE);
    CONTROL_ENABLE(CONTROL_BTNCLEAR);
    CONTROL_ENABLE(CONTROL_BTNREPEAT);
    CONTROL_ENABLE(CONTROL_BTNREPEATONE);
    CONTROL_ENABLE(CONTROL_BTNPLAY);

    if (g_application.IsPlayingAudio() && g_playlistPlayer.GetCurrentPlaylist() == PLAYLIST_MUSIC)
    {
      CONTROL_ENABLE(CONTROL_BTNNEXT);
      CONTROL_ENABLE(CONTROL_BTNPREVIOUS);
    }
    else
    {
      CONTROL_DISABLE(CONTROL_BTNNEXT);
      CONTROL_DISABLE(CONTROL_BTNPREVIOUS);
    }

    // disable repeat options if clear on end is enabled
    if (g_guiSettings.GetBool("MusicPlaylist.ClearPlaylistsOnEnd"))
    {
      g_playlistPlayer.Repeat(PLAYLIST_MUSIC, false);
      g_playlistPlayer.RepeatOne(PLAYLIST_MUSIC, false);
      CONTROL_DISABLE(CONTROL_BTNREPEAT);
      CONTROL_DISABLE(CONTROL_BTNREPEATONE);
    }
  }
  else
  {
    CONTROL_DISABLE(CONTROL_BTNSHUFFLE);
    CONTROL_DISABLE(CONTROL_BTNRANDOMIZE);
    CONTROL_DISABLE(CONTROL_BTNSAVE);
    CONTROL_DISABLE(CONTROL_BTNCLEAR);
    CONTROL_DISABLE(CONTROL_BTNREPEAT);
    CONTROL_DISABLE(CONTROL_BTNREPEATONE);
    CONTROL_DISABLE(CONTROL_BTNPLAY);
    CONTROL_DISABLE(CONTROL_BTNNEXT);
    CONTROL_DISABLE(CONTROL_BTNPREVIOUS);
  }

  // Update object count label
  int iItems = m_vecItems.Size();
  if (iItems)
  {
    CFileItem* pItem = m_vecItems[0];
    if (pItem->IsParentFolder()) iItems--;
  }
  WCHAR wszText[20];
  const WCHAR* szText = g_localizeStrings.Get(127).c_str();
  swprintf(wszText, L"%i %s", iItems, szText);

  SET_CONTROL_LABEL(CONTROL_LABELFILES, wszText);
}

void CGUIWindowMusicPlayList::OnClick(int iItem)
{
  if ( iItem < 0 || iItem >= m_vecItems.Size() ) return ;
  CFileItem* pItem = m_vecItems[iItem];
  CStdString strPath = pItem->m_strPath;
  g_playlistPlayer.SetCurrentPlaylist( PLAYLIST_MUSIC );
  g_playlistPlayer.Reset();
  g_playlistPlayer.Play( iItem );
}

void CGUIWindowMusicPlayList::OnItemLoaded(CFileItem* pItem)
{
  // FIXME: get the position of the item in the playlist
  int iTrack = 0;
  for (int i = 0; i < m_vecItems.Size(); ++i)
  {
    if (pItem == m_vecItems[i])
    {
      iTrack = i + 1;
      break;
    }
  }

  if (pItem->m_musicInfoTag.Loaded())
  { // set label 1+2 from tags
    auto_ptr<CGUIViewState> pState(CGUIViewState::GetViewState(GetID(), m_vecItems));
    if (pState.get()) m_hideExtensions = pState->HideExtensions();
    pItem->FormatLabel(g_guiSettings.GetString("MyMusic.TrackFormat"));
    pItem->FormatLabel2(g_guiSettings.GetString("MyMusic.TrackFormatRight"));

  } // if (pItem->m_musicInfoTag.Loaded())
  else
  {
    // Our tag may have a duration even if its not loaded
    if (pItem->m_musicInfoTag.GetDuration())
    {
      int nDuration = pItem->m_musicInfoTag.GetDuration();
      if (nDuration > 0)
      {
        CStdString str;
        CUtil::SecondsToHMSString(nDuration, str);
        pItem->SetLabel2(str);
      }
    }
    else if (pItem->GetLabel() == "") // pls labels come in preformatted
    {
      // No music info and it's not CDDA so we'll just show the filename
      CStdString str;
      str = CUtil::GetTitleFromPath(pItem->m_strPath);
      str.Format("%02.2i. %s ", iTrack, str);
      pItem->SetLabel(str);
    }
  }

  //  MusicDb items already have thumbs
  if (!pItem->IsMusicDb())
  {
    g_graphicsContext.Lock();
    // Remove default icons
    pItem->FreeIcons();
    // and reset thumbs and default icons
    pItem->SetMusicThumb();
    pItem->FillInDefaultIcon();
    g_graphicsContext.Unlock();
  }
}

bool CGUIWindowMusicPlayList::Update(const CStdString& strDirectory)
{
  if (m_tagloader.IsLoading())
    m_tagloader.StopThread();

  bool bRet=CGUIWindowMusicBase::Update(strDirectory);

  m_tagloader.Load(m_vecItems);

  return bRet;
}

void CGUIWindowMusicPlayList::ClearFileItems()
{
  m_viewControl.Clear();
  for (int i=0; i<m_vecItems.Size(); ++i)
  {
    CFileItem* item=m_vecItems[i];
    item->FreeMemory();
  }
  m_vecItems.ClearKeepPointer();
}

void CGUIWindowMusicPlayList::OnPopupMenu(int iItem)
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
  bool bSelected = m_vecItems[iItem]->IsSelected(); // item maybe selected (playlistitem)
  m_vecItems[iItem]->Select(true);
  // popup the context menu
  CGUIDialogContextMenu *pMenu = (CGUIDialogContextMenu *)m_gWindowManager.GetWindow(WINDOW_DIALOG_CONTEXT_MENU);
  if (!pMenu) return ;
  // load our menu
  pMenu->Initialize();

  // add the needed buttons
  int btn_MoveUp = pMenu->AddButton(13332); // Move Up
  int btn_MoveDn = pMenu->AddButton(13333); // Move Down
  int btn_Delete = pMenu->AddButton(15015); // Remove
  int btn_Return = pMenu->AddButton(12010); // Return to My Music

  // position it correctly
  pMenu->SetPosition(iPosX - pMenu->GetWidth() / 2, iPosY - pMenu->GetHeight() / 2);
  pMenu->DoModal(GetID());

  int btnid = pMenu->GetButton();
  if( btnid  == btn_MoveUp ) // Move Up
  {
    OnMove(iItem, ACTION_MOVE_ITEM_UP);
  }
  else if( btnid  == btn_MoveDn )  // Move Down
  {
    OnMove(iItem, ACTION_MOVE_ITEM_DOWN);
  }
  else if( btnid  == btn_Delete )  // Delete
  {
    RemovePlayListItem(iItem);
    return;
  }
  else if( btnid  == btn_Return )  // Return
  {
    m_gWindowManager.PreviousWindow();
    return;
  }
  m_vecItems[iItem]->Select(bSelected);
}

void CGUIWindowMusicPlayList::OnMove(int iItem, int iAction)
{
  if (iItem < 0 || iItem >= m_vecItems.Size()) return;

  bool bRestart = m_tagloader.IsLoading();
  if (bRestart)
    m_tagloader.StopThread();

  MoveCurrentPlayListItem(iItem, iAction);

  if (bRestart)
    m_tagloader.Load(m_vecItems);
}
