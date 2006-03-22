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
#include "GUIListControl.h"
#include "GUIPassword.h"
#include "FileSystem/ZipManager.h"
#include "FileSystem/StackDirectory.h"
#include "GUIDialogContextMenu.h"
#include "GUIDialogFileStacking.h"
#include "GUIWindowFileManager.h"

#include "SkinInfo.h"

#define CONTROL_BTNVIEWASICONS     2 
#define CONTROL_BTNSORTBY          3
#define CONTROL_BTNSORTASC         4
#define CONTROL_BTNTYPE            5
#define CONTROL_LIST              50
#define CONTROL_THUMBS            51
#define CONTROL_BIGLIST           52
#define CONTROL_LABELFILES        12

#define CONTROL_PLAY_DVD          6
#define CONTROL_STACK             7
#define CONTROL_BTNSCAN           8
#define CONTROL_IMDB              9
#define CONTROL_BTNSHOWMODE       10

CGUIWindowVideoBase::CGUIWindowVideoBase(DWORD dwID, const CStdString &xmlFile)
    : CGUIMediaWindow(dwID, xmlFile)
{
  m_bDisplayEmptyDatabaseMessage = false;
  m_iShowMode = VIDEO_SHOW_ALL;
}

CGUIWindowVideoBase::~CGUIWindowVideoBase()
{
}

