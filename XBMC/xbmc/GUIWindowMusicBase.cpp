#include "stdafx.h"
#include "GUIWindowMusicBase.h"
#include "MusicInfoTagLoaderFactory.h"
#include "GUIWindowMusicInfo.h"
#include "FileSystem/ZipManager.h"
#include "PlayListFactory.h"
#include "Util.h"
#include "PlayListM3U.h"
#include "Application.h"
#include "PlayListPlayer.h"
#include "GUIListControl.h"
#include "FileSystem/DirectoryCache.h"
#include "CDRip/CDDARipper.h"
#include "GUIPassword.h"
#include "GUIDialogMusicScan.h"
#include "GUIDialogContextMenu.h"
#include "GUIWindowFileManager.h"
#ifdef PRE_SKIN_VERSION_2_0_COMPATIBILITY
#include "SkinInfo.h"
#endif

#define CONTROL_BTNVIEWASICONS  2
#define CONTROL_BTNSORTBY       3
#define CONTROL_BTNSORTASC      4
#define CONTROL_BTNTYPE         5
#define CONTROL_LIST            50
#define CONTROL_THUMBS          51
#define CONTROL_BIGLIST         52

#define CONTROL_BTNSEARCH       8


using namespace MUSIC_GRABBER;
using namespace DIRECTORY;
using namespace PLAYLIST;

CGUIWindowMusicBase::CGUIWindowMusicBase(DWORD dwID, const CStdString &xmlFile)
    : CGUIMediaWindow(dwID, xmlFile)
{
  m_bDisplayEmptyDatabaseMessage = false;
}

CGUIWindowMusicBase::~CGUIWindowMusicBase ()
{
}

/// \brief Handle actions on window.
/// \param action Action that can be reacted on.
bool CGUIWindowMusicBase::OnAction(const CAction& action)
{
  if (action.wID == ACTION_PREVIOUS_MENU)
  {
    CGUIDialogMusicScan *musicScan = (CGUIDialogMusicScan *)m_gWindowManager.GetWindow(WINDOW_DIALOG_MUSIC_SCAN);
    if (musicScan && !musicScan->IsRunning())
    {
      CUtil::ThumbCacheClear();
      CUtil::RemoveTempFiles();
    }
  }

  if (action.wID == ACTION_SHOW_PLAYLIST)
  {
    m_gWindowManager.ActivateWindow(WINDOW_MUSIC_PLAYLIST);
    return true;
  }

  return CGUIMediaWindow::OnAction(action);
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
  case GUI_MSG_WINDOW_DEINIT:
    {
      g_musicDatabase.Close();
    }
    break;

  case GUI_MSG_WINDOW_INIT:
    {
      int iLastControl = m_iLastControl;
      CGUIWindow::OnMessage(message);

      g_musicDatabase.Open();

      m_dlgProgress = (CGUIDialogProgress*)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);

      Update(m_vecItems.m_strPath);

      if (iLastControl > -1)
      {
        SET_CONTROL_FOCUS(iLastControl, 0);
      }
      else
      {
        SET_CONTROL_FOCUS(m_dwDefaultFocusControlID, 0);
      }

      if (m_iSelectedItem > -1)
      {
        m_viewControl.SetSelectedItem(m_iSelectedItem);
      }

      // save current window, unless the current window is the music playlist window
      if (GetID() != WINDOW_MUSIC_PLAYLIST)
        g_stSettings.m_iMyMusicStartWindow = GetID();

      return true;
    }
    break;

  case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();
      if (iControl == CONTROL_BTNTYPE)
      {
        CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_BTNTYPE);
        m_gWindowManager.SendMessage(msg);

        int nWindow = WINDOW_MUSIC_FILES + msg.GetParam1();

        if (nWindow == GetID())
          return true;

        g_stSettings.m_iMyMusicStartWindow = nWindow;
        g_settings.Save();
        m_gWindowManager.ChangeActiveWindow(nWindow);

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
          if (m_vecItems.m_strPath.Equals(CUtil::MusicPlaylistsLocation()))
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
  }
  return CGUIMediaWindow::OnMessage(message);
}

