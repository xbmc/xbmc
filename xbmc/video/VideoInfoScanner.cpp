/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "threads/SystemClock.h"
#include "FileItem.h"
#include "VideoInfoScanner.h"
#include "filesystem/DirectoryCache.h"
#include "Util.h"
#include "NfoFile.h"
#include "utils/RegExp.h"
#include "utils/md5.h"
#include "filesystem/MultiPathDirectory.h"
#include "filesystem/StackDirectory.h"
#include "VideoInfoDownloader.h"
#include "GUIInfoManager.h"
#include "filesystem/File.h"
#include "dialogs/GUIDialogExtendedProgressBar.h"
#include "dialogs/GUIDialogProgress.h"
#include "dialogs/GUIDialogYesNo.h"
#include "dialogs/GUIDialogOK.h"
#include "interfaces/AnnouncementManager.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "utils/StringUtils.h"
#include "guilib/LocalizeStrings.h"
#include "guilib/GUIWindowManager.h"
#include "utils/log.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "video/VideoLibraryQueue.h"
#include "video/VideoThumbLoader.h"
#include "TextureCache.h"
#include "GUIUserMessages.h"
#include "URL.h"

using namespace std;
using namespace XFILE;
using namespace ADDON;

namespace VIDEO
{

  CVideoInfoScanner::CVideoInfoScanner()
  {
    m_bStop = false;
    m_bRunning = false;
    m_handle = NULL;
    m_showDialog = false;
    m_bCanInterrupt = false;
    m_currentItem = 0;
    m_itemCount = 0;
    m_bClean = false;
    m_scanAll = false;
  }

  CVideoInfoScanner::~CVideoInfoScanner()
  {
  }

  void CVideoInfoScanner::Process()
  {
    m_bStop = false;

    try
    {
      if (m_showDialog && !CSettings::Get().GetBool("videolibrary.backgroundupdate"))
      {
        CGUIDialogExtendedProgressBar* dialog =
          (CGUIDialogExtendedProgressBar*)g_windowManager.GetWindow(WINDOW_DIALOG_EXT_PROGRESS);
        if (dialog)
           m_handle = dialog->GetHandle(g_localizeStrings.Get(314));
      }

      // check if we only need to perform a cleaning
      if (m_bClean && m_pathsToScan.empty())
      {
        std::set<int> paths;
        CVideoLibraryQueue::Get().CleanLibrary(paths, false, m_handle);

        if (m_handle)
          m_handle->MarkFinished();
        m_handle = NULL;

        m_bRunning = false;

        return;
      }

      unsigned int tick = XbmcThreads::SystemClockMillis();

      m_database.Open();

      m_bCanInterrupt = true;

      CLog::Log(LOGNOTICE, "VideoInfoScanner: Starting scan ..");
      ANNOUNCEMENT::CAnnouncementManager::Get().Announce(ANNOUNCEMENT::VideoLibrary, "xbmc", "OnScanStarted");

      // Reset progress vars
      m_currentItem = 0;
      m_itemCount = -1;

      // Database operations should not be canceled
      // using Interupt() while scanning as it could
      // result in unexpected behaviour.
      m_bCanInterrupt = false;

      bool bCancelled = false;
      while (!bCancelled && !m_pathsToScan.empty())
      {
        /*
         * A copy of the directory path is used because the path supplied is
         * immediately removed from the m_pathsToScan set in DoScan(). If the
         * reference points to the entry in the set a null reference error
         * occurs.
         */
        std::string directory = *m_pathsToScan.begin();
        if (!CDirectory::Exists(directory))
        {
          /*
           * Note that this will skip clean (if m_bClean is enabled) if the directory really
           * doesn't exist rather than a NAS being switched off.  A manual clean from settings
           * will still pick up and remove it though.
           */
          CLog::Log(LOGWARNING, "%s directory '%s' does not exist - skipping scan%s.", __FUNCTION__, CURL::GetRedacted(directory).c_str(), m_bClean ? " and clean" : "");
          m_pathsToScan.erase(m_pathsToScan.begin());
        }
        else if (!DoScan(directory))
          bCancelled = true;
      }

      if (!bCancelled)
      {
        if (m_bClean)
          CVideoLibraryQueue::Get().CleanLibrary(m_pathsToClean, false, m_handle);
        else
        {
          if (m_handle)
            m_handle->SetTitle(g_localizeStrings.Get(331));
          m_database.Compress(false);
        }
      }

      g_infoManager.ResetLibraryBools();
      m_database.Close();

      tick = XbmcThreads::SystemClockMillis() - tick;
      CLog::Log(LOGNOTICE, "VideoInfoScanner: Finished scan. Scanning for video info took %s", StringUtils::SecondsToTimeString(tick / 1000).c_str());
    }
    catch (...)
    {
      CLog::Log(LOGERROR, "VideoInfoScanner: Exception while scanning.");
    }
    
    m_bRunning = false;
    ANNOUNCEMENT::CAnnouncementManager::Get().Announce(ANNOUNCEMENT::VideoLibrary, "xbmc", "OnScanFinished");
    
    if (m_handle)
      m_handle->MarkFinished();
    m_handle = NULL;
  }

  void CVideoInfoScanner::Start(const std::string& strDirectory, bool scanAll)
  {
    m_strStartDir = strDirectory;
    m_scanAll = scanAll;
    m_pathsToScan.clear();
    m_pathsToClean.clear();

    m_database.Open();
    if (strDirectory.empty())
    { // scan all paths in the database.  We do this by scanning all paths in the db, and crossing them off the list as
      // we go.
      m_database.GetPaths(m_pathsToScan);
    }
    else
    { // scan all the paths of this subtree that is in the database
      vector<string> rootDirs;
      if (URIUtils::IsMultiPath(strDirectory))
        CMultiPathDirectory::GetPaths(strDirectory, rootDirs);
      else
        rootDirs.push_back(strDirectory);

      for (vector<string>::const_iterator it = rootDirs.begin(); it < rootDirs.end(); ++it)
      {
        m_pathsToScan.insert(*it);
        vector< pair<int, string> > subpaths;
        m_database.GetSubPaths(*it, subpaths);
        for (vector< pair<int, string> >::iterator it = subpaths.begin(); it < subpaths.end(); ++it)
          m_pathsToScan.insert(it->second);
      }
    }
    m_database.Close();
    m_bClean = g_advancedSettings.m_bVideoLibraryCleanOnUpdate;

    m_bRunning = true;
    Process();
  }

  void CVideoInfoScanner::Stop()
  {
    if (m_bCanInterrupt)
      m_database.Interupt();

    m_bStop = true;
  }

  static void OnDirectoryScanned(const std::string& strDirectory)
  {
    CGUIMessage msg(GUI_MSG_DIRECTORY_SCANNED, 0, 0, 0);
    msg.SetStringParam(strDirectory);
    g_windowManager.SendThreadMessage(msg);
  }

  bool CVideoInfoScanner::IsExcluded(const std::string& strDirectory) const
  {
    std::string noMediaFile = URIUtils::AddFileToFolder(strDirectory, ".nomedia");
    return CFile::Exists(noMediaFile);
  }

