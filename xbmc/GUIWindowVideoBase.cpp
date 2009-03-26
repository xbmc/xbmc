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

#include "stdafx.h"
#include "GUIWindowVideoBase.h"
#include "Util.h"
#include "utils/IMDB.h"
#include "utils/RegExp.h"
#include "utils/GUIInfoManager.h"
#include "GUIWindowVideoInfo.h"
#include "GUIWindowVideoNav.h" 
#include "GUIDialogFileBrowser.h"
#include "GUIDialogVideoScan.h"
#include "GUIDialogSmartPlaylistEditor.h"
#include "GUIDialogProgress.h"
#include "PlayListFactory.h"
#include "Application.h"
#include "NfoFile.h"
#include "Picture.h"
#include "utils/fstrcmp.h"
#include "PlayListPlayer.h"
#include "GUIPassword.h"
#include "FileSystem/ZipManager.h"
#include "FileSystem/StackDirectory.h"
#include "FileSystem/MultiPathDirectory.h"
#include "GUIDialogFileStacking.h"
#include "GUIDialogMediaSource.h"
#include "GUIWindowFileManager.h"
#include "FileSystem/VideoDatabaseDirectory.h"
#include "PartyModeManager.h"
#include "GUIWindowManager.h"
#include "GUIDialogOK.h"
#include "GUIDialogSelect.h"
#include "GUIDialogKeyboard.h"
#include "FileSystem/Directory.h"
#include "PlayList.h"

#include "SkinInfo.h"

using namespace std;
using namespace XFILE;
using namespace DIRECTORY;
using namespace MEDIA_DETECT;
using namespace PLAYLIST;
using namespace VIDEODATABASEDIRECTORY;
using namespace VIDEO;

#define CONTROL_BTNVIEWASICONS     2
#define CONTROL_BTNSORTBY          3
#define CONTROL_BTNSORTASC         4
#define CONTROL_BTNTYPE            5
#define CONTROL_LIST              50
#define CONTROL_THUMBS            51
#define CONTROL_BIGLIST           52
#define CONTROL_LABELFILES        12

#define CONTROL_PLAY_DVD           6
#define CONTROL_STACK              7
#define CONTROL_BTNSCAN            8
#define CONTROL_IMDB               9

CGUIWindowVideoBase::CGUIWindowVideoBase(DWORD dwID, const CStdString &xmlFile)
    : CGUIMediaWindow(dwID, xmlFile)
{
  m_thumbLoader.SetObserver(this);
}

CGUIWindowVideoBase::~CGUIWindowVideoBase()
{
}

