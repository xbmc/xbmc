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
#include "GUIWindowVideoBase.h"
#include "Util.h"
#include "Utils/IMDB.h"
#include "Utils/HTTP.h"
#include "Utils/RegExp.h"
#include "GUIWindowVideoInfo.h"
#include "PlayListFactory.h"
#include "Application.h"
#include "NFOFile.h"
#include "Picture.h"
#include "utils/fstrcmp.h"
#include "PlayListPlayer.h"
#ifdef PRE_SKIN_VERSION_2_1_COMPATIBILITY
#include "GUIFontManager.h"
#endif
#include "GUIPassword.h"
#include "FileSystem/ZipManager.h"
#include "FileSystem/StackDirectory.h"
#include "GUIDialogContextMenu.h"
#include "GUIDialogFileStacking.h"
#include "GUIDialogMediaSource.h"
#include "GUIWindowFileManager.h"
#include "FileSystem/VideoDatabaseDirectory.h"

#include "SkinInfo.h"

using namespace XFILE;
using namespace DIRECTORY;
using namespace PLAYLIST;
using namespace VIDEODATABASEDIRECTORY;

#define CONTROL_BTNVIEWASICONS     2
#define CONTROL_BTNSORTBY          3
#define CONTROL_BTNSORTASC         4
#define CONTROL_BTNTYPE            5
#define CONTROL_LIST              50
#define CONTROL_THUMBS            51
#define CONTROL_BIGLIST           52
#define CONTROL_LABELFILES        12
#define CONTROL_LABELEMPTY        13

#define CONTROL_PLAY_DVD          6
#define CONTROL_STACK             7
#define CONTROL_BTNSCAN           8
#define CONTROL_IMDB              9
#define CONTROL_BTNSHOWMODE       10
#define CONTROL_BTNSHOWALL        14

CGUIWindowVideoBase::CGUIWindowVideoBase(DWORD dwID, const CStdString &xmlFile)
    : CGUIMediaWindow(dwID, xmlFile)
{
  m_bDisplayEmptyDatabaseMessage = false;
  m_thumbLoader.SetObserver(this);
}

CGUIWindowVideoBase::~CGUIWindowVideoBase()
{
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
      if (GetID() != WINDOW_VIDEO_PLAYLIST && g_stSettings.m_iVideoStartWindow != GetID())
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

        if (nNewWindow != GetID())
        {
          g_stSettings.m_iVideoStartWindow = nNewWindow;
          g_settings.Save();
          m_gWindowManager.ChangeActiveWindow(nNewWindow);
          CGUIMessage msg2(GUI_MSG_SETFOCUS, nNewWindow, CONTROL_BTNTYPE);
          g_graphicsContext.SendMessage(msg2);
        }

        return true;
      }
      else if (iControl == CONTROL_BTNSHOWMODE)
      {
        g_stSettings.m_iMyVideoWatchMode++;
        if (g_stSettings.m_iMyVideoWatchMode > VIDEO_SHOW_WATCHED)
          g_stSettings.m_iMyVideoWatchMode = VIDEO_SHOW_ALL;
        g_settings.Save();
        Update(m_vecItems.m_strPath);
        return true;
      }
      else if (iControl == CONTROL_BTNSHOWALL)
      {
        if (g_stSettings.m_iMyVideoWatchMode == VIDEO_SHOW_ALL)
          g_stSettings.m_iMyVideoWatchMode = VIDEO_SHOW_UNWATCHED;
        else
          g_stSettings.m_iMyVideoWatchMode = VIDEO_SHOW_ALL;
        g_settings.Save();
        Update(m_vecItems.m_strPath);
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
          CStdString strDir;
          CUtil::GetDirectory(m_vecItems[iItem]->m_strPath,strDir);
          m_database.GetScraperForPath(strDir,info.strPath,info.strContent);
          CScraperParser parser;
          if (parser.Load("q:\\system\\scrapers\\video\\"+info.strPath))
            info.strTitle = parser.GetName();

          strDir = m_vecItems[iItem]->m_strPath;
          if (m_vecItems[iItem]->m_bIsFolder)
            CUtil::AddSlashAtEnd(strDir);
          if (!info.strContent.IsEmpty() || m_database.HasMovieInfo(strDir))
            OnInfo(iItem,info);
          
          return true;
        }
        else if (iAction == ACTION_CONTEXT_MENU || iAction == ACTION_MOUSE_RIGHT_CLICK)
        {
          OnPopupMenu(iItem);
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
          // must be at the title window
          if (GetID() == WINDOW_VIDEO_NAV)
            OnDeleteItem(iItem);

          // or be at the files window and have file deletion enabled
          else if (GetID() == WINDOW_VIDEO_FILES && g_guiSettings.GetBool("filelists.allowfiledeletion"))
            OnDeleteItem(iItem);

          // or be at the video playlists location
          else if (m_vecItems.m_strPath.Equals("special://videoplaylists/"))
            OnDeleteItem(iItem);
          else
            return false;
          
          return true;
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

  SET_CONTROL_LABEL(CONTROL_BTNSHOWMODE, g_localizeStrings.Get(16100 + g_stSettings.m_iMyVideoWatchMode));

  SET_CONTROL_SELECTED(GetID(),CONTROL_BTNSHOWALL,g_stSettings.m_iMyVideoWatchMode != VIDEO_SHOW_ALL);

  CGUIMediaWindow::UpdateButtons();
}

