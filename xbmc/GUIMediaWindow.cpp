#include "stdafx.h"
#include "GUIMediaWindow.h"
#include "util.h"
#include "detectdvdtype.h"
#include "PlayListPlayer.h"
#include "FileSystem/ZipManager.h"
#include "GUIPassword.h"
#include "Application.h"
#ifdef PRE_SKIN_VERSION_2_0_COMPATIBILITY
#include "SkinInfo.h"
#endif

#define CONTROL_BTNVIEWASICONS     2
#define CONTROL_BTNSORTBY          3
#define CONTROL_BTNSORTASC         4
#define CONTROL_LIST              50
#define CONTROL_THUMBS            51
#define CONTROL_BIG_LIST          52
#define CONTROL_LABELFILES        12


CGUIMediaWindow::CGUIMediaWindow(DWORD id, const char *xmlFile)
    : CGUIWindow(id, xmlFile)
{
  m_vecItems.m_strPath = "?";
  m_iLastControl = -1;
  m_iSelectedItem = -1;

  m_guiState.reset(CGUIViewState::GetViewState(GetID(), m_vecItems));
}

CGUIMediaWindow::~CGUIMediaWindow()
{
}

void CGUIMediaWindow::OnWindowLoaded()
{
  CGUIWindow::OnWindowLoaded();
  m_viewControl.Reset();
  m_viewControl.SetParentWindow(GetID());
  m_viewControl.AddView(VIEW_METHOD_LIST, GetControl(CONTROL_LIST));
  m_viewControl.AddView(VIEW_METHOD_ICONS, GetControl(CONTROL_THUMBS));
  m_viewControl.AddView(VIEW_METHOD_LARGE_ICONS, GetControl(CONTROL_THUMBS));
  m_viewControl.AddView(VIEW_METHOD_LARGE_LIST, GetControl(CONTROL_BIG_LIST));
  m_viewControl.SetViewControlID(CONTROL_BTNVIEWASICONS);
}

void CGUIMediaWindow::OnWindowUnload()
{
  CGUIWindow::OnWindowUnload();
  m_viewControl.Reset();
}

const CFileItem *CGUIMediaWindow::GetCurrentListItem() const
{
  int iItem = m_viewControl.GetSelectedItem();
  if (iItem < 0) return NULL;
  return m_vecItems[iItem];
}

bool CGUIMediaWindow::OnAction(const CAction &action)
{
  if (action.wID == ACTION_PARENT_DIR)
  {
    GoParentFolder();
    return true;
  }

  if (action.wID == ACTION_PREVIOUS_MENU)
  {
    m_gWindowManager.PreviousWindow();
    return true;
  }

  return CGUIWindow::OnAction(action);
}

