
#include "stdafx.h"
#include "GUIWindowMusicSongs.h"
#include "PlayListFactory.h"
#include "Util.h"
#include "Application.h"
#include "CUEDocument.h"
#include "GUIPassword.h"
#include "GUIDialogMusicScan.h"
#include "GUIDialogContextMenu.h"
#include "GUIViewState.h"

#define CONTROL_BTNVIEWASICONS     2 
#define CONTROL_BTNSORTBY          3
#define CONTROL_BTNSORTASC         4
#define CONTROL_BTNTYPE            5
#define CONTROL_LIST              50
#define CONTROL_THUMBS            51
#define CONTROL_LABELFILES        12

#define CONTROL_BTNPLAYLISTS   7
#define CONTROL_BTNSCAN      9
#define CONTROL_BTNREC      10
#define CONTROL_BTNRIP      11


CGUIWindowMusicSongs::CGUIWindowMusicSongs(void)
    : CGUIWindowMusicBase(WINDOW_MUSIC_FILES, "MyMusicSongs.xml")
{
  m_vecItems.m_strPath="?";

  // Remove old HD cache every time XBMC is loaded
  DeleteDirectoryCache();
}

CGUIWindowMusicSongs::~CGUIWindowMusicSongs(void)
{
}

bool CGUIWindowMusicSongs::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_INIT:
    {
      // removed the start window check from files view
      // the window translator does it by using a virtual window id (5)

      // check for a passed destination path
      CStdString strDestination = message.GetStringParam();
      if (!strDestination.IsEmpty())
      {
        message.SetStringParam("");
        CLog::Log(LOGINFO, "Attempting to quickpath to: %s", strDestination.c_str());
        m_vecPathHistory.clear();
      }

      // is this the first time the window is opened?
      if (m_vecItems.m_strPath == "?" && strDestination.IsEmpty())
      {
        strDestination = g_stSettings.m_szDefaultMusic;
        m_vecItems.m_strPath=strDestination;
        CLog::Log(LOGINFO, "Attempting to default to: %s", strDestination.c_str());
      }

      // try to open the destination path
      if (!strDestination.IsEmpty())
      {
        // default parameters if the jump fails
        m_vecItems.m_strPath.Empty();

        bool bIsBookmarkName = false;
        int iIndex = CUtil::GetMatchingShare(strDestination, g_settings.m_vecMyMusicShares, bIsBookmarkName);
        if (iIndex > -1)
        {
          // set current directory to matching share
          if (bIsBookmarkName)
            m_vecItems.m_strPath=g_settings.m_vecMyMusicShares[iIndex].strPath;
          else
            m_vecItems.m_strPath=strDestination;
          CLog::Log(LOGINFO, "  Success! Opened destination path: %s", strDestination.c_str());
        }
        else
        {
          CLog::Log(LOGERROR, "  Failed! Destination parameter (%s) does not match a valid share!", strDestination.c_str());
        }

        // need file filters or GetDirectory in SetHistoryPath fails
        m_rootDir.SetMask(g_stSettings.m_szMyMusicExtensions);
        m_rootDir.SetShares(g_settings.m_vecMyMusicShares);
        SetHistoryForPath(m_vecItems.m_strPath);
      }

      m_rootDir.SetMask(g_stSettings.m_szMyMusicExtensions);
      m_rootDir.SetShares(g_settings.m_vecMyMusicShares);

      if (m_vecItems.IsCDDA() || m_vecItems.IsOnDVD())
      {
        // No disc in drive but current directory is a dvd share
        if (!CDetectDVDMedia::IsDiscInDrive())
          m_vecItems.m_strPath.Empty();

        // look if disc has changed outside this window and url is still the same
        CFileItem dvdUrl;
        dvdUrl.m_strPath = m_rootDir.GetDVDDriveUrl();
        if (m_vecItems.IsCDDA() && !dvdUrl.IsCDDA())
          m_vecItems.m_strPath.Empty();
        if (m_vecItems.IsOnDVD() && !dvdUrl.IsOnDVD())
          m_vecItems.m_strPath.Empty();
      }

      return CGUIWindowMusicBase::OnMessage(message);
    }
    break;

  case GUI_MSG_DIRECTORY_SCANNED:
    {
      CFileItem directory(message.GetStringParam(), true);

      // Only update thumb on a local drive
      if (directory.IsHD())
      {
        CStdString strParent;
        CUtil::GetParentPath(directory.m_strPath, strParent);
        if (directory.m_strPath == m_vecItems.m_strPath || strParent == m_vecItems.m_strPath)
        {
          Update(m_vecItems.m_strPath);
        }
      }
    }
    break;

  case GUI_MSG_DVDDRIVE_EJECTED_CD:
    {
      DeleteRemoveableMediaDirectoryCache();
    }
    break;

  case GUI_MSG_SCAN_FINISHED:
    {
      Update(m_vecItems.m_strPath);
    }
    break;

  case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();

      if (iControl == CONTROL_BTNPLAYLISTS)
      {
        if (!m_vecItems.m_strPath.Equals(CUtil::MusicPlaylistsLocation()))
          Update(CUtil::MusicPlaylistsLocation());
      }
      else if (iControl == CONTROL_BTNSCAN)
      {
        OnScan();
      }
      else if (iControl == CONTROL_BTNREC)
      {
        if (g_application.IsPlayingAudio() )
        {
          if (g_application.m_pPlayer->CanRecord() )
          {
            bool bIsRecording = g_application.m_pPlayer->IsRecording();
            g_application.m_pPlayer->Record(!bIsRecording);
            UpdateButtons();
          }
        }
      }
      else if (iControl == CONTROL_BTNRIP)
      {
        OnRipCD();
      }
    }
    break;
  }

  return CGUIWindowMusicBase::OnMessage(message);
}

