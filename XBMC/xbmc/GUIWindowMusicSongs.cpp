
#include "stdafx.h"
#include "GUIWindowMusicSongs.h"
#include "PlayListFactory.h"
#include "Util.h"
#include "Application.h"
#include "CUEDocument.h"
#include "AutoSwitch.h"
#include "GUIPassword.h"

#define CONTROL_BTNVIEWASICONS  2
#define CONTROL_BTNSORTBY     3
#define CONTROL_BTNSORTASC    4

#define CONTROL_BTNTYPE      6
#define CONTROL_BTNPLAYLISTS   7
#define CONTROL_BTNSCAN      9
#define CONTROL_BTNREC      10
#define CONTROL_BTNRIP      11

#define CONTROL_LABELFILES        12

#define CONTROL_LIST       50
#define CONTROL_THUMBS      51

struct SSortMusicSongs
{
  static bool Sort(CFileItem* pStart, CFileItem* pEnd)
  {
    CFileItem& rpStart = *pStart;
    CFileItem& rpEnd = *pEnd;
    if (rpStart.GetLabel() == "..") return true;
    if (rpEnd.GetLabel() == "..") return false;
    bool bGreater = true;
    if (m_bSortAscending) bGreater = false;

    if (rpStart.m_bIsFolder == rpEnd.m_bIsFolder)
    {
      char szfilename1[1024];
      char szfilename2[1024];

      switch ( m_iSortMethod )
      {
      case 0:  // Sort by Listlabel
        strcpy(szfilename1, rpStart.GetLabel().c_str());
        strcpy(szfilename2, rpEnd.GetLabel().c_str());
        break;

      case 1:  // Sort by Date
        if ( rpStart.m_stTime.wYear > rpEnd.m_stTime.wYear ) return bGreater;
        if ( rpStart.m_stTime.wYear < rpEnd.m_stTime.wYear ) return !bGreater;

        if ( rpStart.m_stTime.wMonth > rpEnd.m_stTime.wMonth ) return bGreater;
        if ( rpStart.m_stTime.wMonth < rpEnd.m_stTime.wMonth ) return !bGreater;

        if ( rpStart.m_stTime.wDay > rpEnd.m_stTime.wDay ) return bGreater;
        if ( rpStart.m_stTime.wDay < rpEnd.m_stTime.wDay ) return !bGreater;

        if ( rpStart.m_stTime.wHour > rpEnd.m_stTime.wHour ) return bGreater;
        if ( rpStart.m_stTime.wHour < rpEnd.m_stTime.wHour ) return !bGreater;

        if ( rpStart.m_stTime.wMinute > rpEnd.m_stTime.wMinute ) return bGreater;
        if ( rpStart.m_stTime.wMinute < rpEnd.m_stTime.wMinute ) return !bGreater;

        if ( rpStart.m_stTime.wSecond > rpEnd.m_stTime.wSecond ) return bGreater;
        if ( rpStart.m_stTime.wSecond < rpEnd.m_stTime.wSecond ) return !bGreater;
        return true;
        break;

      case 2:  // Sort by Size
        if ( rpStart.m_dwSize > rpEnd.m_dwSize) return bGreater;
        if ( rpStart.m_dwSize < rpEnd.m_dwSize) return !bGreater;
        return true;
        break;

      case 3:  // Sort by TrackNum
        if ( rpStart.m_musicInfoTag.GetTrackAndDiskNumber() > rpEnd.m_musicInfoTag.GetTrackAndDiskNumber()) return bGreater;
        if ( rpStart.m_musicInfoTag.GetTrackAndDiskNumber() < rpEnd.m_musicInfoTag.GetTrackAndDiskNumber()) return !bGreater;
        return true;
        break;

      case 4:  // Sort by Duration
        if ( rpStart.m_musicInfoTag.GetDuration() > rpEnd.m_musicInfoTag.GetDuration()) return bGreater;
        if ( rpStart.m_musicInfoTag.GetDuration() < rpEnd.m_musicInfoTag.GetDuration()) return !bGreater;
        return true;
        break;

      case 5:  // Sort by Title
        strcpy(szfilename1, rpStart.m_musicInfoTag.GetTitle());
        strcpy(szfilename2, rpEnd.m_musicInfoTag.GetTitle());
        break;

      case 6:  // Sort by Artist
        strcpy(szfilename1, rpStart.m_musicInfoTag.GetArtist());
        strcpy(szfilename2, rpEnd.m_musicInfoTag.GetArtist());
        break;

      case 7:  // Sort by Album
        strcpy(szfilename1, rpStart.m_musicInfoTag.GetAlbum());
        strcpy(szfilename2, rpEnd.m_musicInfoTag.GetAlbum());
        break;

      case 8:  // Sort by FileName
        sprintf(szfilename1, "%s%07i", rpStart.m_strPath.c_str(), rpStart.m_lStartOffset);
        sprintf(szfilename2, "%s%07i", rpEnd.m_strPath.c_str(), rpEnd.m_lStartOffset);
        break;

      case 9:  // Sort by share type
        if ( rpStart.m_iDriveType > rpEnd.m_iDriveType) return bGreater;
        if ( rpStart.m_iDriveType < rpEnd.m_iDriveType) return !bGreater;
        strcpy(szfilename1, rpStart.GetLabel());
        strcpy(szfilename2, rpEnd.GetLabel());
        break;

      default:  // Sort by Filename by default
        strcpy(szfilename1, rpStart.GetLabel().c_str());
        strcpy(szfilename2, rpEnd.GetLabel().c_str());
        break;
      }


      for (int i = 0; i < (int)strlen(szfilename1); i++)
        szfilename1[i] = tolower((unsigned char)szfilename1[i]);

      for (int i = 0; i < (int)strlen(szfilename2); i++)
      {
        szfilename2[i] = tolower((unsigned char)szfilename2[i]);
      }
      //return (rpStart.strPath.compare( rpEnd.strPath )<0);

      if (m_bSortAscending)
        return (strcmp(szfilename1, szfilename2) < 0);
      else
        return (strcmp(szfilename1, szfilename2) >= 0);
    }
    if (!rpStart.m_bIsFolder) return false;
    return true;
  }