void CGUIWindowVideoBase::OnInfo(int iItem, const SScraperInfo& info)
{
  if ( iItem < 0 || iItem >= m_vecItems.Size() ) return ;
  CFileItem* pItem = m_vecItems[iItem];
  // ShowIMDB can kill the item as this window can be closed while we do it,
  // so take a copy of the item now
  CFileItem item(*pItem);
  if (item.IsVideoDb())
  {
    if (item.GetVideoInfoTag()->m_strFileNameAndPath.IsEmpty())
      item.m_strPath = item.GetVideoInfoTag()->m_strPath;
    else
      item.m_strPath = item.GetVideoInfoTag()->m_strFileNameAndPath;
  }
  ShowIMDB(&item, info);
  Update(m_vecItems.m_strPath);
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

void CGUIWindowVideoBase::ShowIMDB(CFileItem *item, const SScraperInfo& info)
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

  CIMDB IMDB;
  IMDB.SetScraperInfo(info);

  bool bUpdate = false;
  bool bFound = false;

  if (!pDlgProgress) return ;
  if (!pDlgSelect) return ;
  if (!pDlgInfo) return ;
  CUtil::ClearCache();

  // 1.  Check for already downloaded information, and if we have it, display our dialog
  //     Return if no Refresh is needed.
  bool bHasInfo=false;

  CVideoInfoTag movieDetails;
  movieDetails.Reset();
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
      if (m_database.HasEpisodeInfo(item->m_strPath))
      {
        bHasInfo = true;
        m_database.GetEpisodeInfo(item->m_strPath, movieDetails);
      }
    }
  }
  if (bHasInfo)
  {
    if (info.strContent.IsEmpty()) // disable refresh button
      movieDetails.m_strIMDBNumber = "xx"+movieDetails.m_strIMDBNumber;
    pDlgInfo->SetMovie(movieDetails, item);
    pDlgInfo->DoModal();
    item->SetThumbnailImage(pDlgInfo->GetThumbnail());
    if ( !pDlgInfo->NeedRefresh() ) return ;
  }
  
  // quietly return if Internet lookups are disabled
  if (!g_guiSettings.GetBool("network.enableinternet")) return ;
  if (!g_settings.m_vecProfiles[g_settings.m_iLastLoadedProfileIndex].canWriteDatabases() && !g_passwordManager.bMasterUser)
    return;

  CIMDBUrl url;
  bool hasDetails(false);

  // 2. Look for a nfo File to get the search URL
  CStdString nfoFile = GetnfoFile(item);
  if ( !nfoFile.IsEmpty() )
  {
    CLog::Log(LOGDEBUG,"Found matching nfo file: %s", nfoFile.c_str());
    CNfoFile nfoReader(info.strContent);
    if ( nfoReader.Create(nfoFile) == S_OK)
    {
      if (nfoReader.m_strScraper == "NFO")
      {
        CLog::Log(LOGDEBUG, __FUNCTION__" Got details from nfo");
        nfoReader.GetDetails(movieDetails);
        hasDetails = true;
      }
      else
      {
        CScraperUrl scrUrl(nfoReader.m_strImDbUrl); 
	      url.m_scrURL.push_back(scrUrl);
        url.m_strID = nfoReader.m_strImDbNr;
        SScraperInfo info2(info);
        info2.strPath = nfoReader.m_strScraper;
        IMDB.SetScraperInfo(info2);
        CLog::Log(LOGDEBUG,"-- nfo scraper: %s", nfoReader.m_strScraper.c_str());
        CLog::Log(LOGDEBUG,"-- nfo url: %s", url.m_scrURL[0].m_url.c_str());
      }
    }
    else
      CLog::Log(LOGERROR,"Unable to find an imdb url in nfo file: %s", nfoFile.c_str());
  }

  CStdString movieName = item->GetLabel();
  // 3. Run a loop so that if we Refresh we re-run this block
  bool needsRefresh(false);
  do
  {
    // 4. if we don't have a url, or need to refresh the search
    //    then do the web search
    if (!hasDetails && (url.m_scrURL.size() == 0 || needsRefresh))
    {
      // 4a. show dialog that we're busy querying www.imdb.com
      CStdString strHeading;
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

      if (!hasDetails && IMDB.FindMovie(movieName, movielist, pDlgProgress))
      {
        pDlgProgress->Close();
        if (movielist.size() > 0)
        {
          // 4c. found movies - allow selection of the movie we found if several
//          if (movielist.size() == 1)
//          {
//            url = movielist[0];
//          }
//          else
          {
            int iString = 196;
            if (info.strContent.Equals("tvshows"))
              iString = 20356;
            pDlgSelect->SetHeading(iString);
            pDlgSelect->Reset();
            for (unsigned int i = 0; i < movielist.size(); ++i)
              pDlgSelect->Add(movielist[i].m_strTitle);
            pDlgSelect->EnableButton(true);
            pDlgSelect->SetButtonLabel(413); // manual
            pDlgSelect->DoModal();

            // and wait till user selects one
            int iSelectedMovie = pDlgSelect->GetSelectedLabel();
            if (iSelectedMovie >= 0)
              url = movielist[iSelectedMovie];
            else if (!pDlgSelect->IsButtonPressed())
            {
              return; // user backed out
            }
          }
        }
      }
    }
    // 4c. Check if url is still empty - occurs if user has selected to do a manual
    //     lookup, or if the IMDb lookup failed or was cancelled.
    if (!hasDetails && url.m_scrURL.size() == 0)
    {
      // Check for cancel of the progress dialog
      pDlgProgress->Close();
      if (pDlgProgress->IsCanceled())
      {
        return;
      }

      // Prompt the user to input the movieName
      int iString = 16009;
      if (info.strContent.Equals("tvshows"))
        iString = 20357;
      if (!CGUIDialogKeyboard::ShowAndGetInput(movieName, g_localizeStrings.Get(iString), false))
      {
        return; // user backed out
      }

      needsRefresh = true;
    }
    else
    {
      // 5. Download the movie information
      // show dialog that we're downloading the movie info
      int iString=198;
      if (info.strContent.Equals("tvshows"))
      {
        if (item->m_bIsFolder)
          iString = 20353;
        else
          iString = 20361;
      }

      pDlgProgress->SetHeading(iString);
      pDlgProgress->SetLine(0, movieName);
      pDlgProgress->SetLine(1, url.m_strTitle);
      pDlgProgress->SetLine(2, "");
      pDlgProgress->StartModal();
      pDlgProgress->Progress();

      // get the movie info
      if (hasDetails || IMDB.GetDetails(url, movieDetails, pDlgProgress))
      {
        if (info.strContent.Equals("tvshows"))
        {
          pDlgProgress->SetLine(2, 20354);
          pDlgProgress->Progress();
          IMDB_EPISODELIST episodes;
          CIMDBUrl url;
          long lShowId=-1;
          if (!item->m_bIsFolder) // fetch tvshow info from database
          {
            CStdString path;
            CUtil::GetDirectory(item->m_strPath,path);
            lShowId = m_database.GetTvShowInfo(path);
            m_database.GetTvShowInfo(path,movieDetails,lShowId);
            pDlgProgress->SetLine(1, movieDetails.m_strTitle);            
            pDlgProgress->Progress();
          }
          else
          {
            lShowId = m_database.GetTvShowInfo(item->m_strPath);
            if (lShowId > -1)
            {
              CVideoInfoTag movieDetails2;
              m_database.GetTvShowInfo(item->m_strPath,movieDetails2,lShowId);
              movieDetails.m_iEpisode = movieDetails2.m_iEpisode; // keep # of episodes
              m_database.DeleteDetailsForTvShow(item->m_strPath);
            }
            lShowId = m_database.SetDetailsForTvShow(item->m_strPath,movieDetails);
          }
          
          url.Parse(movieDetails.m_strEpisodeGuide);
          if (IMDB.GetEpisodeList(url,episodes))
          {
            if (!item->m_bIsFolder)
              m_database.DeleteEpisode(item->m_strPath);
            OnProcessSeriesFolder(episodes,item,lShowId,IMDB,pDlgProgress);
            if (!item->m_bIsFolder)
            {
              if (!m_database.GetEpisodeInfo(item->m_strPath,movieDetails))
              {        
                pDlgProgress->Close();
                return;
              }
            }
          }
          else
          {
            pDlgProgress->Close();
            return;
          }
        }
        // got all movie details :-)
        OutputDebugString("got details\n");
        pDlgProgress->Close();

        // now show the imdb info
        OutputDebugString("show info\n");

        // Add to the database if applicable
        if (info.strContent.Equals("movies") && item->m_strPath && (g_settings.m_vecProfiles[g_settings.m_iLastLoadedProfileIndex].canWriteDatabases() || g_passwordManager.bMasterUser))
        {
          m_database.DeleteDetailsForMovie(item->m_strPath);
          m_database.SetDetailsForMovie(item->m_strPath, movieDetails);
        }

        // remove directory caches
        CUtil::DeleteVideoDatabaseDirectoryCache();

        pDlgInfo->SetMovie(movieDetails, item);
        pDlgInfo->DoModal();
        item->SetThumbnailImage(pDlgInfo->GetThumbnail());
        needsRefresh = pDlgInfo->NeedRefresh();
      }
      else
      {
        pDlgProgress->Close();
        if (pDlgProgress->IsCanceled())
        {
          return; // user cancelled
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
        return;
      }
    }
    // 6. Check for a refresh
  } while (needsRefresh);
}