void CGUIWindowMusicSongs::OnScan()
{
  CGUIDialogMusicScan *musicScan = (CGUIDialogMusicScan *)m_gWindowManager.GetWindow(WINDOW_DIALOG_MUSIC_SCAN);
  if (musicScan && musicScan->IsScanning())
  {
    musicScan->StopScanning();
    return ;
  }

  // check whether we have scanned here before
  bool bUpdateAll = false;
  CStdString strPaths;
  g_musicDatabase.GetSubpathsFromPath(m_vecItems.m_strPath, strPaths);
  if (strPaths.length() > 2)
  { // yes, we have, we should prompt the user to ask if they want
    // to do a full scan, or just add new items...
    if (CGUIDialogYesNo::ShowAndGetInput(189, 702, 703, 704))
      bUpdateAll = true;
  }

  CUtil::DeleteDatabaseDirectoryCache();

  // Start background loader
  int iControl=GetFocusedControl();
  if (musicScan) musicScan->StartScanning(m_vecItems.m_strPath, bUpdateAll);
  SET_CONTROL_FOCUS(iControl, 0);
  UpdateButtons();
  return ;
}

void CGUIWindowMusicSongs::UpdateButtons()
{
  CGUIWindowMusicBase::UpdateButtons();

  bool bIsPlaying = g_application.IsPlayingAudio();
  bool bCanRecord = false;
  bool bIsRecording = false;

  if (bIsPlaying)
  {
    bCanRecord = g_application.m_pPlayer->CanRecord();
    bIsRecording = g_application.m_pPlayer->IsRecording();
  }

  // Update Record button
  if (bIsPlaying && bCanRecord)
  {
    CONTROL_ENABLE(CONTROL_BTNREC);
    if (bIsRecording)
    {
      SET_CONTROL_LABEL(CONTROL_BTNREC, 265); //Stop Recording
    }
    else
    {
      SET_CONTROL_LABEL(CONTROL_BTNREC, 264); //Record
    }
  }
  else
  {
    SET_CONTROL_LABEL(CONTROL_BTNREC, 264); //Record
    CONTROL_DISABLE(CONTROL_BTNREC);
  }

  // Update CDDA Rip button
  CCdInfo *pCdInfo = CDetectDVDMedia::GetCdInfo();
  if (CDetectDVDMedia::IsDiscInDrive() && pCdInfo && pCdInfo->IsAudio(1))
  {
    CONTROL_ENABLE(CONTROL_BTNRIP);
  }
  else
  {
    CONTROL_DISABLE(CONTROL_BTNRIP);
  }

  // Disable scan button if shoutcast
  if (m_vecItems.IsVirtualDirectoryRoot() || m_vecItems.IsShoutCast() || m_vecItems.IsLastFM() || m_vecItems.IsMusicDb())
  {
    CONTROL_DISABLE(CONTROL_BTNSCAN);
  }
  else
  {
    CONTROL_ENABLE(CONTROL_BTNSCAN);
  }

  CGUIDialogMusicScan *musicScan = (CGUIDialogMusicScan *)m_gWindowManager.GetWindow(WINDOW_DIALOG_MUSIC_SCAN);
  if (musicScan && musicScan->IsScanning())
  {
    SET_CONTROL_LABEL(CONTROL_BTNSCAN, 14056); // Stop Scan
  }
  else
  {
    SET_CONTROL_LABEL(CONTROL_BTNSCAN, 102); // Scan
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

void CGUIWindowMusicSongs::OnClick(int iItem)
{
  if ( iItem < 0 || iItem >= m_vecItems.Size() ) return ;
  CFileItem* pItem = m_vecItems[iItem];
  CStdString strPath = pItem->m_strPath;
  if (pItem->m_bIsFolder)
  {
    if ( pItem->m_bIsShareOrDrive )
    {
      if ( !g_passwordManager.IsItemUnlocked( pItem, "music" ) )
        return ;

      if ( !HaveDiscOrConnection( pItem->m_strPath, pItem->m_iDriveType ) )
        return ;
    }
    if (pItem->IsParentFolder())
    {
      // go back a directory
      GoParentFolder();

      // GoParentFolder() calls Update(), so just return
      return;
    }
    if (!Update(strPath))
      ShowShareErrorMessage(pItem);
  }
  else if (pItem->IsZIP() && g_guiSettings.GetBool("MusicFiles.HandleArchives")) // mount zip archive
  {
    CShare shareZip;
    shareZip.strPath.Format("zip://Z:\\,%i,,%s,\\",1, pItem->m_strPath.c_str() );
    m_rootDir.AddShare(shareZip);
    Update(shareZip.strPath);
  }
  else if (pItem->IsRAR()&& g_guiSettings.GetBool("MusicFiles.HandleArchives")) // mount rar archive
  {
    CShare shareRar;
    shareRar.strPath.Format("rar://Z:\\,%i,,%s,\\",EXFILE_AUTODELETE, pItem->m_strPath.c_str() );
    m_rootDir.AddShare(shareRar);
    Update(shareRar.strPath);
  }
  else
  {
    if (pItem->IsPlayList())
    {
      LoadPlayList(strPath);
    }
    else
    {
      if (g_guiSettings.GetBool("MusicFiles.AutoPlayNextItem"))
      {
        //play and add current directory to temporary playlist
        int nFolderCount = 0;
        g_playlistPlayer.GetPlaylist( PLAYLIST_MUSIC_TEMP ).Clear();
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
            g_playlistPlayer.GetPlaylist(PLAYLIST_MUSIC_TEMP).Add(playlistItem);
          }
          else if (i <= iItem)
            iNoSongs++;
        }

        // Save current window and directory to know where the selected item was
        m_nTempPlayListWindow = GetID();
        m_strTempPlayListDirectory = m_vecItems.m_strPath;
        if (CUtil::HasSlashAtEnd(m_strTempPlayListDirectory))
          m_strTempPlayListDirectory.Delete(m_strTempPlayListDirectory.size() - 1);

        g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_MUSIC_TEMP);
        g_playlistPlayer.Play(iItem - nFolderCount - iNoSongs);
      }
      else
      {
        // Reset Playlistplayer, playback started now does
        // not use the playlistplayer.
        g_playlistPlayer.Reset();
        g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_NONE);
        g_application.PlayFile(*pItem);
      }
      return;
    }
  }
}