  static int m_iSortMethod;
  static int m_bSortAscending;
  static CStdString m_strDirectory;
};
int SSortMusicSongs::m_iSortMethod;
int SSortMusicSongs::m_bSortAscending;
CStdString SSortMusicSongs::m_strDirectory;

CGUIWindowMusicSongs::CGUIWindowMusicSongs(void)
    : CGUIWindowMusicBase()
{
  m_Directory.m_strPath = "?";
  m_iViewAsIcons = -1;
  m_iViewAsIconsRoot = -1;

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
      /*
      // This window is started by the home window.
      // Now we decide which my music window has to be shown and
      // switch to the my music window the user last activated.
      if (g_stSettings.m_iMyMusicStartWindow!=GetID())
      {
       m_gWindowManager.ActivateWindow(g_stSettings.m_iMyMusicStartWindow);
       return false;
      }
      */

      // check for a passed destination path
      CStdString strDestination = message.GetStringParam();
      if (!strDestination.IsEmpty())
      {
        message.SetStringParam("");
        g_stSettings.m_iMyMusicStartWindow = GetID();
        CLog::Log(LOGINFO, "Attempting to quickpath to: %s", strDestination.c_str());
      }

      // unless we have a destination path, switch to the last my music window
      if (g_stSettings.m_iMyMusicStartWindow != GetID())
      {
        m_gWindowManager.ActivateWindow(g_stSettings.m_iMyMusicStartWindow);
        return false;
      }

      // is this the first time the window is opened?
      if (m_Directory.m_strPath == "?" && strDestination.IsEmpty())
      {
        m_Directory.m_strPath = strDestination = g_stSettings.m_szDefaultMusic;
        CLog::Log(LOGINFO, "Attempting to default to: %s", strDestination.c_str());
      }

      // try to open the destination path
      if (!strDestination.IsEmpty())
      {
        // default parameters if the jump fails
        m_Directory.m_strPath = "";

        bool bIsBookmarkName = false;
        int iIndex = CUtil::GetMatchingShare(strDestination, g_settings.m_vecMyMusicShares, bIsBookmarkName);
        if (iIndex > -1)
        {
          // set current directory to matching share
          if (bIsBookmarkName)
            m_Directory.m_strPath = g_settings.m_vecMyMusicShares[iIndex].strPath;
          else
            m_Directory.m_strPath = strDestination;
          CLog::Log(LOGINFO, "  Success! Opened destination path: %s", strDestination.c_str());
        }
        else
        {
          CLog::Log(LOGERROR, "  Failed! Destination parameter (%s) does not match a valid share!", strDestination.c_str());
        }

        // need file filters or GetDirectory in SetHistoryPath fails
        m_rootDir.SetMask(g_stSettings.m_szMyMusicExtensions);
        m_rootDir.SetShares(g_settings.m_vecMyMusicShares);

        SetHistoryForPath(m_Directory.m_strPath);
      }

      if (m_Directory.IsCDDA() || m_Directory.IsDVD() || m_Directory.IsISO9660())
      {
        // No disc in drive but current directory is a dvd share
        if (!CDetectDVDMedia::IsDiscInDrive())
          m_Directory.m_strPath.Empty();

        // look if disc has changed outside this window and url is still the same
        CFileItem dvdUrl;
        dvdUrl.m_strPath = m_rootDir.GetDVDDriveUrl();
        if (m_Directory.IsCDDA() && !dvdUrl.IsCDDA())
          m_Directory.m_strPath.Empty();
        if (m_Directory.IsDVD() && !dvdUrl.IsDVD())
          m_Directory.m_strPath.Empty();
        if (m_Directory.IsISO9660() && !dvdUrl.IsISO9660())
          m_Directory.m_strPath.Empty();
      }

      if (m_iViewAsIcons == -1 && m_iViewAsIconsRoot == -1)
      {
        m_iViewAsIcons = g_stSettings.m_iMyMusicSongsViewAsIcons;
        m_iViewAsIconsRoot = g_stSettings.m_iMyMusicSongsRootViewAsIcons;
      }

      return CGUIWindowMusicBase::OnMessage(message);

      /*
      if (bFirstTime)
      {
       // Set directory history for default path
       SetHistoryForPath(m_Directory.m_strPath);
       bFirstTime=false;
      }
      return true;
      */
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
        if (directory.m_strPath == m_Directory.m_strPath || strParent == m_Directory.m_strPath)
        {
          Update(m_Directory.m_strPath);
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
      //UpdateButtons();
      Sleep(200);
      Update(m_Directory.m_strPath);
    }
    break;

  case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();

      if (iControl == CONTROL_BTNSORTBY) // sort by
      {
        if (m_Directory.IsVirtualDirectoryRoot())
        {
          if (g_stSettings.m_iMyMusicSongsRootSortMethod == 0)
            g_stSettings.m_iMyMusicSongsRootSortMethod = 9;
          else
            g_stSettings.m_iMyMusicSongsRootSortMethod = 0;
        }
        else
        {
          g_stSettings.m_iMyMusicSongsSortMethod++;
          if (g_stSettings.m_iMyMusicSongsSortMethod >= 9) g_stSettings.m_iMyMusicSongsSortMethod = 0;
          if (g_stSettings.m_iMyMusicSongsSortMethod >= 3) g_stSettings.m_iMyMusicSongsSortMethod = 8;
        }
        g_settings.Save();

        UpdateButtons();
        UpdateListControl();

        int nItem = m_viewControl.GetSelectedItem();
        if (nItem < 0) break;
        CFileItem*pItem = m_vecItems[nItem];
        CStdString strSelected = pItem->m_strPath;

        CStdString strDirectory = m_Directory.m_strPath;
        if (CUtil::HasSlashAtEnd(strDirectory))
          strDirectory.Delete(strDirectory.size() - 1);
        if (!strDirectory.IsEmpty() && m_nTempPlayListWindow == GetID() && m_strTempPlayListDirectory == strDirectory && g_application.IsPlayingAudio() && g_playlistPlayer.GetCurrentPlaylist() == PLAYLIST_MUSIC_TEMP)
        {
          int nSong = g_playlistPlayer.GetCurrentSong();
          const CPlayList::CPlayListItem item = g_playlistPlayer.GetPlaylist(PLAYLIST_MUSIC_TEMP)[nSong];
          g_playlistPlayer.GetPlaylist(PLAYLIST_MUSIC_TEMP).Clear();
          g_playlistPlayer.Reset();
          int nFolderCount = 0;
          for (int i = 0; i < (int)m_vecItems.Size(); i++)
          {
            CFileItem* pItem = m_vecItems[i];
            if (pItem->m_bIsFolder)
            {
              nFolderCount++;
              continue;
            }
            CPlayList::CPlayListItem playlistItem ;
            CUtil::ConvertFileItemToPlayListItem(pItem, playlistItem);
            g_playlistPlayer.GetPlaylist(PLAYLIST_MUSIC_TEMP).Add(playlistItem);
            if (item.GetFileName() == pItem->m_strPath)
              g_playlistPlayer.SetCurrentSong(i - nFolderCount);
          }
        }

        for (int i = 0; i < (int)m_vecItems.Size(); i++)
        {
          CFileItem* pItem = m_vecItems[i];
          if (pItem->m_strPath == strSelected)
          {
            m_viewControl.SetSelectedItem(i);
            break;
          }
        }
      }
      else if (iControl == CONTROL_BTNVIEWASICONS)
      {
        CGUIWindowMusicBase::OnMessage(message);
        g_stSettings.m_iMyMusicSongsRootViewAsIcons = m_iViewAsIconsRoot;
        g_stSettings.m_iMyMusicSongsViewAsIcons = m_iViewAsIcons;
        g_settings.Save();
        return true;
      }
      else if (iControl == CONTROL_BTNSORTASC) // sort asc
      {
        if (m_Directory.IsVirtualDirectoryRoot())
          g_stSettings.m_bMyMusicSongsRootSortAscending = !g_stSettings.m_bMyMusicSongsRootSortAscending;
        else
          g_stSettings.m_bMyMusicSongsSortAscending = !g_stSettings.m_bMyMusicSongsSortAscending;

        g_settings.Save();
        UpdateButtons();
        UpdateListControl();
      }
      else if (iControl == CONTROL_BTNPLAYLISTS)
      {
        CStdString strDirectory;
        strDirectory.Format("%s\\playlists", g_stSettings.m_szAlbumDirectory);
        if (strDirectory != m_Directory.m_strPath)
        {
          Update(strDirectory);
        }
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

void CGUIWindowMusicSongs::GetDirectory(const CStdString &strDirectory, CFileItemList &items)
{
  if (items.Size() )
  {
    // cleanup items
    items.Clear();
  }

  CStdString strParentPath;
  bool bParentExists = CUtil::GetParentPath(strDirectory, strParentPath);

  CStdString strPlayListDir;
  strPlayListDir.Format("%s\\playlists", g_stSettings.m_szAlbumDirectory);
  if (strPlayListDir == strDirectory)
  {
    bParentExists = true;
    strParentPath = m_strPrevDir;
  }

  // check if current directory is a root share
  if ( !m_rootDir.IsShare(strDirectory) )
  {
    // no, do we got a parent dir?
    if ( bParentExists )
    {
      // yes
      if (!g_guiSettings.GetBool("MusicLists.HideParentDirItems"))
      {
        CFileItem *pItem = new CFileItem("..");
        pItem->m_strPath = strParentPath;
        pItem->m_bIsFolder = true;
        pItem->m_bIsShareOrDrive = false;
        items.Add(pItem);
      }
      m_strParentPath = strParentPath;
    }
  }
  else
  {
    // yes, this is the root of a share
    // add parent path to the virtual directory
    if (!g_guiSettings.GetBool("MusicLists.HideParentDirItems"))
    {
      CFileItem *pItem = new CFileItem("..");
      pItem->m_strPath = "";
      pItem->m_bIsShareOrDrive = false;
      pItem->m_bIsFolder = true;
      items.Add(pItem);
    }
    m_strParentPath = "";
  }
  m_rootDir.GetDirectory(strDirectory, items);

  // check for .CUE files here.
  items.FilterCueItems();

  if (strPlayListDir != strDirectory)
  {
    m_strPrevDir = strDirectory;
  }

}

void CGUIWindowMusicSongs::OnScan()
{
  if (g_application.m_guiDialogMusicScan.IsRunning())
  {
    g_application.m_guiDialogMusicScan.StopScanning();
    UpdateButtons();
    return ;
  }

  // check whether we have scanned here before
  bool bUpdateAll = false;
  CStdString strPaths;
  g_musicDatabase.GetSubpathsFromPath(m_Directory.m_strPath, strPaths);
  if (strPaths.length() > 2)
  { // yes, we have, we should prompt the user to ask if they want
    // to do a full scan, or just add new items...
    CGUIDialogYesNo *pDialog = &(g_application.m_guiDialogYesNo);
    pDialog->SetHeading(189);
    pDialog->SetLine(0, 702);
    pDialog->SetLine(1, 703);
    pDialog->SetLine(2, 704);
    pDialog->DoModal(GetID());
    if (pDialog->IsConfirmed()) bUpdateAll = true;
  }

  CUtil::DeleteDatabaseDirectoryCache();

  // Start background loader
  g_application.m_guiDialogMusicScan.StartScanning(m_Directory.m_strPath, bUpdateAll);
  UpdateButtons();
  return ;
}

void CGUIWindowMusicSongs::LoadPlayList(const CStdString& strPlayList)
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
      CGUIDialogOK* pDlgOK = (CGUIDialogOK*)m_gWindowManager.GetWindow(WINDOW_DIALOG_OK);
      if (pDlgOK)
      {
        pDlgOK->SetHeading(6);
        pDlgOK->SetLine(0, L"");
        pDlgOK->SetLine(1, 477);
        pDlgOK->SetLine(2, L"");
        pDlgOK->DoModal(GetID());
      }
      return ; //hmmm unable to load playlist?
    }
  }

  if (g_application.ProcessAndStartPlaylist(strPlayList, *pPlayList, PLAYLIST_MUSIC))
  {
    // activate the playlist window if its not activated yet
    if (GetID() == m_gWindowManager.GetActiveWindow())
    {
      m_gWindowManager.ActivateWindow(WINDOW_MUSIC_PLAYLIST);
    }
  }
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
  if (m_Directory.IsVirtualDirectoryRoot() || m_Directory.IsShoutCast())
  {
    CONTROL_DISABLE(CONTROL_BTNSCAN);
  }
  else
  {
    CONTROL_ENABLE(CONTROL_BTNSCAN);
  }

  if (g_application.m_guiDialogMusicScan.IsRunning())
  {
    SET_CONTROL_LABEL(CONTROL_BTNSCAN, 14056); // Stop Scan
  }
  else
  {
    SET_CONTROL_LABEL(CONTROL_BTNSCAN, 102); // Scan
  }

  // Update sorting control
  bool bSortAscending = false;
  if (m_Directory.IsVirtualDirectoryRoot())
    bSortAscending = g_stSettings.m_bMyMusicSongsRootSortAscending;
  else
    bSortAscending = g_stSettings.m_bMyMusicSongsSortAscending;

  if (bSortAscending)
  {
    CGUIMessage msg(GUI_MSG_DESELECTED, GetID(), CONTROL_BTNSORTASC);
    g_graphicsContext.SendMessage(msg);
  }
  else
  {
    CGUIMessage msg(GUI_MSG_SELECTED, GetID(), CONTROL_BTNSORTASC);
    g_graphicsContext.SendMessage(msg);
  }

  if ( m_Directory.IsVirtualDirectoryRoot() )
    m_viewControl.SetCurrentView(m_iViewAsIconsRoot);
  else
    m_viewControl.SetCurrentView(m_iViewAsIcons);

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

  // Update sort by button
  if (m_Directory.IsVirtualDirectoryRoot())
  {
    if (g_stSettings.m_iMyMusicSongsRootSortMethod == 0)
    {
      SET_CONTROL_LABEL(CONTROL_BTNSORTBY, g_stSettings.m_iMyMusicSongsRootSortMethod + 103);
    }
    else
    {
      SET_CONTROL_LABEL(CONTROL_BTNSORTBY, 498); // Sort by: Type
    }
  }
  else
  {
    if (g_stSettings.m_iMyMusicSongsSortMethod <= 2)
    {
      // Sort by Name(ItemLabel), Date, Size
      SET_CONTROL_LABEL(CONTROL_BTNSORTBY, g_stSettings.m_iMyMusicSongsSortMethod + 103);
    }
    else
    {
      // Sort by FileName
      SET_CONTROL_LABEL(CONTROL_BTNSORTBY, 363);
    }
  }
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
    Update(strPath);
  }
  else if (pItem->IsZIP()) // mount zip archive
  {
    CShare shareZip;
    shareZip.strPath.Format("zip://Z:\\,%i,,%s,\\",1, pItem->m_strPath.c_str() );
    m_rootDir.AddShare(shareZip);
    Update(shareZip.strPath);
  }
  else if (pItem->IsRAR()) // mount rar archive
  {
    CShare shareRar;
    shareRar.strPath.Format("rar://Z:\\,%i,,%s,\\",EXFILE_AUTODELETE|EXFILE_OVERWRITE, pItem->m_strPath.c_str() );
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
      if (g_guiSettings.GetBool("MyMusic.AutoPlayNextItem"))
      {
        //play and add current directory to temporary playlist
        int nFolderCount = 0;
        g_playlistPlayer.GetPlaylist( PLAYLIST_MUSIC_TEMP ).Clear();
        g_playlistPlayer.Reset();
        int iNoSongs = 0;
        for ( int i = 0; i < m_vecItems.Size(); i++ )
        {
          CFileItem* pItem = m_vecItems[i];
          if ( pItem->m_bIsFolder )
          {
            nFolderCount++;
            continue;
          }
          if (!pItem->IsPlayList())
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
        m_strTempPlayListDirectory = m_Directory.m_strPath;
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
    }
  }
}

