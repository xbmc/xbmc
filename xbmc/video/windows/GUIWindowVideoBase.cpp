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
#include "GUIWindowVideoBase.h"
#include "Util.h"
#include "video/VideoInfoDownloader.h"
#include "utils/RegExp.h"
#include "utils/Variant.h"
#include "addons/AddonManager.h"
#include "addons/IAddon.h"
#include "video/dialogs/GUIDialogVideoInfo.h"
#include "GUIWindowVideoNav.h"
#include "dialogs/GUIDialogFileBrowser.h"
#include "video/dialogs/GUIDialogVideoScan.h"
#include "dialogs/GUIDialogSmartPlaylistEditor.h"
#include "dialogs/GUIDialogProgress.h"
#include "dialogs/GUIDialogYesNo.h"
#include "playlists/PlayListFactory.h"
#include "Application.h"
#include "NfoFile.h"
#include "PlayListPlayer.h"
#include "GUIPassword.h"
#include "filesystem/ZipManager.h"
#include "filesystem/StackDirectory.h"
#include "filesystem/MultiPathDirectory.h"
#include "video/dialogs/GUIDialogFileStacking.h"
#include "dialogs/GUIDialogMediaSource.h"
#include "windows/GUIWindowFileManager.h"
#include "filesystem/VideoDatabaseDirectory.h"
#include "PartyModeManager.h"
#include "guilib/GUIWindowManager.h"
#include "dialogs/GUIDialogOK.h"
#include "dialogs/GUIDialogSelect.h"
#include "dialogs/GUIDialogKeyboard.h"
#include "filesystem/Directory.h"
#include "playlists/PlayList.h"
#include "settings/Settings.h"
#include "settings/AdvancedSettings.h"
#include "settings/GUISettings.h"
#include "settings/GUIDialogContentSettings.h"
#include "guilib/LocalizeStrings.h"
#include "utils/StringUtils.h"
#include "utils/log.h"
#include "utils/FileUtils.h"
#include "utils/URIUtils.h"

#include "addons/Skin.h"

using namespace std;
using namespace XFILE;
using namespace PLAYLIST;
using namespace VIDEODATABASEDIRECTORY;
using namespace VIDEO;
using namespace ADDON;

#define CONTROL_BTNVIEWASICONS     2
#define CONTROL_BTNSORTBY          3
#define CONTROL_BTNSORTASC         4
#define CONTROL_BTNTYPE            5
#define CONTROL_LABELFILES        12

#define CONTROL_PLAY_DVD           6
#define CONTROL_STACK              7
#define CONTROL_BTNSCAN            8

CGUIWindowVideoBase::CGUIWindowVideoBase(int id, const CStdString &xmlFile)
    : CGUIMediaWindow(id, xmlFile)
{
  m_thumbLoader.SetObserver(this);
  m_thumbLoader.SetStreamDetailsObserver(this);
  m_stackingAvailable = true;
}

CGUIWindowVideoBase::~CGUIWindowVideoBase()
{
}

bool CGUIWindowVideoBase::OnAction(const CAction &action)
{
  if (action.GetID() == ACTION_SCAN_ITEM)
    return OnContextButton(m_viewControl.GetSelectedItem(),CONTEXT_BUTTON_SCAN);

  return CGUIMediaWindow::OnAction(action);
}

bool CGUIWindowVideoBase::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_DEINIT:
    if (m_thumbLoader.IsLoading())
      m_thumbLoader.StopThread();
    m_database.Close();
    break;

  case GUI_MSG_WINDOW_INIT:
    {
      m_database.Open();

      m_dlgProgress = (CGUIDialogProgress*)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS);

      // save current window, unless the current window is the video playlist window
      if (GetID() != WINDOW_VIDEO_PLAYLIST && g_settings.m_iVideoStartWindow != GetID())
      {
        g_settings.m_iVideoStartWindow = GetID();
        g_settings.Save();
      }

      return CGUIMediaWindow::OnMessage(message);
    }
    break;

  case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();
      if (iControl == CONTROL_STACK)
      {
        g_settings.m_videoStacking = !g_settings.m_videoStacking;
        g_settings.Save();
        UpdateButtons();
        Update( m_vecItems->m_strPath );
      }
      else if (iControl == CONTROL_PLAY_DVD)
      {
        // play movie...
        CUtil::PlayDVD();
      }
      else if (iControl == CONTROL_BTNTYPE)
      {
        CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_BTNTYPE);
        g_windowManager.SendMessage(msg);

        int nSelected = msg.GetParam1();
        int nNewWindow = WINDOW_VIDEO_FILES;
        switch (nSelected)
        {
        case 0:  // Movies
          nNewWindow = WINDOW_VIDEO_FILES;
          break;
        case 1:  // Library
          nNewWindow = WINDOW_VIDEO_NAV;
          break;
        }

        if (nNewWindow != GetID())
        {
          g_settings.m_iVideoStartWindow = nNewWindow;
          g_settings.Save();
          g_windowManager.ChangeActiveWindow(nNewWindow);
          CGUIMessage msg2(GUI_MSG_SETFOCUS, nNewWindow, CONTROL_BTNTYPE);
          g_windowManager.SendMessage(msg2);
        }

        return true;
      }
      else if (m_viewControl.HasControl(iControl))  // list/thumb control
      {
        // get selected item
        int iItem = m_viewControl.GetSelectedItem();
        int iAction = message.GetParam1();

        // iItem is checked for validity inside these routines
        if (iAction == ACTION_QUEUE_ITEM || iAction == ACTION_MOUSE_MIDDLE_CLICK)
        {
          OnQueueItem(iItem);
          return true;
        }
        else if (iAction == ACTION_SHOW_INFO)
        {
          return OnInfo(iItem);
        }
        else if (iAction == ACTION_PLAYER_PLAY && !g_application.IsPlayingVideo())
        {
          return OnResumeItem(iItem);
        }
        else if (iAction == ACTION_DELETE_ITEM)
        {
          // is delete allowed?
          if (g_settings.GetCurrentProfile().canWriteDatabases())
          {
            // must be at the title window
            if (GetID() == WINDOW_VIDEO_NAV)
              OnDeleteItem(iItem);

            // or be at the files window and have file deletion enabled
            else if (GetID() == WINDOW_VIDEO_FILES && g_guiSettings.GetBool("filelists.allowfiledeletion"))
              OnDeleteItem(iItem);

            // or be at the video playlists location
            else if (m_vecItems->m_strPath.Equals("special://videoplaylists/"))
              OnDeleteItem(iItem);
            else
              return false;

            return true;
          }
        }
      }
    }
    break;
  }
  return CGUIMediaWindow::OnMessage(message);
}

void CGUIWindowVideoBase::UpdateButtons()
{
  // Remove labels from the window selection
  CGUIMessage msg(GUI_MSG_LABEL_RESET, GetID(), CONTROL_BTNTYPE);
  g_windowManager.SendMessage(msg);

  // Add labels to the window selection
  CStdString strItem = g_localizeStrings.Get(744); // Files
  CGUIMessage msg2(GUI_MSG_LABEL_ADD, GetID(), CONTROL_BTNTYPE);
  msg2.SetLabel(strItem);
  g_windowManager.SendMessage(msg2);

  strItem = g_localizeStrings.Get(14022); // Library
  msg2.SetLabel(strItem);
  g_windowManager.SendMessage(msg2);

  // Select the current window as default item
  int nWindow = g_settings.m_iVideoStartWindow-WINDOW_VIDEO_FILES;
  CONTROL_SELECT_ITEM(CONTROL_BTNTYPE, nWindow);

  CONTROL_ENABLE(CONTROL_BTNSCAN);

  SET_CONTROL_LABEL(CONTROL_STACK, 14000);  // Stack
  SET_CONTROL_SELECTED(GetID(), CONTROL_STACK, g_settings.m_videoStacking);
  CONTROL_ENABLE_ON_CONDITION(CONTROL_STACK, m_stackingAvailable);
  
  CGUIMediaWindow::UpdateButtons();
}

