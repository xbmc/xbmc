/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "system.h"
#include "GUIWindowMusicSongs.h"
#include "Util.h"
#include "GUIInfoManager.h"
#include "Application.h"
#include "GUIPassword.h"
#include "dialogs/GUIDialogYesNo.h"
#include "GUIUserMessages.h"
#include "FileItem.h"
#include "profiles/ProfilesManager.h"
#include "storage/MediaManager.h"
#include "settings/MediaSourceSettings.h"
#include "settings/Settings.h"
#include "input/Key.h"
#include "guilib/LocalizeStrings.h"
#include "utils/URIUtils.h"
#include "utils/StringUtils.h"
#include "Autorun.h"
#include "cdrip/CDDARipper.h"
#include "ContextMenuManager.h"

#define CONTROL_BTNVIEWASICONS     2
#define CONTROL_BTNSORTBY          3
#define CONTROL_BTNSORTASC         4
#define CONTROL_BTNTYPE            5
#define CONTROL_LABELFILES        12

#define CONTROL_BTNPLAYLISTS       7
#define CONTROL_BTNSCAN            9
#define CONTROL_BTNREC            10
#define CONTROL_BTNRIP            11

#ifdef HAS_DVD_DRIVE
using namespace MEDIA_DETECT;
#endif

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
      if (m_vecItems->GetPath() == "?" && message.GetStringParam().empty())
        message.SetStringParam(CMediaSourceSettings::Get().GetDefaultSource("music"));

      return CGUIWindowMusicBase::OnMessage(message);
    }
    break;

  case GUI_MSG_DIRECTORY_SCANNED:
    {
      CFileItem directory(message.GetStringParam(), true);

      // Only update thumb on a local drive
      if (directory.IsHD())
      {
        std::string strParent;
        URIUtils::GetParentPath(directory.GetPath(), strParent);
        if (directory.GetPath() == m_vecItems->GetPath() || strParent == m_vecItems->GetPath())
          Refresh();
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
        if (!m_vecItems->IsPath("special://musicplaylists/"))
          Update("special://musicplaylists/");
      }
      else if (iControl == CONTROL_BTNSCAN)
      {
        OnScan(-1);
      }
      else if (iControl == CONTROL_BTNREC)
      {
        if (g_application.m_pPlayer->IsPlayingAudio() )
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
  std::string strPath;
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

void CGUIWindowMusicSongs::DoScan(const std::string &strPath)
{
  if (g_application.IsMusicScanning())
  {
    g_application.StopMusicScan();
    return;
  }

  // Start background loader
  int iControl=GetFocusedControlID();
  g_application.StartMusicScan(strPath);
  SET_CONTROL_FOCUS(iControl, 0);
  UpdateButtons();

  return;
}

bool CGUIWindowMusicSongs::GetDirectory(const std::string &strDirectory, CFileItemList &items)
{
  if (!CGUIWindowMusicBase::GetDirectory(strDirectory, items))
    return false;

  // check for .CUE files here.
  items.FilterCueItems();

  std::string label;
  if (items.GetLabel().empty() && m_rootDir.IsSource(items.GetPath(), CMediaSourceSettings::Get().GetSources("music"), &label)) 
    items.SetLabel(label);

  return true;
}

void CGUIWindowMusicSongs::OnPrepareFileItems(CFileItemList &items)
{
  CGUIWindowMusicBase::OnPrepareFileItems(items);

  RetrieveMusicInfo();
}

void CGUIWindowMusicSongs::UpdateButtons()
{
  CGUIWindowMusicBase::UpdateButtons();

  bool bIsPlaying = g_application.m_pPlayer->IsPlayingAudio();
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
      m_vecItems->IsMusicDb())
  {
    CONTROL_DISABLE(CONTROL_BTNSCAN);
  }
  else
  {
    CONTROL_ENABLE(CONTROL_BTNSCAN);
  }

  if (g_application.IsMusicScanning())
  {
    SET_CONTROL_LABEL(CONTROL_BTNSCAN, 14056); // Stop Scan
  }
  else
  {
    SET_CONTROL_LABEL(CONTROL_BTNSCAN, 102); // Scan
  }

  // Update object count label
  std::string items = StringUtils::Format("%i %s", m_vecItems->GetObjectCount(), g_localizeStrings.Get(127).c_str());
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
    bool inPlaylists = m_vecItems->IsPath(CUtil::MusicPlaylistsLocation()) ||
                       m_vecItems->IsPath("special://musicplaylists/");

    if (m_vecItems->IsVirtualDirectoryRoot() || m_vecItems->GetPath() == "sources://music/")
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
        {
          if (CJobManager::GetInstance().IsProcessing("cdrip"))
            buttons.Add(CONTEXT_BUTTON_CANCEL_RIP_CD, 14100);
          else
            buttons.Add(CONTEXT_BUTTON_RIP_CD, 600);
        }
      }
#endif
      CGUIMediaWindow::GetContextButtons(itemNumber, buttons);
    }
    else
    {
      CGUIWindowMusicBase::GetContextButtons(itemNumber, buttons);
      if (item->GetProperty("pluginreplacecontextitems").asBoolean())
        return;
      if (!item->IsPlayList() && !item->IsPlugin() && !item->IsScript())
      {
        if (item->IsAudio())
          buttons.Add(CONTEXT_BUTTON_SONG_INFO, 658); // Song Info
        else if (!item->IsParentFolder() &&
                 !StringUtils::StartsWithNoCase(item->GetPath(), "new") && item->m_bIsFolder)
        {
          if (m_musicdatabase.GetAlbumIdByPath(item->GetPath()) > -1)
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
         (CProfilesManager::Get().GetCurrentProfile().canWriteDatabases() || g_passwordManager.bMasterUser))
      {
        buttons.Add(CONTEXT_BUTTON_CDDB, 16002);
      }

      if (!item->IsParentFolder() && !item->IsReadOnly())
      {
        // either we're at the playlist location or its been explicitly allowed
        if (inPlaylists || CSettings::Get().GetBool("filelists.allowfiledeletion"))
        {
          buttons.Add(CONTEXT_BUTTON_DELETE, 117);
          buttons.Add(CONTEXT_BUTTON_RENAME, 118);
        }
      }
    }

    // Add the scan button(s)
    if (g_application.IsMusicScanning())
      buttons.Add(CONTEXT_BUTTON_STOP_SCANNING, 13353); // Stop Scanning
    else if (!inPlaylists && !m_vecItems->IsInternetStream()           &&
             !item->IsPath("add") && !item->IsParentFolder() &&
             !item->IsPlugin()                                         &&
             !StringUtils::StartsWithNoCase(item->GetPath(), "addons://")              &&
            (CProfilesManager::Get().GetCurrentProfile().canWriteDatabases() || g_passwordManager.bMasterUser))
    {
      buttons.Add(CONTEXT_BUTTON_SCAN, 13352);
    }
    if (item->IsPlugin() || item->IsScript() || m_vecItems->IsPlugin())
      buttons.Add(CONTEXT_BUTTON_PLUGIN_SETTINGS, 1045);
  }
  if (!m_vecItems->IsVirtualDirectoryRoot() && !m_vecItems->IsPlugin())
    buttons.Add(CONTEXT_BUTTON_SWITCH_MEDIA, 523);
  CGUIWindowMusicBase::GetNonContextButtons(buttons);

  CContextMenuManager::Get().AddVisibleItems(item, buttons);
}