  bool CVideoInfoScanner::DoScan(const std::string& strDirectory)
  {
    if (m_handle)
    {
      m_handle->SetText(g_localizeStrings.Get(20415));
    }

    /*
     * Remove this path from the list we're processing. This must be done prior to
     * the check for file or folder exclusion to prevent an infinite while loop
     * in Process().
     */
    set<std::string>::iterator it = m_pathsToScan.find(strDirectory);
    if (it != m_pathsToScan.end())
      m_pathsToScan.erase(it);

    // load subfolder
    CFileItemList items;
    bool foundDirectly = false;
    bool bSkip = false;

    SScanSettings settings;
    ScraperPtr info = m_database.GetScraperForPath(strDirectory, settings, foundDirectly);
    CONTENT_TYPE content = info ? info->Content() : CONTENT_NONE;

    // exclude folders that match our exclude regexps
    const vector<string> &regexps = content == CONTENT_TVSHOWS ? g_advancedSettings.m_tvshowExcludeFromScanRegExps
                                                         : g_advancedSettings.m_moviesExcludeFromScanRegExps;

    if (CUtil::ExcludeFileOrFolder(strDirectory, regexps))
      return true;

    if (IsExcluded(strDirectory))
    {
      CLog::Log(LOGWARNING, "Skipping item '%s' with '.nomedia' file in parent directory, it won't be added to the library.", CURL::GetRedacted(strDirectory).c_str());
      return true;
    }

    bool ignoreFolder = !m_scanAll && settings.noupdate;
    if (content == CONTENT_NONE || ignoreFolder)
      return true;

    std::string hash, dbHash;
    if (content == CONTENT_MOVIES ||content == CONTENT_MUSICVIDEOS)
    {
      if (m_handle)
      {
        int str = content == CONTENT_MOVIES ? 20317:20318;
        m_handle->SetTitle(StringUtils::Format(g_localizeStrings.Get(str).c_str(), info->Name().c_str()));
      }

      std::string fastHash;
      if (g_advancedSettings.m_bVideoLibraryUseFastHash)
        fastHash = GetFastHash(strDirectory, regexps);

      if (m_database.GetPathHash(strDirectory, dbHash) && !fastHash.empty() && fastHash == dbHash)
      { // fast hashes match - no need to process anything
        hash = fastHash;
      }
      else
      { // need to fetch the folder
        CDirectory::GetDirectory(strDirectory, items, g_advancedSettings.m_videoExtensions);
        items.Stack();

        // check whether to re-use previously computed fast hash
        if (!CanFastHash(items, regexps) || fastHash.empty())
          GetPathHash(items, hash);
        else
          hash = fastHash;
      }

      if (hash == dbHash)
      { // hash matches - skipping
        CLog::Log(LOGDEBUG, "VideoInfoScanner: Skipping dir '%s' due to no change%s", CURL::GetRedacted(strDirectory).c_str(), !fastHash.empty() ? " (fasthash)" : "");
        bSkip = true;
      }
      else if (hash.empty())
      { // directory empty or non-existent - add to clean list and skip
        CLog::Log(LOGDEBUG, "VideoInfoScanner: Skipping dir '%s' as it's empty or doesn't exist - adding to clean list", CURL::GetRedacted(strDirectory).c_str());
        if (m_bClean)
          m_pathsToClean.insert(m_database.GetPathId(strDirectory));
        bSkip = true;
      }
      else if (dbHash.empty())
      { // new folder - scan
        CLog::Log(LOGDEBUG, "VideoInfoScanner: Scanning dir '%s' as not in the database", CURL::GetRedacted(strDirectory).c_str());
      }
      else
      { // hash changed - rescan
        CLog::Log(LOGDEBUG, "VideoInfoScanner: Rescanning dir '%s' due to change (%s != %s)", CURL::GetRedacted(strDirectory).c_str(), dbHash.c_str(), hash.c_str());
      }
    }
    else if (content == CONTENT_TVSHOWS)
    {
      if (m_handle)
        m_handle->SetTitle(StringUtils::Format(g_localizeStrings.Get(20319).c_str(), info->Name().c_str()));

      if (foundDirectly && !settings.parent_name_root)
      {
        CDirectory::GetDirectory(strDirectory, items, g_advancedSettings.m_videoExtensions);
        items.SetPath(strDirectory);
        GetPathHash(items, hash);
        bSkip = true;
        if (!m_database.GetPathHash(strDirectory, dbHash) || dbHash != hash)
          bSkip = false;
        else
          items.Clear();
      }
      else
      {
        CFileItemPtr item(new CFileItem(URIUtils::GetFileName(strDirectory)));
        item->SetPath(strDirectory);
        item->m_bIsFolder = true;
        items.Add(item);
        items.SetPath(URIUtils::GetParentPath(item->GetPath()));
      }
    }

    if (!bSkip)
    {
      if (RetrieveVideoInfo(items, settings.parent_name_root, content))
      {
        if (!m_bStop && (content == CONTENT_MOVIES || content == CONTENT_MUSICVIDEOS))
        {
          m_database.SetPathHash(strDirectory, hash);
          if (m_bClean)
            m_pathsToClean.insert(m_database.GetPathId(strDirectory));
          CLog::Log(LOGDEBUG, "VideoInfoScanner: Finished adding information from dir %s", CURL::GetRedacted(strDirectory).c_str());
        }
      }
      else
      {
        if (m_bClean)
          m_pathsToClean.insert(m_database.GetPathId(strDirectory));
        CLog::Log(LOGDEBUG, "VideoInfoScanner: No (new) information was found in dir %s", CURL::GetRedacted(strDirectory).c_str());
      }
    }
    else if (hash != dbHash && (content == CONTENT_MOVIES || content == CONTENT_MUSICVIDEOS))
    { // update the hash either way - we may have changed the hash to a fast version
      m_database.SetPathHash(strDirectory, hash);
    }

    if (m_handle)
      OnDirectoryScanned(strDirectory);

    for (int i = 0; i < items.Size(); ++i)
    {
      CFileItemPtr pItem = items[i];

      if (m_bStop)
        break;

      // if we have a directory item (non-playlist) we then recurse into that folder
      // do not recurse for tv shows - we have already looked recursively for episodes
      if (pItem->m_bIsFolder && !pItem->IsParentFolder() && !pItem->IsPlayList() && settings.recurse > 0 && content != CONTENT_TVSHOWS)
      {
        if (!DoScan(pItem->GetPath()))
        {
          m_bStop = true;
        }
      }
    }
    return !m_bStop;
  }

  bool CVideoInfoScanner::RetrieveVideoInfo(CFileItemList& items, bool bDirNames, CONTENT_TYPE content, bool useLocal, CScraperUrl* pURL, bool fetchEpisodes, CGUIDialogProgress* pDlgProgress)
  {
    if (pDlgProgress)
    {
      if (items.Size() > 1 || (items[0]->m_bIsFolder && fetchEpisodes))
      {
        pDlgProgress->ShowProgressBar(true);
        pDlgProgress->SetPercentage(0);
      }
      else
        pDlgProgress->ShowProgressBar(false);

      pDlgProgress->Progress();
    }

    m_database.Open();

    bool FoundSomeInfo = false;
    vector<int> seenPaths;
    for (int i = 0; i < (int)items.Size(); ++i)
    {
      m_nfoReader.Close();
      CFileItemPtr pItem = items[i];

      // we do this since we may have a override per dir
      ScraperPtr info2 = m_database.GetScraperForPath(pItem->m_bIsFolder ? pItem->GetPath() : items.GetPath());
      if (!info2) // skip
        continue;

      // Discard all exclude files defined by regExExclude
      if (CUtil::ExcludeFileOrFolder(pItem->GetPath(), (content == CONTENT_TVSHOWS) ? g_advancedSettings.m_tvshowExcludeFromScanRegExps
                                                                                    : g_advancedSettings.m_moviesExcludeFromScanRegExps))
        continue;

      if (info2->Content() == CONTENT_MOVIES || info2->Content() == CONTENT_MUSICVIDEOS)
      {
        if (m_handle)
          m_handle->SetPercentage(i*100.f/items.Size());
      }

      // clear our scraper cache
      info2->ClearCache();

      INFO_RET ret = INFO_CANCELLED;
      if (info2->Content() == CONTENT_TVSHOWS)
        ret = RetrieveInfoForTvShow(pItem.get(), bDirNames, info2, useLocal, pURL, fetchEpisodes, pDlgProgress);
      else if (info2->Content() == CONTENT_MOVIES)
        ret = RetrieveInfoForMovie(pItem.get(), bDirNames, info2, useLocal, pURL, pDlgProgress);
      else if (info2->Content() == CONTENT_MUSICVIDEOS)
        ret = RetrieveInfoForMusicVideo(pItem.get(), bDirNames, info2, useLocal, pURL, pDlgProgress);
      else
      {
        CLog::Log(LOGERROR, "VideoInfoScanner: Unknown content type %d (%s)", info2->Content(), CURL::GetRedacted(pItem->GetPath()).c_str());
        FoundSomeInfo = false;
        break;
      }
      if (ret == INFO_CANCELLED || ret == INFO_ERROR)
      {
        FoundSomeInfo = false;
        break;
      }
      if (ret == INFO_ADDED || ret == INFO_HAVE_ALREADY)
        FoundSomeInfo = true;
      else if (ret == INFO_NOT_FOUND)
      {
        CLog::Log(LOGWARNING, "No information found for item '%s', it won't be added to the library.", CURL::GetRedacted(pItem->GetPath()).c_str());
      }

      pURL = NULL;

      // Keep track of directories we've seen
      if (m_bClean && pItem->m_bIsFolder)
        seenPaths.push_back(m_database.GetPathId(pItem->GetPath()));
    }

    if (content == CONTENT_TVSHOWS && ! seenPaths.empty())
    {
      vector< pair<int,string> > libPaths;
      m_database.GetSubPaths(items.GetPath(), libPaths);
      for (vector< pair<int,string> >::iterator i = libPaths.begin(); i < libPaths.end(); ++i)
      {
        if (find(seenPaths.begin(), seenPaths.end(), i->first) == seenPaths.end())
          m_pathsToClean.insert(i->first);
      }
    }
    if(pDlgProgress)
      pDlgProgress->ShowProgressBar(false);

    m_database.Close();
    return FoundSomeInfo;
  }

  INFO_RET CVideoInfoScanner::RetrieveInfoForTvShow(CFileItem *pItem, bool bDirNames, ScraperPtr &info2, bool useLocal, CScraperUrl* pURL, bool fetchEpisodes, CGUIDialogProgress* pDlgProgress)
  {
    long idTvShow = -1;
    if (pItem->m_bIsFolder)
      idTvShow = m_database.GetTvShowId(pItem->GetPath());
    else
    {
      std::string strPath = URIUtils::GetDirectory(pItem->GetPath());
      idTvShow = m_database.GetTvShowId(strPath);
    }
    if (idTvShow > -1 && (fetchEpisodes || !pItem->m_bIsFolder))
    {
      INFO_RET ret = RetrieveInfoForEpisodes(pItem, idTvShow, info2, useLocal, pDlgProgress);
      if (ret == INFO_ADDED)
        m_database.SetPathHash(pItem->GetPath(), pItem->GetProperty("hash").asString());
      return ret;
    }

    if (ProgressCancelled(pDlgProgress, pItem->m_bIsFolder ? 20353 : 20361, pItem->GetLabel()))
      return INFO_CANCELLED;

    if (m_handle)
      m_handle->SetText(pItem->GetMovieName(bDirNames));

    CNfoFile::NFOResult result=CNfoFile::NO_NFO;
    CScraperUrl scrUrl;
    // handle .nfo files
    if (useLocal)
      result = CheckForNFOFile(pItem, bDirNames, info2, scrUrl);
    if (result == CNfoFile::FULL_NFO)
    {
      pItem->GetVideoInfoTag()->Reset();
      m_nfoReader.GetDetails(*pItem->GetVideoInfoTag());

      long lResult = AddVideo(pItem, info2->Content(), bDirNames, useLocal);
      if (lResult < 0)
        return INFO_ERROR;
      if (fetchEpisodes)
      {
        INFO_RET ret = RetrieveInfoForEpisodes(pItem, lResult, info2, useLocal, pDlgProgress);
        if (ret == INFO_ADDED)
          m_database.SetPathHash(pItem->GetPath(), pItem->GetProperty("hash").asString());
        return ret;
      }
      return INFO_ADDED;
    }
    if (result == CNfoFile::URL_NFO || result == CNfoFile::COMBINED_NFO)
      pURL = &scrUrl;

    CScraperUrl url;
    int retVal = 0;
    if (pURL)
      url = *pURL;
    else if ((retVal = FindVideo(pItem->GetMovieName(bDirNames), info2, url, pDlgProgress)) <= 0)
      return retVal < 0 ? INFO_CANCELLED : INFO_NOT_FOUND;

    long lResult=-1;
    if (GetDetails(pItem, url, info2, result == CNfoFile::COMBINED_NFO ? &m_nfoReader : NULL, pDlgProgress))
    {
      if ((lResult = AddVideo(pItem, info2->Content(), false, useLocal)) < 0)
        return INFO_ERROR;
    }
    if (fetchEpisodes)
    {
      INFO_RET ret = RetrieveInfoForEpisodes(pItem, lResult, info2, useLocal, pDlgProgress);
      if (ret == INFO_ADDED)
        m_database.SetPathHash(pItem->GetPath(), pItem->GetProperty("hash").asString());
    }
    return INFO_ADDED;
  }