void CGUIWindowVideoBase::OnInfo(CFileItem* pItem, const ADDON::ScraperPtr& scraper)
{
  if (!pItem)
    return;

  if (pItem->IsParentFolder() || pItem->m_bIsShareOrDrive || pItem->m_strPath.Equals("add"))
    return;

  // ShowIMDB can kill the item as this window can be closed while we do it,
  // so take a copy of the item now
  CFileItem item(*pItem);
  if (item.IsVideoDb() && item.HasVideoInfoTag())
  {
    if (item.GetVideoInfoTag()->m_strFileNameAndPath.IsEmpty())
      item.m_strPath = item.GetVideoInfoTag()->m_strPath;
    else
      item.m_strPath = item.GetVideoInfoTag()->m_strFileNameAndPath;
  }
  else
  {
    if (item.m_bIsFolder && scraper && scraper->Content() != CONTENT_TVSHOWS)
    {
      CFileItemList items;
      CDirectory::GetDirectory(item.m_strPath, items,"",true,false,DIR_CACHE_ONCE,true,true);
      items.Stack();

      // check for media files
      bool bFoundFile(false);
      for (int i = 0; i < items.Size(); ++i)
      {
        CFileItemPtr item2 = items[i];

        if (item2->IsVideo() && !item2->IsPlayList() &&
            !CUtil::ExcludeFileOrFolder(item2->m_strPath, g_advancedSettings.VideoSettings->MoviesExcludeFromScanRegExps()))
        {
          item.m_strPath = item2->m_strPath;
          item.m_bIsFolder = false;
          bFoundFile = true;
          break;
        }
      }

      // no video file in this folder
      if (!bFoundFile)
      {
        CGUIDialogOK::ShowAndGetInput(13346,20349,20022,20022);
        return;
      }
    }
  }

  // we need to also request any thumbs be applied to the folder item
  if (pItem->m_bIsFolder)
    item.SetProperty("set_folder_thumb", pItem->m_strPath);

  bool modified = ShowIMDB(&item, scraper);
  if (modified &&
     (g_windowManager.GetActiveWindow() == WINDOW_VIDEO_FILES ||
      g_windowManager.GetActiveWindow() == WINDOW_VIDEO_NAV)) // since we can be called from the music library we need this check
  {
    int itemNumber = m_viewControl.GetSelectedItem();
    Update(m_vecItems->m_strPath);
    m_viewControl.SetSelectedItem(itemNumber);
  }
}

// ShowIMDB is called as follows:
// 1.  To lookup info on a file.
// 2.  To lookup info on a folder (which may or may not contain a file)
// 3.  To lookup info just for fun (no file or folder related)

// We just need the item object for this.
// A "blank" item object is sent for 3.
// If a folder is sent, currently it sets strFolder and bFolder
// this is only used for setting the folder thumb, however.

// Steps should be:

// 1.  Check database to see if we have this information already
// 2.  Else, check for a nfoFile to get the URL
// 3.  Run a loop to check for refresh
// 4.  If no URL is present do a search to get the URL
// 4.  Once we have the URL, download the details
// 5.  Once we have the details, add to the database if necessary (case 1,2)
//     and show the information.
// 6.  Check for a refresh, and if so, go to 3.