void CGUIWindowMusicBase::OnWindowLoaded()
{
#ifdef PRE_SKIN_VERSION_2_0_COMPATIBILITY
  if (g_SkinInfo.GetVersion() < 1.8)
  {
    ChangeControlID(6, CONTROL_BTNTYPE, CGUIControl::GUICONTROL_SELECTBUTTON);
  }
#endif
  CGUIMediaWindow::OnWindowLoaded();
}

/// \brief Set window to a specific directory
/// \param strDirectory The directory to be displayed in list/thumb control
bool CGUIWindowMusicBase::Update(const CStdString &strDirectory)
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
    return !Update(strOldDirectory); // We assume, we can get the parent 
                                     // directory again, but we have to 
                                     // return false to be able to eg. show 
                                     // an error message.

  // if we're getting the root bookmark listing
  // make sure the path history is clean
  if (strDirectory.IsEmpty())
    m_history.ClearPathHistory();

  if (m_guiState.get() && m_guiState->HideExtensions())
    m_vecItems.RemoveExtensions();

  RetrieveMusicInfo();

  if (!m_vecItems.IsVirtualDirectoryRoot())
  {
    m_vecItems.SetMusicThumbs();
    m_vecItems.FillInDefaultIcons();
  }

  m_guiState.reset(CGUIViewState::GetViewState(GetID(), m_vecItems));
  FormatItemLabels();
  SortItems(m_vecItems);
  m_viewControl.SetItems(m_vecItems);

  UpdateButtons();

  strSelectedItem = m_history.GetSelectedItem(m_vecItems.m_strPath);

  int iCurrentPlaylistSong = -1;
  // Search current playlist item
  CStdString strCurrentDirectory = m_vecItems.m_strPath;
  if (CUtil::HasSlashAtEnd(strCurrentDirectory))
    strCurrentDirectory.Delete(strCurrentDirectory.size() - 1);
  // Is this window responsible for the current playlist?
  if (m_guiState.get() && g_playlistPlayer.GetCurrentPlaylist()==m_guiState->GetPlaylist() && strCurrentDirectory==m_guiState->GetPlaylistDirectory())
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

  m_history.DumpPathHistory();

  return true;
}