bool CGUIWindowVideoBase::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_DEINIT:
    m_database.Close();
    break;

  case GUI_MSG_WINDOW_INIT:
    {
      m_rootDir.SetMask(g_stSettings.m_szMyVideoExtensions);
      m_rootDir.SetShares(g_settings.m_vecMyVideoShares);

      m_database.Open();

      m_dlgProgress = (CGUIDialogProgress*)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
      
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
        int nNewWindow = WINDOW_VIDEOS;
        switch (nSelected)
        {
        case 0:  // Movies
          nNewWindow = WINDOW_VIDEOS;
          break;
        case 1:  // Genre
          nNewWindow = WINDOW_VIDEO_GENRE;
          break;
        case 2:  // Actors
          nNewWindow = WINDOW_VIDEO_ACTOR;
          break;
        case 3:  // Year
          nNewWindow = WINDOW_VIDEO_YEAR;
          break;
        case 4:  // Titel
          nNewWindow = WINDOW_VIDEO_TITLE;
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
      else if (m_viewControl.HasControl(iControl))  // list/thumb control
      {
        // get selected item
        int iItem = m_viewControl.GetSelectedItem();
        int iAction = message.GetParam1();

        // iItem is checked for validity inside these routines
        if (iAction == ACTION_QUEUE_ITEM || iAction == ACTION_MOUSE_MIDDLE_CLICK)
        {
          OnQueueItem(iItem);
        }
        else if (iAction == ACTION_SHOW_INFO)
        {
          OnInfo(iItem);
        }
        else if (iAction == ACTION_CONTEXT_MENU || iAction == ACTION_MOUSE_RIGHT_CLICK)
        {
          OnPopupMenu(iItem);
        }
       	else if (iAction == ACTION_PLAYER_PLAY && !g_application.IsPlayingVideo())
        {
          OnResumeItem(iItem);
        }
        else if (iAction == ACTION_DELETE_ITEM)
        {
          // is delete allowed?
          // must be at the title window
          if (GetID() == WINDOW_VIDEO_TITLE)
            OnDeleteItem(iItem);

          // or be at the files window and have file deletion enabled
          else if (GetID() == WINDOW_VIDEOS && g_guiSettings.GetBool("VideoFiles.AllowFileDeletion"))
            OnDeleteItem(iItem);

          // or be at the video playlists location
          if (m_vecItems.m_strPath.Equals(CUtil::VideoPlaylistsLocation()))
            OnDeleteItem(iItem);

          else
            return false;
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

  strItem = g_localizeStrings.Get(135); // Genre
  msg2.SetLabel(strItem);
  g_graphicsContext.SendMessage(msg2);

  strItem = g_localizeStrings.Get(344); // Actors
  msg2.SetLabel(strItem);
  g_graphicsContext.SendMessage(msg2);

  strItem = g_localizeStrings.Get(345); // Year
  msg2.SetLabel(strItem);
  g_graphicsContext.SendMessage(msg2);

  strItem = g_localizeStrings.Get(369); // Titel
  msg2.SetLabel(strItem);
  g_graphicsContext.SendMessage(msg2);

  // Select the current window as default item
  int nWindow = 0;
  if (g_stSettings.m_iVideoStartWindow == WINDOW_VIDEO_GENRE) nWindow = 1;
  if (g_stSettings.m_iVideoStartWindow == WINDOW_VIDEO_ACTOR) nWindow = 2;
  if (g_stSettings.m_iVideoStartWindow == WINDOW_VIDEO_YEAR) nWindow = 3;
  if (g_stSettings.m_iVideoStartWindow == WINDOW_VIDEO_TITLE) nWindow = 4;
  CONTROL_SELECT_ITEM(CONTROL_BTNTYPE, nWindow);

  // disable scan and manual imdb controls if internet lookups are disabled
  if (g_guiSettings.GetBool("Network.EnableInternet"))
  {
    CONTROL_ENABLE(CONTROL_BTNSCAN);
    CONTROL_ENABLE(CONTROL_IMDB);
  }
  else
  {
    CONTROL_DISABLE(CONTROL_BTNSCAN);
    CONTROL_DISABLE(CONTROL_IMDB);
  }

  SET_CONTROL_LABEL(CONTROL_BTNSHOWMODE, g_localizeStrings.Get(16100 + m_iShowMode));
  CGUIMediaWindow::UpdateButtons();
}

void CGUIWindowVideoBase::OnInfo(int iItem)
{
  if ( iItem < 0 || iItem >= m_vecItems.Size() ) return ;
  CFileItem* pItem = m_vecItems[iItem];
  CStdString strFile = CUtil::GetFileName(pItem->m_strPath);
  ShowIMDB(pItem);
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

void CGUIWindowVideoBase::ShowIMDB(CFileItem *item)
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
    pDlgInfo->SetMovie(movieDetails, item->GetThumbnailImage());
    pDlgInfo->DoModal(GetID());
    item->SetThumbnailImage(pDlgInfo->GetThumbnail());
    if ( !pDlgInfo->NeedRefresh() ) return ;

    // quietly return if Internet lookups are disabled
    if (!g_guiSettings.GetBool("Network.EnableInternet")) return ;

    m_database.DeleteMovieInfo(item->m_strPath);
  }

  // quietly return if Internet lookups are disabled
  if (!g_guiSettings.GetBool("Network.EnableInternet")) return ;

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
        CIMDBMovie movieDetails;
        url.m_strURL = nfoReader.m_strImDbUrl;
        CLog::Log(LOGDEBUG,"-- imdb url: %s", url.m_strURL.c_str());
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
    if (url.m_strURL.IsEmpty() || needsRefresh)
    {
      // 4a. show dialog that we're busy querying www.imdb.com
      pDlgProgress->SetHeading(197);
      pDlgProgress->SetLine(0, movieName);
      pDlgProgress->SetLine(1, "");
      pDlgProgress->SetLine(2, "");
      pDlgProgress->StartModal(GetID());
      pDlgProgress->Progress();

      // 4b. do the websearch
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
          pDlgSelect->DoModal(GetID());

          // and wait till user selects one
          int iSelectedMovie = pDlgSelect->GetSelectedLabel();
          if (iSelectedMovie >= 0)
            url = movielist[iSelectedMovie];
          else if (!pDlgSelect->IsButtonPressed())
            return; // user backed out
        }
      }
    }
    // 4c. Check if url is still empty - occurs if user has selected to do a manual
    //     lookup, or if the IMDb lookup failed or was cancelled.
    if (url.m_strURL.IsEmpty())
    {
      // Check for cancel of the progress dialog
      pDlgProgress->Close();
      if (pDlgProgress->IsCanceled())
        return;

      // Prompt the user to input the movieName
      if (!CGUIDialogKeyboard::ShowAndGetInput(movieName, g_localizeStrings.Get(16009), false))
        return; // user backed out

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
      pDlgProgress->StartModal(GetID());
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
        if (item->m_strPath)
          m_database.SetMovieInfo(item->m_strPath, movieDetails);

        pDlgInfo->SetMovie(movieDetails, item->GetThumbnailImage());
        pDlgInfo->DoModal(GetID());
        item->SetThumbnailImage(pDlgInfo->GetThumbnail());
        needsRefresh = pDlgInfo->NeedRefresh();
      }
      else
      {
        pDlgProgress->Close();
        if (pDlgProgress->IsCanceled())
          return; // user cancelled
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
          pDlgOK->DoModal(GetID());
        }
        return;
      }
    }
    // 6. Check for a refresh
  } while (needsRefresh);
}

