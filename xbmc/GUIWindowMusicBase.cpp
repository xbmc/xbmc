#include "stdafx.h"
#include "GUIWindowMusicBase.h"
#include "MusicInfoTagLoaderFactory.h"
#include "GUIWindowMusicInfo.h"
#include "FileSystem/HDdirectory.h"
#include "FileSystem/ZipManager.h"
#include "PlayListFactory.h"
#include "Util.h"
#include "PlayListM3U.h"
#include "Application.h"
#include "PlayListPlayer.h"
#include "GUIThumbnailPanel.h"
#include "GUIListControl.h"
#include "FileSystem/DirectoryCache.h"
#include "CDRip/CDDARipper.h"
#include "GUIPassword.h"
#include "AutoSwitch.h"
#include "GUIFontManager.h"
#include "GUIDialogMusicScan.h"
#include "GUIDialogContextMenu.h"


#define CONTROL_BTNVIEWASICONS  2
#define CONTROL_BTNTYPE         6
#define CONTROL_BTNSEARCH       8

#define CONTROL_LIST        50
#define CONTROL_THUMBS      51
#define CONTROL_BIGLIST     52

using namespace MUSIC_GRABBER;
using namespace DIRECTORY;
using namespace PLAYLIST;

int CGUIWindowMusicBase::m_nTempPlayListWindow = 0;
CStdString CGUIWindowMusicBase::m_strTempPlayListDirectory = "";

CGUIWindowMusicBase::CGUIWindowMusicBase(DWORD dwID, const CStdString &xmlFile)
    : CGUIWindow(dwID, xmlFile)
{
  m_nSelectedItem = -1;
  m_iLastControl = -1;
  m_bDisplayEmptyDatabaseMessage = false;
  m_Directory.m_bIsFolder = true;
  m_bSectionsLoaded=false;
}

CGUIWindowMusicBase::~CGUIWindowMusicBase ()
{
}

/// \brief Handle actions on window.
/// \param action Action that can be reacted on.
bool CGUIWindowMusicBase::OnAction(const CAction& action)
{
  if (action.wID == ACTION_PARENT_DIR)
  {
    GoParentFolder();
    return true;
  }

  if (action.wID == ACTION_PREVIOUS_MENU)
  {
    CGUIDialogMusicScan *musicScan = (CGUIDialogMusicScan *)m_gWindowManager.GetWindow(WINDOW_DIALOG_MUSIC_SCAN);
    if (musicScan && !musicScan->IsRunning())
    {
      CUtil::ThumbCacheClear();
      CUtil::RemoveTempFiles();
    }

    m_gWindowManager.ActivateWindow(WINDOW_HOME);
    return true;
  }

  if (action.wID == ACTION_SHOW_PLAYLIST)
  {
    m_gWindowManager.ActivateWindow(WINDOW_MUSIC_PLAYLIST);
    return true;
  }

  return CGUIWindow::OnAction(action);
}

/*!
 \brief Handle messages on window.
 \param message GUI Message that can be reacted on.
 \return if a message can't be processed, return \e false
 
 On these messages this class reacts.\n
 When retrieving...
  - #GUI_MSG_PLAYBACK_ENDED\n
   ...and...
  - #GUI_MSG_PLAYBACK_STOPPED\n
   ...it deselects the current playing item in list/thumb control,
   if we are in a temporary playlist or in playlistwindow
  - #GUI_MSG_PLAYLIST_PLAY_NEXT_PREV\n
   ...the next playing item is set in list/thumb control
  - #GUI_MSG_DVDDRIVE_EJECTED_CD\n
   ...it will look, if m_strDirectory contains a path from a DVD share.
   If it is, Update() is called with a empty directory.
  - #GUI_MSG_DVDDRIVE_CHANGED_CD\n
   ...and m_strDirectory is empty, Update is called to renew icons after
   disc is changed.
  - #GUI_MSG_WINDOW_DEINIT\n
   ...the last focused control is saved to m_iLastControl.
  - #GUI_MSG_WINDOW_INIT\n
   ...the musicdatabase is opend and the music extensions and shares are set.
   The last focused control is set.
  - #GUI_MSG_CLICKED\n
   ... the base class reacts on the following controls:\n
    Buttons:\n
    - #CONTROL_BTNVIEWASICONS - switch between list, thumb and with large items
    - #CONTROL_BTNTYPE - switch between music windows
    - #CONTROL_BTNSEARCH - Search for items\n
    Other Controls:
    - #CONTROL_LIST and #CONTROL_THUMB\n
     Have the following actions in message them clicking on them.
     - #ACTION_QUEUE_ITEM - add selected item to playlist
     - #ACTION_SHOW_INFO - retrieve album info from the internet
     - #ACTION_SELECT_ITEM - Item has been selected. Overwrite OnClick() to react on it
 */