/// \brief Retrieves music info for albums from allmusic.com and displays them in CGUIWindowMusicInfo
/// \param iItem Item in list/thumb control
void CGUIWindowMusicBase::OnInfo(int iItem)
{
  if ( iItem < 0 || iItem >= m_vecItems.Size() ) return ;
  CFileItem* pItem;
  pItem = m_vecItems[iItem];
  if (pItem->m_bIsFolder && pItem->IsParentFolder()) return ;

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
  CStdString strAlbumName;
  CStdString strArtistName;

  CAlbum album;
  if (pItem->IsMusicDb())
  {
    strAlbumName=pItem->m_musicInfoTag.GetAlbum();
    strArtistName=pItem->m_musicInfoTag.GetArtist();

    if (pItem->m_bIsFolder)
    { // Extract album id from musicdb path
      int nPos=strPath.ReverseFind("/");
      if (nPos>-1)
      {
        int idAlbum=atol(strPath.Right(strPath.size()-nPos-1));
        g_musicDatabase.GetPathFromAlbumId(idAlbum, strPath);
      }
    }
    else
    { // Extract song id from musicdb file
      strPath=pItem->m_strPath;
      int nPos=strPath.ReverseFind("/");
      if (nPos>-1)
      {
        CUtil::RemoveExtension(strPath);
        int idSong=atol(strPath.Right(strPath.size()-nPos-1));
        g_musicDatabase.GetPathFromSongId(idSong, strPath);
      }
    }

    if (g_musicDatabase.GetAlbumsByPath(strPath, albums))
    {
      if (albums.size() == 1)
        bSaveDirThumb = true;
    }

    bSaveDb=true;
  }
  else if (pItem->m_musicInfoTag.Loaded())
  {
    strAlbumName = pItem->m_musicInfoTag.GetAlbum();
    strArtistName = pItem->m_musicInfoTag.GetArtist();
    if (strAlbumName.IsEmpty())
      strAlbumName = strLabel;

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
        //CStdString strAlbum = *albums.begin();
        //strLabel = strAlbum;
        bSaveDirThumb = true;
      }
    }
  }
  else if (pItem->m_bIsFolder && g_musicDatabase.GetAlbumsByPath(strPath, albums))
  { // Normal folder, query database for albums in this directory

    if (albums.size() == 1)
    {
      CAlbum& album = albums[0];
      strAlbumName = album.strAlbum;
      strArtistName = album.strArtist;
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
          CStdString strTemp = album.strAlbum;
          if (!album.strArtist.IsEmpty())
            strTemp += " - " + album.strArtist;
          pDlg->Add(strTemp);
        }
        //pDlg->Sort();
        pDlg->DoModal(GetID());

        // and wait till user selects one
        int iSelectedAlbum = pDlg->GetSelectedLabel();
        if (iSelectedAlbum < 0)
        {
          if (m_dlgProgress) m_dlgProgress->Close();
          return ;
        }

        // get album and artist
        strAlbumName = albums[iSelectedAlbum].strAlbum;
        strArtistName = albums[iSelectedAlbum].strArtist;
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

    set<CAlbum> setAlbums;

    // Get album names found in directory
    for (int i = 0; i < items.Size(); i++)
    {
      CFileItem* pItem = items[i];
      if (pItem->m_musicInfoTag.Loaded() && !pItem->m_musicInfoTag.GetAlbum().IsEmpty())
      {
        CAlbum album;
        album.strAlbum = pItem->m_musicInfoTag.GetAlbum();
        album.strArtist = pItem->m_musicInfoTag.GetArtist();
        setAlbums.insert(album);
      }
    }

    // no album found in folder
    // use the item label, we may find something?
    if (setAlbums.size() == 0)
    {
      if (m_dlgProgress) m_dlgProgress->Close();
      strAlbumName = pItem->GetLabel();
      bSaveDirThumb = true;
    }

    // one album, get the album and artist
    else if (setAlbums.size() == 1)
    {
      CAlbum album = *setAlbums.begin();
      strAlbumName = album.strAlbum;
      strArtistName = album.strArtist;
      bSaveDirThumb = true;
    }

    // many albums, let the user choose
    else if (setAlbums.size() > 1)
    {
      // convert set into vector for display
      VECALBUMS albums;
      set<CAlbum>::iterator it;
      for (it = setAlbums.begin(); it != setAlbums.end(); it++)
      albums.push_back(*it);

      // select dialog
      CGUIDialogSelect *pDlg = (CGUIDialogSelect*)m_gWindowManager.GetWindow(WINDOW_DIALOG_SELECT);
      if (pDlg)
      {
        pDlg->SetHeading(181);
        pDlg->Reset();
        pDlg->EnableButton(false);

        for (int i = 0; i < (int)albums.size(); ++i)
        {
          CAlbum& album = albums[i];
          CStdString strAlbum = album.strAlbum;
          if (!album.strArtist.IsEmpty())
            strAlbum += " - " + album.strArtist;
          pDlg->Add(strAlbum);
        }
        //pDlg->Sort();
        pDlg->DoModal(GetID());

        // and wait till user selects one
        int iSelectedAlbum = pDlg->GetSelectedLabel();
        if (iSelectedAlbum < 0)
        {
          if (m_dlgProgress) m_dlgProgress->Close();
          return ;
        }
        strAlbumName = albums[iSelectedAlbum].strAlbum;
        strArtistName = albums[iSelectedAlbum].strArtist;
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
        // get album and artist
        CStdString strAlbum = tag.GetAlbum();
        if (!strAlbum.IsEmpty())
        {
          strAlbumName = strAlbum;
          strArtistName = tag.GetArtist();
        }
      }
    }
  }

  if (m_dlgProgress) m_dlgProgress->Close();

  ShowAlbumInfo(strAlbumName, strArtistName, strPath, bSaveDb, bSaveDirThumb, false);
}