void CGUIWindowVideoBase::Render()
{
  if (m_bDisplayEmptyDatabaseMessage)
  {
    SET_CONTROL_LABEL(CONTROL_LABELEMPTY,g_localizeStrings.Get(745)+'\n'+g_localizeStrings.Get(746))
  }
  else
  {
    SET_CONTROL_LABEL(CONTROL_LABELEMPTY,"")
  }

  CGUIMediaWindow::Render();
}

void CGUIWindowVideoBase::OnManualIMDB()
{
  CStdString strInput;
  if (!CGUIDialogKeyboard::ShowAndGetInput(strInput, g_localizeStrings.Get(16009), false)) return ;

  CFileItem item(strInput);
  item.m_strPath = "Z:\\";
  ::DeleteFile(item.GetCachedVideoThumb().c_str());

  SScraperInfo info;
  info.strContent = "movies";
  info.strPath = "imdb.xml";
  info.strTitle = "IMDb";

  ShowIMDB(&item,info);
  
  return ;
}

bool CGUIWindowVideoBase::IsCorrectDiskInDrive(const CStdString& strFileName, const CStdString& strDVDLabel)
{
  CDetectDVDMedia::WaitMediaReady();
  CCdInfo* pCdInfo = CDetectDVDMedia::GetCdInfo();
  if (pCdInfo == NULL) return false;
  if (!CFile::Exists(strFileName)) return false;
  CStdString label = pCdInfo->GetDiscLabel().TrimRight(" ");
  int iLabelCD = label.GetLength();
  int iLabelDB = strDVDLabel.GetLength();
  if (iLabelDB < iLabelCD) return false;
  CStdString dbLabel = strDVDLabel.Left(iLabelCD);
  return (dbLabel == label);
}

bool CGUIWindowVideoBase::CheckMovie(const CStdString& strFileName)
{
  if (!m_database.HasMovieInfo(strFileName) ) return true;

  CVideoInfoTag movieDetails;
  m_database.GetMovieInfo(strFileName, movieDetails);
  CFileItem movieFile(movieDetails.m_strFileNameAndPath, false);
  if ( !movieFile.IsOnDVD()) return true;
  CGUIDialogOK *pDlgOK = (CGUIDialogOK*)m_gWindowManager.GetWindow(WINDOW_DIALOG_OK);
  if (!pDlgOK) return true;
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
  if ( iItem < 0 || iItem >= m_vecItems.Size() ) return ;

  int iOldSize=g_playlistPlayer.GetPlaylist(PLAYLIST_VIDEO).size();

  CFileItem item(*m_vecItems[iItem]);
  if (item.IsRAR() || item.IsZIP())
    return;

  //  Allow queuing of unqueueable items
  //  when we try to queue them directly
  if (!item.CanQueue())
    item.SetCanQueue(true);

  CFileItemList queuedItems;
  AddItemToPlayList(&item, queuedItems);
  g_playlistPlayer.Add(PLAYLIST_VIDEO, queuedItems);
  // video does not auto play on queue like music
  m_viewControl.SetSelectedItem(iItem + 1);
}

void CGUIWindowVideoBase::AddItemToPlayList(const CFileItem* pItem, CFileItemList &queuedItems)
{
  if (!pItem->CanQueue() || pItem->IsRAR() || pItem->IsZIP() || pItem->IsParentFolder()) // no zip/rar enques thank you!
    return;

  if (pItem->m_bIsFolder)
  {
    if (pItem->IsParentFolder()) return;

    // Check if we add a locked share
    if ( pItem->m_bIsShareOrDrive )
    {
      CFileItem item = *pItem;
      if ( !g_passwordManager.IsItemUnlocked( &item, "video" ) )
        return ;
    }

    // recursive
    CFileItemList items;
    GetDirectory(pItem->m_strPath, items);
    SortItems(items);

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
      if ( NULL != pPlayList.get())
      {
        // load it
        if (!pPlayList->Load(pItem->m_strPath))
        {
          CGUIDialogOK::ShowAndGetInput(6, 0, 477, 0);
          return; //hmmm unable to load playlist?
        }

        CPlayList playlist = *pPlayList;
        for (int i = 0; i < (int)playlist.size(); ++i)
          AddItemToPlayList(&playlist[i], queuedItems);
        return;
      }
    }
    else if(pItem->IsInternetStream())
    { // just queue the internet stream, it will be expanded on play
      queuedItems.Add(new CFileItem(*pItem));
    }
    else if (!pItem->IsNFO() && pItem->IsVideo())
    {
      queuedItems.Add(new CFileItem(*pItem));
    }

  }
}

