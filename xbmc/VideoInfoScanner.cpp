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
#include "VideoInfoScanner.h"
#include "FileSystem/DirectoryCache.h"
#include "GUIWindowVideoFiles.h"
#include "Util.h"
#include "nfofile.h"
#include "utils/RegExp.h"
#include "Picture.h"
#include "FileSystem/StackDirectory.h"

using namespace DIRECTORY;
using namespace XFILE;

CVideoInfoScanner::CVideoInfoScanner()
{
  m_bRunning = false;
  m_pObserver = NULL;
  m_bCanInterrupt = false;
  m_currentItem=0;
  m_itemCount=0;
}

CVideoInfoScanner::~CVideoInfoScanner()
{
}

void CVideoInfoScanner::Process()
{
  try
  {
    DWORD dwTick = timeGetTime();

    m_database.Open();

    if (m_pObserver)
      m_pObserver->OnStateChanged(PREPARING);

    m_bCanInterrupt = true;

    // check whether we have content set for the path.
    int iFound;
    m_database.GetScraperForPath(m_strStartDir,m_info.strPath,m_info.strContent,iFound);

    CLog::Log(LOGDEBUG, __FUNCTION__" - Starting scan");

    bool bOKtoScan = true;
    if (m_bUpdateAll)
    {
      if (m_pObserver)
        m_pObserver->OnStateChanged(REMOVING_OLD);

      m_database.RemoveContentForPath(m_strStartDir);
    }

    if (bOKtoScan)
    {
      if (m_pObserver)
        m_pObserver->OnStateChanged(FETCHING_VIDEO_INFO);

      // Reset progress vars
      m_currentItem=0;
      m_itemCount=-1;

      // Create the thread to count all files to be scanned
      CThread fileCountReader(this);
      if (m_pObserver)
        fileCountReader.Create();

      // Database operations should not be canceled
      // using Interupt() while scanning as it could
      // result in unexpected behaviour.
      m_bCanInterrupt = false;

      bool bCommit = false;
      if (bOKtoScan)
        bCommit = DoScan(m_strStartDir,m_settings);

      if (bCommit)
      {
        m_database.CommitTransaction();

        if (m_bUpdateAll)
        {
          if (m_pObserver)
            m_pObserver->OnStateChanged(COMPRESSING_DATABASE);

          m_database.Compress();
        }
      }
      else
        m_database.RollbackTransaction();

      fileCountReader.StopThread();

    }
    else
      m_database.RollbackTransaction();

    m_database.Close();
    CLog::Log(LOGDEBUG, __FUNCTION__" - Finished scan");

    dwTick = timeGetTime() - dwTick;
    CStdString strTmp, strTmp1;
    StringUtils::SecondsToTimeString(dwTick / 1000, strTmp1);
    strTmp.Format("My Videos: Scanning for video info using worker thread, operation took %s", strTmp1);
    CLog::Log(LOGNOTICE, strTmp.c_str());

    m_bRunning = false;
    if (m_pObserver)
      m_pObserver->OnFinished();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "VideoInfoScanner: Exception while scanning.");
  }
}

void CVideoInfoScanner::Start(const CStdString& strDirectory, const SScraperInfo& info, const SScanSettings& settings, bool bUpdateAll)
{
  m_strStartDir = strDirectory;
  m_bUpdateAll = bUpdateAll;
  m_settings = settings;
  m_info = info;
  StopThread();
  Create();
  m_bRunning = true;
}

bool CVideoInfoScanner::IsScanning()
{
  return m_bRunning;
}

void CVideoInfoScanner::Stop()
{
  if (m_bCanInterrupt)
    m_database.Interupt();

  StopThread();
}

void CVideoInfoScanner::SetObserver(IVideoInfoScannerObserver* pObserver)
{
  m_pObserver = pObserver;
}

