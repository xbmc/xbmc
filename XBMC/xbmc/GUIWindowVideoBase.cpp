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
#include "AutoSwitch.h"
#include "GUIFontManager.h"

#define CONTROL_BTNVIEWASICONS  2
#define CONTROL_BTNSORTBY     3
#define CONTROL_BTNSORTASC    4
#define CONTROL_BTNTYPE           5
#define CONTROL_PLAY_DVD          6
#define CONTROL_STACK             7
#define CONTROL_BTNSCAN           8
#define CONTROL_IMDB              9
#define CONTROL_LIST       50
#define CONTROL_THUMBS      51
#define CONTROL_LABELFILES         12

struct SSortVideoListByName
{
  bool operator()(CStdString& strItem1, CStdString& strItem2)
  {
    return StringUtils::AlphaNumericCompare(strItem1.c_str(), strItem2.c_str());
  }
};

struct SSortVideoByName
{
  bool operator()(CFileItem* pStart, CFileItem* pEnd)
  {
    CFileItem& rpStart = *pStart;
    CFileItem& rpEnd = *pEnd;
    if (rpStart.GetLabel() == "..") return true;
    if (rpEnd.GetLabel() == "..") return false;
    bool bGreater = true;
    if (m_bSortAscending) bGreater = false;
    if ( rpStart.m_bIsFolder == rpEnd.m_bIsFolder)
    {
      char szfilename1[1024];
      char szfilename2[1024];

      switch ( m_iSortMethod )
      {
      case 0:  // Sort by Filename
        strcpy(szfilename1, rpStart.GetLabel().c_str());
        strcpy(szfilename2, rpEnd.GetLabel().c_str());
        break;
      case 1:  // Sort by Date
        if ( rpStart.m_stTime.wYear > rpEnd.m_stTime.wYear ) return bGreater;
        if ( rpStart.m_stTime.wYear < rpEnd.m_stTime.wYear ) return !bGreater;

        if ( rpStart.m_stTime.wMonth > rpEnd.m_stTime.wMonth ) return bGreater;
        if ( rpStart.m_stTime.wMonth < rpEnd.m_stTime.wMonth ) return !bGreater;

        if ( rpStart.m_stTime.wDay > rpEnd.m_stTime.wDay ) return bGreater;
        if ( rpStart.m_stTime.wDay < rpEnd.m_stTime.wDay ) return !bGreater;

        if ( rpStart.m_stTime.wHour > rpEnd.m_stTime.wHour ) return bGreater;
        if ( rpStart.m_stTime.wHour < rpEnd.m_stTime.wHour ) return !bGreater;

        if ( rpStart.m_stTime.wMinute > rpEnd.m_stTime.wMinute ) return bGreater;
        if ( rpStart.m_stTime.wMinute < rpEnd.m_stTime.wMinute ) return !bGreater;

        if ( rpStart.m_stTime.wSecond > rpEnd.m_stTime.wSecond ) return bGreater;
        if ( rpStart.m_stTime.wSecond < rpEnd.m_stTime.wSecond ) return !bGreater;
        return true;
        break;

      case 2:
        if ( rpStart.m_dwSize > rpEnd.m_dwSize) return bGreater;
        if ( rpStart.m_dwSize < rpEnd.m_dwSize) return !bGreater;
        return true;
        break;

      case 3:  // Sort by share type
        if ( rpStart.m_iDriveType > rpEnd.m_iDriveType) return bGreater;
        if ( rpStart.m_iDriveType < rpEnd.m_iDriveType) return !bGreater;
        strcpy(szfilename1, rpStart.GetLabel());
        strcpy(szfilename2, rpEnd.GetLabel());
        break;

      default:  // Sort by Filename by default
        strcpy(szfilename1, rpStart.GetLabel().c_str());
        strcpy(szfilename2, rpEnd.GetLabel().c_str());
        break;
      }


      for (int i = 0; i < (int)strlen(szfilename1); i++)
        szfilename1[i] = tolower((unsigned char)szfilename1[i]);

      for (i = 0; i < (int)strlen(szfilename2); i++)
        szfilename2[i] = tolower((unsigned char)szfilename2[i]);
      //return (rpStart.strPath.compare( rpEnd.strPath )<0);

      if (m_bSortAscending)
        return (strcmp(szfilename1, szfilename2) < 0);
      else
        return (strcmp(szfilename1, szfilename2) >= 0);
    }
    if (!rpStart.m_bIsFolder) return false;
    return true;
  }
  bool m_bSortAscending;
  int m_iSortMethod;
};