void CGUIWindowVideoBase::DisplayEmptyDatabaseMessage(bool bDisplay)
{
  m_bDisplayEmptyDatabaseMessage = bDisplay;
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
    for(unsigned i = 0; i<movies.size();i++)
    {
      CBookmark bookmark;
      if(m_database.GetResumeBookMark(movies[i], bookmark))
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
    if(m_database.GetResumeBookMark(item->m_strPath, bookmark))
      startoffset = (long)(bookmark.timeInSeconds*75);
  }
  m_database.Close();
  return startoffset;
}

bool CGUIWindowVideoBase::OnClick(int iItem)
{
  if (g_guiSettings.GetBool("myvideos.autoresume"))
    OnResumeItem(iItem);
  else
    return CGUIMediaWindow::OnClick(iItem);

  return true;
}

void CGUIWindowVideoBase::OnRestartItem(int iItem)
{
  CGUIMediaWindow::OnClick(iItem);
}

void CGUIWindowVideoBase::OnResumeItem(int iItem)
{
  m_vecItems[iItem]->m_lStartOffset = STARTOFFSET_RESUME;
  CGUIMediaWindow::OnClick(iItem);
}

void CGUIWindowVideoBase::OnPopupMenu(int iItem, bool bContextDriven /* = true */)
{
  // empty list in files view?
  if (GetID() == WINDOW_VIDEO_FILES && m_vecItems.Size() == 0)
    bContextDriven = false;
  if (bContextDriven && (iItem < 0 || iItem >= m_vecItems.Size())) return;

  // calculate our position
  float posX = 200, posY = 100;
  const CGUIControl *pList = GetControl(CONTROL_LIST);
  if (pList)
  {
    posX = pList->GetXPosition() + pList->GetWidth() / 2;
    posY = pList->GetYPosition() + pList->GetHeight() / 2;
  }

  // popup the context menu
  CGUIDialogContextMenu *pMenu = (CGUIDialogContextMenu *)m_gWindowManager.GetWindow(WINDOW_DIALOG_CONTEXT_MENU);
  if (!pMenu) return ;

  // load our menu
  pMenu->Initialize();

  // contextual buttons
  int btn_PlayPart = 0;       // For stacks
  int btn_Queue = 0;					// Add to Playlist
  int btn_PlayWith = 0;				// Play
  int btn_Restart = 0;				// Restart Video from Beginning
  int btn_Resume = 0;					// Resume Video
  int btn_Show_Info = 0;			// Show Video Information
  int btn_AddToDatabase = 0;  // Manual add to Database
  int btn_Assign = 0;         // Assign content to directory
  int btn_Update = 0;         // Update content information
  int btn_Mark_UnWatched = 0;	// Clear Watched Status (DB)
  int btn_Mark_Watched = 0;		// Set Watched Status (DB)
  int btn_Update_Title = 0;		// Change Title (DB)
  int btn_Delete = 0;					// Delete
  int btn_Rename = 0;					// Rename

  bool bSelected = false;
  VECPLAYERCORES vecCores;
  int iFound = 0;
  SScraperInfo info;

  // contextual items only appear when the list is not empty
  if (bContextDriven)
  {
    // mark the item
    bSelected = m_vecItems[iItem]->IsSelected(); // item may already be selected (playlistitem)
    m_vecItems[iItem]->Select(true);

    // get players
    CPlayerCoreFactory::GetPlayers(*m_vecItems[iItem], vecCores);

    if (m_vecItems[iItem]->HasVideoInfoTag())
    {
      m_database.GetScraperForPath(m_vecItems[iItem]->GetVideoInfoTag()->m_strPath,info.strPath,info.strContent,iFound);
    }
    else
      m_database.GetScraperForPath(m_vecItems[iItem]->m_strPath,info.strPath,info.strContent,iFound);
    
    int iString = 13346;
    if (info.strContent.Equals("tvshows"))
    {
      if (m_vecItems[iItem]->m_bIsFolder)
        iString = 20351;
      else 
        iString = 20352;
    }

    bool bIsGotoParent = m_vecItems[iItem]->IsParentFolder();
    if (!bIsGotoParent)
    {
      // don't show the add to playlist button in playlist window
      if (GetID() != WINDOW_VIDEO_PLAYLIST)
      {
        if (m_vecItems[iItem]->IsStack())
        {
          vector<long> times;
          if (m_database.GetStackTimes(m_vecItems[iItem]->m_strPath,times))
            btn_PlayPart = pMenu->AddButton(20324);
        }
        if (GetID() == WINDOW_VIDEO_NAV)
        {
          if (!m_vecItems.m_strPath.IsEmpty())
            btn_Queue = pMenu->AddButton(13347);      // Add to Playlist
        }
        else
          btn_Queue = pMenu->AddButton(13347);      // Add to Playlist

        if (vecCores.size() >= 1)
          btn_PlayWith = pMenu->AddButton(15213);
        // allow a folder to be ad-hoc queued and played by the default player
        else if (GetID() == WINDOW_VIDEO_FILES && (m_vecItems[iItem]->m_bIsFolder || m_vecItems[iItem]->IsPlayList()))
          btn_PlayWith = pMenu->AddButton(208);

        // if autoresume is enabled then add restart video button
        // check to see if the Resume Video button is applicable
        if (GetResumeItemOffset(m_vecItems[iItem]) > 0)
          if (g_guiSettings.GetBool("myvideos.autoresume"))
            btn_Restart = pMenu->AddButton(20132);    // Restart Video
          else
            btn_Resume = pMenu->AddButton(13381);     // Resume Video
      }

      if (GetID() == WINDOW_VIDEO_FILES && (g_settings.m_vecProfiles[g_settings.m_iLastLoadedProfileIndex].canWriteDatabases() || g_passwordManager.bMasterUser))
      {
        CScraperParser parser;
        if (parser.Load("q:\\system\\scrapers\\video\\"+info.strPath))
          info.strTitle = parser.GetName();

        if (m_vecItems[iItem]->m_bIsFolder)
        {
          if (iFound==0)
          {
            CStdString strPath(m_vecItems[iItem]->m_strPath);
            CUtil::AddSlashAtEnd(strPath);
            if ((info.strContent.Equals("movies") && m_database.HasMovieInfo(strPath)) || (info.strContent.Equals("tvshows") && m_database.HasTvShowInfo(strPath)))
              btn_Show_Info = pMenu->AddButton(iString);

            btn_Assign = pMenu->AddButton(20333);
          }
          else
          {
            btn_Show_Info = pMenu->AddButton(iString);
            btn_Update = pMenu->AddButton(13349);
            btn_Assign = pMenu->AddButton(20333);
          }
        }
        else
        {
          if ((info.strContent.Equals("movies") && (iFound > 0 || m_database.HasMovieInfo(m_vecItems[iItem]->m_strPath))) || m_database.HasEpisodeInfo(m_vecItems[iItem]->m_strPath))
            btn_Show_Info = pMenu->AddButton(iString);
          m_database.Open();
          if (!bIsGotoParent)
          {
            if (m_database.GetMovieInfo(m_vecItems[iItem]->m_strPath)<0 && m_database.GetEpisodeInfo(m_vecItems[iItem]->m_strPath)<0)
              btn_AddToDatabase = pMenu->AddButton(527); // Add to Database
          }
          m_database.Close();
        }
      }
    }
    if ((GetID() == WINDOW_VIDEO_NAV && !m_vecItems[iItem]->m_bIsFolder) || (GetID() == WINDOW_VIDEO_NAV && info.strContent.Equals("tvshows")))
      btn_Show_Info = pMenu->AddButton(iString);

    // is the item a database movie?
    if (GetID() == WINDOW_VIDEO_NAV && m_vecItems[iItem]->HasVideoInfoTag() && (g_settings.m_vecProfiles[g_settings.m_iLastLoadedProfileIndex].canWriteDatabases() || g_passwordManager.bMasterUser))
    {
      if (m_vecItems[iItem]->GetVideoInfoTag()->m_iSeason > 0 || m_vecItems[iItem]->GetVideoInfoTag()->m_strShowTitle.IsEmpty()) // only episodes and movies
      {
        if (m_vecItems[iItem]->GetVideoInfoTag()->m_bWatched)
          btn_Mark_UnWatched = pMenu->AddButton(16104); //Mark as UnWatched
        else
          btn_Mark_Watched = pMenu->AddButton(16103);   //Mark as Watched
      }
      if (g_settings.m_vecProfiles[g_settings.m_iLastLoadedProfileIndex].canWriteDatabases() || g_passwordManager.bMasterUser)
        btn_Update_Title = pMenu->AddButton(16105); //Edit Title
    }
    if (!bIsGotoParent)
    {
      // video playlists or file operations are allowed
      if ((m_vecItems.m_strPath.Equals("special://videoplaylists/")) || (GetID() == WINDOW_VIDEO_FILES && g_guiSettings.GetBool("filelists.allowfiledeletion")))
      {
        if (!m_vecItems[iItem]->IsReadOnly())
        { // enable only if writeable
          btn_Delete = pMenu->AddButton(117);
          btn_Rename = pMenu->AddButton(118);
        }
      }
      // delete titles from database
      if (GetID() == WINDOW_VIDEO_NAV && (g_settings.m_vecProfiles[g_settings.m_iLastLoadedProfileIndex].canWriteDatabases() || g_passwordManager.bMasterUser))
      {
        CVideoDatabaseDirectory dir;
        NODE_TYPE node = dir.GetDirectoryChildType(m_vecItems.m_strPath);
        if (node == NODE_TYPE_TITLE_MOVIES || node == NODE_TYPE_EPISODES || node == NODE_TYPE_TITLE_TVSHOWS)
          btn_Delete = pMenu->AddButton(646);
      }
    }
  } // if (bContextDriven)
  // non-contextual buttons
  int btn_Settings = pMenu->AddButton(5);			// Settings
  int btn_GoToRoot = 0;
  if (!m_vecItems.m_strPath.IsEmpty())
    btn_GoToRoot = pMenu->AddButton(20128);

  int btn_Switch = 0;													// Switch Media
  int btn_NowPlaying = 0;											// Now Playing

  // Switch Media is only visible in files window
  if (GetID() == WINDOW_VIDEO_FILES)
  {
    btn_Switch = pMenu->AddButton(523); // switch media
  }

  // Now Playing... at the very bottom of the list for easy access
  if (g_playlistPlayer.GetPlaylist(PLAYLIST_VIDEO).size() > 0)
      btn_NowPlaying = pMenu->AddButton(13350);

  // position it correctly
  pMenu->SetPosition(posX - pMenu->GetWidth() / 2, posY - pMenu->GetHeight() / 2);
  pMenu->DoModal();

  int btnid = pMenu->GetButton();
  if (btnid>0)
  {
    // play part
    if (btnid == btn_PlayPart)
    {
      CFileItemList items;
      CDirectory::GetDirectory(m_vecItems[iItem]->m_strPath,items);
      CGUIDialogFileStacking* dlg = (CGUIDialogFileStacking*)m_gWindowManager.GetWindow(WINDOW_DIALOG_FILESTACKING);
      if (!dlg)
        return;
      dlg->SetNumberOfFiles(items.Size());
      dlg->DoModal();
      int btn2 = dlg->GetSelectedFile();
      if (btn2 > 0)
      {
        if (btn2 > 1)
        {
          vector<long> times;
          if (!m_database.GetStackTimes(m_vecItems[iItem]->m_strPath, times)) // need to calculate them times
            return;

          m_vecItems[iItem]->m_lStartOffset = times[btn2-2]*75; // wtf?
        }
        else
          m_vecItems[iItem]->m_lStartOffset = 0;

        OnClick(iItem);
      }
    }
    // queue
    if (btnid == btn_Queue)
    {
      OnQueueItem(iItem);
    }
    // play
    else if (btnid == btn_PlayWith)
    {
      // if folder, play with default player
      if (m_vecItems[iItem]->m_bIsFolder)
      {
        PlayItem(iItem);
      }
      else
      {
        g_application.m_eForcedNextPlayer = CPlayerCoreFactory::SelectPlayerDialog(vecCores, posX, posY);
        if( g_application.m_eForcedNextPlayer != EPC_NONE )
          OnClick(iItem);
      }
    }
    // restart
    else if (btnid == btn_Restart)
    {
      OnRestartItem(iItem);
    }
    // resume
    else if (btnid == btn_Resume)
    {
      OnResumeItem(iItem);
    }
    else if (btnid == btn_Assign)
    {
      OnAssignContent(iItem,iFound,info);
    }
    else if (btnid  == btn_Update) // update content 
    {
      OnScan(m_vecItems[iItem]->m_strPath,info);
    }
    // video info
    else if (btnid == btn_Show_Info)
    {
      OnInfo(iItem,info);
    }
    // unwatched
    else if (btnid == btn_Mark_UnWatched)
    {
      MarkUnWatched(iItem);
    }
    // watched
    else if (btnid == btn_Mark_Watched)
    {
      MarkWatched(iItem);
    }
    // update title
    else if (btnid == btn_Update_Title)
    {
      UpdateVideoTitle(iItem);
    }
    // delete
    else if (btnid == btn_Delete)
    {
      OnDeleteItem(iItem);
    }
    // rename
    else if (btnid == btn_Rename)
    {
      OnRenameItem(iItem);
    }
    // settings
    else if (btnid == btn_Settings)
    {
      m_gWindowManager.ActivateWindow(WINDOW_SETTINGS_MYVIDEOS);
      return;
    }
    // go to root
    else if (btnid == btn_GoToRoot)
    {
      Update("");
      return;
    }
    // switch media
    else if (btnid == btn_Switch)
    {
      CGUIDialogContextMenu::SwitchMedia("video", m_vecItems.m_strPath, posX, posY);
      return;
    }
    // now playing
    else if (btnid ==  btn_NowPlaying)
    {
      m_gWindowManager.ActivateWindow(WINDOW_VIDEO_PLAYLIST);
      return;
    }
    else if (btnid == btn_AddToDatabase)
    {
      AddToDatabase(iItem);
    }
  }
  if (iItem < m_vecItems.Size() && iItem > -1)
    m_vecItems[iItem]->Select(bSelected);
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
  // Reset Playlistplayer, playback started now does
  // not use the playlistplayer.
  g_playlistPlayer.Reset();
  g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_NONE);

  if ( iItem < 0 || iItem >= (int)m_vecItems.Size() ) return false;
  CFileItem* pItem = m_vecItems[iItem];

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

    if( item->m_lStartOffset == STARTOFFSET_RESUME )
    {
      startoffset = GetResumeItemOffset(item);

      if( startoffset & 0xF0000000 ) /* file is specified as a flag */
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
        for(unsigned i = 0; i < times.size(); i++)
        {
          totaltime += times[i]*75;
          if( startoffset < totaltime )
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
        if (selectedFile < 1) return ;
      }
    }
    // add to our movie list
    for (unsigned int i = 0; i < movies.size(); i++)
    {
      movieList.Add(new CFileItem(movies[i], false));
    }
  }
  else
  {
    movieList.Add(new CFileItem(*item));
  }

  g_playlistPlayer.Reset();
  g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_VIDEO);
  CPlayList& playlist = g_playlistPlayer.GetPlaylist(PLAYLIST_VIDEO);
  playlist.Clear();
  for (int i = selectedFile - 1; i < (int)movieList.Size(); ++i)
  {
    CPlayList::CPlayListItem playlistItem;
    CUtil::ConvertFileItemToPlayListItem(movieList[i], playlistItem);
    if (i == selectedFile - 1)
      playlistItem.m_lStartOffset = startoffset;
    playlist.Add(playlistItem);
  }
  // play movie...
  g_playlistPlayer.Play(0);
}