bool CVideoInfoScanner::DoScan(const CStdString& strDirectory, const SScanSettings& settings)
{
  if (m_pObserver)
    m_pObserver->OnDirectoryChanged(strDirectory);

  CLog::Log(LOGDEBUG, __FUNCTION__" - Scanning dir: %s", strDirectory.c_str());
  // load subfolder
  CFileItemList items;
  CGUIWindowVideoFiles* pWindow = (CGUIWindowVideoFiles*)m_gWindowManager.GetWindow(WINDOW_VIDEO_FILES);
  int iFound;
  m_database.GetScraperForPath(strDirectory,m_info.strPath,m_info.strContent,iFound);
  if (m_info.strContent.Equals("movies"))
  {
    pWindow->GetStackedDirectory(strDirectory, items);
  }
  else if (m_info.strContent.Equals("tvshows"))
  {
    if (iFound == 1)
    {
      CDirectory::GetDirectory(strDirectory,items,g_stSettings.m_videoExtensions);
    }
    else
    {
      CFileItem item(CUtil::GetFileName(strDirectory));
      item.m_strPath = strDirectory;
      item.m_bIsFolder = true;
      items.Add(new CFileItem(item));
      CUtil::GetParentPath(item.m_strPath,items.m_strPath);
    }
  }
  items.Sort(SORT_METHOD_LABEL, SORT_ORDER_ASC);

  RetrieveVideoInfo(items,settings.parent_name_root,m_info);
  if (1) // TODO
  {
    if (m_pObserver)
      m_pObserver->OnDirectoryScanned(strDirectory);
  }
  CLog::Log(LOGDEBUG, __FUNCTION__" - Finished dir: %s", strDirectory.c_str());

  for (int i = 0; i < items.Size(); ++i)
  {
    CFileItem *pItem = items[i];

    if (m_bStop)
      break;
    // if we have a directory item (non-playlist) we then recurse into that folder
    if (pItem->m_bIsFolder && !pItem->GetLabel().Equals("sample") && !pItem->IsParentFolder() && !pItem->IsPlayList() && settings.recurse && !m_info.strContent.Equals("tvshows")) // do not recurse for tv shows - we have already looked recursively for episodes
    {
      CStdString strPath=pItem->m_strPath;
      SScanSettings settings2(settings);
      settings2.recurse--;
      settings2.parent_name_root = settings2.parent_name;
      if (!DoScan(strPath,settings2))
      {
        m_bStop = true;
      }
    }
  }
  return !m_bStop;
}