bool CGUIWindowVideoBase::ShowIMDB(CFileItem *item, const ScraperPtr &info2)
{
  /*
  CLog::Log(LOGDEBUG,"CGUIWindowVideoBase::ShowIMDB");
  CLog::Log(LOGDEBUG,"  strMovie  = [%s]", strMovie.c_str());
  CLog::Log(LOGDEBUG,"  strFile   = [%s]", strFile.c_str());
  CLog::Log(LOGDEBUG,"  strFolder = [%s]", strFolder.c_str());
  CLog::Log(LOGDEBUG,"  bFolder   = [%s]", ((int)bFolder ? "true" : "false"));
  */

  CGUIDialogProgress* pDlgProgress = (CGUIDialogProgress*)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
  CGUIDialogSelect* pDlgSelect = (CGUIDialogSelect*)g_windowManager.GetWindow(WINDOW_DIALOG_SELECT);
  CGUIDialogVideoInfo* pDlgInfo = (CGUIDialogVideoInfo*)g_windowManager.GetWindow(WINDOW_DIALOG_VIDEO_INFO);

  ScraperPtr info(info2); // use this as nfo might change it..

  if (!pDlgProgress) return false;
  if (!pDlgSelect) return false;
  if (!pDlgInfo) return false;

  // 1.  Check for already downloaded information, and if we have it, display our dialog
  //     Return if no Refresh is needed.
  bool bHasInfo=false;

  CVideoInfoTag movieDetails;
  movieDetails.Reset();
  if (info)
  {
    m_database.Open(); // since we can be called from the music library

    if (info->Content() == CONTENT_MOVIES)
    {
      if (m_database.HasMovieInfo(item->m_strPath))
      {
        bHasInfo = true;
        m_database.GetMovieInfo(item->m_strPath, movieDetails);
      }
    }
    if (info->Content() == CONTENT_TVSHOWS)
    {
      if (item->m_bIsFolder)
      {
        if (m_database.HasTvShowInfo(item->m_strPath))
        {
          bHasInfo = true;
          m_database.GetTvShowInfo(item->m_strPath, movieDetails);
        }
      }
      else
      {
        int EpisodeHint=-1;
        if (item->HasVideoInfoTag())
          EpisodeHint = item->GetVideoInfoTag()->m_iEpisode;
        int idEpisode=-1;
        if ((idEpisode = m_database.GetEpisodeId(item->m_strPath,EpisodeHint)) > -1)
        {
          bHasInfo = true;
          m_database.GetEpisodeInfo(item->m_strPath, movieDetails, idEpisode);
        }
        else
        {
          // !! WORKAROUND !!
          // As we cannot add an episode to a non-existing tvshow entry, we have to check the parent directory
          // to see if it`s already in our video database. If it's not yet part of the database we will exit here.
          // (Ticket #4764)
          //
          // NOTE: This will fail for episodes on multipath shares, as the parent path isn't what is stored in the
          //       database.  Possible solutions are to store the paths in the db separately and rely on the show
          //       stacking stuff, or to modify GetTvShowId to do support multipath:// shares
          CStdString strParentDirectory;
          URIUtils::GetParentPath(item->m_strPath, strParentDirectory);
          if (m_database.GetTvShowId(strParentDirectory) < 0)
          {
            CLog::Log(LOGERROR,"%s: could not add episode [%s]. tvshow does not exist yet..", __FUNCTION__, item->m_strPath.c_str());
            return false;
          }
        }
      }
    }
    if (info->Content() == CONTENT_MUSICVIDEOS)
    {
      if (m_database.HasMusicVideoInfo(item->m_strPath))
      {
        bHasInfo = true;
        m_database.GetMusicVideoInfo(item->m_strPath, movieDetails);
      }
    }
    m_database.Close();
  }
  else if(item->HasVideoInfoTag())
  {
    bHasInfo = true;
    movieDetails = *item->GetVideoInfoTag();
  }
  
  bool needsRefresh = false;
  if (bHasInfo)
  {
    if (!info || info->Content() == CONTENT_NONE) // disable refresh button
      movieDetails.m_strIMDBNumber = "xx"+movieDetails.m_strIMDBNumber;
    *item->GetVideoInfoTag() = movieDetails;
    pDlgInfo->SetMovie(item);
    pDlgInfo->DoModal();
    needsRefresh = pDlgInfo->NeedRefresh();
    if (!needsRefresh)
      return pDlgInfo->HasUpdatedThumb();
  }

  // quietly return if Internet lookups are disabled
  if (!g_settings.GetCurrentProfile().canWriteDatabases() && !g_passwordManager.bMasterUser)
    return false;

  if(!info)
    return false;

  CGUIDialogVideoScan* pDialog = (CGUIDialogVideoScan*)g_windowManager.GetWindow(WINDOW_DIALOG_VIDEO_SCAN);
  if (pDialog && pDialog->IsScanning())
  {
    CGUIDialogOK::ShowAndGetInput(13346,14057,-1,-1);
    return false;
  }

  m_database.Open();
  // 2. Look for a nfo File to get the search URL
  SScanSettings settings;
  info = m_database.GetScraperForPath(item->m_strPath,settings);

  if (!info)
    return false;

  // Get the correct movie title
  CStdString movieName = item->GetMovieName(settings.parent_name);

  CScraperUrl scrUrl;
  CVideoInfoScanner scanner;
  bool hasDetails = false;
  bool listNeedsUpdating = false;
  bool ignoreNfo = false;
  // 3. Run a loop so that if we Refresh we re-run this block
  do
  {
    if (!ignoreNfo)
    {
      CNfoFile::NFOResult nfoResult = scanner.CheckForNFOFile(item,settings.parent_name_root,info,scrUrl);
      if (nfoResult == CNfoFile::ERROR_NFO)
        ignoreNfo = true;
      else
      if (nfoResult != CNfoFile::NO_NFO)
        hasDetails = true;

      if (needsRefresh)
      {
        bHasInfo = true;
        if (nfoResult == CNfoFile::URL_NFO || nfoResult == CNfoFile::COMBINED_NFO || nfoResult == CNfoFile::FULL_NFO)
        {
          if (CGUIDialogYesNo::ShowAndGetInput(13346,20446,20447,20022))
          {
            hasDetails = false;
            ignoreNfo = true;
            scrUrl.Clear();
            info = info2;
          }
        }
      }
    }

    // 4. if we don't have an url, or need to refresh the search
    //    then do the web search
    MOVIELIST movielist;
    if (info->Content() == CONTENT_TVSHOWS && !item->m_bIsFolder)
      hasDetails = true;

    if (!hasDetails && (scrUrl.m_url.size() == 0 || needsRefresh))
    {
      // 4a. show dialog that we're busy querying www.imdb.com
      CStdString strHeading;
      strHeading.Format(g_localizeStrings.Get(197),info->Name().c_str());
      pDlgProgress->SetHeading(strHeading);
      pDlgProgress->SetLine(0, movieName);
      pDlgProgress->SetLine(1, "");
      pDlgProgress->SetLine(2, "");
      pDlgProgress->StartModal();
      pDlgProgress->Progress();

      // 4b. do the websearch
      info->ClearCache();
      CVideoInfoDownloader imdb(info);
      int returncode = imdb.FindMovie(movieName, movielist, pDlgProgress);
      if (returncode > 0)
      {
        pDlgProgress->Close();
        if (movielist.size() > 0)
        {
          int iString = 196;
          if (info->Content() == CONTENT_TVSHOWS)
            iString = 20356;
          pDlgSelect->SetHeading(iString);
          pDlgSelect->Reset();
          for (unsigned int i = 0; i < movielist.size(); ++i)
            pDlgSelect->Add(movielist[i].strTitle);
          pDlgSelect->EnableButton(true, 413); // manual
          pDlgSelect->DoModal();

          // and wait till user selects one
          int iSelectedMovie = pDlgSelect->GetSelectedLabel();
          if (iSelectedMovie >= 0)
          {
            scrUrl = movielist[iSelectedMovie];
            CLog::Log(LOGDEBUG, "%s: user selected movie '%s' with URL '%s'",
              __FUNCTION__, scrUrl.strTitle.c_str(), scrUrl.m_url[0].m_url.c_str());
          }
          else if (!pDlgSelect->IsButtonPressed())
          {
            m_database.Close();
            return listNeedsUpdating; // user backed out
          }
        }
      }
      else if (returncode == -1 || !CVideoInfoScanner::DownloadFailed(pDlgProgress))
      {
        pDlgProgress->Close();
        return false;
      }
    }
    // 4c. Check if url is still empty - occurs if user has selected to do a manual
    //     lookup, or if the IMDb lookup failed or was cancelled.
    if (!hasDetails && scrUrl.m_url.size() == 0)
    {
      // Check for cancel of the progress dialog
      pDlgProgress->Close();
      if (pDlgProgress->IsCanceled())
      {
        m_database.Close();
        return listNeedsUpdating;
      }

      // Prompt the user to input the movieName
      int iString = 16009;
      if (info->Content() == CONTENT_TVSHOWS)
        iString = 20357;
      if (!CGUIDialogKeyboard::ShowAndGetInput(movieName, g_localizeStrings.Get(iString), false))
      {
        m_database.Close();
        return listNeedsUpdating; // user backed out
      }

      needsRefresh = true;
    }
    else
    {
      // 5. Download the movie information
      // show dialog that we're downloading the movie info
      CFileItemList list;
      CStdString strPath=item->m_strPath;
      if (item->IsVideoDb())
      {
        CFileItemPtr newItem(new CFileItem(*item->GetVideoInfoTag()));
        list.Add(newItem);
        strPath = item->GetVideoInfoTag()->m_strPath;
      }
      else
      {
        CFileItemPtr newItem(new CFileItem(*item));
        list.Add(newItem);
      }

      if (item->m_bIsFolder)
        URIUtils::GetParentPath(strPath,list.m_strPath);
      else
        URIUtils::GetDirectory(strPath,list.m_strPath);

      int iString=198;
      if (info->Content() == CONTENT_TVSHOWS)
      {
        if (item->m_bIsFolder)
          iString = 20353;
        else
          iString = 20361;
      }
      if (info->Content() == CONTENT_MUSICVIDEOS)
        iString = 20394;
      pDlgProgress->SetHeading(iString);
      pDlgProgress->SetLine(0, movieName);
      pDlgProgress->SetLine(1, scrUrl.strTitle);
      pDlgProgress->SetLine(2, "");
      pDlgProgress->StartModal();
      pDlgProgress->Progress();
      if (bHasInfo)
      {
        if (info->Content() == CONTENT_MOVIES)
          m_database.DeleteMovie(item->m_strPath);
        if (info->Content() == CONTENT_TVSHOWS && !item->m_bIsFolder)
          m_database.DeleteEpisode(item->m_strPath,movieDetails.m_iDbId);
        if (info->Content() == CONTENT_MUSICVIDEOS)
          m_database.DeleteMusicVideo(item->m_strPath);
        if (info->Content() == CONTENT_TVSHOWS && item->m_bIsFolder)
        {
          if (pDlgInfo->RefreshAll())
            m_database.DeleteTvShow(item->m_strPath);
          else
            m_database.DeleteDetailsForTvShow(item->m_strPath);
        }
      }
      if (scanner.RetrieveVideoInfo(list,settings.parent_name_root,info->Content(),!ignoreNfo,&scrUrl,pDlgInfo->RefreshAll(),pDlgProgress))
      {
        if (info->Content() == CONTENT_MOVIES)
          m_database.GetMovieInfo(item->m_strPath,movieDetails);
        if (info->Content() == CONTENT_MUSICVIDEOS)
          m_database.GetMusicVideoInfo(item->m_strPath,movieDetails);
        if (info->Content() == CONTENT_TVSHOWS)
        {
          // update tvshow info to get updated episode numbers
          if (item->m_bIsFolder)
            m_database.GetTvShowInfo(item->m_strPath,movieDetails);
          else
            m_database.GetEpisodeInfo(item->m_strPath,movieDetails);
        }

        // got all movie details :-)
        OutputDebugString("got details\n");
        pDlgProgress->Close();

        // now show the imdb info
        OutputDebugString("show info\n");

        // remove directory caches and reload images
        CUtil::DeleteVideoDatabaseDirectoryCache();
        CGUIMessage reload(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_REFRESH_THUMBS);
        OnMessage(reload);

        *item->GetVideoInfoTag() = movieDetails;
        pDlgInfo->SetMovie(item);
        pDlgInfo->DoModal();
        item->SetThumbnailImage(pDlgInfo->GetThumbnail());
        needsRefresh = pDlgInfo->NeedRefresh();
        listNeedsUpdating = true;
      }
      else
      {
        pDlgProgress->Close();
        if (pDlgProgress->IsCanceled())
        {
          m_database.Close();
          return listNeedsUpdating; // user cancelled
        }
        CGUIDialogOK::ShowAndGetInput(195, movieName, 0, 0);
        m_database.Close();
        return listNeedsUpdating;
      }
    }
  // 6. Check for a refresh
  } while (needsRefresh);
  m_database.Close();
  return listNeedsUpdating;
}

void CGUIWindowVideoBase::OnQueueItem(int iItem)
{
  if ( iItem < 0 || iItem >= m_vecItems->Size() ) return ;

  // we take a copy so that we can alter the queue state
  CFileItemPtr item(new CFileItem(*m_vecItems->Get(iItem)));
  if (item->IsRAR() || item->IsZIP())
    return;

  //  Allow queuing of unqueueable items
  //  when we try to queue them directly
  if (!item->CanQueue())
    item->SetCanQueue(true);

  CFileItemList queuedItems;
  AddItemToPlayList(item, queuedItems);
  // if party mode, add items but DONT start playing
  if (g_partyModeManager.IsEnabled(PARTYMODECONTEXT_VIDEO))
  {
    g_partyModeManager.AddUserSongs(queuedItems, false);
    return;
  }

  g_playlistPlayer.Add(PLAYLIST_VIDEO, queuedItems);
  // video does not auto play on queue like music
  m_viewControl.SetSelectedItem(iItem + 1);
}