CGUIWindowVideoBase::CGUIWindowVideoBase()
    : CGUIWindow(0)
{
  m_Directory.m_strPath = "?";
  m_Directory.m_bIsFolder = true;
  m_iItemSelected = -1;
  m_iLastControl = -1;
  m_iViewAsIcons = -1;
  m_iViewAsIconsRoot = -1;
  m_bDisplayEmptyDatabaseMessage = false;
}

CGUIWindowVideoBase::~CGUIWindowVideoBase()
{}


void CGUIWindowVideoBase::OnAction(const CAction &action)
{
  if (action.wID == ACTION_PARENT_DIR)
  {
    GoParentFolder();
    return ;
  }

  if (action.wID == ACTION_PREVIOUS_MENU)
  {
    m_gWindowManager.ActivateWindow(WINDOW_HOME);
    return ;
  }
  CGUIWindow::OnAction(action);
}

bool CGUIWindowVideoBase::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_DVDDRIVE_EJECTED_CD:
    {
      if ( !m_Directory.IsVirtualDirectoryRoot() )
      {
        if ( m_Directory.IsCDDA() || m_Directory.IsDVD() || m_Directory.IsISO9660() )
        {
          // Disc has changed and we are inside a DVD Drive share, get out of here :)
          m_Directory.m_strPath = "";
          Update( m_Directory.m_strPath );
        }
      }
      else
      {
        int iItem = m_viewControl.GetSelectedItem();
        Update( m_Directory.m_strPath );
        m_viewControl.SetSelectedItem(iItem);
      }
    }
    break;

  case GUI_MSG_DVDDRIVE_CHANGED_CD:
    {
      if ( m_Directory.IsVirtualDirectoryRoot() )
      {
        int iItem = m_viewControl.GetSelectedItem();
        Update( m_Directory.m_strPath );
        m_viewControl.SetSelectedItem(iItem);
      }
    }
    break;
  case GUI_MSG_WINDOW_DEINIT:
    m_iLastControl = GetFocusedControl();
    m_iItemSelected = m_viewControl.GetSelectedItem();
    ClearFileItems();
    m_database.Close();
    break;

  case GUI_MSG_WINDOW_INIT:
    {
      int iLastControl = m_iLastControl;
      CGUIWindow::OnMessage(message);

      LoadViewMode();

      m_database.Open();
      m_dlgProgress = (CGUIDialogProgress*)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);

      m_rootDir.SetMask(g_stSettings.m_szMyVideoExtensions);
      m_rootDir.SetShares(g_settings.m_vecMyVideoShares);

      Update(m_Directory.m_strPath);

 //     UpdateThumbPanel();

      if (iLastControl > -1)
      {
        SET_CONTROL_FOCUS(iLastControl, 0);
      }
      else
      {
        SET_CONTROL_FOCUS(m_dwDefaultFocusControlID, 0);
      }

      if (m_iItemSelected >= 0)
      {
        m_viewControl.SetSelectedItem(m_iItemSelected);
      }
      return true;
    }
    break;

  case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();
      if (iControl == CONTROL_BTNVIEWASICONS)
      {
        // cycle LIST->ICONS->LARGEICONS
        if (m_Directory.IsVirtualDirectoryRoot())
        {
          m_iViewAsIconsRoot++;
          if (m_iViewAsIconsRoot > VIEW_AS_LARGE_ICONS) m_iViewAsIconsRoot = VIEW_AS_LIST;
        }
        else
        {
          m_iViewAsIcons++;
          if (m_iViewAsIcons > VIEW_AS_LARGE_ICONS) m_iViewAsIcons = VIEW_AS_LIST;
        }
        SaveViewMode();
        UpdateButtons();
      }
      else if (iControl == CONTROL_PLAY_DVD)
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
          m_gWindowManager.ActivateWindow(nNewWindow);
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
        if (iAction == ACTION_SELECT_ITEM || iAction == ACTION_MOUSE_LEFT_CLICK)
        {
          OnClick(iItem);
        }
        else if (iAction == ACTION_QUEUE_ITEM || iAction == ACTION_MOUSE_MIDDLE_CLICK)
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
      }
      else if (iControl == CONTROL_IMDB)
      {
        OnManualIMDB();
      }
    }
  case GUI_MSG_SETFOCUS:
    {
      if (m_viewControl.HasControl(message.GetControlId()) && m_viewControl.GetCurrentControl() != message.GetControlId())
      {
        m_viewControl.SetFocused();
        return true;
      }
    }
  }
  return CGUIWindow::OnMessage(message);
}

