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
#include "GUIPassword.h"
#include "GUIDialogContextMenu.h"
#include "FileSystem/StackDirectory.h"

#define CONTROL_LIST              50

#define CONTROL_PLAY_DVD          6
#define CONTROL_STACK             7
#define CONTROL_BTNSCAN           8
#define CONTROL_IMDB              9
#define CONTROL_BTNPLAYLISTS  13

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
      // check for a passed destination path
      CStdString strDestination = message.GetStringParam();
      if (!strDestination.IsEmpty())
      {
        message.SetStringParam("");
        g_stSettings.m_iVideoStartWindow = GetID();
        CLog::Log(LOGINFO, "Attempting to quickpath to: %s", strDestination.c_str());
        // reset directory path, as we have effectively cleared it here
        m_history.ClearPathHistory();
      }

      // is this the first time accessing this window?
      // a quickpath overrides the a default parameter
      if (m_vecItems.m_strPath == "?" && strDestination.IsEmpty())
      {
        m_vecItems.m_strPath = strDestination = g_stSettings.m_szDefaultVideos;
        CLog::Log(LOGINFO, "Attempting to default to: %s", strDestination.c_str());
      }

      // try to open the destination path
      if (!strDestination.IsEmpty())
      {
        // open playlists location
        if (strDestination.Equals("$PLAYLISTS"))
        {
          m_vecItems.m_strPath = CUtil::VideoPlaylistsLocation();
          CLog::Log(LOGINFO, "  Success! Opening destination path: %s", m_vecItems.m_strPath.c_str());
        }
        else
        {
          // default parameters if the jump fails
          m_vecItems.m_strPath = "";

          bool bIsBookmarkName = false;
          int iIndex = CUtil::GetMatchingShare(strDestination, g_settings.m_vecMyVideoShares, bIsBookmarkName);
          if (iIndex > -1)
          {
            // set current directory to matching share
            if (bIsBookmarkName)
              m_vecItems.m_strPath = g_settings.m_vecMyVideoShares[iIndex].strPath;
            else
              m_vecItems.m_strPath = strDestination;
            CLog::Log(LOGINFO, "  Success! Opened destination path: %s", strDestination.c_str());
          }
          else
          {
            CLog::Log(LOGERROR, "  Failed! Destination parameter (%s) does not match a valid share!", strDestination.c_str());
          }
        }

        // need file filters or GetDirectory in SetHistoryPath fails
        m_rootDir.SetMask(g_stSettings.m_szMyVideoExtensions);
        m_rootDir.SetShares(g_settings.m_vecMyVideoShares);
        SetHistoryForPath(m_vecItems.m_strPath);
      }

      return CGUIWindowVideoBase::OnMessage(message);
    }
    break;

  case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();
      if (iControl == CONTROL_BTNSCAN)
      {
        OnScan();
      }
      else if (iControl == CONTROL_STACK)
      {
        // toggle between the following states:
        //   0 : no stacking
        //   1 : stacking
        g_stSettings.m_iMyVideoStack++;
        
        if (g_stSettings.m_iMyVideoStack > STACK_SIMPLE) 
          g_stSettings.m_iMyVideoStack = STACK_NONE;

        if (g_stSettings.m_iMyVideoStack != STACK_NONE)
          g_stSettings.m_bMyVideoCleanTitles = true;
        else
          g_stSettings.m_bMyVideoCleanTitles = false;
        g_settings.Save();
        UpdateButtons();
        Update( m_vecItems.m_strPath );
      }
      else if (iControl == CONTROL_BTNPLAYLISTS)
      {
        if (!m_vecItems.m_strPath.Equals(CUtil::VideoPlaylistsLocation()))
        {
          CStdString strParent = m_vecItems.m_strPath;
          UpdateButtons();
          Update(CUtil::VideoPlaylistsLocation());
        }
      }
      // list/thumb panel 
      else if (m_viewControl.HasControl(iControl))
      {
        int iItem = m_viewControl.GetSelectedItem();
        int iAction = message.GetParam1();

        const CFileItem* pItem = m_vecItems[iItem];

        // use play button to add folders of items to temp playlist
        if (iAction == ACTION_PLAYER_PLAY && pItem->m_bIsFolder && !pItem->IsParentFolder())
        {
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
        }
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
  SET_CONTROL_LABEL(CONTROL_STACK, g_stSettings.m_iMyVideoStack + 14000);
}