void CGUIWindowMusicSongs::OnFileItemFormatLabel(CFileItem* pItem)
{
  // set label 1 & 2 from format string
  if (pItem->m_musicInfoTag.Loaded())
  {
    SetLabelFromTag(pItem);
  }
  else
  { // no tag info, so we disable the file extension if it has one
    if (g_guiSettings.GetBool("FileLists.HideExtensions"))
      pItem->RemoveExtension();

    // and then set label 2
    int nMyMusicSortMethod = 0;
    if (m_Directory.IsVirtualDirectoryRoot())
      nMyMusicSortMethod = g_stSettings.m_iMyMusicSongsRootSortMethod;
    else
      nMyMusicSortMethod = g_stSettings.m_iMyMusicSongsSortMethod;

    if (nMyMusicSortMethod == 0 || nMyMusicSortMethod == 2 || nMyMusicSortMethod == 8)
    {
      if (pItem->m_bIsFolder)
      {
        if (!pItem->IsShoutCast())
          pItem->SetLabel2("");
      }
      else
      {
        if (pItem->m_dwSize > 0)
        {
          CStdString strFileSize;
          CUtil::GetFileSize(pItem->m_dwSize, strFileSize);
          pItem->SetLabel2(strFileSize);
        }
        if (nMyMusicSortMethod == 0 || nMyMusicSortMethod == 8)
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
      if (pItem->m_stTime.wYear && (!pItem->IsShoutCast()))
      {
        CStdString strDateTime;
        CUtil::GetDate(pItem->m_stTime, strDateTime);
        pItem->SetLabel2(strDateTime);
      }
      else if (!pItem->IsShoutCast())
        pItem->SetLabel2("");
    }
  }

  // set thumbs and default icons
  if (!pItem->m_bIsShareOrDrive)
  {
    pItem->SetMusicThumb();
    pItem->FillInDefaultIcon();
  }
}

void CGUIWindowMusicSongs::DoSort(CFileItemList& items)
{
  SSortMusicSongs::m_strDirectory = m_Directory.m_strPath;

  if (m_Directory.IsVirtualDirectoryRoot())
  {
    SSortMusicSongs::m_iSortMethod = g_stSettings.m_iMyMusicSongsRootSortMethod;
    SSortMusicSongs::m_bSortAscending = g_stSettings.m_bMyMusicSongsRootSortAscending;
  }
  else
  {
    SSortMusicSongs::m_iSortMethod = g_stSettings.m_iMyMusicSongsSortMethod;
    SSortMusicSongs::m_bSortAscending = g_stSettings.m_bMyMusicSongsSortAscending;
  }

  items.Sort(SSortMusicSongs::Sort);
}

void CGUIWindowMusicSongs::OnRetrieveMusicInfo(CFileItemList& items)
{
  int nFolderCount = items.GetFolderCount();
  // Skip items with folders only
  if (nFolderCount == (int)items.Size())
    return ;

  MAPSONGS songsMap;
  // get all information for all files in current directory from database
  g_musicDatabase.GetSongsByPath(m_Directory.m_strPath, songsMap);

  // Nothing in database and id3 tags disabled; dont load tags from cdda files
  if (songsMap.size() == 0 && !g_guiSettings.GetBool("MyMusic.UseTags"))
    return ;

  // Do we have cached items
  MAPFILEITEMS itemsMap;
  LoadDirectoryCache(m_Directory.m_strPath, itemsMap);

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
      else if (g_guiSettings.GetBool("MyMusic.UseTags"))
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
          CURL url(m_Directory.m_strPath);
          CStdString strStrippedPath;
          url.GetURLWithoutUserDetails(strStrippedPath);
          m_dlgProgress->SetHeading(189);
          m_dlgProgress->SetLine(0, 505);
          m_dlgProgress->SetLine(1, "");
          m_dlgProgress->SetLine(2, strStrippedPath );
          m_dlgProgress->StartModal(GetID());
          m_dlgProgress->ShowProgressBar(true);
          m_dlgProgress->SetPercentage((i*100) / items.Size());
          m_dlgProgress->Progress();
          bProgressVisible = true;
        }
      }
    }

    if (bProgressVisible && ((i % 10) == 0 || i == items.Size() - 1))
    {
      m_dlgProgress->SetPercentage((i*100) / items.Size());
      m_dlgProgress->Progress();
    }

    // Progress key presses from controller or remote
    if (bProgressVisible && m_dlgProgress)
      m_dlgProgress->ProgressKeys();

    // Canceled by the user, finish
    if (bProgressVisible && m_dlgProgress && m_dlgProgress->IsCanceled())
      break;

  } // for (int i=0; i < (int)items.size(); ++i)

  // Save the hdd cache if there are more songs in this directory then loaded from database
  if ((m_dlgProgress && !m_dlgProgress->IsCanceled()) && songsMap.size() != (items.Size() - iTaglessFiles))
    SaveDirectoryCache(m_Directory.m_strPath, items);

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