void CGUIWindowVideoBase::OnWindowLoaded()
{
  // PRE1.3 transfer id's 10, 11 to 50, 51
  if (!GetControl(CONTROL_LIST))
  {
    for (unsigned int i=0; i < m_vecControls.size(); i++)
    {
      CGUIControl *pControl = m_vecControls[i];
      if (pControl->GetID() == 10) pControl->SetID(CONTROL_LIST);
      if (pControl->GetControlIdUp() == 10) pControl->SetNavigation(CONTROL_LIST, pControl->GetControlIdDown(), pControl->GetControlIdLeft(), pControl->GetControlIdRight());
      if (pControl->GetControlIdDown() == 10) pControl->SetNavigation(pControl->GetControlIdUp(), CONTROL_LIST, pControl->GetControlIdLeft(), pControl->GetControlIdRight());
      if (pControl->GetControlIdLeft() == 10) pControl->SetNavigation(pControl->GetControlIdUp(), pControl->GetControlIdDown(), CONTROL_LIST, pControl->GetControlIdRight());
      if (pControl->GetControlIdRight() == 10) pControl->SetNavigation(pControl->GetControlIdUp(), pControl->GetControlIdDown(), pControl->GetControlIdLeft(), CONTROL_LIST);
    }
  }
  if (!GetControl(CONTROL_THUMBS))
  {
    for (unsigned int i=0; i < m_vecControls.size(); i++)
    {
      CGUIControl *pControl = m_vecControls[i];
      if (pControl->GetID() == 11) pControl->SetID(CONTROL_THUMBS);
      if (pControl->GetControlIdUp() == 11) pControl->SetNavigation(CONTROL_THUMBS, pControl->GetControlIdDown(), pControl->GetControlIdLeft(), pControl->GetControlIdRight());
      if (pControl->GetControlIdDown() == 11) pControl->SetNavigation(pControl->GetControlIdUp(), CONTROL_THUMBS, pControl->GetControlIdLeft(), pControl->GetControlIdRight());
      if (pControl->GetControlIdLeft() == 11) pControl->SetNavigation(pControl->GetControlIdUp(), pControl->GetControlIdDown(), CONTROL_THUMBS, pControl->GetControlIdRight());
      if (pControl->GetControlIdRight() == 11) pControl->SetNavigation(pControl->GetControlIdUp(), pControl->GetControlIdDown(), pControl->GetControlIdLeft(), CONTROL_THUMBS);
    }
  }
  // PRE1.3
  m_viewControl.Reset();
  m_viewControl.SetParentWindow(GetID());
  m_viewControl.AddView(VIEW_AS_LIST, GetControl(CONTROL_LIST));
  m_viewControl.AddView(VIEW_AS_ICONS, GetControl(CONTROL_THUMBS));
  m_viewControl.AddView(VIEW_AS_LARGE_ICONS, GetControl(CONTROL_THUMBS));
  m_viewControl.SetViewControlID(CONTROL_BTNVIEWASICONS);
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

//  UpdateThumbPanel();

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

  if ( m_Directory.IsVirtualDirectoryRoot() )
    m_viewControl.SetCurrentView(m_iViewAsIconsRoot);
  else
    m_viewControl.SetCurrentView(m_iViewAsIcons);

  SET_CONTROL_LABEL(CONTROL_BTNSORTBY, SortMethod());

  if (SortAscending())
  {
    CGUIMessage msg(GUI_MSG_DESELECTED, GetID(), CONTROL_BTNSORTASC);
    g_graphicsContext.SendMessage(msg);
  }
  else
  {
    CGUIMessage msg(GUI_MSG_SELECTED, GetID(), CONTROL_BTNSORTASC);
    g_graphicsContext.SendMessage(msg);
  }

  int iItems = m_vecItems.Size();
  if (iItems)
  {
    CFileItem* pItem = m_vecItems[0];
    if (pItem->GetLabel() == "..") iItems--;
  }
  WCHAR wszText[20];
  const WCHAR* szText = g_localizeStrings.Get(127).c_str();
  swprintf(wszText, L"%i %s", iItems, szText);
  SET_CONTROL_LABEL(CONTROL_LABELFILES, wszText);
}

