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
#include "GUIPassword.h"
#include "GUIDialogContextMenu.h"
#include "GUIDialogMediaSource.h"
#include "FileSystem/StackDirectory.h"
#include "utils/RegExp.h"

#define CONTROL_LIST              50
#define CONTROL_THUMBS            51

#define CONTROL_PLAY_DVD          6
#define CONTROL_STACK             7
#define CONTROL_BTNSCAN           8
#define CONTROL_IMDB              9
#define CONTROL_BTNPLAYLISTS  13

CGUIWindowVideoFiles::CGUIWindowVideoFiles()
: CGUIWindowVideoBase(WINDOW_VIDEO_FILES, "MyVideo.xml")
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
        // open root
        if (strDestination.Equals("$ROOT"))
        {
          m_vecItems.m_strPath = "";
          CLog::Log(LOGINFO, "  Success! Opening root listing.");
        }
        // open playlists location
        else if (strDestination.Equals("$PLAYLISTS"))
        {
          m_vecItems.m_strPath = "special://videoplaylists/";
          CLog::Log(LOGINFO, "  Success! Opening destination path: %s", m_vecItems.m_strPath.c_str());
        }
        else
        {
          // default parameters if the jump fails
          m_vecItems.m_strPath = "";

          bool bIsBookmarkName = false;

          SetupShares();
          VECSHARES shares;
          m_rootDir.GetShares(shares);
          int iIndex = CUtil::GetMatchingShare(strDestination, shares, bIsBookmarkName);
          if (iIndex > -1)
          {
            bool bDoStuff = true;
            if (shares[iIndex].m_iHasLock == 2)
            {
              CFileItem item(shares[iIndex]);
              if (!g_passwordManager.IsItemUnlocked(&item,"video"))
              {
                m_vecItems.m_strPath = ""; // no u don't
                bDoStuff = false;
                CLog::Log(LOGINFO, "  Failure! Failed to unlock destination path: %s", strDestination.c_str());
              }
            }
            // set current directory to matching share
            if (bDoStuff)
            {
              if (bIsBookmarkName)
                m_vecItems.m_strPath=shares[iIndex].strPath;
              else
                m_vecItems.m_strPath=strDestination;
              CUtil::RemoveSlashAtEnd(m_vecItems.m_strPath);
              CLog::Log(LOGINFO, "  Success! Opened destination path: %s", strDestination.c_str());
            }
          }
          else
          {
            CLog::Log(LOGERROR, "  Failed! Destination parameter (%s) does not match a valid share!", strDestination.c_str());
          }
        }

        SetHistoryForPath(m_vecItems.m_strPath);
      }

      return CGUIWindowVideoBase::OnMessage(message);
    }
    break;

  case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();
      //if (iControl == CONTROL_BTNSCAN)
      //{
      //  OnScan();
     // }
      /*else*/ if (iControl == CONTROL_STACK)
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
        if (!m_vecItems.m_strPath.Equals("special://videoplaylists/"))
        {
          CStdString strParent = m_vecItems.m_strPath;
          UpdateButtons();
          Update("special://videoplaylists/");
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
          if (pItem->IsDVD())
            return CAutorun::PlayDisc();

          if (pItem->m_bIsShareOrDrive)
            return false;
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
          return true;
        }
      }
    }
  }
  return CGUIWindowVideoBase::OnMessage(message);
}


void CGUIWindowVideoFiles::UpdateButtons()
{
  CGUIWindowVideoBase::UpdateButtons();
  const CGUIControl *stack = GetControl(CONTROL_STACK);
  if (stack)
  {
    if (stack->GetControlType() == CGUIControl::GUICONTROL_RADIO)
    {
      SET_CONTROL_SELECTED(GetID(), CONTROL_STACK, g_stSettings.m_iMyVideoStack == STACK_SIMPLE);
      SET_CONTROL_LABEL(CONTROL_STACK, 14000);  // Stack
    }
    else
    {
      SET_CONTROL_LABEL(CONTROL_STACK, g_stSettings.m_iMyVideoStack + 14000);
    }
  }
}