bool CGUIWindowMusicBase::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_PLAYBACK_STARTED:
    {
      UpdateButtons();
    }
    break;

  case GUI_MSG_PLAYBACK_ENDED:
  case GUI_MSG_PLAYBACK_STOPPED:
  case GUI_MSG_PLAYLISTPLAYER_STOPPED:
    {
      CStdString strDirectory = m_Directory.m_strPath;
      if (CUtil::HasSlashAtEnd(strDirectory))
        strDirectory.Delete(strDirectory.size() - 1);
      if ((m_nTempPlayListWindow == GetID() && m_strTempPlayListDirectory == strDirectory)
          || (GetID() == WINDOW_MUSIC_PLAYLIST) )
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
      CStdString strDirectory = m_Directory.m_strPath;
      if (CUtil::HasSlashAtEnd(strDirectory))
        strDirectory.Delete(strDirectory.size() - 1);
      if ((nCurrentPlaylist == PLAYLIST_MUSIC_TEMP && m_nTempPlayListWindow == GetID() && m_strTempPlayListDirectory == strDirectory )
          || (GetID() == WINDOW_MUSIC_PLAYLIST && nCurrentPlaylist == PLAYLIST_MUSIC))
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

        int nFolderCount = m_vecItems.GetFolderCount();

        // is the previous item in this directory
        for (int i = nFolderCount, n = 0; i < m_vecItems.Size(); i++)
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
              if (!pItem->IsPlayList() && !pItem->IsNFO())
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

  case GUI_MSG_DVDDRIVE_EJECTED_CD:
    {
      if ( !m_Directory.IsVirtualDirectoryRoot() )
      {
        if (m_Directory.IsCDDA() || m_Directory.IsDVD() || m_Directory.IsISO9660())
        {
          // Disc has changed and we are inside a DVD Drive share, get out of here :)
          Update("");
        }
      }
      else
      {
        int iItem = m_viewControl.GetSelectedItem();
        Update(m_Directory.m_strPath);
        m_viewControl.SetSelectedItem(iItem);
      }
    }
    break;

  case GUI_MSG_DVDDRIVE_CHANGED_CD:
    {
      if (m_Directory.IsVirtualDirectoryRoot())
      {
        int iItem = m_viewControl.GetSelectedItem();
        Update(m_Directory.m_strPath);
        m_viewControl.SetSelectedItem(iItem);
      }
    }
    break;

  case GUI_MSG_WINDOW_DEINIT:
    {
      m_nSelectedItem = m_viewControl.GetSelectedItem();
      m_iLastControl = GetFocusedControl();
      ClearFileItems();
      g_musicDatabase.Close();
      if (m_bSectionsLoaded)
      {
        CSectionLoader::Unload("LIBID3");
//        CSectionLoader::Unload("LIBMP4");
        m_bSectionsLoaded=false;
      }
    }
    break;

  case GUI_MSG_WINDOW_INIT:
    {
      int iLastControl = m_iLastControl;
      CGUIWindow::OnMessage(message);

      CSectionLoader::Load("LIBID3");
//      CSectionLoader::Load("LIBMP4");
      m_bSectionsLoaded=true;

      g_musicDatabase.Open();

      m_dlgProgress = (CGUIDialogProgress*)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);

      m_rootDir.SetMask(g_stSettings.m_szMyMusicExtensions);
      m_rootDir.SetShares(g_settings.m_vecMyMusicShares);

      Update(m_Directory.m_strPath);

      if (iLastControl > -1)
      {
        SET_CONTROL_FOCUS(iLastControl, 0);
      }
      else
      {
        SET_CONTROL_FOCUS(m_dwDefaultFocusControlID, 0);
      }

      if (m_nSelectedItem > -1)
      {
        m_viewControl.SetSelectedItem(m_nSelectedItem);
      }
      return true;
    }
    break;

  case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();
      if (iControl == CONTROL_BTNVIEWASICONS)
      {
        if ( m_Directory.IsVirtualDirectoryRoot() )
        {
          m_iViewAsIconsRoot++;
          if (m_iViewAsIconsRoot > VIEW_AS_LARGE_ICONS) m_iViewAsIconsRoot = VIEW_AS_LIST;
        }
        else
        {
          m_iViewAsIcons++;
          if (m_iViewAsIcons > VIEW_AS_LARGE_ICONS) m_iViewAsIcons = VIEW_AS_LIST;
        }
        UpdateButtons();
      }
      else if (iControl == CONTROL_BTNTYPE)
      {
        CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_BTNTYPE);
        m_gWindowManager.SendMessage(msg);

        int nWindow = WINDOW_MUSIC_FILES + msg.GetParam1();

        if (nWindow == GetID())
          return true;

        g_stSettings.m_iMyMusicStartWindow = nWindow;
        g_settings.Save();
        m_gWindowManager.ActivateWindow(g_stSettings.m_iMyMusicStartWindow);

        CGUIMessage msg2(GUI_MSG_SETFOCUS, g_stSettings.m_iMyMusicStartWindow, CONTROL_BTNTYPE);
        g_graphicsContext.SendMessage(msg2);

        return true;
      }
      else if (iControl == CONTROL_BTNSEARCH)
      {
        OnSearch();
      }
      else if (m_viewControl.HasControl(iControl))  // list/thumb control
      {
        int iItem = m_viewControl.GetSelectedItem();
        int iAction = message.GetParam1();

        // iItem is checked for validity inside these routines
        if (iAction == ACTION_QUEUE_ITEM || iAction == ACTION_MOUSE_MIDDLE_CLICK)
        {
          OnQueueItem(iItem);
        }
        else if (iAction == ACTION_SELECT_ITEM || iAction == ACTION_MOUSE_LEFT_CLICK)
        {
          OnClick(iItem);
        }
        else if (iAction == ACTION_SHOW_INFO)
        {
          OnInfo(iItem);
        }
        else if (iAction == ACTION_CONTEXT_MENU || iAction == ACTION_MOUSE_RIGHT_CLICK)
        {
          OnPopupMenu(iItem);
        }
        else if (iAction == ACTION_DELETE_ITEM)
        {
          // is delete allowed?
          // must be at the playlists directory
          CStdString strDirectory;
          strDirectory.Format("%s\\playlists", g_stSettings.m_szAlbumDirectory);
          if (strDirectory.Equals(m_Directory.m_strPath))
            OnDeleteItem(iItem);

          // or be at the files window and have file deletion enabled
          else if (GetID() == WINDOW_MUSIC_FILES && g_guiSettings.GetBool("MusicFiles.AllowFileDeletion"))
            OnDeleteItem(iItem);

          else
            return false;
        }
        // use play button to add folders of items to temp playlist
        else if (iAction == ACTION_PLAYER_PLAY)
        {
          // if playback is paused or playback speed != 1, return
          if (g_application.IsPlayingAudio())
          {
            if (g_application.m_pPlayer->IsPaused()) return false;
            if (g_application.GetPlaySpeed() != 1) return false;
          }

          // not playing audio, or playback speed == 1
          PlayItem(iItem);
        }
      }
    }
  case GUI_MSG_SETFOCUS:
    {
      if (m_viewControl.HasControl(message.GetControlId()) && m_viewControl.GetCurrentControl() != message.GetControlId())
      {
        m_viewControl.SetFocused();
        return true;
      }
    }
  }
  return CGUIWindow::OnMessage(message);
}

void CGUIWindowMusicBase::OnWindowLoaded()
{
  CGUIWindow::OnWindowLoaded();
  // add the view controls to our view controller
  m_viewControl.Reset();
  m_viewControl.SetParentWindow(GetID());
  m_viewControl.AddView(VIEW_AS_LIST, GetControl(CONTROL_LIST));
  m_viewControl.AddView(VIEW_AS_ICONS, GetControl(CONTROL_THUMBS));
  m_viewControl.AddView(VIEW_AS_LARGE_ICONS, GetControl(CONTROL_THUMBS));
  m_viewControl.AddView(VIEW_AS_LARGE_LIST, GetControl(CONTROL_BIGLIST));
  m_viewControl.SetViewControlID(CONTROL_BTNVIEWASICONS);
}

/// \brief Remove items from list/thumb control and \e m_vecItems.
void CGUIWindowMusicBase::ClearFileItems()
{
  m_viewControl.Clear();
  m_vecItems.Clear(); // will clean up everything
}

/// \brief Updates list/thumb control
/// Sets item labels (text and thumbs), sorts items and adds them to the control
void CGUIWindowMusicBase::UpdateListControl()
{
  // Cache available album thumbs
  g_directoryCache.InitMusicThumbCache();

  for (int i = 0; i < m_vecItems.Size(); i++)
  {
    CFileItem* pItem = m_vecItems[i];

    // Format label for listcontrol
    // and set thumb/icon for item
    OnFileItemFormatLabel(pItem);
  }

  g_directoryCache.ClearMusicThumbCache();

  DoSort(m_vecItems);

  m_viewControl.SetItems(m_vecItems);
}