  INFO_RET CVideoInfoScanner::RetrieveInfoForMovie(CFileItem *pItem, bool bDirNames, ScraperPtr &info2, bool useLocal, CScraperUrl* pURL, CGUIDialogProgress* pDlgProgress)
  {
    if (pItem->m_bIsFolder || !pItem->IsVideo() || pItem->IsNFO() ||
       (pItem->IsPlayList() && !URIUtils::HasExtension(pItem->GetPath(), ".strm")))
      return INFO_NOT_NEEDED;

    if (ProgressCancelled(pDlgProgress, 198, pItem->GetLabel()))
      return INFO_CANCELLED;

    if (m_database.HasMovieInfo(pItem->GetPath()))
      return INFO_HAVE_ALREADY;

    if (m_handle)
      m_handle->SetText(pItem->GetMovieName(bDirNames));

    CNfoFile::NFOResult result=CNfoFile::NO_NFO;
    CScraperUrl scrUrl;
    // handle .nfo files
    if (useLocal)
      result = CheckForNFOFile(pItem, bDirNames, info2, scrUrl);
    if (result == CNfoFile::FULL_NFO)
    {
      pItem->GetVideoInfoTag()->Reset();
      m_nfoReader.GetDetails(*pItem->GetVideoInfoTag());

      if (AddVideo(pItem, info2->Content(), bDirNames, true) < 0)
        return INFO_ERROR;
      return INFO_ADDED;
    }
    if (result == CNfoFile::URL_NFO || result == CNfoFile::COMBINED_NFO)
      pURL = &scrUrl;

    CScraperUrl url;
    int retVal = 0;
    if (pURL)
      url = *pURL;
    else if ((retVal = FindVideo(pItem->GetMovieName(bDirNames), info2, url, pDlgProgress)) <= 0)
      return retVal < 0 ? INFO_CANCELLED : INFO_NOT_FOUND;

    if (GetDetails(pItem, url, info2, result == CNfoFile::COMBINED_NFO ? &m_nfoReader : NULL, pDlgProgress))
    {
      if (AddVideo(pItem, info2->Content(), bDirNames, useLocal) < 0)
        return INFO_ERROR;
      return INFO_ADDED;
    }
    // TODO: This is not strictly correct as we could fail to download information here or error, or be cancelled
    return INFO_NOT_FOUND;
  }

  INFO_RET CVideoInfoScanner::RetrieveInfoForMusicVideo(CFileItem *pItem, bool bDirNames, ScraperPtr &info2, bool useLocal, CScraperUrl* pURL, CGUIDialogProgress* pDlgProgress)
  {
    if (pItem->m_bIsFolder || !pItem->IsVideo() || pItem->IsNFO() ||
       (pItem->IsPlayList() && !URIUtils::HasExtension(pItem->GetPath(), ".strm")))
      return INFO_NOT_NEEDED;

    if (ProgressCancelled(pDlgProgress, 20394, pItem->GetLabel()))
      return INFO_CANCELLED;

    if (m_database.HasMusicVideoInfo(pItem->GetPath()))
      return INFO_HAVE_ALREADY;

    if (m_handle)
      m_handle->SetText(pItem->GetMovieName(bDirNames));

    CNfoFile::NFOResult result=CNfoFile::NO_NFO;
    CScraperUrl scrUrl;
    // handle .nfo files
    if (useLocal)
      result = CheckForNFOFile(pItem, bDirNames, info2, scrUrl);
    if (result == CNfoFile::FULL_NFO)
    {
      pItem->GetVideoInfoTag()->Reset();
      m_nfoReader.GetDetails(*pItem->GetVideoInfoTag());

      if (AddVideo(pItem, info2->Content(), bDirNames, true) < 0)
        return INFO_ERROR;
      return INFO_ADDED;
    }
    if (result == CNfoFile::URL_NFO || result == CNfoFile::COMBINED_NFO)
      pURL = &scrUrl;

    CScraperUrl url;
    int retVal = 0;
    if (pURL)
      url = *pURL;
    else if ((retVal = FindVideo(pItem->GetMovieName(bDirNames), info2, url, pDlgProgress)) <= 0)
      return retVal < 0 ? INFO_CANCELLED : INFO_NOT_FOUND;

    if (GetDetails(pItem, url, info2, result == CNfoFile::COMBINED_NFO ? &m_nfoReader : NULL, pDlgProgress))
    {
      if (AddVideo(pItem, info2->Content(), bDirNames, useLocal) < 0)
        return INFO_ERROR;
      return INFO_ADDED;
    }
    // TODO: This is not strictly correct as we could fail to download information here or error, or be cancelled
    return INFO_NOT_FOUND;
  }

  INFO_RET CVideoInfoScanner::RetrieveInfoForEpisodes(CFileItem *item, long showID, const ADDON::ScraperPtr &scraper, bool useLocal, CGUIDialogProgress *progress)
  {
    // enumerate episodes
    EPISODELIST files;
    if (!EnumerateSeriesFolder(item, files))
      return INFO_HAVE_ALREADY;
    if (files.size() == 0) // no update or no files
      return INFO_NOT_NEEDED;

    if (m_bStop || (progress && progress->IsCanceled()))
      return INFO_CANCELLED;

    CVideoInfoTag showInfo;
    m_database.GetTvShowInfo("", showInfo, showID);
    INFO_RET ret = OnProcessSeriesFolder(files, scraper, useLocal, showInfo, progress);

    if (ret == INFO_ADDED)
    {
      map<int, map<string, string> > seasonArt;
      m_database.GetTvShowSeasonArt(showID, seasonArt);

      bool updateSeasonArt = false;
      for (map<int, map<string, string> >::const_iterator i = seasonArt.begin(); i != seasonArt.end(); ++i)
      {
        if (i->second.empty())
        {
          updateSeasonArt = true;
          break;
        }
      }

      if (updateSeasonArt)
      {
        CVideoInfoDownloader loader(scraper);
        loader.GetArtwork(showInfo);
        GetSeasonThumbs(showInfo, seasonArt, CVideoThumbLoader::GetArtTypes(MediaTypeSeason), useLocal);
        for (map<int, map<string, string> >::const_iterator i = seasonArt.begin(); i != seasonArt.end(); ++i)
        {
          int seasonID = m_database.AddSeason(showID, i->first);
          m_database.SetArtForItem(seasonID, MediaTypeSeason, i->second);
        }
      }
    }
    return ret;
  }