void CGUIWindowVideoBase::AddItemToPlayList(const CFileItemPtr &pItem, CFileItemList &queuedItems)
{
  if (!pItem->CanQueue() || pItem->IsRAR() || pItem->IsZIP() || pItem->IsParentFolder()) // no zip/rar enques thank you!
    return;

  if (pItem->m_bIsFolder)
  {
    if (pItem->IsParentFolder())
      return;

    // Check if we add a locked share
    if ( pItem->m_bIsShareOrDrive )
    {
      CFileItem item = *pItem;
      if ( !g_passwordManager.IsItemUnlocked( &item, "video" ) )
        return;
    }

    // recursive
    CFileItemList items;
    GetDirectory(pItem->m_strPath, items);
    FormatAndSort(items);

    int watchedMode = g_settings.GetWatchMode(m_vecItems->GetContent());
    bool unwatchedOnly = watchedMode == VIDEO_SHOW_UNWATCHED;
    bool watchedOnly = watchedMode == VIDEO_SHOW_WATCHED;
    for (int i = 0; i < items.Size(); ++i)
    {
      if (items[i]->m_bIsFolder)
      {
        CStdString strPath = items[i]->m_strPath;
        URIUtils::RemoveSlashAtEnd(strPath);
        strPath.ToLower();
        if (strPath.size() > 6)
        {
          CStdString strSub = strPath.substr(strPath.size()-6);
          if (strPath.Mid(strPath.size()-6).Equals("sample")) // skip sample folders
            continue;
        }
      }
      else if (items[i]->HasVideoInfoTag() &&
       ((unwatchedOnly && items[i]->GetVideoInfoTag()->m_playCount > 0) ||
        (watchedOnly && items[i]->GetVideoInfoTag()->m_playCount <= 0)))
        continue;

      AddItemToPlayList(items[i], queuedItems);
    }
  }
  else
  {
    // just an item
    if (pItem->IsPlayList())
    {
      auto_ptr<CPlayList> pPlayList (CPlayListFactory::Create(*pItem));
      if (pPlayList.get())
      {
        // load it
        if (!pPlayList->Load(pItem->m_strPath))
        {
          CGUIDialogOK::ShowAndGetInput(6, 0, 477, 0);
          return; //hmmm unable to load playlist?
        }

        CPlayList playlist = *pPlayList;
        for (int i = 0; i < (int)playlist.size(); ++i)
        {
          AddItemToPlayList(playlist[i], queuedItems);
        }
        return;
      }
    }
    else if(pItem->IsInternetStream())
    { // just queue the internet stream, it will be expanded on play
      queuedItems.Add(pItem);
    }
    else if (pItem->IsPlugin() && pItem->GetProperty("isplayable") == "true")
    { // a playable python files
      queuedItems.Add(pItem);
    }
    else if (pItem->IsVideoDb())
    { // this case is needed unless we allow IsVideo() to return true for videodb items,
      // but then we have issues with playlists of videodb items
      CFileItemPtr item(new CFileItem(*pItem->GetVideoInfoTag()));
      queuedItems.Add(item);
    }
    else if (!pItem->IsNFO() && pItem->IsVideo())
    {
      queuedItems.Add(pItem);
    }
  }
}

int  CGUIWindowVideoBase::GetResumeItemOffset(const CFileItem *item)
{
  // do not resume livetv
  if (item->IsLiveTV())
    return 0;

  CVideoDatabase db;
  db.Open();
  long startoffset = 0;

  if (item->IsStack() && (!g_guiSettings.GetBool("myvideos.treatstackasfile") ||
                          CFileItem(CStackDirectory::GetFirstStackedFile(item->m_strPath),false).IsDVDImage()) )
  {

    CStdStringArray movies;
    GetStackedFiles(item->m_strPath, movies);

    /* check if any of the stacked files have a resume bookmark */
    for (unsigned i = 0; i<movies.size();i++)
    {
      CBookmark bookmark;
      if (db.GetResumeBookMark(movies[i], bookmark))
      {
        startoffset = (long)(bookmark.timeInSeconds*75);
        startoffset += 0x10000000 * (i+1); /* store file number in here */
        break;
      }
    }
  }
  else if (!item->IsNFO() && !item->IsPlayList())
  {
    CBookmark bookmark;
    CStdString strPath = item->m_strPath;
    if ((item->IsVideoDb() || item->IsDVD()) && item->HasVideoInfoTag())
      strPath = item->GetVideoInfoTag()->m_strFileNameAndPath;

    if (db.GetResumeBookMark(strPath, bookmark))
      startoffset = (long)(bookmark.timeInSeconds*75);
  }
  db.Close();

  return startoffset;
}

bool CGUIWindowVideoBase::OnClick(int iItem)
{
  return CGUIMediaWindow::OnClick(iItem);
}

bool CGUIWindowVideoBase::OnSelect(int iItem)
{
  if (iItem < 0 || iItem >= m_vecItems->Size())
    return false;

  CFileItemPtr item = m_vecItems->Get(iItem);

  if (!item->m_bIsFolder && item->m_strPath != "add")
    return OnFileAction(iItem, g_guiSettings.GetInt("myvideos.selectaction"));

  return CGUIMediaWindow::OnSelect(iItem);
}

bool CGUIWindowVideoBase::OnFileAction(int iItem, int action)
{
  CFileItemPtr item = m_vecItems->Get(iItem);
  
  switch (action)
  {
  case SELECT_ACTION_CHOOSE:
    {
      CContextButtons choices;
      bool resume = false;

      if (!item->IsLiveTV())
      {
        CStdString resumeString = GetResumeString(*item);
        if (!resumeString.IsEmpty()) 
        {
          resume = true;
          choices.Add(SELECT_ACTION_RESUME, resumeString);
          choices.Add(SELECT_ACTION_PLAY, 12021);   // Start from beginning
        }
      }
      if (!resume)
        choices.Add(SELECT_ACTION_PLAY, 208);   // Play

      choices.Add(SELECT_ACTION_INFO, 22081); // Info
      choices.Add(SELECT_ACTION_MORE, 22082); // More
      int value = CGUIDialogContextMenu::ShowAndGetChoice(choices);
      if (value < 0)
        return true;

      return OnFileAction(iItem, value);
    }
    break;
  case SELECT_ACTION_PLAY_OR_RESUME:
    return OnResumeItem(iItem);
  case SELECT_ACTION_INFO:
    if (OnInfo(iItem))
      return true;
    break;
  case SELECT_ACTION_MORE:
    OnPopupMenu(iItem);
    return true;
  case SELECT_ACTION_RESUME:
    item->m_lStartOffset = STARTOFFSET_RESUME;
    break;
  case SELECT_ACTION_PLAY:
  default:
    break;
  }
  return OnClick(iItem);
}

bool CGUIWindowVideoBase::OnInfo(int iItem) 
{
  if (iItem < 0 || iItem >= m_vecItems->Size())
    return false;

  CFileItemPtr item = m_vecItems->Get(iItem);

  if (item->m_strPath.Equals("add") || item->IsParentFolder())
    return false;

  ADDON::ScraperPtr scraper;
  if (!m_vecItems->IsPlugin() && !m_vecItems->IsRSS() && !m_vecItems->IsLiveTV())
  {
    CStdString strDir;
    if (item->IsVideoDb()       &&
        item->HasVideoInfoTag() &&
        !item->GetVideoInfoTag()->m_strPath.IsEmpty())
    {
      strDir = item->GetVideoInfoTag()->m_strPath;
    }
    else
      URIUtils::GetDirectory(item->m_strPath,strDir);

    SScanSettings settings;
    bool foundDirectly = false;
    scraper = m_database.GetScraperForPath(strDir, settings, foundDirectly);

    if (!scraper &&
        !(m_database.HasMovieInfo(item->m_strPath) ||
          m_database.HasTvShowInfo(strDir)           ||
          m_database.HasEpisodeInfo(item->m_strPath)))
    {
      return false;
    }

    if (scraper && scraper->Content() == CONTENT_TVSHOWS && foundDirectly && !settings.parent_name_root) // dont lookup on root tvshow folder
      return true;
  }

  OnInfo(item.get(), scraper);

  return true;
}

void CGUIWindowVideoBase::OnRestartItem(int iItem)
{
  CGUIMediaWindow::OnClick(iItem);
}

CStdString CGUIWindowVideoBase::GetResumeString(CFileItem item) 
{
  CStdString resumeString;
  CVideoDatabase db;
  if (db.Open())
  {
    CBookmark bookmark;
    CStdString itemPath(item.m_strPath);
    if (item.IsVideoDb() || item.IsDVD())
      itemPath = item.GetVideoInfoTag()->m_strFileNameAndPath;
    if (db.GetResumeBookMark(itemPath, bookmark) )
      resumeString.Format(g_localizeStrings.Get(12022).c_str(), StringUtils::SecondsToTimeString(lrint(bookmark.timeInSeconds)).c_str());
    db.Close();
  }
  return resumeString;
}

bool CGUIWindowVideoBase::ShowResumeMenu(CFileItem &item)
{
  if (!item.m_bIsFolder && !item.IsLiveTV())
  {
    CStdString resumeString = GetResumeString(item);
    if (!resumeString.IsEmpty())
    { // prompt user whether they wish to resume
      CContextButtons choices;
      choices.Add(1, resumeString);
      choices.Add(2, 12021); // start from the beginning
      int retVal = CGUIDialogContextMenu::ShowAndGetChoice(choices);
      if (retVal < 0)
        return false; // don't do anything
      if (retVal == 1)
        item.m_lStartOffset = STARTOFFSET_RESUME;
    }
  }
  return true;
}