bool CGUIWindowMusicSongs::OnContextButton(int itemNumber, CONTEXT_BUTTON button)
{
  CFileItemPtr item;
  if (itemNumber >= 0 && itemNumber < m_vecItems->Size())
    item = m_vecItems->Get(itemNumber);
  if (CGUIDialogContextMenu::OnContextButton("music", item, button))
  {
    if (button == CONTEXT_BUTTON_REMOVE_SOURCE)
      OnRemoveSource(itemNumber);

    Update("");
    return true;
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

#ifdef HAS_CDDA_RIPPER
  case CONTEXT_BUTTON_CANCEL_RIP_CD:
    CCDDARipper::GetInstance().CancelJobs();
    return true;
#endif

  case CONTEXT_BUTTON_CDDB:
    if (m_musicdatabase.LookupCDDBInfo(true))
      Refresh();
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

bool CGUIWindowMusicSongs::Update(const std::string &strDirectory, bool updateFilterPath /* = true */)
{
  if (m_thumbLoader.IsLoading())
    m_thumbLoader.StopThread();

  if (!CGUIMediaWindow::Update(strDirectory, updateFilterPath))
    return false;

  if (m_vecItems->GetContent().empty())
    m_vecItems->SetContent("files");
  m_thumbLoader.Load(*m_vecItems);

  return true;
}

void CGUIWindowMusicSongs::OnRemoveSource(int iItem)
{
  bool bCanceled;
  if (CGUIDialogYesNo::ShowAndGetInput(522, 20340, bCanceled))
  {
    MAPSONGS songs;
    CMusicDatabase database;
    database.Open();
    database.RemoveSongsFromPath(m_vecItems->Get(iItem)->GetPath(),songs,false);
    database.CleanupOrphanedItems();
    g_infoManager.ResetLibraryBools();
  }
}

std::string CGUIWindowMusicSongs::GetStartFolder(const std::string &dir)
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
