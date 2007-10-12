/*
 *      Copyright (C) 2005-2007 Team XboxMediaCenter
 *      http://www.xboxmediacenter.com
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include "GUIWindowMusicSongs.h"
#include "Util.h"
#include "utils/GUIInfoManager.h"
#include "Application.h"
#include "CueDocument.h"
#include "GUIPassword.h"
#include "GUIDialogMusicScan.h"

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

  m_thumbLoader.SetObserver(this);
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
  case GUI_MSG_WINDOW_DEINIT:
    if (m_thumbLoader.IsLoading())
      m_thumbLoader.StopThread();
    break;
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
        strDestination = g_settings.m_defaultMusicSource;
        m_vecItems.m_strPath=strDestination;
        CLog::Log(LOGINFO, "Attempting to default to: %s", strDestination.c_str());
      }

      // try to open the destination path
      if (!strDestination.IsEmpty())
      {
        // open root
        if (strDestination.Equals("$ROOT"))
        {
          m_vecItems.m_strPath = "";
          CLog::Log(LOGINFO, "  Success! Opening root listing.");
        }
        // open playlists location
        else if (strDestination.Equals("$PLAYLISTS"))
        {
          m_vecItems.m_strPath = "special://musicplaylists/";
          CLog::Log(LOGINFO, "  Success! Opening destination path: %s", m_vecItems.m_strPath.c_str());
        }
        else
        {
          // default parameters if the jump fails
          m_vecItems.m_strPath.Empty();

          bool bIsSourceName = false;

          SetupShares();
          VECSHARES shares;
          m_rootDir.GetShares(shares);
          int iIndex = CUtil::GetMatchingShare(strDestination, shares, bIsSourceName);
          if (iIndex > -1)
          {
            bool unlocked = true;
            if (shares[iIndex].m_iHasLock == 2)
            {
              CFileItem item(shares[iIndex]);
              if (!g_passwordManager.IsItemUnlocked(&item,"music"))
              {
                m_vecItems.m_strPath = ""; // no u don't
                unlocked = false;
                CLog::Log(LOGINFO, "  Failure! Failed to unlock destination path: %s", strDestination.c_str());
              }
            }
            // set current directory to matching share
            if (unlocked)
            {
              if (bIsSourceName)
                m_vecItems.m_strPath=shares[iIndex].strPath;
              else
                m_vecItems.m_strPath=strDestination;
              CLog::Log(LOGINFO, "  Success! Opened destination path: %s (%s)", strDestination.c_str(), m_vecItems.m_strPath.c_str());
            }
          }
          else
          {
            CLog::Log(LOGERROR, "  Failed! Destination parameter (%s) does not match a valid source!", strDestination.c_str());
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
      if (message.GetParam1()==GUI_MSG_REMOVED_MEDIA)
        DeleteRemoveableMediaDirectoryCache();
    }
    break;

  case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();

      if (iControl == CONTROL_BTNPLAYLISTS)
      {
        if (!m_vecItems.m_strPath.Equals("special://musicplaylists/"))
          Update("special://musicplaylists/");
      }
      else if (iControl == CONTROL_BTNSCAN)
      {
        OnScan(-1);
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

void CGUIWindowMusicSongs::OnScan(int iItem)
{
  CStdString strPath;
  if (iItem < 0 || iItem >= m_vecItems.Size())
    strPath = m_vecItems.m_strPath;
  else if (m_vecItems[iItem]->m_bIsFolder)
    strPath = m_vecItems[iItem]->m_strPath;
  else
  { // TODO: MUSICDB - should we allow scanning a single item into the database?
    //       This will require changes to the info scanner, which assumes we're running on a folder
    strPath = m_vecItems.m_strPath;
  }
  DoScan(strPath);
}

void CGUIWindowMusicSongs::DoScan(const CStdString &strPath)
{
  CGUIDialogMusicScan *musicScan = (CGUIDialogMusicScan *)m_gWindowManager.GetWindow(WINDOW_DIALOG_MUSIC_SCAN);
  if (musicScan && musicScan->IsScanning())
  {
    musicScan->StopScanning();
    return ;
  }

  // Start background loader
  int iControl=GetFocusedControlID();
  if (musicScan) musicScan->StartScanning(strPath);
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

  items.SetCachedMusicThumbs();
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
  static int iOldLeftControl=-1;
  if (m_vecItems.IsShoutCast() || m_vecItems.IsLastFM())
  {
    CONTROL_DISABLE(CONTROL_BTNVIEWASICONS);
    CGUIControl* pControl = (CGUIControl*)GetControl(CONTROL_LIST);
    if (pControl)
      if (pControl->GetControlIdLeft() == CONTROL_BTNVIEWASICONS)
      {
        iOldLeftControl = pControl->GetControlIdLeft();
        pControl->SetNavigation(pControl->GetControlIdUp(),pControl->GetControlIdDown(),CONTROL_BTNSORTBY,pControl->GetControlIdRight());
      }
  }
  else
  {
    CONTROL_ENABLE(CONTROL_BTNVIEWASICONS);
    if (iOldLeftControl != -1)
    {
      CGUIControl* pControl = (CGUIControl*)GetControl(CONTROL_LIST);
      if (pControl)
        pControl->SetNavigation(pControl->GetControlIdUp(),pControl->GetControlIdDown(),CONTROL_BTNVIEWASICONS,pControl->GetControlIdRight());
    }
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

void CGUIWindowMusicSongs::GetContextButtons(int itemNumber, CContextButtons &buttons)
{
  CFileItem *item = (itemNumber >= 0 && itemNumber < m_vecItems.Size()) ? m_vecItems[itemNumber] : NULL;

  if (item)
  {
    // are we in the playlists location?
    bool inPlaylists = m_vecItems.m_strPath.Equals(CUtil::MusicPlaylistsLocation()) || m_vecItems.m_strPath.Equals("special://musicplaylists/");

    if (m_vecItems.IsVirtualDirectoryRoot())
    {
      // get the usual music shares, and anything for all media windows
      CShare *share = CGUIDialogContextMenu::GetShare("music", item);
      CGUIDialogContextMenu::GetContextButtons("music", share, buttons);
      // enable Rip CD an audio disc
      if (CDetectDVDMedia::IsDiscInDrive() && item->IsCDDA())
      {
        // those cds can also include Audio Tracks: CDExtra and MixedMode!
        CCdInfo *pCdInfo = CDetectDVDMedia::GetCdInfo();
        if ( pCdInfo->IsAudio(1) || pCdInfo->IsCDExtra(1) || pCdInfo->IsMixedMode(1) )
          buttons.Add(CONTEXT_BUTTON_RIP_CD, 600);
      }
      CGUIMediaWindow::GetContextButtons(itemNumber, buttons);
    }
    else
    {
      CGUIWindowMusicBase::GetContextButtons(itemNumber, buttons);
      if (!item->IsPlayList())
      {
        if (item->IsAudio())
          buttons.Add(CONTEXT_BUTTON_SONG_INFO, 658); // Song Info
        else if (!item->IsParentFolder())
          buttons.Add(CONTEXT_BUTTON_INFO, 13351); // Album Info
      }

      // enable Rip CD Audio or Track button if we have an audio disc
      if (CDetectDVDMedia::IsDiscInDrive() && m_vecItems.IsCDDA())
      {
        // those cds can also include Audio Tracks: CDExtra and MixedMode!
        CCdInfo *pCdInfo = CDetectDVDMedia::GetCdInfo();
        if ( pCdInfo->IsAudio(1) || pCdInfo->IsCDExtra(1) || pCdInfo->IsMixedMode(1) )
          buttons.Add(CONTEXT_BUTTON_RIP_TRACK, 610);
      }

      // enable CDDB lookup if the current dir is CDDA
      if (CDetectDVDMedia::IsDiscInDrive() && m_vecItems.IsCDDA() && (g_settings.m_vecProfiles[g_settings.m_iLastLoadedProfileIndex].canWriteDatabases() || g_passwordManager.bMasterUser))
        buttons.Add(CONTEXT_BUTTON_CDDB, 16002);

      if (!item->IsParentFolder() && !item->IsReadOnly())
      {
        // either we're at the playlist location or its been explicitly allowed
        if (inPlaylists || g_guiSettings.GetBool("filelists.allowfiledeletion"))
        {
          buttons.Add(CONTEXT_BUTTON_DELETE, 117);
          buttons.Add(CONTEXT_BUTTON_RENAME, 118);
        }
      }
    }

    // Add the scan button(s)
    CGUIDialogMusicScan *pScanDlg = (CGUIDialogMusicScan *)m_gWindowManager.GetWindow(WINDOW_DIALOG_MUSIC_SCAN);
    if (pScanDlg)
    {
      if (pScanDlg->IsScanning())
        buttons.Add(CONTEXT_BUTTON_STOP_SCANNING, 13353);	// Stop Scanning
      else if (!inPlaylists && !m_vecItems.IsInternetStream() &&
              (g_settings.m_vecProfiles[g_settings.m_iLastLoadedProfileIndex].canWriteDatabases() || g_passwordManager.bMasterUser))
        buttons.Add(CONTEXT_BUTTON_SCAN, 13352);
    }
  }
  if (!m_vecItems.IsVirtualDirectoryRoot())
    buttons.Add(CONTEXT_BUTTON_SWITCH_MEDIA, 523);
  CGUIWindowMusicBase::GetNonContextButtons(buttons);
}

bool CGUIWindowMusicSongs::OnContextButton(int itemNumber, CONTEXT_BUTTON button)
{
  CFileItem *item = (itemNumber >= 0 && itemNumber < m_vecItems.Size()) ? m_vecItems[itemNumber] : NULL;
  if ( m_vecItems.IsVirtualDirectoryRoot() && item)
  {
    CShare *share = CGUIDialogContextMenu::GetShare("music", item);
    if (CGUIDialogContextMenu::OnContextButton("music", share, button))
    {
      Update("");
      return true;
    }
  }

  switch (button)
  {
  case CONTEXT_BUTTON_SCAN:
    OnScan(itemNumber);
    return true;

  case CONTEXT_BUTTON_RIP_TRACK:
    OnRipTrack(itemNumber);
    return true;

  case CONTEXT_BUTTON_RIP_CD:
    OnRipCD();
    return true;

  case CONTEXT_BUTTON_CDDB:
    if (m_musicdatabase.LookupCDDBInfo(true))
      Update(m_vecItems.m_strPath);
    return true;

  case CONTEXT_BUTTON_DELETE:
    OnDeleteItem(itemNumber);
    return true;

  case CONTEXT_BUTTON_RENAME:
    OnRenameItem(itemNumber);
    return true;

  case CONTEXT_BUTTON_SWITCH_MEDIA:
		CGUIDialogContextMenu::SwitchMedia("music", m_vecItems.m_strPath);
		return true;
  default:
    break;
  }
  return CGUIWindowMusicBase::OnContextButton(itemNumber, button);
}

void CGUIWindowMusicSongs::DeleteDirectoryCache()
{
  WIN32_FIND_DATA wfd;
  memset(&wfd, 0, sizeof(wfd));

  CStdString searchPath = _P("Z:\\*.fi");
  CAutoPtrFind hFind( FindFirstFile(searchPath.c_str(), &wfd));
  if (!hFind.isValid())
    return ;
  do
  {
    if ( !(wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) )
    {
      CStdString strFile = _P("Z:\\");
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

  CStdString searchPath = _P("Z:\\r-*.fi");
  CAutoPtrFind hFind( FindFirstFile(searchPath.c_str(), &wfd));
  if (!hFind.isValid())
    return ;
  do
  {
    if ( !(wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) )
    {
      CStdString strFile = _P("Z:\\");
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

  // we're at the root source listing
  if (m_vecItems.IsVirtualDirectoryRoot() && !m_vecItems[iItem]->IsDVD())
    return;

  if (m_vecItems[iItem]->IsDVD())
    CAutorun::PlayDisc();
  else
    CGUIWindowMusicBase::PlayItem(iItem);
}

bool CGUIWindowMusicSongs::Update(const CStdString &strDirectory)
{
  if (m_thumbLoader.IsLoading())
    m_thumbLoader.StopThread();

  if (!CGUIMediaWindow::Update(strDirectory))
    return false;

  g_infoManager.m_content = "files";
  m_thumbLoader.Load(m_vecItems);
  return true;
}