/// \brief Set window to a specific directory
/// \param strDirectory The directory to be displayed in list/thumb control
void CGUIWindowMusicBase::Update(const CStdString &strDirectory)
{
  // get selected item
  int iItem = m_viewControl.GetSelectedItem();
  CStdString strSelectedItem = "";
  if (iItem >= 0 && iItem < m_vecItems.Size())
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

  RetrieveMusicInfo();

  UpdateListControl();
  UpdateButtons();

  strSelectedItem = m_history.Get(m_Directory.m_strPath);

  int iCurrentPlaylistSong = -1;
  // Search current playlist item
  CStdString strCurrentDirectory = m_Directory.m_strPath;
  if (CUtil::HasSlashAtEnd(strCurrentDirectory))
    strCurrentDirectory.Delete(strCurrentDirectory.size() - 1);
  if ((m_nTempPlayListWindow == GetID() && m_strTempPlayListDirectory == strCurrentDirectory && g_application.IsPlayingAudio()
       && g_playlistPlayer.GetCurrentPlaylist() == PLAYLIST_MUSIC_TEMP)
      || (GetID() == WINDOW_MUSIC_PLAYLIST && g_playlistPlayer.GetCurrentPlaylist() == PLAYLIST_MUSIC && g_application.IsPlayingAudio()) )
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
}

/// \brief Call to go to parent folder
void CGUIWindowMusicBase::GoParentFolder()
{
  CURL url(m_Directory.m_strPath);
  if ((url.GetProtocol() == "rar") || (url.GetProtocol() == "zip")) 
  {
    // check for step-below, if, unmount rar
    if (url.GetFileName().IsEmpty())
    {
      if (url.GetProtocol() == "zip")
        g_ZipManager.release(m_Directory.m_strPath); // release resources
      m_rootDir.RemoveShare(m_Directory.m_strPath);
      CStdString strPath;
      CUtil::GetDirectory(url.GetHostName(),strPath);
      Update(strPath);
      return;
    }
  }
  CStdString strPath(m_strParentPath), strOldPath(m_Directory.m_strPath);
  Update(strPath);

  if (!g_guiSettings.GetBool("LookAndFeel.FullDirectoryHistory"))
    m_history.Remove(strOldPath); //Delete current path
}