  bool CVideoInfoScanner::EnumerateSeriesFolder(CFileItem* item, EPISODELIST& episodeList)
  {
    CFileItemList items;
    const vector<string> &regexps = g_advancedSettings.m_tvshowExcludeFromScanRegExps;

    bool bSkip = false;

    if (item->m_bIsFolder)
    {
      /*
       * Note: DoScan() will not remove this path as it's not recursing for tvshows.
       * Remove this path from the list we're processing in order to avoid hitting
       * it twice in the main loop.
       */
      set<std::string>::iterator it = m_pathsToScan.find(item->GetPath());
      if (it != m_pathsToScan.end())
        m_pathsToScan.erase(it);

      std::string hash, dbHash;
      if (g_advancedSettings.m_bVideoLibraryUseFastHash)
        hash = GetRecursiveFastHash(item->GetPath(), regexps);

      if (m_database.GetPathHash(item->GetPath(), dbHash) && !hash.empty() && dbHash == hash)
      {
        // fast hashes match - no need to process anything
        bSkip = true;
      }

      // fast hash cannot be computed or we need to rescan. fetch the listing.
      if (!bSkip)
      {
        int flags = DIR_FLAG_DEFAULTS;
        if (!hash.empty())
          flags |= DIR_FLAG_NO_FILE_INFO;

        CUtil::GetRecursiveListing(item->GetPath(), items, g_advancedSettings.m_videoExtensions, flags);

        // fast hash failed - compute slow one
        if (hash.empty())
        {
          GetPathHash(items, hash);
          if (dbHash == hash)
          {
            // slow hashes match - no need to process anything
            bSkip = true;
          }
        }
      }

      if (bSkip)
      {
        CLog::Log(LOGDEBUG, "VideoInfoScanner: Skipping dir '%s' due to no change", CURL::GetRedacted(item->GetPath()).c_str());
        // update our dialog with our progress
        if (m_handle)
          OnDirectoryScanned(item->GetPath());
        return false;
      }

      if (dbHash.empty())
        CLog::Log(LOGDEBUG, "VideoInfoScanner: Scanning dir '%s' as not in the database", CURL::GetRedacted(item->GetPath()).c_str());
      else
        CLog::Log(LOGDEBUG, "VideoInfoScanner: Rescanning dir '%s' due to change (%s != %s)", CURL::GetRedacted(item->GetPath()).c_str(), dbHash.c_str(), hash.c_str());

      if (m_bClean)
      {
        m_pathsToClean.insert(m_database.GetPathId(item->GetPath()));
        m_database.GetPathsForTvShow(m_database.GetTvShowId(item->GetPath()), m_pathsToClean);
      }
      item->SetProperty("hash", hash);
    }
    else
    {
      CFileItemPtr newItem(new CFileItem(*item));
      items.Add(newItem);
    }

    /*
    stack down any dvd folders
    need to sort using the full path since this is a collapsed recursive listing of all subdirs
    video_ts.ifo files should sort at the top of a dvd folder in ascending order

    /foo/bar/video_ts.ifo
    /foo/bar/vts_x_y.ifo
    /foo/bar/vts_x_y.vob
    */

    // since we're doing this now anyway, should other items be stacked?
    items.Sort(SortByPath, SortOrderAscending);
    int x = 0;
    while (x < items.Size())
    {
      if (items[x]->m_bIsFolder)
      {
        x++;
        continue;
      }

      std::string strPathX, strFileX;
      URIUtils::Split(items[x]->GetPath(), strPathX, strFileX);
      //CLog::Log(LOGDEBUG,"%i:%s:%s", x, strPathX.c_str(), strFileX.c_str());

      const int y = x + 1;
      if (StringUtils::EqualsNoCase(strFileX, "VIDEO_TS.IFO"))
      {
        while (y < items.Size())
        {
          std::string strPathY, strFileY;
          URIUtils::Split(items[y]->GetPath(), strPathY, strFileY);
          //CLog::Log(LOGDEBUG," %i:%s:%s", y, strPathY.c_str(), strFileY.c_str());

          if (StringUtils::EqualsNoCase(strPathY, strPathX))
            /*
            remove everything sorted below the video_ts.ifo file in the same path.
            understandbly this wont stack correctly if there are other files in the the dvd folder.
            this should be unlikely and thus is being ignored for now but we can monitor the
            where the path changes and potentially remove the items above the video_ts.ifo file.
            */
            items.Remove(y);
          else
            break;
        }
      }
      x++;
    }

    // enumerate
    for (int i=0;i<items.Size();++i)
    {
      if (items[i]->m_bIsFolder)
        continue;
      std::string strPath = URIUtils::GetDirectory(items[i]->GetPath());
      URIUtils::RemoveSlashAtEnd(strPath); // want no slash for the test that follows

      if (StringUtils::EqualsNoCase(URIUtils::GetFileName(strPath), "sample"))
        continue;

      // Discard all exclude files defined by regExExcludes
      if (CUtil::ExcludeFileOrFolder(items[i]->GetPath(), regexps))
        continue;

      /*
       * Check if the media source has already set the season and episode or original air date in
       * the VideoInfoTag. If it has, do not try to parse any of them from the file path to avoid
       * any false positive matches.
       */
      if (ProcessItemByVideoInfoTag(items[i].get(), episodeList))
        continue;

      if (!EnumerateEpisodeItem(items[i].get(), episodeList))
        CLog::Log(LOGDEBUG, "VideoInfoScanner: Could not enumerate file %s", CURL::GetRedacted(CURL::Decode(items[i]->GetPath())).c_str());
    }
    return true;
  }

  bool CVideoInfoScanner::ProcessItemByVideoInfoTag(const CFileItem *item, EPISODELIST &episodeList)
  {
    if (!item->HasVideoInfoTag())
      return false;

    const CVideoInfoTag* tag = item->GetVideoInfoTag();
    /*
     * First check the season and episode number. This takes precedence over the original air
     * date and episode title. Must be a valid season and episode number combination.
     */
    if (tag->m_iSeason > -1 && tag->m_iEpisode > 0)
    {
      EPISODE episode;
      episode.strPath = item->GetPath();
      episode.iSeason = tag->m_iSeason;
      episode.iEpisode = tag->m_iEpisode;
      episode.isFolder = false;
      episodeList.push_back(episode);
      CLog::Log(LOGDEBUG, "%s - found match for: %s. Season %d, Episode %d", __FUNCTION__,
                CURL::GetRedacted(episode.strPath).c_str(), episode.iSeason, episode.iEpisode);
      return true;
    }

    /*
     * Next preference is the first aired date. If it exists use that for matching the TV Show
     * information. Also set the title in case there are multiple matches for the first aired date.
     */
    if (tag->m_firstAired.IsValid())
    {
      EPISODE episode;
      episode.strPath = item->GetPath();
      episode.strTitle = tag->m_strTitle;
      episode.isFolder = false;
      /*
       * Set season and episode to -1 to indicate to use the aired date.
       */
      episode.iSeason = -1;
      episode.iEpisode = -1;
      /*
       * The first aired date string must be parseable.
       */
      episode.cDate = item->GetVideoInfoTag()->m_firstAired;
      episodeList.push_back(episode);
      CLog::Log(LOGDEBUG, "%s - found match for: '%s', firstAired: '%s' = '%s', title: '%s'",
        __FUNCTION__, CURL::GetRedacted(episode.strPath).c_str(), tag->m_firstAired.GetAsDBDateTime().c_str(),
                episode.cDate.GetAsLocalizedDate().c_str(), episode.strTitle.c_str());
      return true;
    }

    /*
     * Next preference is the episode title. If it exists use that for matching the TV Show
     * information.
     */
    if (!tag->m_strTitle.empty())
    {
      EPISODE episode;
      episode.strPath = item->GetPath();
      episode.strTitle = tag->m_strTitle;
      episode.isFolder = false;
      /*
       * Set season and episode to -1 to indicate to use the title.
       */
      episode.iSeason = -1;
      episode.iEpisode = -1;
      episodeList.push_back(episode);
      CLog::Log(LOGDEBUG,"%s - found match for: '%s', title: '%s'", __FUNCTION__,
                CURL::GetRedacted(episode.strPath).c_str(), episode.strTitle.c_str());
      return true;
    }

    /*
     * There is no further episode information available if both the season and episode number have
     * been set to 0. Return the match as true so no further matching is attempted, but don't add it
     * to the episode list.
     */
    if (tag->m_iSeason == 0 && tag->m_iEpisode == 0)
    {
      CLog::Log(LOGDEBUG,"%s - found exclusion match for: %s. Both Season and Episode are 0. Item will be ignored for scanning.",
                __FUNCTION__, CURL::GetRedacted(item->GetPath()).c_str());
      return true;
    }

    return false;
  }

  bool CVideoInfoScanner::EnumerateEpisodeItem(const CFileItem *item, EPISODELIST& episodeList)
  {
    SETTINGS_TVSHOWLIST expression = g_advancedSettings.m_tvshowEnumRegExps;

    std::string strLabel;

    // remove path to main file if it's a bd or dvd folder to regex the right (folder) name
    if (item->IsOpticalMediaFile())
    {
      strLabel = item->GetLocalMetadataPath();
      URIUtils::RemoveSlashAtEnd(strLabel);
    }
    else
      strLabel = item->GetPath();

    // URLDecode in case an episode is on a http/https/dav/davs:// source and URL-encoded like foo%201x01%20bar.avi
    strLabel = CURL::Decode(strLabel);

    for (unsigned int i=0;i<expression.size();++i)
    {
      CRegExp reg(true, CRegExp::autoUtf8);
      if (!reg.RegComp(expression[i].regexp))
        continue;

      int regexppos, regexp2pos;
      //CLog::Log(LOGDEBUG,"running expression %s on %s",expression[i].regexp.c_str(),strLabel.c_str());
      if ((regexppos = reg.RegFind(strLabel.c_str())) < 0)
        continue;

      EPISODE episode;
      episode.strPath = item->GetPath();
      episode.iSeason = -1;
      episode.iEpisode = -1;
      episode.cDate.SetValid(false);
      episode.isFolder = false;

      bool byDate = expression[i].byDate ? true : false;
      int defaultSeason = expression[i].defaultSeason;

      if (byDate)
      {
        if (!GetAirDateFromRegExp(reg, episode))
          continue;

        CLog::Log(LOGDEBUG, "VideoInfoScanner: Found date based match %s (%s) [%s]", CURL::GetRedacted(episode.strPath).c_str(),
                  episode.cDate.GetAsLocalizedDate().c_str(), expression[i].regexp.c_str());
      }
      else
      {
        if (!GetEpisodeAndSeasonFromRegExp(reg, episode, defaultSeason))
          continue;

        CLog::Log(LOGDEBUG, "VideoInfoScanner: Found episode match %s (s%ie%i) [%s]", CURL::GetRedacted(episode.strPath).c_str(),
                  episode.iSeason, episode.iEpisode, expression[i].regexp.c_str());
      }

      // Grab the remainder from first regexp run
      // as second run might modify or empty it.
      std::string remainder(reg.GetMatch(3));

      /*
       * Check if the files base path is a dedicated folder that contains
       * only this single episode. If season and episode match with the
       * actual media file, we set episode.isFolder to true.
       */
      std::string strBasePath = item->GetBaseMoviePath(true);
      URIUtils::RemoveSlashAtEnd(strBasePath);
      strBasePath = URIUtils::GetFileName(strBasePath);

      if (reg.RegFind(strBasePath.c_str()) > -1)
      {
        EPISODE parent;
        if (byDate)
        {
          GetAirDateFromRegExp(reg, parent);
          if (episode.cDate == parent.cDate)
            episode.isFolder = true;
        }
        else
        {
          GetEpisodeAndSeasonFromRegExp(reg, parent, defaultSeason);
          if (episode.iSeason == parent.iSeason && episode.iEpisode == parent.iEpisode)
            episode.isFolder = true;
        }
      }

      // add what we found by now
      episodeList.push_back(episode);

      CRegExp reg2(true, CRegExp::autoUtf8);
      // check the remainder of the string for any further episodes.
      if (!byDate && reg2.RegComp(g_advancedSettings.m_tvshowMultiPartEnumRegExp))
      {
        int offset = 0;

        // we want "long circuit" OR below so that both offsets are evaluated
        while (((regexp2pos = reg2.RegFind(remainder.c_str() + offset)) > -1) | ((regexppos = reg.RegFind(remainder.c_str() + offset)) > -1))
        {
          if (((regexppos <= regexp2pos) && regexppos != -1) ||
             (regexppos >= 0 && regexp2pos == -1))
          {
            GetEpisodeAndSeasonFromRegExp(reg, episode, defaultSeason);

            CLog::Log(LOGDEBUG, "VideoInfoScanner: Adding new season %u, multipart episode %u [%s]",
                      episode.iSeason, episode.iEpisode,
                      g_advancedSettings.m_tvshowMultiPartEnumRegExp.c_str());

            episodeList.push_back(episode);
            remainder = reg.GetMatch(3);
            offset = 0;
          }
          else if (((regexp2pos < regexppos) && regexp2pos != -1) ||
                   (regexp2pos >= 0 && regexppos == -1))
          {
            episode.iEpisode = atoi(reg2.GetMatch(1).c_str());
            CLog::Log(LOGDEBUG, "VideoInfoScanner: Adding multipart episode %u [%s]",
                      episode.iEpisode, g_advancedSettings.m_tvshowMultiPartEnumRegExp.c_str());
            episodeList.push_back(episode);
            offset += regexp2pos + reg2.GetFindLen();
          }
        }
      }
      return true;
    }
    return false;
  }

