
#include "stdafx.h"
#include "GUIWindowMusicSongs.h"
#include "Util.h"
#include "Application.h"
#include "CUEDocument.h"
#include "GUIPassword.h"
#include "GUIDialogMusicScan.h"
#include "GUIDialogContextMenu.h"

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
        m_history.ClearPathHistory();
      }

      // is this the first time the window is opened?
      if (m_vecItems.m_strPath == "?" && strDestination.IsEmpty())
      {
        strDestination = g_stSettings.m_szDefaultMusic;
        m_vecItems.m_strPath=strDestination;
        CLog::Log(LOGINFO, "Attempting to default to: %s", strDestination.c_str());
      }

      m_rootDir.SetMask(g_stSettings.m_musicExtensions);
      m_rootDir.SetShares(g_settings.m_vecMyMusicShares);

      // try to open the destination path
      if (!strDestination.IsEmpty())
      {
        // open playlists location
        if (strDestination.Equals("$PLAYLISTS"))
        {
          m_vecItems.m_strPath = CUtil::MusicPlaylistsLocation();
          CLog::Log(LOGINFO, "  Success! Opening destination path: %s", m_vecItems.m_strPath.c_str());
        }
        else
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
            CUtil::RemoveSlashAtEnd(m_vecItems.m_strPath);
            CLog::Log(LOGINFO, "  Success! Opened destination path: %s", strDestination.c_str());
          }
          else
          {
            CLog::Log(LOGERROR, "  Failed! Destination parameter (%s) does not match a valid share!", strDestination.c_str());
          }
        }

        // need file filters or GetDirectory in SetHistoryPath fails
        SetHistoryForPath(m_vecItems.m_strPath);
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

  case GUI_MSG_NOTIFY_ALL:
    {
      if (message.GetParam1()==GUI_MSG_DVDDRIVE_EJECTED_CD)
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
  m_musicdatabase.GetSubpathsFromPath(m_vecItems.m_strPath, strPaths);
  if (strPaths.length() > 2)
  { // yes, we have, we should prompt the user to ask if they want
    // to do a full scan, or just add new items...
    if (CGUIDialogYesNo::ShowAndGetInput(189, 702, 0, 0,20024,20025))
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

bool CGUIWindowMusicSongs::GetDirectory(const CStdString &strDirectory, CFileItemList &items)
{
  if (!CGUIWindowMusicBase::GetDirectory(strDirectory, items))
    return false;

  // check for .CUE files here.
  items.FilterCueItems();

  return true;
}

void CGUIWindowMusicSongs::OnPrepareFileItems(CFileItemList &items)
{
  RetrieveMusicInfo();

  items.SetMusicThumbs();
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
  CStdString items;
  items.Format("%i %s", iItems, g_localizeStrings.Get(127).c_str());
  SET_CONTROL_LABEL(CONTROL_LABELFILES, items);
}

void CGUIWindowMusicSongs::OnRetrieveMusicInfo(CFileItemList& items)
{
  if (items.GetFolderCount()==items.Size() || items.IsMusicDb() || !g_guiSettings.GetBool("musicfiles.usetags"))
    return;

  // Start the music info loader thread
  m_musicInfoLoader.SetProgressCallback(m_dlgProgress);
  m_musicInfoLoader.Load(items);

  bool bShowProgress=!m_gWindowManager.IsRouted();
  bool bProgressVisible=false;

  DWORD dwTick=timeGetTime();

  while (m_musicInfoLoader.IsLoading())
  {
    if (bShowProgress)
    { // Do we have to init a progress dialog?
      DWORD dwElapsed=timeGetTime()-dwTick;

      if (!bProgressVisible && dwElapsed>1500 && m_dlgProgress)
      { // tag loading takes more then 1.5 secs, show a progress dialog
        CURL url(m_vecItems.m_strPath);
        CStdString strStrippedPath;
        url.GetURLWithoutUserDetails(strStrippedPath);
        m_dlgProgress->SetHeading(189);
        m_dlgProgress->SetLine(0, 505);
        m_dlgProgress->SetLine(1, "");
        m_dlgProgress->SetLine(2, strStrippedPath );
        m_dlgProgress->StartModal();
        m_dlgProgress->ShowProgressBar(true);
        bProgressVisible = true;
      }

      if (bProgressVisible && m_dlgProgress)
      { // keep GUI alive
        m_dlgProgress->Progress();
      }
    } // if (bShowProgress)
    Sleep(1);
  } // while (m_musicInfoLoader.IsLoading())

  if (bProgressVisible && m_dlgProgress)
    m_dlgProgress->Close();
}

/// \brief Search for a song or a artist with search string \e strSearch in the musicdatabase and return the found \e items
/// \param strSearch The search string
/// \param items Items Found
void CGUIWindowMusicSongs::DoSearch(const CStdString& strSearch, CFileItemList& items)
{
  VECALBUMS albums;
  m_musicdatabase.FindAlbumsByName(strSearch, albums);

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
  m_musicdatabase.FindSongsByNameAndArtist(strSearch, songs);

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

    // and do the popup menu
    if (CGUIDialogContextMenu::BookmarksMenu("music", m_vecItems[iItem], iPosX, iPosY))
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