void CGUIWindowVideoBase::OnSort()
{
  FormatItemLabels();
  SortItems(m_vecItems);
  m_viewControl.SetItems(m_vecItems);
}

void CGUIWindowVideoBase::ClearFileItems()
{
  m_viewControl.Clear();
  m_vecItems.Clear(); // will clean up everything
}

bool CGUIWindowVideoBase::HaveDiscOrConnection( CStdString& strPath, int iDriveType )
{
  if ( iDriveType == SHARE_TYPE_DVD )
  {
    CDetectDVDMedia::WaitMediaReady();
    if ( !CDetectDVDMedia::IsDiscInDrive() )
    {
      CGUIDialogOK* dlg = (CGUIDialogOK*)m_gWindowManager.GetWindow(WINDOW_DIALOG_OK);
      if (dlg)
      {
        dlg->SetHeading( 218 );
        dlg->SetLine( 0, 219 );
        dlg->SetLine( 1, L"" );
        dlg->SetLine( 2, L"" );
        dlg->DoModal( GetID() );
      }
      int iItem = m_viewControl.GetSelectedItem();
      Update( m_Directory.m_strPath );
      m_viewControl.SetSelectedItem(iItem);
      return false;
    }
  }
  else if ( iDriveType == SHARE_TYPE_REMOTE )
  {
    // TODO: Handle not connected to a remote share
    if ( !CUtil::IsEthernetConnected() )
    {
      CGUIDialogOK* dlg = (CGUIDialogOK*)m_gWindowManager.GetWindow(WINDOW_DIALOG_OK);
      if (dlg)
      {
        dlg->SetHeading( 220 );
        dlg->SetLine( 0, 221 );
        dlg->SetLine( 1, L"" );
        dlg->SetLine( 2, L"" );
        dlg->DoModal( GetID() );
      }
      return false;
    }
  }
  else
    return true;
  return true;
}

void CGUIWindowVideoBase::OnInfo(int iItem)
{
  if ( iItem < 0 || iItem >= m_vecItems.Size() ) return ;
  CFileItem* pItem = m_vecItems[iItem];
  VECMOVIESFILES movies;
  m_database.GetFiles(atol(pItem->m_strPath), movies);
  if (movies.size() <= 0) return ;
  CStdString strFilePath = movies[0];
  CStdString strFile = CUtil::GetFileName(strFilePath);
  ShowIMDB(strFile, strFilePath, "" , false);
}