  bool CVideoInfoScanner::GetEpisodeAndSeasonFromRegExp(CRegExp &reg, EPISODE &episodeInfo, int defaultSeason)
  {
    std::string season(reg.GetMatch(1));
    std::string episode(reg.GetMatch(2));

    if (!season.empty() || !episode.empty())
    {
      char* endptr = NULL;
      if (season.empty() && !episode.empty())
      { // no season specified -> assume defaultSeason
        episodeInfo.iSeason = defaultSeason;
        if ((episodeInfo.iEpisode = CUtil::TranslateRomanNumeral(episode.c_str())) == -1)
          episodeInfo.iEpisode = strtol(episode.c_str(), &endptr, 10);
      }
      else if (!season.empty() && episode.empty())
      { // no episode specification -> assume defaultSeason
        episodeInfo.iSeason = defaultSeason;
        if ((episodeInfo.iEpisode = CUtil::TranslateRomanNumeral(season.c_str())) == -1)
          episodeInfo.iEpisode = atoi(season.c_str());
      }
      else
      { // season and episode specified
        episodeInfo.iSeason = atoi(season.c_str());
        episodeInfo.iEpisode = strtol(episode.c_str(), &endptr, 10);
      }
      if (endptr)
      {
        if (isalpha(*endptr))
          episodeInfo.iSubepisode = *endptr - (islower(*endptr) ? 'a' : 'A') + 1;
        else if (*endptr == '.')
          episodeInfo.iSubepisode = atoi(endptr+1);
      }
      return true;
    }
    return false;
  }

  bool CVideoInfoScanner::GetAirDateFromRegExp(CRegExp &reg, EPISODE &episodeInfo)
  {
    std::string param1(reg.GetMatch(1));
    std::string param2(reg.GetMatch(2));
    std::string param3(reg.GetMatch(3));

    if (!param1.empty() && !param2.empty() && !param3.empty())
    {
      // regular expression by date
      int len1 = param1.size();
      int len2 = param2.size();
      int len3 = param3.size();

      if (len1==4 && len2==2 && len3==2)
      {
        // yyyy mm dd format
        episodeInfo.cDate.SetDate(atoi(param1.c_str()), atoi(param2.c_str()), atoi(param3.c_str()));
      }
      else if (len1==2 && len2==2 && len3==4)
      {
        // mm dd yyyy format
        episodeInfo.cDate.SetDate(atoi(param3.c_str()), atoi(param1.c_str()), atoi(param2.c_str()));
      }
    }
    return episodeInfo.cDate.IsValid();
  }

  long CVideoInfoScanner::AddVideo(CFileItem *pItem, const CONTENT_TYPE &content, bool videoFolder /* = false */, bool useLocal /* = true */, const CVideoInfoTag *showInfo /* = NULL */, bool libraryImport /* = false */)
  {
    // ensure our database is open (this can get called via other classes)
    if (!m_database.Open())
      return -1;

    if (!libraryImport)
      GetArtwork(pItem, content, videoFolder, useLocal, showInfo ? showInfo->m_strPath : "");

    // ensure the art map isn't completely empty by specifying an empty thumb
    map<string, string> art = pItem->GetArt();
    if (art.empty())
      art["thumb"] = "";

    CVideoInfoTag &movieDetails = *pItem->GetVideoInfoTag();
    if (movieDetails.m_basePath.empty())
      movieDetails.m_basePath = pItem->GetBaseMoviePath(videoFolder);
    movieDetails.m_parentPathID = m_database.AddPath(URIUtils::GetParentPath(movieDetails.m_basePath));

    movieDetails.m_strFileNameAndPath = pItem->GetPath();

    if (pItem->m_bIsFolder)
      movieDetails.m_strPath = pItem->GetPath();

    std::string strTitle(movieDetails.m_strTitle);

    if (showInfo && content == CONTENT_TVSHOWS)
    {
      strTitle = StringUtils::Format("%s - %ix%i - %s", showInfo->m_strTitle.c_str(), movieDetails.m_iSeason, movieDetails.m_iEpisode, strTitle.c_str());
    }

    std::string redactPath(CURL::GetRedacted(CURL::Decode(pItem->GetPath())));

    CLog::Log(LOGDEBUG, "VideoInfoScanner: Adding new item to %s:%s", TranslateContent(content).c_str(), redactPath.c_str());
    long lResult = -1;

    if (content == CONTENT_MOVIES)
    {
      // find local trailer first
      std::string strTrailer = pItem->FindTrailer();
      if (!strTrailer.empty())
        movieDetails.m_strTrailer = strTrailer;

      lResult = m_database.SetDetailsForMovie(pItem->GetPath(), movieDetails, art);
      movieDetails.m_iDbId = lResult;
      movieDetails.m_type = MediaTypeMovie;

      // setup links to shows if the linked shows are in the db
      for (unsigned int i=0; i < movieDetails.m_showLink.size(); ++i)
      {
        CFileItemList items;
        m_database.GetTvShowsByName(movieDetails.m_showLink[i], items);
        if (items.Size())
          m_database.LinkMovieToTvshow(lResult, items[0]->GetVideoInfoTag()->m_iDbId, false);
        else
          CLog::Log(LOGDEBUG, "VideoInfoScanner: Failed to link movie %s to show %s", movieDetails.m_strTitle.c_str(), movieDetails.m_showLink[i].c_str());
      }
    }
    else if (content == CONTENT_TVSHOWS)
    {
      if (pItem->m_bIsFolder)
      {
        /*
         multipaths are not stored in the database, so in the case we have one,
         we split the paths, and compute the parent paths in each case.
         */
        vector<string> multipath;
        if (!URIUtils::IsMultiPath(pItem->GetPath()) || !CMultiPathDirectory::GetPaths(pItem->GetPath(), multipath))
          multipath.push_back(pItem->GetPath());
        vector< pair<string, string> > paths;
        for (vector<string>::const_iterator i = multipath.begin(); i != multipath.end(); ++i)
          paths.push_back(make_pair(*i, URIUtils::GetParentPath(*i)));

        map<int, map<string, string> > seasonArt;

        if (!libraryImport)
          GetSeasonThumbs(movieDetails, seasonArt, CVideoThumbLoader::GetArtTypes(MediaTypeSeason), useLocal);

        lResult = m_database.SetDetailsForTvShow(paths, movieDetails, art, seasonArt);
        movieDetails.m_iDbId = lResult;
        movieDetails.m_type = MediaTypeTvShow;
      }
      else
      {
        // we add episode then set details, as otherwise set details will delete the
        // episode then add, which breaks multi-episode files.
        int idShow = showInfo ? showInfo->m_iDbId : -1;
        int idEpisode = m_database.AddEpisode(idShow, pItem->GetPath());
        lResult = m_database.SetDetailsForEpisode(pItem->GetPath(), movieDetails, art, idShow, idEpisode);
        movieDetails.m_iDbId = lResult;
        movieDetails.m_type = MediaTypeEpisode;
        movieDetails.m_strShowTitle = showInfo ? showInfo->m_strTitle : "";
        if (movieDetails.m_fEpBookmark > 0)
        {
          movieDetails.m_strFileNameAndPath = pItem->GetPath();
          CBookmark bookmark;
          bookmark.timeInSeconds = movieDetails.m_fEpBookmark;
          bookmark.seasonNumber = movieDetails.m_iSeason;
          bookmark.episodeNumber = movieDetails.m_iEpisode;
          m_database.AddBookMarkForEpisode(movieDetails, bookmark);
        }
      }
    }
    else if (content == CONTENT_MUSICVIDEOS)
    {
      lResult = m_database.SetDetailsForMusicVideo(pItem->GetPath(), movieDetails, art);
      movieDetails.m_iDbId = lResult;
      movieDetails.m_type = MediaTypeMusicVideo;
    }

    if (g_advancedSettings.m_bVideoLibraryImportWatchedState || libraryImport)
      m_database.SetPlayCount(*pItem, movieDetails.m_playCount, movieDetails.m_lastPlayed);

    if ((g_advancedSettings.m_bVideoLibraryImportResumePoint || libraryImport) &&
        movieDetails.m_resumePoint.IsSet())
      m_database.AddBookMarkToFile(pItem->GetPath(), movieDetails.m_resumePoint, CBookmark::RESUME);

    m_database.Close();

    CFileItemPtr itemCopy = CFileItemPtr(new CFileItem(*pItem));
    CVariant data;
    if (m_bRunning)
      data["transaction"] = true;
    ANNOUNCEMENT::CAnnouncementManager::Get().Announce(ANNOUNCEMENT::VideoLibrary, "xbmc", "OnUpdate", itemCopy, data);
    return lResult;
  }