void CGUIWindowVideoBase::OnDeleteItem(int iItem)
{
  if ( iItem < 0 || iItem >= m_vecItems.Size()) return;
  // HACK: stacked files need to be treated as folders in order to be deleted
  if (m_vecItems[iItem]->IsStack())
    m_vecItems[iItem]->m_bIsFolder = true;
  if (g_settings.m_vecProfiles[g_settings.m_iLastLoadedProfileIndex].getLockMode() != LOCK_MODE_EVERYONE && g_settings.m_vecProfiles[g_settings.m_iLastLoadedProfileIndex].filesLocked())
    if (!g_passwordManager.IsMasterLockUnlocked(true))
      return;
  if (!CGUIWindowFileManager::DeleteItem(m_vecItems[iItem]))
    return;
  Update(m_vecItems.m_strPath);
  m_viewControl.SetSelectedItem(iItem);
}

void CGUIWindowVideoBase::MarkUnWatched(int iItem)
{
  if ( iItem < 0 || iItem >= m_vecItems.Size() ) return ;
  CFileItem* pItem = m_vecItems[iItem];
  int iType=0;
  if (pItem->HasVideoInfoTag() && pItem->GetVideoInfoTag()->m_iSeason > 0) // episode
    iType = 1;
  m_database.MarkAsUnWatched(atol(pItem->GetVideoInfoTag()->m_strSearchString.c_str()),iType>0);
  CUtil::DeleteVideoDatabaseDirectoryCache();
  m_viewControl.SetSelectedItem(iItem);
  Update(m_vecItems.m_strPath);
}

