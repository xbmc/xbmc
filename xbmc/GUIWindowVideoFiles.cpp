//

#include "stdafx.h"
#include "GUIWindowVideoFiles.h"
#include "Util.h"
#include "Picture.h"
#include "Utils/IMDB.h"
#include "Utils/HTTP.h"
#include "GUIWindowVideoInfo.h"
#include "PlayListFactory.h"
#include "Application.h"
#include "NFOFile.h"
#include "Utils/fstrcmp.h"
#include "PlayListPlayer.h"
#include "GUIThumbnailPanel.h"
#include "GUIListControl.h"
#include "AutoSwitch.h"
#include "GUIPassword.h"
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

struct SSortVideoByName
{
  static bool Sort(CFileItem* pStart, CFileItem* pEnd)
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
        return StringUtils::AlphaNumericCompare(szfilename1, szfilename2);
      else
        return !StringUtils::AlphaNumericCompare(szfilename1, szfilename2);
    }
    if (!rpStart.m_bIsFolder) return false;
    return true;
  }
  static bool m_bSortAscending;
  static int m_iSortMethod;
};
bool SSortVideoByName::m_bSortAscending;
int SSortVideoByName::m_iSortMethod;

CGUIWindowVideoFiles::CGUIWindowVideoFiles()
{
}

CGUIWindowVideoFiles::~CGUIWindowVideoFiles()
{
}

void CGUIWindowVideoFiles::OnAction(const CAction &action)
{
  if (action.wID == ACTION_SHOW_PLAYLIST)
  {
    OutputDebugString("activate guiwindowvideoplaylist!\n");
    m_gWindowManager.ActivateWindow(WINDOW_VIDEO_PLAYLIST);
    return ;
  }

  CGUIWindowVideoBase::OnAction(action);
}

bool CGUIWindowVideoFiles::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_DVDDRIVE_EJECTED_CD:
  case GUI_MSG_DVDDRIVE_CHANGED_CD:
  case GUI_MSG_WINDOW_DEINIT:
    CGUIWindowVideoBase::OnMessage(message);
    break;

  case GUI_MSG_WINDOW_INIT:
    {
      /*
      // This window is started by the home window.
      // Now we decide which my music window has to be shown and
      // switch to the my music window the user last activated.
      if (g_stSettings.m_iVideoStartWindow >0 && g_stSettings.m_iVideoStartWindow !=GetID() )
      {
       m_gWindowManager.ActivateWindow(g_stSettings.m_iVideoStartWindow);
       return false;
      }
      */

      // check for a passed destination path
      CStdString strDestination = message.GetStringParam();
      if (!strDestination.IsEmpty())
      {
        message.SetStringParam("");
        g_stSettings.m_iVideoStartWindow = GetID();
        CLog::Log(LOGINFO, "Attempting to quickpath to: %s", strDestination.c_str());
      }

      // unless we had a destination paramter switch to the last my music window
      if (g_stSettings.m_iVideoStartWindow > 0 && g_stSettings.m_iVideoStartWindow != GetID() )
      {
        m_gWindowManager.ActivateWindow(g_stSettings.m_iVideoStartWindow);
        return false;
      }

      // is this the first time accessing this window?
      // a quickpath overrides the a default parameter
      if (m_Directory.m_strPath == "?" && strDestination.IsEmpty())
      {
        m_Directory.m_strPath = strDestination = g_stSettings.m_szDefaultVideos;
        CLog::Log(LOGINFO, "Attempting to default to: %s", strDestination.c_str());
      }

      // try to open the destination path
      if (!strDestination.IsEmpty())
      {
        // default parameters if the jump fails
        m_Directory.m_strPath = "";

        bool bIsBookmarkName = false;
        int iIndex = CUtil::GetMatchingShare(strDestination, g_settings.m_vecMyVideoShares, bIsBookmarkName);
        if (iIndex > -1)
        {
          // set current directory to matching share
          if (bIsBookmarkName)
            m_Directory.m_strPath = g_settings.m_vecMyVideoShares[iIndex].strPath;
          else
            m_Directory.m_strPath = strDestination;
          CLog::Log(LOGINFO, "  Success! Opened destination path: %s", strDestination.c_str());
        }
        else
        {
          CLog::Log(LOGERROR, "  Failed! Destination parameter (%s) does not match a valid share!", strDestination.c_str());
        }

        // need file filters or GetDirectory in SetHistoryPath fails
        m_rootDir.SetMask(g_stSettings.m_szMyVideoExtensions);
        m_rootDir.SetShares(g_settings.m_vecMyVideoShares);

        SetHistoryForPath(m_Directory.m_strPath);
      }


      if (m_iViewAsIcons == -1 && m_iViewAsIconsRoot == -1)
      {
        m_iViewAsIcons = g_stSettings.m_iMyVideoViewAsIcons;
        m_iViewAsIconsRoot = g_stSettings.m_iMyVideoRootViewAsIcons;
      }

      return CGUIWindowVideoBase::OnMessage(message);
    }
    break;

  case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();
      if (iControl == CONTROL_BTNSORTBY) // sort by
      {
        if (m_Directory.IsVirtualDirectoryRoot())
        {
          if (g_stSettings.m_iMyVideoRootSortMethod == 0)
            g_stSettings.m_iMyVideoRootSortMethod = 3;
          else
            g_stSettings.m_iMyVideoRootSortMethod = 0;
        }
        else
        {
          g_stSettings.m_iMyVideoSortMethod++;
          if (g_stSettings.m_iMyVideoSortMethod >= 3) g_stSettings.m_iMyVideoSortMethod = 0;
        }
        g_settings.Save();
        UpdateButtons();
        OnSort();
      }
      else if (iControl == CONTROL_BTNSORTASC) // sort asc
      {
        if (m_Directory.IsVirtualDirectoryRoot())
          g_stSettings.m_bMyVideoRootSortAscending = !g_stSettings.m_bMyVideoRootSortAscending;
        else
          g_stSettings.m_bMyVideoSortAscending = !g_stSettings.m_bMyVideoSortAscending;

        g_settings.Save();
        UpdateButtons();
        OnSort();
      }
      else if (iControl == CONTROL_BTNSCAN)
      {
        OnScan();
      }

      else if (iControl == CONTROL_STACK)
      {
        // toggle between the following states:
        //   0 : no stacking
        //   1 : simple stacking
        //   2 : fuzzy stacking
        g_stSettings.m_iMyVideoVideoStack++;
        if (g_stSettings.m_iMyVideoVideoStack > STACK_SIMPLE) g_stSettings.m_iMyVideoVideoStack = STACK_NONE;
        if (g_stSettings.m_iMyVideoVideoStack != STACK_NONE)
          g_stSettings.m_bMyVideoCleanTitles = true;
        else
          g_stSettings.m_bMyVideoCleanTitles = false;
        g_settings.Save();
        UpdateButtons();
        Update( m_Directory.m_strPath );
      }
      else
        return CGUIWindowVideoBase::OnMessage(message);
    }
  }
  return CGUIWindowVideoBase::OnMessage(message);
}