bool CVideoInfoScanner::RetrieveVideoInfo(CFileItemList& items, bool bDirNames, const SScraperInfo& info, CIMDBUrl* pURL /* = NULL */, CGUIDialogProgress* m_dlgProgress /* = NULL */)
{
  CStdString strMovieName;
  CIMDB IMDB;
  IMDB.SetScraperInfo(info);

  if (bDirNames && info.strContent.Equals("movies"))
  {
    strMovieName = items.m_strPath;
    CUtil::RemoveSlashAtEnd(strMovieName);
    strMovieName = CUtil::GetFileName(strMovieName);
  }

  if (m_dlgProgress)
  {
    if (items.Size() > 1 || items[0]->m_bIsFolder)
    {
      m_dlgProgress->ShowProgressBar(true);
      m_dlgProgress->SetPercentage(0);
    }
    else
      m_dlgProgress->ShowProgressBar(false);
    
    m_dlgProgress->Progress();
  }

  // for every file found
  IMDB_EPISODELIST episodes;
  long lTvShowId = -1;
  m_database.Open();
  for (int i = 0; i < (int)items.Size(); ++i)
  {
    CFileItem* pItem = items[i];
    if (info.strContent.Equals("movies"))
    {
      if (m_pObserver)
      {
        m_pObserver->OnSetCurrentProgress(i,items.Size());
        if (!pItem->m_bIsFolder)
          m_pObserver->OnSetProgress(m_currentItem++,m_itemCount);
      }
    }
    if (info.strContent.Equals("tvshows"))
    {
      //if (!pItem->m_bIsFolder) // we only want folders - files are handled in onprocessseriesfolder
      //  continue;
      long lTvShowId2;
      if (pItem->m_bIsFolder)
        lTvShowId2 = m_database.GetTvShowInfo(pItem->m_strPath);
      else
      {
        CStdString strPath;
        CUtil::GetDirectory(pItem->m_strPath,strPath);
        lTvShowId2 = m_database.GetTvShowInfo(strPath);
      }
      if (lTvShowId2 > -1)
      {
        if (lTvShowId2 != lTvShowId)
        {
          lTvShowId = lTvShowId2;
          // fetch episode guide
          CVideoInfoTag details;
          m_database.GetTvShowInfo(pItem->m_strPath,details,lTvShowId);
          CIMDBUrl url;
          //convert m_strEpisodeGuide in url.m_scrURL
          url.Parse(details.m_strEpisodeGuide);
          if (m_dlgProgress)
          {
            if (pItem->m_bIsFolder)
              m_dlgProgress->SetHeading(20353);
            else
              m_dlgProgress->SetHeading(20361);
            m_dlgProgress->SetLine(0, pItem->GetLabel());
            m_dlgProgress->SetLine(1,details.m_strTitle);
            m_dlgProgress->SetLine(2,20354);
            m_dlgProgress->Progress();
          }
          if (!IMDB.GetEpisodeList(url,episodes))
          {
            if (m_dlgProgress)
              m_dlgProgress->Close();
            m_database.RollbackTransaction();
            m_database.Close();
            return false;
          }
        }
        if (m_bStop || (m_dlgProgress && m_dlgProgress->IsCanceled()))
        {
          if (m_dlgProgress)
            m_dlgProgress->Close();
          m_database.RollbackTransaction();
          m_database.Close();
          return false;
        }
        if (m_pObserver)
          m_pObserver->OnDirectoryChanged(pItem->m_strPath);

        OnProcessSeriesFolder(episodes,pItem,lTvShowId2,IMDB,m_dlgProgress);
        continue;
      }
      else
      {
        SScraperInfo info2;
        int iFound;
        m_database.GetScraperForPath(items.m_strPath,info2.strPath,info2.strContent,iFound);
        if (iFound != 1) // we need this when we scan a non-root tvshow folder
        {
          CUtil::GetParentPath(pItem->m_strPath,strMovieName);
          CUtil::RemoveSlashAtEnd(strMovieName);
          strMovieName = CUtil::GetFileName(strMovieName);
          *pItem = items;
        }
        else
        {
          CUtil::RemoveSlashAtEnd(pItem->m_strPath);
          strMovieName = CUtil::GetFileName(pItem->m_strPath);
        }
      }
    }

    if (!pItem->m_bIsFolder || info.strContent.Equals("tvshows"))
    {
      if ((pItem->IsVideo() && !pItem->IsNFO() && !pItem->IsPlayList()) || info.strContent.Equals("tvshows") )
      {
        if (!bDirNames && !info.strContent.Equals("tvshows"))
        {
          if(pItem->IsLabelPreformated())
            strMovieName = pItem->GetLabel();
          else
          {
            if (pItem->IsStack())
              strMovieName = pItem->GetLabel();
            else
              strMovieName = CUtil::GetFileName(pItem->m_strPath);
            CUtil::RemoveExtension(strMovieName);
          }
        }

        if (m_dlgProgress)
        {
          int iString=198;
          if (info.strContent.Equals("tvshows"))
          {
            if (pItem->m_bIsFolder)
              iString = 20353;
            else
              iString = 20361;
          }
          m_dlgProgress->SetHeading(iString);
          m_dlgProgress->SetLine(0, pItem->GetLabel());
          m_dlgProgress->SetLine(2,"");
          m_dlgProgress->Progress();
          if (m_dlgProgress->IsCanceled()) 
          {
            m_dlgProgress->Close();
            m_database.RollbackTransaction();
            m_database.Close();
            return false;
          }
        }
        if (m_bStop)
        {
          m_database.RollbackTransaction();
          m_database.Close();
          return false;
        }
        if (info.strContent.Equals("movies"))
        {
          if (!m_database.HasMovieInfo(pItem->m_strPath))
          {
            // handle .nfo files
            CStdString strNfoFile = GetnfoFile(pItem);
            if (!strNfoFile.IsEmpty())
            {
              CLog::Log(LOGDEBUG,"Found matching nfo file: %s", strNfoFile.c_str());
              CNfoFile nfoReader(info.strContent);
              if (nfoReader.Create(strNfoFile) == S_OK)
              {
                if (nfoReader.m_strScraper == "NFO")
                {
                  CLog::Log(LOGDEBUG, __FUNCTION__" Got details from nfo");
                  CVideoInfoTag movieDetails;
                  nfoReader.GetDetails(movieDetails);
                  AddMovieAndGetThumb(pItem, "movies", movieDetails, -1, m_dlgProgress);
                  continue;
                }
                else
                {
                  CIMDBUrl url;
                  CScraperUrl scrUrl(nfoReader.m_strImDbUrl); 
                  url.m_scrURL.push_back(scrUrl);
                  CLog::Log(LOGDEBUG,"-- nfo-scraper: %s", nfoReader.m_strScraper.c_str());
                  CLog::Log(LOGDEBUG,"-- nfo url: %s", url.m_scrURL[0].m_url.c_str());
                  url.m_strID  = nfoReader.m_strImDbNr;
                  SScraperInfo info2(info);
                  info2.strPath = nfoReader.m_strScraper;
                  GetIMDBDetails(pItem, url, info2, m_dlgProgress);
                  continue;
                }
              }
              else
                CLog::Log(LOGERROR,"Unable to find an imdb url in nfo file: %s", strNfoFile.c_str());
            }
          }
          else
            continue;
        }

        IMDB_MOVIELIST movielist;
        if (pURL || IMDB.FindMovie(strMovieName, movielist, m_dlgProgress))
        {
          CIMDBUrl url;
          int iMoviesFound=1;
          if (!pURL)
          {
            iMoviesFound = movielist.size();
            if (iMoviesFound)
              url = movielist[0];
          }
          else
          {
            url = *pURL;
          }
          if (iMoviesFound > 0)
          {
            CUtil::ClearCache();
            long lResult=GetIMDBDetails(pItem, url, info);
            if (info.strContent.Equals("tvshows"))
            {
              // fetch episode guide
              CVideoInfoTag details;
              m_database.GetTvShowInfo(pItem->m_strPath,details,lResult);
              CIMDBUrl url;
              url.Parse(details.m_strEpisodeGuide);
              IMDB_EPISODELIST episodes;
              if (IMDB.GetEpisodeList(url,episodes))
              {
                if (m_pObserver)
                  m_pObserver->OnDirectoryChanged(pItem->m_strPath);
                OnProcessSeriesFolder(episodes,pItem,lResult,IMDB,m_dlgProgress);
              }
            }
          }
        }
      }
    }
  }
  if(m_dlgProgress)
    m_dlgProgress->ShowProgressBar(false);

  m_database.Close();
  return true;
}

