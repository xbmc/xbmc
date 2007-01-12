//

#include "stdafx.h"
#include "GUIWindowVideoBase.h"
#include "Util.h"
#include "Utils/IMDB.h"
#include "Utils/HTTP.h"
#include "GUIWindowVideoInfo.h"
#include "PlayListFactory.h"
#include "Application.h"
#include "NFOFile.h"
#include "utils/fstrcmp.h"
#include "PlayListPlayer.h"
#include "GUIThumbnailPanel.h"
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

using namespace VIDEODATABASEDIRECTORY;

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
      if (GetID() != WINDOW_VIDEO_PLAYLIST)
        g_stSettings.m_iVideoStartWindow = GetID();
      
      
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
  CGUIMediaWindow::UpdateButtons();
}

void CGUIWindowVideoBase::OnInfo(int iItem, const SScraperInfo& info)
{
  if ( iItem < 0 || iItem >= m_vecItems.Size() ) return ;
  CFileItem* pItem = m_vecItems[iItem];
  CStdString strFile = CUtil::GetFileName(pItem->m_strPath);
  // ShowIMDB can kill the item as this window can be closed while we do it,
  // so take a copy of the item now
  CFileItem item(*pItem);
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
  bool bUpdate = false;
  bool bFound = false;

  if (!pDlgProgress) return ;
  if (!pDlgSelect) return ;
  if (!pDlgInfo) return ;
  CUtil::ClearCache();

  // 1.  Check for already downloaded information, and if we have it, display our dialog
  //     Return if no Refresh is needed.
  if (m_database.HasMovieInfo(item->m_strPath))
  {
    CIMDBMovie movieDetails;
    m_database.GetMovieInfo(item->m_strPath, movieDetails);
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
  CIMDBMovie movieDetails;

  // 2. Look for a nfo File to get the search URL
  CStdString nfoFile = GetnfoFile(item);
  if ( !nfoFile.IsEmpty() )
  {
    CLog::Log(LOGDEBUG,"Found matching nfo file: %s", nfoFile.c_str());
    if ( CFile::Cache(nfoFile, "Z:\\movie.nfo", NULL, NULL))
    {
      CNfoFile nfoReader;
      if ( nfoReader.Create("Z:\\movie.nfo") == S_OK)
      {
	    	url.m_strURL.push_back(nfoReader.m_strImDbUrl);
        CLog::Log(LOGDEBUG,"-- imdb url: %s", url.m_strURL[0].c_str());
      }
      else
        CLog::Log(LOGERROR,"Unable to find an imdb url in nfo file: %s", nfoFile.c_str());
    }
    else
      CLog::Log(LOGERROR,"Unable to cache nfo file: %s", nfoFile.c_str());
  }

  CStdString movieName = item->GetLabel();
  // 3. Run a loop so that if we Refresh we re-run this block
  bool needsRefresh(false);
  do
  {
    // 4. if we don't have a url, or need to refresh the search
    //    then do the web search
    if (url.m_strURL.size() == 0 || needsRefresh)
    {
      // 4a. show dialog that we're busy querying www.imdb.com
      pDlgProgress->SetHeading(197);
      pDlgProgress->SetLine(0, movieName);
      pDlgProgress->SetLine(1, "");
      pDlgProgress->SetLine(2, "");
      pDlgProgress->StartModal();
      pDlgProgress->Progress();

      // 4b. do the websearch
      IMDB.SetScraperInfo(info);
      IMDB_MOVIELIST movielist;
      if (IMDB.FindMovie(movieName, movielist, pDlgProgress))
      {
        pDlgProgress->Close();
        if (movielist.size() > 0)
        {
          // 4c. found movies - allow selection of the movie we found
          pDlgSelect->SetHeading(196);
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
    // 4c. Check if url is still empty - occurs if user has selected to do a manual
    //     lookup, or if the IMDb lookup failed or was cancelled.
    if (url.m_strURL.size() == 0)
    {
      // Check for cancel of the progress dialog
      pDlgProgress->Close();
      if (pDlgProgress->IsCanceled())
      {
        return;
      }

      // Prompt the user to input the movieName
      if (!CGUIDialogKeyboard::ShowAndGetInput(movieName, g_localizeStrings.Get(16009), false))
      {
        return; // user backed out
      }

      needsRefresh = true;
    }
    else
    {
      // 5. Download the movie information
      // show dialog that we're downloading the movie info
      pDlgProgress->SetHeading(198);
      pDlgProgress->SetLine(0, movieName);
      pDlgProgress->SetLine(1, url.m_strTitle);
      pDlgProgress->SetLine(2, "");
      pDlgProgress->StartModal();
      pDlgProgress->Progress();

      // get the movie info
      if (IMDB.GetDetails(url, movieDetails, pDlgProgress))
      {
        // got all movie details :-)
        OutputDebugString("got details\n");
        pDlgProgress->Close();

        // now show the imdb info
        OutputDebugString("show info\n");

        // Add to the database if applicable
        if (item->m_strPath && (g_settings.m_vecProfiles[g_settings.m_iLastLoadedProfileIndex].canWriteDatabases() || g_passwordManager.bMasterUser))
        {
          m_database.SetMovieInfo(item->m_strPath, movieDetails);
          // remove directory caches
          CFileItemList items;
          CDirectory::GetDirectory("z:\\",items,".fi",false);
          for (int i=0;i<items.Size();++i)
          {
            if (!items[i]->m_bIsFolder)
            {
              CFile::Delete(items[i]->m_strPath);
            }
          }
        }

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

  CIMDBMovie movieDetails;
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
  // add item 2 playlist
  CFileItem movieItem(*m_vecItems[iItem]);
  if (movieItem.IsRAR() || movieItem.IsZIP())
    return;
  if (movieItem.IsStack())
  {
    // TODO: Remove this once the main player code is capable of handling the stack:// protocol
    vector<CStdString> movies;
    GetStackedFiles(movieItem.m_strPath, movies);
    if (movies.size() <= 0) return;
    for (int i = 0; i < (int)movies.size(); ++i)
    {
      CFileItem movieFile(movies[i], false);
      CStdString strFileNum;
      strFileNum.Format("(%2.2i)",i+1);
      movieFile.SetLabel(movieItem.GetLabel() + " " + strFileNum);
      AddItemToPlayList(&movieFile);
    }
  }
  else
    AddItemToPlayList(&movieItem);
  //move to next item
  m_viewControl.SetSelectedItem(iItem + 1);
}

void CGUIWindowVideoBase::AddItemToPlayList(const CFileItem* pItem, int iPlaylist /* = PLAYLIST_VIDEO */)
{
  if (pItem->m_bIsFolder)
  {
    // Check if we add a locked share
    if ( pItem->m_bIsShareOrDrive )
    {
      CFileItem item = *pItem;
      if ( !g_passwordManager.IsItemUnlocked( &item, "video" ) )
        return ;
    }

    // recursive
    if (pItem->IsParentFolder()) return ;
    CStdString strDirectory = m_vecItems.m_strPath;
    m_vecItems.m_strPath = pItem->m_strPath;
    CFileItemList items;
    GetDirectory(m_vecItems.m_strPath, items);

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
          if (strPath.substr(strPath.size()-6) == "sample") // skip sample folders
            continue;
        }
      }
      AddItemToPlayList(items[i], iPlaylist);
    }
    m_vecItems.m_strPath = strDirectory;
  }
  else
  {
    if (!pItem->IsNFO() && pItem->IsVideo() && !pItem->IsPlayList())
    {
      CPlayList::CPlayListItem playlistItem ;
      playlistItem.SetFileName(pItem->m_strPath);
      playlistItem.SetDescription(pItem->GetLabel());
      playlistItem.SetDuration(pItem->m_musicInfoTag.GetDuration());
      g_playlistPlayer.GetPlaylist(iPlaylist).Add(playlistItem);
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
  int btn_Queue = 0;					// Add to Playlist
  int btn_PlayWith = 0;				// Play
  int btn_Restart = 0;				// Restart Video from Beginning
  int btn_Resume = 0;					// Resume Video
  int btn_Show_Info = 0;			// Show Video Information
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

		bool bIsGotoParent = m_vecItems[iItem]->IsParentFolder();
		if (!bIsGotoParent)
		{
			// don't show the add to playlist button in playlist window
      if (GetID() != WINDOW_VIDEO_PLAYLIST)
      {
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
        CStdString strPath = m_vecItems[iItem]->m_strPath;
        CUtil::RemoveSlashAtEnd(strPath);
        if (m_vecItems[iItem]->m_bIsFolder && m_database.GetScraperForPath(strPath, info.strPath, info.strContent))
          iFound = 1;
        else
        {
          CStdString strParent;
          while (CUtil::GetParentPath(strPath, strParent))
          {
            CUtil::RemoveSlashAtEnd(strParent);
            if (m_database.GetScraperForPath(strParent, info.strPath, info.strContent))
            {
              iFound = 2;
              break;
            }
            strPath = strParent;
          }
        }
        if (m_vecItems[iItem]->m_bIsFolder)
        {
          if (iFound==0)
          {
              btn_Assign = pMenu->AddButton(20333);
          }
          else
          {
            btn_Show_Info = pMenu->AddButton(13346);
            btn_Update = pMenu->AddButton(13349);
            btn_Assign = pMenu->AddButton(20333);
          }
        }
        else
        {
          btn_Show_Info = pMenu->AddButton(13346);
          if(!iFound)
            pMenu->EnableButton(btn_Show_Info, false);
        }
      }
    }
    if (GetID() == WINDOW_VIDEO_NAV && !m_vecItems[iItem]->m_bIsFolder)
      btn_Show_Info = pMenu->AddButton(13346);

		// is the item a database movie?
		if (GetID() == WINDOW_VIDEO_NAV && !m_vecItems[iItem]->m_musicInfoTag.GetURL().IsEmpty() && (g_settings.m_vecProfiles[g_settings.m_iLastLoadedProfileIndex].canWriteDatabases() || g_passwordManager.bMasterUser))
		{
			// uses Loaded to hold Watched/UnWatched status
			if (m_vecItems[iItem]->m_musicInfoTag.Loaded())
				btn_Mark_UnWatched = pMenu->AddButton(16104); //Mark as UnWatched
			else
				btn_Mark_Watched = pMenu->AddButton(16103);   //Mark as Watched

			if (g_settings.m_vecProfiles[g_settings.m_iLastLoadedProfileIndex].canWriteDatabases() || g_passwordManager.bMasterUser)
				btn_Update_Title = pMenu->AddButton(16105); //Edit Title
		}

	  // hide delete button unless enabled, or in title window
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
        if (node == NODE_TYPE_TITLE)
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
      OnScan(m_vecItems[iItem]->m_strPath,info,-1,-1);
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
  }
  if (iItem < m_vecItems.Size())
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

  PlayMovie(pItem);

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
  g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_VIDEO_TEMP);
  CPlayList& playlist = g_playlistPlayer.GetPlaylist(PLAYLIST_VIDEO_TEMP);
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
  g_playlistPlayer.PlayNext();
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
  m_database.MarkAsUnWatched(atol(pItem->m_musicInfoTag.GetURL()));
  CUtil::ClearFileItemCache();
  m_viewControl.SetSelectedItem(iItem);
  Update(m_vecItems.m_strPath);
}

//Add Mark a Title as watched
void CGUIWindowVideoBase::MarkWatched(int iItem)
{
  if ( iItem < 0 || iItem >= m_vecItems.Size() ) return ;
  CFileItem* pItem = m_vecItems[iItem];
  m_database.MarkAsWatched(atol(pItem->m_musicInfoTag.GetURL()));
  CUtil::ClearFileItemCache();
  m_viewControl.SetSelectedItem(iItem);
  Update(m_vecItems.m_strPath);
}

//Add change a title's name
void CGUIWindowVideoBase::UpdateVideoTitle(int iItem)
{
  if ( iItem < 0 || iItem >= m_vecItems.Size() ) return ;
  CFileItem* pItem = m_vecItems[iItem];

  //Get Current Name
  CIMDBMovie detail;
  m_database.GetMovieInfo("", detail, atol(pItem->m_musicInfoTag.GetURL()));
  CStdString strInput;
  strInput = detail.m_strTitle;

  //Get the new title
  if (!CGUIDialogKeyboard::ShowAndGetInput(strInput, g_localizeStrings.Get(16105), false)) return ;
  m_database.UpdateMovieTitle(atol(pItem->m_musicInfoTag.GetURL()), strInput);
  UpdateVideoTitleXML(detail.m_strIMDBNumber, strInput);
  m_viewControl.SetSelectedItem(iItem);
  Update(m_vecItems.m_strPath);
}

bool CGUIWindowVideoBase::UpdateVideoTitleXML(const CStdString strIMDBNumber, CStdString& strTitle)
{
  CStdString strXMLFile;
  CUtil::AddFileToFolder(g_settings.GetIMDbFolder(), strIMDBNumber + ".xml", strXMLFile);
  TiXmlBase::SetCondenseWhiteSpace(false);
  TiXmlDocument doc;
  if (!doc.LoadFile(strXMLFile))
    return false;

  TiXmlNode *details = doc.FirstChild("details");
  if (!details)
  {
    CLog::Log(LOGERROR, "IMDB: Invalid xml file");
    return false;
  }

  TiXmlNode* pNode = details->FirstChild("title");
  if (!pNode)
    return false;
  if (pNode->FirstChild())
      pNode->FirstChild()->SetValue(strTitle);
  else
    return false;

  return doc.SaveFile();
}

void CGUIWindowVideoBase::LoadPlayList(const CStdString& strPlayList, int iPlayList /* = PLAYLIST_VIDEO */)
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

    // clear current temp playlist
    g_playlistPlayer.ClearPlaylist(PLAYLIST_VIDEO_TEMP);
    g_playlistPlayer.Reset();

    // recursively add items to temp playlist
    AddItemToPlayList(&item, PLAYLIST_VIDEO_TEMP);

    // play!
    g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_VIDEO_TEMP);
    g_playlistPlayer.Play();
  }
  else if (!g_advancedSettings.m_playlistAsFolders && pItem->IsPlayList())
  {
    LoadPlayList(pItem->m_strPath, PLAYLIST_VIDEO_TEMP);
  }
  // otherwise just play the song
  else
    OnClick(iItem);
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

void CGUIWindowVideoBase::SetDatabaseDirectory(const VECMOVIES &movies, CFileItemList &items)
{
  DWORD time = timeGetTime();
  for (int i = 0; i < (int)movies.size(); ++i)
  {
    CIMDBMovie movie = movies[i];
    // add the appropiate movies to m_vecItems based on the showmode
/*    if (
      (g_stSettings.m_iMyVideoWatchMode == VIDEO_SHOW_ALL) ||
      (g_stSettings.m_iMyVideoWatchMode == VIDEO_SHOW_WATCHED && movie.m_bWatched == true) ||
      (g_stSettings.m_iMyVideoWatchMode == VIDEO_SHOW_UNWATCHED && movie.m_bWatched == false)
      )*/
    {
      // mark watched movies when showing all
      CStdString strTitle = movie.m_strTitle;
      CFileItem *pItem = new CFileItem(strTitle);
      pItem->m_strTitle=strTitle;
      pItem->m_strPath = movie.m_strFileNameAndPath;
      pItem->m_bIsFolder = false;
      pItem->m_bIsShareOrDrive = false;

      pItem->m_fRating = movie.m_fRating;
      SYSTEMTIME time;
      time.wYear = movie.m_iYear;
      pItem->m_musicInfoTag.SetReleaseDate(time);
//      pItem->m_strDVDLabel = movie.m_strDVDLabel;
      pItem->SetOverlayImage(CGUIListItem::ICON_OVERLAY_UNWATCHED,movie.m_bWatched);

      // Hack for extra info
      pItem->m_musicInfoTag.SetTitle(movie.m_strTitle);
      pItem->m_musicInfoTag.SetArtist(movie.m_strDirector);
      pItem->m_musicInfoTag.SetGenre(movie.m_strGenre);
      pItem->m_musicInfoTag.SetURL(movie.m_strSearchString);
      pItem->m_musicInfoTag.SetLoaded(movie.m_bWatched);
      // End hack for extra info

      items.Add(pItem);
    }
  }
  CLog::Log(LOGDEBUG, "Time taken for SetDatabaseDirectory(): %i", timeGetTime() - time);
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

