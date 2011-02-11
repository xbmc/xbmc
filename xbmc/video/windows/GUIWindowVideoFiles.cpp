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
#include "GUIWindowVideoFiles.h"
#include "Util.h"
#include "playlists/PlayListFactory.h"
#include "Application.h"
#include "NfoFile.h"
#include "PlayListPlayer.h"
#include "GUIPassword.h"
#include "dialogs/GUIDialogMediaSource.h"
#include "settings/GUIDialogContentSettings.h"
#include "video/dialogs/GUIDialogVideoScan.h"
#include "filesystem/MultiPathDirectory.h"
#include "utils/RegExp.h"
#include "guilib/GUIWindowManager.h"
#include "dialogs/GUIDialogOK.h"
#include "dialogs/GUIDialogYesNo.h"
#include "filesystem/File.h"
#include "playlists/PlayList.h"
#include "settings/Settings.h"
#include "settings/GUISettings.h"
#include "utils/log.h"
#include "utils/URIUtils.h"

using namespace std;
using namespace XFILE;
using namespace PLAYLIST;
using namespace VIDEO;

#define CONTROL_PLAY_DVD           6
#define CONTROL_STACK              7
#define CONTROL_BTNSCAN            8
#define CONTROL_BTNPLAYLISTS      13

CGUIWindowVideoFiles::CGUIWindowVideoFiles()
: CGUIWindowVideoBase(WINDOW_VIDEO_FILES, "MyVideo.xml")
{
}

CGUIWindowVideoFiles::~CGUIWindowVideoFiles()
{
}

bool CGUIWindowVideoFiles::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_INIT:
    {
      // is this the first time accessing this window?
      if (m_vecItems->m_strPath == "?" && message.GetStringParam().IsEmpty())
        message.SetStringParam(g_settings.m_defaultVideoSource);

      return CGUIWindowVideoBase::OnMessage(message);
    }
    break;

  case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();
      if (iControl == CONTROL_BTNPLAYLISTS)
      {
        if (!m_vecItems->m_strPath.Equals("special://videoplaylists/"))
        {
          CStdString strParent = m_vecItems->m_strPath;
          UpdateButtons();
          Update("special://videoplaylists/");
        }
      }
      // list/thumb panel
      else if (m_viewControl.HasControl(iControl))
      {
        int iItem = m_viewControl.GetSelectedItem();
        int iAction = message.GetParam1();

        const CFileItemPtr pItem = m_vecItems->Get(iItem);

        // use play button to add folders of items to temp playlist
        if (iAction == ACTION_PLAYER_PLAY && pItem->m_bIsFolder && !pItem->IsParentFolder())
        {
#ifdef HAS_DVD_DRIVE
          if (pItem->IsDVD())
            return MEDIA_DETECT::CAutorun::PlayDisc();
#endif

          if (pItem->m_bIsShareOrDrive)
            return false;
          // if playback is paused or playback speed != 1, return
          if (g_application.IsPlayingVideo())
          {
            if (g_application.m_pPlayer->IsPaused()) return false;
            if (g_application.GetPlaySpeed() != 1) return false;
          }
          // not playing, or playback speed == 1
          // queue folder or playlist into temp playlist and play
          if ((pItem->m_bIsFolder && !pItem->IsParentFolder()) || pItem->IsPlayList())
            PlayItem(iItem);
          return true;
        }
      }
    }
  }
  return CGUIWindowVideoBase::OnMessage(message);
}

bool CGUIWindowVideoFiles::OnAction(const CAction &action)
{
  if (action.GetID() == ACTION_TOGGLE_WATCHED)
  {
    CFileItemPtr pItem = m_vecItems->Get(m_viewControl.GetSelectedItem());
    if (pItem->IsParentFolder())
      return false;

    if (pItem && pItem->GetOverlayImage().Equals("OverlayWatched.png"))
      return OnContextButton(m_viewControl.GetSelectedItem(),CONTEXT_BUTTON_MARK_UNWATCHED);
    else
      return OnContextButton(m_viewControl.GetSelectedItem(),CONTEXT_BUTTON_MARK_WATCHED);
  }
  return CGUIWindowVideoBase::OnAction(action);
}

