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
#include "GUIFontManager.h"
#include "FileSystem/ZipManager.h"
#include "FileSystem/StackDirectory.h"
#include "GUIDialogContextMenu.h"
#include "GUIDialogFileStacking.h"
#include "GUIWindowFileManager.h"

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
      int iLastControl = m_iLastControl;
      CGUIWindow::OnMessage(message);

      m_database.Open();
      m_dlgProgress = (CGUIDialogProgress*)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
      
      m_rootDir.SetMask(g_stSettings.m_szMyVideoExtensions);
      m_rootDir.SetShares(g_settings.m_vecMyVideoShares);

      Update(m_vecItems.m_strPath);

      if (iLastControl > -1)
      {
        SET_CONTROL_FOCUS(iLastControl, 0);
      }
      else
      {
        SET_CONTROL_FOCUS(m_dwDefaultFocusControlID, 0);
      }

      if (m_iSelectedItem >= 0)
      {
        m_viewControl.SetSelectedItem(m_iSelectedItem);
      }
      return true;
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
  case GUI_MSG_NOTIFY_ALL:
    { // Message is received even if this window is inactive
      
      //  Is there a dvd share in this window?
      if (!m_rootDir.GetDVDDriveUrl().IsEmpty())
      {
        if (message.GetParam1()==GUI_MSG_DVDDRIVE_EJECTED_CD)
        {
          if (m_vecItems.IsVirtualDirectoryRoot() && IsActive())
          {
            int iItem = m_viewControl.GetSelectedItem();
            Update(m_vecItems.m_strPath);
            m_viewControl.SetSelectedItem(iItem);
          }
          else if (m_vecItems.IsCDDA() || m_vecItems.IsOnDVD())
          { // Disc has changed and we are inside a DVD Drive share, get out of here :)
            if (IsActive()) Update("");
            else 
            {
              m_vecPathHistory.clear();
              m_vecItems.m_strPath="";
            }
          }

          return true;
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

void CGUIWindowVideoBase::OnSort()
{
  FormatItemLabels();
  // Save current item selection
  CStdString currentItem = "";
  int nItem = m_viewControl.GetSelectedItem();
  if (nItem >= 0)
    currentItem = m_vecItems[nItem]->m_strPath;

  // sort and update the view
  SortItems(m_vecItems);
  m_viewControl.SetItems(m_vecItems);

  // restore current item selection
  for (int i = 0; i < (int)m_vecItems.Size(); i++)
  {
    CFileItem* pItem = m_vecItems[i];
    if (pItem->m_strPath == currentItem)
    {
      m_viewControl.SetSelectedItem(i);
      return;
    }
  }
}

void CGUIWindowVideoBase::OnInfo(int iItem)
{
  if ( iItem < 0 || iItem >= m_vecItems.Size() ) return ;
  CFileItem* pItem = m_vecItems[iItem];
  CStdString strFilePath;
  m_database.GetFilePath(atol(pItem->m_strPath), strFilePath);
  if (strFilePath.IsEmpty()) return;
  CStdString strFile = CUtil::GetFileName(strFilePath);
  ShowIMDB(strFile, strFilePath, "" , false);
}

void CGUIWindowVideoBase::ShowIMDB(const CStdString& strMovie, const CStdString& strFile, const CStdString& strFolder, bool bFolder)
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
  CStdString strMovieName = strMovie;

  if (m_database.HasMovieInfo(strFile))
  {
    CIMDBMovie movieDetails;
    m_database.GetMovieInfo(strFile, movieDetails);
    pDlgInfo->SetMovie(movieDetails);
    pDlgInfo->DoModal(GetID());
    if ( !pDlgInfo->NeedRefresh() ) return ;

    // quietly return if Internet lookups are disabled
    if (!g_guiSettings.GetBool("Network.EnableInternet")) return ;

    m_database.DeleteMovieInfo(strFile);
  }

  // quietly return if Internet lookups are disabled
  if (!g_guiSettings.GetBool("Network.EnableInternet")) return ;

  // handle .nfo files
  CStdString strExtension, strNfoFile;
  CUtil::GetExtension(strFile, strExtension);

  // already an .nfo file?
  if ( strcmpi(strExtension.c_str(), ".nfo") == 0 )
    strNfoFile = strFile;
  // no, create .nfo file
  else
    CUtil::ReplaceExtension(strFile, ".nfo", strNfoFile);

  // test file existance
  if (!strNfoFile.IsEmpty() && !CFile::Exists(strNfoFile))
      strNfoFile.Empty();

  // try looking for .nfo file for a stacked item
  CURL url(strFile);
  if (url.GetProtocol() == "stack")
  {
    // first try .nfo file matching first file in stack
    CStackDirectory dir;
    CStdString firstFile = dir.GetFirstStackedFile(strFile);
    CUtil::ReplaceExtension(firstFile, ".nfo", strNfoFile);
    // else try .nfo file matching stacked title
    if (!CFile::Exists(strNfoFile))
    {
      CStdString stackedTitlePath = dir.GetStackedTitlePath(strFile);
      CUtil::ReplaceExtension(stackedTitlePath, ".nfo", strNfoFile);
      if (!CFile::Exists(strNfoFile))
        strNfoFile.Empty();
    }
  }

  if ( !strNfoFile.IsEmpty() )
  {
    CLog::Log(LOGDEBUG,"Found matching nfo file: %s", strNfoFile.c_str());
    if ( CFile::Cache(strNfoFile, "Z:\\movie.nfo", NULL, NULL))
    {
      CNfoFile nfoReader;
      if ( nfoReader.Create("Z:\\movie.nfo") == S_OK)
      {
        CIMDBUrl url;
        CIMDBMovie movieDetails;
        url.m_strURL = nfoReader.m_strImDbUrl;
        //url.m_strURL.push_back(nfoReader.m_strImDbUrl);
        CLog::Log(LOGDEBUG,"-- imdb url: %s", url.m_strURL.c_str());

        // show dialog that we're downloading the movie info
        pDlgProgress->SetHeading(198);
        pDlgProgress->SetLine(0, strMovieName);
        pDlgProgress->SetLine(1, url.m_strTitle);
        pDlgProgress->SetLine(2, "");
        pDlgProgress->StartModal(GetID());
        pDlgProgress->Progress();

        if ( IMDB.GetDetails(url, movieDetails, pDlgProgress) )
        {
          m_database.SetMovieInfo(strFile, movieDetails);
          // now show the imdb info
          pDlgProgress->Close();
          pDlgInfo->SetMovie(movieDetails);
          pDlgInfo->DoModal(GetID());
          bFound = true;
          bUpdate = true;
          if ( !pDlgInfo->NeedRefresh() ) return ;
          m_database.DeleteMovieInfo(strFile);
        }
      }
      else
        CLog::Log(LOGERROR,"Unable to find an imdb url in nfo file: %s", strNfoFile.c_str());
    }
    else
      CLog::Log(LOGERROR,"Unable to cache nfo file: %s", strNfoFile.c_str());
  }

  if (!g_guiSettings.GetBool("FileLists.HideExtensions") && !bFolder)
    CUtil::RemoveExtension(strMovieName);

  bool bContinue;
  do
  {
    bContinue = false;
    if (!bFound)
    {
      // show dialog that we're busy querying www.imdb.com
      pDlgProgress->SetHeading(197);
      pDlgProgress->SetLine(0, strMovieName);
      pDlgProgress->SetLine(1, "");
      pDlgProgress->SetLine(2, "");
      pDlgProgress->StartModal(GetID());
      pDlgProgress->Progress();

      bool bError = true;


      OutputDebugString("query imdb\n");
      IMDB_MOVIELIST movielist;
      if (IMDB.FindMovie(strMovieName, movielist, pDlgProgress))
      {
        pDlgProgress->Close();

        int iMoviesFound = movielist.size();
        if (iMoviesFound > 0)
        {
          OutputDebugString("found 1 or more movies\n");
          int iSelectedMovie = 0;
          if (iMoviesFound > 0)  // always ask user to select (allows manual lookup)
          {
            // more then 1 movie found
            // ask user to select 1
//            OutputDebugString("found more then 1 movie\n");
            pDlgSelect->SetHeading(196);
            pDlgSelect->Reset();
            for (int i = 0; i < iMoviesFound; ++i)
            {
              CIMDBUrl url = movielist[i];
              pDlgSelect->Add(url.m_strTitle);
            }
            pDlgSelect->EnableButton(true);
            pDlgSelect->SetButtonLabel(413); // manual

            pDlgSelect->DoModal(GetID());

            // and wait till user selects one
            iSelectedMovie = pDlgSelect->GetSelectedLabel();
            if (iSelectedMovie < 0)
            {
              if (!pDlgSelect->IsButtonPressed()) return ;
              if (!CGUIDialogKeyboard::ShowAndGetInput(strMovieName, (CStdStringW)g_localizeStrings.Get(16009), false)) return ;
              bContinue = true;
              bError = false;
            }
          }

          if (iSelectedMovie >= 0)
          {
            OutputDebugString("get details\n");
            CIMDBMovie movieDetails;
            movieDetails.m_strSearchString = strFile;
            CIMDBUrl& url = movielist[iSelectedMovie];

            // show dialog that we're downloading the movie info
            pDlgProgress->SetHeading(198);
            pDlgProgress->SetLine(0, strMovieName);
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
              bError = false;

              // now show the imdb info
              OutputDebugString("show info\n");
              m_database.SetMovieInfo(strFile, movieDetails);
              pDlgInfo->SetMovie(movieDetails);
              pDlgInfo->DoModal(GetID());
              if (!pDlgInfo->NeedRefresh())
              {
                bUpdate = true;
                if (bFolder)
                {
                  // copy icon to folder also;
                  CStdString strThumbOrg;
                  CUtil::GetVideoThumbnail(movieDetails.m_strIMDBNumber, strThumbOrg);
                  if (CFile::Exists(strThumbOrg))
                  {
                    CStdString strFolderImage;
                    CUtil::AddFileToFolder(strFolder, "folder.jpg", strFolderImage);
                    CFileItem folder(strFolder, true);
                    if (folder.IsRemote() || folder.IsOnDVD())
                    {
                      CStdString strThumb;
                      CUtil::GetThumbnail( strFolderImage, strThumb);
                      CFile::Cache(strThumbOrg.c_str(), strThumb.c_str(), NULL, NULL);
                    }
                    else
                    {
                      CFile::Cache(strThumbOrg.c_str(), strFolderImage.c_str(), NULL, NULL);
                    }
                  }
                }
              }
              else
              {
                bContinue = true;
                strMovieName = strMovie;
              }
            }
            else
            {
              OutputDebugString("failed to get details\n");
              pDlgProgress->Close();
              bError = !pDlgProgress->IsCanceled();
            }
          }
        }
        else
        {
          pDlgProgress->Close();
          if (!CGUIDialogKeyboard::ShowAndGetInput(strMovieName, (CStdStringW)g_localizeStrings.Get(16009), false)) return ;
          bContinue = true;
          bError = false;
        }
      }
      else
      {
        pDlgProgress->Close();
        if (pDlgProgress->IsCanceled()) return;
        if (!CGUIDialogKeyboard::ShowAndGetInput(strMovieName, (CStdStringW)g_localizeStrings.Get(16009), false)) return ;
        bContinue = true;
        bError = false;
      }

      if (bError)
      {
        // show dialog...
        CGUIDialogOK *pDlgOK = (CGUIDialogOK*)m_gWindowManager.GetWindow(WINDOW_DIALOG_OK);
        if (pDlgOK)
        {
          pDlgOK->SetHeading(195);
          pDlgOK->SetLine(0, strMovieName);
          pDlgOK->SetLine(1, L"");
          pDlgOK->SetLine(2, L"");
          pDlgOK->SetLine(3, L"");
          pDlgOK->DoModal(GetID());
        }
      }
    }
  }
  while (bContinue);

  if (bUpdate)
  {
    int iSelectedItem = m_viewControl.GetSelectedItem();

    // Refresh all items
    for (int i = 0; i < m_vecItems.Size(); ++i)
    {
      CFileItem* pItem = m_vecItems[i];
      pItem->FreeIcons();
    }

    m_vecItems.SetThumbs();
    SetIMDBThumbs(m_vecItems);
    m_vecItems.FillInDefaultIcons();
    UpdateButtons();
  }
}

void CGUIWindowVideoBase::Render()
{
  CGUIWindow::Render();
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
    CGUIFont *pFont = g_fontManager.GetFont(pControl->GetFontName());
    if (pFont)
    {
      float fWidth, fHeight;
      CStdStringW wszText = g_localizeStrings.Get(745); // "No scanned information for this view"
      CStdStringW wszText2 = g_localizeStrings.Get(746); // "Switch back to Files view"
      pFont->GetTextExtent(wszText, &fWidth, &fHeight);
      pFont->DrawText((float)iX, (float)iY - fHeight, 0xffffffff, wszText.c_str(), XBFONT_CENTER_X | XBFONT_CENTER_Y);
      pFont->DrawText((float)iX, (float)iY + fHeight, 0xffffffff, wszText2.c_str(), XBFONT_CENTER_X | XBFONT_CENTER_Y);
    }
  }
}