bool CGUIWindowVideoFiles::GetDirectory(const CStdString &strDirectory, CFileItemList &items)
{
  if (!CGUIWindowVideoBase::GetDirectory(strDirectory, items))
    return false;

  if (!items.IsStack() && g_stSettings.m_iMyVideoStack != STACK_NONE)
    items.Stack();

  return true;
}

void CGUIWindowVideoFiles::OnPrepareFileItems(CFileItemList &items)
{
  CGUIWindowVideoBase::OnPrepareFileItems(items);
  if (g_stSettings.m_bMyVideoCleanTitles)
    items.CleanFileNames();
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

bool CGUIWindowVideoFiles::OnPlayMedia(int iItem)
{
  if ( iItem < 0 || iItem >= (int)m_vecItems.Size() ) return false;
  CFileItem* pItem = m_vecItems[iItem];

  if (pItem->IsDVD())
    return CAutorun::PlayDisc();

  if (pItem->m_bIsShareOrDrive)
  	return false;

  if (pItem->m_strPath == "add" && pItem->GetLabel() == g_localizeStrings.Get(1026)) // 'add source button' in empty root
  {
    if (CGUIDialogMediaSource::ShowAndAddMediaSource("video"))
    {
      Update("");
      return true;
    }
    return false;
  }
  else
  {
    AddFileToDatabase(pItem);

    return CGUIWindowVideoBase::OnPlayMedia(iItem);
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
  if (pItem->m_bIsFolder && pItem->IsParentFolder()) return ;
  if (pItem->m_bIsShareOrDrive) // oh no u don't
    return ;
  if (pItem->m_bIsFolder)
  {
    // IMDB is done on a folder
    // stack and then find first file in folder
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
        // check for a "CD1" folder
        if (item->GetLabel().CompareNoCase("CD1") == 0)
        {
          CFileItemList items;
          GetStackedDirectory(item->m_strPath, items);
          for (int i = 0; i < items.Size(); i++)
          {
            CFileItem *item = items[i];
            if (!item->m_bIsFolder && item->IsVideo() && !item->IsNFO() && !item->IsPlayList() )
            {
              bFoundFile = true;
              strFile = item->m_strPath;
              break;
            }
          }
          if (bFoundFile)
            break;
        }
      }
    }
    if (!bFoundFile)
    {
      // no video file in this folder?
      // then just lookup IMDB info and show it
      ShowIMDB(pItem);
      m_viewControl.SetSelectedItem(iSelectedItem);
      return ;
    }
  }

  // setup our item with the label and thumb information
  CFileItem item(strFile, false);
  item.SetLabel(pItem->GetLabel());
  item.SetCachedVideoThumb();
  if (!item.HasThumbnail()) // inherit from the original item if it exists
    item.SetThumbnailImage(pItem->GetThumbnailImage());

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

  // save our old item, as ShowIMDB() may cause this item to vanish
  CFileItem oldItem(*pItem);

  ShowIMDB(&item);
  // apply any IMDb icon to our item
  if (oldItem.m_bIsFolder)
    ApplyIMDBThumbToFolder(oldItem.m_strPath, item.GetThumbnailImage());
  Update(m_vecItems.m_strPath);
}

void CGUIWindowVideoFiles::AddFileToDatabase(const CFileItem* pItem)
{
  if (!pItem->IsVideo()) return ;
  if ( pItem->IsNFO()) return ;
  if ( pItem->IsPlayList()) return ;
 
  /* subtitles are assumed not to exist here, if we need it  */
  /* player should update this when it figures out if it has */
  m_database.AddFile(pItem->m_strPath);
//    AddMovie(pItem->m_strPath, strCDLabel, false);
}