bool CGUIWindowVideoFiles::GetDirectory(const CStdString &strDirectory, CFileItemList &items)
{
  if (!CGUIWindowVideoBase::GetDirectory(strDirectory, items))
    return false;

  ADDON::ScraperPtr info2 = m_database.GetScraperForPath(strDirectory);
  if (info2 && info2->Content() != CONTENT_NONE)
    items.SetContent(ADDON::TranslateContent(info2->Content()));
  else if (items.GetContent().IsEmpty())
    items.SetContent("files");

  items.SetThumbnailImage("");
  items.SetVideoThumb();

  return true;
}

bool CGUIWindowVideoFiles::OnPlayMedia(int iItem)
{
  if ( iItem < 0 || iItem >= (int)m_vecItems->Size() ) return false;
  CFileItemPtr pItem = m_vecItems->Get(iItem);

#ifdef HAS_DVD_DRIVE
  if (pItem->IsDVD())
    return MEDIA_DETECT::CAutorun::PlayDisc();
#endif

  if (pItem->m_bIsShareOrDrive)
    return false;

  AddFileToDatabase(pItem.get());

  return CGUIWindowVideoBase::OnPlayMedia(iItem);
}

void CGUIWindowVideoFiles::AddFileToDatabase(const CFileItem* pItem)
{
  if (!pItem->IsVideo()) return ;
  if ( pItem->IsNFO()) return ;
  if ( pItem->IsPlayList()) return ;

  /* subtitles are assumed not to exist here, if we need it  */
  /* player should update this when it figures out if it has */
  m_database.AddFile(pItem->m_strPath);

  if (pItem->IsStack())
  { // get stacked items
    // TODO: This should be removed as soon as we no longer need the individual
    // files for saving settings etc.
    vector<CStdString> movies;
    GetStackedFiles(pItem->m_strPath, movies);
    for (unsigned int i = 0; i < movies.size(); i++)
    {
      CFileItem item(movies[i], false);
      m_database.AddFile(item.m_strPath);
    }
  }
}

bool CGUIWindowVideoFiles::OnUnAssignContent(const CStdString &path, int label1, int label2, int label3)
{
  bool bCanceled;
  CVideoDatabase db;
  db.Open();
  if (CGUIDialogYesNo::ShowAndGetInput(label1,label2,label3,20022,bCanceled))
  {
    db.RemoveContentForPath(path);
    db.Close();
    CUtil::DeleteVideoDatabaseDirectoryCache();
    return true;
  }
  else
  {
    if (!bCanceled)
    {
      ADDON::ScraperPtr info;
      SScanSettings settings;
      settings.exclude = true;
      db.SetScraperForPath(path,info,settings);
    }
  }
  db.Close();

  return false;
}

void CGUIWindowVideoFiles::OnAssignContent(const CStdString &path, int iFound, ADDON::ScraperPtr& info, SScanSettings& settings)
{
  bool bScan=false;
  CVideoDatabase db;
  db.Open();
  if (iFound == 0)
  {
    info = db.GetScraperForPath(path, settings);
  }

  ADDON::ScraperPtr info2(info);

  if (CGUIDialogContentSettings::Show(info, settings, bScan))
  {
    if(settings.exclude || (!info && info2))
    {
      OnUnAssignContent(path,20375,20340,20341);
    }
    else if (info != info2)
    {
      if (OnUnAssignContent(path,20442,20443,20444))
        bScan = true;
    }

    db.SetScraperForPath(path,info,settings);

    if (bScan)
    {
      CGUIDialogVideoScan* pDialog = (CGUIDialogVideoScan*)g_windowManager.GetWindow(WINDOW_DIALOG_VIDEO_SCAN);
      if (pDialog)
        pDialog->StartScanning(path, true);
    }
  }
}

void CGUIWindowVideoFiles::GetStackedDirectory(const CStdString &strPath, CFileItemList &items)
{
  items.Clear();
  m_rootDir.GetDirectory(strPath, items);
  items.Stack();
}

void CGUIWindowVideoFiles::LoadPlayList(const CStdString& strPlayList)
{
  // load a playlist like .m3u, .pls
  // first get correct factory to load playlist
  auto_ptr<CPlayList> pPlayList (CPlayListFactory::Create(strPlayList));
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
  if (g_application.ProcessAndStartPlaylist(strPlayList, *pPlayList, PLAYLIST_VIDEO))
  {
    if (m_guiState.get())
      m_guiState->SetPlaylistDirectory("playlistvideo://");
    // activate the playlist window if its not activated yet
    if (GetID() == g_windowManager.GetActiveWindow() && iSize > 1)
    {
      g_windowManager.ActivateWindow(WINDOW_VIDEO_PLAYLIST);
    }
  }
}