//Add Mark a Title as watched
void CGUIWindowVideoBase::MarkWatched(int iItem)
{
  if ( iItem < 0 || iItem >= m_vecItems.Size() ) return ;
  CFileItem* pItem = m_vecItems[iItem];
  int iType=0;
  if (pItem->HasVideoInfoTag() && pItem->GetVideoInfoTag()->m_iSeason > 0) // episode
    iType = 1;
  m_database.MarkAsWatched(atol(pItem->GetVideoInfoTag()->m_strSearchString.c_str()),iType>0);
  CUtil::DeleteVideoDatabaseDirectoryCache();
  m_viewControl.SetSelectedItem(iItem);
  Update(m_vecItems.m_strPath);
}

//Add change a title's name
void CGUIWindowVideoBase::UpdateVideoTitle(int iItem)
{
  if ( iItem < 0 || iItem >= m_vecItems.Size() ) return ;
  CFileItem* pItem = m_vecItems[iItem];
  
  CVideoInfoTag detail;
  int iType=0;
  if (pItem->HasVideoInfoTag() && !pItem->GetVideoInfoTag()->m_strShowTitle.IsEmpty()) // tvshow
    iType = 2;
  if (pItem->HasVideoInfoTag() && pItem->GetVideoInfoTag()->m_iSeason > 0) // episode
    iType = 1;

  if (iType == 0) // movies
    m_database.GetMovieInfo("", detail, atol(pItem->GetVideoInfoTag()->m_strSearchString.c_str()));
  if (iType == 1) //  episodes
    m_database.GetEpisodeInfo(pItem->m_strPath,detail,atol(pItem->GetVideoInfoTag()->m_strSearchString.c_str()));
  if (iType == 2) // tvshows
    m_database.GetTvShowInfo(pItem->GetVideoInfoTag()->m_strFileNameAndPath,detail,atol(pItem->GetVideoInfoTag()->m_strSearchString.c_str()));

  CStdString strInput;
  strInput = detail.m_strTitle;

  //Get the new title
  if (!CGUIDialogKeyboard::ShowAndGetInput(strInput, g_localizeStrings.Get(16105), false)) return ;
  m_database.UpdateMovieTitle(atol(pItem->GetVideoInfoTag()->m_strSearchString), strInput, iType);
  CUtil::DeleteVideoDatabaseDirectoryCache();
  m_viewControl.SetSelectedItem(iItem);
  Update(m_vecItems.m_strPath);
}