// This function is run by another thread
void CVideoInfoScanner::Run()
{
  m_itemCount=CountFiles(m_strStartDir);
}

// Recurse through all folders we scan and count files
int CVideoInfoScanner::CountFiles(const CStdString& strPath)
{
  int count=0;
  // load subfolder
  CFileItemList items;
  CLog::Log(LOGDEBUG, __FUNCTION__" - processing dir: %s", strPath.c_str());
  CDirectory::GetDirectory(strPath, items, g_stSettings.m_videoExtensions, true);
  if (m_info.strContent.Equals("movies"))
    items.Stack();

  for (int i=0; i<items.Size(); ++i)
  {
    CFileItem* pItem=items[i];

    if (m_bStop)
      return 0;

    if (pItem->m_bIsFolder)
      count+=CountFiles(pItem->m_strPath);
    else if (pItem->IsVideo() && !pItem->IsPlayList() && !pItem->IsNFO())
      count++;
  }
  CLog::Log(LOGDEBUG, __FUNCTION__" - finished processing dir: %s", strPath.c_str());
  return count;
}

void CVideoInfoScanner::EnumerateSeriesFolder(const CFileItem* item, IMDB_EPISODELIST& episodeList)
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
    CUtil::RemoveSlashAtEnd(strPath);
    if (CUtil::GetFileName(strPath).Equals("sample"))
      continue;
    for (unsigned int j=0;j<expression.size();++j)
    {
      CRegExp reg;
      if (!reg.RegComp(expression[j]))
        break;
      CStdString strLabel=items[i]->m_strPath;
      strLabel.MakeLower();
      CLog::Log(LOGDEBUG,"running expression %s on label %s",expression[j].c_str(),strLabel.c_str());
      if (reg.RegFind(strLabel.c_str()) > -1)
      {
        char* season = reg.GetReplaceString("\\1");
        char* episode = reg.GetReplaceString("\\2");
        if (season && episode)
        {
          CLog::Log(LOGDEBUG,"found match %s %s %s",strLabel.c_str(),season,episode);
          int iSeason = atoi(season);
          int iEpisode = atoi(episode);
          std::pair<int,int> key(iSeason,iEpisode);
          CIMDBUrl url;
          url.m_scrURL.push_back(CScraperUrl(items[i]->m_strPath));
          episodeList.insert(std::make_pair<std::pair<int,int>,CIMDBUrl>(key,url));
          if (j >= iTwoParters)
            break;
        }
      }
    }
  }
}

