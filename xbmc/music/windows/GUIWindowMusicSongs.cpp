/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "system.h"
#include "GUIWindowMusicSongs.h"
#include "Util.h"
#include "GUIInfoManager.h"
#include "Application.h"
#include "CueDocument.h"
#include "GUIPassword.h"
#include "music/dialogs/GUIDialogMusicScan.h"
#include "dialogs/GUIDialogYesNo.h"
#include "GUIUserMessages.h"
#include "guilib/GUIWindowManager.h"
#include "FileItem.h"
#include "storage/MediaManager.h"
#include "settings/Settings.h"
#include "settings/GUISettings.h"
#include "guilib/LocalizeStrings.h"
#include "utils/log.h"
#include "utils/URIUtils.h"
#include "Autorun.h"

#define CONTROL_BTNVIEWASICONS     2
#define CONTROL_BTNSORTBY          3
#define CONTROL_BTNSORTASC         4
#define CONTROL_BTNTYPE            5
#define CONTROL_LABELFILES        12

#define CONTROL_BTNPLAYLISTS       7
#define CONTROL_BTNSCAN            9
#define CONTROL_BTNREC            10
#define CONTROL_BTNRIP            11


CGUIWindowMusicSongs::CGUIWindowMusicSongs(void)
    : CGUIWindowMusicBase(WINDOW_MUSIC_FILES, "MyMusicSongs.xml")
{
  m_vecItems->SetPath("?");

  m_thumbLoader.SetObserver(this);
  // Remove old HD cache every time XBMC is loaded
  CUtil::DeleteDirectoryCache();
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

      // is this the first time the window is opened?
      if (m_vecItems->GetPath() == "?" && message.GetStringParam().IsEmpty())
        message.SetStringParam(g_settings.m_defaultMusicSource);

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
        URIUtils::GetParentPath(directory.GetPath(), strParent);
        if (directory.GetPath() == m_vecItems->GetPath() || strParent == m_vecItems->GetPath())
        {
          Update(m_vecItems->GetPath());
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
        if (!m_vecItems->GetPath().Equals("special://musicplaylists/"))
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

bool CGUIWindowMusicSongs::OnAction(const CAction& action)
{
  if (action.GetID() == ACTION_SCAN_ITEM)
  {
    int item = m_viewControl.GetSelectedItem();
    if (item > -1 && m_vecItems->Get(item)->m_bIsFolder)
      OnScan(item);

    return true;
  }

  return CGUIWindowMusicBase::OnAction(action);
}

void CGUIWindowMusicSongs::OnScan(int iItem)
{
  CStdString strPath;
  if (iItem < 0 || iItem >= m_vecItems->Size())
    strPath = m_vecItems->GetPath();
  else if (m_vecItems->Get(iItem)->m_bIsFolder)
    strPath = m_vecItems->Get(iItem)->GetPath();
  else
  { // TODO: MUSICDB - should we allow scanning a single item into the database?
    //       This will require changes to the info scanner, which assumes we're running on a folder
    strPath = m_vecItems->GetPath();
  }
  DoScan(strPath);
}

void CGUIWindowMusicSongs::DoScan(const CStdString &strPath)
{
  CGUIDialogMusicScan *musicScan = (CGUIDialogMusicScan *)g_windowManager.GetWindow(WINDOW_DIALOG_MUSIC_SCAN);
  if (musicScan && musicScan->IsScanning())
  {
    musicScan->StopScanning();
    return;
  }

  // Start background loader
  int iControl=GetFocusedControlID();
  if (musicScan) musicScan->StartScanning(strPath);
  SET_CONTROL_FOCUS(iControl, 0);
  UpdateButtons();

  return;
}

bool CGUIWindowMusicSongs::GetDirectory(const CStdString &strDirectory, CFileItemList &items)
{
  if (!CGUIWindowMusicBase::GetDirectory(strDirectory, items))
    return false;

  // check for .CUE files here.
  items.FilterCueItems();

  CStdString label;
  if (items.GetLabel().IsEmpty() && m_rootDir.IsSource(items.GetPath(), g_settings.GetSourcesFromType("music"), &label)) 
    items.SetLabel(label);

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
  if (g_mediaManager.IsAudio())
  {
    CONTROL_ENABLE(CONTROL_BTNRIP);
  }
  else
  {
    CONTROL_DISABLE(CONTROL_BTNRIP);
  }

  // Disable scan button if shoutcast
  if (m_vecItems->IsVirtualDirectoryRoot() ||
      m_vecItems->IsLastFM() || m_vecItems->IsMusicDb())
  {
    CONTROL_DISABLE(CONTROL_BTNSCAN);
  }
  else
  {
    CONTROL_ENABLE(CONTROL_BTNSCAN);
  }

  CGUIDialogMusicScan *musicScan = (CGUIDialogMusicScan *)g_windowManager.GetWindow(WINDOW_DIALOG_MUSIC_SCAN);
  if (musicScan && musicScan->IsScanning())
  {
    SET_CONTROL_LABEL(CONTROL_BTNSCAN, 14056); // Stop Scan
  }
  else
  {
    SET_CONTROL_LABEL(CONTROL_BTNSCAN, 102); // Scan
  }

  // Update object count label
  CStdString items;
  items.Format("%i %s", m_vecItems->GetObjectCount(), g_localizeStrings.Get(127).c_str());
  SET_CONTROL_LABEL(CONTROL_LABELFILES, items);
}

void CGUIWindowMusicSongs::GetContextButtons(int itemNumber, CContextButtons &buttons)
{
  CFileItemPtr item;
  if (itemNumber >= 0 && itemNumber < m_vecItems->Size())
    item = m_vecItems->Get(itemNumber);

  if (item)
  {
    // are we in the playlists location?
    bool inPlaylists = m_vecItems->GetPath().Equals(CUtil::MusicPlaylistsLocation()) ||
                       m_vecItems->GetPath().Equals("special://musicplaylists/");

    if (m_vecItems->IsVirtualDirectoryRoot())
    {
      // get the usual music shares, and anything for all media windows
      CGUIDialogContextMenu::GetContextButtons("music", item, buttons);
#ifdef HAS_DVD_DRIVE
      // enable Rip CD an audio disc
      if (g_mediaManager.IsDiscInDrive() && item->IsCDDA())
      {
        // those cds can also include Audio Tracks: CDExtra and MixedMode!
        CCdInfo *pCdInfo = g_mediaManager.GetCdInfo();
        if (pCdInfo->IsAudio(1) || pCdInfo->IsCDExtra(1) || pCdInfo->IsMixedMode(1))
          buttons.Add(CONTEXT_BUTTON_RIP_CD, 600);
      }
#endif
      CGUIMediaWindow::GetContextButtons(itemNumber, buttons);
    }
    else
    {
      CGUIWindowMusicBase::GetContextButtons(itemNumber, buttons);
      if (item->GetProperty("pluginreplacecontextitems").asBoolean())
        return;
      if (!item->IsPlayList())
      {
        if (item->IsAudio() && !item->IsLastFM())
          buttons.Add(CONTEXT_BUTTON_SONG_INFO, 658); // Song Info
        else if (!item->IsParentFolder() && !item->IsLastFM() &&
                 !item->GetPath().Left(3).Equals("new") && item->m_bIsFolder)
        {
#if 0
          if (m_musicdatabase.GetAlbumIdByPath(item->GetPath()) > -1)
#endif
            buttons.Add(CONTEXT_BUTTON_INFO, 13351); // Album Info
        }
      }

#ifdef HAS_DVD_DRIVE
      // enable Rip CD Audio or Track button if we have an audio disc
      if (g_mediaManager.IsDiscInDrive() && m_vecItems->IsCDDA())
      {
        // those cds can also include Audio Tracks: CDExtra and MixedMode!
        CCdInfo *pCdInfo = g_mediaManager.GetCdInfo();
        if (pCdInfo->IsAudio(1) || pCdInfo->IsCDExtra(1) || pCdInfo->IsMixedMode(1))
          buttons.Add(CONTEXT_BUTTON_RIP_TRACK, 610);
      }
#endif

      // enable CDDB lookup if the current dir is CDDA
      if (g_mediaManager.IsDiscInDrive() && m_vecItems->IsCDDA() &&
         (g_settings.GetCurrentProfile().canWriteDatabases() || g_passwordManager.bMasterUser))
      {
        buttons.Add(CONTEXT_BUTTON_CDDB, 16002);
      }

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
    CGUIDialogMusicScan *pScanDlg = (CGUIDialogMusicScan *)g_windowManager.GetWindow(WINDOW_DIALOG_MUSIC_SCAN);
    if (pScanDlg)
    {
      if (pScanDlg->IsScanning())
        buttons.Add(CONTEXT_BUTTON_STOP_SCANNING, 13353); // Stop Scanning
      else if (!inPlaylists && !m_vecItems->IsInternetStream()           &&
               !item->IsLastFM()                                         &&
               !item->GetPath().Equals("add") && !item->IsParentFolder() &&
               !item->IsPlugin()                                         &&
              (g_settings.GetCurrentProfile().canWriteDatabases() || g_passwordManager.bMasterUser))
      {
        buttons.Add(CONTEXT_BUTTON_SCAN, 13352);
      }
    }
    if (item->IsPlugin() || item->IsScript() || m_vecItems->IsPlugin())
      buttons.Add(CONTEXT_BUTTON_PLUGIN_SETTINGS, 1045);
  }
  if (!m_vecItems->IsVirtualDirectoryRoot())
    buttons.Add(CONTEXT_BUTTON_SWITCH_MEDIA, 523);
  CGUIWindowMusicBase::GetNonContextButtons(buttons);
}

bool CGUIWindowMusicSongs::OnContextButton(int itemNumber, CONTEXT_BUTTON button)
{
  CFileItemPtr item;
  if (itemNumber >= 0 && itemNumber < m_vecItems->Size())
    item = m_vecItems->Get(itemNumber);
  if ( m_vecItems->IsVirtualDirectoryRoot() && item)
  {
    if (CGUIDialogContextMenu::OnContextButton("music", item, button))
    {
      if (button == CONTEXT_BUTTON_REMOVE_SOURCE)
        OnRemoveSource(itemNumber);

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
      Update(m_vecItems->GetPath());
    return true;

  case CONTEXT_BUTTON_DELETE:
    OnDeleteItem(itemNumber);
    return true;

  case CONTEXT_BUTTON_RENAME:
    OnRenameItem(itemNumber);
    return true;

  case CONTEXT_BUTTON_SWITCH_MEDIA:
    CGUIDialogContextMenu::SwitchMedia("music", m_vecItems->GetPath());
    return true;
  default:
    break;
  }
  return CGUIWindowMusicBase::OnContextButton(itemNumber, button);
}

void CGUIWindowMusicSongs::DeleteRemoveableMediaDirectoryCache()
{
  CUtil::DeleteDirectoryCache("r-");
}

void CGUIWindowMusicSongs::PlayItem(int iItem)
{
  // unlike additemtoplaylist, we need to check the items here
  // before calling it since the current playlist will be stopped
  // and cleared!

  // we're at the root source listing
  if (m_vecItems->IsVirtualDirectoryRoot() && !m_vecItems->Get(iItem)->IsDVD())
    return;

#ifdef HAS_DVD_DRIVE
  if (m_vecItems->Get(iItem)->IsDVD())
    MEDIA_DETECT::CAutorun::PlayDiscAskResume(m_vecItems->Get(iItem)->GetPath());
  else
#endif
    CGUIWindowMusicBase::PlayItem(iItem);
}

bool CGUIWindowMusicSongs::Update(const CStdString &strDirectory)
{
  if (m_thumbLoader.IsLoading())
    m_thumbLoader.StopThread();

  if (!CGUIMediaWindow::Update(strDirectory))
    return false;

  if (m_vecItems->GetContent().IsEmpty())
    m_vecItems->SetContent("files");
  m_thumbLoader.Load(*m_vecItems);

  return true;
}

void CGUIWindowMusicSongs::OnRemoveSource(int iItem)
{
  bool bCanceled;
  if (CGUIDialogYesNo::ShowAndGetInput(522,20340,20341,20022,bCanceled))
  {
    CSongMap songs;
    CMusicDatabase database;
    database.Open();
    database.RemoveSongsFromPath(m_vecItems->Get(iItem)->GetPath(),songs,false);
    database.CleanupOrphanedItems();
    g_infoManager.ResetLibraryBools();
  }
}

CStdString CGUIWindowMusicSongs::GetStartFolder(const CStdString &dir)
{
  SetupShares();
  VECSOURCES shares;
  m_rootDir.GetSources(shares);
  bool bIsSourceName = false;
  int iIndex = CUtil::GetMatchingSource(dir, shares, bIsSourceName);
  if (iIndex > -1)
  {
    if (iIndex < (int)shares.size() && shares[iIndex].m_iHasLock == 2)
    {
      CFileItem item(shares[iIndex]);
      if (!g_passwordManager.IsItemUnlocked(&item,"music"))
        return "";
    }
    // set current directory to matching share
    if (bIsSourceName)
      return shares[iIndex].strPath;
    return dir;
  }
  return CGUIWindowMusicBase::GetStartFolder(dir);
}