void CGUIWindowVideoFiles::UpdateButtons()
{
  CGUIWindowVideoBase::UpdateButtons();
  SET_CONTROL_LABEL(CONTROL_STACK, g_stSettings.m_iMyVideoVideoStack + 14000);
}

void CGUIWindowVideoFiles::FormatItemLabels()
{
  for (int i = 0; i < (int)m_vecItems.Size(); i++)
  {
    CFileItem* pItem = m_vecItems[i];
    if (g_stSettings.m_iMyVideoSortMethod == 0 || g_stSettings.m_iMyVideoSortMethod == 2)
    {
      if (pItem->m_bIsFolder) pItem->SetLabel2("");
      else
      {
        CStdString strFileSize;
        CUtil::GetFileSize(pItem->m_dwSize, strFileSize);
        pItem->SetLabel2(strFileSize);
      }
    }
    else
    {
      if (pItem->m_stTime.wYear)
      {
        CStdString strDateTime;
        CUtil::GetDate(pItem->m_stTime, strDateTime);
        pItem->SetLabel2(strDateTime);
      }
      else
        pItem->SetLabel2("");
    }
  }
}

void CGUIWindowVideoFiles::SortItems(CFileItemList& items)
{
  if (m_Directory.IsVirtualDirectoryRoot())
  {
    SSortVideoByName::m_iSortMethod = g_stSettings.m_iMyVideoRootSortMethod;
    SSortVideoByName::m_bSortAscending = g_stSettings.m_bMyVideoRootSortAscending;
  }
  else
  {
    SSortVideoByName::m_iSortMethod = g_stSettings.m_iMyVideoSortMethod;
    SSortVideoByName::m_bSortAscending = g_stSettings.m_bMyVideoSortAscending;
  }
  items.Sort(SSortVideoByName::Sort);
}