bool CGUIMediaWindow::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_DEINIT:
    {
      m_iSelectedItem = m_viewControl.GetSelectedItem();
      m_iLastControl = GetFocusedControl();
      CGUIWindow::OnMessage(message);
      // Call ClearFileItems() after our window has finished doing any WindowClose
      // animations
      ClearFileItems();
      return true;
    }
    break;

  case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();
      if (iControl == CONTROL_BTNVIEWASICONS)
      {
        if (m_guiState.get())
          while (!m_viewControl.HasViewMode(m_guiState->SetNextViewAsControl()));

        UpdateButtons();
        return true;
      }
      else if (iControl == CONTROL_BTNSORTASC) // sort asc
      {
        if (m_guiState.get())
          m_guiState->SetNextSortOrder();
        UpdateFileList();
        return true;
      }
      else if (iControl == CONTROL_BTNSORTBY) // sort by
      {
        if (m_guiState.get())
          m_guiState->SetNextSortMethod();
        UpdateFileList();
        return true;
      }
      else if (m_viewControl.HasControl(iControl))  // list/thumb control
      {
        int iItem = m_viewControl.GetSelectedItem();
        int iAction = message.GetParam1();
        if (iItem < 0) break;
        if (iAction == ACTION_SELECT_ITEM || iAction == ACTION_MOUSE_LEFT_CLICK)
        {
          OnClick(iItem);
        }
      }
    }
    break;

  case GUI_MSG_SETFOCUS:
    {
      if (m_viewControl.HasControl(message.GetControlId()) && m_viewControl.GetCurrentControl() != message.GetControlId())
      {
        m_viewControl.SetFocused();
        return true;
      }
    }
    break;

  case GUI_MSG_NOTIFY_ALL:
    { // Message is received even if this window is inactive
      
      //  Is there a dvd share in this window?
      if (!m_rootDir.GetDVDDriveUrl().IsEmpty())
      {
        if (message.GetParam1()==GUI_MSG_DVDDRIVE_EJECTED_CD)
        {
          if (m_vecItems.IsVirtualDirectoryRoot() && IsActive())
          {
            int iItem = m_viewControl.GetSelectedItem();
            Update(m_vecItems.m_strPath);
            m_viewControl.SetSelectedItem(iItem);
          }
          else if (m_vecItems.IsCDDA() || m_vecItems.IsOnDVD())
          { // Disc has changed and we are inside a DVD Drive share, get out of here :)
            if (IsActive()) Update("");
            else 
            {
              m_history.ClearPathHistory();
              m_vecItems.m_strPath="";
            }
          }

          return true;
        }
        else if (message.GetParam1()==GUI_MSG_DVDDRIVE_CHANGED_CD)
        { // State of the dvd-drive changed (Open/Busy label,...), so update it
          if (m_vecItems.IsVirtualDirectoryRoot() && IsActive())
          {
            int iItem = m_viewControl.GetSelectedItem();
            Update(m_vecItems.m_strPath);
            m_viewControl.SetSelectedItem(iItem);
          }

          return true;
        }
      }
    }
    break;

  case GUI_MSG_PLAYBACK_STARTED:
    {
      UpdateButtons();
    }
    break;

  case GUI_MSG_PLAYBACK_ENDED:
  case GUI_MSG_PLAYBACK_STOPPED:
  case GUI_MSG_PLAYLISTPLAYER_STOPPED:
    {
      CStdString strDirectory = m_vecItems.m_strPath;
      if (CUtil::HasSlashAtEnd(strDirectory))
        strDirectory.Delete(strDirectory.size() - 1);
      if (m_guiState.get() && strDirectory==m_guiState->GetPlaylistDirectory())
      {
        for (int i = 0; i < m_vecItems.Size(); ++i)
        {
          CFileItem* pItem = m_vecItems[i];
          if (pItem && pItem->IsSelected())
          {
            pItem->Select(false);
            break;
          }
        }
      }

      UpdateButtons();
    }
    break;

  case GUI_MSG_PLAYLISTPLAYER_STARTED:
  case GUI_MSG_PLAYLISTPLAYER_CHANGED:
    {
      // started playing another song...
      int nCurrentPlaylist = message.GetParam1();
      CStdString strDirectory = m_vecItems.m_strPath;
      if (CUtil::HasSlashAtEnd(strDirectory))
        strDirectory.Delete(strDirectory.size() - 1);
      if (m_guiState.get() && g_playlistPlayer.GetCurrentPlaylist()==m_guiState->GetPlaylist() && strDirectory==m_guiState->GetPlaylistDirectory())
      {
        int nCurrentItem = 0;
        int nPreviousItem = -1;
        if (message.GetMessage() == GUI_MSG_PLAYLISTPLAYER_STARTED)
        {
          nCurrentItem = message.GetParam2();
        }
        else if (message.GetMessage() == GUI_MSG_PLAYLISTPLAYER_CHANGED)
        {
          nCurrentItem = LOWORD(message.GetParam2());
          nPreviousItem = HIWORD(message.GetParam2());
        }
        CStdString strCurrentItem = g_playlistPlayer.GetPlaylist(g_playlistPlayer.GetCurrentPlaylist())[nCurrentItem].m_strPath;

        int nFolderCount = m_vecItems.GetFolderCount();

        // is the previous item in this directory
        for (int i = nFolderCount; i < m_vecItems.Size(); i++)
        {
          CFileItem* pItem = m_vecItems[i];

          if (pItem)
            pItem->Select(false);
        }

        if (nFolderCount + nCurrentItem < m_vecItems.Size())
        {
          for (int i = nFolderCount, n = 0; i < m_vecItems.Size(); i++)
          {
            CFileItem* pItem = m_vecItems[i];

            if (pItem)
            {
              if (!pItem->IsPlayList() && !pItem->IsNFO() && !pItem->IsRAR() && !pItem->IsZIP())
                n++;
              if ((n - 1) == nCurrentItem)
              {
                pItem->Select(true);
                break;
              }
            }
          } // for (int i=nFolderCount, n=0; i<(int)m_vecItems.size(); i++)
        }

      }
    }
    break;
  }

  return CGUIWindow::OnMessage(message);
}