void CGUIWindowMusicSongs::OnFileItemFormatLabel(CFileItem* pItem)
{
  // set label 1 & 2 from format string
  if (!pItem->m_bIsFolder && pItem->m_musicInfoTag.Loaded())
  {
    SetLabelFromTag(pItem);
  }
  else
  { // no tag info, so we disable the file extension if it has one
    if (m_hideExtensions)
      pItem->RemoveExtension();

    // and then set label 2
    auto_ptr<CGUIViewState> pState(CGUIViewState::GetViewState(GetID(), m_vecItems));
    if (pState.get())
    {
      SORT_METHOD SortMethod = pState->GetSortMethod();

      if (SortMethod == SORT_METHOD_LABEL || SortMethod == SORT_METHOD_SIZE || SortMethod == SORT_METHOD_FILE)
      {
        if (pItem->m_bIsFolder)
        {
          if (!pItem->IsShoutCast() && !pItem->IsLastFM())
            pItem->SetLabel2("");
        }
        else
        {
          if (pItem->m_dwSize > 0)
            pItem->SetFileSizeLabel();
          if (SortMethod == SORT_METHOD_LABEL || SortMethod == SORT_METHOD_FILE)
          {
            int nDuration = pItem->m_musicInfoTag.GetDuration();
            if (nDuration > 0)
            {
              CStdString strDuration;
              CUtil::SecondsToHMSString(nDuration, strDuration);
              pItem->SetLabel2(strDuration);
            }
          }
        }
      }
      else
      {
        if (pItem->m_stTime.wYear && (!pItem->IsShoutCast() && !pItem->IsLastFM()))
        {
          CStdString strDateTime;
          CUtil::GetDate(pItem->m_stTime, strDateTime);
          pItem->SetLabel2(strDateTime);
        }
        else if (!pItem->IsShoutCast() && !pItem->IsLastFM())
          pItem->SetLabel2("");
      }
    }
  }

  // set thumbs and default icons
  if (!pItem->m_bIsShareOrDrive)
  {
    pItem->SetMusicThumb();
    pItem->FillInDefaultIcon();
  }
}

