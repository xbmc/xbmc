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
#include "PlayListPlayer.h"
#include "GUIThumbnailPanel.h"
#include "GUIListControl.h"
#include "AutoSwitch.h"
#include "GUIPassword.h"
#include "GUIFontManager.h"
#include "GUIDialogContextMenu.h"

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
      CStdString strStart, strEnd;

      switch ( m_iSortMethod )
      {
      case 0:  // Sort by Filename
        strStart = rpStart.GetLabel();
        strEnd = rpEnd.GetLabel();
        strcpy(szfilename1, strStart.c_str());
        strcpy(szfilename2, strEnd.c_str());
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
        strStart = rpStart.GetLabel();
        strEnd = rpEnd.GetLabel();
        strcpy(szfilename1, strStart.c_str());
        strcpy(szfilename2, strEnd.c_str());
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
: CGUIWindowVideoBase(WINDOW_VIDEOS, "MyVideo.xml")
{
}

CGUIWindowVideoFiles::~CGUIWindowVideoFiles()
{
}

bool CGUIWindowVideoFiles::OnAction(const CAction &action)
{
  if (action.wID == ACTION_SHOW_PLAYLIST)
  {
    OutputDebugString("activate guiwindowvideoplaylist!\n");
    m_gWindowManager.ActivateWindow(WINDOW_VIDEO_PLAYLIST);
    return true;
  }

  return CGUIWindowVideoBase::OnAction(action);
}

bool CGUIWindowVideoFiles::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
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
        
        if (g_stSettings.m_iMyVideoVideoStack > STACK_SIMPLE) 
          g_stSettings.m_iMyVideoVideoStack = STACK_NONE;

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
      if (pItem->m_bIsFolder)
        pItem->SetLabel2("");
      else
        pItem->SetFileSizeLabel();
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
    // in the case of sort by date or sort by size, it makes sense to have newest or largest
    // as the default order (ie same as normal alphabetic order)
    if (g_stSettings.m_iMyVideoSortMethod == 1 || g_stSettings.m_iMyVideoSortMethod == 2)
      SSortVideoByName::m_bSortAscending = !SSortVideoByName::m_bSortAscending;
  }
  items.Sort(SSortVideoByName::Sort);
}

bool CGUIWindowVideoFiles::Update(const CStdString &strDirectory)
{
  if (!UpdateDir(strDirectory))
    return false;

  if (!m_Directory.IsVirtualDirectoryRoot() && g_guiSettings.GetBool("VideoFiles.UseAutoSwitching"))
  {
    m_iViewAsIcons = CAutoSwitch::GetView(m_vecItems);

    int iFocusedControl = GetFocusedControl();

//    UpdateThumbPanel();
    UpdateButtons();
  }

  return true;
}

bool CGUIWindowVideoFiles::UpdateDir(const CStdString &strDirectory)
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

  CStdString strOldDirectory=m_Directory.m_strPath;
  m_Directory.m_strPath = strDirectory;

  CFileItemList items;
  if (!GetDirectory(m_Directory.m_strPath, items))
  {
    m_Directory.m_strPath = strOldDirectory;
    return false;
  }

  m_history.Set(strSelectedItem, strOldDirectory);

  ClearFileItems();

  m_vecItems.AppendPointer(items);
  items.ClearKeepPointer();

  if (g_stSettings.m_iMyVideoVideoStack != STACK_NONE)
  {
    //sort list ascending by filename before stacking...
    SSortVideoByName::m_iSortMethod = 0;
    SSortVideoByName::m_bSortAscending = 1;
    m_vecItems.Sort(SSortVideoByName::Sort);
    m_vecItems.Stack();
  }

  m_iLastControl = GetFocusedControl();

  m_vecItems.SetThumbs();
  if (g_stSettings.m_bMyVideoCleanTitles)
    m_vecItems.CleanFileNames();
  else if (g_guiSettings.GetBool("FileLists.HideExtensions"))
    m_vecItems.RemoveExtensions();

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

  return true;
}