bool CGUIWindowVideoBase::OnResumeItem(int iItem)
{
  if (iItem < 0 || iItem >= m_vecItems->Size()) return true;
  CFileItemPtr item = m_vecItems->Get(iItem);

  if (item->m_bIsFolder)
  {
    // resuming directories isn't supported yet. play.
    PlayItem(iItem);
    return true;
  }

  CStdString resumeString = GetResumeString(*item);

  if (!resumeString.IsEmpty())
  {
    CContextButtons choices;
    choices.Add(SELECT_ACTION_RESUME, resumeString);
    choices.Add(SELECT_ACTION_PLAY, 12021);   // Start from beginning
    int value = CGUIDialogContextMenu::ShowAndGetChoice(choices);
    if (value < 0)
      return true;
    return OnFileAction(iItem, value);
  }

  return OnFileAction(iItem, SELECT_ACTION_PLAY);
}

void CGUIWindowVideoBase::OnStreamDetails(const CStreamDetails &details, const CStdString &strFileName, long lFileId)
{
  CVideoDatabase db;
  if (db.Open())
  {
    if (lFileId < 0)
      db.SetStreamDetailsForFile(details, strFileName);
    else
      db.SetStreamDetailsForFileId(details, lFileId);

    db.Close();
  }
}

void CGUIWindowVideoBase::GetContextButtons(int itemNumber, CContextButtons &buttons)
{
  CFileItemPtr item;
  if (itemNumber >= 0 && itemNumber < m_vecItems->Size())
    item = m_vecItems->Get(itemNumber);

  // contextual buttons
  if (item && !item->GetPropertyBOOL("pluginreplacecontextitems"))
  {
    if (!item->IsParentFolder())
    {
      CStdString path(item->m_strPath);
      if (item->IsVideoDb() && item->HasVideoInfoTag())
        path = item->GetVideoInfoTag()->m_strFileNameAndPath;

      if (!item->IsPlugin() && !item->IsAddonsPath() && !item->IsLiveTV())
      {
        if (URIUtils::IsStack(path))
        {
          vector<int> times;
          if (m_database.GetStackTimes(path,times))
            buttons.Add(CONTEXT_BUTTON_PLAY_PART, 20324);
        }

        if (!m_vecItems->m_strPath.IsEmpty() && !item->m_strPath.Left(19).Equals("newsmartplaylist://")
            && !m_vecItems->m_strPath.Left(10).Equals("sources://"))
        {
          buttons.Add(CONTEXT_BUTTON_QUEUE_ITEM, 13347);      // Add to Playlist
        }

        // allow a folder to be ad-hoc queued and played by the default player
        if (item->m_bIsFolder || (item->IsPlayList() && !g_advancedSettings.ShowPlaylistAsFolders()))
        {
          buttons.Add(CONTEXT_BUTTON_PLAY_ITEM, 208);
        }
      }
      if (!item->m_bIsFolder && !(item->IsPlayList() && !g_advancedSettings.ShowPlaylistAsFolders()))
      { // get players
        VECPLAYERCORES vecCores;
        if (item->IsVideoDb())
        {
          CFileItem item2;
          item2.m_strPath = item->GetVideoInfoTag()->m_strFileNameAndPath;
          CPlayerCoreFactory::GetPlayers(item2, vecCores);
        }
        else
          CPlayerCoreFactory::GetPlayers(*item, vecCores);
        if (vecCores.size() > 1)
          buttons.Add(CONTEXT_BUTTON_PLAY_WITH, 15213);
      }
      if (item->IsSmartPlayList())
      {
        buttons.Add(CONTEXT_BUTTON_PLAY_PARTYMODE, 15216); // Play in Partymode
      }

      // if autoresume is enabled then add restart video button
      // check to see if the Resume Video button is applicable
      // only if the video is NOT a DVD (in that case the resume button will be added by CGUIDialogContextMenu::GetContextButtons)
      if (!item->IsDVD() && GetResumeItemOffset(item.get()) > 0)
      {
        buttons.Add(CONTEXT_BUTTON_RESUME_ITEM, GetResumeString(*(item.get())));     // Resume Video
      }
      if (item->HasVideoInfoTag() && !item->m_bIsFolder && item->GetVideoInfoTag()->m_iEpisode > -1)
      {
        buttons.Add(CONTEXT_BUTTON_PLAY_AND_QUEUE, 13412);
      }
      if (item->IsSmartPlayList() || m_vecItems->IsSmartPlayList())
        buttons.Add(CONTEXT_BUTTON_EDIT_SMART_PLAYLIST, 586);
    }
  }
  CGUIMediaWindow::GetContextButtons(itemNumber, buttons);
}

void CGUIWindowVideoBase::GetNonContextButtons(int itemNumber, CContextButtons &buttons)
{
  if (g_playlistPlayer.GetPlaylist(PLAYLIST_VIDEO).size() > 0)
    buttons.Add(CONTEXT_BUTTON_NOW_PLAYING, 13350);
}

bool CGUIWindowVideoBase::OnContextButton(int itemNumber, CONTEXT_BUTTON button)
{
  CFileItemPtr item;
  if (itemNumber >= 0 && itemNumber < m_vecItems->Size())
    item = m_vecItems->Get(itemNumber);
  switch (button)
  {
  case CONTEXT_BUTTON_SET_CONTENT:
    {
      SScanSettings settings;
      ADDON::ScraperPtr info = m_database.GetScraperForPath(item->HasVideoInfoTag() ? item->GetVideoInfoTag()->m_strPath : item->m_strPath, settings);
      OnAssignContent(item->m_strPath,0, info, settings);
      return true;
    }
  case CONTEXT_BUTTON_PLAY_PART:
    {
      CFileItemList items;
      CStdString path(item->m_strPath);
      if (item->IsVideoDb())
        path = item->GetVideoInfoTag()->m_strFileNameAndPath;

      CDirectory::GetDirectory(path,items);
      CGUIDialogFileStacking* dlg = (CGUIDialogFileStacking*)g_windowManager.GetWindow(WINDOW_DIALOG_FILESTACKING);
      if (!dlg) return true;
      dlg->SetNumberOfFiles(items.Size());
      dlg->DoModal();
      int btn2 = dlg->GetSelectedFile();
      if (btn2 > 0)
      {
        if (btn2 > 1)
        {
          vector<int> times;
          if (m_database.GetStackTimes(path,times))
            item->m_lStartOffset = times[btn2-2]*75; // wtf?
        }
        else
          item->m_lStartOffset = 0;

        // call CGUIMediaWindow::OnClick() as otherwise autoresume will kick in
        CGUIMediaWindow::OnClick(itemNumber);
      }
      return true;
    }
  case CONTEXT_BUTTON_QUEUE_ITEM:
    OnQueueItem(itemNumber);
    return true;

  case CONTEXT_BUTTON_PLAY_ITEM:
    PlayItem(itemNumber);
    return true;

  case CONTEXT_BUTTON_PLAY_WITH:
    {
      VECPLAYERCORES vecCores;
      if (item->IsVideoDb())
      {
        CFileItem item2(*item->GetVideoInfoTag());
        CPlayerCoreFactory::GetPlayers(item2, vecCores);
      }
      else
        CPlayerCoreFactory::GetPlayers(*item, vecCores);
      g_application.m_eForcedNextPlayer = CPlayerCoreFactory::SelectPlayerDialog(vecCores);
      if (g_application.m_eForcedNextPlayer != EPC_NONE)
        OnClick(itemNumber);
      return true;
    }

  case CONTEXT_BUTTON_PLAY_PARTYMODE:
    g_partyModeManager.Enable(PARTYMODECONTEXT_VIDEO, m_vecItems->Get(itemNumber)->m_strPath);
    return true;

  case CONTEXT_BUTTON_RESTART_ITEM:
    OnRestartItem(itemNumber);
    return true;

  case CONTEXT_BUTTON_RESUME_ITEM:
    return OnFileAction(itemNumber, SELECT_ACTION_RESUME);

  case CONTEXT_BUTTON_NOW_PLAYING:
    g_windowManager.ActivateWindow(WINDOW_VIDEO_PLAYLIST);
    return true;

  case CONTEXT_BUTTON_INFO:
    {
      ADDON::ScraperPtr info;
      VIDEO::SScanSettings settings;
      GetScraperForItem(item.get(), info, settings);

      OnInfo(item.get(),info);
      return true;
    }
  case CONTEXT_BUTTON_STOP_SCANNING:
    {
      CGUIDialogVideoScan *pScanDlg = (CGUIDialogVideoScan *)g_windowManager.GetWindow(WINDOW_DIALOG_VIDEO_SCAN);
      if (pScanDlg && pScanDlg->IsScanning())
        pScanDlg->StopScanning();
      return true;
    }
  case CONTEXT_BUTTON_SCAN:
  case CONTEXT_BUTTON_UPDATE_TVSHOW:
    {
      if( !item)
        return false;
      ADDON::ScraperPtr info;
      SScanSettings settings;
      GetScraperForItem(item.get(), info, settings);
      CStdString strPath = item->m_strPath;
      if (item->IsVideoDb() && (!item->m_bIsFolder || item->GetVideoInfoTag()->m_strPath.IsEmpty()))
        return false;

      if (item->IsVideoDb())
        strPath = item->GetVideoInfoTag()->m_strPath;

      if (!info || info->Content() == CONTENT_NONE)
        return false;

      if (item->m_bIsFolder)
      {
        m_database.SetPathHash(strPath,""); // to force scan
        OnScan(strPath);
      }
      else
        OnInfo(item.get(),info);

      return true;
    }
  case CONTEXT_BUTTON_DELETE:
    OnDeleteItem(itemNumber);
    return true;
  case CONTEXT_BUTTON_EDIT_SMART_PLAYLIST:
    {
      CStdString playlist = m_vecItems->Get(itemNumber)->IsSmartPlayList() ? m_vecItems->Get(itemNumber)->m_strPath : m_vecItems->m_strPath; // save path as activatewindow will destroy our items
      if (CGUIDialogSmartPlaylistEditor::EditPlaylist(playlist, "video"))
      { // need to update
        m_vecItems->RemoveDiscCache(GetID());
        Update(m_vecItems->m_strPath);
      }
      return true;
    }
  case CONTEXT_BUTTON_RENAME:
    OnRenameItem(itemNumber);
    return true;
  case CONTEXT_BUTTON_MARK_WATCHED:
    {
      int newSelection = m_viewControl.GetSelectedItem() + 1;
      MarkWatched(item,true);
      m_viewControl.SetSelectedItem(newSelection);

      CUtil::DeleteVideoDatabaseDirectoryCache();
      Update(m_vecItems->m_strPath);
      return true;
    }
  case CONTEXT_BUTTON_MARK_UNWATCHED:
    MarkWatched(item,false);
    CUtil::DeleteVideoDatabaseDirectoryCache();
    Update(m_vecItems->m_strPath);
    return true;
  case CONTEXT_BUTTON_PLAY_AND_QUEUE:
    return OnPlayAndQueueMedia(item);
  default:
    break;
  }
  return CGUIMediaWindow::OnContextButton(itemNumber, button);
}