long CVideoInfoScanner::AddMovieAndGetThumb(CFileItem *pItem, const CStdString &content, const CVideoInfoTag &movieDetails, long idShow, CGUIDialogProgress* pDialog /* == NULL */)
{
  long lResult=-1;
  // add to all movies in the stacked set
  if (content.Equals("movies"))
    m_database.SetDetailsForMovie(pItem->m_strPath, movieDetails);
  else if (content.Equals("tvshows"))
  {
    if (pItem->m_bIsFolder)
    {
      lResult=m_database.SetDetailsForTvShow(pItem->m_strPath, movieDetails);
    }
    else
    {
      long lEpisodeId = m_database.AddEpisode(idShow,pItem->m_strPath);

      lResult=m_database.SetDetailsForEpisode(pItem->m_strPath,movieDetails,idShow, lEpisodeId);
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
    CHTTP http;
    if (pDialog)
    {
      pDialog->SetLine(2, 415);
      pDialog->Progress();
    }

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

void CVideoInfoScanner::OnProcessSeriesFolder(IMDB_EPISODELIST& episodes, const CFileItem* item, long lShowId, CIMDB& IMDB, CGUIDialogProgress* pDlgProgress /* = NULL */)
{
  if (pDlgProgress)
  {
    pDlgProgress->SetLine(2, 20361);
    pDlgProgress->SetPercentage(0);
    pDlgProgress->ShowProgressBar(true);
    pDlgProgress->Progress();
  }
  IMDB_EPISODELIST files;
  EnumerateSeriesFolder(item,files);

  int iMax = files.size();
  int iCurr = 0;
  m_database.Open();
  for (IMDB_EPISODELIST::iterator iter = files.begin();iter != files.end();++iter)
  {
    if (pDlgProgress)
    {
      pDlgProgress->SetLine(2, 20361);
      pDlgProgress->SetPercentage((int)((float)(iCurr++)/iMax*100));
      pDlgProgress->Progress();
    }
    if (m_pObserver)
    {
      m_pObserver->OnSetProgress(m_currentItem++,m_itemCount);
      m_pObserver->OnSetCurrentProgress(iCurr++,iMax);
    }
    IMDB_EPISODELIST::iterator iter2 = episodes.find(iter->first);
    if (iter2 != episodes.end())
    {
      if (pDlgProgress && pDlgProgress->IsCanceled() || m_bStop)
      {
        if (pDlgProgress)
          pDlgProgress->Close();
        m_database.RollbackTransaction();
        m_database.Close();
        return;
      }

      CVideoInfoTag episodeDetails;
      if (m_database.GetEpisodeInfo(iter->second.m_scrURL[0].m_url,iter2->first.second) > -1)
        continue;

      if (!IMDB.GetEpisodeDetails(iter2->second,episodeDetails,pDlgProgress))
        break;
      episodeDetails.m_iSeason = iter2->first.first;
      episodeDetails.m_iEpisode = iter2->first.second;
      CFileItem item;
      item.m_strPath = iter->second.m_scrURL[0].m_url;
      AddMovieAndGetThumb(&item,"tvshows",episodeDetails,lShowId);
    }
  }
  m_database.Close();
}

CStdString CVideoInfoScanner::GetnfoFile(CFileItem *item)
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

long CVideoInfoScanner::GetIMDBDetails(CFileItem *pItem, CIMDBUrl &url, const SScraperInfo& info, CGUIDialogProgress* pDialog /* = NULL */)
{
  CIMDB IMDB;
  CVideoInfoTag movieDetails;
  IMDB.SetScraperInfo(info);

  if ( IMDB.GetDetails(url, movieDetails, pDialog) )
    return AddMovieAndGetThumb(pItem, info.strContent, movieDetails, -1);
  return -1;
}