void CGUIWindowVideoBase::ShowIMDB(const CStdString& strMovie, const CStdString& strFile, const CStdString& strFolder, bool bFolder)
{
  CGUIDialogOK* pDlgOK = (CGUIDialogOK*)m_gWindowManager.GetWindow(WINDOW_DIALOG_OK);
  CGUIDialogProgress* pDlgProgress = (CGUIDialogProgress*)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
  CGUIDialogSelect* pDlgSelect = (CGUIDialogSelect*)m_gWindowManager.GetWindow(WINDOW_DIALOG_SELECT);
  CGUIWindowVideoInfo* pDlgInfo = (CGUIWindowVideoInfo*)m_gWindowManager.GetWindow(WINDOW_VIDEO_INFO);

  CIMDB IMDB;
  bool bUpdate(false);
  bool bFound = false;

  if (!pDlgOK) return ;
  if (!pDlgProgress) return ;
  if (!pDlgSelect) return ;
  if (!pDlgInfo) return ;
  CUtil::ClearCache();
  CStdString strMovieName = strMovie;

  if (m_database.HasMovieInfo(strFile) )
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
  CStdString strExtension;
  CUtil::GetExtension(strFile, strExtension);
  if ( CUtil::cmpnocase(strExtension.c_str(), ".nfo") == 0)
  {
    CFile file;
    if ( file.Cache(strFile, "Z:\\movie.nfo", NULL, NULL))
    {
      CNfoFile nfoReader;
      if ( nfoReader.Create("Z:\\movie.nfo") == S_OK)
      {
        CIMDBUrl url;
        CIMDBMovie movieDetails;
        url.m_strURL = nfoReader.m_strImDbUrl;
        if ( IMDB.GetDetails(url, movieDetails) )
        {
          // now show the imdb info
          pDlgInfo->SetMovie(movieDetails);
          pDlgInfo->DoModal(GetID());
          bFound = true;
          bUpdate = true;
          if ( !pDlgInfo->NeedRefresh() ) return ;
          m_database.DeleteMovieInfo(strFile);
        }
      }
    }
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
      if (IMDB.FindMovie(strMovieName, movielist) )
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
              if (!CGUIDialogKeyboard::ShowAndGetInput(strMovieName, false)) return ;
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
            if ( IMDB.GetDetails(url, movieDetails) )
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
                  if (CUtil::FileExists(strThumbOrg))
                  {
                    CStdString strFolderImage;
                    CUtil::AddFileToFolder(strFolder, "folder.jpg", strFolderImage);
                    CFileItem folder(strFolder, true);
                    if (folder.IsRemote() || folder.IsDVD() || folder.IsISO9660() )
                    {
                      CStdString strThumb;
                      CUtil::GetThumbnail( strFolderImage, strThumb);
                      CFile file;
                      file.Cache(strThumbOrg.c_str(), strThumb.c_str(), NULL, NULL);
                    }
                    else
                    {
                      CFile file;
                      file.Cache(strThumbOrg.c_str(), strFolderImage.c_str(), NULL, NULL);
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
            }
          }
        }
        else
        {
          pDlgProgress->Close();
          if (!CGUIDialogKeyboard::ShowAndGetInput(strMovieName, false)) return ;
          bContinue = true;
          bError = false;
        }
      }
      else
      {
        pDlgProgress->Close();
        if (!CGUIDialogKeyboard::ShowAndGetInput(strMovieName, false)) return ;
        bContinue = true;
        bError = false;
      }

      if (bError)
      {
        // show dialog...
        pDlgOK->SetHeading(195);
        pDlgOK->SetLine(0, strMovieName);
        pDlgOK->SetLine(1, L"");
        pDlgOK->SetLine(2, L"");
        pDlgOK->SetLine(3, L"");
        pDlgOK->DoModal(GetID());
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

    // HACK: If we are in files view
    // autoswitch between list/thumb control
    if (GetID() == WINDOW_VIDEOS && !m_Directory.IsVirtualDirectoryRoot() && g_guiSettings.GetBool("VideoLists.UseAutoSwitching"))
    {
      m_iViewAsIcons = CAutoSwitch::GetView(m_vecItems);
      UpdateButtons();
    }
  }
}