bool CGUIWindowVideoFiles::GetDirectory(const CStdString &strDirectory, CFileItemList &items)
{
  if (!CGUIWindowVideoBase::GetDirectory(strDirectory, items))
    return false;

  if (!m_vecItems.IsStack() && g_stSettings.m_iMyVideoStack != STACK_NONE)
  {
    //sort list ascending by filename before stacking...
    m_vecItems.Sort(SORT_METHOD_LABEL, SORT_ORDER_ASC);
    m_vecItems.Stack();
  }

  return true;
}

bool CGUIWindowVideoFiles::Update(const CStdString &strDirectory)
{
  // get selected item
  int iItem = m_viewControl.GetSelectedItem();
  CStdString strSelectedItem = "";
  if (iItem >= 0 && iItem < (int)m_vecItems.Size())
  {
    CFileItem* pItem = m_vecItems[iItem];
    if (!pItem->IsParentFolder())
    {
      GetDirectoryHistoryString(pItem, strSelectedItem);
      m_history.SetSelectedItem(strSelectedItem, m_vecItems.m_strPath);
    }
  }

  CStdString strOldDirectory = m_vecItems.m_strPath;

  m_history.SetSelectedItem(strSelectedItem, strOldDirectory);

  ClearFileItems();

  if (!GetDirectory(strDirectory, m_vecItems))
    return !Update(strOldDirectory); // We assume, we can get the parent 
                                     // directory again, but we have to 
                                     // return false to be able to eg. show 
                                     // an error message.

  // if we're getting the root bookmark listing
  // make sure the path history is clean
  if (strDirectory.IsEmpty())
    m_history.ClearPathHistory();

  m_iLastControl = GetFocusedControl();

  m_vecItems.SetThumbs();
  
  if (g_stSettings.m_bMyVideoCleanTitles)
    m_vecItems.CleanFileNames();
  else if (m_guiState.get() && m_guiState->HideExtensions())
    m_vecItems.RemoveExtensions();
  
  SetIMDBThumbs(m_vecItems);
  m_vecItems.FillInDefaultIcons();

  // changed this from OnSort() because it was incorrectly selecting
  // the wrong item!
  m_guiState.reset(CGUIViewState::GetViewState(GetID(), m_vecItems));
  FormatItemLabels();
  SortItems(m_vecItems);
  m_viewControl.SetItems(m_vecItems);

  UpdateButtons();

  strSelectedItem = m_history.GetSelectedItem(m_vecItems.m_strPath);
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

  m_history.AddPath(strDirectory);

  return true;
}

bool CGUIWindowVideoFiles::OnClick(int iItem)
{
  if ( iItem < 0 || iItem >= (int)m_vecItems.Size() ) return true;
  CFileItem* pItem = m_vecItems[iItem];
  CStdString strExtension;
  CUtil::GetExtension(pItem->m_strPath, strExtension);

  if ( strcmpi(strExtension.c_str(), ".nfo") == 0)
  {
    OnInfo(iItem);
    return true;
  }

  return CGUIWindowVideoBase::OnClick(iItem);
}