void CGUIWindowVideoFiles::GetContextButtons(int itemNumber, CContextButtons &buttons)
{
  CFileItemPtr item;
  if (itemNumber >= 0 && itemNumber < m_vecItems->Size())
    item = m_vecItems->Get(itemNumber);

  CGUIDialogVideoScan *pScanDlg = (CGUIDialogVideoScan *)g_windowManager.GetWindow(WINDOW_DIALOG_VIDEO_SCAN);
  if (item && !item->m_strPath.IsEmpty())
  {
    // are we in the playlists location?
    if (m_vecItems->IsVirtualDirectoryRoot())
    {
      // get the usual shares, and anything for all media windows
      CGUIDialogContextMenu::GetContextButtons("video", item, buttons);
      CGUIMediaWindow::GetContextButtons(itemNumber, buttons);
      // add scan button somewhere here
      if (pScanDlg && pScanDlg->IsScanning())
        buttons.Add(CONTEXT_BUTTON_STOP_SCANNING, 13353);  // Stop Scanning
      if (!item->IsDVD() && item->m_strPath != "add" &&
         (g_settings.GetCurrentProfile().canWriteDatabases() || g_passwordManager.bMasterUser))
      {
        CVideoDatabase database;
        database.Open();
        ADDON::ScraperPtr info = database.GetScraperForPath(item->m_strPath);

        CGUIDialogVideoScan *pScanDlg = (CGUIDialogVideoScan *)g_windowManager.GetWindow(WINDOW_DIALOG_VIDEO_SCAN);
        if (!pScanDlg || (pScanDlg && !pScanDlg->IsScanning()))
        {
          if (!item->IsLiveTV() && !item->IsPlugin() && !item->IsAddonsPath())
          {
            if (info && info->Content() != CONTENT_NONE)
              buttons.Add(CONTEXT_BUTTON_SET_CONTENT, 20442);
            else
              buttons.Add(CONTEXT_BUTTON_SET_CONTENT, 20333);
          }
        }

        if (info && (!pScanDlg || (pScanDlg && !pScanDlg->IsScanning())))
          buttons.Add(CONTEXT_BUTTON_SCAN, 13349);
      }
    }
    else
    {
      CGUIWindowVideoBase::GetContextButtons(itemNumber, buttons);
      if (!item->GetPropertyBOOL("pluginreplacecontextitems") && !item->IsParentFolder())
      {
        // Movie Info button
        if (pScanDlg && pScanDlg->IsScanning())
          buttons.Add(CONTEXT_BUTTON_STOP_SCANNING, 13353);
        if (g_settings.GetCurrentProfile().canWriteDatabases() || g_passwordManager.bMasterUser)
        {
          ADDON::ScraperPtr info;
          VIDEO::SScanSettings settings;
          GetScraperForItem(item.get(), info, settings);

          int infoString = 13346;

          if (info && info->Content() == CONTENT_TVSHOWS)
            infoString = item->m_bIsFolder ? 20351 : 20352;
          if (info && info->Content() == CONTENT_MUSICVIDEOS)
            infoString = 20393;
          if(item->IsLiveTV())
            infoString = 20352;

          if (item->m_bIsFolder)
          {
            if (!pScanDlg || (pScanDlg && !pScanDlg->IsScanning()))
            {
              if (!item->IsPlayList() && !item->IsLiveTV() && !item->IsPlugin() && !item->IsAddonsPath())
              {
                if (info && info->Content() != CONTENT_NONE)
                  buttons.Add(CONTEXT_BUTTON_SET_CONTENT, 20442);
                else
                  buttons.Add(CONTEXT_BUTTON_SET_CONTENT, 20333);
              }
            }
            if (!info)
            { // scraper not set - allow movie information or set content
              CStdString strPath(item->m_strPath);
              URIUtils::AddSlashAtEnd(strPath);
              if (m_database.HasMovieInfo(strPath) || m_database.HasTvShowInfo(strPath))
                buttons.Add(CONTEXT_BUTTON_INFO, infoString);
            }
            else
            { // scraper found - allow movie information, scan for new content, or set different type of content
              if (info->Content() != CONTENT_MUSICVIDEOS)
                buttons.Add(CONTEXT_BUTTON_INFO, infoString);
              if (info->Content() != CONTENT_NONE)
                if (!pScanDlg || (pScanDlg && !pScanDlg->IsScanning()))
                  buttons.Add(CONTEXT_BUTTON_SCAN, 13349);
            }
          }
          else
          {
            // single file
            buttons.Add(CONTEXT_BUTTON_INFO, infoString);

            if (!item->IsLiveTV())
            {
              if (!m_database.HasMovieInfo(item->m_strPath) 
              &&  !m_database.HasEpisodeInfo(item->m_strPath))
                buttons.Add(CONTEXT_BUTTON_ADD_TO_LIBRARY, 527); // Add to Database
              else  
                buttons.Add(CONTEXT_BUTTON_DELETE, 646); // Remove from Database
            }
          }
        }
      }
      if (!item->IsParentFolder())
      {
        if ((m_vecItems->m_strPath.Equals("special://videoplaylists/")) ||
             g_guiSettings.GetBool("filelists.allowfiledeletion"))
        { // video playlists or file operations are allowed
          if (!item->IsReadOnly())
          {
            buttons.Add(CONTEXT_BUTTON_DELETE, 117);
            buttons.Add(CONTEXT_BUTTON_RENAME, 118);
          }
        }
      }
      if (m_vecItems->IsPlugin() && item->HasVideoInfoTag() && !item->GetPropertyBOOL("pluginreplacecontextitems"))
        buttons.Add(CONTEXT_BUTTON_INFO,13346); // only movie information for now

      if (!item->IsPlugin() && !item->IsLiveTV() && !item->IsAddonsPath())
      {
        if (item->m_bIsFolder)
        {
          // Have both options for folders since we don't know whether all childs are watched/unwatched
          buttons.Add(CONTEXT_BUTTON_MARK_UNWATCHED, 16104); //Mark as UnWatched
          buttons.Add(CONTEXT_BUTTON_MARK_WATCHED, 16103);   //Mark as Watched
        }
        else
        if (item->GetOverlayImage().Equals("OverlayWatched.png"))
          buttons.Add(CONTEXT_BUTTON_MARK_UNWATCHED, 16104); //Mark as UnWatched
        else
          buttons.Add(CONTEXT_BUTTON_MARK_WATCHED, 16103);   //Mark as Watched
      }
      if (item->IsPlugin() || item->m_strPath.Left(9).Equals("script://") || m_vecItems->IsPlugin())
        buttons.Add(CONTEXT_BUTTON_PLUGIN_SETTINGS, 1045);
    }
  }
  else
  {
    if (pScanDlg && pScanDlg->IsScanning())
      buttons.Add(CONTEXT_BUTTON_STOP_SCANNING, 13353);  // Stop Scanning
  }
  if(!(item && item->GetPropertyBOOL("pluginreplacecontextitems")))
  {
    if (!m_vecItems->IsVirtualDirectoryRoot())
      buttons.Add(CONTEXT_BUTTON_SWITCH_MEDIA, 523);

    CGUIWindowVideoBase::GetNonContextButtons(itemNumber, buttons);
  }
}