  string ContentToMediaType(CONTENT_TYPE content, bool folder)
  {
    switch (content)
    {
      case CONTENT_MOVIES:
        return MediaTypeMovie;
      case CONTENT_MUSICVIDEOS:
        return MediaTypeMusicVideo;
      case CONTENT_TVSHOWS:
        return folder ? MediaTypeTvShow : MediaTypeEpisode;
      default:
        return "";
    }
  }

  std::string CVideoInfoScanner::GetArtTypeFromSize(unsigned int width, unsigned int height)
  {
    std::string type = "thumb";
    if (width*5 < height*4)
      type = "poster";
    else if (width*1 > height*4)
      type = "banner";
    return type;
  }

  void CVideoInfoScanner::GetArtwork(CFileItem *pItem, const CONTENT_TYPE &content, bool bApplyToDir, bool useLocal, const std::string &actorArtPath)
  {
    CVideoInfoTag &movieDetails = *pItem->GetVideoInfoTag();
    movieDetails.m_fanart.Unpack();
    movieDetails.m_strPictureURL.Parse();

    CGUIListItem::ArtMap art = pItem->GetArt();

    // get and cache thumb images
    vector<string> artTypes = CVideoThumbLoader::GetArtTypes(ContentToMediaType(content, pItem->m_bIsFolder));
    vector<string>::iterator i = find(artTypes.begin(), artTypes.end(), "fanart");
    if (i != artTypes.end())
      artTypes.erase(i); // fanart is handled below
    bool lookForThumb = find(artTypes.begin(), artTypes.end(), "thumb") == artTypes.end() &&
                        art.find("thumb") == art.end();
    // find local art
    if (useLocal)
    {
      for (vector<string>::const_iterator i = artTypes.begin(); i != artTypes.end(); ++i)
      {
        if (art.find(*i) == art.end())
        {
          std::string image = CVideoThumbLoader::GetLocalArt(*pItem, *i, bApplyToDir);
          if (!image.empty())
            art.insert(make_pair(*i, image));
        }
      }
      // find and classify the local thumb (backcompat) if available
      if (lookForThumb)
      {
        std::string image = CVideoThumbLoader::GetLocalArt(*pItem, "thumb", bApplyToDir);
        if (!image.empty())
        { // cache the image and determine sizing
          CTextureDetails details;
          if (CTextureCache::Get().CacheImage(image, details))
          {
            std::string type = GetArtTypeFromSize(details.width, details.height);
            if (art.find(type) == art.end())
              art.insert(make_pair(type, image));
          }
        }
      }
    }

    // find online art
    for (vector<string>::const_iterator i = artTypes.begin(); i != artTypes.end(); ++i)
    {
      if (art.find(*i) == art.end())
      {
        std::string image = GetImage(pItem, false, bApplyToDir, *i);
        if (!image.empty())
          art.insert(make_pair(*i, image));
      }
    }

    // use the first piece of online art as the first art type if no thumb type is available yet
    if (art.empty() && lookForThumb)
    {
      std::string image = GetImage(pItem, false, bApplyToDir, "thumb");
      if (!image.empty())
        art.insert(make_pair(artTypes.front(), image));
    }

    // get & save fanart image (treated separately due to it being stored in m_fanart)
    bool isEpisode = (content == CONTENT_TVSHOWS && !pItem->m_bIsFolder);
    if (!isEpisode && art.find("fanart") == art.end())
    {
      string fanart = GetFanart(pItem, useLocal);
      if (!fanart.empty())
        art.insert(make_pair("fanart", fanart));
    }

    for (CGUIListItem::ArtMap::const_iterator i = art.begin(); i != art.end(); ++i)
      CTextureCache::Get().BackgroundCacheImage(i->second);

    pItem->SetArt(art);

    // parent folder to apply the thumb to and to search for local actor thumbs
    std::string parentDir = URIUtils::GetBasePath(pItem->GetPath());
    if (CSettings::Get().GetBool("videolibrary.actorthumbs"))
      FetchActorThumbs(movieDetails.m_cast, actorArtPath.empty() ? parentDir : actorArtPath);
    if (bApplyToDir)
      ApplyThumbToFolder(parentDir, art["thumb"]);
  }

  std::string CVideoInfoScanner::GetImage(CFileItem *pItem, bool useLocal, bool bApplyToDir, const std::string &type)
  {
    std::string thumb;
    if (useLocal)
      thumb = CVideoThumbLoader::GetLocalArt(*pItem, type, bApplyToDir);

    if (thumb.empty())
    {
      thumb = CScraperUrl::GetThumbURL(pItem->GetVideoInfoTag()->m_strPictureURL.GetFirstThumb(type));
      if (!thumb.empty())
      {
        if (thumb.find("http://") == string::npos &&
            thumb.find("/") == string::npos &&
            thumb.find("\\") == string::npos)
        {
          std::string strPath = URIUtils::GetDirectory(pItem->GetPath());
          thumb = URIUtils::AddFileToFolder(strPath, thumb);
        }
      }
    }
    return thumb;
  }

  std::string CVideoInfoScanner::GetFanart(CFileItem *pItem, bool useLocal)
  {
    if (!pItem)
      return "";
    std::string fanart = pItem->GetArt("fanart");
    if (fanart.empty() && useLocal)
      fanart = pItem->FindLocalArt("fanart.jpg", true);
    if (fanart.empty())
      fanart = pItem->GetVideoInfoTag()->m_fanart.GetImageURL();
    return fanart;
  }

  INFO_RET CVideoInfoScanner::OnProcessSeriesFolder(EPISODELIST& files, const ADDON::ScraperPtr &scraper, bool useLocal, const CVideoInfoTag& showInfo, CGUIDialogProgress* pDlgProgress /* = NULL */)
  {
    if (pDlgProgress)
    {
      pDlgProgress->SetLine(1, showInfo.m_strTitle);
      pDlgProgress->SetLine(2, 20361);
      pDlgProgress->SetPercentage(0);
      pDlgProgress->ShowProgressBar(true);
      pDlgProgress->Progress();
    }

    EPISODELIST episodes;
    bool hasEpisodeGuide = false;

    int iMax = files.size();
    int iCurr = 1;
    for (EPISODELIST::iterator file = files.begin(); file != files.end(); ++file)
    {
      m_nfoReader.Close();
      if (pDlgProgress)
      {
        pDlgProgress->SetLine(2, 20361);
        pDlgProgress->SetPercentage((int)((float)(iCurr++)/iMax*100));
        pDlgProgress->Progress();
      }
      if (m_handle)
        m_handle->SetPercentage(100.f*iCurr++/iMax);

      if ((pDlgProgress && pDlgProgress->IsCanceled()) || m_bStop)
        return INFO_CANCELLED;

      if (m_database.GetEpisodeId(file->strPath, file->iEpisode, file->iSeason) > -1)
      {
        if (m_handle)
          m_handle->SetText(g_localizeStrings.Get(20415));
        continue;
      }

      CFileItem item;
      item.SetPath(file->strPath);

      // handle .nfo files
      CNfoFile::NFOResult result=CNfoFile::NO_NFO;
      CScraperUrl scrUrl;
      ScraperPtr info(scraper);
      item.GetVideoInfoTag()->m_iEpisode = file->iEpisode;
      if (useLocal)
        result = CheckForNFOFile(&item, false, info,scrUrl);
      if (result == CNfoFile::FULL_NFO)
      {
        m_nfoReader.GetDetails(*item.GetVideoInfoTag());
        // override with episode and season number from file if available
        if (file->iEpisode > -1)
        {
          item.GetVideoInfoTag()->m_iEpisode = file->iEpisode;
          item.GetVideoInfoTag()->m_iSeason = file->iSeason;
        }
        if (AddVideo(&item, CONTENT_TVSHOWS, file->isFolder, true, &showInfo) < 0)
          return INFO_ERROR;
        continue;
      }

      if (!hasEpisodeGuide)
      {
        // fetch episode guide
        if (!showInfo.m_strEpisodeGuide.empty())
        {
          CScraperUrl url;
          url.ParseEpisodeGuide(showInfo.m_strEpisodeGuide);

          if (pDlgProgress)
          {
            pDlgProgress->SetLine(2, 20354);
            pDlgProgress->Progress();
          }

          CVideoInfoDownloader imdb(scraper);
          if (!imdb.GetEpisodeList(url, episodes))
            return INFO_NOT_FOUND;

          hasEpisodeGuide = true;
        }
      }

      if (episodes.empty())
      {
        CLog::Log(LOGERROR, "VideoInfoScanner: Asked to lookup episode %s"
                            " online, but we have no episode guide. Check your tvshow.nfo and make"
                            " sure the <episodeguide> tag is in place.", CURL::GetRedacted(file->strPath).c_str());
        continue;
      }

      EPISODE key(file->iSeason, file->iEpisode, file->iSubepisode);
      EPISODE backupkey(file->iSeason, file->iEpisode, 0);
      bool bFound = false;
      EPISODELIST::iterator guide = episodes.begin();;
      EPISODELIST matches;

      for (; guide != episodes.end(); ++guide )
      {
        if ((file->iEpisode!=-1) && (file->iSeason!=-1))
        {
          if (key==*guide)
          {
            bFound = true;
            break;
          }
          else if ((file->iSubepisode!=0) && (backupkey==*guide))
          {
            matches.push_back(*guide);
            continue;
          }
        }
        if (file->cDate.IsValid() && guide->cDate.IsValid() && file->cDate==guide->cDate)
        {
          matches.push_back(*guide);
          continue;
        }
        if (!guide->cScraperUrl.strTitle.empty() && StringUtils::EqualsNoCase(guide->cScraperUrl.strTitle, file->strTitle))
        {
          bFound = true;
          break;
        }
      }

      if (!bFound)
      {
        /*
         * If there is only one match or there are matches but no title to compare with to help
         * identify the best match, then pick the first match as the best possible candidate.
         *
         * Otherwise, use the title to further refine the best match.
         */
        if (matches.size() == 1 || (file->strTitle.empty() && matches.size() > 1))
        {
          guide = matches.begin();
          bFound = true;
        }
        else if (!file->strTitle.empty())
        {
          double minscore = 0; // Default minimum score is 0 to find whatever is the best match.

          EPISODELIST *candidates;
          if (matches.empty()) // No matches found using earlier criteria. Use fuzzy match on titles across all episodes.
          {
            minscore = 0.8; // 80% should ensure a good match.
            candidates = &episodes;
          }
          else // Multiple matches found. Use fuzzy match on the title with already matched episodes to pick the best.
            candidates = &matches;

          vector<string> titles;
          for (guide = candidates->begin(); guide != candidates->end(); ++guide)
          {
            StringUtils::ToLower(guide->cScraperUrl.strTitle);
            titles.push_back(guide->cScraperUrl.strTitle);
          }

          double matchscore;
          std::string loweredTitle(file->strTitle);
          StringUtils::ToLower(loweredTitle);
          int index = StringUtils::FindBestMatch(loweredTitle, titles, matchscore);
          if (matchscore >= minscore)
          {
            guide = candidates->begin() + index;
            bFound = true;
            CLog::Log(LOGDEBUG,"%s fuzzy title match for show: '%s', title: '%s', match: '%s', score: %f >= %f",
                      __FUNCTION__, showInfo.m_strTitle.c_str(), file->strTitle.c_str(), titles[index].c_str(), matchscore, minscore);
          }
        }
      }

      if (bFound)
      {
        CVideoInfoDownloader imdb(scraper);
        CFileItem item;
        item.SetPath(file->strPath);
        if (!imdb.GetEpisodeDetails(guide->cScraperUrl, *item.GetVideoInfoTag(), pDlgProgress))
          return INFO_NOT_FOUND; // TODO: should we just skip to the next episode?
          
        // Only set season/epnum from filename when it is not already set by a scraper
        if (item.GetVideoInfoTag()->m_iSeason == -1)
          item.GetVideoInfoTag()->m_iSeason = guide->iSeason;
        if (item.GetVideoInfoTag()->m_iEpisode == -1)
          item.GetVideoInfoTag()->m_iEpisode = guide->iEpisode;
          
        if (AddVideo(&item, CONTENT_TVSHOWS, file->isFolder, useLocal, &showInfo) < 0)
          return INFO_ERROR;
      }
      else
      {
        CLog::Log(LOGDEBUG,"%s - no match for show: '%s', season: %d, episode: %d.%d, airdate: '%s', title: '%s'",
                  __FUNCTION__, showInfo.m_strTitle.c_str(), file->iSeason, file->iEpisode, file->iSubepisode,
                  file->cDate.GetAsLocalizedDate().c_str(), file->strTitle.c_str());
      }
    }
    return INFO_ADDED;
  }