void CGUIWindowVideoFiles::OnRetrieveVideoInfo(CFileItemList& items, const CStdString& strScraper, const CStdString& strContent, bool bDirNames)
{
  // for every file found
  for (int i = 0; i < (int)items.Size(); ++i)
  {
    g_application.ResetScreenSaver();
    CFileItem* pItem = items[i];

    // This code tests if we have a DVD folder
    if (pItem->m_bIsFolder)
    { // test to see whether we have a DVD folder
      CStdString dvdFile;
      CUtil::AddFileToFolder(pItem->m_strPath, "VIDEO_TS.IFO", dvdFile);
      if (!CFile::Exists(dvdFile))
      { // try <folder>/VIDEO_TS/VIDEO_TS.IFO
        CStdString dvdFolder;
        CUtil::AddFileToFolder(pItem->m_strPath, "VIDEO_TS", dvdFolder);
        CUtil::AddFileToFolder(dvdFolder, "VIDEO_TS.IFO", dvdFile);
      }
      if (CFile::Exists(dvdFile))
      { // have a dvd folder, set the path to that item
        // and make it a file - the code below will take care of the rest
        // note that the label is the foldername (good to lookup on)
        pItem->m_strPath = dvdFile;
        pItem->m_bIsFolder = false;
      }
    }

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
        CStdString strPath;
        CUtil::GetDirectory(pItem->m_strPath,strPath);
        AddFileToDatabase(pItem);
        m_database.SetScraperForPath(strPath,strScraper,strContent);
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
          CStdString strNfoFile = GetnfoFile(pItem);
          if ( !strNfoFile.IsEmpty() )
          {
            CLog::Log(LOGDEBUG,"Found matching nfo file: %s", strNfoFile.c_str());
            if ( CFile::Cache(strNfoFile, "Z:\\movie.nfo", NULL, NULL))
            {
              CNfoFile nfoReader;
              if ( nfoReader.Create("Z:\\movie.nfo") == S_OK)
              {
                CIMDBUrl url;
                url.m_strURL.push_back(nfoReader.m_strImDbUrl);
                //url.m_strURL.push_back(nfoReader.m_strImDbUrl);
                CLog::Log(LOGDEBUG,"-- imdb url: %s", url.m_strURL[0].c_str());
                GetIMDBDetails(pItem, url, strScraper);
                continue;
              }
              else
                CLog::Log(LOGERROR,"Unable to find an imdb url in nfo file: %s", strNfoFile.c_str());
            }
            else
              CLog::Log(LOGERROR,"Unable to cache nfo file: %s", strNfoFile.c_str());
          }

          CStdString strMovieName;
          if (bDirNames)
          {
            CStdString strPath = items.m_strPath;
            CUtil::AddSlashAtEnd(strPath);
            CUtil::GetDirectoryName(strPath,strMovieName);
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
          if (IMDB.FindMovie(strMovieName, movielist, strScraper, m_dlgProgress))
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
              GetIMDBDetails(pItem, url, strScraper);
            }
          }
        }
      }
    }
  }
}

void CGUIWindowVideoFiles::OnScan(const CStdString& strPath, const CStdString& strScraper, const CStdString& strContent)
{
  // GetStackedDirectory() now sets and restores the stack state!
  bool bDirNames=true;
  if (strContent.Equals("movies"))
  {
    bool bCanceled;
    if (CGUIDialogYesNo::ShowAndGetInput(13346,20332,-1,-1,20330,20331,bCanceled))
      bDirNames = false;
    
    if (bCanceled)
      return;
  }
  CFileItemList items;
  GetStackedDirectory(strPath, items);
  m_database.SetScraperForPath(strPath,strScraper,strContent);
  DoScan(strPath, items, strScraper, strContent, bDirNames);
  CUtil::ClearFileItemCache();
  Update(m_vecItems.m_strPath);
}

void CGUIWindowVideoFiles::OnUnAssignContent(int iItem)
{
  if (CGUIDialogYesNo::ShowAndGetInput(20339,20340,20341,20022))
  {
    m_database.RemoveContentForPath(m_vecItems[iItem]->m_strPath);
    CUtil::ClearFileItemCache();
  }
}