void CGUIWindowMusicSongs::OnSearchItemFound(const CFileItem* pSelItem)
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

    CStdString strParentPath;
    while (CUtil::GetParentPath(strPath, strParentPath))
    {
      m_history.Set(strPath, strParentPath);
      strPath = strParentPath;
    }
    m_history.Set(strPath, "");

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

void CGUIWindowMusicSongs::Update(const CStdString &strDirectory)
{
  CGUIWindowMusicBase::Update(strDirectory);
  if (!m_Directory.IsVirtualDirectoryRoot() && g_guiSettings.GetBool("MusicLists.UseAutoSwitching"))
  {
    m_iViewAsIcons = CAutoSwitch::GetView(m_vecItems);
    UpdateButtons();
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

void CGUIWindowMusicSongs::SetHistoryForPath(const CStdString& strDirectory)
{
  if (!strDirectory.IsEmpty())
  {
    // Build the directory history for default path
    CStdString strPath, strParentPath;
    strPath = strDirectory;
    CFileItemList items;
    GetDirectory("", items);

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
          m_history.Set(strHistory, "");
          return ;
        }
      }

      m_history.Set(strPath, strParentPath);
      strPath = strParentPath;
      while (CUtil::HasSlashAtEnd(strPath))
        strPath.Delete(strPath.size() - 1);
    }
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
  if ( m_Directory.IsVirtualDirectoryRoot() )
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
      Update(m_Directory.m_strPath);
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
  if (directory.IsCDDA() || directory.IsDVD() || directory.IsISO9660())
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
  if (directory.IsCDDA() || directory.IsDVD() || directory.IsISO9660())
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
  if (m_Directory.m_strPath.IsEmpty())
    return;

  CGUIWindowMusicBase::PlayItem(iItem);
}