void CGUIWindowMusicSongs::OnRetrieveMusicInfo(CFileItemList& items)
{
  int nFolderCount = items.GetFolderCount();
  // Skip items with folders only
  if (nFolderCount == (int)items.Size())
    return ;

  MAPSONGS songsMap;
  // get all information for all files in current directory from database
  g_musicDatabase.GetSongsByPath(m_vecItems.m_strPath, songsMap);

  // Nothing in database and id3 tags disabled; dont load tags from cdda files
  if (songsMap.size() == 0 && !g_guiSettings.GetBool("MusicFiles.UseTags"))
    return ;

  // Do we have cached items
  MAPFILEITEMS itemsMap;
  LoadDirectoryCache(m_vecItems.m_strPath, itemsMap);

  bool bShowProgress = false;
  bool bProgressVisible = false;
  if (!m_gWindowManager.IsRouted())
    bShowProgress = true;

  DWORD dwTick = timeGetTime();
  int iTaglessFiles = 0;

  // for every file found, but skip folder
  for (int i = 0; i < (int)items.Size(); ++i)
  {
    CFileItem* pItem = items[i];

    // dont try reading tags for folders, playlists or shoutcast streams
    if (pItem->m_bIsFolder || pItem->IsPlayList() || pItem->IsInternetStream())
    {
      iTaglessFiles++;
      continue;
    }

    // is the tag for this file already loaded?
    if (!pItem->m_musicInfoTag.Loaded())
    {
      // no, then we gonna load it.
      IMAPSONGS itSong;
      IMAPFILEITEMS itItem;

      // Is items load from the database
      if ((itSong = songsMap.find(pItem->m_strPath)) != songsMap.end())
      {
        CSong song = itSong->second;
        pItem->m_musicInfoTag.SetSong(song);
        pItem->SetThumbnailImage(song.strThumb);
      } // Query map if we previously cached the file on HD
      else if ((itItem = itemsMap.find(pItem->m_strPath)) != itemsMap.end() && CUtil::CompareSystemTime(&itItem->second->m_stTime, &pItem->m_stTime) == 0)
      {
        pItem->m_musicInfoTag = itItem->second->m_musicInfoTag;
        pItem->SetThumbnailImage(itItem->second->GetThumbnailImage());
      } // if id3 tag scanning is turned on
      else if (g_guiSettings.GetBool("MusicFiles.UseTags"))
      {
        // then parse tag from file
        CMusicInfoTagLoaderFactory factory;
        auto_ptr<IMusicInfoTagLoader> pLoader (factory.CreateLoader(pItem->m_strPath));
        if (NULL != pLoader.get())
          pLoader->Load(pItem->m_strPath, pItem->m_musicInfoTag); // get tag from file
      }
    } // if (!tag.Loaded() )

    // Should we init a progress dialog
    if (bShowProgress && !bProgressVisible)
    {
      DWORD dwElapsed = timeGetTime() - dwTick;

      // if tag loading took more then 1.5 secs. till now
      // show the progress dialog
      if (dwElapsed > 1500)
      {
        if (m_dlgProgress)
        {
          CURL url(m_vecItems.m_strPath);
          CStdString strStrippedPath;
          url.GetURLWithoutUserDetails(strStrippedPath);
          m_dlgProgress->SetHeading(189);
          m_dlgProgress->SetLine(0, 505);
          m_dlgProgress->SetLine(1, "");
          m_dlgProgress->SetLine(2, strStrippedPath );
          m_dlgProgress->StartModal(GetID());
          m_dlgProgress->ShowProgressBar(true);
          m_dlgProgress->SetProgressBarMax(items.GetFileCount());
          m_dlgProgress->StepProgressBar(i-iTaglessFiles);
          m_dlgProgress->Progress();
          bProgressVisible = true;
        }
      }
    }


    if (bProgressVisible)
    {
      m_dlgProgress->StepProgressBar();
      m_dlgProgress->Progress();
    }

    // Canceled by the user, finish
    if (bProgressVisible && m_dlgProgress && m_dlgProgress->IsCanceled())
      break;

  } // for (int i=0; i < (int)items.size(); ++i)

  // Save the hdd cache if there are more songs in this directory then loaded from database
  if ((m_dlgProgress && !m_dlgProgress->IsCanceled()) && songsMap.size() != (items.Size() - iTaglessFiles))
    SaveDirectoryCache(m_vecItems.m_strPath, items);

  // cleanup cache loaded from HD
  IMAPFILEITEMS it = itemsMap.begin();
  while (it != itemsMap.end())
  {
    delete it->second;
    it++;
  }
  itemsMap.erase(itemsMap.begin(), itemsMap.end());

  if (bShowProgress && m_dlgProgress)
    m_dlgProgress->Close();
}