void CGUIWindowVideoBase::Render()
{
  CGUIMediaWindow::Render();

  if (m_bDisplayEmptyDatabaseMessage)
  {
    int iX = 400;
    int iY = 400;
    CGUIListControl *pControl = (CGUIListControl *)GetControl(CONTROL_LIST);
    if (pControl)
    {
      iX = pControl->GetXPosition() + pControl->GetWidth() / 2;
      iY = pControl->GetYPosition() + pControl->GetHeight() / 2;
    }
    CGUIFont *pFont = pControl->GetLabelInfo().font;
    if (pFont)
    {
      float fWidth, fHeight;
      CStdStringW utf16NoScannedInfo, utf16SwitchToFiles;
      g_charsetConverter.utf8ToUTF16(g_localizeStrings.Get(745), utf16NoScannedInfo); // "No scanned information for this view"
      g_charsetConverter.utf8ToUTF16(g_localizeStrings.Get(746), utf16SwitchToFiles); // "Switch back to Files view"
      pFont->GetTextExtent(utf16NoScannedInfo.c_str(), &fWidth, &fHeight);
      pFont->DrawText((float)iX, (float)iY - fHeight, 0xffffffff, 0, utf16NoScannedInfo.c_str(), XBFONT_CENTER_X | XBFONT_CENTER_Y);
      pFont->DrawText((float)iX, (float)iY + fHeight, 0xffffffff, 0, utf16SwitchToFiles.c_str(), XBFONT_CENTER_X | XBFONT_CENTER_Y);
    }
  }
}