/// \brief Tests if a network/removeable share is available
/// \param strPath Root share to go into
/// \param iDriveType If share is remote, dvd or hd. See: CShare
/// \return If drive is available, returns \e true
/// \todo Handle not connected to a remote share
bool CGUIWindowMusicBase::HaveDiscOrConnection( CStdString& strPath, int iDriveType )
{
  if ( iDriveType == SHARE_TYPE_DVD )
  {
    CDetectDVDMedia::WaitMediaReady();
    if ( !CDetectDVDMedia::IsDiscInDrive() )
    {
      CGUIDialogOK::ShowAndGetInput(218, 219, 0, 0);
      // Update listcontrol, maybe share
      // was selected while disc change
      int iItem = m_viewControl.GetSelectedItem();
      Update( m_Directory.m_strPath );
      m_viewControl.SetSelectedItem(iItem);
      return false;
    }
  }
  else if (iDriveType == SHARE_TYPE_REMOTE)
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

/// \brief Retrieves music info for albums from allmusic.com and displays them in CGUIWindowMusicInfo
/// \param iItem Item in list/thumb control
void CGUIWindowMusicBase::OnInfo(int iItem)
{
  if ( iItem < 0 || iItem >= m_vecItems.Size() ) return ;
  CFileItem* pItem;
  pItem = m_vecItems[iItem];
  if (pItem->m_bIsFolder && pItem->GetLabel() == "..") return ;

  // show dialog box indicating we're searching the album name
  if (m_dlgProgress)
  {
    m_dlgProgress->SetHeading(185);
    m_dlgProgress->SetLine(0, 501);
    m_dlgProgress->SetLine(1, "");
    m_dlgProgress->SetLine(2, "");
    m_dlgProgress->StartModal(GetID());
    m_dlgProgress->Progress();
  }

  CStdString strPath;
  if (pItem->m_bIsFolder)
  {
    strPath = pItem->m_strPath;
    if (CUtil::HasSlashAtEnd(strPath))
      strPath.Delete(strPath.size() - 1);
  }
  else
  {
    CUtil::GetDirectory(pItem->m_strPath, strPath);
  }

  // Try to find an album name for this item.
  // Only save to database, if album name is found there.
  VECALBUMS albums;
  bool bSaveDb = false;
  bool bSaveDirThumb = false;
  CStdString strLabel = pItem->GetLabel();

  CAlbum album;
  if (pItem->m_musicInfoTag.Loaded())
  {
    CStdString strAlbum = pItem->m_musicInfoTag.GetAlbum();
    if (!strAlbum.IsEmpty())
      strLabel = strAlbum;

    if (g_musicDatabase.GetAlbumsByPath(strPath, albums))
    {
      if (albums.size() == 1)
        bSaveDirThumb = true;

      bSaveDb = true;
    }
    else if (!pItem->m_bIsFolder) // handle files
    {
      set<CStdString> albums;

      // Get album names found in directory
      for (int i = 0; i < m_vecItems.Size(); i++)
      {
        CFileItem* pItem = m_vecItems[i];
        if (pItem->m_musicInfoTag.Loaded() && !pItem->m_musicInfoTag.GetAlbum().IsEmpty())
        {
          CStdString strAlbum = pItem->m_musicInfoTag.GetAlbum();
          albums.insert(strAlbum);
        }
      }

      // the only album in this directory?
      if (albums.size() == 1)
      {
        CStdString strAlbum = *albums.begin();
        strLabel = strAlbum;
        bSaveDirThumb = true;
      }
    }
  }
  else if (pItem->m_bIsFolder && g_musicDatabase.GetAlbumsByPath(strPath, albums))
  { // Normal folder, query database for albums in this directory

    if (albums.size() == 1)
    {
      CAlbum& album = albums[0];
      strLabel = album.strAlbum;
      bSaveDirThumb = true;
    }
    else
    {
      // More then one album is found in this directory
      // let the user choose
      CGUIDialogSelect *pDlg = (CGUIDialogSelect*)m_gWindowManager.GetWindow(WINDOW_DIALOG_SELECT);
      if (pDlg)
      {
        pDlg->SetHeading(181);
        pDlg->Reset();
        pDlg->EnableButton(false);

        for (int i = 0; i < (int)albums.size(); ++i)
        {
          CAlbum& album = albums[i];
          pDlg->Add(album.strAlbum);
        }
        pDlg->Sort();
        pDlg->DoModal(GetID());

        // and wait till user selects one
        int iSelectedAlbum = pDlg->GetSelectedLabel();
        if (iSelectedAlbum < 0)
        {
          if (m_dlgProgress) m_dlgProgress->Close();
          return ;
        }

        strLabel = pDlg->GetSelectedLabelText();
      }
    }

    bSaveDb = true;
  }
  else if (pItem->m_bIsFolder)
  {
    // No album name found for folder found in database. Look into
    // the directory, but don't save albuminfo to database.
    CFileItemList items;
    GetDirectory(strPath, items);
    OnRetrieveMusicInfo(items);

    set<CStdString> albums;

    // Get album names found in directory
    for (int i = 0; i < items.Size(); i++)
    {
      CFileItem* pItem = items[i];
      if (pItem->m_musicInfoTag.Loaded() && !pItem->m_musicInfoTag.GetAlbum().IsEmpty())
      {
        CStdString strAlbum = pItem->m_musicInfoTag.GetAlbum();
        if (!strAlbum.IsEmpty())
          albums.insert(strAlbum);
      }
    }

    // no album found in folder use the
    // item label, we may find something?
    if (albums.size() == 0)
    {
      if (m_dlgProgress) m_dlgProgress->Close();
      bSaveDirThumb = true;
    }

    if (albums.size() == 1)
    {
      CStdString strAlbum = *albums.begin();
      strLabel = strAlbum;
      bSaveDirThumb = true;
    }

    if (albums.size() > 1)
    {
      // More then one album is found in this directory
      // let the user choose
      CGUIDialogSelect *pDlg = (CGUIDialogSelect*)m_gWindowManager.GetWindow(WINDOW_DIALOG_SELECT);
      if (pDlg)
      {
        pDlg->SetHeading(181);
        pDlg->Reset();
        pDlg->EnableButton(false);

        for (set<CStdString>::iterator it = albums.begin(); it != albums.end(); it++)
        {
          CStdString strAlbum = *it;
          pDlg->Add(strAlbum);
        }
        pDlg->Sort();
        pDlg->DoModal(GetID());

        // and wait till user selects one
        int iSelectedAlbum = pDlg->GetSelectedLabel();
        if (iSelectedAlbum < 0)
        {
          if (m_dlgProgress) m_dlgProgress->Close();
          return ;
        }

        strLabel = pDlg->GetSelectedLabelText();
      }
    }
  }
  else
  {
    // single file, not in database
    // get correct tag parser
    CMusicInfoTagLoaderFactory factory;
    auto_ptr<IMusicInfoTagLoader> pLoader (factory.CreateLoader(pItem->m_strPath));
    if (NULL != pLoader.get())
    {
      // get id3tag
      CMusicInfoTag& tag = pItem->m_musicInfoTag;
      if ( pLoader->Load(pItem->m_strPath, tag))
      {
        // get album
        CStdString strAlbum = tag.GetAlbum();
        if (!strAlbum.IsEmpty())
        {
          strLabel = strAlbum;
        }
      }
    }
  }

  if (m_dlgProgress) m_dlgProgress->Close();

  ShowAlbumInfo(strLabel, strPath, bSaveDb, bSaveDirThumb, false);
}

void CGUIWindowMusicBase::ShowAlbumInfo(const CStdString& strAlbum, const CStdString& strPath, bool bSaveDb, bool bSaveDirThumb, bool bRefresh)
{
  bool bUpdate = false;
  // check cache
  CAlbum albuminfo;
  VECSONGS songs;
  if (!bRefresh && g_musicDatabase.GetAlbumInfo(strAlbum, strPath, albuminfo, songs))
  {
    vector<CMusicSong> vecSongs;
    for (int i = 0; i < (int)songs.size(); i++)
    {
      CSong& song = songs[i];

      CMusicSong musicSong(song.iTrack, song.strTitle, song.iDuration);
      vecSongs.push_back(musicSong);
    }

    CMusicAlbumInfo album;
    album.Set(albuminfo);
    album.SetSongs(vecSongs);

    CGUIWindowMusicInfo *pDlgAlbumInfo = (CGUIWindowMusicInfo*)m_gWindowManager.GetWindow(WINDOW_MUSIC_INFO);
    if (pDlgAlbumInfo)
    {
      pDlgAlbumInfo->SetAlbum(album);
      pDlgAlbumInfo->DoModal(GetID());

      if (!pDlgAlbumInfo->NeedRefresh()) return ;
      bRefresh = true;
    }
  }

  // If we are scanning for music info in the background,
  // other writing access to the database is prohibited.
  CGUIDialogMusicScan* dlgMusicScan = (CGUIDialogMusicScan*)m_gWindowManager.GetWindow(WINDOW_DIALOG_MUSIC_SCAN);
  if (dlgMusicScan->IsRunning())
  {
    CGUIDialogOK::ShowAndGetInput(189, 14057, 0, 0);
    return;
  }

  // find album info
  CMusicAlbumInfo album;
  if (FindAlbumInfo(strAlbum, album))
  {
    // download the album info
    bool bLoaded = album.Loaded();
    if ( bLoaded )
    {
      // set album title from musicinfotag, not the one we got from allmusic.com
      album.SetTitle(strAlbum);
      // set path, needed to store album in database
      album.SetAlbumPath(strPath);

      if (bSaveDb)
      {
        CAlbum albuminfo;
        albuminfo.strAlbum = album.GetTitle();
        albuminfo.strArtist = album.GetArtist();
        albuminfo.strGenre = album.GetGenre();
        albuminfo.strTones = album.GetTones();
        albuminfo.strStyles = album.GetStyles();
        albuminfo.strReview = album.GetReview();
        albuminfo.strImage = album.GetImageURL();
        albuminfo.iRating = album.GetRating();
        albuminfo.iYear = atol( album.GetDateOfRelease().c_str() );
        albuminfo.strPath = album.GetAlbumPath();

        for (int i = 0; i < (int)album.GetNumberOfSongs(); i++)
        {
          CMusicSong musicSong = album.GetSong(i);

          CSong song;
          song.iTrack = musicSong.GetTrack();
          song.strTitle = musicSong.GetSongName();
          song.iDuration = musicSong.GetDuration();

          songs.push_back(song);
        }

        // save to database
        if (bRefresh)
          g_musicDatabase.UpdateAlbumInfo(albuminfo, songs);
        else
          g_musicDatabase.AddAlbumInfo(albuminfo, songs);
      }
      if (m_dlgProgress)
        m_dlgProgress->Close();

      // ok, show album info
      CGUIWindowMusicInfo *pDlgAlbumInfo = (CGUIWindowMusicInfo*)m_gWindowManager.GetWindow(WINDOW_MUSIC_INFO);
      if (pDlgAlbumInfo)
      {
        pDlgAlbumInfo->SetAlbum(album);
        pDlgAlbumInfo->DoModal(GetID());

        // Save directory thumb
        if (bSaveDirThumb)
        {
          CStdString strThumb;
          CUtil::GetAlbumThumb(album.GetTitle(), album.GetAlbumPath(), strThumb);
          // Was the download of the album art
          // from allmusic.com successfull...
          if (CFile::Exists(strThumb))
          {
            // ...yes...
            CFileItem item(album.GetAlbumPath(), true);
            if (!item.IsCDDA())
            {
              // ...also save a copy as directory thumb,
              // if the album isn't located on an audio cd
              CStdString strFolderThumb;
              CUtil::GetAlbumFolderThumb(album.GetAlbumPath(), strFolderThumb);
              ::CopyFile(strThumb, strFolderThumb, false);
            }
          }
        }
        if (pDlgAlbumInfo->NeedRefresh())
        {
          ShowAlbumInfo(strAlbum, strPath, bSaveDb, bSaveDirThumb, true);
          return ;
        }
      }
      bUpdate = true;
    }
    else
    {
      // failed 2 download album info
      CGUIDialogOK::ShowAndGetInput(185, 0, 500, 0);
    }
  }

  if (bUpdate)
  {
    int iSelectedItem = m_viewControl.GetSelectedItem();
    if (iSelectedItem >= 0 && m_vecItems[iSelectedItem] && m_vecItems[iSelectedItem]->m_bIsFolder)
    {
      // refresh only the icon of
      // the current folder
      m_vecItems[iSelectedItem]->FreeIcons();
      m_vecItems[iSelectedItem]->SetMusicThumb();
      m_vecItems[iSelectedItem]->FillInDefaultIcon();
    }
    else
    {
      // Refresh all items
      for (int i = 0; i < m_vecItems.Size(); ++i)
      {
        CFileItem* pItem = m_vecItems[i];
        pItem->FreeIcons();
      }

      m_vecItems.SetMusicThumbs();
      m_vecItems.FillInDefaultIcons();
    }

    // HACK: If we are in files view
    // autoswitch between list/thumb control
    if (GetID() == WINDOW_MUSIC_FILES && !m_Directory.IsVirtualDirectoryRoot() && g_guiSettings.GetBool("MusicFiles.UseAutoSwitching"))
    {
      m_iViewAsIcons = CAutoSwitch::GetView(m_vecItems);
      m_viewControl.SetCurrentView(m_iViewAsIcons);
      UpdateButtons();
    }
  }

  if (m_dlgProgress)
    m_dlgProgress->Close();

}

/// \brief Can be overwritten to implement an own tag filling function.
/// \param items File items to fill
void CGUIWindowMusicBase::OnRetrieveMusicInfo(CFileItemList& items)
{
}

/// \brief Retrieve tag information for \e m_vecItems
void CGUIWindowMusicBase::RetrieveMusicInfo()
{
  DWORD dwTick = timeGetTime();

  OnRetrieveMusicInfo(m_vecItems);

  dwTick = timeGetTime() - dwTick;
  CStdString strTmp;
  strTmp.Format("RetrieveMusicInfo() took %imsec\n", dwTick);
  OutputDebugString(strTmp.c_str());
}

/// \brief Add selected list/thumb control item to playlist and start playing
/// \param iItem Selected Item in list/thumb control
void CGUIWindowMusicBase::OnQueueItem(int iItem)
{
  if ( iItem < 0 || iItem >= m_vecItems.Size() ) return ;

  int iOldSize=g_playlistPlayer.GetPlaylist(PLAYLIST_MUSIC).size();

  // add item 2 playlist
  const CFileItem* pItem = m_vecItems[iItem];
  AddItemToPlayList(pItem);

  //move to next item
  m_viewControl.SetSelectedItem(iItem + 1);
  if (g_playlistPlayer.GetPlaylist(PLAYLIST_MUSIC).size() && !g_application.IsPlayingAudio() )
  {
    g_playlistPlayer.Reset();
    g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_MUSIC);
    if (g_playlistPlayer.ShuffledPlay(PLAYLIST_MUSIC))
      g_playlistPlayer.Play();
    else
      g_playlistPlayer.Play(iOldSize);  //  Start playlist with the first new song added
  }
}