void CGUIWindowVideoBase::GetStackedFiles(const CStdString &strFilePath1, vector<CStdString> &movies)
{
  CStdString strFilePath = strFilePath1;  // we're gonna be altering it

  movies.clear();

  CURL url(strFilePath);
  if (url.GetProtocol() == "stack")
  {
    CStackDirectory dir;
    CFileItemList items;
    dir.GetDirectory(strFilePath, items);
    for (int i = 0; i < items.Size(); ++i)
      movies.push_back(items[i]->m_strPath);
  }
  if (movies.empty())
    movies.push_back(strFilePath);
}

bool CGUIWindowVideoBase::OnPlayMedia(int iItem)
{
  if ( iItem < 0 || iItem >= (int)m_vecItems->Size() )
    return false;

  CFileItemPtr pItem = m_vecItems->Get(iItem);

  // party mode
  if (g_partyModeManager.IsEnabled(PARTYMODECONTEXT_VIDEO))
  {
    CPlayList playlistTemp;
    playlistTemp.Add(pItem);
    g_partyModeManager.AddUserSongs(playlistTemp, true);
    return true;
  }

  // Reset Playlistplayer, playback started now does
  // not use the playlistplayer.
  g_playlistPlayer.Reset();
  g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_NONE);

  CFileItem item(*pItem);
  if (pItem->IsVideoDb())
  {
    item.m_strPath = pItem->GetVideoInfoTag()->m_strFileNameAndPath;
    item.SetProperty("original_listitem_url", pItem->m_strPath);
  }

  PlayMovie(&item);

  return true;
}

bool CGUIWindowVideoBase::OnPlayAndQueueMedia(const CFileItemPtr &item)
{
  // Get the current playlist and make sure it is not shuffled
  int iPlaylist = m_guiState->GetPlaylist();
  if (iPlaylist != PLAYLIST_NONE && g_playlistPlayer.IsShuffled(iPlaylist))
     g_playlistPlayer.SetShuffle(iPlaylist, false);

  // Call the base method to actually queue the items
  // and start playing the given item
  return CGUIMediaWindow::OnPlayAndQueueMedia(item);
}

void CGUIWindowVideoBase::PlayMovie(const CFileItem *item)
{
  CFileItemList movieList;
  int selectedFile = 1;
  long startoffset = item->m_lStartOffset;

  if (item->IsStack() && (!g_guiSettings.GetBool("myvideos.treatstackasfile") ||
                          CFileItem(CStackDirectory::GetFirstStackedFile(item->m_strPath),false).IsDVDImage()) )
  {
    CStdStringArray movies;
    GetStackedFiles(item->m_strPath, movies);

    if (item->m_lStartOffset == STARTOFFSET_RESUME)
    {
      startoffset = GetResumeItemOffset(item);

      if (startoffset & 0xF0000000) /* file is specified as a flag */
      {
        selectedFile = (startoffset>>28);
        startoffset = startoffset & ~0xF0000000;
      }
      else
      {
        /* attempt to start on a specific time in a stack */
        /* if we are lucky, we might have stored timings for */
        /* this stack at some point */

        m_database.Open();

        /* figure out what file this time offset is */
        vector<int> times;
        m_database.GetStackTimes(item->m_strPath, times);
        long totaltime = 0;
        for (unsigned i = 0; i < times.size(); i++)
        {
          totaltime += times[i]*75;
          if (startoffset < totaltime )
          {
            selectedFile = i+1;
            startoffset -= totaltime - times[i]*75; /* rebase agains selected file */
            break;
          }
        }
        m_database.Close();
      }
    }
    else
    { // show file stacking dialog
      CGUIDialogFileStacking* dlg = (CGUIDialogFileStacking*)g_windowManager.GetWindow(WINDOW_DIALOG_FILESTACKING);
      if (dlg)
      {
        dlg->SetNumberOfFiles(movies.size());
        dlg->DoModal();
        selectedFile = dlg->GetSelectedFile();
        if (selectedFile < 1)
          return;
      }
    }
    // add to our movie list
    for (unsigned int i = 0; i < movies.size(); i++)
    {
      CFileItemPtr movieItem(new CFileItem(movies[i], false));
      movieList.Add(movieItem);
    }
  }
  else
  {
    CFileItemPtr movieItem(new CFileItem(*item));
    movieList.Add(movieItem);
  }

  g_playlistPlayer.Reset();
  g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_VIDEO);
  CPlayList& playlist = g_playlistPlayer.GetPlaylist(PLAYLIST_VIDEO);
  playlist.Clear();
  for (int i = selectedFile - 1; i < (int)movieList.Size(); ++i)
  {
    if (i == selectedFile - 1)
      movieList[i]->m_lStartOffset = startoffset;
    playlist.Add(movieList[i]);
  }

  if(m_thumbLoader.IsLoading())
    m_thumbLoader.StopAsync();

  // play movie...
  g_playlistPlayer.Play(0);

  if(!g_application.IsPlayingVideo())
    m_thumbLoader.Load(*m_vecItems);
}

void CGUIWindowVideoBase::OnDeleteItem(int iItem)
{
  if ( iItem < 0 || iItem >= m_vecItems->Size())
    return;

  OnDeleteItem(m_vecItems->Get(iItem));

  Update(m_vecItems->m_strPath);
  m_viewControl.SetSelectedItem(iItem);
}

void CGUIWindowVideoBase::OnDeleteItem(CFileItemPtr item)
{
  // HACK: stacked files need to be treated as folders in order to be deleted
  if (item->IsStack())
    item->m_bIsFolder = true;
  if (g_settings.GetCurrentProfile().getLockMode() != LOCK_MODE_EVERYONE &&
      g_settings.GetCurrentProfile().filesLocked())
  {
    if (!g_passwordManager.IsMasterLockUnlocked(true))
      return;
  }

  if (g_guiSettings.GetBool("filelists.allowfiledeletion") &&
      CUtil::SupportsFileOperations(item->m_strPath))
    CFileUtils::DeleteItem(item);
}