// \brief Updates the states (enable, disable, visible...) 
// of the controls defined by this window
// Override this function in a derived class to add new controls
void CGUIMediaWindow::UpdateButtons()
{
  if (m_guiState.get())
  {
    // Update sorting controls
    if (m_guiState->GetSortOrder()==SORT_ORDER_NONE)
    {
      CONTROL_DISABLE(CONTROL_BTNSORTASC);
    }
    else
    {
      CONTROL_ENABLE(CONTROL_BTNSORTASC);
      if (m_guiState->GetSortOrder()==SORT_ORDER_ASC)
      {
        CGUIMessage msg(GUI_MSG_DESELECTED, GetID(), CONTROL_BTNSORTASC);
        g_graphicsContext.SendMessage(msg);
      }
      else
      {
        CGUIMessage msg(GUI_MSG_SELECTED, GetID(), CONTROL_BTNSORTASC);
        g_graphicsContext.SendMessage(msg);
      }
    }

    // Update list/thumb control
    m_viewControl.SetCurrentView(m_guiState->GetViewAsControl());

    // Update sort by button
    if (m_guiState->GetSortMethod()==SORT_METHOD_NONE)
    {
      CONTROL_DISABLE(CONTROL_BTNSORTBY);
    }
    else
    {
      CONTROL_ENABLE(CONTROL_BTNSORTBY);
    }
    SET_CONTROL_LABEL(CONTROL_BTNSORTBY, m_guiState->GetSortMethodLabel());
  }

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

void CGUIMediaWindow::ClearFileItems()
{
  m_viewControl.Clear();
  m_vecItems.Clear(); // will clean up everything
}

// \brief Sorts Fileitems based on the sort method and sort oder provided by guiViewState
void CGUIMediaWindow::SortItems(CFileItemList &items)
{
  auto_ptr<CGUIViewState> guiState(CGUIViewState::GetViewState(GetID(), items));

  if (guiState.get())
  {
    items.Sort(guiState->GetSortMethod(), guiState->GetSortOrder());

    // Should these items be saved to the hdd
    if (items.GetCacheToDisc())
      items.Save();
  }
}

// \brief Formats item labels based on the formatting provided by guiViewState
void CGUIMediaWindow::FormatItemLabels()
{
  if (!m_guiState.get())
    return;

  CGUIViewState::LABEL_MASKS labelMasks;
  m_guiState->GetSortMethodLabelMasks(labelMasks);

  for (int i=0; i<m_vecItems.Size(); ++i)
  {
    CFileItem* pItem=m_vecItems[i];

    if (pItem->IsLabelPreformated())
      continue;

    pItem->FormatLabel(pItem->m_bIsFolder ? labelMasks.m_strLabelFolder : labelMasks.m_strLabelFile);
    pItem->FormatLabel2(pItem->m_bIsFolder ? labelMasks.m_strLabel2Folder : labelMasks.m_strLabel2File);
  }
}

// \brief Prepares and adds the fileitems list/thumb panel
void CGUIMediaWindow::OnSort()
{
  FormatItemLabels();
  SortItems(m_vecItems);
  m_viewControl.SetItems(m_vecItems);
}

/*!
  \brief Overwrite to fill fileitems from a source
  \param strDirectory Path to read
  \param items Fill with items specified in \e strDirectory
  */
bool CGUIMediaWindow::GetDirectory(const CStdString &strDirectory, CFileItemList &items)
{
  // cleanup items
  if (items.Size())
    items.Clear();

  CStdString strParentPath=m_history.GetParentPath();

  CLog::Log(LOGDEBUG,"CGUIMediaWindow::GetDirectory (%s)", strDirectory.c_str());
  CLog::Log(LOGDEBUG,"  ParentPath = [%s]", strParentPath.c_str());

  if (m_guiState.get() && !m_guiState->HideParentDirItems())
  {
    CFileItem *pItem = new CFileItem("..");
    pItem->m_strPath = strParentPath;
    pItem->m_bIsFolder = true;
    pItem->m_bIsShareOrDrive = false;
    items.Add(pItem);
  }

  if (!m_rootDir.GetDirectory(strDirectory, items))
    return false;

  return true;
}

// \brief Set window to a specific directory
// \param strDirectory The directory to be displayed in list/thumb control
// This function calls OnPrepareFileItems() and OnFinalizeFileItems()
bool CGUIMediaWindow::Update(const CStdString &strDirectory)
{
  // get selected item
  int iItem = m_viewControl.GetSelectedItem();
  CStdString strSelectedItem = "";
  if (iItem >= 0 && iItem < m_vecItems.Size())
  {
    CFileItem* pItem = m_vecItems[iItem];
    if (!pItem->IsParentFolder())
    {
      GetDirectoryHistoryString(pItem, strSelectedItem);
    }
  }

  CStdString strOldDirectory = m_vecItems.m_strPath;

  m_history.SetSelectedItem(strSelectedItem, strOldDirectory);

  ClearFileItems();

  if (!GetDirectory(strDirectory, m_vecItems))
  {
    CLog::Log(LOGERROR,"CGUIMediaWindow::GetDirectory(%s) failed", strDirectory.c_str());
    return !Update(strOldDirectory); // We assume, we can get the parent 
  }                                  // directory again, but we have to 
                                     // return false to be able to eg. show 
                                     // an error message.

  // if we're getting the root bookmark listing
  // make sure the path history is clean
  if (strDirectory.IsEmpty())
    m_history.ClearPathHistory();

  m_iLastControl = GetFocusedControl();

  //  Ask the derived class if it wants to load additional info
  //  for the fileitems like media info or additional 
  //  filtering on the items, setting thumbs.
  if (!m_vecItems.IsVirtualDirectoryRoot())
  {
    OnPrepareFileItems(m_vecItems);

    m_vecItems.FillInDefaultIcons();
  }

  m_guiState.reset(CGUIViewState::GetViewState(GetID(), m_vecItems));
  OnSort();
  UpdateButtons();

  if (!m_vecItems.IsVirtualDirectoryRoot())
  {
    // Ask the devived class if it wants to do custom list operations,
    // eg. changing the label
    OnFinalizeFileItems(m_vecItems);

    if (m_guiState.get() && m_guiState->HideExtensions())
      m_vecItems.RemoveExtensions();
  }

  strSelectedItem = m_history.GetSelectedItem(m_vecItems.m_strPath);

  int iCurrentPlaylistSong = -1;
  CStdString strCurrentPlaylistSong;
  // Search current playlist item
  CStdString strCurrentDirectory = m_vecItems.m_strPath;
  if (CUtil::HasSlashAtEnd(strCurrentDirectory))
    strCurrentDirectory.Delete(strCurrentDirectory.size() - 1);
  // Is this window responsible for the current playlist?
  if (g_application.IsPlaying() && m_guiState.get() && 
      g_playlistPlayer.GetCurrentPlaylist()==m_guiState->GetPlaylist() && 
      strCurrentDirectory==m_guiState->GetPlaylistDirectory())
  {
    iCurrentPlaylistSong = g_playlistPlayer.GetCurrentSong();
  }

  bool bSelectedFound = false, bCurrentSongFound = false;
  int iSongInDirectory = -1;
  for (int i = 0; i < m_vecItems.Size(); ++i)
  {
    CFileItem* pItem = m_vecItems[i];

    // unselect all items
    if (pItem)
      pItem->Select(false);

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
    if (!bCurrentSongFound && iCurrentPlaylistSong > -1)
    {
      if (!pItem->m_bIsFolder && !pItem->IsPlayList() && !pItem->IsNFO())
        iSongInDirectory++;
      if (iSongInDirectory == iCurrentPlaylistSong)
      {
        pItem->Select(true);
        bCurrentSongFound = true;
      }
    }
  }

  m_history.AddPath(strDirectory);

  //m_history.DumpPathHistory();

  return true;
}

// \brief This function will be called by Update() before the
// labels of the fileitems are formatted. Override this function
// to set custom thumbs or load additional media info. 
// It's used to load tag info for music.
void CGUIMediaWindow::OnPrepareFileItems(CFileItemList &items)
{

}

// \brief This function will be called by Update() after the
// labels of the fileitems are formatted. Override this function
// to modify the fileitems. Eg. to modify the item label
void CGUIMediaWindow::OnFinalizeFileItems(CFileItemList &items)
{

}

// \brief With this function you can react on a users click in the list/thumb panel.
// It returns true, if the click is handled.
// This function calls OnPlayMedia()
bool CGUIMediaWindow::OnClick(int iItem)
{
  if ( iItem < 0 || iItem >= (int)m_vecItems.Size() ) return true;
  CFileItem* pItem = m_vecItems[iItem];

  if (pItem->IsParentFolder())
  {
    GoParentFolder();
    return true;
  }
  else if (pItem->m_bIsFolder)
  {
    if ( pItem->m_bIsShareOrDrive )
    {
      const CStdString& strLockType=m_guiState->GetLockType();
      if (!strLockType.IsEmpty() && !g_passwordManager.IsItemUnlocked(pItem, strLockType))
          return true;

      if (!HaveDiscOrConnection(pItem->m_strPath, pItem->m_iDriveType))
        return true;
    }

    CFileItem directory(*pItem);
    if (!Update(directory.m_strPath))
      ShowShareErrorMessage(&directory);

    return true;
  }
  else if (pItem->IsZIP() && m_guiState.get() && m_guiState->HandleArchives()) // mount zip archive
  {
    CShare shareZip;
    shareZip.strPath.Format("zip://Z:\\,%i,,%s,\\",1, pItem->m_strPath.c_str() );
    m_rootDir.AddShare(shareZip);
    Update(shareZip.strPath);

    return true;
  }
  else if (pItem->IsRAR()&& m_guiState.get() && m_guiState->HandleArchives()) // mount rar archive
  {
    CShare shareRar;
    shareRar.strPath.Format("rar://Z:\\,%i,,%s,\\",EXFILE_AUTODELETE, pItem->m_strPath.c_str() );
    m_rootDir.AddShare(shareRar);
    Update(shareRar.strPath);

    return true;
  }
  else
  {
    m_iSelectedItem = m_viewControl.GetSelectedItem();

    if (pItem->IsPlayList())
    {
      CStdString strPath=pItem->m_strPath;
      LoadPlayList(pItem->m_strPath);
      return true;
    }
    else if (m_guiState.get() && m_guiState->AutoPlayNextItem() && !g_application.m_bMusicPartyMode)
    {
      //play and add current directory to temporary playlist
      int iPlaylist=m_guiState->GetPlaylist();
      if (iPlaylist!=PLAYLIST_NONE)
      {
        int nFolderCount = 0;
        g_playlistPlayer.ClearPlaylist( iPlaylist );
        g_playlistPlayer.Reset();
        int iNoSongs = 0;
        for ( int i = 0; i < m_vecItems.Size(); i++ )
        {
          CFileItem* pItem = m_vecItems[i];
          
          if (pItem->m_bIsFolder)
          {
            nFolderCount++;
            continue;
          }
          if (!pItem->IsPlayList() && !pItem->IsZIP() && !pItem->IsRAR())
          {
            CPlayList::CPlayListItem playlistItem ;
            CUtil::ConvertFileItemToPlayListItem(pItem, playlistItem);
            g_playlistPlayer.GetPlaylist(iPlaylist).Add(playlistItem);
          }
          else if (i <= iItem)
            iNoSongs++;
        }

        // Save current window and directory to know where the selected item was
        CStdString strPlayListDirectory=m_vecItems.m_strPath;
        if (CUtil::HasSlashAtEnd(strPlayListDirectory))
          strPlayListDirectory.Delete(strPlayListDirectory.size() - 1);

        if (m_guiState.get())
          m_guiState->SetPlaylistDirectory(strPlayListDirectory);

        g_playlistPlayer.SetCurrentPlaylist(iPlaylist);
        g_playlistPlayer.Play(iItem - nFolderCount - iNoSongs);
      }

      return true;
    }
    else
    {
      return OnPlayMedia(iItem);
    }
  }

  return false;
}

// \brief Checks if there is a disc in the dvd drive and whether the
// network is connected or not.
bool CGUIMediaWindow::HaveDiscOrConnection(CStdString& strPath, int iDriveType)
{
  if (iDriveType==SHARE_TYPE_DVD)
  {
    MEDIA_DETECT::CDetectDVDMedia::WaitMediaReady();
    if (!MEDIA_DETECT::CDetectDVDMedia::IsDiscInDrive())
    {
      CGUIDialogOK::ShowAndGetInput(218, 219, 0, 0);
      return false;
    }
  }
  else if (iDriveType==SHARE_TYPE_REMOTE)
  {
    // TODO: Handle not connected to a remote share
    if ( !CUtil::IsEthernetConnected() )
    {
      CGUIDialogOK::ShowAndGetInput(220, 221, 0, 0);
      return false;
    }
  }

  return true;
}

// \brief Shows a standard errormessage for a given pItem.
void CGUIMediaWindow::ShowShareErrorMessage(CFileItem* pItem)
{
  if (pItem->m_bIsShareOrDrive)
  {
    int idMessageText=0;
    const CURL& url=pItem->GetAsUrl();
    const CStdString& strHostName=url.GetHostName();

    if (pItem->m_iDriveType!=SHARE_TYPE_REMOTE) //  Local shares incl. dvd drive
      idMessageText=15300;
    else if (url.GetProtocol()=="xbms" && strHostName.IsEmpty()) //  xbms server discover
      idMessageText=15302;
    else if (url.GetProtocol()=="smb" && strHostName.IsEmpty()) //  smb workgroup
      idMessageText=15303;
    else  //  All other remote shares
      idMessageText=15301;

    CGUIDialogOK::ShowAndGetInput(220, idMessageText, 0, 0);
  }
}

// \brief The functon goes up one level in the directory tree
void CGUIMediaWindow::GoParentFolder()
{
  //m_history.DumpPathHistory();

  // remove current directory if its on the stack
  if (m_history.GetParentPath() == m_vecItems.m_strPath)
      m_history.RemoveParentPath();

  // if vector is not empty, pop parent
  // if vector is empty, parent is bookmark listing
  CStdString strParent=m_history.RemoveParentPath();

  // check for special archive path
  const CURL& url=m_vecItems.GetAsUrl();
  if ((url.GetProtocol() == "rar") || (url.GetProtocol() == "zip")) 
  {
    // check for step-below, if, unmount rar
    if (url.GetFileName().IsEmpty())
    {
      if (url.GetProtocol() == "zip")
        g_ZipManager.release(m_vecItems.m_strPath); // release resources
      m_rootDir.RemoveShare(m_vecItems.m_strPath);
      CStdString strPath;
      CUtil::GetDirectory(url.GetHostName(),strPath);
      Update(strPath);
      return;
    }
  }

  CStdString strOldPath(m_vecItems.m_strPath);
  Update(strParent);

  if (!g_guiSettings.GetBool("LookAndFeel.FullDirectoryHistory"))
    m_history.RemoveSelectedItem(strOldPath); //Delete current path

}

// \brief Override the function to change the default behavior on how
// a selected item history should look like
void CGUIMediaWindow::GetDirectoryHistoryString(const CFileItem* pItem, CStdString& strHistoryString)
{
  if (pItem->m_bIsShareOrDrive)
  {
    // We are in the virual directory

    // History string of the DVD drive
    // must be handel separately
    if (pItem->m_iDriveType == SHARE_TYPE_DVD)
    {
      // Remove disc label from item label
      // and use as history string, m_strPath
      // can change for new discs
      CStdString strLabel = pItem->GetLabel();
      int nPosOpen = strLabel.Find('(');
      int nPosClose = strLabel.ReverseFind(')');
      if (nPosOpen > -1 && nPosClose > -1 && nPosClose > nPosOpen)
      {
        strLabel.Delete(nPosOpen + 1, (nPosClose) - (nPosOpen + 1));
        strHistoryString = strLabel;
      }
      else
        strHistoryString = strLabel;
    }
    else
    {
      // Other items in virual directory
      CStdString strPath = pItem->m_strPath;
      while (CUtil::HasSlashAtEnd(strPath))
        strPath.Delete(strPath.size() - 1);

      strHistoryString = pItem->GetLabel() + strPath;
    }
  }
  else
  {
    // Normal directory items
    strHistoryString = pItem->m_strPath;

    if (CUtil::HasSlashAtEnd(strHistoryString))
      strHistoryString.Delete(strHistoryString.size() - 1);
  }
}

// \brief Call this function to create a directory history for the
// path given by strDirectory.
void CGUIMediaWindow::SetHistoryForPath(const CStdString& strDirectory)
{
  if (!strDirectory.IsEmpty())
  {
    // Build the directory history for default path
    CStdString strPath, strParentPath;
    strPath = strDirectory;
    while (CUtil::HasSlashAtEnd(strPath))
      strPath.Delete(strPath.size() - 1);

    CFileItemList items;
    m_rootDir.GetDirectory("", items);

    m_history.ClearPathHistory();

    while (CUtil::GetParentPath(strPath, strParentPath))
    {
      bool bSet = false;
      for (int i = 0; i < (int)items.Size(); ++i)
      {
        CFileItem* pItem = items[i];
        while (CUtil::HasSlashAtEnd(pItem->m_strPath))
          pItem->m_strPath.Delete(pItem->m_strPath.size() - 1);
        if (pItem->m_strPath == strPath)
        {
          CStdString strHistory;
          GetDirectoryHistoryString(pItem, strHistory);
          m_history.SetSelectedItem(strHistory, "");
          m_history.AddPathFront(strPath);
          m_history.AddPathFront("");

          //m_history.DumpPathHistory();
          return ;
        }
      }

      m_history.AddPathFront(strPath);
      m_history.SetSelectedItem(strPath, strParentPath);
      strPath = strParentPath;
      while (CUtil::HasSlashAtEnd(strPath))
        strPath.Delete(strPath.size() - 1);
    }
  }
  else
    m_history.ClearPathHistory();

  //m_history.DumpPathHistory();
}

// \brief Override if you want to change the default behavior, what is done
// when the user clicks on a file.
// This function is called by OnClick()
bool CGUIMediaWindow::OnPlayMedia(int iItem)
{
  // Reset Playlistplayer, playback started now does
  // not use the playlistplayer.
  g_playlistPlayer.Reset();
  g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_NONE);
  CFileItem* pItem=m_vecItems[iItem];

  return g_application.PlayFile(*pItem);
}