void CGUIWindowVideoBase::LoadPlayList(const CStdString& strPlayList, int iPlayList /* = PLAYLIST_VIDEO */)
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

    // recursively add items to list
    CFileItemList queuedItems;
    AddItemToPlayList(&item, queuedItems);

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

CStdString CGUIWindowVideoBase::GetnfoFile(CFileItem *item)
{
  CStdString nfoFile;
  // Find a matching .nfo file
  if (item->m_bIsFolder)
  {
    // see if there is a unique nfo file in this folder, and if so, use that
    CFileItemList items;
    CDirectory dir;
    if (dir.GetDirectory(item->m_strPath, items, ".nfo") && items.Size())
    {
      int numNFO = -1;
      for (int i = 0; i < items.Size(); i++)
      {
        if (items[i]->IsNFO())
        {
          if (numNFO == -1)
            numNFO = i;
          else
          {
            numNFO = -1;
            break;
          }
        }
      }
      if (numNFO > -1)
        return items[numNFO]->m_strPath;
    }
  }

  // file
  CStdString strExtension;
  CUtil::GetExtension(item->m_strPath, strExtension);

  if (CUtil::IsInRAR(item->m_strPath)) // we have a rarred item - we want to check outside the rars
  {
    CFileItem item2(*item);
    CURL url(item->m_strPath);
    CStdString strPath;
    CUtil::GetDirectory(url.GetHostName(),strPath);
    CUtil::AddFileToFolder(strPath,CUtil::GetFileName(item->m_strPath),item2.m_strPath);
    return GetnfoFile(&item2);
  }

  // already an .nfo file?
  if ( strcmpi(strExtension.c_str(), ".nfo") == 0 )
    nfoFile = item->m_strPath;
  // no, create .nfo file
  else
    CUtil::ReplaceExtension(item->m_strPath, ".nfo", nfoFile);

  // test file existance
  if (!nfoFile.IsEmpty() && !CFile::Exists(nfoFile))
      nfoFile.Empty();

  // try looking for .nfo file for a stacked item
  if (item->IsStack())
  {
    // first try .nfo file matching first file in stack
    CStackDirectory dir;
    CStdString firstFile = dir.GetFirstStackedFile(item->m_strPath);
    CFileItem item2;
    item2.m_strPath = firstFile;
    nfoFile = GetnfoFile(&item2);
    // else try .nfo file matching stacked title
    if (nfoFile.IsEmpty())
    {
      CStdString stackedTitlePath = dir.GetStackedTitlePath(item->m_strPath);
      item2.m_strPath = stackedTitlePath;
      nfoFile = GetnfoFile(&item2);
    }
  }

  if (nfoFile.IsEmpty()) // final attempt - strip off any cd1 folders
  {
    CStdString strPath;
    CUtil::GetDirectory(item->m_strPath,strPath);
    CFileItem item2;
    if (strPath.Mid(strPath.size()-3).Equals("cd1"))
    {
      strPath = strPath.Mid(0,strPath.size()-3);
      CUtil::AddFileToFolder(strPath,CUtil::GetFileName(item->m_strPath),item2.m_strPath);
      return GetnfoFile(&item2);
    }
  }

  return nfoFile;
}

void CGUIWindowVideoBase::ApplyIMDBThumbToFolder(const CStdString &folder, const CStdString &imdbThumb)
{
  // copy icon to folder also;
  if (CFile::Exists(imdbThumb))
  {
    CFileItem folderItem(folder, true);
    CStdString strThumb(folderItem.GetCachedVideoThumb());
    CFile::Cache(imdbThumb.c_str(), strThumb.c_str(), NULL, NULL);
  }
}

bool CGUIWindowVideoBase::Update(const CStdString &strDirectory)
{
  if (m_thumbLoader.IsLoading())
    m_thumbLoader.StopThread();

  if (!CGUIMediaWindow::Update(strDirectory))
    return false;

  m_thumbLoader.Load(m_vecItems);

  return true;
}

void CGUIWindowVideoBase::OnPrepareFileItems(CFileItemList &items)
{
  items.SetCachedVideoThumbs();
}