void CGUIWindowVideoBase::Render()
{
  CGUIWindow::Render();
  if (m_bDisplayEmptyDatabaseMessage)
  {
    CGUIListControl *pControl = (CGUIListControl *)GetControl(CONTROL_LIST);
    int iX = pControl->GetXPosition() + pControl->GetWidth() / 2;
    int iY = pControl->GetYPosition() + pControl->GetHeight() / 2;
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
  // don't call Update() with m_strParentPath, as we update m_strParentPath before the
  // directory is retrieved.
  CStdString strPath(m_strParentPath), strOldPath(m_Directory.m_strPath);
  Update(strPath);
  UpdateButtons();

  if (!g_guiSettings.GetBool("FileLists.FullDirectoryHistory"))
    m_history.Remove(strOldPath); //Delete current path

}

void CGUIWindowVideoBase::OnManualIMDB()
{
  CStdString strInput;
  if (!CGUIDialogKeyboard::ShowAndGetInput(strInput, false)) return ;

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
  if (!CUtil::FileExists(strFileName)) return false;
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
  if ( !movieFile.IsDVD() && !movieFile.IsISO9660()) return true;
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
  const CFileItem* pItem = m_vecItems[iItem];
  AddItemToPlayList(pItem);

  //move to next item
  m_viewControl.SetSelectedItem(iItem + 1);
}

void CGUIWindowVideoBase::AddItemToPlayList(const CFileItem* pItem)
{
  if (pItem->m_bIsFolder)
  {
    // Check if we add a locked share
    if ( pItem->m_bIsShareOrDrive )
    {
      CFileItem item = *pItem;
      if ( !g_passwordManager.IsItemUnlocked( &item, "music" ) )
        return ;
    }

    // recursive
    if (pItem->GetLabel() == "..") return ;
    CStdString strDirectory = m_Directory.m_strPath;
    m_Directory.m_strPath = pItem->m_strPath;
    CFileItemList items;
    GetDirectory(m_Directory.m_strPath, items);

    SortItems(items);

    for (int i = 0; i < items.Size(); ++i)
    {
      AddItemToPlayList(items[i]);
    }
    m_Directory.m_strPath = strDirectory;
  }
  else
  {
    if (!pItem->IsNFO() && pItem->IsVideo() && !pItem->IsPlayList())
    {
      CPlayList::CPlayListItem playlistItem ;
      playlistItem.SetFileName(pItem->m_strPath);
      playlistItem.SetDescription(pItem->GetLabel());
      playlistItem.SetDuration(pItem->m_musicInfoTag.GetDuration());
      g_playlistPlayer.GetPlaylist( PLAYLIST_VIDEO ).Add(playlistItem);
    }
  }
}

void CGUIWindowVideoBase::DisplayEmptyDatabaseMessage(bool bDisplay)
{
  m_bDisplayEmptyDatabaseMessage = bDisplay;
}

void CGUIWindowVideoBase::OnPopupMenu(int iItem)
{
  if (iItem < 0 || iItem >= m_vecItems.Size()) return ;
  // calculate our position
  int iPosX = 200;
  int iPosY = 100;
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
  // clean any buttons not needed
  pMenu->ClearButtons();
  // add the needed buttons
  pMenu->AddButton(13346); // Show Video Information
  pMenu->AddButton(13381);  // Resume Video
  pMenu->AddButton(13347); // Add to Playlist
  pMenu->AddButton(13350); // Now Playing...
  pMenu->AddButton(13349); // Query Info For All Files
  pMenu->AddButton(13348); // Search IMDb...
  pMenu->AddButton(5);   // Settings

  // check to see if the Resume Video button is applicable
  int startOffset = 0;
  pMenu->EnableButton(2, false);
  // grab any stacked files associated with this item
  vector<CStdString> movies;
  GetStackedFiles(m_vecItems[iItem]->m_strPath, movies);
  if (!m_vecItems[iItem]->IsNFO() && !m_vecItems[iItem]->IsPlayList())
  { // ok, we have a video file at least
    // grab the database info.  If stacking is enabled, we may have to check multiple files...
    for (unsigned int i=0; i < movies.size(); i++)
    {
      CVideoSettings settings;
      m_database.GetVideoSettings(movies[i], settings);
      if (settings.m_ResumeTime > 0)
      {
        startOffset = settings.m_ResumeTime + 0x10000000 * i; // assume no more than 16 files are stacked
        pMenu->EnableButton(2, true);
        break;
      }
    }
  }
  // turn off the now playing button if nothing is playing
  /* if (!g_application.IsPlayingVideo())
    pMenu->EnableButton(4, false);*/
  bool bIsGotoParent = m_vecItems[iItem]->GetLabel() == "..";
  if (bIsGotoParent)
  {
    pMenu->EnableButton(1, false);
    pMenu->EnableButton(2, false);
    pMenu->EnableButton(3, false);
  }
  // turn off the query info button if we aren't in files view
  if (GetID() != WINDOW_VIDEOS)
    pMenu->EnableButton(5, false);
  // position it correctly
  pMenu->SetPosition(iPosX - pMenu->GetWidth() / 2, iPosY - pMenu->GetHeight() / 2);
  pMenu->DoModal(GetID());
  switch (pMenu->GetButton())
  {
  case 1:  // Show Video Information
    OnInfo(iItem);
    break;
  case 2:  // Resume Video
    // set the start offset
    m_vecItems[iItem]->m_lStartOffset = startOffset;
    OnClick(iItem);
    break;
  case 3:  // Add to Playlist
    OnQueueItem(iItem);
    break;
  case 4:  // Now Playing...
    m_gWindowManager.ActivateWindow(WINDOW_VIDEO_PLAYLIST);
    return ;
    break;
  case 5:  // Query Info For All Files
    OnScan();
    break;
  case 6:  // Search IMDb...
    OnManualIMDB();
    break;
  case 7:  // Settings
    m_gWindowManager.ActivateWindow(WINDOW_SETTINGS_MYVIDEOS);
    return ;
    break;
  }
  m_vecItems[iItem]->Select(bSelected);
}

void CGUIWindowVideoBase::GetStackedFiles(const CStdString &strFilePath, vector<CStdString> &movies)
{
  if (CUtil::IsNaturalNumber(strFilePath))
  { // we have a database view
    movies.clear();
    m_database.GetFiles(atol(strFilePath.c_str()), movies);
    sort(movies.begin(), movies.end(), SSortVideoListByName());
    return;
  }
  // get the path and filename
  CStdString strPath;
  CStdString strFileName;
  CUtil::Split(strFilePath, strPath, strFileName);
//  if (CUtil::HasSlashAtEnd(strPath))
//    strPath.Delete(strPath.size() - 1);
  movies.clear();
  if (g_stSettings.m_iMyVideoVideoStack == STACK_NONE)
  {
    movies.push_back(strFilePath);
    return;
  }
  CStdString fileTitle;
  CStdString volumeNumber;
  if (g_stSettings.m_iMyVideoVideoStack == STACK_SIMPLE)
  {
    if (!CUtil::GetVolumeFromFileName(strFileName, fileTitle, volumeNumber))
    {
      // nothing to stack...
      movies.push_back(strFilePath);
      return;
    }
  }
  // ok - we're good to go - let's search for stacked files
  CFileItemList items;
  m_rootDir.GetDirectory(strPath, items);
  for (int i = 0; i < (int)items.Size(); ++i)
  {
    CFileItem *pItemTmp = items[i];
    if (!pItemTmp->IsNFO() && !pItemTmp->IsPlayList() && pItemTmp->IsVideo())
    {
      CStdString fileNameTemp = CUtil::GetFileName(pItemTmp->m_strPath);
      bool stackFile = false;

      if (strFileName.Equals(fileNameTemp))
      {
        stackFile = true;
      }
      else if (g_stSettings.m_iMyVideoVideoStack == STACK_FUZZY)
      {
        // fuzzy stacking
        double fPercentage = fstrcmp(fileNameTemp, strFileName, COMPARE_PERCENTAGE_MIN);
        if (fPercentage >= COMPARE_PERCENTAGE)
        {
          stackFile = true;
        }
      }
      else
      {
        // simple stacking
        CStdString fileTitle2;
        CStdString volumeNumber2;
        if (CUtil::GetVolumeFromFileName(fileNameTemp, fileTitle2, volumeNumber2))
        {
          if (fileTitle.Equals(fileTitle2))
          {
            stackFile = true;
          }
        }
      }

      if (stackFile)
      {
        movies.push_back(pItemTmp->m_strPath);
      }
    }
  }
  // check if we have anything at all to stack
  if (movies.empty())
    movies.push_back(strFileName);
  // sort them
  sort(movies.begin(), movies.end(), SSortVideoListByName());
}

void CGUIWindowVideoBase::PlayMovies(VECMOVIESFILES &movies, long lStartOffset)
{
  if (movies.size() <= 0) return ;
  if (!CheckMovie(movies[0])) return ;
  int iSelectedFile = 1;
  if (movies.size() > 1)
  {
    if (lStartOffset)
    { // have a startoffset, figure out which file it is
      iSelectedFile = ((lStartOffset & 0x10000000) >> 28) + 1;
    }
    else
    {
      CGUIDialogFileStacking* dlg = (CGUIDialogFileStacking*)m_gWindowManager.GetWindow(WINDOW_DIALOG_FILESTACKING);
      if (dlg)
      {
        dlg->SetNumberOfFiles(movies.size());
        dlg->DoModal(GetID());
        iSelectedFile = dlg->GetSelectedFile();
        if (iSelectedFile < 1) return ;
      }
    }
  }

  g_playlistPlayer.Reset();
  g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_VIDEO_TEMP);
  CPlayList& playlist = g_playlistPlayer.GetPlaylist(PLAYLIST_VIDEO_TEMP);
  playlist.Clear();
  for (int i = iSelectedFile - 1; i < (int)movies.size(); ++i)
  {
    CPlayList::CPlayListItem item;
    item.SetFileName(movies[i]);
    if (i == iSelectedFile - 1)
      item.m_lStartOffset = lStartOffset & 0x0fffffff;
    playlist.Add(item);
  }

  // play movie...
  g_playlistPlayer.PlayNext();
}