/// \brief Add file or folder and its subfolders to playlist
/// \param pItem The file item to add
void CGUIWindowMusicBase::AddItemToPlayList(const CFileItem* pItem)
{
  if (pItem->m_bIsFolder)
  {
    // Check if we add a locked share
    if ( pItem->m_bIsShareOrDrive )
    {
      CFileItem item = *pItem;
      if ( !g_passwordManager.IsItemUnlocked( &item, "music" ) )
        return ;
    }

    // recursive
    if (pItem->GetLabel() == "..") return ;
    CStdString strDirectory = m_Directory.m_strPath;
    m_Directory.m_strPath = pItem->m_strPath;
    CFileItemList items;
    GetDirectory(m_Directory.m_strPath, items);
    DoSort(items);
    for (int i = 0; i < items.Size(); ++i)
      AddItemToPlayList(items[i]);
    m_Directory.m_strPath = strDirectory;
  }
  else
  {
    if (!pItem->IsNFO() && pItem->IsAudio() && !pItem->IsPlayList())
    {
      CPlayList::CPlayListItem playlistItem;
      CUtil::ConvertFileItemToPlayListItem(pItem, playlistItem);
      g_playlistPlayer.GetPlaylist(PLAYLIST_MUSIC).Add(playlistItem);
    }
  }
}

/// \brief Make the actual search for the OnSearch function.
/// \param strSearch The search string
/// \param items Items Found
void CGUIWindowMusicBase::DoSearch(const CStdString& strSearch, CFileItemList& items)
{
}

/// \brief Search the current directory for a string got from the virtual keyboard
void CGUIWindowMusicBase::OnSearch()
{
  CStdString strSearch;
  if ( !CGUIDialogKeyboard::ShowAndGetInput(strSearch, false) )
    return ;

  strSearch.ToLower();
  if (m_dlgProgress)
  {
    m_dlgProgress->SetHeading(194);
    m_dlgProgress->SetLine(0, strSearch);
    m_dlgProgress->SetLine(1, L"");
    m_dlgProgress->SetLine(2, L"");
    m_dlgProgress->StartModal(GetID());
    m_dlgProgress->Progress();
  }
  CFileItemList items;
  DoSearch(strSearch, items);

  if (items.Size())
  {
    CGUIDialogSelect* pDlgSelect = (CGUIDialogSelect*)m_gWindowManager.GetWindow(WINDOW_DIALOG_SELECT);
    pDlgSelect->Reset();
    pDlgSelect->SetHeading(283);
    CUtil::SortFileItemsByName(items);

    for (int i = 0; i < (int)items.Size(); i++)
    {
      CFileItem* pItem = items[i];
      pDlgSelect->Add(pItem->GetLabel());
    }

    pDlgSelect->DoModal(GetID());

    int iItem = pDlgSelect->GetSelectedLabel();
    if (iItem < 0)
    {
      if (m_dlgProgress) m_dlgProgress->Close();
      return ;
    }

    CFileItem* pSelItem = new CFileItem(*items[iItem]);

    OnSearchItemFound(pSelItem);

    delete pSelItem;
    if (m_dlgProgress) m_dlgProgress->Close();
  }
  else
  {
    if (m_dlgProgress) m_dlgProgress->Close();
    CGUIDialogOK::ShowAndGetInput(194, 284, 0, 0);
  }
}