void CGUIWindowVideoFiles::OnPlayMedia(int iItem)
{
  if ( iItem < 0 || iItem >= (int)m_vecItems.Size() ) return;
  CFileItem* pItem = m_vecItems[iItem];

  AddFileToDatabase(pItem);

  CGUIWindowVideoBase::OnPlayMedia(iItem);
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
  if (pItem->m_bIsFolder && pItem->IsParentFolder()) return ;
  if (pItem->m_bIsFolder)
  {
    // IMDB is done on a folder
    // stack and then find first file in folder
    strFolder = pItem->m_strPath;
    bFolder = true;
    CFileItemList vecitems;
    GetStackedDirectory(pItem->m_strPath, vecitems);
    bool bFoundFile(false);
    for (int i = 0; i < (int)vecitems.Size(); ++i)
    {
      CFileItem *item = vecitems[i];
      if (!item->m_bIsFolder)
      {
        if (item->IsVideo() && !item->IsNFO() && !item->IsPlayList() )
        {
          bFoundFile = true;
          strFile = item->m_strPath;
          break;
        }
      }
      else
      { // check for a dvdfolder
        if (item->GetLabel().CompareNoCase("VIDEO_TS") == 0)
        { // found a dvd folder - grab the main .ifo file
          CUtil::AddFileToFolder(item->m_strPath, "VIDEO_TS.IFO", strFile);
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

  CFileItem item(strFile, false);
  AddFileToDatabase(&item);

  /* uncommented stack code for consistency with OnRetrieveVideoInfo */
  vector<CStdString> movies;
  if (pItem->IsStack())
  { // add the individual files as well at this point
    // TODO: This should be removed as soon as we no longer need the individual
    // files for saving settings etc.
    GetStackedFiles(strFile, movies);
    for (unsigned int i = 0; i < movies.size(); i++)
    {
      CFileItem item(movies[i], false);
      AddFileToDatabase(&item);
    }
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
        vector<CStdString> movies;
        AddFileToDatabase(pItem);
        if (pItem->IsStack())
        { // get stacked items
          // TODO: This should be removed as soon as we no longer need the individual
          // files for saving settings etc.
          GetStackedFiles(pItem->m_strPath, movies);
          for (unsigned int i = 0; i < movies.size(); i++)
          {
            CFileItem item(movies[i], false);
            AddFileToDatabase(&item);
          }
        }
        if (!m_database.HasMovieInfo(pItem->m_strPath))
        {
          // handle .nfo files
          CStdString strNfoFile;
          CUtil::ReplaceExtension(pItem->m_strPath, ".nfo", strNfoFile);
          if (!CFile::Exists(strNfoFile))
            strNfoFile.Empty();

          // try looking for .nfo file for a stacked item
          if (pItem->IsStack())
          {
            // first try .nfo file matching first file in stack
            CStackDirectory dir;
            CStdString firstFile = dir.GetFirstStackedFile(pItem->m_strPath);
            CUtil::ReplaceExtension(firstFile, ".nfo", strNfoFile);
            // else try .nfo file matching stacked title
            if (!CFile::Exists(strNfoFile))
            {
              CStdString stackedTitlePath = dir.GetStackedTitlePath(pItem->m_strPath);
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
                url.m_strURL = nfoReader.m_strImDbUrl;
                //url.m_strURL.push_back(nfoReader.m_strImDbUrl);
                CLog::Log(LOGDEBUG,"-- imdb url: %s", url.m_strURL.c_str());
                GetIMDBDetails(pItem, url);
                continue;
              }
              else
                CLog::Log(LOGERROR,"Unable to find an imdb url in nfo file: %s", strNfoFile.c_str());
            }
            else
              CLog::Log(LOGERROR,"Unable to cache nfo file: %s", strNfoFile.c_str());
          }
          CStdString strMovieName;

          // this causes very bad matches for files on a disc!
          if ( /* pItem->IsOnDVD() || */ pItem->IsDVDFile())
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
            strMovieName = CUtil::GetFileName(pItem->GetLabel());
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
          if (IMDB.FindMovie(strMovieName, movielist, m_dlgProgress))
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
  // GetStackedDirectory() now sets and restores the stack state!
  CFileItemList items;
  GetStackedDirectory(m_vecItems.m_strPath, items);
  DoScan(m_vecItems.m_strPath, items);
  Update(m_vecItems.m_strPath);
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
        if (!pItem->IsParentFolder())
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
  int iStack = g_stSettings.m_iMyVideoStack;
  g_stSettings.m_iMyVideoStack = STACK_SIMPLE;

  //sort list ascending by filename before stacking...
  items.Sort(SORT_METHOD_LABEL, SORT_ORDER_ASC);
  items.Stack();

  // restore stack
  g_stSettings.m_iMyVideoStack = iStack;
}

void CGUIWindowVideoFiles::SetIMDBThumbs(CFileItemList& items)
{
  VECMOVIES movies;
  m_database.GetMoviesByPath(m_vecItems.m_strPath, movies);
  for (int x = 0; x < (int)items.Size(); ++x)
  {
    CFileItem* pItem = items[x];
    if (!pItem->m_bIsFolder && pItem->GetThumbnailImage() == "")
    {
      // if a stack item, get first file
      CStdString strPath = pItem->m_strPath;
      if (pItem->IsStack())
      {
        CStackDirectory dir;
        strPath = dir.GetFirstStackedFile(pItem->m_strPath);
      }
      CStdString strFile = CUtil::GetFileName(strPath);
      //CLog::Log(LOGDEBUG,"Setting IMDB thumb for [%s] -> [%s]", pItem->m_strPath.c_str(), strFile.c_str());

      if (strFile.size() > 0)
      {
        for (int i = 0; i < (int)movies.size(); ++i)
        {
          CIMDBMovie& info = movies[i];
          CStdString strMovieFile = info.m_strFile;
          if (strMovieFile[0] == '\\' || strMovieFile[0] == '/')
            strMovieFile.Delete(0, 1);

          // stacked items
          CURL url(info.m_strPath);
          if (url.GetProtocol().Equals("stack"))
          {
            CStdString strPathTemp = info.m_strPath;
            CStdString strMoviePath;
            CUtil::AddFileToFolder(strPathTemp, strMovieFile, strMoviePath);

            // check all items in the stack
            CFileItemList tempItems;
            CStackDirectory dir;
            dir.GetDirectory(strMoviePath, tempItems);
            for (int j = 0; j < (int)tempItems.Size(); ++j)
            {
              CFileItem* tempItem = tempItems[j];
              strMovieFile = CUtil::GetFileName(tempItem->m_strPath);
              //CLog::Log(LOGDEBUG,"  Testing stack item [%s] -> [%s]", strFile.c_str(), strMovieFile.c_str());
              if (strMovieFile.Equals(strFile))
                break;
            }
          }
          //CLog::Log(LOGDEBUG,"  Testing [%s] -> [%s]", info.m_strPath.c_str(), strMovieFile.c_str());

          if (strMovieFile.Equals(strFile) /*|| pItem->GetLabel() == info.m_strTitle*/)
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
    if (m_guiState.get())
      m_guiState->SetPlaylistDirectory("");
    // activate the playlist window if its not activated yet
    if (GetID() == m_gWindowManager.GetActiveWindow() && iSize > 1)
    {
      m_gWindowManager.ActivateWindow(WINDOW_VIDEO_PLAYLIST);
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
  if ( m_vecItems.IsVirtualDirectoryRoot() )
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
      Update(m_vecItems.m_strPath);
      return ;
    }
    m_vecItems[iItem]->Select(false);
    return ;
  }
  CGUIWindowVideoBase::OnPopupMenu(iItem);
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
        CLog::Log(LOGERROR,"Could not make imdb thumb from %s", strImage.c_str());
        ::DeleteFile(strThumb.c_str());
      }
      ::DeleteFile(strTemp.c_str());
    }
  }
}

void CGUIWindowVideoFiles::OnQueueItem(int iItem)
{
  CGUIWindowVideoBase::OnQueueItem(iItem);
}