void CGUIWindowVideoFiles::Update(const CStdString &strDirectory)
{
  UpdateDir(strDirectory);
  if (!m_Directory.IsVirtualDirectoryRoot() && g_guiSettings.GetBool("VideoLists.UseAutoSwitching"))
  {
    m_iViewAsIcons = CAutoSwitch::GetView(m_vecItems);

    int iFocusedControl = GetFocusedControl();

//    UpdateThumbPanel();
    UpdateButtons();
  }
}

void CGUIWindowVideoFiles::UpdateDir(const CStdString &strDirectory)
{
  // get selected item
  int iItem = m_viewControl.GetSelectedItem();
  CStdString strSelectedItem = "";
  if (iItem >= 0 && iItem < (int)m_vecItems.Size())
  {
    CFileItem* pItem = m_vecItems[iItem];
    if (pItem->GetLabel() != "..")
    {
      GetDirectoryHistoryString(pItem, strSelectedItem);
      m_history.Set(strSelectedItem, m_Directory.m_strPath);
    }
  }
  ClearFileItems();

  CStdString strParentPath;
  bool bParentExists = CUtil::GetParentPath(strDirectory, strParentPath);

  // check if current directory is a root share
  if ( !m_rootDir.IsShare(strDirectory) )
  {
    // no, do we got a parent dir?
    if ( bParentExists )
    {
      // yes
      if (!g_guiSettings.GetBool("VideoLists.HideParentDirItems"))
      {
        CFileItem *pItem = new CFileItem("..");
        pItem->m_strPath = strParentPath;
        pItem->m_bIsFolder = true;
        pItem->m_bIsShareOrDrive = false;
        m_vecItems.Add(pItem);
      }
      m_strParentPath = strParentPath;
    }
  }
  else
  {
    // yes, this is the root of a share
    // add parent path to the virtual directory
    if (!g_guiSettings.GetBool("VideoLists.HideParentDirItems"))
    {
      CFileItem *pItem = new CFileItem("..");
      pItem->m_strPath = "";
      pItem->m_bIsShareOrDrive = false;
      pItem->m_bIsFolder = true;
      m_vecItems.Add(pItem);
    }
    m_strParentPath.Empty();
  }

  m_Directory.m_strPath = strDirectory;

  if (g_stSettings.m_iMyVideoVideoStack != STACK_NONE)
  {
    CFileItemList items;
    m_rootDir.GetDirectory(strDirectory, items);
    bool bDVDFolder(false);
    //Figure out first if we are in a folder that contains a dvd
    for (int i = 0; i < (int)items.Size(); ++i) //Do it this way to avoid an extra roundtrip to files
    {
      CFileItem* pItem1 = items[i];
      if (CStdString(CUtil::GetFileName(pItem1->m_strPath)).Equals("VIDEO_TS.IFO"))
      {
        bDVDFolder = true;
        m_vecItems.Add(new CFileItem(*pItem1));
        items.Remove(i); //Make sure this is not included in the comeing search as it would have been deleted.
        break;
      }
    }

    for (int i = 0; i < items.Size(); ++i)
    {
      bool bAdd(true);
      CFileItem* pItem1 = items[i];
      if (pItem1->IsNFO())
      {
        bAdd = false;
      }
      else if (bDVDFolder && pItem1->IsDVDFile(true, true)) //Hide all dvdfiles
      {
        bAdd = false;
      }
      else
      {
        //don't stack folders and playlists
        if ((!pItem1->m_bIsFolder) && !pItem1->IsPlayList())
        {
          CStdString fileName1 = CUtil::GetFileName(pItem1->m_strPath);

          CStdString fileTitle;
          CStdString volumeNumber;

          bool searchForStackedFiles = false;
          if (g_stSettings.m_iMyVideoVideoStack == STACK_FUZZY)
          {
            searchForStackedFiles = true;
          }
          else
          {
            searchForStackedFiles = CUtil::GetVolumeFromFileName(fileName1, fileTitle, volumeNumber);
          }

          if (searchForStackedFiles)
          {
            for (int x = 0; x < (int)items.Size(); ++x)
            {
              if (i != x)
              {
                CFileItem* pItem2 = items[x];
                if ((!pItem2->m_bIsFolder) && !pItem2->IsPlayList())
                {
                  CStdString fileName2 = CUtil::GetFileName(pItem2->m_strPath);

                  if (g_stSettings.m_iMyVideoVideoStack == STACK_FUZZY)
                  {
                    // use "fuzzy" stacking

                    double fPercentage = fstrcmp(fileName1, fileName2, COMPARE_PERCENTAGE_MIN);
                    if (fPercentage >= COMPARE_PERCENTAGE)
                    {
                      int iGreater = strcmp(fileName1, fileName2);
                      if (iGreater > 0)
                      {
                        bAdd = false;
                        break;
                      }
                    }
                  }
                  else
                  {
                    // use traditional "simple" stacking (like XBMP)
                    // file name must end in -CD[n], where only the first
                    // one (-CD1) will be added to the display list

                    CStdString fileTitle2;
                    CStdString volumeNumber2;
                    if (CUtil::GetVolumeFromFileName(fileName2, fileTitle2, volumeNumber2))
                    {
                      // TODO: check volumePrefix - they should be in the
                      // same category, but not necessarily equal!

                      if (fileTitle.Equals(fileTitle2) && strcmp(volumeNumber.c_str(), volumeNumber2.c_str()) > 0)
                      {
                        bAdd = false;
                        break;
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
      if (bAdd)
      {
        m_vecItems.Add(new CFileItem(*pItem1));
      }
    }
  }
  else
  {
    CFileItemList items;
    m_rootDir.GetDirectory(strDirectory, m_vecItems);
  }

  m_iLastControl = GetFocusedControl();

  m_vecItems.SetThumbs();
  if ((g_guiSettings.GetBool("FileLists.HideExtensions"))/* || (g_stSettings.m_bMyVideoCleanTitles)*/)
    m_vecItems.RemoveExtensions();
  if (g_stSettings.m_bMyVideoCleanTitles)
    m_vecItems.CleanFileNames();

  SetIMDBThumbs(m_vecItems);
  m_vecItems.FillInDefaultIcons();
  OnSort();
  UpdateButtons();

  strSelectedItem = m_history.Get(m_Directory.m_strPath);
  for (int i = 0; i < (int)m_vecItems.Size(); ++i)
  {
    CFileItem* pItem = m_vecItems[i];
    CStdString strHistory;
    GetDirectoryHistoryString(pItem, strHistory);
    if (strHistory == strSelectedItem)
    {
      m_viewControl.SetSelectedItem(i);
      break;
    }
  }
}

void CGUIWindowVideoFiles::OnClick(int iItem)
{
  if ( iItem < 0 || iItem >= (int)m_vecItems.Size() ) return ;
  CFileItem* pItem = m_vecItems[iItem];
  CStdString strPath = pItem->m_strPath;
  CStdString strExtension;
  CUtil::GetExtension(pItem->m_strPath, strExtension);
  if ( CUtil::cmpnocase(strExtension.c_str(), ".nfo") == 0)
  {
    OnInfo(iItem);
    return ;
  }

  if (pItem->m_bIsFolder)
  {
    m_iItemSelected = -1;
    if ( pItem->m_bIsShareOrDrive )
    {
      if ( !g_passwordManager.IsItemUnlocked( pItem, "video" ) )
        return ;

      if ( !HaveDiscOrConnection( pItem->m_strPath, pItem->m_iDriveType ) )
        return ;
    }
    Update(strPath);
  }
  else
  {
    // Reset Playlistplayer, we may have played something
    // from another playlist. New playback started now may
    // not use the playlistplayer.
    g_playlistPlayer.Reset();
    g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_NONE);
    // Set selected item
    m_iItemSelected = m_viewControl.GetSelectedItem();
    if (pItem->IsPlayList())
    {
      LoadPlayList(pItem->m_strPath);
      return ;
    }
    if (!CheckMovie(pItem->m_strPath)) return;
    vector<CStdString> movies;
    GetStackedFiles(pItem->m_strPath, movies);
    for (int i = 0; i < (int)movies.size(); ++i)
    {
      CFileItem item(movies[i], false);
      AddFileToDatabase(&item);
    }
    PlayMovies(movies, pItem->m_lStartOffset);
  }
}

void CGUIWindowVideoFiles::OnInfo(int iItem)
{
  if ( iItem < 0 || iItem >= (int)m_vecItems.Size() ) return ;
  bool bFolder(false);
  CStdString strFolder = "";
  int iSelectedItem = m_viewControl.GetSelectedItem();
  CFileItem* pItem = m_vecItems[iItem];
  CStdString strFile = pItem->m_strPath;
  CStdString strMovie = pItem->GetLabel();
  if (pItem->m_bIsFolder && strMovie == "..") return ;
  if (pItem->m_bIsFolder)
  {
    // IMDB is done on a folder, find first file in folder
    strFolder = pItem->m_strPath;
    bFolder = true;
    CFileItemList vecitems;
    m_rootDir.GetDirectory(pItem->m_strPath, vecitems);
    bool bFoundFile(false);
    for (int i = 0; i < (int)vecitems.Size(); ++i)
    {
      CFileItem *pItem = vecitems[i];
      if (!pItem->m_bIsFolder)
      {
        if (pItem->IsVideo() && !pItem->IsNFO() && !pItem->IsPlayList() )
        {
          strFile = pItem->m_strPath;
          bFoundFile = true;
          break;
        }
      }
    }
    if (!bFoundFile)
    {
      // no video file in this folder?
      // then just lookup IMDB info and show it
      ShowIMDB(strMovie, strFolder, strFolder, true /*false*/);  // true for bFolder will save the thumb to the local disk (if applicable)
      // this should happen for the case where a folder only contains a bunch of folders as well.
      m_viewControl.SetSelectedItem(iSelectedItem);
      return ;
    }
  }

  vector<CStdString> movies;
  GetStackedFiles(strFile, movies);
  for (unsigned int i = 0; i < movies.size(); i++)
  {
    CFileItem item(movies[i], false);
    AddFileToDatabase(&item);
  }

  ShowIMDB(strMovie, strFile, strFolder, bFolder);
  m_viewControl.SetSelectedItem(iSelectedItem);
}

void CGUIWindowVideoFiles::Render()
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

void CGUIWindowVideoFiles::AddFileToDatabase(const CFileItem* pItem)
{
  CStdString strCDLabel = "";
  bool bHassubtitles = false;

  if (!pItem->IsVideo()) return ;
  if ( pItem->IsNFO()) return ;
  if ( pItem->IsPlayList()) return ;

  // get disc label for dvd's / iso9660
  if (pItem->IsDVD() || pItem->IsISO9660())
  {
    CCdInfo* pinfo = CDetectDVDMedia::GetCdInfo();
    if (pinfo)
    {
      strCDLabel = pinfo->GetDiscLabel();
    }
  }

  char * sub_exts[] = { ".utf", ".utf8", ".utf-8", ".sub", ".srt", ".smi", ".rt", ".txt", ".ssa", ".aqt", ".jss", ".ass", ".idx", ".ifo", NULL};
  // check if movie has subtitles
  int ipos = 0;
  while (sub_exts[ipos])
  {
    CStdString strSubTitleFile = pItem->m_strPath;
    CUtil::ReplaceExtension(pItem->m_strPath, sub_exts[ipos], strSubTitleFile);
    CFile file;
    if (file.Open(strSubTitleFile, false) )
    {
      bHassubtitles = true;
      break;
    }
    ipos++;
  }
  m_database.AddMovie(pItem->m_strPath, strCDLabel, bHassubtitles);
}

void CGUIWindowVideoFiles::OnRetrieveVideoInfo(CFileItemList& items)
{

  // for every file found
  for (int i = 0; i < (int)items.Size(); ++i)
  {
    g_application.ResetScreenSaver();
    CFileItem* pItem = items[i];
    if (!pItem->m_bIsFolder)
    {
      if (pItem->IsVideo() && !pItem->IsNFO() && !pItem->IsPlayList() )
      {
        CStdString strItem;
        strItem.Format("%i/%i", i + 1, items.Size());
        if (m_dlgProgress)
        {
          m_dlgProgress->SetLine(0, strItem);
          m_dlgProgress->SetLine(1, CUtil::GetFileName(pItem->m_strPath) );
          m_dlgProgress->Progress();
          if (m_dlgProgress->IsCanceled()) return ;
        }
        // get stacked items
        vector<CStdString> movies;
        GetStackedFiles(pItem->m_strPath, movies);
        for (unsigned int i = 0; i < movies.size(); i++)
        {
          CFileItem item(movies[i], false);
          AddFileToDatabase(&item);
        }
        if (!m_database.HasMovieInfo(pItem->m_strPath))
        {
          // do IMDB lookup...
          CStdString strMovieName = CUtil::GetFileName(pItem->m_strPath);
          CUtil::RemoveExtension(strMovieName);

          if (m_dlgProgress)
          {
            m_dlgProgress->SetHeading(197);
            m_dlgProgress->SetLine(0, strMovieName);
            m_dlgProgress->SetLine(1, "");
            m_dlgProgress->SetLine(2, "");
            m_dlgProgress->Progress();
          }


          CIMDB IMDB;
          IMDB_MOVIELIST movielist;
          if (IMDB.FindMovie(strMovieName, movielist) )
          {
            int iMoviesFound = movielist.size();
            if (iMoviesFound > 0)
            {
              CIMDBMovie movieDetails;
              movieDetails.m_strSearchString = pItem->m_strPath;
              CIMDBUrl& url = movielist[0];

              // show dialog that we're downloading the movie info
              if (m_dlgProgress)
              {
                m_dlgProgress->SetHeading(198);
                m_dlgProgress->SetLine(0, strMovieName);
                m_dlgProgress->SetLine(1, url.m_strTitle);
                m_dlgProgress->SetLine(2, "");
                m_dlgProgress->Progress();
              }

              CUtil::ClearCache();
              if ( IMDB.GetDetails(url, movieDetails) )
              {
                // add to all movies in the stacked set
                m_database.SetMovieInfo(pItem->m_strPath, movieDetails);
                // get & save thumbnail
                CStdString strThumb = "";
                CStdString strImage = movieDetails.m_strPictureURL;
                if (strImage.size() > 0 && movieDetails.m_strSearchString.size() > 0)
                {


                  CUtil::GetVideoThumbnail(movieDetails.m_strIMDBNumber, strThumb);
                  ::DeleteFile(strThumb.c_str());

                  CHTTP http;
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
                  http.Download(strImage, strTemp);

                  try
                  {
                    CPicture picture;
                    picture.Convert(strTemp, strThumb);
                  }
                  catch (...)
                  {
                    ::DeleteFile(strThumb.c_str());
                  }
                  ::DeleteFile(strTemp.c_str());
                }
              }
            }
          }
        }
      }
    }
  }
}

void CGUIWindowVideoFiles::OnScan()
{
  DoScan(m_vecItems);
}

bool CGUIWindowVideoFiles::DoScan(CFileItemList& items)
{
  // remove username + password from m_strDirectory for display in Dialog
  CURL url(m_Directory.m_strPath);
  CStdString strStrippedPath;
  url.GetURLWithoutUserDetails(strStrippedPath);

  if (m_dlgProgress)
  {
    m_dlgProgress->SetHeading(189);
    m_dlgProgress->SetLine(0, "");
    m_dlgProgress->SetLine(1, "");
    m_dlgProgress->SetLine(2, strStrippedPath );
    m_dlgProgress->StartModal(GetID());
  }

  OnRetrieveVideoInfo(items);

  bool bCancel = false;
  if (m_dlgProgress)
  {
    m_dlgProgress->SetLine(2, strStrippedPath );
    if (m_dlgProgress->IsCanceled())
    {
      bCancel = true;
    }
  }

  if (!bCancel)
  {
    for (int i = 0; i < (int)items.Size(); ++i)
    {
      CFileItem *pItem = items[i];
      if (m_dlgProgress)
      {
        if (m_dlgProgress->IsCanceled())
        {
          bCancel = true;
          break;
        }
      }
      if ( pItem->m_bIsFolder)
      {
        if (pItem->GetLabel() != "..")
        {
          // load subfolder
          CStdString strDir = m_Directory.m_strPath;
          m_Directory.m_strPath = pItem->m_strPath;
          CFileItemList subDirItems;
          m_rootDir.GetDirectory(pItem->m_strPath, subDirItems);
          if (m_dlgProgress)
            m_dlgProgress->Close();
          if (!DoScan(subDirItems))
          {
            bCancel = true;
          }

          m_Directory.m_strPath = strDir;
          if (bCancel) break;
        }
      }
    }
  }

  if (m_dlgProgress) m_dlgProgress->Close();
  return !bCancel;
}

void CGUIWindowVideoFiles::SetIMDBThumbs(CFileItemList& items)
{
  VECMOVIES movies;
  m_database.GetMoviesByPath(m_Directory.m_strPath, movies);
  for (int x = 0; x < (int)items.Size(); ++x)
  {
    CFileItem* pItem = items[x];
    if (!pItem->m_bIsFolder && pItem->GetThumbnailImage() == "")
    {
      for (int i = 0; i < (int)movies.size(); ++i)
      {
        CIMDBMovie& info = movies[i];
        CStdString strFile = CUtil::GetFileName(pItem->m_strPath);
        if (info.m_strFile[0] == '\\' || info.m_strFile[0] == '/')
          info.m_strFile.Delete(0, 1);

        if (strFile.size() > 0)
        {
          if (info.m_strFile == strFile /*|| pItem->GetLabel() == info.m_strTitle*/)
          {
            CStdString strThumb;
            CUtil::GetVideoThumbnail(info.m_strIMDBNumber, strThumb);
            if (CUtil::FileExists(strThumb))
              pItem->SetThumbnailImage(strThumb);

            break;
          }
        }
      }
    }
  }
}

bool CGUIWindowVideoFiles::SortAscending()
{
  if (m_Directory.IsVirtualDirectoryRoot())
    return g_stSettings.m_bMyVideoRootSortAscending;
  else
    return g_stSettings.m_bMyVideoSortAscending;
}

int CGUIWindowVideoFiles::SortMethod()
{
  if (m_Directory.IsVirtualDirectoryRoot())
  {
    if (g_stSettings.m_iMyVideoRootSortMethod == 0)
      return 103;
    else
      return 498; // Sort by: Type
  }
  else
    return g_stSettings.m_iMyVideoSortMethod + 103;
}

void CGUIWindowVideoFiles::LoadPlayList(const CStdString& strPlayList)
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
      CGUIDialogOK* pDlgOK = (CGUIDialogOK*)m_gWindowManager.GetWindow(WINDOW_DIALOG_OK);
      if (pDlgOK)
      {
        pDlgOK->SetHeading(6);
        pDlgOK->SetLine(0, L"");
        pDlgOK->SetLine(1, 477);
        pDlgOK->SetLine(2, L"");
        pDlgOK->DoModal(GetID());
      }
      return ; //hmmm unable to load playlist?
    }

    // how many songs are in the new playlist
    if (pPlayList->size() == 1)
    {
      // just 1 song? then play it (no need to have a playlist of 1 song)
      CPlayList::CPlayListItem item = (*pPlayList)[0];
      g_application.PlayFile(CFileItem(item));
      return ;
    }

    // clear current playlist
    g_playlistPlayer.GetPlaylist(PLAYLIST_VIDEO).Clear();

    // add each item of the playlist to the playlistplayer
    for (int i = 0; i < (int)pPlayList->size(); ++i)
    {
      const CPlayList::CPlayListItem& playListItem = (*pPlayList)[i];
      CStdString strLabel = playListItem.GetDescription();
      if (strLabel.size() == 0)
        strLabel = CUtil::GetTitleFromPath(playListItem.GetFileName());

      CPlayList::CPlayListItem playlistItem;
      playlistItem.SetFileName(playListItem.GetFileName());
      playlistItem.SetDescription(strLabel);
      playlistItem.SetDuration(playListItem.GetDuration());
      g_playlistPlayer.GetPlaylist( PLAYLIST_VIDEO ).Add(playlistItem);
    }
  }

  // if we got a playlist
  if (g_playlistPlayer.GetPlaylist( PLAYLIST_VIDEO ).size() )
  {
    // then get 1st song
    CPlayList& playlist = g_playlistPlayer.GetPlaylist( PLAYLIST_VIDEO );
    const CPlayList::CPlayListItem& item = playlist[0];

    // and start playing it
    g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_VIDEO);
    g_playlistPlayer.Reset();
    g_playlistPlayer.Play(0);

    // and activate the playlist window if its not activated yet
    if (GetID() == m_gWindowManager.GetActiveWindow())
    {
      m_gWindowManager.ActivateWindow(WINDOW_VIDEO_PLAYLIST);
    }
  }
}

void CGUIWindowVideoFiles::GetDirectory(const CStdString &strDirectory, CFileItemList &items)
{
  if (items.Size() )
  {
    // cleanup items
    items.Clear();
  }

  CStdString strParentPath;
  bool bParentExists = CUtil::GetParentPath(strDirectory, strParentPath);

  // check if current directory is a root share
  if ( !m_rootDir.IsShare(strDirectory) )
  {
    // no, do we got a parent dir?
    if ( bParentExists )
    {
      // yes
      if (!g_guiSettings.GetBool("VideoLists.HideParentDirItems"))
      {
        CFileItem *pItem = new CFileItem("..");
        pItem->m_strPath = strParentPath;
        pItem->m_bIsFolder = true;
        pItem->m_bIsShareOrDrive = false;
        items.Add(pItem);
      }
      m_strParentPath = strParentPath;
    }
  }
  else
  {
    // yes, this is the root of a share
    // add parent path to the virtual directory
    if (!g_guiSettings.GetBool("VideoLists.HideParentDirItems"))
    {
      CFileItem *pItem = new CFileItem("..");
      pItem->m_strPath = "";
      pItem->m_bIsShareOrDrive = false;
      pItem->m_bIsFolder = true;
      items.Add(pItem);
    }
    m_strParentPath = "";
  }
  m_rootDir.GetDirectory(strDirectory, items);

}

/// \brief Can be overwritten to build an own history string for \c m_history
/// \param pItem Item to build the history string from
/// \param strHistoryString History string build as return value
void CGUIWindowVideoFiles::GetDirectoryHistoryString(const CFileItem* pItem, CStdString& strHistoryString)
{
  if (pItem->m_bIsShareOrDrive)
  {
    // We are in the virual directory

    // History string of the DVD drive
    // must be handel separately
    if (pItem->m_iDriveType == SHARE_TYPE_DVD)
    {
      // Remove disc label from item label
      // and use as history string, m_strPath
      // can change for new discs
      CStdString strLabel = pItem->GetLabel();
      int nPosOpen = strLabel.Find('(');
      int nPosClose = strLabel.ReverseFind(')');
      if (nPosOpen > -1 && nPosClose > -1 && nPosClose > nPosOpen)
      {
        strLabel.Delete(nPosOpen + 1, (nPosClose) - (nPosOpen + 1));
        strHistoryString = strLabel;
      }
      else
        strHistoryString = strLabel;
    }
    else
    {
      // Other items in virual directory
      CStdString strPath = pItem->m_strPath;
      while (CUtil::HasSlashAtEnd(strPath))
        strPath.Delete(strPath.size() - 1);

      strHistoryString = pItem->GetLabel() + strPath;
    }
  }
  else
  {
    // Normal directory items
    strHistoryString = pItem->m_strPath;

    if (CUtil::HasSlashAtEnd(strHistoryString))
      strHistoryString.Delete(strHistoryString.size() - 1);
  }
}

void CGUIWindowVideoFiles::SetHistoryForPath(const CStdString& strDirectory)
{
  if (!strDirectory.IsEmpty())
  {
    // Build the directory history for default path
    CStdString strPath, strParentPath;
    strPath = strDirectory;
    CFileItemList items;
    GetDirectory("", items);

    while (CUtil::GetParentPath(strPath, strParentPath))
    {
      bool bSet = false;
      for (int i = 0; i < items.Size(); ++i)
      {
        CFileItem* pItem = items[i];
        while (CUtil::HasSlashAtEnd(pItem->m_strPath))
          pItem->m_strPath.Delete(pItem->m_strPath.size() - 1);
        if (pItem->m_strPath == strPath)
        {
          CStdString strHistory;
          GetDirectoryHistoryString(pItem, strHistory);
          m_history.Set(strHistory, "");
          return ;
        }
      }

      m_history.Set(strPath, strParentPath);
      strPath = strParentPath;
      while (CUtil::HasSlashAtEnd(strPath))
        strPath.Delete(strPath.size() - 1);
    }
  }
}

void CGUIWindowVideoFiles::OnPopupMenu(int iItem)
{
  // calculate our position
  int iPosX = 200;
  int iPosY = 100;
  CGUIListControl *pList = (CGUIListControl *)GetControl(CONTROL_LIST);
  if (pList)
  {
    iPosX = pList->GetXPosition() + pList->GetWidth() / 2;
    iPosY = pList->GetYPosition() + pList->GetHeight() / 2;
  }
  if ( m_Directory.IsVirtualDirectoryRoot() )
  {
    if (iItem < 0)
    { // TODO: We should add the option here for shares to be added if there aren't any
      return ;
    }
    // mark the item
    m_vecItems[iItem]->Select(true);

    bool bMaxRetryExceeded = false;
    if (g_stSettings.m_iMasterLockMaxRetry != 0)
      bMaxRetryExceeded = !(m_vecItems[iItem]->m_iBadPwdCount < g_stSettings.m_iMasterLockMaxRetry);

    // and do the popup menu
    if (CGUIDialogContextMenu::BookmarksMenu("video", m_vecItems[iItem]->GetLabel(), m_vecItems[iItem]->m_strPath, m_vecItems[iItem]->m_iLockMode, bMaxRetryExceeded, iPosX, iPosY))
    {
      m_rootDir.SetShares(g_settings.m_vecMyVideoShares);
      Update(m_Directory.m_strPath);
      return ;
    }
    m_vecItems[iItem]->Select(false);
    return ;
  }
  CGUIWindowVideoBase::OnPopupMenu(iItem);
}

void CGUIWindowVideoFiles::LoadViewMode()
{
  m_iViewAsIconsRoot = g_stSettings.m_iMyVideoRootViewAsIcons;
  m_iViewAsIcons = g_stSettings.m_iMyVideoViewAsIcons;
}

void CGUIWindowVideoFiles::SaveViewMode()
{
  g_stSettings.m_iMyVideoRootViewAsIcons = m_iViewAsIconsRoot;
  g_stSettings.m_iMyVideoViewAsIcons = m_iViewAsIcons;
  g_settings.Save();
}