void CGUIWindowVideoBase::OnManualIMDB()
{
  CStdString strInput;
  if (!CGUIDialogKeyboard::ShowAndGetInput(strInput, g_localizeStrings.Get(16009), false)) return ;

  CStdString strThumb;
  CUtil::GetThumbnail("Z:\\", strThumb);
  ::DeleteFile(strThumb.c_str());

  CFileItem item(strInput);
  ShowIMDB(&item);
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
  CFileItem movieFile(movieDetails.m_strPath, false);
  if ( !movieFile.IsOnDVD()) return true;
  CGUIDialogOK *pDlgOK = (CGUIDialogOK*)m_gWindowManager.GetWindow(WINDOW_DIALOG_OK);
  if (!pDlgOK) return true;
  while (1)
  {
    if (IsCorrectDiskInDrive(strFileName, movieDetails.m_strDVDLabel))
    {
      return true;
    }
    pDlgOK->SetHeading( 428);
    pDlgOK->SetLine( 0, 429 );
    pDlgOK->SetLine( 1, movieDetails.m_strDVDLabel );
    pDlgOK->SetLine( 2, "" );
    pDlgOK->DoModal( GetID() );
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

int  CGUIWindowVideoBase::GetResumeItemOffset(CFileItem *item)
{
  int startOffset = 0;
  if (item->IsStack())
  {
    CVideoSettings settings;
    if (m_database.GetVideoSettings(item->m_strPath, settings))
      return settings.m_ResumeTime;

    // TODO: Remove this code once the player can handle the stack:// protocol.
    // grab the database info.  If stacking is enabled, we may have to check multiple files...
    vector<CStdString> movies;
    GetStackedFiles(item->m_strPath, movies);
    for (unsigned int i=0; i < movies.size(); i++)
    {
      CVideoSettings settings;

      m_database.GetVideoSettings(movies[i], settings);

      if (settings.m_ResumeTime > 0)
      {
        startOffset = settings.m_ResumeTime + 0x10000000 * i; // assume no more than 16 files are stacked
        break;
      }
    }
  }
  else if (!item->IsNFO() && !item->IsPlayList())
  {
    CVideoSettings settings;
    m_database.GetVideoSettings(item->m_strPath, settings);
    startOffset = settings.m_ResumeTime;
  }
  return startOffset;
}

bool CGUIWindowVideoBase::OnClick(int iItem)
{
  if (g_guiSettings.GetBool("VideoPlayer.AutoResume"))
    OnResumeItem(iItem);
  else
    return CGUIMediaWindow::OnClick(iItem);
  
  return true;
}

void CGUIWindowVideoBase::OnResumeItem(int iItem)
{
  m_vecItems[iItem]->m_lStartOffset = GetResumeItemOffset(m_vecItems[iItem]);
  CGUIMediaWindow::OnClick(iItem);
}

void CGUIWindowVideoBase::OnPopupMenu(int iItem)
{
  if (iItem < 0 || iItem >= m_vecItems.Size()) return ;
  
  // calculate our position
  int iPosX = 200, iPosY = 100;
  CGUIListControl *pList = (CGUIListControl *)GetControl(CONTROL_LIST);
  if (pList)
  {
    iPosX = pList->GetXPosition() + pList->GetWidth() / 2;
    iPosY = pList->GetYPosition() + pList->GetHeight() / 2;
  }

  // mark the item
  bool bSelected = m_vecItems[iItem]->IsSelected(); // item maybe selected (playlistitem)
  m_vecItems[iItem]->Select(true);

  // popup the context menu
  CGUIDialogContextMenu *pMenu = (CGUIDialogContextMenu *)m_gWindowManager.GetWindow(WINDOW_DIALOG_CONTEXT_MENU);
  if (!pMenu) return ;

  // load our menu
  pMenu->Initialize();
  bool bIsGotoParent = m_vecItems[iItem]->IsParentFolder();

  int btn_Show_Info     = 0; // Show Video Information
  int btn_Resume        = 0; // Resume Video
  int btn_PlayWith      = 0; // Play
  int btn_Queue         = 0; // Add to Playlist

  // check what players we have
  VECPLAYERCORES vecCores;
  CPlayerCoreFactory::GetPlayers(*m_vecItems[iItem], vecCores);

  if (!bIsGotoParent)
  {
    // turn off the query info button if we are in playlists view
    if (GetID() != WINDOW_VIDEO_PLAYLIST)
      btn_Show_Info = pMenu->AddButton(13346);
    
    // check to see if the Resume Video button is applicable
    if (GetResumeItemOffset(m_vecItems[iItem]) > 0)               
      btn_Resume = pMenu->AddButton(13381);     // Resume Video

    if (vecCores.size() >= 1)
      btn_PlayWith = pMenu->AddButton(15213);
    // allow a folder to be ad-hoc queued and played by the default player
    else if (GetID() == WINDOW_VIDEOS && (m_vecItems[iItem]->m_bIsFolder || m_vecItems[iItem]->IsPlayList()))
      btn_PlayWith = pMenu->AddButton(208);

    // don't show the add to playlist button in playlist window
    if (GetID() != WINDOW_VIDEO_PLAYLIST)
      btn_Queue = pMenu->AddButton(13347);      // Add to Playlist
  }

  // turn off the now playing button if playlist is empty or if we are in playlist window
  int btn_Now_Playing = 0;                          
  if (GetID() != WINDOW_VIDEO_PLAYLIST && g_playlistPlayer.GetPlaylist(PLAYLIST_VIDEO).size() > 0)
    btn_Now_Playing = pMenu->AddButton(13350);    // Now Playing...

  // hide scan button unless we're in files window
  int btn_Query = 0;
  if (GetID() == WINDOW_VIDEOS)
    btn_Query = pMenu->AddButton(13349);            // Query Info For All Files

  int btn_Search_IMDb = 0;
  if (!bIsGotoParent)
    btn_Search_IMDb   = pMenu->AddButton(13348);  // Search IMDb...

  int btn_Mark_UnWatched = 0;
  int btn_Mark_Watched   = 0;
  int btn_Update_Title   = 0;
  //if (GetID() == WINDOW_VIDEO_TITLE || GetID() == WINDOW_VIDEO_GENRE || GetID() == WINDOW_VIDEO_ACTOR || GetID() == WINDOW_VIDEO_YEAR)
  // is the item a database movie?
  if (GetID() != WINDOW_VIDEOS && !m_vecItems[iItem]->m_musicInfoTag.GetURL().IsEmpty())
  {
    // uses Loaded to hold Watched/UnWatched status
    if (m_vecItems[iItem]->m_musicInfoTag.Loaded())
      btn_Mark_UnWatched = pMenu->AddButton(16104); //Mark as UnWatched
    else
      btn_Mark_Watched = pMenu->AddButton(16103);   //Mark as Watched

    /*
	  if (m_iShowMode == VIDEO_SHOW_ALL)
	  {
      btn_Mark_Watched = pMenu->AddButton(16103);   //Mark as Watched
      btn_Mark_UnWatched = pMenu->AddButton(16104); //Mark as UnWatched
	  }
	  else if (m_iShowMode == VIDEO_SHOW_UNWATCHED)
	  {
      btn_Mark_Watched = pMenu->AddButton(16103); //Mark as Watched
	  }
    else if (m_iShowMode == VIDEO_SHOW_WATCHED)
    {
      btn_Mark_UnWatched = pMenu->AddButton(16104); //Mark as UnWatched
    }
    */

    btn_Update_Title = pMenu->AddButton(16105); //Edit Title
  }
  
  // hide delete button unless enabled, or in title window
  int btn_Delete = 0;
  int btn_Rename = 0;
  if (!bIsGotoParent)
  {
    if ((m_vecItems.m_strPath.Equals(CUtil::VideoPlaylistsLocation())) || (GetID() == WINDOW_VIDEOS && g_guiSettings.GetBool("VideoFiles.AllowFileDeletion")))
    {
      btn_Delete = pMenu->AddButton(117);
      btn_Rename = pMenu->AddButton(118);

      // disable these functions if not supported by the protocol
      if (!CUtil::SupportsFileOperations(m_vecItems[iItem]->m_strPath))
      {
        pMenu->EnableButton(btn_Delete, false);
        pMenu->EnableButton(btn_Rename, false);
      }
    }
    if (GetID() == WINDOW_VIDEO_TITLE)
      btn_Delete = pMenu->AddButton(646);
  }

  // GeminiServer Todo: Set a MasterLock Option to Enable or disable Settings incontext menu!
  int btn_Settings      = pMenu->AddButton(5);      // Settings

  // position it correctly
  pMenu->SetPosition(iPosX - pMenu->GetWidth() / 2, iPosY - pMenu->GetHeight() / 2);
  pMenu->DoModal(GetID());
  
  int btnid = pMenu->GetButton();
  if (btnid>0)
  {
    if (btnid == btn_Show_Info)
    {
      OnInfo(iItem);
    }
    else if (btnid == btn_Resume)
    {
      OnResumeItem(iItem);
    }
    else if (btnid == btn_PlayWith)
    {
      // if folder, play with default player
      if (m_vecItems[iItem]->m_bIsFolder)
      {
        PlayItem(iItem);
      }
      else
      {
        g_application.m_eForcedNextPlayer = CPlayerCoreFactory::SelectPlayerDialog(vecCores, iPosX, iPosY);
        if( g_application.m_eForcedNextPlayer != EPC_NONE )
          OnClick(iItem);
      }
    }
    else if (btnid == btn_Queue)
    {
      OnQueueItem(iItem);
    }
    else if (btnid ==  btn_Now_Playing)
    {
      m_gWindowManager.ActivateWindow(WINDOW_VIDEO_PLAYLIST);
      return;
    }
    else if (btnid == btn_Query)
    {
      OnScan();
    }
    else if (btnid == btn_Search_IMDb)
    {
      OnManualIMDB();
    }
    else if (btnid == btn_Mark_UnWatched)
	  {
		  MarkUnWatched(iItem);
		  Update(m_vecItems.m_strPath);
	  }
	  else if (btnid == btn_Mark_Watched)
	  {
		  MarkWatched(iItem);
		  Update(m_vecItems.m_strPath);
	  }
	  else if (btnid == btn_Update_Title)
	  {
		  UpdateVideoTitle(iItem);
		  Update(m_vecItems.m_strPath);
	  }
    else if (btnid == btn_Settings)
    {
      //MasterPassword
      int iLockSettings = g_guiSettings.GetInt("Masterlock.LockSettingsFilemanager");
      if (iLockSettings == 1 || iLockSettings == 3) 
      {
        if (g_passwordManager.IsMasterLockLocked(true))
          m_gWindowManager.ActivateWindow(WINDOW_SETTINGS_MYVIDEOS);
      }
      else m_gWindowManager.ActivateWindow(WINDOW_SETTINGS_MYVIDEOS); 
      return;
    }
    else if (btnid == btn_Delete)
    {
      OnDeleteItem(iItem);
    }
    else if (btnid == btn_Rename)
    {
      OnRenameItem(iItem);
    }
  }
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

  PlayMovie(pItem);

  return true;
}

void CGUIWindowVideoBase::PlayMovie(const CFileItem *item)
{
  CFileItemList movieList;
  int selectedFile = 1;
  // VideoPlayer.BypassCDSelection values:
  // 0 = never
  // 1 = immediately
  // 2-37 = 5-180 seconds
  if (item->IsStack() && g_guiSettings.GetInt("VideoPlayer.BypassCDSelection") != 1)
  {
    // TODO: Once the players are capable of playing a stack, we should remove
    // this code in favour of just using the resume feature.
    CStdStringArray movies;
    GetStackedFiles(item->m_strPath, movies);
    if (item->m_lStartOffset)
      selectedFile = ((item->m_lStartOffset & 0x10000000) >> 28) + 1;
    else
    { // show file stacking dialog
      CGUIDialogFileStacking* dlg = (CGUIDialogFileStacking*)m_gWindowManager.GetWindow(WINDOW_DIALOG_FILESTACKING);
      if (dlg)
      {
        dlg->SetNumberOfFiles(movies.size());
        dlg->DoModal(GetID());
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
      playlistItem.m_lStartOffset = item->m_lStartOffset & 0x0fffffff;
    playlist.Add(playlistItem);
  }

  // play movie...
  g_playlistPlayer.PlayNext();
}

void CGUIWindowVideoBase::OnDeleteItem(int iItem)
{
  if ( iItem < 0 || iItem >= m_vecItems.Size()) return;
  if (!CGUIWindowFileManager::DeleteItem(m_vecItems[iItem]))
    return;
  Update(m_vecItems.m_strPath);
  m_viewControl.SetSelectedItem(iItem);
}

void CGUIWindowVideoBase::OnRenameItem(int iItem)
{
  if ( iItem < 0 || iItem >= m_vecItems.Size()) return;
  if (!CGUIWindowFileManager::RenameFile(m_vecItems[iItem]->m_strPath))
    return;
  Update(m_vecItems.m_strPath);
  m_viewControl.SetSelectedItem(iItem);
}

void CGUIWindowVideoBase::MarkUnWatched(int iItem)
{
  if ( iItem < 0 || iItem >= m_vecItems.Size() ) return ;
  CFileItem* pItem = m_vecItems[iItem];
  m_database.MarkAsUnWatched(atol(pItem->m_musicInfoTag.GetURL()));
  Update(m_vecItems.m_strPath);
}

//Add Mark a Title as watched
void CGUIWindowVideoBase::MarkWatched(int iItem)
{
  if ( iItem < 0 || iItem >= m_vecItems.Size() ) return ;
  CFileItem* pItem = m_vecItems[iItem];
  m_database.MarkAsWatched(atol(pItem->m_musicInfoTag.GetURL()));
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
  Update(m_vecItems.m_strPath);
}

bool CGUIWindowVideoBase::UpdateVideoTitleXML(const CStdString strIMDBNumber, CStdString& strTitle)
{
  CStdString strXMLFile;
  strXMLFile.Format("%s\\imdb\\%s.xml", g_stSettings.m_szAlbumDirectory, strIMDBNumber.c_str());
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
      m_guiState->SetPlaylistDirectory("");
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
  else if (pItem->IsPlayList())
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
    CUtil::ReplaceExtension(firstFile, ".nfo", nfoFile);
    // else try .nfo file matching stacked title
    if (!CFile::Exists(nfoFile))
    {
      CStdString stackedTitlePath = dir.GetStackedTitlePath(item->m_strPath);
      CUtil::ReplaceExtension(stackedTitlePath, ".nfo", nfoFile);
      if (!CFile::Exists(nfoFile))
        nfoFile.Empty();
    }
  }

  return nfoFile;
}

void CGUIWindowVideoBase::SetDatabaseDirectory(const VECMOVIES &movies, CFileItemList &items)
{
  for (int i = 0; i < (int)movies.size(); ++i)
  {
    CIMDBMovie movie = movies[i];
    // add the appropiate movies to m_vecItems based on the showmode
    if (
      (m_iShowMode == VIDEO_SHOW_ALL) ||
      (m_iShowMode == VIDEO_SHOW_WATCHED && movie.m_bWatched == true) ||
      (m_iShowMode == VIDEO_SHOW_UNWATCHED && movie.m_bWatched == false)
      )
    {
      // mark watched movies when showing all
      CStdString strTitle = movie.m_strTitle;
      CFileItem *pItem = new CFileItem(strTitle);
      pItem->m_strTitle=strTitle;
      pItem->m_strPath = movie.m_strFileNameAndPath;
      pItem->m_bIsFolder = false;
      pItem->m_bIsShareOrDrive = false;
      pItem->SetThumb();
      
      if (!pItem->HasThumbnail())
      {
        CStdString strThumb;
        CUtil::GetVideoThumbnail(movie.m_strIMDBNumber, strThumb);
        if (CFile::Exists(strThumb))
          pItem->SetThumbnailImage(strThumb);
      }
      pItem->m_fRating = movie.m_fRating;
      SYSTEMTIME time;
      time.wYear = movie.m_iYear;
      pItem->m_musicInfoTag.SetReleaseDate(time);
      pItem->m_strDVDLabel = movie.m_strDVDLabel;
      pItem->SetOverlayImage(CGUIListItem::ICON_OVERLAY_UNWATCHED,movie.m_bWatched);

      // Hack for extra info
      pItem->m_musicInfoTag.SetTitle(movie.m_strTitle);
      pItem->m_musicInfoTag.SetArtist(movie.m_strDirector);
      pItem->m_musicInfoTag.SetGenre(movie.m_strGenre);
      pItem->m_musicInfoTag.SetURL(movie.m_strSearchString);
      pItem->m_musicInfoTag.SetLoaded(movie.m_bWatched);
      // End hack for extra info

      m_vecItems.Add(pItem);
    }
  }
}

void CGUIWindowVideoBase::ApplyIMDBThumbToFolder(const CStdString &folder, const CStdString &imdbThumb)
{
  // copy icon to folder also;
  if (CFile::Exists(imdbThumb))
  {
    CStdString strFolderImage;
    CUtil::AddFileToFolder(folder, "folder.jpg", strFolderImage);
    CFileItem folderItem(folder, true);
    if (folderItem.IsRemote() || folderItem.IsOnDVD())
    {
      CStdString strThumb;
      CUtil::GetThumbnail( strFolderImage, strThumb);
      CFile::Cache(imdbThumb.c_str(), strThumb.c_str(), NULL, NULL);
    }
    else
    {
      CFile::Cache(imdbThumb.c_str(), strFolderImage.c_str(), NULL, NULL);
    }
  }
}

void CGUIWindowVideoBase::SetIMDBThumb(CFileItem *item, const CStdString &imdbNumber)
{
  if (!item->m_bIsFolder && !item->HasThumbnail())
  {
    CStdString strThumb;
    CUtil::GetVideoThumbnail(imdbNumber, strThumb);
    if (CFile::Exists(strThumb))
      item->SetThumbnailImage(strThumb);
  }
}