void CGUIWindowVideoBase::GoParentFolder()
{
  // remove current directory if its on the stack
  if (m_vecPathHistory.size() > 0)
  {
    if (m_vecPathHistory.back() == m_vecItems.m_strPath)
      m_vecPathHistory.pop_back();
  }
  // if vector is not empty, pop parent
  // if vector is empty, parent is bookmark listing
  CStdString strParent = "";
  if (m_vecPathHistory.size() > 0)
  {
    strParent = m_vecPathHistory.back();
    m_vecPathHistory.pop_back();
  }
  CLog::Log(LOGDEBUG,"CGUIWindowVideoBase::GoParentFolder(), strParent = [%s]", strParent.c_str());

  CURL url(m_vecItems.m_strPath);
  // if we treat stacks as directories, then use this
  /*if (url.GetProtocol() == "stack")
  {
    m_rootDir.RemoveShare(m_vecItems.m_strPath);
    CUtil::GetDirectory(m_vecItems.m_strPath.Mid(8), m_strParentPath);
  }*/
  if ((url.GetProtocol() == "rar") || (url.GetProtocol() == "zip")) 
  {
    // check for step-below, if, unmount rar
    if (url.GetFileName().IsEmpty())
    {
      if (url.GetProtocol() == "zip")
        g_ZipManager.release(m_vecItems.m_strPath); // release resources
      m_rootDir.RemoveShare(m_vecItems.m_strPath);
      CStdString strPath;
      CUtil::GetDirectory(url.GetHostName(),strPath);
      Update(strPath);
      UpdateButtons();
      return;
    }
  }

  CStdString strOldPath = m_vecItems.m_strPath;
  Update(strParent);
  UpdateButtons();  // not sure why this is required in my videos, but not in music or pictures

  if (!g_guiSettings.GetBool("LookAndFeel.FullDirectoryHistory"))
    m_history.Remove(strOldPath); //Delete current path
}