// \brief Synchonize the fileitems with the playlistplayer
// It recreated the playlist of the playlistplayer based
// on the fileitems of the window
void CGUIMediaWindow::UpdateFileList()
{
  int nItem = m_viewControl.GetSelectedItem();
  CFileItem* pItem = m_vecItems[nItem];
  const CStdString& strSelected = pItem->m_strPath;

  OnSort();
  UpdateButtons();
  
  m_viewControl.SetSelectedItem(strSelected);

  //  set the currently playing item as selected, if its in this directory
  CStdString strDirectory = m_vecItems.m_strPath;
  if (CUtil::HasSlashAtEnd(strDirectory))
    strDirectory.Delete(strDirectory.size() - 1);
  int iPlaylist=m_guiState->GetPlaylist();
  if (iPlaylist!=PLAYLIST_NONE)
  {
    if (!strDirectory.IsEmpty() && m_guiState.get() && g_playlistPlayer.GetCurrentPlaylist()==iPlaylist && strDirectory==m_guiState->GetPlaylistDirectory())
    {
      int nSong = g_playlistPlayer.GetCurrentSong();
      CStdString strCurrentSong = g_playlistPlayer.GetPlaylist(iPlaylist)[nSong].GetFileName();
      g_playlistPlayer.ClearPlaylist(iPlaylist);
      g_playlistPlayer.Reset();
      int nFolderCount = 0;
      int iNoSongs = 0;
      for (int i = 0; i < (int)m_vecItems.Size(); i++)
      {
        CFileItem* pItem = m_vecItems[i];
        if (pItem->m_bIsFolder)
        {
          nFolderCount++;
          continue;
        }
        if (!pItem->IsPlayList() && !pItem->IsZIP() && !pItem->IsRAR())
        {
          CPlayList::CPlayListItem playlistItem ;
          CUtil::ConvertFileItemToPlayListItem(pItem, playlistItem);
          g_playlistPlayer.GetPlaylist(iPlaylist).Add(playlistItem);
        }
        else iNoSongs++;

        if (strCurrentSong == pItem->m_strPath)
          g_playlistPlayer.SetCurrentSong(i - nFolderCount - iNoSongs);
      }
    }
  }
}