bool CGUIWindowVideoBase::OnAction(const CAction &action)
{
  if (action.wID == ACTION_SHOW_PLAYLIST)
  {
    OutputDebugString("activate guiwindowvideoplaylist!\n");
    m_gWindowManager.ActivateWindow(WINDOW_VIDEO_PLAYLIST);
    return true;
  }
  if (action.wID == ACTION_SCAN_ITEM)
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

      m_dlgProgress = (CGUIDialogProgress*)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);

      // save current window, unless the current window is the video playlist window
      if (GetID() != WINDOW_VIDEO_PLAYLIST && (DWORD)g_stSettings.m_iVideoStartWindow != GetID())
      {
        g_stSettings.m_iVideoStartWindow = GetID();
        g_settings.Save();
      }

      return CGUIMediaWindow::OnMessage(message);
    }
    break;

  case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();
      if (iControl == CONTROL_PLAY_DVD)
      {
        // play movie...
        CUtil::PlayDVD();
      }
      else if (iControl == CONTROL_BTNTYPE)
      {
        CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_BTNTYPE);
        m_gWindowManager.SendMessage(msg);

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

        if ((DWORD) nNewWindow != GetID())
        {
          g_stSettings.m_iVideoStartWindow = nNewWindow;
          g_settings.Save();
          m_gWindowManager.ChangeActiveWindow(nNewWindow);
          CGUIMessage msg2(GUI_MSG_SETFOCUS, nNewWindow, CONTROL_BTNTYPE);
          g_graphicsContext.SendMessage(msg2);
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
          SScraperInfo info;
          SScanSettings settings;
          CStdString strDir;
          if (iItem < 0 || iItem >= m_vecItems->Size())
            return false;

          CFileItemPtr item = m_vecItems->Get(iItem);
          if (!m_vecItems->IsPlugin())
          {
          if (item->IsVideoDb()       && 
              item->HasVideoInfoTag() && 
             !item->GetVideoInfoTag()->m_strPath.IsEmpty())
          {
            strDir = item->GetVideoInfoTag()->m_strPath;
          }
          else
            CUtil::GetDirectory(item->m_strPath,strDir);

            int iFound;
            m_database.GetScraperForPath(strDir, info, settings, iFound);

          if (info.strContent.IsEmpty() && 
            !(m_database.HasMovieInfo(item->m_strPath) || 
              m_database.HasTvShowInfo(strDir)                           || 
              m_database.HasEpisodeInfo(item->m_strPath)))
          {
            // hack
            CGUIDialogVideoScan* pDialog = (CGUIDialogVideoScan*)m_gWindowManager.GetWindow(WINDOW_DIALOG_VIDEO_SCAN);
            if (pDialog && pDialog->IsScanning())
              return true;

            CStdString strOldPath = item->m_strPath;
            item->m_strPath = strDir;
            OnAssignContent(iItem,1, info, settings);
            item->m_strPath = strOldPath;
            return true;
          }

          if (info.strContent.Equals("tvshows") && iFound == 1 && !settings.parent_name_root) // dont lookup on root tvshow folder
            return true;
          }
          else
            info.strContent = "plugin";

          OnInfo(item.get(),info);

          return true;
        }
        else if (iAction == ACTION_PLAYER_PLAY && !g_application.IsPlayingVideo())
        {
          OnResumeItem(iItem);
          return true;
        }
        else if (iAction == ACTION_DELETE_ITEM)
        {
          // is delete allowed?
          if (g_settings.m_vecProfiles[g_settings.m_iLastLoadedProfileIndex].canWriteDatabases())
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
      else if (iControl == CONTROL_IMDB)
      {
        OnManualIMDB();
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
  g_graphicsContext.SendMessage(msg);

  // Add labels to the window selection
  CStdString strItem = g_localizeStrings.Get(744); // Files
  CGUIMessage msg2(GUI_MSG_LABEL_ADD, GetID(), CONTROL_BTNTYPE);
  msg2.SetLabel(strItem);
  g_graphicsContext.SendMessage(msg2);

  strItem = g_localizeStrings.Get(14022); // Library
  msg2.SetLabel(strItem);
  g_graphicsContext.SendMessage(msg2);

  // Select the current window as default item
  int nWindow = g_stSettings.m_iVideoStartWindow-WINDOW_VIDEO_FILES;
  CONTROL_SELECT_ITEM(CONTROL_BTNTYPE, nWindow);

  // disable scan and manual imdb controls if internet lookups are disabled
  if (g_guiSettings.GetBool("network.enableinternet"))
  {
    CONTROL_ENABLE(CONTROL_BTNSCAN);
    CONTROL_ENABLE(CONTROL_IMDB);
  }
  else
  {
    CONTROL_DISABLE(CONTROL_BTNSCAN);
    CONTROL_DISABLE(CONTROL_IMDB);
  }

  CGUIMediaWindow::UpdateButtons();
}

void CGUIWindowVideoBase::OnInfo(CFileItem* pItem, const SScraperInfo& info)
{
  if ( !pItem ) return ;
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
  bool modified = ShowIMDB(&item, info);
  if (modified && !info.strContent.Equals("plugin") &&
     (m_gWindowManager.GetActiveWindow() == WINDOW_VIDEO_FILES ||
      m_gWindowManager.GetActiveWindow() == WINDOW_VIDEO_NAV)) // since we can be called from the music library we need this check
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

bool CGUIWindowVideoBase::ShowIMDB(CFileItem *item, const SScraperInfo& info2)
{
  /*
  CLog::Log(LOGDEBUG,"CGUIWindowVideoBase::ShowIMDB");
  CLog::Log(LOGDEBUG,"  strMovie  = [%s]", strMovie.c_str());
  CLog::Log(LOGDEBUG,"  strFile   = [%s]", strFile.c_str());
  CLog::Log(LOGDEBUG,"  strFolder = [%s]", strFolder.c_str());
  CLog::Log(LOGDEBUG,"  bFolder   = [%s]", ((int)bFolder ? "true" : "false"));
  */

  CGUIDialogProgress* pDlgProgress = (CGUIDialogProgress*)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
  CGUIDialogSelect* pDlgSelect = (CGUIDialogSelect*)m_gWindowManager.GetWindow(WINDOW_DIALOG_SELECT);
  CGUIWindowVideoInfo* pDlgInfo = (CGUIWindowVideoInfo*)m_gWindowManager.GetWindow(WINDOW_VIDEO_INFO);

  CVideoInfoScanner scanner;
  scanner.m_IMDB.SetScraperInfo(info2);
  SScraperInfo info(info2); // use this as nfo might change it..

  if (!pDlgProgress) return false;
  if (!pDlgSelect) return false;
  if (!pDlgInfo) return false;
  CUtil::ClearCache();
  CScraperParser::ClearCache();

  // 1.  Check for already downloaded information, and if we have it, display our dialog
  //     Return if no Refresh is needed.
  bool bHasInfo=false;

  CVideoInfoTag movieDetails;
  movieDetails.Reset();
  m_database.Open(); // since we can be called from the music library

  if (info.strContent.Equals("movies"))
  {
    if (m_database.HasMovieInfo(item->m_strPath))
    {
      bHasInfo = true;
      m_database.GetMovieInfo(item->m_strPath, movieDetails);
    }
  }
  if (info.strContent.Equals("tvshows"))
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
      // !! WORKAROUND !!
      // As we cannot add an episode to a non-existing tvshow entry, we have to check the parent directory
      // to see if it`s already in our video database. If it's not yet part of the database we will exit here.
      // (Ticket #4764)
      CStdString strParentDirectory;
      CUtil::GetParentPath(item->m_strPath,strParentDirectory);
      if (m_database.GetTvShowId(strParentDirectory) < 0)
      {
    	CLog::Log(LOGERROR,"%s: could not add episode [%s]. tvshow does not exist yet..", __FUNCTION__, item->m_strPath.c_str());
	return false;
      }

      long lEpisodeHint=-1;
      if (item->HasVideoInfoTag())
        lEpisodeHint = item->GetVideoInfoTag()->m_iEpisode;
      long lEpisodeId=-1;
      if ((lEpisodeId = m_database.GetEpisodeId(item->m_strPath,lEpisodeHint)) > -1)
      {
        bHasInfo = true;
        m_database.GetEpisodeInfo(item->m_strPath, movieDetails, lEpisodeId);
      }
    }
  }
  if (info.strContent.Equals("musicvideos"))
  {
    if (m_database.HasMusicVideoInfo(item->m_strPath))
    {
      bHasInfo = true;
      m_database.GetMusicVideoInfo(item->m_strPath, movieDetails);
    }
  }
  if (info.strContent.Equals("plugin"))
  {
    if (!item->HasVideoInfoTag())
      return false;
    movieDetails = *item->GetVideoInfoTag();

    bHasInfo = true;
  }
  m_database.Close();
  if (bHasInfo)
  {
    if (info.strContent.IsEmpty()) // disable refresh button
      movieDetails.m_strIMDBNumber = "xx"+movieDetails.m_strIMDBNumber;
    if (info.strContent.Equals("plugin")) // disable refresh+get thumb button
      movieDetails.m_strIMDBNumber = "xxplugin";
    *item->GetVideoInfoTag() = movieDetails;
    pDlgInfo->SetMovie(item);
    pDlgInfo->DoModal();
    if ( !pDlgInfo->NeedRefresh() )
      return pDlgInfo->HasUpdatedThumb();
  }

  // quietly return if Internet lookups are disabled
  if (!g_guiSettings.GetBool("network.enableinternet")) return false;
  if (!g_settings.m_vecProfiles[g_settings.m_iLastLoadedProfileIndex].canWriteDatabases() && !g_passwordManager.bMasterUser)
    return false;

  CGUIDialogVideoScan* pDialog = (CGUIDialogVideoScan*)m_gWindowManager.GetWindow(WINDOW_DIALOG_VIDEO_SCAN);
  if (pDialog && pDialog->IsScanning())
  {
    CGUIDialogOK::ShowAndGetInput(13346,14057,-1,-1);
    return false;
  }

  CScraperUrl scrUrl;
  bool hasDetails(false);

  m_database.Open();
  // 2. Look for a nfo File to get the search URL
  SScanSettings settings;
  m_database.GetScraperForPath(item->m_strPath,info,settings);
  CStdString nfoFile;

  if (!info.settings.GetPluginRoot() && info.settings.GetSettings().IsEmpty()) // check for settings, if they are around load defaults - to workaround the nastyness
  {
    CScraperParser parser;
    CStdString strPath;
    if (!info.strContent.IsEmpty())
      strPath = "special://xbmc/system/scrapers/video/"+info.strPath;
    if (!strPath.IsEmpty() && parser.Load(strPath) && parser.HasFunction("GetSettings"))
    {
      info.settings.LoadSettingsXML("special://xbmc/system/scrapers/video/" + info.strPath);
      info.settings.SaveFromDefault();
    }
  }

  CNfoFile::NFOResult result = scanner.CheckForNFOFile(item,settings.parent_name_root,info,scrUrl);
  if (result == CNfoFile::FULL_NFO)
    hasDetails = true;
  if (result == CNfoFile::URL_NFO || result == CNfoFile::COMBINED_NFO)
    scanner.m_IMDB.SetScraperInfo(info);

  // Get the correct movie title to search for
  CStdString movieName = CUtil::GetMovieName(item, settings.parent_name);

  // 3. Run a loop so that if we Refresh we re-run this block
  bool listNeedsUpdating(false);
  bool needsRefresh(false);
  do
  {
    // 4. if we don't have a url, or need to refresh the search
    //    then do the web search
    if (!hasDetails && (scrUrl.m_url.size() == 0 || needsRefresh))
    {
      // 4a. show dialog that we're busy querying www.imdb.com
      CStdString strHeading;
      scanner.m_IMDB.SetScraperInfo(info);
      strHeading.Format(g_localizeStrings.Get(197),info.strTitle.c_str());
      pDlgProgress->SetHeading(strHeading);
      pDlgProgress->SetLine(0, movieName);
      pDlgProgress->SetLine(1, "");
      pDlgProgress->SetLine(2, "");
      pDlgProgress->StartModal();
      pDlgProgress->Progress();

      // 4b. do the websearch
      IMDB_MOVIELIST movielist;
      if (info.strContent.Equals("tvshows") && !item->m_bIsFolder)
        hasDetails = true;

      if (!hasDetails && scanner.m_IMDB.FindMovie(movieName, movielist, pDlgProgress))
      {
        pDlgProgress->Close();
        if (movielist.size() > 0)
        {
          int iString = 196;
          if (info.strContent.Equals("tvshows"))
            iString = 20356;
          pDlgSelect->SetHeading(iString);
          pDlgSelect->Reset();
          for (unsigned int i = 0; i < movielist.size(); ++i)
            pDlgSelect->Add(movielist[i].strTitle);
          pDlgSelect->EnableButton(true);
          pDlgSelect->SetButtonLabel(413); // manual
          pDlgSelect->DoModal();

          // and wait till user selects one
          int iSelectedMovie = pDlgSelect->GetSelectedLabel();
          if (iSelectedMovie >= 0)
            scrUrl = movielist[iSelectedMovie];
          else if (!pDlgSelect->IsButtonPressed())
          {
            m_database.Close();
            return listNeedsUpdating; // user backed out
          }
        }
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
      if (info.strContent.Equals("tvshows"))
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
        CUtil::GetParentPath(strPath,list.m_strPath);
      else
        CUtil::GetDirectory(strPath,list.m_strPath);

      int iString=198;
      if (info.strContent.Equals("tvshows"))
      {
        if (item->m_bIsFolder)
          iString = 20353;
        else
          iString = 20361;
      }
      if (info.strContent.Equals("musicvideos"))
        iString = 20394;
      pDlgProgress->SetHeading(iString);
      pDlgProgress->SetLine(0, movieName);
      pDlgProgress->SetLine(1, scrUrl.strTitle);
      pDlgProgress->SetLine(2, "");
      pDlgProgress->StartModal();
      pDlgProgress->Progress();
      if (bHasInfo)
      {
        if (info.strContent.Equals("movies"))
          m_database.DeleteMovie(item->m_strPath);
        if (info.strContent.Equals("tvshows") && !item->m_bIsFolder)
          m_database.DeleteEpisode(item->m_strPath,movieDetails.m_iDbId);
        if (info.strContent.Equals("musicvideos"))
          m_database.DeleteMusicVideo(item->m_strPath);
        if (info.strContent.Equals("tvshows") && item->m_bIsFolder)
        {
          if (pDlgInfo->RefreshAll())
            m_database.DeleteTvShow(item->m_strPath);
          else
            m_database.DeleteDetailsForTvShow(item->m_strPath);
        }
      }
      if (scanner.RetrieveVideoInfo(list,false,info,!pDlgInfo->RefreshAll(),&scrUrl,pDlgProgress))
      {
        if (info.strContent.Equals("movies"))
          m_database.GetMovieInfo(item->m_strPath,movieDetails);
        if (info.strContent.Equals("musicvideos"))
          m_database.GetMusicVideoInfo(item->m_strPath,movieDetails);
        if (info.strContent.Equals("tvshows"))
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

        // remove directory caches
        CUtil::DeleteVideoDatabaseDirectoryCache();

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
        OutputDebugString("failed to get details\n");
        // show dialog...
        CGUIDialogOK *pDlgOK = (CGUIDialogOK*)m_gWindowManager.GetWindow(WINDOW_DIALOG_OK);
        if (pDlgOK)
        {
          pDlgOK->SetHeading(195);
          pDlgOK->SetLine(0, movieName);
          pDlgOK->SetLine(1, "");
          pDlgOK->SetLine(2, "");
          pDlgOK->SetLine(3, "");
          pDlgOK->DoModal();
        }
        m_database.Close();
        return listNeedsUpdating;
      }
    }
  // 6. Check for a refresh
  } while (needsRefresh);
  m_database.Close();
  return listNeedsUpdating;
}

void CGUIWindowVideoBase::OnManualIMDB()
{
  CStdString strInput;
  if (!CGUIDialogKeyboard::ShowAndGetInput(strInput, g_localizeStrings.Get(16009), false))
    return;

  CFileItem item(strInput);
  item.m_strPath = "Z:\\";
  ::DeleteFile(item.GetCachedVideoThumb().c_str());

  SScraperInfo info;
  info.strContent = "movies";
  info.strPath = "imdb.xml";
  info.strTitle = "IMDb";

  ShowIMDB(&item,info);

  return;
}

bool CGUIWindowVideoBase::IsCorrectDiskInDrive(const CStdString& strFileName, const CStdString& strDVDLabel)
{
  CDetectDVDMedia::WaitMediaReady();
  CCdInfo* pCdInfo = CDetectDVDMedia::GetCdInfo();
  if (pCdInfo == NULL)
    return false;
  if (!CFile::Exists(strFileName))
    return false;
  CStdString label = pCdInfo->GetDiscLabel().TrimRight(" ");
  int iLabelCD = label.GetLength();
  int iLabelDB = strDVDLabel.GetLength();
  if (iLabelDB < iLabelCD)
    return false;
  CStdString dbLabel = strDVDLabel.Left(iLabelCD);

  return (dbLabel == label);
}

bool CGUIWindowVideoBase::CheckMovie(const CStdString& strFileName)
{
  if (!m_database.HasMovieInfo(strFileName))
    return true;

  CVideoInfoTag movieDetails;
  m_database.GetMovieInfo(strFileName, movieDetails);
  CFileItem movieFile(movieDetails.m_strFileNameAndPath, false);
  if (!movieFile.IsOnDVD())
    return true;
  CGUIDialogOK *pDlgOK = (CGUIDialogOK*)m_gWindowManager.GetWindow(WINDOW_DIALOG_OK);
  if (!pDlgOK)
    return true;
  while (1)
  {
//    if (IsCorrectDiskInDrive(strFileName, movieDetails.m_strDVDLabel))
 //   {
      return true;
 //   }
    pDlgOK->SetHeading( 428);
    pDlgOK->SetLine( 0, 429 );
//    pDlgOK->SetLine( 1, movieDetails.m_strDVDLabel );
    pDlgOK->SetLine( 2, "" );
    pDlgOK->DoModal();
    if (!pDlgOK->IsConfirmed())
    {
      break;
    }
  }
  return false;
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

    for (int i = 0; i < items.Size(); ++i)
    {
      if (items[i]->m_bIsFolder)
      {
        CStdString strPath = items[i]->m_strPath;
        if (CUtil::HasSlashAtEnd(strPath))
          strPath.erase(strPath.size()-1);
        strPath.ToLower();
        if (strPath.size() > 6)
        {
          CStdString strSub = strPath.substr(strPath.size()-6);
          if (strPath.Mid(strPath.size()-6).Equals("sample")) // skip sample folders
            continue;
        }
      }
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
  m_database.Open();
  long startoffset = 0;

  if (item->IsStack() && !g_guiSettings.GetBool("myvideos.treatstackasfile") )
  {

    CStdStringArray movies;
    GetStackedFiles(item->m_strPath, movies);

    /* check if any of the stacked files have a resume bookmark */
    for (unsigned i = 0; i<movies.size();i++)
    {
      CBookmark bookmark;
      if (m_database.GetResumeBookMark(movies[i], bookmark))
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
    if (item->IsVideoDb() && item->HasVideoInfoTag())
      strPath = item->GetVideoInfoTag()->m_strFileNameAndPath;

    if (m_database.GetResumeBookMark(strPath, bookmark))
      startoffset = (long)(bookmark.timeInSeconds*75);
  }
  m_database.Close();

  return startoffset;
}

bool CGUIWindowVideoBase::OnClick(int iItem)
{
  CFileItemPtr pItem = m_vecItems->Get(iItem);

  if (!pItem->m_bIsFolder && (unsigned)pItem->m_strPath.Find("playlist://") == CStdString::npos && !pItem->Exists())
  {
    if (!CGUIWindowVideoNav::DeleteItem(pItem.get(),true))
      return true;

    // update list
    m_vecItems->RemoveDiscCache();
    Update(m_vecItems->m_strPath);
    m_viewControl.SetSelectedItem(iItem);
  }
  else
  {
    if (g_guiSettings.GetInt("myvideos.resumeautomatically") != RESUME_NO)
      OnResumeItem(iItem);
    else
      return CGUIMediaWindow::OnClick(iItem);
  }

  return true;
}

void CGUIWindowVideoBase::OnRestartItem(int iItem)
{
  CGUIMediaWindow::OnClick(iItem);
}

void CGUIWindowVideoBase::OnResumeItem(int iItem)
{
  if (iItem < 0 || iItem >= m_vecItems->Size()) return;
  CFileItemPtr item = m_vecItems->Get(iItem);
  // we always resume the movie if the user doesn't want us to ask
  bool resumeItem = g_guiSettings.GetInt("myvideos.resumeautomatically") != RESUME_ASK;
  if (!item->m_bIsFolder && !resumeItem)
  {
    // check to see whether we have a resume offset available
    CVideoDatabase db;
    if (db.Open())
    {
      CBookmark bookmark;
      CStdString itemPath(item->m_strPath);
      if (item->IsVideoDb())
        itemPath = item->GetVideoInfoTag()->m_strFileNameAndPath;
      if (db.GetResumeBookMark(itemPath, bookmark) )
      { // prompt user whether they wish to resume
        vector<CStdString> choices;
        CStdString resumeString, time;
        StringUtils::SecondsToTimeString(lrint(bookmark.timeInSeconds), time);
        resumeString.Format(g_localizeStrings.Get(12022).c_str(), time.c_str());
        choices.push_back(resumeString);
        choices.push_back(g_localizeStrings.Get(12021)); // start from the beginning
        int retVal = CGUIDialogContextMenu::ShowAndGetChoice(choices, GetContextPosition());
        if (!retVal)
          return; // don't do anything
        resumeItem = (retVal == 1);
      }
      db.Close();
    }
  }
  if (resumeItem)
    item->m_lStartOffset = STARTOFFSET_RESUME;
  CGUIMediaWindow::OnClick(iItem);
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
      if (CUtil::IsStack(path))
      {
        vector<long> times;
        if (m_database.GetStackTimes(path,times))
          buttons.Add(CONTEXT_BUTTON_PLAY_PART, 20324);
      }

      if (GetID() != WINDOW_VIDEO_NAV || (!m_vecItems->m_strPath.IsEmpty() &&
         !item->m_strPath.Left(19).Equals("newsmartplaylist://")))
      {
        buttons.Add(CONTEXT_BUTTON_QUEUE_ITEM, 13347);      // Add to Playlist
      }

      // allow a folder to be ad-hoc queued and played by the default player
      if (item->m_bIsFolder || (item->IsPlayList() &&
         !g_advancedSettings.m_playlistAsFolders))
      {
        buttons.Add(CONTEXT_BUTTON_PLAY_ITEM, 208);
      }
      else
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
      if (GetResumeItemOffset(item.get()) > 0)
      {
        if (g_guiSettings.GetInt("myvideos.resumeautomatically") == RESUME_YES)
          buttons.Add(CONTEXT_BUTTON_RESTART_ITEM, 20132);    // Restart Video
        if (g_guiSettings.GetInt("myvideos.resumeautomatically") == RESUME_NO)
          buttons.Add(CONTEXT_BUTTON_RESUME_ITEM, 13381);     // Resume Video
      }
      if (item->IsSmartPlayList() || m_vecItems->IsSmartPlayList())
        buttons.Add(CONTEXT_BUTTON_EDIT_SMART_PLAYLIST, 586);
    }
  }
  CGUIMediaWindow::GetContextButtons(itemNumber, buttons);
}

void CGUIWindowVideoBase::GetNonContextButtons(int itemNumber, CContextButtons &buttons)
{
  if (!m_vecItems->m_strPath.IsEmpty())
    buttons.Add(CONTEXT_BUTTON_GOTO_ROOT, 20128);
  if (g_playlistPlayer.GetPlaylist(PLAYLIST_VIDEO).size() > 0)
    buttons.Add(CONTEXT_BUTTON_NOW_PLAYING, 13350);
  buttons.Add(CONTEXT_BUTTON_SETTINGS, 5);
}

bool CGUIWindowVideoBase::OnContextButton(int itemNumber, CONTEXT_BUTTON button)
{
  CFileItemPtr item;
  if (itemNumber >= 0 && itemNumber < m_vecItems->Size())
    item = m_vecItems->Get(itemNumber);
  switch (button)
  {
  case CONTEXT_BUTTON_PLAY_PART:
    {
      CFileItemList items;
      CStdString path(item->m_strPath);
      if (item->IsVideoDb())
        path = item->GetVideoInfoTag()->m_strFileNameAndPath;

      CDirectory::GetDirectory(path,items);
      CGUIDialogFileStacking* dlg = (CGUIDialogFileStacking*)m_gWindowManager.GetWindow(WINDOW_DIALOG_FILESTACKING);
      if (!dlg) return true;
      dlg->SetNumberOfFiles(items.Size());
      dlg->DoModal();
      int btn2 = dlg->GetSelectedFile();
      if (btn2 > 0)
      {
        if (btn2 > 1)
        {
          vector<long> times;
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
    OnResumeItem(itemNumber);
    return true;

  case CONTEXT_BUTTON_GOTO_ROOT:
    Update("");
    return true;

  case CONTEXT_BUTTON_NOW_PLAYING:
    m_gWindowManager.ActivateWindow(WINDOW_VIDEO_PLAYLIST);
    return true;

  case CONTEXT_BUTTON_SETTINGS:
    m_gWindowManager.ActivateWindow(WINDOW_SETTINGS_MYVIDEOS);
    return true;

  case CONTEXT_BUTTON_INFO:
    {
      SScraperInfo info;
      if (m_vecItems->IsPlugin())
        info.strContent = "plugin";
      else
      {
        VIDEO::SScanSettings settings;
        GetScraperForItem(item.get(), info, settings);
      }

      OnInfo(item.get(),info);
      return true;
    }
  case CONTEXT_BUTTON_STOP_SCANNING:
    {
      CGUIDialogVideoScan *pScanDlg = (CGUIDialogVideoScan *)m_gWindowManager.GetWindow(WINDOW_DIALOG_VIDEO_SCAN);
      if (pScanDlg && pScanDlg->IsScanning())
        pScanDlg->StopScanning();
      return true;
    }
  case CONTEXT_BUTTON_SCAN:
  case CONTEXT_BUTTON_UPDATE_TVSHOW:
    {
      if( !item)
        return false;
      SScraperInfo info;
      SScanSettings settings;
      GetScraperForItem(item.get(), info, settings);
      CStdString strPath = item->m_strPath;
      if (item->IsVideoDb() && (!item->m_bIsFolder || item->GetVideoInfoTag()->m_strPath.IsEmpty()))
        return false;

      if (item->IsVideoDb())
        strPath = item->GetVideoInfoTag()->m_strPath;

      if (info.strContent.IsEmpty())
        return false;

      if (item->m_bIsFolder)
      {
        m_database.SetPathHash(strPath,""); // to force scan
        OnScan(strPath,info,settings);
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
        m_vecItems->RemoveDiscCache();
        Update(m_vecItems->m_strPath);
      }
      return true;
    }
  case CONTEXT_BUTTON_RENAME:
    OnRenameItem(itemNumber);
    return true;
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

  if (pItem->m_strPath == "add" && pItem->GetLabel() == g_localizeStrings.Get(1026)) // 'add source button' in empty root
  {
    if (CGUIDialogMediaSource::ShowAndAddMediaSource("video"))
    {
      Update("");
      return true;
    }
    return false;
  }

  CFileItem item(*pItem);
  if (pItem->IsVideoDb())
  {
    item = CFileItem(*pItem->GetVideoInfoTag());
    item.m_lStartOffset = pItem->m_lStartOffset;
  }

  PlayMovie(&item);

  return true;
}

void CGUIWindowVideoBase::PlayMovie(const CFileItem *item)
{
  CFileItemList movieList;
  int selectedFile = 1;
  long startoffset = item->m_lStartOffset;

  if (item->IsStack() && !g_guiSettings.GetBool("myvideos.treatstackasfile"))
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
        vector<long> times;
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
      CGUIDialogFileStacking* dlg = (CGUIDialogFileStacking*)m_gWindowManager.GetWindow(WINDOW_DIALOG_FILESTACKING);
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

  CFileItemPtr item = m_vecItems->Get(iItem);
  // HACK: stacked files need to be treated as folders in order to be deleted
  if (item->IsStack())
    item->m_bIsFolder = true;
  if (g_settings.m_vecProfiles[g_settings.m_iLastLoadedProfileIndex].getLockMode() != LOCK_MODE_EVERYONE &&
      g_settings.m_vecProfiles[g_settings.m_iLastLoadedProfileIndex].filesLocked())
  {
    if (!g_passwordManager.IsMasterLockUnlocked(true))
      return;
  }

  if (!CGUIWindowFileManager::DeleteItem(item.get()))
    return;

  Update(m_vecItems->m_strPath);
  m_viewControl.SetSelectedItem(iItem);
}

void CGUIWindowVideoBase::MarkUnWatched(const CFileItemPtr &item)
{
  // dont allow update while scanning
  CGUIDialogVideoScan* pDialogScan = (CGUIDialogVideoScan*)m_gWindowManager.GetWindow(WINDOW_DIALOG_VIDEO_SCAN);
  if (pDialogScan && pDialogScan->IsScanning())
  {
    CGUIDialogOK::ShowAndGetInput(257, 0, 14057, 0);
    return;
  }

  CVideoDatabase database;
  database.Open();
  CFileItemList items;
  if (item->m_bIsFolder)
  {
    CVideoDatabaseDirectory dir;
    CStdString strPath = item->m_strPath;
    if (dir.GetDirectoryChildType(item->m_strPath) == NODE_TYPE_SEASONS)
      strPath += "-1/";
    dir.GetDirectory(strPath,items);
  }
  else
    items.Add(item);

  for (int i=0;i<items.Size();++i)
  {
    CFileItemPtr pItem=items[i];
    if (pItem->HasVideoInfoTag() && pItem->GetVideoInfoTag()->m_playCount == 0)
      continue;

    database.MarkAsUnWatched(*pItem);
  }
}

//Add Mark a Title as watched
void CGUIWindowVideoBase::MarkWatched(const CFileItemPtr &item)
{
  // dont allow update while scanning
  CGUIDialogVideoScan* pDialogScan = (CGUIDialogVideoScan*)m_gWindowManager.GetWindow(WINDOW_DIALOG_VIDEO_SCAN);
  if (pDialogScan && pDialogScan->IsScanning())
  {
    CGUIDialogOK::ShowAndGetInput(257, 0, 14057, 0);
    return;
  }

  CVideoDatabase database;
  database.Open();
  CFileItemList items;
  if (item->m_bIsFolder)
  {
    CVideoDatabaseDirectory dir;
    CStdString strPath = item->m_strPath;
    if (dir.GetDirectoryChildType(item->m_strPath) == NODE_TYPE_SEASONS)
      strPath += "-1/";
    dir.GetDirectory(strPath,items);
  }
  else
    items.Add(item);

  for (int i=0;i<items.Size();++i)
  {
    CFileItemPtr pItem=items[i];
    if (pItem->HasVideoInfoTag() && pItem->GetVideoInfoTag()->m_playCount > 0)
      continue;

    database.MarkAsWatched(*pItem);
  }
}

//Add change a title's name
void CGUIWindowVideoBase::UpdateVideoTitle(const CFileItem* pItem)
{
  // dont allow update while scanning
  CGUIDialogVideoScan* pDialogScan = (CGUIDialogVideoScan*)m_gWindowManager.GetWindow(WINDOW_DIALOG_VIDEO_SCAN);
  if (pDialogScan && pDialogScan->IsScanning())
  {
    CGUIDialogOK::ShowAndGetInput(257, 0, 14057, 0);
    return;
  }

  CVideoInfoTag detail;
  CVideoDatabase database;
  database.Open();

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

  if (iType == VIDEODB_CONTENT_MOVIES)
    database.GetMovieInfo("", detail, pItem->GetVideoInfoTag()->m_iDbId);
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

  database.UpdateMovieTitle(pItem->GetVideoInfoTag()->m_iDbId, strInput, iType);
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
  if (pItem->m_bIsFolder)
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
  if (items.m_strPath == "special://videoplaylists/" && !items.Contains("newplaylist://"))
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

  return bResult;
}

void CGUIWindowVideoBase::OnPrepareFileItems(CFileItemList &items)
{
  if (!items.m_strPath.Equals("plugin://video/"))
    items.SetCachedVideoThumbs();
}

void CGUIWindowVideoBase::AddToDatabase(int iItem)
{
  if (iItem < 0 || iItem >= m_vecItems->Size())
    return;

  CFileItemPtr pItem = m_vecItems->Get(iItem);
  if (pItem->IsParentFolder() || pItem->m_bIsFolder)
    return;

  bool bGotXml = false;
  CVideoInfoTag movie;
  movie.Reset();

  // look for matching xml file first
  CStdString strXml = pItem->m_strPath + ".xml";
  if (pItem->IsStack())
  {
    // for a stack, use the first file in the stack
    CStackDirectory stack;
    strXml = stack.GetFirstStackedFile(pItem->m_strPath) + ".xml";
  }
  CStdString strCache = CUtil::AddFileToFolder("special://temp/", CUtil::MakeLegalFileName(CUtil::GetFileName(strXml), true));
  if (CFile::Exists(strXml))
  {
    bGotXml = true;
    CLog::Log(LOGDEBUG,"%s: found matching xml file:[%s]", __FUNCTION__, strXml.c_str());
    CFile::Cache(strXml, strCache);
    CIMDB imdb;
    if (!imdb.LoadXML(strCache, movie, false))
    {
      CLog::Log(LOGERROR,"%s: Could not parse info in file:[%s]", __FUNCTION__, strXml.c_str());
      bGotXml = false;
    }
  }

  // prompt for data
  if (!bGotXml)
  {
    // enter a new title
    CStdString strTitle = pItem->GetLabel();
    if (!CGUIDialogKeyboard::ShowAndGetInput(strTitle, g_localizeStrings.Get(528), false)) // Enter Title
      return;

    // pick genre
    CGUIDialogSelect* pSelect = (CGUIDialogSelect*)m_gWindowManager.GetWindow(WINDOW_DIALOG_SELECT);
    if (!pSelect)
      return;

    pSelect->SetHeading(530); // Select Genre
    pSelect->Reset();
    CFileItemList items;
    if (!CDirectory::GetDirectory("videodb://1/1/", items))
      return;
    pSelect->SetItems(&items);
    pSelect->EnableButton(true);
    pSelect->SetButtonLabel(531); // New Genre
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
  }

  // everything is ok, so add to database
  m_database.Open();
  long lMovieId = m_database.AddMovie(pItem->m_strPath);
  movie.m_strIMDBNumber.Format("xx%08i", lMovieId);
  m_database.SetDetailsForMovie(pItem->m_strPath, movie);
  m_database.Close();

  // done...
  CGUIDialogOK *pDialog = (CGUIDialogOK*)m_gWindowManager.GetWindow(WINDOW_DIALOG_OK);
  if (pDialog)
  {
    pDialog->SetHeading(20177); // Done
    pDialog->SetLine(0, movie.m_strTitle);
    pDialog->SetLine(1, movie.m_strGenre);
    pDialog->SetLine(2, movie.m_strIMDBNumber);
    pDialog->SetLine(3, "");
    pDialog->DoModal();
  }

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

  if (items.Size())
  {
    CGUIDialogSelect* pDlgSelect = (CGUIDialogSelect*)m_gWindowManager.GetWindow(WINDOW_DIALOG_SELECT);
    pDlgSelect->Reset();
    pDlgSelect->SetHeading(283);
    items.Sort(SORT_METHOD_LABEL, SORT_ORDER_ASC);

    for (int i = 0; i < (int)items.Size(); i++)
    {
      CFileItemPtr pItem = items[i];
      pDlgSelect->Add(pItem->GetLabel());
    }

    pDlgSelect->DoModal();

    int iItem = pDlgSelect->GetSelectedLabel();
    if (iItem < 0)
    {
      if (m_dlgProgress)
        m_dlgProgress->Close();

      return ;
    }

    CFileItemPtr pSelItem = items[iItem];

    OnSearchItemFound(pSelItem.get());

    if (m_dlgProgress)
      m_dlgProgress->Close();
  }
  else
  {
    if (m_dlgProgress)
      m_dlgProgress->Close();

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
    CUtil::GetParentPath(strPath, strParentPath);

    Update(strParentPath);

    SetHistoryForPath(strParentPath);

    strPath = pSelItem->m_strPath;
    CURL url(strPath);
    if (pSelItem->IsSmb() && !CUtil::HasSlashAtEnd(strPath))
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
    CUtil::GetDirectory(pSelItem->m_strPath, strPath);

    Update(strPath);

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

int CGUIWindowVideoBase::GetScraperForItem(CFileItem *item, SScraperInfo &info, SScanSettings& settings)
{
  if (!item)
    return 0;

  int found = 0;
  if (item->HasVideoInfoTag())  // files view shouldn't need this check I think?
    m_database.GetScraperForPath(item->GetVideoInfoTag()->m_strPath,info,settings,found);
  else
    m_database.GetScraperForPath(item->m_strPath,info,settings,found);

  return found;
}

void CGUIWindowVideoBase::OnScan(const CStdString& strPath, const SScraperInfo& info, const SScanSettings& settings)
{
  CGUIDialogVideoScan* pDialog = (CGUIDialogVideoScan*)m_gWindowManager.GetWindow(WINDOW_DIALOG_VIDEO_SCAN);
  if (pDialog)
    pDialog->StartScanning(strPath,info,settings,false);
}