bool CGUIWindowVideoFiles::OnContextButton(int itemNumber, CONTEXT_BUTTON button)
{
  CFileItemPtr item;
  if (itemNumber >= 0 && itemNumber < m_vecItems->Size())
    item = m_vecItems->Get(itemNumber);
  if ( m_vecItems->IsVirtualDirectoryRoot() && item)
  {
    if (CGUIDialogContextMenu::OnContextButton("video", item, button))
    {
      //TODO should we search DB for entries from plugins?
      if (button == CONTEXT_BUTTON_REMOVE_SOURCE && !item->IsPlugin()
          && !item->IsLiveTV() &&!item->IsRSS())
      {
        OnUnAssignContent(item->m_strPath,20375,20340,20341);
      }
      Update("");
      return true;
    }
  }

  switch (button)
  {
  case CONTEXT_BUTTON_SWITCH_MEDIA:
    CGUIDialogContextMenu::SwitchMedia("video", m_vecItems->m_strPath);
    return true;

  case CONTEXT_BUTTON_ADD_TO_LIBRARY:
    AddToDatabase(itemNumber);
    return true;

  default:
    break;
  }
  return CGUIWindowVideoBase::OnContextButton(itemNumber, button);
}

CStdString CGUIWindowVideoFiles::GetStartFolder(const CStdString &dir)
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
      if (!g_passwordManager.IsItemUnlocked(&item,"video"))
        return "";
    }
    // set current directory to matching share
    if (bIsSourceName)
      return shares[iIndex].strPath;
    return dir;
  }
  return CGUIWindowVideoBase::GetStartFolder(dir);
}