void CGUIWindowVideoFiles::OnAssignContent(int iItem)
{
  CFileItemList items;
  CDirectory::GetDirectory("q:\\system\\scrapers\\video",items,".xml",false);
  CGUIDialogSelect *pDialog = (CGUIDialogSelect*)m_gWindowManager.GetWindow(WINDOW_DIALOG_SELECT);
  pDialog->SetHeading(20334);
  pDialog->Reset();
  std::map<CStdString,std::vector<std::pair<CStdString,CStdString> > > scrapers; // key = content type, first = name, second = file
  for (int i=0;i<items.Size();++i)
  {
    if (!items[i]->m_bIsFolder)
    {
      TiXmlDocument doc;
      doc.LoadFile(items[i]->m_strPath);
      if (doc.RootElement())
      {
        const char* content = doc.RootElement()->Attribute("content");
        const char* name = doc.RootElement()->Attribute("name");
        if (content && name)
        {
          std::map<CStdString,std::vector<std::pair<CStdString,CStdString> > >::iterator iter=scrapers.find(content);
          if (iter != scrapers.end())
          {
            iter->second.push_back(std::make_pair<CStdString,CStdString>(name,items[i]->m_strPath));
          }
          else
          {
            std::vector<std::pair<CStdString,CStdString> > vec;
            vec.push_back(std::make_pair<CStdString,CStdString>(name,items[i]->m_strPath));
            scrapers.insert(std::make_pair<CStdString,std::vector<std::pair<CStdString,CStdString> > >(content,vec));
          }
          CStdString strContent = content;
          strContent.Replace("movies",g_localizeStrings.Get(20336));
          strContent.Replace("tvshows",g_localizeStrings.Get(20337));
          pDialog->Add(strContent);
        }
      }
    }
  }

  pDialog->EnableButton(false);
  pDialog->DoModal();
  int iDummy = pDialog->GetSelectedLabel();
  if (iDummy < 0)
    return;

  // okay, we got content - now grab which scraper
  CFileItem item = pDialog->GetSelectedItem();
  pDialog->SetHeading(20335);
  pDialog->Reset();
  CStdString strLabel = item.GetLabel();
  strLabel.Replace(g_localizeStrings.Get(20336),"movies");
  strLabel.Replace(g_localizeStrings.Get(20337),"tvshows");
  
  for (std::vector<std::pair<CStdString,CStdString> >::iterator iter = scrapers[strLabel].begin();iter != scrapers[strLabel].end();++iter)
    pDialog->Add(iter->first);

  pDialog->EnableButton(false);
  pDialog->DoModal();
  iDummy = pDialog->GetSelectedLabel();
  if (iDummy < 0) // canceled on scraper screen
    return;

  if (item.GetLabel().Equals(g_localizeStrings.Get(20336)))
  {
    if (CGUIDialogYesNo::ShowAndGetInput(13346,20342,20343,-1))
      OnScan(m_vecItems[iItem]->m_strPath,scrapers[strLabel][iDummy].second,strLabel);
    else
      m_database.SetScraperForPath(m_vecItems[iItem]->m_strPath,scrapers[strLabel][iDummy].second,strLabel);
  }
  else if (item.GetLabel().Equals(g_localizeStrings.Get(20337)))
  {
    items.Clear();
    CUtil::GetRecursiveListing(m_vecItems[iItem]->m_strPath,items,g_stSettings.m_videoExtensions);
    // enumerate
    std::vector<CStdString> expression;
    expression.push_back("[^\\.\\-_ ]*[\\.\\-_\\[ ]*[Ss]?([0-9]*)[^0-9]*[Ee]?([0-9]*)"); // foo.s01.e01, foo.1x09.avi
    expression.push_back("[^\\.\\-_ ]*[^\\.\\-_ ]([0-9])([0-9]*)"); // foo.103*

    std::map<int,std::vector<std::pair<int,CStdString> > > episodes;
    for (int i=0;i<items.Size();++i)
    {
      if (items[i]->m_bIsFolder)
        continue;
      for (unsigned int j=0;j<expression.size();++j)
      {
        CRegExp reg;
        if (!reg.RegComp(expression[j]))
          break;
        CLog::DebugLog("label: %s",items[i]->GetLabel().c_str());
        if (reg.RegFind(items[i]->GetLabel().c_str()) > -1)
        {
          char* season = reg.GetReplaceString("\\1");
          char* episode = reg.GetReplaceString("\\2");
          if (season && episode)
          {
            int iSeason = atoi(season);
            int iEpisode = atoi(episode);
            std::map<int,std::vector<std::pair<int,CStdString> > >::iterator iter = episodes.find(iSeason);
            if (iter == episodes.end()) // none from this season - add to map
            {
              std::vector<std::pair<int,CStdString> > vec;
              vec.push_back(std::make_pair<int,CStdString>(iEpisode,items[i]->m_strPath));
              episodes.insert(std::make_pair<int,std::vector<std::pair<int,CStdString> > >(iSeason,vec));
            }
            else
            {
              iter->second.push_back(std::make_pair<int,CStdString>(iEpisode,items[i]->m_strPath));
            }
            break;
          }
        }
      }
    }
  }
}