void CGUIWindowVideoBase::OnManualIMDB()
{
  CStdString strInput;
  if (!CGUIDialogKeyboard::ShowAndGetInput(strInput, (CStdStringW)g_localizeStrings.Get(16009), false)) return ;

  CStdString strThumb;
  CUtil::GetThumbnail("Z:\\", strThumb);
  ::DeleteFile(strThumb.c_str());

  ShowIMDB(strInput, "Z:\\", "Z:\\", false);
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
    pDlgOK->SetLine( 2, L"" );
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

  if (CUtil::IsNaturalNumber(movieItem.m_strPath))
  {
    CStdString filePath;
    m_database.GetFilePath(atol(movieItem.m_strPath), movieItem.m_strPath);
  }
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

int  CGUIWindowVideoBase::ResumeItemOffset(int iItem)
{
  int startOffset = 0;
  if (m_vecItems[iItem]->IsStack())
  {
    CVideoSettings settings;
    if (m_database.GetVideoSettings(m_vecItems[iItem]->m_strPath, settings))
      return settings.m_ResumeTime;

    // TODO: Remove this code once the player can handle the stack:// protocol.
    // grab the database info.  If stacking is enabled, we may have to check multiple files...
    vector<CStdString> movies;
    GetStackedFiles(m_vecItems[iItem]->m_strPath, movies);
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
  else if (!m_vecItems[iItem]->IsNFO() && !m_vecItems[iItem]->IsPlayList())
  {
    CVideoSettings settings;
    m_database.GetVideoSettings(m_vecItems[iItem]->m_strPath, settings);
    startOffset = settings.m_ResumeTime;
  }
  return startOffset;
}

void CGUIWindowVideoBase::OnResumeItem(int iItem)
{
  m_vecItems[iItem]->m_lStartOffset = ResumeItemOffset(iItem);
  OnClick(iItem);
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

  int btn_Show_Info     = 0; // Show Video Information
  int btn_Resume        = 0; // Resume Video
  int btn_Queue         = 0; // Add to Playlist
  bool bIsGotoParent = m_vecItems[iItem]->IsParentFolder();
  if (!bIsGotoParent)
  {
    // turn off the query info button if we are in playlists view
    if (GetID() != WINDOW_VIDEO_PLAYLIST || GetID() == WINDOW_VIDEOS && m_vecItems[iItem]->m_bIsFolder)
      btn_Show_Info = pMenu->AddButton(13346);  // Show Video Information
    
    // check to see if the Resume Video button is applicable
    if(ResumeItemOffset(iItem)>0)               
      btn_Resume = pMenu->AddButton(13381);     // Resume Video
    
    // don't show the add to playlist button in playlist window
    if (GetID() != WINDOW_VIDEO_PLAYLIST )
      btn_Queue = pMenu->AddButton(13347);      // Add to Playlist
  }

  // check what players we have
  VECPLAYERCORES vecCores;
  CPlayerCoreFactory::GetPlayers(*m_vecItems[iItem], vecCores);
  int btn_PlayWith  = 0;
  if( vecCores.size() >= 1 ) btn_PlayWith = pMenu->AddButton(15213);

  // turn off the now playing button if playlist is empty or if we are in playlist window
  int btn_Now_Playing = 0;                          
  if ((GetID() != WINDOW_VIDEO_PLAYLIST ) && (g_playlistPlayer.GetPlaylist(PLAYLIST_VIDEO).size() > 0) )
    btn_Now_Playing   = pMenu->AddButton(13350);    // Now Playing...

  // hide scan button unless we're in files window
  int btn_Query = 0;
  if (GetID() == WINDOW_VIDEOS)
    btn_Query = pMenu->AddButton(13349);            // Query Info For All Files

  int btn_Search_IMDb = 0;
  if (!bIsGotoParent)
    btn_Search_IMDb   = pMenu->AddButton(13348);  // Search IMDb...

  int btn_Mark_UnWatched = 0;
  int btn_Mark_Watched = 0;
  int btn_Update_Title = 0;
  if (GetID() == WINDOW_VIDEO_TITLE || GetID() == WINDOW_VIDEO_GENRE || GetID() == WINDOW_VIDEO_ACTOR || GetID() == WINDOW_VIDEO_YEAR)
  {
	  if (m_iShowMode == VIDEO_SHOW_ALL)
	  {
      btn_Mark_Watched = pMenu->AddButton(16103); //Mark as Watched
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
    btn_Update_Title = pMenu->AddButton(16105); //Edit Title
  }
  
  // hide delete button unless enabled, or in title window
  int btn_Delete = 0, btn_Rename = 0;
  if (!bIsGotoParent)
  {
    if ((m_vecItems.m_strPath.Equals(CUtil::VideoPlaylistsLocation())) || (GetID() == WINDOW_VIDEOS && g_guiSettings.GetBool("VideoFiles.AllowFileDeletion")))
    {
      btn_Delete = pMenu->AddButton(117);             // Delete
      btn_Rename = pMenu->AddButton(118);             // Rename
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
    else if (btnid == btn_PlayWith)
    {
      g_application.m_eForcedNextPlayer = CPlayerCoreFactory::SelectPlayerDialog(vecCores, iPosX, iPosY);
      if( g_application.m_eForcedNextPlayer != EPC_NONE )
        OnClick(iItem);
    }
  }
  m_vecItems[iItem]->Select(bSelected);
}

void CGUIWindowVideoBase::GetStackedFiles(const CStdString &strFilePath1, vector<CStdString> &movies)
{
  CStdString strFilePath = strFilePath1;  // we're gonna be altering it

  if (CUtil::IsNaturalNumber(strFilePath))
  { // we have a database view
    m_database.GetFilePath(atol(strFilePath.c_str()), strFilePath);
  }

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

void CGUIWindowVideoBase::PlayMovie(const CFileItem *item)
{
  CFileItem movie(*item);
  vector<CStdString> movieList;
  int selectedFile = 1;
  if (CUtil::IsNaturalNumber(movie.m_strPath))
  { // database file - get the true path
    m_database.GetFilePath(atol(movie.m_strPath), movie.m_strPath);
  }
  // VideoPlayer.BypassCDSelection values:
  // 0 = never
  // 1 = immediately
  // 2-37 = 5-180 seconds
  if (movie.IsStack() && g_guiSettings.GetInt("VideoPlayer.BypassCDSelection") != 1)
  {
    // TODO: Once the players are capable of playing a stack, we should remove
    // this code in favour of just using the resume feature.
    GetStackedFiles(movie.m_strPath, movieList);
    if (movie.m_lStartOffset)
      selectedFile = ((movie.m_lStartOffset & 0x10000000) >> 28) + 1;
    else
    { // show file stacking dialog
      CGUIDialogFileStacking* dlg = (CGUIDialogFileStacking*)m_gWindowManager.GetWindow(WINDOW_DIALOG_FILESTACKING);
      if (dlg)
      {
        dlg->SetNumberOfFiles(movieList.size());
        dlg->DoModal(GetID());
        selectedFile = dlg->GetSelectedFile();
        if (selectedFile < 1) return ;
      }
    }
  }
  else
    movieList.push_back(movie.m_strPath);

  g_playlistPlayer.Reset();
  g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_VIDEO_TEMP);
  CPlayList& playlist = g_playlistPlayer.GetPlaylist(PLAYLIST_VIDEO_TEMP);
  playlist.Clear();
  for (int i = selectedFile - 1; i < (int)movieList.size(); ++i)
  {
    CPlayList::CPlayListItem item;
    item.SetFileName(movieList[i]);
    if (i == selectedFile - 1)
      item.m_lStartOffset = movie.m_lStartOffset & 0x0fffffff;
    playlist.Add(item);
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

void CGUIWindowVideoBase::ShowShareErrorMessage(CFileItem* pItem)
{
  if (pItem->m_bIsShareOrDrive)
  {
    int idMessageText=0;
    CURL url(pItem->m_strPath);
    const CStdString& strHostName=url.GetHostName();

    if (pItem->m_iDriveType!=SHARE_TYPE_REMOTE) //  Local shares incl. dvd drive
      idMessageText=15300;
    else if (url.GetProtocol()=="xbms" && strHostName.IsEmpty()) //  xbms server discover
      idMessageText=15302;
    else if (url.GetProtocol()=="smb" && strHostName.IsEmpty()) //  smb workgroup
      idMessageText=15303;
    else  //  All other remote shares
      idMessageText=15301;

    CGUIDialogOK::ShowAndGetInput(220, idMessageText, 0, 0);
  }
}

void CGUIWindowVideoBase::MarkUnWatched(int iItem)
{
  if ( iItem < 0 || iItem >= m_vecItems.Size() ) return ;
  CFileItem* pItem = m_vecItems[iItem];
  m_database.MarkAsUnWatched(atol(pItem->m_strPath));
  Update(m_vecItems.m_strPath);
}

//Add Mark a Title as watched
void CGUIWindowVideoBase::MarkWatched(int iItem)
{
  if ( iItem < 0 || iItem >= m_vecItems.Size() ) return ;
  CFileItem* pItem = m_vecItems[iItem];
  m_database.MarkAsWatched(atol(pItem->m_strPath));
  Update(m_vecItems.m_strPath);
}

//Add change a title's name
void CGUIWindowVideoBase::UpdateVideoTitle(int iItem)
{
  if ( iItem < 0 || iItem >= m_vecItems.Size() ) return ;
  CFileItem* pItem = m_vecItems[iItem];

  //Get Current Name
  CIMDBMovie detail;
  m_database.GetMovieInfo(L"", detail, atol(pItem->m_strPath));
  CStdString strInput;
  strInput = detail.m_strTitle;

  //Get the new title
  if (!CGUIDialogKeyboard::ShowAndGetInput(strInput, (CStdStringW)g_localizeStrings.Get(16105), false)) return ;
  m_database.UpdateMovieTitle(atol(pItem->m_strPath), strInput);
  Update(m_vecItems.m_strPath);
}