  std::string CVideoInfoScanner::GetnfoFile(CFileItem *item, bool bGrabAny) const
  {
    std::string nfoFile;
    // Find a matching .nfo file
    if (!item->m_bIsFolder)
    {
      if (URIUtils::IsInRAR(item->GetPath())) // we have a rarred item - we want to check outside the rars
      {
        CFileItem item2(*item);
        CURL url(item->GetPath());
        std::string strPath = URIUtils::GetDirectory(url.GetHostName());
        item2.SetPath(URIUtils::AddFileToFolder(strPath, URIUtils::GetFileName(item->GetPath())));
        return GetnfoFile(&item2, bGrabAny);
      }

      // grab the folder path
      std::string strPath = URIUtils::GetDirectory(item->GetPath());

      if (bGrabAny && !item->IsStack())
      { // looking up by folder name - movie.nfo takes priority - but not for stacked items (handled below)
        nfoFile = URIUtils::AddFileToFolder(strPath, "movie.nfo");
        if (CFile::Exists(nfoFile))
          return nfoFile;
      }

      // try looking for .nfo file for a stacked item
      if (item->IsStack())
      {
        // first try .nfo file matching first file in stack
        CStackDirectory dir;
        std::string firstFile = dir.GetFirstStackedFile(item->GetPath());
        CFileItem item2;
        item2.SetPath(firstFile);
        nfoFile = GetnfoFile(&item2, bGrabAny);
        // else try .nfo file matching stacked title
        if (nfoFile.empty())
        {
          std::string stackedTitlePath = dir.GetStackedTitlePath(item->GetPath());
          item2.SetPath(stackedTitlePath);
          nfoFile = GetnfoFile(&item2, bGrabAny);
        }
      }
      else
      {
        // already an .nfo file?
        if (URIUtils::HasExtension(item->GetPath(), ".nfo"))
          nfoFile = item->GetPath();
        // no, create .nfo file
        else
          nfoFile = URIUtils::ReplaceExtension(item->GetPath(), ".nfo");
      }

      // test file existence
      if (!nfoFile.empty() && !CFile::Exists(nfoFile))
        nfoFile.clear();

      if (nfoFile.empty()) // final attempt - strip off any cd1 folders
      {
        URIUtils::RemoveSlashAtEnd(strPath); // need no slash for the check that follows
        CFileItem item2;
        if (StringUtils::EndsWithNoCase(strPath, "cd1"))
        {
          strPath.erase(strPath.size() - 3);
          item2.SetPath(URIUtils::AddFileToFolder(strPath, URIUtils::GetFileName(item->GetPath())));
          return GetnfoFile(&item2, bGrabAny);
        }
      }

      if (nfoFile.empty() && item->IsOpticalMediaFile())
      {
        CFileItem parentDirectory(item->GetLocalMetadataPath(), true);
        nfoFile = GetnfoFile(&parentDirectory, true);
      }
    }
    // folders (or stacked dvds) can take any nfo file if there's a unique one
    if (item->m_bIsFolder || item->IsOpticalMediaFile() || (bGrabAny && nfoFile.empty()))
    {
      // see if there is a unique nfo file in this folder, and if so, use that
      CFileItemList items;
      CDirectory dir;
      std::string strPath;
      if (item->m_bIsFolder)
        strPath = item->GetPath();
      else
        strPath = URIUtils::GetDirectory(item->GetPath());

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
          return items[numNFO]->GetPath();
      }
    }