void CGUIWindowMusicBase::ShowAlbumInfo(const CStdString& strAlbum, const CStdString& strPath, bool bSaveDb, bool bSaveDirThumb, bool bRefresh)
{
  ShowAlbumInfo(strAlbum, "", strPath, bSaveDb, bSaveDirThumb, bRefresh);
}

void CGUIWindowMusicBase::ShowAlbumInfo(const CStdString& strAlbum, const CStdString& strArtist, const CStdString& strPath, bool bSaveDb, bool bSaveDirThumb, bool bRefresh)
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
  if (FindAlbumInfo(strAlbum, strArtist, album))
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
        // TODO: check for previous thumb here, and abort thumb saving

        CStdString strThumb;
        CUtil::GetAlbumThumb(album.GetTitle(), album.GetAlbumPath(), strThumb);

        CHTTP http;
        http.Download(album.GetImageURL(), strThumb);

        if (bSaveDb && CFile::Exists(strThumb))
          g_musicDatabase.SaveAlbumThumb(album.GetTitle(), album.GetAlbumPath(), strThumb);

        pDlgAlbumInfo->SetAlbum(album);
        pDlgAlbumInfo->DoModal(GetID());

        // Save directory thumb
        if (bSaveDirThumb)
        {
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
          ShowAlbumInfo(strAlbum, strArtist, strPath, bSaveDb, bSaveDirThumb, true);
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

    //  Do we have to autoswitch to the thumb control?
    UpdateButtons();
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
  CFileItem item(*m_vecItems[iItem]);
  
  if (item.IsRAR() || item.IsZIP())
    return;

  //  Allow queuing of unqueueable items
  //  when we try to queue them directly
  if (!item.CanQueue())
    item.SetCanQueue(true);

  AddItemToPlayList(&item);

  //move to next item
  m_viewControl.SetSelectedItem(iItem + 1);
  if (g_playlistPlayer.GetPlaylist(PLAYLIST_MUSIC).size() && !g_application.IsPlayingAudio() )
  {
    if (m_guiState.get())
      m_guiState->SetPlaylistDirectory("");

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
void CGUIWindowMusicBase::AddItemToPlayList(const CFileItem* pItem, int iPlayList /* = PLAYLIST_MUSIC */)
{
  if (!pItem->CanQueue() || pItem->IsRAR() || pItem->IsZIP()) // no zip/rar enques thank you!
    return;

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
    if (pItem->IsParentFolder()) return ;
    CFileItemList items;
    GetDirectory(pItem->m_strPath, items);
    SortItems(items);
    for (int i = 0; i < items.Size(); ++i)
      AddItemToPlayList(items[i], iPlayList);
  }
  else
  {
    if (!pItem->IsNFO() && pItem->IsAudio() && !pItem->IsPlayList())
    {
      CPlayList::CPlayListItem playlistItem;
      CUtil::ConvertFileItemToPlayListItem(pItem, playlistItem);
      if (m_guiState.get() && m_guiState->HideExtensions())
        playlistItem.RemoveExtension();
      g_playlistPlayer.GetPlaylist(iPlayList).Add(playlistItem);
    }
    if (pItem->IsPlayList())
    {
      CPlayListFactory factory;
      auto_ptr<CPlayList> pPlayList (factory.Create(pItem->m_strPath));
      if ( NULL != pPlayList.get())
      {
        // load it
        if (!pPlayList->Load(pItem->m_strPath))
        {
          CGUIDialogOK::ShowAndGetInput(6, 0, 477, 0);
          return; //hmmm unable to load playlist?
        }

        bool hideExtensions=false;
        if (m_guiState.get()) hideExtensions=m_guiState->HideExtensions();

        CPlayList playlist = *pPlayList;
        for (int i = 0; i < (int)playlist.size(); ++i)
        {
          CPlayList::CPlayListItem playlistItem = playlist[i];
          if (hideExtensions)
            playlistItem.RemoveExtension();
          g_playlistPlayer.GetPlaylist(iPlayList).Add(playlistItem);
        }
      }
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
  if ( !CGUIDialogKeyboard::ShowAndGetInput(strSearch, (CStdStringW)g_localizeStrings.Get(16017), false) )
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
    items.Sort(SORT_METHOD_LABEL, SORT_ORDER_ASC);

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

    CFileItem* pSelItem = items[iItem];

    OnSearchItemFound(pSelItem);

    if (m_dlgProgress) m_dlgProgress->Close();
  }
  else
  {
    if (m_dlgProgress) m_dlgProgress->Close();
    CGUIDialogOK::ShowAndGetInput(194, 284, 0, 0);
  }
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

  // Select the current window as default item
  CONTROL_SELECT_ITEM(CONTROL_BTNTYPE, g_stSettings.m_iMyMusicStartWindow - WINDOW_MUSIC_FILES);

  CGUIMediaWindow::UpdateButtons();
}

/// \brief React on the selected search item
/// \param pItem Search result item
void CGUIWindowMusicBase::OnSearchItemFound(const CFileItem* pSelItem)
{
  if (pSelItem->m_bIsFolder)
  {
    CStdString strPath = pSelItem->m_strPath;
    CStdString strParentPath;
    CUtil::GetParentPath(strPath, strParentPath);

    Update(strParentPath);

    SetHistoryForPath(strParentPath);

    strPath = pSelItem->m_strPath;
    CURL url(strPath);
    if (pSelItem->IsSmb() && !CUtil::HasSlashAtEnd(strPath))
      strPath += "/";

    for (int i = 0; i < m_vecItems.Size(); i++)
    {
      CFileItem* pItem = m_vecItems[i];
      if (pItem->m_strPath == strPath)
      {
        m_viewControl.SetSelectedItem(i);
        break;
      }
    }
  }
  else
  {
    CStdString strPath;
    CUtil::GetDirectory(pSelItem->m_strPath, strPath);

    Update(strPath);

    SetHistoryForPath(strPath);

    for (int i = 0; i < (int)m_vecItems.Size(); i++)
    {
      CFileItem* pItem = m_vecItems[i];
      if (pItem->m_strPath == pSelItem->m_strPath)
      {
        m_viewControl.SetSelectedItem(i);
        break;
      }
    }
  }
  m_viewControl.SetFocused();
}

bool CGUIWindowMusicBase::FindAlbumInfo(const CStdString& strAlbum, const CStdString& strArtist, CMusicAlbumInfo& album)
{
  // quietly return if Internet lookups are disabled
  if (!g_guiSettings.GetBool("Network.EnableInternet")) return false;

  // show dialog box indicating we're searching the album
  if (m_dlgProgress)
  {
    m_dlgProgress->SetHeading(185);
    m_dlgProgress->SetLine(0, strAlbum);
    m_dlgProgress->SetLine(1, strArtist);
    m_dlgProgress->SetLine(2, "");
    m_dlgProgress->StartModal(GetID());
  }

  try
  {
    CMusicInfoScraper scraper;
    scraper.FindAlbuminfo(strAlbum, strArtist);

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

            int iBest = -1;
            for (int i = 0; i < iAlbumCount; ++i)
            {
              CMusicAlbumInfo& info = scraper.GetAlbum(i);
              double fRelevance = CUtil::AlbumRelevance(info.GetTitle(), strAlbum, info.GetArtist(), strArtist);

              // are there any items with 100% relevance?
              if (fRelevance == 1.0f && iBest > -2)
              {
                // there was no best item so make this best item
                if (iBest == -1)
                  iBest = i;
                // there was another item with 100% relevance so the user has to choose
                else
                  iBest = -2;
              }

              // set the label to [relevance]  album - artist
              CStdString strTemp;
              strTemp.Format("[%0.2f]  %s", fRelevance, info.GetTitle2());
              pDlg->Add(strTemp);
            }
            // autochoose the iBest item
            if (iBest > -1)
            {
              pDlg->SetSelected(iBest);
              pDlg->Close();
            }
            // otherwise allow the user to choose
            else
            {
              // sort by relevance
              pDlg->Sort(false);
              pDlg->DoModal(GetID());
            }

            // and wait till user selects one
            iSelectedAlbum = pDlg->GetSelectedLabel();
            if (iSelectedAlbum < 0)
            {
              if (!pDlg->IsButtonPressed()) return false;
              CStdString strNewAlbum = strAlbum;
              if (!CGUIDialogKeyboard::ShowAndGetInput(strNewAlbum, (CStdStringW)g_localizeStrings.Get(16011), false)) return false;
              if (strNewAlbum == "") return false;

              CStdString strNewArtist = strArtist;
              if (!CGUIDialogKeyboard::ShowAndGetInput(strNewArtist, (CStdStringW)g_localizeStrings.Get(16025), false)) return false;

              if (m_dlgProgress)
              {
                m_dlgProgress->SetLine(0, strNewAlbum);
                m_dlgProgress->SetLine(1, strNewArtist);
                m_dlgProgress->Progress();
              }

              return FindAlbumInfo(strNewAlbum, strNewArtist, album);
            }

            // match the selected item back to an item in the scraper vector...
            // dialog select only allows text items
            // we sorted the list by relevance but the scraper vector is not sorted similarly
            // so the text needs to be matched back to the correct scraper item
            // [0.96]  Album - Artist
            // 12345678
            CStdString strSelectedAlbum = pDlg->GetSelectedLabelText();            
            strSelectedAlbum = strSelectedAlbum.Right(strSelectedAlbum.size() - 8); 
            for (int i = 0; i < iAlbumCount; ++i)
            {
              CMusicAlbumInfo& info = scraper.GetAlbum(i);
              if (strSelectedAlbum.Equals(info.GetTitle2()))
              {
                iSelectedAlbum = i;
                i = iAlbumCount;
              }
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
        return false;
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
    CGUIFont *pFont = pControl->GetLabelInfo().font;
    if (pFont)
    {
      float fWidth, fHeight;
      CStdStringW wszText = g_localizeStrings.Get(745); // "No scanned information for this view"
      CStdStringW wszText2 = g_localizeStrings.Get(746); // "Switch back to Files view"
      pFont->GetTextExtent(wszText, &fWidth, &fHeight);
      pFont->DrawText((float)iX, (float)iY - fHeight, 0xffffffff, 0, wszText.c_str(), XBFONT_CENTER_X | XBFONT_CENTER_Y);
      pFont->DrawText((float)iX, (float)iY + fHeight, 0xffffffff, 0, wszText2.c_str(), XBFONT_CENTER_X | XBFONT_CENTER_Y);
    }
  }
}

void CGUIWindowMusicBase::OnPopupMenu(int iItem)
{
  if ( iItem < 0 || iItem >= m_vecItems.Size() ) return ;
  // calculate our position
  int iPosX = 200, iPosY = 100;
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
  bool bIsGotoParent = m_vecItems[iItem]->IsParentFolder();
  
  int btn_Info      = 0; // Music Information
  int btn_PlayWith  = 0; // Play using alternate player
  int btn_Queue     = 0; // Queue Item
  
  VECPLAYERCORES vecCores;
  CPlayerCoreFactory::GetPlayers(*m_vecItems[iItem], vecCores);
  
  // turn off info/play/queue if the current item is goto parent ..
  if (!bIsGotoParent)
  {
    if (GetID() != WINDOW_MUSIC_PLAYLIST)
      btn_Info = pMenu->AddButton(13351);

    if (vecCores.size() >= 1)
      btn_PlayWith = pMenu->AddButton(15213);
    // allow a folder to be ad-hoc queued and played by the default player
    else if (m_vecItems[iItem]->m_bIsFolder || m_vecItems[iItem]->IsPlayList())
      btn_PlayWith = pMenu->AddButton(208);

    // don't show the add to playlist button in playlist window
    if (GetID() != WINDOW_MUSIC_PLAYLIST)
      btn_Queue = pMenu->AddButton(13347);
  }
  
  // enable Now Playing if there are items in the playlist
  int btn_Playlist = 0;
  if (GetID() == WINDOW_MUSIC_FILES && g_playlistPlayer.GetPlaylist(PLAYLIST_MUSIC).size() > 0)
    btn_Playlist = pMenu->AddButton(13350);

  int btn_Scan = 0;
  CGUIDialogMusicScan *pScanDlg = (CGUIDialogMusicScan *)m_gWindowManager.GetWindow(WINDOW_DIALOG_MUSIC_SCAN);
  if (pScanDlg && pScanDlg->IsRunning())
  {
    // turn off the Scan button if we're not in files view or a internet stream
    if (GetID() == WINDOW_MUSIC_FILES || !m_vecItems.IsInternetStream() )
      btn_Scan = pMenu->AddButton(13353);         // Stop Scanning
  }
  else
  {
    if (GetID() == WINDOW_MUSIC_FILES || !m_vecItems.IsInternetStream() )
      btn_Scan = pMenu->AddButton(13352);         // Scan Folder to Database
  }

  // Search is always visisble
  int btn_Search = pMenu->AddButton(137);
  
  // enable Rip CD Audio button if we have an audio disc
  int btn_Rip  = 0;
  if (CDetectDVDMedia::IsDiscInDrive())
  {
    // GeminiServer those cd's can also include Audio Tracks: CDExtra and MixedMode!
    CCdInfo *pCdInfo = CDetectDVDMedia::GetCdInfo(); 
    if ( pCdInfo->IsAudio(1) || pCdInfo->IsCDExtra(1) || pCdInfo->IsMixedMode(1) )
      btn_Rip = pMenu->AddButton(600);
  }

  // enable CDDB lookup if the current dir is CDDA
  int btn_CDDB   = 0; // CDDB lookup
  if (CUtil::IsCDDA(m_vecItems.m_strPath))
    btn_CDDB = pMenu->AddButton(16002);

  int btn_Delete = 0; // Delete
  int btn_Rename = 0; // Rename
  if (!bIsGotoParent)
  {
    if (m_vecItems.m_strPath.Equals(CUtil::MusicPlaylistsLocation()) || g_guiSettings.GetBool("MusicFiles.AllowFileDeletion"))
    {
      btn_Delete = pMenu->AddButton(117);               // Delete
      btn_Rename = pMenu->AddButton(118);               // Rename
    }
  }

  // GeminiServer Todo: Set a MasterLock Option to Enable or disable Settings incontext menu!
  int btn_Settings = pMenu->AddButton(5);    // Settings...

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
    else if (btnid == btn_PlayWith)
    {
      // if folder, play with default player
      if (m_vecItems[iItem]->m_bIsFolder)
      {
        PlayItem(iItem);
      }
      else
      {
        // Play With...
        g_application.m_eForcedNextPlayer = CPlayerCoreFactory::SelectPlayerDialog(vecCores, iPosX, iPosY);
        if( g_application.m_eForcedNextPlayer != EPC_NONE )
          OnClick(iItem);
      }
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
        Update(m_vecItems.m_strPath);
    }
    // Delete
    else if (btnid == btn_Delete)
    {
      OnDeleteItem(iItem);
    }
    //Rename
    else if (btnid == btn_Rename)
    {
      OnRenameItem(iItem);
    }
    // Settings
    else if (btnid == btn_Settings)
    {
      //MasterPassword
      int iLockSettings = g_guiSettings.GetInt("Masterlock.LockSettingsFilemanager");
      if (iLockSettings == 1 || iLockSettings == 3) 
      {
        if (g_passwordManager.IsMasterLockLocked(true))
          m_gWindowManager.ActivateWindow(WINDOW_SETTINGS_MYMUSIC); 
          
      }
      else m_gWindowManager.ActivateWindow(WINDOW_SETTINGS_MYMUSIC); 
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

void CGUIWindowMusicBase::PlayItem(int iItem)
{
  // restrictions should be placed in the appropiate window code
  // only call the base code if the item passes since this clears
  // the currently playing temp playlist

  const CFileItem* pItem = m_vecItems[iItem];
  // if its a folder, build a temp playlist
  if (pItem->m_bIsFolder)
  {
    CFileItem item(*m_vecItems[iItem]);
  
    //  Allow queuing of unqueueable items
    //  when we try to queue them directly
    if (!item.CanQueue())
      item.SetCanQueue(true);

    // skip ".."
    if (item.IsParentFolder())
      return;

    int iPlaylist=m_guiState->GetPlaylist();
    // clear current temp playlist
    g_playlistPlayer.GetPlaylist(iPlaylist).Clear();
    g_playlistPlayer.Reset();

    // recursively add items to temp playlist
    AddItemToPlayList(&item, iPlaylist);

    CStdString strPlayListDirectory = m_vecItems.m_strPath;
    if (CUtil::HasSlashAtEnd(strPlayListDirectory))
      strPlayListDirectory.Delete(strPlayListDirectory.size() - 1);

    m_guiState->SetPlaylistDirectory(strPlayListDirectory);
    // play!
    g_playlistPlayer.SetCurrentPlaylist(iPlaylist);
    g_playlistPlayer.Play();
  }
  else if (pItem->IsPlayList())
  {
    LoadPlayList(pItem->m_strPath);
  }
  // otherwise just play the song
  else
  {
    OnClick(iItem);
  }
}

void CGUIWindowMusicBase::OnDeleteItem(int iItem)
{
  if ( iItem < 0 || iItem >= m_vecItems.Size()) return;
  if (!CGUIWindowFileManager::DeleteItem(m_vecItems[iItem]))
    return;

  Update(m_vecItems.m_strPath);
  m_viewControl.SetSelectedItem(iItem);
}

void CGUIWindowMusicBase::OnRenameItem(int iItem)
{
  if ( iItem < 0 || iItem >= m_vecItems.Size()) return;
  if (!CGUIWindowFileManager::RenameFile(m_vecItems[iItem]->m_strPath))
    return;

  Update(m_vecItems.m_strPath);
  m_viewControl.SetSelectedItem(iItem);
}

void CGUIWindowMusicBase::LoadPlayList(const CStdString& strPlayList)
{
  // load a playlist like .m3u, .pls
  // first get correct factory to load playlist
  CPlayListFactory factory;
  auto_ptr<CPlayList> pPlayList (factory.Create(strPlayList));
  if ( NULL != pPlayList.get())
  {
    // load it
    if (!pPlayList->Load(strPlayList))
    {
      CGUIDialogOK::ShowAndGetInput(6, 0, 477, 0);
      return ; //hmmm unable to load playlist?
    }
  }

  int iSize = pPlayList->size();
  if (g_application.ProcessAndStartPlaylist(strPlayList, *pPlayList, PLAYLIST_MUSIC))
  {
    if (m_guiState.get())
      m_guiState->SetPlaylistDirectory("");
    // activate the playlist window if its not activated yet
    if (GetID() == m_gWindowManager.GetActiveWindow() && iSize > 1)
    {
      m_gWindowManager.ActivateWindow(WINDOW_MUSIC_PLAYLIST);
    }
  }
}