/// \brief Search for a song or a artist with search string \e strSearch in the musicdatabase and return the found \e items
/// \param strSearch The search string
/// \param items Items Found
void CGUIWindowMusicSongs::DoSearch(const CStdString& strSearch, CFileItemList& items)
{
  VECALBUMS albums;
  g_musicDatabase.FindAlbumsByName(strSearch, albums);

  if (albums.size())
  {
    CStdString strAlbum = g_localizeStrings.Get(483); // Album
    for (int i = 0; i < (int)albums.size(); i++)
    {
      CAlbum& album = albums[i];
      CFileItem* pItem = new CFileItem(album);
      pItem->SetLabel("[" + strAlbum + "] " + album.strAlbum + " - " + album.strArtist);
      items.Add(pItem);
    }
  }

  VECSONGS songs;
  g_musicDatabase.FindSongsByNameAndArtist(strSearch, songs);

  if (songs.size())
  {
    CStdString strSong = g_localizeStrings.Get(179); // Song
    for (int i = 0; i < (int)songs.size(); i++)
    {
      CSong& song = songs[i];
      CFileItem* pItem = new CFileItem(song);
      pItem->SetLabel("[" + strSong + "] " + song.strTitle + " - " + song.strArtist + " - " + song.strAlbum);
      items.Add(pItem);
    }
  }
}

void CGUIWindowMusicSongs::GetDirectoryHistoryString(const CFileItem* pItem, CStdString& strHistoryString)
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

void CGUIWindowMusicSongs::OnPopupMenu(int iItem)
{
  // We don't check for iItem range here, as we may later support creating shares
  // from a blank starting setup

  // calculate our position
  int iPosX = 200;
  int iPosY = 100;
  const CGUIControl *pList = GetControl(CONTROL_LIST);
  if (pList)
  {
    iPosX = pList->GetXPosition() + pList->GetWidth() / 2;
    iPosY = pList->GetYPosition() + pList->GetHeight() / 2;
  }
  if ( m_vecItems.IsVirtualDirectoryRoot() )
  {
    if (iItem < 0)
    { // TODO: we should check here whether the user can add shares, and have the option to do so
      return ;
    }
    // mark the item
    m_vecItems[iItem]->Select(true);

    bool bMaxRetryExceeded = false;
    if (g_stSettings.m_iMasterLockMaxRetry != 0)
      bMaxRetryExceeded = !(m_vecItems[iItem]->m_iBadPwdCount < g_stSettings.m_iMasterLockMaxRetry);

    // and do the popup menu
    if (CGUIDialogContextMenu::BookmarksMenu("music", m_vecItems[iItem]->GetLabel(), m_vecItems[iItem]->m_strPath, m_vecItems[iItem]->m_iLockMode, bMaxRetryExceeded, iPosX, iPosY))
    {
      m_rootDir.SetShares(g_settings.m_vecMyMusicShares);
      Update(m_vecItems.m_strPath);
      return ;
    }
    m_vecItems[iItem]->Select(false);
    return ;
  }
  CGUIWindowMusicBase::OnPopupMenu(iItem);
}

