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
#include "Util.h"
#include "nfofile.h"
#include "utils/RegExp.h"
#include "utils/md5.h"
#include "Picture.h"
#include "FileSystem/StackDirectory.h"
#include "xbox/xkgeneral.h"

#define REGEXSAMPLEFILE "[-\\._ ]sample[-\\._ ]"

using namespace DIRECTORY;
using namespace XFILE;

namespace VIDEO 
{

  CVideoInfoScanner::CVideoInfoScanner()
  {
    m_bRunning = false;
    m_pObserver = NULL;
    m_bCanInterrupt = false;
    m_currentItem=0;
    m_itemCount=0;
    m_bClean=false;
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

      CLog::Log(LOGDEBUG, __FUNCTION__" - Starting scan");

      // Reset progress vars
      m_currentItem=0;
      m_itemCount=-1;

      // Create the thread to count all files to be scanned
      SetPriority(THREAD_PRIORITY_IDLE);
      CThread fileCountReader(this);    
      if (m_pObserver)
        fileCountReader.Create();

      // Database operations should not be canceled
      // using Interupt() while scanning as it could
      // result in unexpected behaviour.
      m_bCanInterrupt = false;

      bool bCancelled = false;
      for(std::map<CStdString,VIDEO::SScanSettings>::iterator it = m_pathsToScan.begin(); it != m_pathsToScan.end(); it++)
      {
        if(!DoScan(it->first, it->second))
        {
          bCancelled = true;
          break;
        }
      }

      if (!bCancelled)
      {
        m_database.CommitTransaction();

        if (m_bClean)
          m_database.CleanDatabase(m_pObserver);
        if (m_pObserver)
          m_pObserver->OnStateChanged(COMPRESSING_DATABASE);
        m_database.Compress();
      }
      else
        m_database.RollbackTransaction();

      fileCountReader.StopThread();

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
    m_info = info;
    m_pathsToScan.clear();

    if (strDirectory.IsEmpty())
    { // scan all paths in the database.  We do this by scanning all paths in the db, and crossing them off the list as
      // we go.
      m_database.Open();
      m_database.GetPaths(m_pathsToScan);
      m_bClean = true;
      m_database.Close();
    }
    else
    {
      m_pathsToScan.insert(pair<CStdString,SScanSettings>(strDirectory,settings));
      m_bClean = false;
    }

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

  bool CVideoInfoScanner::DoScan(const CStdString& strDirectory, SScanSettings settings)
  {
    if (m_bUpdateAll)
    {
      if (m_pObserver)
        m_pObserver->OnStateChanged(REMOVING_OLD);

      m_database.RemoveContentForPath(strDirectory);
    }

    if (m_pObserver)
      m_pObserver->OnDirectoryChanged(strDirectory);

    // load subfolder
    CFileItemList items;
    int iFound;
    bool bSkip=false;
    m_database.GetScraperForPath(strDirectory,m_info,settings, iFound);
    if (m_info.strContent.IsEmpty() || m_info.strContent.Equals("None"))
      bSkip = true;

    CStdString hash, dbHash;
    if (m_info.strContent.Equals("movies"))
    {
      CDirectory::GetDirectory(strDirectory,items,g_stSettings.m_videoExtensions);
      int iOldStack = g_stSettings.m_iMyVideoStack;
      g_stSettings.m_iMyVideoStack = STACK_SIMPLE;
      items.Stack();
      g_stSettings.m_iMyVideoStack = iOldStack;
      int numFilesInFolder = GetPathHash(items, hash);

      if (!m_database.GetPathHash(strDirectory, dbHash) || dbHash != hash)
      { // path has changed - rescan
        if (dbHash.IsEmpty())
          CLog::Log(LOGDEBUG, __FUNCTION__" Scanning dir '%s' as not in the database", strDirectory.c_str());
        else
          CLog::Log(LOGDEBUG, __FUNCTION__" Rescanning dir '%s' due to change", strDirectory.c_str());
      }
      else
      {
        CLog::Log(LOGDEBUG, __FUNCTION__" Skipping dir '%s' due to no change", strDirectory.c_str());
        m_currentItem += numFilesInFolder;

        // notify our observer of our progress
        if (m_pObserver)
        {
          if (m_itemCount>0)
            m_pObserver->OnSetProgress(m_currentItem, m_itemCount);
          m_pObserver->OnDirectoryScanned(strDirectory);
        }
        bSkip = true;
      }
    }
    else if (m_info.strContent.Equals("tvshows"))
    {
      if (iFound == 1 && !settings.parent_name_root)
      {
        CDirectory::GetDirectory(strDirectory,items,g_stSettings.m_videoExtensions);
        GetPathHash(items, hash);
        bSkip = true;
        if (!m_database.GetPathHash(strDirectory, dbHash) || dbHash != hash)
        {
          m_database.SetPathHash(strDirectory, hash);
          bSkip = false;
        }
        else
          items.Clear();
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
    else if (m_info.strContent.Equals("musicvideos"))
    {
      CDirectory::GetDirectory(strDirectory,items,g_stSettings.m_videoExtensions);
      int numFilesInFolder = GetPathHash(items, hash);

      if (!m_database.GetPathHash(strDirectory, dbHash) || dbHash != hash)
      { // path has changed - rescan
        if (dbHash.IsEmpty())
          CLog::Log(LOGDEBUG, __FUNCTION__" Scanning dir '%s' as not in the database", strDirectory.c_str());
        else
          CLog::Log(LOGDEBUG, __FUNCTION__" Rescanning dir '%s' due to change", strDirectory.c_str());
      }
      else
      {
        CLog::Log(LOGDEBUG, __FUNCTION__" Skipping dir '%s' due to no change", strDirectory.c_str());
        m_currentItem += numFilesInFolder;

        // notify our observer of our progress
        if (m_pObserver)
        {
          if (m_itemCount>0)
            m_pObserver->OnSetProgress(m_currentItem, m_itemCount);
          m_pObserver->OnDirectoryScanned(strDirectory);
        }
        bSkip = true;
      }
    }
    
    if (!bSkip)
    {
      if (m_pObserver)
        m_pObserver->OnStateChanged(FETCHING_VIDEO_INFO);

      RetrieveVideoInfo(items,settings.parent_name_root,m_info);
      if (m_info.strContent.Equals("movies") || m_info.strContent.Equals("musicvideos"))
        m_database.SetPathHash(strDirectory, hash);
    }
    if (m_pObserver)
      m_pObserver->OnDirectoryScanned(strDirectory);
    CLog::Log(LOGDEBUG, __FUNCTION__" - Finished dir: %s", strDirectory.c_str());

    for (int i = 0; i < items.Size(); ++i)
    {
      CFileItem *pItem = items[i];

      if (m_bStop)
        break;
      // if we have a directory item (non-playlist) we then recurse into that folder
      if (pItem->m_bIsFolder && !pItem->GetLabel().Equals("sample") && !pItem->IsParentFolder() && !pItem->IsPlayList() && settings.recurse > 0 && !m_info.strContent.Equals("tvshows")) // do not recurse for tv shows - we have already looked recursively for episodes
      {
        CStdString strPath=pItem->m_strPath;
        SScanSettings settings2;
        settings2.recurse = settings.recurse-1;
        settings2.parent_name_root = settings.parent_name;
        settings2.parent_name = settings.parent_name;
        if (!DoScan(strPath,settings2))
        {
          m_bStop = true;
        }
      }
    }
    return !m_bStop;
  }

  bool CVideoInfoScanner::RetrieveVideoInfo(CFileItemList& items, bool bDirNames, const SScraperInfo& info, bool bRefresh, CIMDBUrl *pURL, CGUIDialogProgress* m_dlgProgress)
  {
    CStdString strMovieName;
    CIMDB IMDB;
    IMDB.SetScraperInfo(info);
    CRegExp regExSample;
    
    if (!regExSample.RegComp(REGEXSAMPLEFILE))
    {
      CLog::Log(LOGERROR, "Unable to compile RegExp for Sample file");
    }

    if (bDirNames && info.strContent.Equals("movies"))
    {
      strMovieName = items.m_strPath;
      CUtil::RemoveSlashAtEnd(strMovieName);
      strMovieName = CUtil::GetFileName(strMovieName);
    }

    if (m_dlgProgress)
    {
      if (items.Size() > 1 || items[0]->m_bIsFolder && !bRefresh)
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
    IMDB_EPISODELIST files;
    long lTvShowId = -1;
    m_database.Open();
    for (int i = 0; i < (int)items.Size(); ++i)
    {
      CFileItem* pItem = items[i];
      SScraperInfo info2;
      if (pItem->m_bIsFolder)
        m_database.GetScraperForPath(pItem->m_strPath,info2);
      else
        m_database.GetScraperForPath(items.m_strPath,info2);

      if (info2.strContent.Equals("None")) // skip
        continue;

      IMDB.SetScraperInfo(info2);
      // Discard all possible sample files defined by regExSample
      if (regExSample.RegFind(CUtil::GetFileName(pItem->m_strPath)) > -1)
      {
        continue;
      }
 
      if (info.strContent.Equals("movies") || info.strContent.Equals("musicvideos"))
      {
        if (m_pObserver)
        {
          m_pObserver->OnSetCurrentProgress(i,items.Size());
          if (!pItem->m_bIsFolder && m_itemCount)
            m_pObserver->OnSetProgress(m_currentItem++,m_itemCount);
        }

      }
      if (info.strContent.Equals("tvshows"))
      {
        long lTvShowId2;
        if (pItem->m_bIsFolder)
          lTvShowId2 = m_database.GetTvShowInfo(pItem->m_strPath);
        else
        {
          CStdString strPath;
          CUtil::GetDirectory(pItem->m_strPath,strPath);
          lTvShowId2 = m_database.GetTvShowInfo(strPath);
        }
        if (lTvShowId2 > -1 && (!bRefresh || !pItem->m_bIsFolder))
        {
          if (lTvShowId2 != lTvShowId)
          {
            lTvShowId = lTvShowId2;
            // fetch episode guide
            CVideoInfoTag details;
            m_database.GetTvShowInfo(pItem->m_strPath,details,lTvShowId);
            files.clear();
            EnumerateSeriesFolder(pItem,files);
            if (files.size() == 0) // no update or no files
              continue;

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

          OnProcessSeriesFolder(episodes,files,lTvShowId2,IMDB,m_dlgProgress);
          continue;
        }
        else
        {
          CUtil::RemoveSlashAtEnd(pItem->m_strPath);
          strMovieName = CUtil::GetFileName(pItem->m_strPath);
          CUtil::AddSlashAtEnd(pItem->m_strPath);
        }
      }

      if (!pItem->m_bIsFolder || info.strContent.Equals("tvshows"))
      {
        if ((pItem->IsVideo() && !pItem->IsNFO() && !pItem->IsPlayList()) || info.strContent.Equals("tvshows") )
        {
          if (!bDirNames && !info.strContent.Equals("tvshows"))
          {
            if (pItem->IsLabelPreformated())
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
            if (info.strContent.Equals("musicvideos"))
              iString = 20394;
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
              CStdString strNfoFile = GetnfoFile(pItem,bDirNames);
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
                    AddMovieAndGetThumb(pItem, "movies", movieDetails, -1, bDirNames, m_dlgProgress);
                    continue;
                  }
                  else
                  {
                    CIMDBUrl url;
                    CScraperUrl scrUrl(nfoReader.m_strImDbUrl); 
                    url.m_scrURL.push_back(scrUrl);
                    CLog::Log(LOGDEBUG,"-- nfo-scraper: %s", nfoReader.m_strScraper.c_str());
                    CLog::Log(LOGDEBUG,"-- nfo url: %s", url.m_scrURL[0].m_url[0].m_url.c_str());
                    url.m_strID  = nfoReader.m_strImDbNr;
                    SScraperInfo info2(info);
                    info2.strPath = nfoReader.m_strScraper;
                    GetIMDBDetails(pItem, url, info2, bDirNames, m_dlgProgress);
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
          else if (info.strContent.Equals("musicvideos"))
          {
            if (!m_database.HasMusicVideoInfo(pItem->m_strPath))
            {
              // handle .nfo files
              CStdString strNfoFile = GetnfoFile(pItem,bDirNames);
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
                    AddMovieAndGetThumb(pItem, "musicvideos", movieDetails, -1, false, m_dlgProgress);
                    continue;
                  }
                }
              }
            }
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
              long lResult=GetIMDBDetails(pItem, url, info,bDirNames&&info.strContent.Equals("movies"));
              if (info.strContent.Equals("tvshows"))
              {
                if (!bRefresh)
                {
                  // fetch episode guide
                  CVideoInfoTag details;
                  m_database.GetTvShowInfo(pItem->m_strPath,details,lResult);
                  CIMDBUrl url;
                  url.Parse(details.m_strEpisodeGuide);
                  IMDB_EPISODELIST episodes;
                  IMDB_EPISODELIST files;
                  EnumerateSeriesFolder(pItem,files);
                  if (IMDB.GetEpisodeList(url,episodes))
                  {
                    OnProcessSeriesFolder(episodes,files,lResult,IMDB,m_dlgProgress);
                  }
                }
                else
                  if (g_guiSettings.GetBool("videolibrary.seasonthumbs"))
                    FetchSeasonThumbs(lResult);
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
    int count = 0;
    while (!m_bStop && m_pathsToCount.size())
      count+=CountFiles(*m_pathsToCount.begin());
    m_itemCount = count;
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
    CRegExp regExSample;
    
    if (!regExSample.RegComp(REGEXSAMPLEFILE))
    {
      CLog::Log(LOGERROR, "Unable to compile RegExp for Sample file");
    }

    if (item->m_bIsFolder)
    {
      CUtil::GetRecursiveListing(item->m_strPath,items,g_stSettings.m_videoExtensions,true);
      CStdString hash, dbHash;
      int numFilesInFolder = GetPathHash(items, hash);

      if (m_database.GetPathHash(item->m_strPath, dbHash) && dbHash == hash)
      {
        m_currentItem += numFilesInFolder;

        // notify our observer of our progress
        if (m_pObserver)
        {
          if (m_itemCount>0)
          {
            m_pObserver->OnSetProgress(m_currentItem, m_itemCount);
            m_pObserver->OnSetCurrentProgress(numFilesInFolder,numFilesInFolder);
          }
          m_pObserver->OnDirectoryScanned(item->m_strPath);
        }
        return;
      }
      m_database.SetPathHash(item->m_strPath,hash);
    }
    else
      items.Add(new CFileItem(*item));

    // enumerate
    CStdStringArray expression = g_advancedSettings.m_tvshowStackRegExps;

    for (int i=0;i<items.Size();++i)
    {
      if (items[i]->m_bIsFolder)
        continue;
      CStdString strPath;
      CUtil::GetDirectory(items[i]->m_strPath,strPath);
      CUtil::RemoveSlashAtEnd(strPath); // want no slash for the test that follows

      if (CUtil::GetFileName(strPath).Equals("sample"))
        continue;

      // Discard all possible sample files defined by regExSample
      CStdString strFileName = CUtil::GetFileName(items[i]->m_strPath);
      strFileName.MakeLower();
      CLog::Log(LOGDEBUG, "Checking if file '%s' is a Sample file", strFileName.c_str());
      if (regExSample.RegFind(strFileName) > -1)
      {
        CLog::Log(LOGDEBUG, "File '%s' discarded as Sample file", strFileName.c_str());
        continue;
      }

      for (unsigned int j=0;j<expression.size();++j)
      {
        CRegExp reg;
        if (!reg.RegComp(expression[j]))
          break;
        CStdString strLabel=items[i]->m_strPath;
        strLabel.MakeLower();
        CLog::Log(LOGDEBUG,"running expression %s on label %s",expression[j].c_str(),strLabel.c_str());
        int regexppos, regexp2pos;

        if ((regexppos = reg.RegFind(strLabel.c_str())) > -1)
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

            // check the remainder of the string for any further episodes.
            CRegExp reg2;
            if (!reg2.RegComp(g_advancedSettings.m_tvshowMultiPartStackRegExp))
              break;

            char *remainder = reg.GetReplaceString("\\3");
            int offset = 0;
            
            // we want "long circuit" OR below so that both offsets are evaluated
            while (((regexp2pos = reg2.RegFind(remainder + offset)) > -1) | ((regexppos = reg.RegFind(remainder + offset)) > -1)) 
            {
              if (((regexppos <= regexp2pos) && regexppos != -1) ||
                 (regexppos >= 0 && regexp2pos == -1))
              {
                season = reg.GetReplaceString("\\1");
                episode = reg.GetReplaceString("\\2");
                key.first = atoi(season);
                key.second = atoi(episode);
                free(season);
                free(episode);
                CLog::Log(LOGDEBUG, "adding new season %u, multipart episode %u", key.first, key.second);
                episodeList.insert(std::make_pair<std::pair<int,int>,CIMDBUrl>(key,url));
                remainder = reg.GetReplaceString("\\3");
                offset = 0;
              } 
              else if (((regexp2pos < regexppos) && regexp2pos != -1) ||
                       (regexp2pos >= 0 && regexppos == -1))
              {
                episode = reg2.GetReplaceString("\\1");
                key.second = atoi(episode);
                free(episode);
                CLog::Log(LOGDEBUG, "adding multipart episode %u", key.second);
                episodeList.insert(std::make_pair<std::pair<int,int>,CIMDBUrl>(key,url));
                offset += regexp2pos + reg2.GetFindLen();
              }
            }
          }
        }
      }
    }
  }

  long CVideoInfoScanner::AddMovieAndGetThumb(CFileItem *pItem, const CStdString &content, const CVideoInfoTag &movieDetails, long idShow, bool bApplyToDir, CGUIDialogProgress* pDialog /* == NULL */)
  {
    long lResult=-1;
    // add to all movies in the stacked set
    if (content.Equals("movies"))
    {
      m_database.SetDetailsForMovie(pItem->m_strPath, movieDetails);
    }
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
    else if (content.Equals("musicvideos"))
    {
      m_database.SetDetailsForMusicVideo(pItem->m_strPath, movieDetails);
    }
    // get & save thumbnail
    CStdString strThumb = "";
    CStdString strImage = movieDetails.m_strPictureURL.GetFirstThumb().m_url;
    if (strImage.size() > 0)
    {
      // check for a cached thumb or user thumb
      pItem->SetVideoThumb();
      strThumb = pItem->GetCachedVideoThumb();

      CHTTP http;
      if (pDialog)
      {
        pDialog->SetLine(2, 415);
        pDialog->Progress();
      }

      string image;
      if (!pItem->HasThumbnail() && http.Get(strImage, image))
      {
        try
        {
          CPicture picture;
          picture.CreateThumbnailFromMemory((const BYTE *)image.c_str(), image.size(), CUtil::GetExtension(strThumb), strThumb);
        }
        catch (...)
        {
          CLog::Log(LOGERROR,"Could not make imdb thumb from %s", strImage.c_str());
          ::DeleteFile(strThumb.c_str());
        }
      }
    }

    if (bApplyToDir)
    {
      CStdString strCheck=pItem->m_strPath;
      CStdString strDirectory;
      if (pItem->IsStack())
        strCheck = CStackDirectory::GetFirstStackedFile(pItem->m_strPath);

      CUtil::GetDirectory(strCheck,strDirectory);
      if (CUtil::IsInRAR(strCheck))
      {
        CStdString strPath=strDirectory;
        CUtil::GetParentPath(strPath,strDirectory);
      }
      if (pItem->IsStack())
      {
        strCheck = strDirectory;
        CUtil::RemoveSlashAtEnd(strCheck);
        if (CUtil::GetFileName(strCheck).size() == 3 && CUtil::GetFileName(strCheck).Left(2).Equals("cd"))
          CUtil::GetDirectory(strCheck,strDirectory);
      }
      ApplyIMDBThumbToFolder(strDirectory,strThumb);
    }

    if (g_guiSettings.GetBool("videolibrary.actorthumbs"))
      FetchActorThumbs(movieDetails.m_cast);
    return lResult;
  }

  void CVideoInfoScanner::OnProcessSeriesFolder(IMDB_EPISODELIST& episodes, IMDB_EPISODELIST& files, long lShowId, CIMDB& IMDB, CGUIDialogProgress* pDlgProgress /* = NULL */)
  {
    if (pDlgProgress)
    {
      pDlgProgress->SetLine(2, 20361);
      pDlgProgress->SetPercentage(0);
      pDlgProgress->ShowProgressBar(true);
      pDlgProgress->Progress();
    }

    int iMax = files.size();
    int iCurr = 1;
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
        if (m_itemCount > 0)
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
        if (m_database.GetEpisodeInfo(iter->second.m_scrURL[0].m_url[0].m_url,iter2->first.second) > -1)
          continue;

        if (!IMDB.GetEpisodeDetails(iter2->second,episodeDetails,pDlgProgress))
          break;
        episodeDetails.m_iSeason = iter2->first.first;
        episodeDetails.m_iEpisode = iter2->first.second;
        CFileItem item;
        item.m_strPath = iter->second.m_scrURL[0].m_url[0].m_url;
        AddMovieAndGetThumb(&item,"tvshows",episodeDetails,lShowId);
      }
    }
    if (g_guiSettings.GetBool("videolibrary.seasonthumbs"))
      FetchSeasonThumbs(lShowId);
    m_database.Close();
  }

  CStdString CVideoInfoScanner::GetnfoFile(CFileItem *item, bool bGrabAny)
  {
    CStdString nfoFile;
    // Find a matching .nfo file
    if (!item->m_bIsFolder)
    {
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
        nfoFile = GetnfoFile(&item2,bGrabAny);
        // else try .nfo file matching stacked title
        if (nfoFile.IsEmpty())
        {
          CStdString stackedTitlePath = dir.GetStackedTitlePath(item->m_strPath);
          item2.m_strPath = stackedTitlePath;
          nfoFile = GetnfoFile(&item2,bGrabAny);
        }
      }

      if (nfoFile.IsEmpty()) // final attempt - strip off any cd1 folders
      {
        CStdString strPath;
        CUtil::GetDirectory(item->m_strPath,strPath);
        CUtil::RemoveSlashAtEnd(strPath); // need no slash for the check that follows
        CFileItem item2;
        if (strPath.Mid(strPath.size()-3).Equals("cd1"))
        {
          strPath = strPath.Mid(0,strPath.size()-3);
          CUtil::AddFileToFolder(strPath,CUtil::GetFileName(item->m_strPath),item2.m_strPath);
          return GetnfoFile(&item2,bGrabAny);
        }
      }
    }
    if (item->m_bIsFolder || bGrabAny && nfoFile.IsEmpty())
    {
      // see if there is a unique nfo file in this folder, and if so, use that
      CFileItemList items;
      CDirectory dir;
      CStdString strPath = item->m_strPath;
      if (!item->m_bIsFolder)
        CUtil::GetDirectory(item->m_strPath,strPath);
      if (dir.GetDirectory(strPath, items, ".nfo") && items.Size())
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
    
    return nfoFile;
  }

  long CVideoInfoScanner::GetIMDBDetails(CFileItem *pItem, CIMDBUrl &url, const SScraperInfo& info, bool bUseDirNames, CGUIDialogProgress* pDialog /* = NULL */)
  {
    CIMDB IMDB;
    CVideoInfoTag movieDetails;
    IMDB.SetScraperInfo(info);

    if ( IMDB.GetDetails(url, movieDetails, pDialog) )
      return AddMovieAndGetThumb(pItem, info.strContent, movieDetails, -1, bUseDirNames);
    return -1;
  }

  void CVideoInfoScanner::ApplyIMDBThumbToFolder(const CStdString &folder, const CStdString &imdbThumb)
  {
    // copy icon to folder also;
    if (CFile::Exists(imdbThumb))
    {
      CFileItem folderItem(folder, true);
      CStdString strThumb(folderItem.GetCachedVideoThumb());
      CFile::Cache(imdbThumb.c_str(), strThumb.c_str(), NULL, NULL);
    }
  }

  int CVideoInfoScanner::GetPathHash(const CFileItemList &items, CStdString &hash)
  {
    // Create a hash based on the filenames, filesize and filedate.  Also count the number of files
    if (0 == items.Size()) return 0;
    MD5_CTX md5state;
    unsigned char md5hash[16];
    char md5HexString[33];
    MD5Init(&md5state);
    int count = 0;
    for (int i = 0; i < items.Size(); ++i)
    {
      const CFileItem *pItem = items[i];
      MD5Update(&md5state, (unsigned char *)pItem->m_strPath.c_str(), (int)pItem->m_strPath.size());
      MD5Update(&md5state, (unsigned char *)&pItem->m_dwSize, sizeof(pItem->m_dwSize));
      FILETIME time = pItem->m_dateTime;
      MD5Update(&md5state, (unsigned char *)&time, sizeof(FILETIME));
      if (pItem->IsVideo() && !pItem->IsPlayList() && !pItem->IsNFO())
        count++;
    }
    MD5Final(md5hash, &md5state);
    XKGeneral::BytesToHexStr(md5hash, 16, md5HexString);
    hash = md5HexString;
    return count;
  }

  void CVideoInfoScanner::FetchSeasonThumbs(long lTvShowId)
  {
    CVideoInfoTag movie;
    m_database.GetTvShowInfo("",movie,lTvShowId);
    CFileItemList items;
    CStdString strPath;
    strPath.Format("videodb://2/2/%i/",lTvShowId);
    m_database.GetSeasonsNav(strPath,items,-1,-1,-1,-1,lTvShowId);
    // used for checking for a season[ ._-](number).tbn
    CFileItemList tbnItems;
    CDirectory::GetDirectory(movie.m_strPath,tbnItems,".tbn");
    for (int i=0;i<items.Size();++i)
    {
      items[i]->SetVideoThumb();
      if (!items[i]->HasThumbnail())
      {
        CStdString strExpression;
        strExpression.Format("season[ ._-](0?%i)\\.tbn",items[i]->GetVideoInfoTag()->m_iSeason);
        bool bDownload=true;
        CRegExp reg;
        if (reg.RegComp(strExpression.c_str()))
        {
          for (int j=0;j<tbnItems.Size();++j)
          {
            CStdString strCheck = CUtil::GetFileName(tbnItems[j]->m_strPath);
            strCheck.ToLower();
            if (reg.RegFind(strCheck.c_str()) > -1)
            {
              CPicture picture;
              picture.DoCreateThumbnail(tbnItems[j]->m_strPath,items[i]->GetCachedSeasonThumb());
              bDownload=false;
              break;
            }
          }
        }
        if (bDownload)
          DownloadThumbnail(items[i]->GetCachedSeasonThumb(),movie.m_strPictureURL.GetSeasonThumb(items[i]->GetVideoInfoTag()->m_iSeason));
      }
    }
  }

  void CVideoInfoScanner::FetchActorThumbs(const vector<SActorInfo>& actors)
  {
    for (unsigned int i=0;i<actors.size();++i)
    {
      CFileItem item;
      item.SetLabel(actors[i].strName);
      CStdString strThumb = item.GetCachedActorThumb();
      if (!CFile::Exists(strThumb) && !actors[i].thumbUrl.GetFirstThumb().m_url.IsEmpty())
        DownloadThumbnail(strThumb,actors[i].thumbUrl.GetFirstThumb());
    }
  }

  bool CVideoInfoScanner::DownloadThumbnail(const CStdString &thumb, const CScraperUrl::SUrlEntry& entry)
  {
    if (entry.m_url.IsEmpty())
      return false;

    CHTTP http;
    http.SetReferer(entry.m_spoof);
    string thumbData;
    if (http.Get(entry.m_url, thumbData))
    {
      try
      {
        CPicture picture;
        picture.CreateThumbnailFromMemory((const BYTE *)thumbData.c_str(), thumbData.size(), CUtil::GetExtension(entry.m_url), thumb);
        return true;
      }
      catch (...)
      {
        ::DeleteFile(thumb.c_str());
      }
    }
    return false;
  }
}