void CGUIWindowVideoBase::MarkWatched(const CFileItemPtr &item, bool bMark)
{
  if (!g_settings.GetCurrentProfile().canWriteDatabases())
    return;
  // dont allow update while scanning
  CGUIDialogVideoScan* pDialogScan = (CGUIDialogVideoScan*)g_windowManager.GetWindow(WINDOW_DIALOG_VIDEO_SCAN);
  if (pDialogScan && pDialogScan->IsScanning())
  {
    CGUIDialogOK::ShowAndGetInput(257, 0, 14057, 0);
    return;
  }

  CVideoDatabase database;
  if (database.Open())
  {
    CFileItemList items;
    if (item->m_bIsFolder)
    {
      CStdString strPath = item->m_strPath;
      CDirectory::GetDirectory(strPath, items);
    }
    else
      items.Add(item);

    for (int i=0;i<items.Size();++i)
    {
      CFileItemPtr pItem=items[i];
      if (pItem->m_bIsFolder)
      {
        MarkWatched(pItem, bMark);
        continue;
      }

      if (pItem->HasVideoInfoTag() &&
          (( bMark && pItem->GetVideoInfoTag()->m_playCount) ||
           (!bMark && !(pItem->GetVideoInfoTag()->m_playCount))))
        continue;

      // Clear resume bookmark
      if (bMark)
        database.ClearBookMarksOfFile(pItem->m_strPath, CBookmark::RESUME);

      database.SetPlayCount(*pItem, bMark ? 1 : 0);
    }
    
    database.Close(); 
  }
}

//Add change a title's name
void CGUIWindowVideoBase::UpdateVideoTitle(const CFileItem* pItem)
{
  // dont allow update while scanning
  CGUIDialogVideoScan* pDialogScan = (CGUIDialogVideoScan*)g_windowManager.GetWindow(WINDOW_DIALOG_VIDEO_SCAN);
  if (pDialogScan && pDialogScan->IsScanning())
  {
    CGUIDialogOK::ShowAndGetInput(257, 0, 14057, 0);
    return;
  }

  CVideoInfoTag detail;
  CVideoDatabase database;
  database.Open();
  CVideoDatabaseDirectory dir;
  CQueryParams params;
  dir.GetQueryParams(pItem->m_strPath,params);
  int iDbId = pItem->GetVideoInfoTag()->m_iDbId;

  VIDEODB_CONTENT_TYPE iType=VIDEODB_CONTENT_MOVIES;
  if (pItem->HasVideoInfoTag() && (!pItem->GetVideoInfoTag()->m_strShowTitle.IsEmpty() ||
      pItem->GetVideoInfoTag()->m_iEpisode > 0))
  {
    iType = VIDEODB_CONTENT_TVSHOWS;
  }
  if (pItem->HasVideoInfoTag() && pItem->GetVideoInfoTag()->m_iSeason > -1 && !pItem->m_bIsFolder)
    iType = VIDEODB_CONTENT_EPISODES;
  if (pItem->HasVideoInfoTag() && !pItem->GetVideoInfoTag()->m_strArtist.IsEmpty())
    iType = VIDEODB_CONTENT_MUSICVIDEOS;
  if (params.GetSetId() != -1 && params.GetMovieId() == -1)
    iType = VIDEODB_CONTENT_MOVIE_SETS;
  if (iType == VIDEODB_CONTENT_MOVIES)
    database.GetMovieInfo("", detail, pItem->GetVideoInfoTag()->m_iDbId);
  if (iType == VIDEODB_CONTENT_MOVIE_SETS)
    database.GetSetInfo(params.GetSetId(), detail);
  if (iType == VIDEODB_CONTENT_EPISODES)
    database.GetEpisodeInfo(pItem->m_strPath,detail,pItem->GetVideoInfoTag()->m_iDbId);
  if (iType == VIDEODB_CONTENT_TVSHOWS)
    database.GetTvShowInfo(pItem->GetVideoInfoTag()->m_strFileNameAndPath,detail,pItem->GetVideoInfoTag()->m_iDbId);
  if (iType == VIDEODB_CONTENT_MUSICVIDEOS)
    database.GetMusicVideoInfo(pItem->GetVideoInfoTag()->m_strFileNameAndPath,detail,pItem->GetVideoInfoTag()->m_iDbId);

  CStdString strInput;
  strInput = detail.m_strTitle;

  //Get the new title
  if (!CGUIDialogKeyboard::ShowAndGetInput(strInput, g_localizeStrings.Get(16105), false))
    return;

  database.UpdateMovieTitle(iDbId, strInput, iType);
}

void CGUIWindowVideoBase::LoadPlayList(const CStdString& strPlayList, int iPlayList /* = PLAYLIST_VIDEO */)
{
  // if partymode is active, we disable it
  if (g_partyModeManager.IsEnabled())
    g_partyModeManager.Disable();

  // load a playlist like .m3u, .pls
  // first get correct factory to load playlist
  auto_ptr<CPlayList> pPlayList (CPlayListFactory::Create(strPlayList));
  if (pPlayList.get())
  {
    // load it
    if (!pPlayList->Load(strPlayList))
    {
      CGUIDialogOK::ShowAndGetInput(6, 0, 477, 0);
      return; //hmmm unable to load playlist?
    }
  }

  if (g_application.ProcessAndStartPlaylist(strPlayList, *pPlayList, iPlayList))
  {
    if (m_guiState.get())
      m_guiState->SetPlaylistDirectory("playlistvideo://");
  }
}

void CGUIWindowVideoBase::PlayItem(int iItem)
{
  // restrictions should be placed in the appropiate window code
  // only call the base code if the item passes since this clears
  // the currently playing temp playlist

  const CFileItemPtr pItem = m_vecItems->Get(iItem);
  // if its a folder, build a temp playlist
  if (pItem->m_bIsFolder && !pItem->IsPlugin())
  {
    // take a copy so we can alter the queue state
    CFileItemPtr item(new CFileItem(*m_vecItems->Get(iItem)));

    //  Allow queuing of unqueueable items
    //  when we try to queue them directly
    if (!item->CanQueue())
      item->SetCanQueue(true);

    // skip ".."
    if (item->IsParentFolder())
      return;

    // recursively add items to list
    CFileItemList queuedItems;
    AddItemToPlayList(item, queuedItems);

    g_playlistPlayer.ClearPlaylist(PLAYLIST_VIDEO);
    g_playlistPlayer.Reset();
    g_playlistPlayer.Add(PLAYLIST_VIDEO, queuedItems);
    g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_VIDEO);
    g_playlistPlayer.Play();
  }
  else if (pItem->IsPlayList())
  {
    // load the playlist the old way
    LoadPlayList(pItem->m_strPath, PLAYLIST_VIDEO);
  }
  else
  {
    // single item, play it
    OnClick(iItem);
  }
}

bool CGUIWindowVideoBase::Update(const CStdString &strDirectory)
{
  if (m_thumbLoader.IsLoading())
    m_thumbLoader.StopThread();

  if (!CGUIMediaWindow::Update(strDirectory))
    return false;

  m_thumbLoader.Load(*m_vecItems);

  return true;
}

bool CGUIWindowVideoBase::GetDirectory(const CStdString &strDirectory, CFileItemList &items)
{
  bool bResult = CGUIMediaWindow::GetDirectory(strDirectory,items);

  // add in the "New Playlist" item if we're in the playlists folder
  if ((items.m_strPath == "special://videoplaylists/") && !items.Contains("newplaylist://"))
  {
    CFileItemPtr newPlaylist(new CFileItem(g_settings.GetUserDataItem("PartyMode-Video.xsp"),false));
    newPlaylist->SetLabel(g_localizeStrings.Get(16035));
    newPlaylist->SetLabelPreformated(true);
    newPlaylist->m_bIsFolder = true;
    items.Add(newPlaylist);

/*    newPlaylist.reset(new CFileItem("newplaylist://", false));
    newPlaylist->SetLabel(g_localizeStrings.Get(525));
    newPlaylist->SetLabelPreformated(true);
    items.Add(newPlaylist);
*/
    newPlaylist.reset(new CFileItem("newsmartplaylist://video", false));
    newPlaylist->SetLabel(g_localizeStrings.Get(21437));  // "new smart playlist..."
    newPlaylist->SetLabelPreformated(true);
    items.Add(newPlaylist);
  }

  m_stackingAvailable = !(items.IsTuxBox() || items.IsPlugin() ||
                          items.IsAddonsPath() || items.IsRSS() ||
                          items.IsInternetStream() || items.IsVideoDb());
  // we may also be in a tvshow files listing
  // (ideally this should be removed, and our stack regexps tidied up if necessary
  // No "normal" episodes should stack, and multi-parts should be supported)
  ADDON::ScraperPtr info = m_database.GetScraperForPath(strDirectory);
  if (info && info->Content() == CONTENT_TVSHOWS)
    m_stackingAvailable = false;

  if (m_stackingAvailable && !items.IsStack() && g_settings.m_videoStacking)
    items.Stack();

  return bResult;
}