/// \brief Can be overwritten to build an own history string for \c m_history
/// \param pItem Item to build the history string from
/// \param strHistoryString History string build as return value
void CGUIWindowMusicBase::GetDirectoryHistoryString(const CFileItem* pItem, CStdString& strHistoryString)
{
  strHistoryString = pItem->m_strPath;

  if (CUtil::HasSlashAtEnd(strHistoryString))
    strHistoryString.Delete(strHistoryString.size() - 1);
}

void CGUIWindowMusicBase::UpdateButtons()
{
  // Update window selection control

  // Remove labels from the window selection
  CGUIMessage msg(GUI_MSG_LABEL_RESET, GetID(), CONTROL_BTNTYPE);
  g_graphicsContext.SendMessage(msg);

  // Add labels to the window selection
  CStdString strItem = g_localizeStrings.Get(744); // Files
  CGUIMessage msg2(GUI_MSG_LABEL_ADD, GetID(), CONTROL_BTNTYPE);
  msg2.SetLabel(strItem);
  g_graphicsContext.SendMessage(msg2);

  strItem = g_localizeStrings.Get(15100); // Library
  msg2.SetLabel(strItem);
  g_graphicsContext.SendMessage(msg2);

  strItem = g_localizeStrings.Get(271); // Top 100
  msg2.SetLabel(strItem);
  g_graphicsContext.SendMessage(msg2);

  // Select the current window as default item
  CONTROL_SELECT_ITEM(CONTROL_BTNTYPE, g_stSettings.m_iMyMusicStartWindow - WINDOW_MUSIC_FILES);
}

/// \brief React on the selected search item
/// \param pItem Search result item
void CGUIWindowMusicBase::OnSearchItemFound(const CFileItem* pItem)
{
}

bool CGUIWindowMusicBase::FindAlbumInfo(const CStdString& strAlbum, CMusicAlbumInfo& album)
{
  // quietly return if Internet lookups are disabled
  if (!g_guiSettings.GetBool("Network.EnableInternet")) return false;

  // show dialog box indicating we're searching the album
  if (m_dlgProgress)
  {
    m_dlgProgress->SetHeading(185);
    m_dlgProgress->SetLine(0, strAlbum);
    m_dlgProgress->SetLine(1, "");
    m_dlgProgress->SetLine(2, "");
    m_dlgProgress->StartModal(GetID());
  }

  try
  {
    CMusicInfoScraper scraper;
    scraper.FindAlbuminfo(strAlbum);

    while (!scraper.Completed())
    {
      if (m_dlgProgress)
      {
        if (m_dlgProgress->IsCanceled())
          scraper.Cancel();
        m_dlgProgress->Progress();
      }
    }

    if (scraper.Successfull())
    {
      // did we found at least 1 album?
      int iAlbumCount = scraper.GetAlbumCount();
      if (iAlbumCount >= 1)
      {
        //yes
        // if we found more then 1 album, let user choose one
        int iSelectedAlbum = 0;
        if (iAlbumCount > 1)
        {
          //show dialog with all albums found
          const WCHAR* szText = g_localizeStrings.Get(181).c_str();
          CGUIDialogSelect *pDlg = (CGUIDialogSelect*)m_gWindowManager.GetWindow(WINDOW_DIALOG_SELECT);
          if (pDlg)
          {
            pDlg->SetHeading(szText);
            pDlg->Reset();
            pDlg->EnableButton(true);
            pDlg->SetButtonLabel(413); // manual

            for (int i = 0; i < iAlbumCount; ++i)
            {
              CMusicAlbumInfo& info = scraper.GetAlbum(i);
              pDlg->Add(info.GetTitle2());
            }
            pDlg->DoModal(GetID());

            // and wait till user selects one
            iSelectedAlbum = pDlg->GetSelectedLabel();
            if (iSelectedAlbum < 0)
            {
              if (!pDlg->IsButtonPressed()) return false;
              CStdString strNewAlbum = strAlbum;
              if (!CGUIDialogKeyboard::ShowAndGetInput(strNewAlbum, false)) return false;
              if (strNewAlbum == "") return false;
              if (m_dlgProgress)
              {
                m_dlgProgress->SetLine(0, strNewAlbum);
                m_dlgProgress->Progress();
              }

              return FindAlbumInfo(strNewAlbum, album);
            }
          }
        }

        // ok, downloading the album info
        scraper.LoadAlbuminfo(iSelectedAlbum);

        while (!scraper.Completed())
        {
          if (m_dlgProgress)
          {
            if (m_dlgProgress->IsCanceled())
              scraper.Cancel();
            m_dlgProgress->Progress();
          }
        }

        if (scraper.Successfull())
          album = scraper.GetAlbum(iSelectedAlbum);

        return scraper.Successfull();
      }
      else
      { // no albums found
        CGUIDialogOK::ShowAndGetInput(185, 0, 187, 0);
      }
    }
    
    if (!scraper.IsCanceled())
    { // unable 2 connect to www.allmusic.com
      CGUIDialogOK::ShowAndGetInput(185, 0, 499, 0);
    }
  }
  catch (...)
  {
    if (m_dlgProgress && m_dlgProgress->IsRunning())
      m_dlgProgress->Close();

    CLog::Log(LOGERROR, "Exception while downloading album info");
  }
  return false;
}

void CGUIWindowMusicBase::DisplayEmptyDatabaseMessage(bool bDisplay)
{
  m_bDisplayEmptyDatabaseMessage = bDisplay;
}