    return nfoFile;
  }

  bool CVideoInfoScanner::GetDetails(CFileItem *pItem, CScraperUrl &url, const ScraperPtr& scraper, CNfoFile *nfoFile, CGUIDialogProgress* pDialog /* = NULL */)
  {
    CVideoInfoTag movieDetails;

    if (m_handle && !url.strTitle.empty())
      m_handle->SetText(url.strTitle);

    CVideoInfoDownloader imdb(scraper);
    bool ret = imdb.GetDetails(url, movieDetails, pDialog);

    if (ret)
    {
      if (nfoFile)
        nfoFile->GetDetails(movieDetails,NULL,true);

      if (m_handle && url.strTitle.empty())
        m_handle->SetText(movieDetails.m_strTitle);

      if (pDialog)
      {
        pDialog->SetLine(1, movieDetails.m_strTitle);
        pDialog->Progress();
      }

      *pItem->GetVideoInfoTag() = movieDetails;
      return true;
    }
    return false; // no info found, or cancelled
  }

  void CVideoInfoScanner::ApplyThumbToFolder(const std::string &folder, const std::string &imdbThumb)
  {
    // copy icon to folder also;
    if (!imdbThumb.empty())
    {
      CFileItem folderItem(folder, true);
      CThumbLoader loader;
      loader.SetCachedImage(folderItem, "thumb", imdbThumb);
    }
  }

  int CVideoInfoScanner::GetPathHash(const CFileItemList &items, std::string &hash)
  {
    // Create a hash based on the filenames, filesize and filedate.  Also count the number of files
    if (0 == items.Size()) return 0;
    XBMC::XBMC_MD5 md5state;
    int count = 0;
    for (int i = 0; i < items.Size(); ++i)
    {
      const CFileItemPtr pItem = items[i];
      md5state.append(pItem->GetPath());
      md5state.append((unsigned char *)&pItem->m_dwSize, sizeof(pItem->m_dwSize));
      FILETIME time = pItem->m_dateTime;
      md5state.append((unsigned char *)&time, sizeof(FILETIME));
      if (pItem->IsVideo() && !pItem->IsPlayList() && !pItem->IsNFO())
        count++;
    }
    hash = md5state.getDigest();
    return count;
  }

  bool CVideoInfoScanner::CanFastHash(const CFileItemList &items, const vector<string> &excludes) const
  {
    if (!g_advancedSettings.m_bVideoLibraryUseFastHash)
      return false;

    for (int i = 0; i < items.Size(); ++i)
    {
      if (items[i]->m_bIsFolder && !CUtil::ExcludeFileOrFolder(items[i]->GetPath(), excludes))
        return false;
    }
    return true;
  }

  std::string CVideoInfoScanner::GetFastHash(const std::string &directory, const vector<string> &excludes) const
  {
    XBMC::XBMC_MD5 md5state;

    if (excludes.size())
      md5state.append(StringUtils::Join(excludes, "|"));

    struct __stat64 buffer;
    if (XFILE::CFile::Stat(directory, &buffer) == 0)
    {
      int64_t time = buffer.st_mtime;
      if (!time)
        time = buffer.st_ctime;
      if (time)
      {
        md5state.append((unsigned char *)&time, sizeof(time));
        return md5state.getDigest();
      }
    }
    return "";
  }

  std::string CVideoInfoScanner::GetRecursiveFastHash(const std::string &directory, const vector<string> &excludes) const
  {
    CFileItemList items;
    items.Add(CFileItemPtr(new CFileItem(directory, true)));
    CUtil::GetRecursiveDirsListing(directory, items, DIR_FLAG_NO_FILE_DIRS | DIR_FLAG_NO_FILE_INFO);

    XBMC::XBMC_MD5 md5state;

    if (excludes.size())
      md5state.append(StringUtils::Join(excludes, "|"));

    int64_t time = 0;
    for (int i=0; i < items.Size(); ++i)
    {
      int64_t stat_time = 0;
      struct __stat64 buffer;
      if (XFILE::CFile::Stat(items[i]->GetPath(), &buffer) == 0)
      {
        // TODO: some filesystems may return the mtime/ctime inline, in which case this is
        // unnecessarily expensive. Consider supporting Stat() in our directory cache?
        stat_time = buffer.st_mtime ? buffer.st_mtime : buffer.st_ctime;
        time += stat_time;
      }

      if (!stat_time)
        return "";
    }

    if (time)
    {
      md5state.append((unsigned char *)&time, sizeof(time));
      return md5state.getDigest();
    }
    return "";
  }

  void CVideoInfoScanner::GetSeasonThumbs(const CVideoInfoTag &show, map<int, map<string, string> > &seasonArt, const vector<string> &artTypes, bool useLocal)
  {
    bool lookForThumb = find(artTypes.begin(), artTypes.end(), "thumb") == artTypes.end();

    // find the maximum number of seasons we have thumbs for (local + remote)
    int maxSeasons = show.m_strPictureURL.GetMaxSeasonThumb();

    CFileItemList items;
    CDirectory::GetDirectory(show.m_strPath, items, ".png|.jpg|.tbn", DIR_FLAG_NO_FILE_DIRS | DIR_FLAG_NO_FILE_INFO);
    CRegExp reg;
    if (items.Size() && reg.RegComp("season([0-9]+)(-[a-z]+)?\\.(tbn|jpg|png)"))
    {
      for (int i = 0; i < items.Size(); i++)
      {
        std::string name = URIUtils::GetFileName(items[i]->GetPath());
        if (reg.RegFind(name) > -1)
        {
          int season = atoi(reg.GetMatch(1).c_str());
          if (season > maxSeasons)
            maxSeasons = season;
        }
      }
    }
    for (int season = -1; season <= maxSeasons; season++)
    {
      // skip if we already have some art
      map<int, map<string, string> >::const_iterator it = seasonArt.find(season);
      if (it != seasonArt.end() && !it->second.empty())
        continue;

      map<string, string> art;
      if (useLocal)
      {
        string basePath;
        if (season == -1)
          basePath = "season-all";
        else if (season == 0)
          basePath = "season-specials";
        else
          basePath = StringUtils::Format("season%02i", season);
        CFileItem artItem(URIUtils::AddFileToFolder(show.m_strPath, basePath), false);

        for (vector<string>::const_iterator i = artTypes.begin(); i != artTypes.end(); ++i)
        {
          std::string image = CVideoThumbLoader::GetLocalArt(artItem, *i, false);
          if (!image.empty())
            art.insert(make_pair(*i, image));
        }
        // find and classify the local thumb (backcompat) if available
        if (lookForThumb)
        {
          std::string image = CVideoThumbLoader::GetLocalArt(artItem, "thumb", false);
          if (!image.empty())
          { // cache the image and determine sizing
            CTextureDetails details;
            if (CTextureCache::Get().CacheImage(image, details))
            {
              std::string type = GetArtTypeFromSize(details.width, details.height);
              if (art.find(type) == art.end())
                art.insert(make_pair(type, image));
            }
          }
        }
      }

      // find online art
      for (vector<string>::const_iterator i = artTypes.begin(); i != artTypes.end(); ++i)
      {
        if (art.find(*i) == art.end())
        {
          string image = CScraperUrl::GetThumbURL(show.m_strPictureURL.GetSeasonThumb(season, *i));
          if (!image.empty())
            art.insert(make_pair(*i, image));
        }
      }
      // use the first piece of online art as the first art type if no thumb type is available yet
      if (art.empty() && lookForThumb)
      {
        string image = CScraperUrl::GetThumbURL(show.m_strPictureURL.GetSeasonThumb(season, "thumb"));
        if (!image.empty())
          art.insert(make_pair(artTypes.front(), image));
      }

      seasonArt[season] = art;
    }
  }

  void CVideoInfoScanner::FetchActorThumbs(vector<SActorInfo>& actors, const std::string& strPath)
  {
    CFileItemList items;
    std::string actorsDir = URIUtils::AddFileToFolder(strPath, ".actors");
    if (CDirectory::Exists(actorsDir))
      CDirectory::GetDirectory(actorsDir, items, ".png|.jpg|.tbn", DIR_FLAG_NO_FILE_DIRS |
                               DIR_FLAG_NO_FILE_INFO);
    for (vector<SActorInfo>::iterator i = actors.begin(); i != actors.end(); ++i)
    {
      if (i->thumb.empty())
      {
        std::string thumbFile = i->strName;
        StringUtils::Replace(thumbFile, ' ', '_');
        for (int j = 0; j < items.Size(); j++)
        {
          std::string compare = URIUtils::GetFileName(items[j]->GetPath());
          URIUtils::RemoveExtension(compare);
          if (!items[j]->m_bIsFolder && compare == thumbFile)
          {
            i->thumb = items[j]->GetPath();
            break;
          }
        }
        if (i->thumb.empty() && !i->thumbUrl.GetFirstThumb().m_url.empty())
          i->thumb = CScraperUrl::GetThumbURL(i->thumbUrl.GetFirstThumb());
        if (!i->thumb.empty())
          CTextureCache::Get().BackgroundCacheImage(i->thumb);
      }
    }
  }

  CNfoFile::NFOResult CVideoInfoScanner::CheckForNFOFile(CFileItem* pItem, bool bGrabAny, ScraperPtr& info, CScraperUrl& scrUrl)
  {
    std::string strNfoFile;
    if (info->Content() == CONTENT_MOVIES || info->Content() == CONTENT_MUSICVIDEOS
        || (info->Content() == CONTENT_TVSHOWS && !pItem->m_bIsFolder))
      strNfoFile = GetnfoFile(pItem, bGrabAny);
    if (info->Content() == CONTENT_TVSHOWS && pItem->m_bIsFolder)
      strNfoFile = URIUtils::AddFileToFolder(pItem->GetPath(), "tvshow.nfo");

    CNfoFile::NFOResult result=CNfoFile::NO_NFO;
    if (!strNfoFile.empty() && CFile::Exists(strNfoFile))
    {
      if (info->Content() == CONTENT_TVSHOWS && !pItem->m_bIsFolder)
        result = m_nfoReader.Create(strNfoFile,info,pItem->GetVideoInfoTag()->m_iEpisode);
      else
        result = m_nfoReader.Create(strNfoFile,info);

      std::string type;
      switch(result)
      {
        case CNfoFile::COMBINED_NFO:
          type = "Mixed";
          break;
        case CNfoFile::FULL_NFO:
          type = "Full";
          break;
        case CNfoFile::URL_NFO:
          type = "URL";
          break;
        case CNfoFile::NO_NFO:
          type = "";
          break;
        default:
          type = "malformed";
      }
      if (result != CNfoFile::NO_NFO)
        CLog::Log(LOGDEBUG, "VideoInfoScanner: Found matching %s NFO file: %s", type.c_str(), CURL::GetRedacted(strNfoFile).c_str());
      if (result == CNfoFile::FULL_NFO)
      {
        if (info->Content() == CONTENT_TVSHOWS)
          info = m_nfoReader.GetScraperInfo();
      }
      else if (result != CNfoFile::NO_NFO && result != CNfoFile::ERROR_NFO)
      {
        scrUrl = m_nfoReader.ScraperUrl();
        info = m_nfoReader.GetScraperInfo();

        CLog::Log(LOGDEBUG, "VideoInfoScanner: Fetching url '%s' using %s scraper (content: '%s')",
          scrUrl.m_url[0].m_url.c_str(), info->Name().c_str(), TranslateContent(info->Content()).c_str());

        if (result == CNfoFile::COMBINED_NFO)
          m_nfoReader.GetDetails(*pItem->GetVideoInfoTag());
      }
    }
    else
      CLog::Log(LOGDEBUG, "VideoInfoScanner: No NFO file found. Using title search for '%s'", CURL::GetRedacted(pItem->GetPath()).c_str());

    return result;
  }

  bool CVideoInfoScanner::DownloadFailed(CGUIDialogProgress* pDialog)
  {
    if (g_advancedSettings.m_bVideoScannerIgnoreErrors)
      return true;

    if (pDialog)
    {
      CGUIDialogOK::ShowAndGetInput(20448, 20449);
      return false;
    }
    return CGUIDialogYesNo::ShowAndGetInput(20448, 20450);
  }

  bool CVideoInfoScanner::ProgressCancelled(CGUIDialogProgress* progress, int heading, const std::string &line1)
  {
    if (progress)
    {
      progress->SetHeading(heading);
      progress->SetLine(0, line1);
      progress->SetLine(2, "");
      progress->Progress();
      return progress->IsCanceled();
    }
    return m_bStop;
  }

  int CVideoInfoScanner::FindVideo(const std::string &videoName, const ScraperPtr &scraper, CScraperUrl &url, CGUIDialogProgress *progress)
  {
    MOVIELIST movielist;
    CVideoInfoDownloader imdb(scraper);
    int returncode = imdb.FindMovie(videoName, movielist, progress);
    if (returncode < 0 || (returncode == 0 && (m_bStop || !DownloadFailed(progress))))
    { // scraper reported an error, or we had an error and user wants to cancel the scan
      m_bStop = true;
      return -1; // cancelled
    }
    if (returncode > 0 && movielist.size())
    {
      url = movielist[0];
      return 1;  // found a movie
    }
    return 0;    // didn't find anything
  }
}