void CGUIWindowVideoFiles::OnClick(int iItem)
{
  if ( iItem < 0 || iItem >= (int)m_vecItems.Size() ) return ;
  CFileItem* pItem = m_vecItems[iItem];
  CStdString strPath = pItem->m_strPath;
  CStdString strExtension;
  CUtil::GetExtension(pItem->m_strPath, strExtension);
  if ( strcmpi(strExtension.c_str(), ".nfo") == 0)
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
    if (!Update(strPath))
      ShowShareErrorMessage(pItem);
  }
  else if (pItem->IsZIP() && g_guiSettings.GetBool("VideoFiles.HandleArchives")) // mount zip archive
  {
    CShare shareZip;
    shareZip.strPath.Format("zip://Z:\\temp\\,%i,,%s,\\",1, pItem->m_strPath.c_str() );
    m_rootDir.AddShare(shareZip);
    Update(shareZip.strPath);
  }
  else if (pItem->IsRAR() && g_guiSettings.GetBool("VideoFiles.HandleArchives")) // mount rar archive
  {
    CShare shareRar;
    shareRar.strPath.Format("rar://Z:\\temp\\,%i,,%s,\\",1, pItem->m_strPath.c_str() );
    m_rootDir.AddShare(shareRar);
    Update(shareRar.strPath);
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
      else
      { // check for a dvdfolder
        if (pItem->m_strPath.CompareNoCase("VIDEO_TS"))
        { // found a dvd folder - grab the main .ifo file
          CUtil::AddFileToFolder(pItem->m_strPath, "video_ts.ifo", strFile);
          if (CFile::Exists(strFile))
          {
            bFoundFile = true;
            break;
          }
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

void CGUIWindowVideoFiles::AddFileToDatabase(const CFileItem* pItem)
{
  CStdString strCDLabel = "";
  bool bHassubtitles = false;

  if (!pItem->IsVideo()) return ;
  if ( pItem->IsNFO()) return ;
  if ( pItem->IsPlayList()) return ;

  // get disc label for dvd's / iso9660
  if (pItem->IsOnDVD())
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
          // handle .nfo files
          CStdString strNfoFile;
          CUtil::ReplaceExtension(pItem->m_strPath, ".nfo", strNfoFile);
          if (!CFile::Exists(strNfoFile))
            strNfoFile.Empty();
          if ( !strNfoFile.IsEmpty() )
          {
            if ( CFile::Cache(strNfoFile, "Z:\\movie.nfo", NULL, NULL))
            {
              CNfoFile nfoReader;
              if ( nfoReader.Create("Z:\\movie.nfo") == S_OK)
              {
                CIMDBUrl url;
                url.m_strURL = nfoReader.m_strImDbUrl;
                GetIMDBDetails(pItem, url);
                continue;
              }
            }
          }
          CStdString strMovieName;
          if (pItem->IsOnDVD() || pItem->IsDVDFile())
          {
            // find the name by back-drilling to the folder name
            CStdString strFolder;
            CUtil::GetDirectory(pItem->m_strPath, strFolder);
            int video_ts = strFolder.ReverseFind("VIDEO_TS");
            if (video_ts == strFolder.size() - 8)
              CUtil::GetDirectory(strFolder, strFolder);
            strMovieName = CUtil::GetFileName(strFolder);
          }
          else
          {
            strMovieName = CUtil::GetFileName(pItem->m_strPath);
            CUtil::RemoveExtension(strMovieName);
          }
          // do IMDB lookup...
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
          if (IMDB.FindMovie(strMovieName, movielist, m_dlgProgress) )
          {
            int iMoviesFound = movielist.size();
            if (iMoviesFound > 0)
            {
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
              GetIMDBDetails(pItem, url);
            }
          }
        }
      }
    }
  }
}

void CGUIWindowVideoFiles::OnScan()
{
  // force stacking on for best results
  // save stack state
  int iStack = g_stSettings.m_iMyVideoVideoStack;
  g_stSettings.m_iMyVideoVideoStack = STACK_SIMPLE;

  CFileItemList items;
  GetStackedDirectory(m_Directory.m_strPath, items);
  DoScan(m_Directory.m_strPath, items);

  // restore stack state
  g_stSettings.m_iMyVideoVideoStack = iStack;
}