void CGUIWindowMusicBase::Render()
{
  CGUIWindow::Render();
  if (m_bDisplayEmptyDatabaseMessage)
  {
    CGUIListControl *pControl = (CGUIListControl *)GetControl(CONTROL_LIST);
    int iX = pControl->GetXPosition() + pControl->GetWidth() / 2;
    int iY = pControl->GetYPosition() + pControl->GetHeight() / 2;
    CGUIFont *pFont = g_fontManager.GetFont(pControl->GetFontName());
    if (pFont)
    {
      float fWidth, fHeight;
      CStdStringW wszText = g_localizeStrings.Get(745); // "No scanned information for this view"
      CStdStringW wszText2 = g_localizeStrings.Get(746); // "Switch back to Files view"
      pFont->GetTextExtent(wszText, &fWidth, &fHeight);
      pFont->DrawText((float)iX, (float)iY - fHeight, 0xffffffff, wszText.c_str(), XBFONT_CENTER_X | XBFONT_CENTER_Y);
      pFont->DrawText((float)iX, (float)iY + fHeight, 0xffffffff, wszText2.c_str(), XBFONT_CENTER_X | XBFONT_CENTER_Y);
    }
  }
}

void CGUIWindowMusicBase::OnPopupMenu(int iItem)
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
  // initialize the menu (loaded on demand)
  pMenu->Initialize();
  // add the needed buttons
  int btn_Info          = pMenu->AddButton(13351);    // Music Information
  int btn_Play          = pMenu->AddButton(13358);    // Play Item
  int btn_Queue         = pMenu->AddButton(13347);    // Queue Item
  int btn_Playlist      = pMenu->AddButton(13350);    // Now Playing...

  int btn_Scan = 0;
  CGUIDialogMusicScan *pScanDlg = (CGUIDialogMusicScan *)m_gWindowManager.GetWindow(WINDOW_DIALOG_MUSIC_SCAN);
  if (pScanDlg && pScanDlg->IsRunning())
    btn_Scan = pMenu->AddButton(13353);               // Stop Scanning
  else
    btn_Scan = pMenu->AddButton(13352);               // Scan Folder to Database

  int btn_Search        = pMenu->AddButton(137);      // Search...
  int btn_Rip           = pMenu->AddButton(600);      // Rip CD Audio
  int btn_CDDB          = pMenu->AddButton(16002);    // CDDB lookup
  
  int btn_Delete = 0;
  CStdString strDirectory;
  strDirectory.Format("%s\\playlists", g_stSettings.m_szAlbumDirectory);
  if (strDirectory.Equals(m_Directory.m_strPath) || g_guiSettings.GetBool("MusicFiles.AllowFileDeletion"))
    btn_Delete = pMenu->AddButton(117);               // Delete
  
  int btn_Settings      = pMenu->AddButton(5);        // Settings...

  // turn off info/queue/play if the current item is goto parent ..
  bool bIsGotoParent = m_vecItems[iItem]->GetLabel() == "..";
  if (bIsGotoParent)
  {
    pMenu->EnableButton(btn_Info, false);
    pMenu->EnableButton(btn_Play, false);
    pMenu->EnableButton(btn_Queue, false);
  }

  // turn off the now playing button if playlist is empty
  if (g_playlistPlayer.GetPlaylist(PLAYLIST_MUSIC).size() <= 0)
    pMenu->EnableButton(btn_Playlist, false);

  // turn off the Scan button if we're not in files view or a internet stream
  if (GetID() != WINDOW_MUSIC_FILES || m_Directory.IsInternetStream())
    pMenu->EnableButton(btn_Scan, false);

  // turn off Rip CD Audio button if we don't have a CDDA disk in
  CCdInfo *pCdInfo = CDetectDVDMedia::GetCdInfo();
  if (!CDetectDVDMedia::IsDiscInDrive() || !pCdInfo || !pCdInfo->IsAudio(1))
    pMenu->EnableButton(btn_Rip, false);

  // turn off CDDB lookup, if the current dir is not CDDA
  if (!m_Directory.IsCDDA())
    pMenu->EnableButton(btn_CDDB, false);

  // position it correctly
  pMenu->SetPosition(iPosX - pMenu->GetWidth() / 2, iPosY - pMenu->GetHeight() / 2);
  pMenu->DoModal(GetID());

  int btnid = pMenu->GetButton();
  if (btnid > 0)
  {
    // Music Information
    if (btnid == btn_Info) 
    {
      OnInfo(iItem);
    }
    // Play Item
    else if (btnid == btn_Play)
    {
      PlayItem(iItem);
    }
    // Queue Item
    else if (btnid == btn_Queue)
    {
      OnQueueItem(iItem);
    }
    // Now Playing...
    else if (btnid == btn_Playlist)
    {
      m_gWindowManager.ActivateWindow(WINDOW_MUSIC_PLAYLIST);
      return;
    }
    // Scan
    else if (btnid == btn_Scan)
    {
      OnScan();
    }
    // Search
    else if (btnid == btn_Search)
    {
      OnSearch();
    }
    // Rip CD...
    else if (btnid == btn_Rip)
    {
      OnRipCD();
    }
    // CDDB lookup
    else if (btnid == btn_CDDB)
    {
      if (g_musicDatabase.LookupCDDBInfo(true))
        Update(m_Directory.m_strPath);
    }
    // Delete
    else if (btnid == btn_Delete)
    {
      OnDeleteItem(iItem);
    }
    // Settings
    else if (btnid == btn_Settings)
    {
      m_gWindowManager.ActivateWindow(WINDOW_SETTINGS_MYMUSIC);
      return;
    }
  }
  m_vecItems[iItem]->Select(bSelected);
}

void CGUIWindowMusicBase::OnRipCD()
{
  CCdInfo *pCdInfo = CDetectDVDMedia::GetCdInfo();
  if (CDetectDVDMedia::IsDiscInDrive() && pCdInfo && pCdInfo->IsAudio(1))
  {
    if (!g_application.CurrentFileItem().IsCDDA())
    {
      CCDDARipper ripper;
      ripper.RipCD();
    }
    else
    {
      CGUIDialogOK* pDlgOK = (CGUIDialogOK*)m_gWindowManager.GetWindow(WINDOW_DIALOG_OK);
      pDlgOK->SetHeading(257); // Error
      pDlgOK->SetLine(0, "Can't rip CD or Track while playing from CD"); //
      pDlgOK->SetLine(1, ""); //
      pDlgOK->SetLine(2, "");
      pDlgOK->DoModal(GetID());
    }
  }
}

void CGUIWindowMusicBase::SetLabelFromTag(CFileItem *pItem)
{
  CStdString strLabel  = ParseFormat(pItem, g_guiSettings.GetString("MyMusic.TrackFormat"));
  CStdString strLabel2 = ParseFormat(pItem, g_guiSettings.GetString("MyMusic.TrackFormatRight"));

  // set label 1
  // if we don't have anything at the moment (due to empty tags),
  // we just remove the extension
  if (strLabel.size())
    pItem->SetLabel(strLabel);
  else if (g_guiSettings.GetBool("FileLists.HideExtensions"))
    pItem->RemoveExtension();

  // set label 2
  if (strLabel2.size())
    pItem->SetLabel2(strLabel2);
}