void CGUIWindowVideoBase::AddToDatabase(int iItem)
{
  if (iItem < 0 || iItem >= m_vecItems.Size()) return;
  CFileItem* pItem;
  pItem = m_vecItems[iItem];
  if (pItem->IsParentFolder()) return;
  if (pItem->m_bIsFolder) return;

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
  CStdString strCache = "Z:\\" + CUtil::GetFileName(strXml);
  CUtil::GetFatXQualifiedPath(strCache);
  if (CFile::Exists(strXml))
  {
    bGotXml = true;
    CLog::Log(LOGDEBUG,__FUNCTION__": found matching xml file:[%s]", strXml.c_str());
    CFile::Cache(strXml, strCache);
    CIMDB imdb;
    if (!imdb.LoadXML(strCache, movie, false))
    {
      CLog::Log(LOGERROR,__FUNCTION__": Could not parse info in file:[%s]", strXml.c_str());
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
    if (!CDirectory::GetDirectory("videodb://1/", items))
      return;
    for (int i = 0; i < items.Size(); ++i)
      pSelect->Add(items[i]->GetLabel());
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

  // Why should we double check title for uniqueness?  Who cares if 2 movies
  // have the same name in the db?

  /*
  // double check title for uniqueness
  items.ClearKeepPointer();
  if (!CDirectory::GetDirectory("videodb://2/", items))
    return;
  for (int i = 0; i < items.Size(); ++i)
  {
    if (items[i]->m_strTitle.Equals(movie.m_strTitle))
    {
      // uh oh, duplicate title
      CGUIDialogOK *pDialog = (CGUIDialogOK*)m_gWindowManager.GetWindow(WINDOW_DIALOG_OK);
      if (pDialog)
      {
        pDialog->SetHeading(529); // Duplicate Title
        pDialog->SetLine(0, movie.m_strTitle);
        pDialog->SetLine(1, "");
        pDialog->SetLine(2, "");
        pDialog->SetLine(3, "");
        pDialog->DoModal();
      }
      return;
    }
  }*/

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
  if ( !CGUIDialogKeyboard::ShowAndGetInput(strSearch, g_localizeStrings.Get(16017), false) )
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
      CFileItem* pItem = items[i];
      pDlgSelect->Add(pItem->GetLabel());
    }

    pDlgSelect->DoModal();

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

void CGUIWindowVideoBase::EnumerateSeriesFolder(const CFileItem* item, IMDB_EPISODELIST& episodeList)


{
  CFileItemList items;
  if (item->m_bIsFolder)
    CUtil::GetRecursiveListing(item->m_strPath,items,g_stSettings.m_videoExtensions,true);
  else
    items.Add(new CFileItem(*item));

  // enumerate
  
  CStdStringArray expression = g_advancedSettings.m_tvshowTwoPartStackRegExps;
  unsigned int iTwoParters=expression.size();
  expression.insert(expression.end(),g_advancedSettings.m_tvshowStackRegExps.begin(),g_advancedSettings.m_tvshowStackRegExps.end());
  for (int i=0;i<items.Size();++i)
  {
    if (items[i]->m_bIsFolder)
      continue;
    CStdString strPath;
    CUtil::GetDirectory(items[i]->m_strPath,strPath);
    if (strPath.Mid(strPath.size()-6).Equals("sample")) // skip sample folders
      continue;
    for (unsigned int j=0;j<expression.size();++j)
    {
      CRegExp reg;
      if (!reg.RegComp(expression[j]))
        break;
      if (reg.RegFind(items[i]->m_strPath.c_str()) > -1)
      {
        char* season = reg.GetReplaceString("\\1");
        char* episode = reg.GetReplaceString("\\2");
        if (season && episode)
        {
          int iSeason = atoi(season);
          int iEpisode = atoi(episode);
          std::pair<int,int> key(iSeason,iEpisode);
          CIMDBUrl url;
          if (j < iTwoParters) // we have a two parter - do the nasty. this gives us a separate file id...
          {
            CStdString strNasty = "stack://"+items[i]->m_strPath;
            url.m_scrURL.push_back(CScraperUrl(strNasty));
          }
          else
            url.m_scrURL.push_back(CScraperUrl(items[i]->m_strPath));
          episodeList.insert(std::make_pair<std::pair<int,int>,CIMDBUrl>(key,url));
          if (j >= iTwoParters)
            break;
        }
      }
    }
  }
}

void CGUIWindowVideoBase::OnProcessSeriesFolder(IMDB_EPISODELIST& episodes, const CFileItem* item, long lShowId, CIMDB& IMDB, CGUIDialogProgress* pDlgProgress /* = NULL */)
{
  if (pDlgProgress)
  {
    if (item->m_bIsFolder)
      pDlgProgress->SetLine(2, 20355);
    else
      pDlgProgress->SetLine(2, 20361);
    pDlgProgress->SetPercentage(0);
    pDlgProgress->ShowProgressBar(true);
    pDlgProgress->Progress();
  }
  IMDB_EPISODELIST files;
  EnumerateSeriesFolder(item,files);

  int iMax = files.size();
  int iCurr = 0;
  m_database.BeginTransaction();
  for (IMDB_EPISODELIST::iterator iter = files.begin();iter != files.end();++iter)
  {
    if (pDlgProgress)
    {
      pDlgProgress->SetLine(2, 20361);
      pDlgProgress->SetPercentage((int)((float)(iCurr++)/iMax*100));
      pDlgProgress->Progress();
    }
    IMDB_EPISODELIST::iterator iter2 = episodes.find(iter->first);
    if (iter2 != episodes.end())
    {
      CVideoInfoTag episodeDetails;
      if (m_database.GetEpisodeInfo(iter->second.m_scrURL[0].m_url) > -1)
        continue;

      if (!IMDB.GetEpisodeDetails(iter2->second,episodeDetails,pDlgProgress))
        break;
      episodeDetails.m_iSeason = iter2->first.first;
      episodeDetails.m_iEpisode = iter2->first.second;
      if (pDlgProgress && pDlgProgress->IsCanceled())
      {
        pDlgProgress->Close();
        m_database.RollbackTransaction();
        return;
      }
      m_database.DeleteDetailsForEpisode(iter->second.m_scrURL[0].m_url);
      CFileItem item;
      item.m_strPath = iter->second.m_scrURL[0].m_url;
      AddMovieAndGetThumb(&item,"tvshows",episodeDetails,lShowId);
    }
  }
  m_database.CommitTransaction();
}

long CGUIWindowVideoBase::AddMovieAndGetThumb(CFileItem *pItem, const CStdString &content, const CVideoInfoTag &movieDetails, long idShow)
{
  long lResult=-1;
  // add to all movies in the stacked set
  if (content.Equals("movies"))
    m_database.SetDetailsForMovie(pItem->m_strPath, movieDetails);
  else if (content.Equals("tvshows"))
  {
    if (pItem->m_bIsFolder)
    {
      CStdString strPath(pItem->m_strPath);
      CUtil::AddSlashAtEnd(strPath);
      lResult=m_database.SetDetailsForTvShow(strPath, movieDetails);
    }
    else
    {
      long lEpisodeId = m_database.GetEpisodeInfo(pItem->m_strPath);
      if (lEpisodeId < 0)
        m_database.AddEpisode(idShow,pItem->m_strPath);

      lResult=m_database.SetDetailsForEpisode(pItem->m_strPath,movieDetails,idShow);
    }
  }
  // get & save thumbnail
  CStdString strThumb = "";
  CStdString strImage = movieDetails.m_strPictureURL.m_url;
  if (strImage.size() > 0)
  {
    // check for a cached thumb or user thumb
    pItem->SetVideoThumb();
    if (pItem->HasThumbnail())
      return lResult;
    strThumb = pItem->GetCachedVideoThumb();

    CStdString strExtension;
    CUtil::GetExtension(strImage, strExtension);
    CStdString strTemp = "Z:\\temp";
    strTemp += strExtension;
    ::DeleteFile(strTemp.c_str());
    if (m_dlgProgress)
    {
      m_dlgProgress->SetLine(2, 415);
      m_dlgProgress->Progress();
    }
    CHTTP http;
    http.Download(strImage, strTemp);

    try
    {
      CPicture picture;
      picture.DoCreateThumbnail(strTemp, strThumb);
    }
    catch (...)
    {
      CLog::Log(LOGERROR,"Could not make imdb thumb from %s", strImage.c_str());
      ::DeleteFile(strThumb.c_str());
    }
    ::DeleteFile(strTemp.c_str());
  }
  
  return lResult;
}