bool CGUIWindowVideoFiles::DoScan(const CStdString &strPath, CFileItemList& items, const CStdString& strScraper, const CStdString& strContent, bool bDirNames)
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
    m_dlgProgress->StartModal();
  }

  OnRetrieveVideoInfo(items,strScraper,strContent,bDirNames);

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
        if (!pItem->IsParentFolder() && pItem->GetLabel().CompareNoCase("sample") != 0)
        {
          // load subfolder
          CFileItemList subDirItems;
          GetStackedDirectory(pItem->m_strPath, subDirItems);
          if (m_dlgProgress)
            m_dlgProgress->Close();
          if (!DoScan(pItem->m_strPath, subDirItems, strScraper, strContent,bDirNames))
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

  /*bool bUnroll = m_guiState->UnrollArchives();
  g_guiSettings.SetBool("filelists.unrollarchives",true);*/

  items.Stack();

  // restore stack
  g_stSettings.m_iMyVideoStack = iStack;
  //g_guiSettings.SetBool("VideoFiles.FileLists",bUnroll);
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
      m_guiState->SetPlaylistDirectory("playlistvideo://");
    // activate the playlist window if its not activated yet
    if (GetID() == m_gWindowManager.GetActiveWindow() && iSize > 1)
    {
      m_gWindowManager.ActivateWindow(WINDOW_VIDEO_PLAYLIST);
    }
  }
}

void CGUIWindowVideoFiles::OnPopupMenu(int iItem, bool bContextDriven /* = true */)
{
  // calculate our position
  float posX = 200;
  float posY = 100;
  const CGUIControl *pList = GetControl(CONTROL_LIST);
  if (pList)
  {
    posX = pList->GetXPosition() + pList->GetWidth() / 2;
    posY = pList->GetYPosition() + pList->GetHeight() / 2;
  }
  if ( m_vecItems.IsVirtualDirectoryRoot() )
  {
    if (iItem < 0)
    { // TODO: We should add the option here for shares to be added if there aren't any
      return ;
    }
    // mark the item
    m_vecItems[iItem]->Select(true);

    // and do the popup menu
    if (CGUIDialogContextMenu::BookmarksMenu("video", m_vecItems[iItem], posX, posY))
    {
      Update(m_vecItems.m_strPath);
      return ;
    }
    m_vecItems[iItem]->Select(false);
    return ;
  }
  CGUIWindowVideoBase::OnPopupMenu(iItem, bContextDriven);
}

void CGUIWindowVideoFiles::GetIMDBDetails(CFileItem *pItem, CIMDBUrl &url, const CStdString& strScraper)
{
  CIMDB IMDB;
  CIMDBMovie movieDetails;
  movieDetails.m_strSearchString = pItem->m_strPath;
  if ( IMDB.GetDetails(url, movieDetails, strScraper, m_dlgProgress) )
  {
    // add to all movies in the stacked set
    m_database.SetMovieInfo(pItem->m_strPath, movieDetails);
    // get & save thumbnail
    CStdString strThumb = "";
    CStdString strImage = movieDetails.m_strPictureURL;
    if (strImage.size() > 0 && movieDetails.m_strSearchString.size() > 0)
    {
      // check for a cached thumb or user thumb
      pItem->SetVideoThumb();
      if (pItem->HasThumbnail())
        return;
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
  }
}

void CGUIWindowVideoFiles::OnQueueItem(int iItem)
{
  CGUIWindowVideoBase::OnQueueItem(iItem);
}