CStdString CGUIWindowMusicBase::ParseFormat(CFileItem *pItem, const CStdString& strFormat)
{
  CStdString strLabel = "";
  CMusicInfoTag& tag = pItem->m_musicInfoTag;
  int iPos1 = 0;
  int iPos2 = strFormat.Find('%', iPos1);
  bool bDoneSomething = !(iPos1 == iPos2); // stuff in front should be applied - everything using this bool is added by spiff
  while (iPos2 >= 0)
  {
    if( (iPos2 > iPos1) && bDoneSomething )
    {
      strLabel += strFormat.Mid(iPos1, iPos2 - iPos1);
      bDoneSomething = false;  
    }
    CStdString str;
    if (strFormat[iPos2 + 1] == 'N' && tag.GetTrackNumber() > 0)
    { // number
      str.Format("%02.2i", tag.GetTrackNumber());
      bDoneSomething = true;
    }
    else if (strFormat[iPos2 + 1] == 'A' && tag.GetArtist().size())
    { // artist
      str = tag.GetArtist();
      bDoneSomething = true;
    }
    else if (strFormat[iPos2 + 1] == 'T' && tag.GetTitle().size())
    { // title
      str = tag.GetTitle();
      bDoneSomething = true;
    }
    else if (strFormat[iPos2 + 1] == 'B' && tag.GetAlbum().size())
    { // album
      str = tag.GetAlbum();
      bDoneSomething = true;
    }
    else if (strFormat[iPos2 + 1] == 'G' && tag.GetGenre().size())
    { // genre
      str = tag.GetGenre();
      bDoneSomething = true;
    }
    else if (strFormat[iPos2 + 1] == 'Y')
    { // year
      str = tag.GetYear();
      bDoneSomething = true;
    }
    else if (strFormat[iPos2 + 1] == 'F')
    { // filename
      str = CUtil::GetTitleFromPath(pItem->m_strPath);
      bDoneSomething = true;
    }
    else if (strFormat[iPos2 + 1] == 'D' && tag.GetDuration() > 0)
    { // duration
      int nDuration = tag.GetDuration();
      CUtil::SecondsToHMSString(nDuration, str);
      bDoneSomething = true;
    }
    else if (strFormat[iPos2 + 1] == '%')
    { // %% to print %
      str = '%';
      bDoneSomething = true;
    }
    strLabel += str;
    iPos1 = iPos2 + 2;
    iPos2 = strFormat.Find('%', iPos1);
  }
  if (iPos1 < (int)strFormat.size())
    strLabel += strFormat.Right(strFormat.size() - iPos1);

  return strLabel;
}

void CGUIWindowMusicBase::AddItemToTempPlayList(const CFileItem* pItem)
{
  if (pItem->m_bIsFolder)
  {
    // Check if we add a locked share
    if ( pItem->m_bIsShareOrDrive )
    {
      CFileItem item = *pItem;
      if ( !g_passwordManager.IsItemUnlocked( &item, "music" ) )
        return ;
    }

    // recursive
    if (pItem->GetLabel() == "..") return ;
    CStdString strDirectory = m_Directory.m_strPath;
    m_Directory.m_strPath = pItem->m_strPath;
    CFileItemList items;
    GetDirectory(m_Directory.m_strPath, items);
    DoSort(items);
    for (int i = 0; i < items.Size(); ++i)
    {
      AddItemToTempPlayList(items[i]);
    }
    m_Directory.m_strPath = strDirectory;
  }
  else
  {
    if (!pItem->IsNFO() && pItem->IsAudio() && !pItem->IsPlayList())
    {
      CPlayList::CPlayListItem playlistItem;
      CUtil::ConvertFileItemToPlayListItem(pItem, playlistItem);
      g_playlistPlayer.GetPlaylist(PLAYLIST_MUSIC_TEMP).Add(playlistItem);
    }
  }
}

void CGUIWindowMusicBase::PlayItem(int iItem)
{
  // restrictions should be placed in the appropiate window code
  // only call the base code if the item passes since this clears
  // the currently playing temp playlist

  const CFileItem* pItem = m_vecItems[iItem];
  // if its a folder, build a temp playlist
  if (pItem->m_bIsFolder)
  {
    // skip ".."
    if (pItem->GetLabel() == "..")
      return;

    // clear current temp playlist
    g_playlistPlayer.GetPlaylist(PLAYLIST_MUSIC_TEMP).Clear();
    g_playlistPlayer.Reset();

    // recursively add items to temp playlist
    AddItemToTempPlayList(pItem);

    // play!
    g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_MUSIC_TEMP);
    g_playlistPlayer.Play();
  }
  // otherwise just play the song
  else
  {
    OnClick(iItem);
  }
}

void CGUIWindowMusicBase::OnWindowUnload()
{
  CGUIWindow::OnWindowUnload();
  m_viewControl.Reset();
}

void CGUIWindowMusicBase::OnDeleteItem(int iItem)
{
  if ( iItem < 0 || iItem >= m_vecItems.Size()) return;
  const CFileItem* pItem = m_vecItems[iItem];
  CStdString strPath = pItem->m_strPath;
  CStdString strFile = CUtil::GetFileName(strPath);
  if (pItem->m_bIsFolder)
    CUtil::GetDirectoryName(strPath, strFile);

  CGUIDialogYesNo* pDialog = (CGUIDialogYesNo*)m_gWindowManager.GetWindow(WINDOW_DIALOG_YES_NO);
  if (pDialog)
  {
    pDialog->SetHeading(122);
    pDialog->SetLine(0, 125);
    pDialog->SetLine(1, strFile.c_str());
    pDialog->SetLine(2, L"");
    pDialog->DoModal(GetID());
    if (!pDialog->IsConfirmed()) return ;
  }

  if (m_dlgProgress) m_dlgProgress->StartModal(GetID());
  if (pItem->m_bIsFolder)
  {
    CStdString strLog;
    strLog.Format("delete folder %s\n", strFile.c_str());
    OutputDebugString(strLog.c_str());

    CDirectory::Remove(strPath.c_str());
    if (m_dlgProgress)
    {
      m_dlgProgress->SetLine(0, 117);
      m_dlgProgress->SetLine(1, strFile);
      m_dlgProgress->SetLine(2, L"");
      m_dlgProgress->Progress();
    }
  }
  else
  {
    CStdString strLog;
    strLog.Format("delete %s\n", strFile.c_str());
    OutputDebugString(strLog.c_str());

    CFile::Delete(strPath.c_str());
    if (m_dlgProgress)
    {
      m_dlgProgress->SetLine(0, 117);
      m_dlgProgress->SetLine(1, strFile);
      m_dlgProgress->SetLine(2, L"");
      m_dlgProgress->Progress();
    }
  }
  if (m_dlgProgress) m_dlgProgress->Close();

  Update(m_Directory.m_strPath);
  m_viewControl.SetSelectedItem(iItem);
}