void CGUIWindowMusicSongs::LoadDirectoryCache(const CStdString& strDirectory, MAPFILEITEMS& items)
{
  CFileItem directory(strDirectory, true);
  if (CUtil::HasSlashAtEnd(directory.m_strPath))
    directory.m_strPath.Delete(directory.m_strPath.size() - 1);

  Crc32 crc;
  crc.ComputeFromLowerCase(directory.m_strPath);

  CStdString strFileName;
  if (directory.IsCDDA() || directory.IsOnDVD())
    strFileName.Format("Z:\\r-%x.fi", crc);
  else
    strFileName.Format("Z:\\%x.fi", crc);

  CFile file;
  if (file.Open(strFileName))
  {
    CArchive ar(&file, CArchive::load);
    int iSize = 0;
    ar >> iSize;
    for (int i = 0; i < iSize; i++)
    {
      CFileItem* pItem = new CFileItem();
      ar >> *pItem;
      items.insert(MAPFILEITEMSPAIR(pItem->m_strPath, pItem));
    }
    ar.Close();
    file.Close();
  }
}

void CGUIWindowMusicSongs::SaveDirectoryCache(const CStdString& strDirectory, CFileItemList& items)
{
  int iSize = items.Size();

  if (iSize <= 0)
    return ;

  CFileItem directory(strDirectory, true);
  if (CUtil::HasSlashAtEnd(directory.m_strPath))
    directory.m_strPath.Delete(directory.m_strPath.size() - 1);

  Crc32 crc;
  crc.ComputeFromLowerCase(directory.m_strPath);

  CStdString strFileName;
  if (directory.IsCDDA() || directory.IsOnDVD())
    strFileName.Format("Z:\\r-%x.fi", crc);
  else
    strFileName.Format("Z:\\%x.fi", crc);

  CFile file;
  if (file.OpenForWrite(strFileName, true, true)) // overwrite always
  {
    CArchive ar(&file, CArchive::store);
    ar << items.Size();
    for (int i = 0; i < iSize; i++)
    {
      CFileItem* pItem = items[i];
      ar << *pItem;
    }
    ar.Close();
    file.Close();
  }

}

void CGUIWindowMusicSongs::DeleteDirectoryCache()
{
  WIN32_FIND_DATA wfd;
  memset(&wfd, 0, sizeof(wfd));

  CAutoPtrFind hFind( FindFirstFile("Z:\\*.fi", &wfd));
  if (!hFind.isValid())
    return ;
  do
  {
    if ( !(wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) )
    {
      CStdString strFile = "Z:\\";
      strFile += wfd.cFileName;
      DeleteFile(strFile.c_str());
    }
  }
  while (FindNextFile(hFind, &wfd));
}

void CGUIWindowMusicSongs::DeleteRemoveableMediaDirectoryCache()
{
  WIN32_FIND_DATA wfd;
  memset(&wfd, 0, sizeof(wfd));

  CAutoPtrFind hFind( FindFirstFile("Z:\\r-*.fi", &wfd));
  if (!hFind.isValid())
    return ;
  do
  {
    if ( !(wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) )
    {
      CStdString strFile = "Z:\\";
      strFile += wfd.cFileName;
      DeleteFile(strFile.c_str());
    }
  }
  while (FindNextFile(hFind, &wfd));
}

void CGUIWindowMusicSongs::PlayItem(int iItem)
{
  // unlike additemtoplaylist, we need to check the items here
  // before calling it since the current playlist will be stopped
  // and cleared!

  // we're at the root bookmark listing
  if (m_vecItems.IsVirtualDirectoryRoot())
    return;

  CGUIWindowMusicBase::PlayItem(iItem);
}