void CGUIWindowVideoBase::OnPrepareFileItems(CFileItemList &items)
{
  if (!items.m_strPath.Equals("plugin://video/"))
    items.SetCachedVideoThumbs();

  if (items.GetContent() != "episodes")
  { // we don't set cached fanart for episodes, as this requires a db fetch per episode
    for (int i = 0; i < items.Size(); ++i)
    {
      CFileItemPtr item = items[i];
      if (!item->HasProperty("fanart_image"))
      {
        CStdString art = item->GetCachedFanart();
        if (CFile::Exists(art))
          item->SetProperty("fanart_image", art);
      }
    }
  }
}

void CGUIWindowVideoBase::AddToDatabase(int iItem)
{
  if (iItem < 0 || iItem >= m_vecItems->Size())
    return;

  CFileItemPtr pItem = m_vecItems->Get(iItem);
  if (pItem->IsParentFolder() || pItem->m_bIsFolder)
    return;

  CVideoInfoTag movie;
  movie.Reset();

  // prompt for data
  // enter a new title
  CStdString strTitle = pItem->GetLabel();
  if (!CGUIDialogKeyboard::ShowAndGetInput(strTitle, g_localizeStrings.Get(528), false)) // Enter Title
    return;

  // pick genre
  CGUIDialogSelect* pSelect = (CGUIDialogSelect*)g_windowManager.GetWindow(WINDOW_DIALOG_SELECT);
  if (!pSelect)
    return;

  pSelect->SetHeading(530); // Select Genre
  pSelect->Reset();
  CFileItemList items;
  if (!CDirectory::GetDirectory("videodb://1/1/", items))
    return;
  pSelect->SetItems(&items);
  pSelect->EnableButton(true, 531); // New Genre
  pSelect->DoModal();
  CStdString strGenre;
  int iSelected = pSelect->GetSelectedLabel();
  if (iSelected >= 0)
    strGenre = items[iSelected]->GetLabel();
  else if (!pSelect->IsButtonPressed())
    return;

  // enter new genre string
  if (strGenre.IsEmpty())
  {
    strGenre = g_localizeStrings.Get(532); // Manual Addition
    if (!CGUIDialogKeyboard::ShowAndGetInput(strGenre, g_localizeStrings.Get(533), false)) // Enter Genre
      return; // user backed out
    if (strGenre.IsEmpty())
      return; // no genre string
  }

  // set movie info
  movie.m_strTitle = strTitle;
  movie.m_strGenre = strGenre;

  // everything is ok, so add to database
  m_database.Open();
  int idMovie = m_database.AddMovie(pItem->m_strPath);
  movie.m_strIMDBNumber.Format("xx%08i", idMovie);
  m_database.SetDetailsForMovie(pItem->m_strPath, movie);
  m_database.Close();

  // done...
  CGUIDialogOK::ShowAndGetInput(20177, movie.m_strTitle, movie.m_strGenre, movie.m_strIMDBNumber);

  // library view cache needs to be cleared
  CUtil::DeleteVideoDatabaseDirectoryCache();
}

/// \brief Search the current directory for a string got from the virtual keyboard
void CGUIWindowVideoBase::OnSearch()
{
  CStdString strSearch;
  if (!CGUIDialogKeyboard::ShowAndGetInput(strSearch, g_localizeStrings.Get(16017), false))
    return ;

  strSearch.ToLower();
  if (m_dlgProgress)
  {
    m_dlgProgress->SetHeading(194);
    m_dlgProgress->SetLine(0, strSearch);
    m_dlgProgress->SetLine(1, "");
    m_dlgProgress->SetLine(2, "");
    m_dlgProgress->StartModal();
    m_dlgProgress->Progress();
  }
  CFileItemList items;
  DoSearch(strSearch, items);

  if (m_dlgProgress)
    m_dlgProgress->Close();

  if (items.Size())
  {
    CGUIDialogSelect* pDlgSelect = (CGUIDialogSelect*)g_windowManager.GetWindow(WINDOW_DIALOG_SELECT);
    pDlgSelect->Reset();
    pDlgSelect->SetHeading(283);

    for (int i = 0; i < (int)items.Size(); i++)
    {
      CFileItemPtr pItem = items[i];
      pDlgSelect->Add(pItem->GetLabel());
    }

    pDlgSelect->DoModal();

    int iItem = pDlgSelect->GetSelectedLabel();
    if (iItem < 0)
      return;

    CFileItemPtr pSelItem = items[iItem];

    OnSearchItemFound(pSelItem.get());
  }
  else
  {
    CGUIDialogOK::ShowAndGetInput(194, 284, 0, 0);
  }
}

/// \brief React on the selected search item
/// \param pItem Search result item
void CGUIWindowVideoBase::OnSearchItemFound(const CFileItem* pSelItem)
{
  if (pSelItem->m_bIsFolder)
  {
    CStdString strPath = pSelItem->m_strPath;
    CStdString strParentPath;
    URIUtils::GetParentPath(strPath, strParentPath);

    Update(strParentPath);

    if (pSelItem->IsVideoDb() && g_settings.m_bMyVideoNavFlatten)
      SetHistoryForPath("");
    else
      SetHistoryForPath(strParentPath);

    strPath = pSelItem->m_strPath;
    CURL url(strPath);
    if (pSelItem->IsSmb() && !URIUtils::HasSlashAtEnd(strPath))
      strPath += "/";

    for (int i = 0; i < m_vecItems->Size(); i++)
    {
      CFileItemPtr pItem = m_vecItems->Get(i);
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
    URIUtils::GetDirectory(pSelItem->m_strPath, strPath);

    Update(strPath);

    if (pSelItem->IsVideoDb() && g_settings.m_bMyVideoNavFlatten)
      SetHistoryForPath("");
    else
      SetHistoryForPath(strPath);

    for (int i = 0; i < (int)m_vecItems->Size(); i++)
    {
      CFileItemPtr pItem = m_vecItems->Get(i);
      if (pItem->m_strPath == pSelItem->m_strPath)
      {
        m_viewControl.SetSelectedItem(i);
        break;
      }
    }
  }
  m_viewControl.SetFocused();
}

int CGUIWindowVideoBase::GetScraperForItem(CFileItem *item, ADDON::ScraperPtr &info, SScanSettings& settings)
{
  if (!item)
    return 0;

  if (m_vecItems->IsPlugin() || m_vecItems->IsRSS())
  {
    info.reset();
    return 0;
  }
  else if(m_vecItems->IsLiveTV())
  {
    info.reset();
    return 0;
  }

  bool foundDirectly = false;
  info = m_database.GetScraperForPath(item->HasVideoInfoTag() ? item->GetVideoInfoTag()->m_strPath : item->m_strPath, settings, foundDirectly);
  return foundDirectly ? 1 : 0;
}

void CGUIWindowVideoBase::OnScan(const CStdString& strPath, bool scanAll)
{
  CGUIDialogVideoScan* pDialog = (CGUIDialogVideoScan*)g_windowManager.GetWindow(WINDOW_DIALOG_VIDEO_SCAN);
  if (pDialog)
    pDialog->StartScanning(strPath, scanAll);
}

CStdString CGUIWindowVideoBase::GetStartFolder(const CStdString &dir)
{
  if (dir.Equals("$PLAYLISTS") || dir.Equals("Playlists"))
    return "special://videoplaylists/";
  else if (dir.Equals("Plugins") || dir.Equals("Addons"))
    return "addons://sources/video/";
  return CGUIMediaWindow::GetStartFolder(dir);
}

void CGUIWindowVideoBase::AppendAndClearSearchItems(CFileItemList &searchItems, const CStdString &prependLabel, CFileItemList &results)
{
  if (!searchItems.Size())
    return;

  searchItems.Sort(g_guiSettings.GetBool("filelists.ignorethewhensorting") ? SORT_METHOD_LABEL_IGNORE_THE : SORT_METHOD_LABEL, SORT_ORDER_ASC);
  for (int i = 0; i < searchItems.Size(); i++)
    searchItems[i]->SetLabel(prependLabel + searchItems[i]->GetLabel());
  results.Append(searchItems);

  searchItems.Clear();
}

bool CGUIWindowVideoBase::OnUnAssignContent(const CStdString &path, int label1, int label2, int label3)
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

void CGUIWindowVideoBase::OnAssignContent(const CStdString &path, int iFound, ADDON::ScraperPtr& info, SScanSettings& settings)
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