bool CGUIWindowVideoFiles::DoScan(const CStdString &strPath, CFileItemList& items)
{
  // remove username + password from strPath for display in Dialog
  CURL url(strPath);
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
          CFileItemList subDirItems;
          GetStackedDirectory(pItem->m_strPath, subDirItems);
          if (m_dlgProgress)
            m_dlgProgress->Close();
          if (!DoScan(pItem->m_strPath, subDirItems))
          {
            bCancel = true;
          }
          if (bCancel) break;
        }
      }
    }
  }

  if (m_dlgProgress) m_dlgProgress->Close();
  return !bCancel;
}

void CGUIWindowVideoFiles::GetStackedDirectory(const CStdString &strPath, CFileItemList &items)
{
  items.Clear();
  m_rootDir.GetDirectory(strPath, items);

  // force stacking to be enabled
  // save stack state
  int iStack = g_stSettings.m_iMyVideoVideoStack;
  g_stSettings.m_iMyVideoVideoStack = STACK_SIMPLE;

  //sort list ascending by filename before stacking...
  SSortVideoByName::m_iSortMethod = 0;
  SSortVideoByName::m_bSortAscending = 1;
  items.Sort(SSortVideoByName::Sort);
  items.Stack();

  // restore stack
  g_stSettings.m_iMyVideoVideoStack = iStack;
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
            if (CFile::Exists(strThumb))
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
      CGUIDialogOK::ShowAndGetInput(6, 0, 477, 0);
      return ; //hmmm unable to load playlist?
    }
  }

  int iSize = pPlayList->size();
  if (g_application.ProcessAndStartPlaylist(strPlayList, *pPlayList, PLAYLIST_VIDEO))
  {
    // activate the playlist window if its not activated yet
    if (GetID() == m_gWindowManager.GetActiveWindow() && iSize > 1)
    {
      m_gWindowManager.ActivateWindow(WINDOW_VIDEO_PLAYLIST);
    }
  }
}

bool CGUIWindowVideoFiles::GetDirectory(const CStdString &strDirectory, CFileItemList &items)
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
      if (!g_guiSettings.GetBool("MyVideos.HideParentDirItems"))
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
    if (!g_guiSettings.GetBool("MyVideos.HideParentDirItems"))
    {
      CFileItem *pItem = new CFileItem("..");
      pItem->m_strPath = "";
      pItem->m_bIsShareOrDrive = false;
      pItem->m_bIsFolder = true;
      items.Add(pItem);
    }
    m_strParentPath = "";
  }

  return m_rootDir.GetDirectory(strDirectory, items);
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

void CGUIWindowVideoFiles::GetIMDBDetails(CFileItem *pItem, CIMDBUrl &url)
{
  CIMDB IMDB;
  CIMDBMovie movieDetails;
  movieDetails.m_strSearchString = pItem->m_strPath;
  if ( IMDB.GetDetails(url, movieDetails, m_dlgProgress) )
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

void CGUIWindowVideoFiles::OnQueueItem(int iItem)
{
  if (iItem < 0 || iItem >= (int)m_vecItems.Size()) return;

  CFileItem* pItem = m_vecItems[iItem];
  if (pItem->m_bIsFolder || g_stSettings.m_iMyVideoVideoStack == STACK_NONE)
  {
    CGUIWindowVideoBase::OnQueueItem(iItem);
    return;
  }

  vector<CStdString> movies;
  GetStackedFiles(pItem->m_strPath, movies);
  if (movies.size() <= 0) return;
  for (int i = 0; i < (int)movies.size(); ++i)
  {
    CFileItem* pMovieFile = new CFileItem(movies[i], false);
    CStdString strFileNum;
    strFileNum.Format("(%2.2i)",i+1);
    pMovieFile->SetLabel(pItem->GetLabel() + " " + strFileNum);
    AddItemToPlayList(pMovieFile);
  }

  //move to next item
  m_viewControl.SetSelectedItem(iItem + 1);
}
