/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "VideoInfoScanner.h"

#include "FileItem.h"
#include "GUIInfoManager.h"
#include "GUIUserMessages.h"
#include "NfoFile.h"
#include "ServiceBroker.h"
#include "TextureCache.h"
#include "URL.h"
#include "Util.h"
#include "VideoInfoDownloader.h"
#include "cores/VideoPlayer/DVDFileInfo.h"
#include "dialogs/GUIDialogExtendedProgressBar.h"
#include "dialogs/GUIDialogProgress.h"
#include "events/EventLog.h"
#include "events/MediaLibraryEvent.h"
#include "filesystem/Directory.h"
#include "filesystem/DirectoryCache.h"
#include "filesystem/File.h"
#include "filesystem/MultiPathDirectory.h"
#include "filesystem/PluginDirectory.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "interfaces/AnnouncementManager.h"
#include "messaging/helpers/DialogHelper.h"
#include "messaging/helpers/DialogOKHelper.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "tags/VideoInfoTagLoaderFactory.h"
#include "utils/Digest.h"
#include "utils/FileExtensionProvider.h"
#include "utils/RegExp.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "utils/log.h"
#include "video/VideoThumbLoader.h"

#include <algorithm>
#include <memory>
#include <utility>

using namespace XFILE;
using namespace ADDON;
using namespace KODI::MESSAGING;

using KODI::MESSAGING::HELPERS::DialogResponse;
using KODI::UTILITY::CDigest;

namespace VIDEO
{

  CVideoInfoScanner::CVideoInfoScanner()
  {
    m_bStop = false;
    m_scanAll = false;
  }

  CVideoInfoScanner::~CVideoInfoScanner()
  = default;

  void CVideoInfoScanner::Process()
  {
    m_bStop = false;

    try
    {
      if (m_showDialog && !CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_VIDEOLIBRARY_BACKGROUNDUPDATE))
      {
        CGUIDialogExtendedProgressBar* dialog =
          CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogExtendedProgressBar>(WINDOW_DIALOG_EXT_PROGRESS);
        if (dialog)
           m_handle = dialog->GetHandle(g_localizeStrings.Get(314));
      }

      // check if we only need to perform a cleaning
      if (m_bClean && m_pathsToScan.empty())
      {
        std::set<int> paths;
        m_database.CleanDatabase(m_handle, paths, false);

        if (m_handle)
          m_handle->MarkFinished();
        m_handle = NULL;

        m_bRunning = false;

        return;
      }

      auto start = std::chrono::steady_clock::now();

      m_database.Open();

      m_bCanInterrupt = true;

      CLog::Log(LOGINFO, "VideoInfoScanner: Starting scan ..");
      CServiceBroker::GetAnnouncementManager()->Announce(ANNOUNCEMENT::VideoLibrary,
                                                         "OnScanStarted");

      // Database operations should not be canceled
      // using Interrupt() while scanning as it could
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
        if (m_bStop)
        {
          bCancelled = true;
        }
        else if (!CDirectory::Exists(directory))
        {
          /*
           * Note that this will skip clean (if m_bClean is enabled) if the directory really
           * doesn't exist rather than a NAS being switched off.  A manual clean from settings
           * will still pick up and remove it though.
           */
          CLog::Log(LOGWARNING, "{} directory '{}' does not exist - skipping scan{}.", __FUNCTION__,
                    CURL::GetRedacted(directory), m_bClean ? " and clean" : "");
          m_pathsToScan.erase(m_pathsToScan.begin());
        }
        else if (!DoScan(directory))
          bCancelled = true;
      }

      if (!bCancelled)
      {
        if (m_bClean)
          m_database.CleanDatabase(m_handle, m_pathsToClean, false);
        else
        {
          if (m_handle)
            m_handle->SetTitle(g_localizeStrings.Get(331));
          m_database.Compress(false);
        }
      }

      CServiceBroker::GetGUI()->GetInfoManager().GetInfoProviders().GetLibraryInfoProvider().ResetLibraryBools();
      m_database.Close();

      auto end = std::chrono::steady_clock::now();
      auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

      CLog::Log(LOGINFO, "VideoInfoScanner: Finished scan. Scanning for video info took {} ms",
                duration.count());
    }
    catch (...)
    {
      CLog::Log(LOGERROR, "VideoInfoScanner: Exception while scanning.");
    }

    m_bRunning = false;
    CServiceBroker::GetAnnouncementManager()->Announce(ANNOUNCEMENT::VideoLibrary,
                                                       "OnScanFinished");

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
      std::vector<std::string> rootDirs;
      if (URIUtils::IsMultiPath(strDirectory))
        CMultiPathDirectory::GetPaths(strDirectory, rootDirs);
      else
        rootDirs.push_back(strDirectory);

      for (std::vector<std::string>::const_iterator it = rootDirs.begin(); it < rootDirs.end(); ++it)
      {
        m_pathsToScan.insert(*it);
        std::vector<std::pair<int, std::string>> subpaths;
        m_database.GetSubPaths(*it, subpaths);
        for (std::vector<std::pair<int, std::string>>::iterator it = subpaths.begin(); it < subpaths.end(); ++it)
          m_pathsToScan.insert(it->second);
      }
    }
    m_database.Close();
    m_bClean = CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_bVideoLibraryCleanOnUpdate;

    m_bRunning = true;
    Process();
  }

  void CVideoInfoScanner::Stop()
  {
    if (m_bCanInterrupt)
      m_database.Interrupt();

    m_bStop = true;
  }

  static void OnDirectoryScanned(const std::string& strDirectory)
  {
    CGUIMessage msg(GUI_MSG_DIRECTORY_SCANNED, 0, 0, 0);
    msg.SetStringParam(strDirectory);
    CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg);
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
    std::set<std::string>::iterator it = m_pathsToScan.find(strDirectory);
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
    const std::vector<std::string> &regexps = content == CONTENT_TVSHOWS ? CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_tvshowExcludeFromScanRegExps
                                                         : CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_moviesExcludeFromScanRegExps;

    if (CUtil::ExcludeFileOrFolder(strDirectory, regexps))
      return true;

    if (HasNoMedia(strDirectory))
      return true;

    bool ignoreFolder = !m_scanAll && settings.noupdate;
    if (content == CONTENT_NONE || ignoreFolder)
      return true;

    if (URIUtils::IsPlugin(strDirectory) && !CPluginDirectory::IsMediaLibraryScanningAllowed(TranslateContent(content), strDirectory))
    {
      CLog::Log(
          LOGINFO,
          "VideoInfoScanner: Plugin '{}' does not support media library scanning for '{}' content",
          CURL::GetRedacted(strDirectory), TranslateContent(content));
      return true;
    }

    std::string hash, dbHash;
    if (content == CONTENT_MOVIES ||content == CONTENT_MUSICVIDEOS)
    {
      if (m_handle)
      {
        int str = content == CONTENT_MOVIES ? 20317:20318;
        m_handle->SetTitle(StringUtils::Format(g_localizeStrings.Get(str), info->Name()));
      }

      std::string fastHash;
      if (CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_bVideoLibraryUseFastHash && !URIUtils::IsPlugin(strDirectory))
        fastHash = GetFastHash(strDirectory, regexps);

      if (m_database.GetPathHash(strDirectory, dbHash) && !fastHash.empty() && StringUtils::EqualsNoCase(fastHash, dbHash))
      { // fast hashes match - no need to process anything
        hash = fastHash;
      }
      else
      { // need to fetch the folder
        CDirectory::GetDirectory(strDirectory, items, CServiceBroker::GetFileExtensionProvider().GetVideoExtensions(),
                                 DIR_FLAG_DEFAULTS);
        // do not consider inner folders with .nomedia
        items.erase(std::remove_if(items.begin(), items.end(),
                                   [this](const CFileItemPtr& item) {
                                     return item->m_bIsFolder && HasNoMedia(item->GetPath());
                                   }),
                    items.end());
        items.Stack();

        // check whether to re-use previously computed fast hash
        if (!CanFastHash(items, regexps) || fastHash.empty())
          GetPathHash(items, hash);
        else
          hash = fastHash;
      }

      if (StringUtils::EqualsNoCase(hash, dbHash))
      { // hash matches - skipping
        CLog::Log(LOGDEBUG, "VideoInfoScanner: Skipping dir '{}' due to no change{}",
                  CURL::GetRedacted(strDirectory), !fastHash.empty() ? " (fasthash)" : "");
        bSkip = true;
      }
      else if (hash.empty())
      { // directory empty or non-existent - add to clean list and skip
        CLog::Log(LOGDEBUG,
                  "VideoInfoScanner: Skipping dir '{}' as it's empty or doesn't exist - adding to "
                  "clean list",
                  CURL::GetRedacted(strDirectory));
        if (m_bClean)
          m_pathsToClean.insert(m_database.GetPathId(strDirectory));
        bSkip = true;
      }
      else if (dbHash.empty())
      { // new folder - scan
        CLog::Log(LOGDEBUG, "VideoInfoScanner: Scanning dir '{}' as not in the database",
                  CURL::GetRedacted(strDirectory));
      }
      else
      { // hash changed - rescan
        CLog::Log(LOGDEBUG, "VideoInfoScanner: Rescanning dir '{}' due to change ({} != {})",
                  CURL::GetRedacted(strDirectory), dbHash, hash);
      }
    }
    else if (content == CONTENT_TVSHOWS)
    {
      if (m_handle)
        m_handle->SetTitle(StringUtils::Format(g_localizeStrings.Get(20319), info->Name()));

      if (foundDirectly && !settings.parent_name_root)
      {
        CDirectory::GetDirectory(strDirectory, items, CServiceBroker::GetFileExtensionProvider().GetVideoExtensions(),
                                 DIR_FLAG_DEFAULTS);
        items.SetPath(strDirectory);
        GetPathHash(items, hash);
        bSkip = true;
        if (!m_database.GetPathHash(strDirectory, dbHash) || !StringUtils::EqualsNoCase(dbHash, hash))
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
          CLog::Log(LOGDEBUG, "VideoInfoScanner: Finished adding information from dir {}",
                    CURL::GetRedacted(strDirectory));
        }
      }
      else
      {
        if (m_bClean)
          m_pathsToClean.insert(m_database.GetPathId(strDirectory));
        CLog::Log(LOGDEBUG, "VideoInfoScanner: No (new) information was found in dir {}",
                  CURL::GetRedacted(strDirectory));
      }
    }
    else if (!StringUtils::EqualsNoCase(hash, dbHash) && (content == CONTENT_MOVIES || content == CONTENT_MUSICVIDEOS))
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
    std::vector<int> seenPaths;
    for (int i = 0; i < items.Size(); ++i)
    {
      CFileItemPtr pItem = items[i];

      // we do this since we may have a override per dir
      ScraperPtr info2 = m_database.GetScraperForPath(pItem->m_bIsFolder ? pItem->GetPath() : items.GetPath());
      if (!info2) // skip
        continue;

      // Discard all .nomedia folders
      if (pItem->m_bIsFolder && HasNoMedia(pItem->GetPath()))
        continue;

      // Discard all exclude files defined by regExExclude
      if (CUtil::ExcludeFileOrFolder(pItem->GetPath(), (content == CONTENT_TVSHOWS) ? CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_tvshowExcludeFromScanRegExps
                                                                    : CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_moviesExcludeFromScanRegExps))
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
        CLog::Log(LOGERROR, "VideoInfoScanner: Unknown content type {} ({})", info2->Content(),
                  CURL::GetRedacted(pItem->GetPath()));
        FoundSomeInfo = false;
        break;
      }
      if (ret == INFO_CANCELLED || ret == INFO_ERROR)
      {
        CLog::Log(LOGWARNING,
                  "VideoInfoScanner: Error {} occurred while retrieving"
                  "information for {}.",
                  ret, CURL::GetRedacted(pItem->GetPath()));
        FoundSomeInfo = false;
        break;
      }
      if (ret == INFO_ADDED || ret == INFO_HAVE_ALREADY)
        FoundSomeInfo = true;
      else if (ret == INFO_NOT_FOUND)
      {
        CLog::Log(LOGWARNING,
                  "No information found for item '{}', it won't be added to the library.",
                  CURL::GetRedacted(pItem->GetPath()));

        MediaType mediaType = MediaTypeMovie;
        if (info2->Content() == CONTENT_TVSHOWS)
          mediaType = MediaTypeTvShow;
        else if (info2->Content() == CONTENT_MUSICVIDEOS)
          mediaType = MediaTypeMusicVideo;

        auto eventLog = CServiceBroker::GetEventLog();
        if (eventLog)
        {
          const std::string itemlogpath = (info2->Content() == CONTENT_TVSHOWS)
                                              ? CURL::GetRedacted(pItem->GetPath())
                                              : URIUtils::GetFileName(pItem->GetPath());

          eventLog->Add(EventPtr(new CMediaLibraryEvent(
              mediaType, pItem->GetPath(), 24145,
              StringUtils::Format(g_localizeStrings.Get(24147), mediaType, itemlogpath),
              EventLevel::Warning)));
        }
      }

      pURL = NULL;

      // Keep track of directories we've seen
      if (m_bClean && pItem->m_bIsFolder)
        seenPaths.push_back(m_database.GetPathId(pItem->GetPath()));
    }

    if (content == CONTENT_TVSHOWS && ! seenPaths.empty())
    {
      std::vector<std::pair<int, std::string>> libPaths;
      m_database.GetSubPaths(items.GetPath(), libPaths);
      for (std::vector<std::pair<int, std::string> >::iterator i = libPaths.begin(); i < libPaths.end(); ++i)
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

  CInfoScanner::INFO_RET
  CVideoInfoScanner::RetrieveInfoForTvShow(CFileItem *pItem,
                                           bool bDirNames,
                                           ScraperPtr &info2,
                                           bool useLocal,
                                           CScraperUrl* pURL,
                                           bool fetchEpisodes,
                                           CGUIDialogProgress* pDlgProgress)
  {
    const bool isSeason =
        pItem->HasVideoInfoTag() && pItem->GetVideoInfoTag()->m_type == MediaTypeSeason;

    int idTvShow = -1;
    int idSeason = -1;
    std::string strPath = pItem->GetPath();
    if (pItem->m_bIsFolder)
    {
      idTvShow = m_database.GetTvShowId(strPath);
      if (isSeason && idTvShow > -1)
        idSeason = m_database.GetSeasonId(idTvShow, pItem->GetVideoInfoTag()->m_iSeason);
    }
    else if (pItem->IsPlugin() && pItem->HasVideoInfoTag() && pItem->GetVideoInfoTag()->m_iIdShow >= 0)
    {
      // for plugin source we cannot get idTvShow from episode path with URIUtils::GetDirectory() in all cases
      // so use m_iIdShow from video info tag if possible
      idTvShow = pItem->GetVideoInfoTag()->m_iIdShow;
      CVideoInfoTag showInfo;
      if (m_database.GetTvShowInfo(std::string(), showInfo, idTvShow, nullptr, 0))
        strPath = showInfo.GetPath();
    }
    else
    {
      strPath = URIUtils::GetDirectory(strPath);
      idTvShow = m_database.GetTvShowId(strPath);
      if (isSeason && idTvShow > -1)
        idSeason = m_database.GetSeasonId(idTvShow, pItem->GetVideoInfoTag()->m_iSeason);
    }
    if (idTvShow > -1 && (!isSeason || idSeason > -1) && (fetchEpisodes || !pItem->m_bIsFolder))
    {
      INFO_RET ret = RetrieveInfoForEpisodes(pItem, idTvShow, info2, useLocal, pDlgProgress);
      if (ret == INFO_ADDED)
        m_database.SetPathHash(strPath, pItem->GetProperty("hash").asString());
      return ret;
    }

    if (ProgressCancelled(pDlgProgress, pItem->m_bIsFolder ? 20353 : 20361,
                          pItem->m_bIsFolder ? pItem->GetVideoInfoTag()->m_strShowTitle
                                             : pItem->GetVideoInfoTag()->m_strTitle))
      return INFO_CANCELLED;

    if (m_handle)
      m_handle->SetText(pItem->GetMovieName(bDirNames));

    CInfoScanner::INFO_TYPE result=CInfoScanner::NO_NFO;
    CScraperUrl scrUrl;
    // handle .nfo files
    std::unique_ptr<IVideoInfoTagLoader> loader;
    if (useLocal)
    {
      loader.reset(CVideoInfoTagLoaderFactory::CreateLoader(*pItem, info2, bDirNames));
      if (loader)
      {
        pItem->GetVideoInfoTag()->Reset();
        result = loader->Load(*pItem->GetVideoInfoTag(), false);
      }
    }

    if (result == CInfoScanner::FULL_NFO)
    {

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
    if (result == CInfoScanner::URL_NFO || result == CInfoScanner::COMBINED_NFO)
    {
      scrUrl = loader->ScraperUrl();
      pURL = &scrUrl;
    }

    CScraperUrl url;
    int retVal = 0;
    std::string movieTitle = pItem->GetMovieName(bDirNames);
    int movieYear = -1; // hint that movie title was not found
    if (result == CInfoScanner::TITLE_NFO)
    {
      CVideoInfoTag* tag = pItem->GetVideoInfoTag();
      movieTitle = tag->GetTitle();
      movieYear = tag->GetYear(); // movieYear is expected to be >= 0
    }
    if (pURL && pURL->HasUrls())
      url = *pURL;
    else if ((retVal = FindVideo(movieTitle, movieYear, info2, url, pDlgProgress)) <= 0)
      return retVal < 0 ? INFO_CANCELLED : INFO_NOT_FOUND;

    CLog::Log(LOGDEBUG, "VideoInfoScanner: Fetching url '{}' using {} scraper (content: '{}')",
              url.GetFirstThumbUrl(), info2->Name(), TranslateContent(info2->Content()));

    long lResult = -1;
    if (GetDetails(pItem, url, info2,
                   (result == CInfoScanner::COMBINED_NFO ||
                    result == CInfoScanner::OVERRIDE_NFO) ? loader.get() : nullptr,
                   pDlgProgress))
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

  CInfoScanner::INFO_RET
  CVideoInfoScanner::RetrieveInfoForMovie(CFileItem *pItem,
                                          bool bDirNames,
                                          ScraperPtr &info2,
                                          bool useLocal,
                                          CScraperUrl* pURL,
                                          CGUIDialogProgress* pDlgProgress)
  {
    if (pItem->m_bIsFolder || !pItem->IsVideo() || pItem->IsNFO() ||
       (pItem->IsPlayList() && !URIUtils::HasExtension(pItem->GetPath(), ".strm")))
      return INFO_NOT_NEEDED;

    if (ProgressCancelled(pDlgProgress, 198, pItem->GetLabel()))
      return INFO_CANCELLED;

    if (m_database.HasMovieInfo(pItem->GetDynPath()))
      return INFO_HAVE_ALREADY;

    if (m_handle)
      m_handle->SetText(pItem->GetMovieName(bDirNames));

    CInfoScanner::INFO_TYPE result = CInfoScanner::NO_NFO;
    CScraperUrl scrUrl;
    // handle .nfo files
    std::unique_ptr<IVideoInfoTagLoader> loader;
    if (useLocal)
    {
      loader.reset(CVideoInfoTagLoaderFactory::CreateLoader(*pItem, info2, bDirNames));
      if (loader)
      {
        pItem->GetVideoInfoTag()->Reset();
        result = loader->Load(*pItem->GetVideoInfoTag(), false);
      }
    }
    if (result == CInfoScanner::FULL_NFO)
    {
      if (AddVideo(pItem, info2->Content(), bDirNames, true) < 0)
        return INFO_ERROR;
      return INFO_ADDED;
    }
    if (result == CInfoScanner::URL_NFO || result == CInfoScanner::COMBINED_NFO)
    {
      scrUrl = loader->ScraperUrl();
      pURL = &scrUrl;
    }

    CScraperUrl url;
    int retVal = 0;
    std::string movieTitle = pItem->GetMovieName(bDirNames);
    int movieYear = -1; // hint that movie title was not found
    if (result == CInfoScanner::TITLE_NFO)
    {
      CVideoInfoTag* tag = pItem->GetVideoInfoTag();
      movieTitle = tag->GetTitle();
      movieYear = tag->GetYear(); // movieYear is expected to be >= 0
    }
    if (pURL && pURL->HasUrls())
      url = *pURL;
    else if ((retVal = FindVideo(movieTitle, movieYear, info2, url, pDlgProgress)) <= 0)
      return retVal < 0 ? INFO_CANCELLED : INFO_NOT_FOUND;

    CLog::Log(LOGDEBUG, "VideoInfoScanner: Fetching url '{}' using {} scraper (content: '{}')",
              url.GetFirstThumbUrl(), info2->Name(), TranslateContent(info2->Content()));

    if (GetDetails(pItem, url, info2,
                   (result == CInfoScanner::COMBINED_NFO ||
                    result == CInfoScanner::OVERRIDE_NFO) ? loader.get() : nullptr,
                   pDlgProgress))
    {
      if (AddVideo(pItem, info2->Content(), bDirNames, useLocal) < 0)
        return INFO_ERROR;
      return INFO_ADDED;
    }
    //! @todo This is not strictly correct as we could fail to download information here or error, or be cancelled
    return INFO_NOT_FOUND;
  }

  CInfoScanner::INFO_RET
  CVideoInfoScanner::RetrieveInfoForMusicVideo(CFileItem *pItem,
                                               bool bDirNames,
                                               ScraperPtr &info2,
                                               bool useLocal,
                                               CScraperUrl* pURL,
                                               CGUIDialogProgress* pDlgProgress)
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

    CInfoScanner::INFO_TYPE result = CInfoScanner::NO_NFO;
    CScraperUrl scrUrl;
    // handle .nfo files
    std::unique_ptr<IVideoInfoTagLoader> loader;
    if (useLocal)
    {
      loader.reset(CVideoInfoTagLoaderFactory::CreateLoader(*pItem, info2, bDirNames));
      if (loader)
      {
        pItem->GetVideoInfoTag()->Reset();
        result = loader->Load(*pItem->GetVideoInfoTag(), false);
      }
    }
    if (result == CInfoScanner::FULL_NFO)
    {
      if (AddVideo(pItem, info2->Content(), bDirNames, true) < 0)
        return INFO_ERROR;
      return INFO_ADDED;
    }
    if (result == CInfoScanner::URL_NFO || result == CInfoScanner::COMBINED_NFO)
    {
      scrUrl = loader->ScraperUrl();
      pURL = &scrUrl;
    }

    CScraperUrl url;
    int retVal = 0;
    std::string movieTitle = pItem->GetMovieName(bDirNames);
    int movieYear = -1; // hint that movie title was not found
    if (result == CInfoScanner::TITLE_NFO)
    {
      CVideoInfoTag* tag = pItem->GetVideoInfoTag();
      movieTitle = tag->GetTitle();
      movieYear = tag->GetYear(); // movieYear is expected to be >= 0
    }
    if (pURL && pURL->HasUrls())
      url = *pURL;
    else if ((retVal = FindVideo(movieTitle, movieYear, info2, url, pDlgProgress)) <= 0)
      return retVal < 0 ? INFO_CANCELLED : INFO_NOT_FOUND;

    CLog::Log(LOGDEBUG, "VideoInfoScanner: Fetching url '{}' using {} scraper (content: '{}')",
              url.GetFirstThumbUrl(), info2->Name(), TranslateContent(info2->Content()));

    if (GetDetails(pItem, url, info2,
                   (result == CInfoScanner::COMBINED_NFO ||
                    result == CInfoScanner::OVERRIDE_NFO) ? loader.get() : nullptr,
                   pDlgProgress))
    {
      if (AddVideo(pItem, info2->Content(), bDirNames, useLocal) < 0)
        return INFO_ERROR;
      return INFO_ADDED;
    }
    //! @todo This is not strictly correct as we could fail to download information here or error, or be cancelled
    return INFO_NOT_FOUND;
  }

  CInfoScanner::INFO_RET
  CVideoInfoScanner::RetrieveInfoForEpisodes(CFileItem *item,
                                             long showID,
                                             const ADDON::ScraperPtr &scraper,
                                             bool useLocal,
                                             CGUIDialogProgress *progress)
  {
    // enumerate episodes
    EPISODELIST files;
    if (!EnumerateSeriesFolder(item, files))
      return INFO_HAVE_ALREADY;
    if (files.empty()) // no update or no files
      return INFO_NOT_NEEDED;

    if (m_bStop || (progress && progress->IsCanceled()))
      return INFO_CANCELLED;

    CVideoInfoTag showInfo;
    m_database.GetTvShowInfo("", showInfo, showID);
    INFO_RET ret = OnProcessSeriesFolder(files, scraper, useLocal, showInfo, progress);

    if (ret == INFO_ADDED)
    {
      std::map<int, std::map<std::string, std::string>> seasonArt;
      m_database.GetTvShowSeasonArt(showID, seasonArt);

      bool updateSeasonArt = false;
      for (std::map<int, std::map<std::string, std::string>>::const_iterator i = seasonArt.begin(); i != seasonArt.end(); ++i)
      {
        if (i->second.empty())
        {
          updateSeasonArt = true;
          break;
        }
      }

      if (updateSeasonArt)
      {
        if (!item->IsPlugin() || scraper->ID() != "metadata.local")
        {
          CVideoInfoDownloader loader(scraper);
          loader.GetArtwork(showInfo);
        }
        GetSeasonThumbs(showInfo, seasonArt, CVideoThumbLoader::GetArtTypes(MediaTypeSeason), useLocal && !item->IsPlugin());
        for (std::map<int, std::map<std::string, std::string> >::const_iterator i = seasonArt.begin(); i != seasonArt.end(); ++i)
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
    const std::vector<std::string> &regexps = CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_tvshowExcludeFromScanRegExps;

    bool bSkip = false;

    if (item->m_bIsFolder)
    {
      /*
       * Note: DoScan() will not remove this path as it's not recursing for tvshows.
       * Remove this path from the list we're processing in order to avoid hitting
       * it twice in the main loop.
       */
      std::set<std::string>::iterator it = m_pathsToScan.find(item->GetPath());
      if (it != m_pathsToScan.end())
        m_pathsToScan.erase(it);

      std::string hash, dbHash;
      bool allowEmptyHash = false;
      if (item->IsPlugin())
      {
        // if plugin has already calculated a hash for directory contents - use it
        // in this case we don't need to get directory listing from plugin for hash checking
        if (item->HasProperty("hash"))
        {
          hash = item->GetProperty("hash").asString();
          allowEmptyHash = true;
        }
      }
      else if (CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_bVideoLibraryUseFastHash)
        hash = GetRecursiveFastHash(item->GetPath(), regexps);

      if (m_database.GetPathHash(item->GetPath(), dbHash) && (allowEmptyHash || !hash.empty()) && StringUtils::EqualsNoCase(dbHash, hash))
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

        CUtil::GetRecursiveListing(item->GetPath(), items, CServiceBroker::GetFileExtensionProvider().GetVideoExtensions(), flags);

        // fast hash failed - compute slow one
        if (hash.empty())
        {
          GetPathHash(items, hash);
          if (StringUtils::EqualsNoCase(dbHash, hash))
          {
            // slow hashes match - no need to process anything
            bSkip = true;
          }
        }
      }

      if (bSkip)
      {
        CLog::Log(LOGDEBUG, "VideoInfoScanner: Skipping dir '{}' due to no change",
                  CURL::GetRedacted(item->GetPath()));
        // update our dialog with our progress
        if (m_handle)
          OnDirectoryScanned(item->GetPath());
        return false;
      }

      if (dbHash.empty())
        CLog::Log(LOGDEBUG, "VideoInfoScanner: Scanning dir '{}' as not in the database",
                  CURL::GetRedacted(item->GetPath()));
      else
        CLog::Log(LOGDEBUG, "VideoInfoScanner: Rescanning dir '{}' due to change ({} != {})",
                  CURL::GetRedacted(item->GetPath()), dbHash, hash);

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
      //CLog::Log(LOGDEBUG,"{}:{}:{}", x, strPathX, strFileX);

      const int y = x + 1;
      if (StringUtils::EqualsNoCase(strFileX, "VIDEO_TS.IFO"))
      {
        while (y < items.Size())
        {
          std::string strPathY, strFileY;
          URIUtils::Split(items[y]->GetPath(), strPathY, strFileY);
          //CLog::Log(LOGDEBUG," {}:{}:{}", y, strPathY, strFileY);

          if (StringUtils::EqualsNoCase(strPathY, strPathX))
            /*
            remove everything sorted below the video_ts.ifo file in the same path.
            understandably this wont stack correctly if there are other files in the the dvd folder.
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
        CLog::Log(LOGDEBUG, "VideoInfoScanner: Could not enumerate file {}", CURL::GetRedacted(items[i]->GetPath()));
    }
    return true;
  }

  bool CVideoInfoScanner::ProcessItemByVideoInfoTag(const CFileItem *item, EPISODELIST &episodeList)
  {
    if (!item->HasVideoInfoTag())
      return false;

    const CVideoInfoTag* tag = item->GetVideoInfoTag();
    bool isValid = false;
    /*
     * First check the season and episode number. This takes precedence over the original air
     * date and episode title. Must be a valid season and episode number combination.
     */
    if (tag->m_iSeason > -1 && tag->m_iEpisode > 0)
      isValid = true;

    // episode 0 with non-zero season is valid! (e.g. prequel episode)
    if (item->IsPlugin() && tag->m_iSeason > 0 && tag->m_iEpisode >= 0)
      isValid = true;

    if (isValid)
    {
      EPISODE episode;
      episode.strPath = item->GetPath();
      episode.iSeason = tag->m_iSeason;
      episode.iEpisode = tag->m_iEpisode;
      episode.isFolder = false;
      // save full item for plugin source
      if (item->IsPlugin())
        episode.item = std::make_shared<CFileItem>(*item);
      episodeList.push_back(episode);
      CLog::Log(LOGDEBUG, "{} - found match for: {}. Season {}, Episode {}", __FUNCTION__,
                CURL::GetRedacted(episode.strPath), episode.iSeason, episode.iEpisode);
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
      CLog::Log(LOGDEBUG, "{} - found match for: '{}', firstAired: '{}' = '{}', title: '{}'",
                __FUNCTION__, CURL::GetRedacted(episode.strPath),
                tag->m_firstAired.GetAsDBDateTime(), episode.cDate.GetAsLocalizedDate(),
                episode.strTitle);
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
      CLog::Log(LOGDEBUG, "{} - found match for: '{}', title: '{}'", __FUNCTION__,
                CURL::GetRedacted(episode.strPath), episode.strTitle);
      return true;
    }

    /*
     * There is no further episode information available if both the season and episode number have
     * been set to 0. Return the match as true so no further matching is attempted, but don't add it
     * to the episode list.
     */
    if (tag->m_iSeason == 0 && tag->m_iEpisode == 0)
    {
      CLog::Log(LOGDEBUG,
                "{} - found exclusion match for: {}. Both Season and Episode are 0. Item will be "
                "ignored for scanning.",
                __FUNCTION__, CURL::GetRedacted(item->GetPath()));
      return true;
    }

    return false;
  }

  bool CVideoInfoScanner::EnumerateEpisodeItem(const CFileItem *item, EPISODELIST& episodeList)
  {
    SETTINGS_TVSHOWLIST expression = CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_tvshowEnumRegExps;

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
    strLabel = CURL::Decode(CURL::GetRedacted(strLabel));

    for (unsigned int i=0;i<expression.size();++i)
    {
      CRegExp reg(true, CRegExp::autoUtf8);
      if (!reg.RegComp(expression[i].regexp))
        continue;

      int regexppos, regexp2pos;
      //CLog::Log(LOGDEBUG,"running expression {} on {}",expression[i].regexp,strLabel);
      if ((regexppos = reg.RegFind(strLabel.c_str())) < 0)
        continue;

      EPISODE episode;
      episode.strPath = item->GetPath();
      episode.iSeason = -1;
      episode.iEpisode = -1;
      episode.cDate.SetValid(false);
      episode.isFolder = false;

      bool byDate = expression[i].byDate ? true : false;
      bool byTitle = expression[i].byTitle;
      int defaultSeason = expression[i].defaultSeason;

      if (byDate)
      {
        if (!GetAirDateFromRegExp(reg, episode))
          continue;

        CLog::Log(LOGDEBUG, "VideoInfoScanner: Found date based match {} ({}) [{}]",
                  CURL::GetRedacted(episode.strPath), episode.cDate.GetAsLocalizedDate(),
                  expression[i].regexp);
      }
      else if (byTitle)
      {
        if (!GetEpisodeTitleFromRegExp(reg, episode))
          continue;

        CLog::Log(LOGDEBUG, "VideoInfoScanner: Found title based match {} ({}) [{}]",
                  CURL::GetRedacted(episode.strPath), episode.strTitle, expression[i].regexp);
      }
      else
      {
        if (!GetEpisodeAndSeasonFromRegExp(reg, episode, defaultSeason))
          continue;

        CLog::Log(LOGDEBUG, "VideoInfoScanner: Found episode match {} (s{}e{}) [{}]",
                  CURL::GetRedacted(episode.strPath), episode.iSeason, episode.iEpisode,
                  expression[i].regexp);
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
      if (!byDate && reg2.RegComp(CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_tvshowMultiPartEnumRegExp))
      {
        int offset = 0;

        // we want "long circuit" OR below so that both offsets are evaluated
        while (static_cast<int>((regexp2pos = reg2.RegFind(remainder.c_str() + offset)) > -1) |
               static_cast<int>((regexppos = reg.RegFind(remainder.c_str() + offset)) > -1))
        {
          if (((regexppos <= regexp2pos) && regexppos != -1) ||
             (regexppos >= 0 && regexp2pos == -1))
          {
            GetEpisodeAndSeasonFromRegExp(reg, episode, defaultSeason);

            CLog::Log(LOGDEBUG, "VideoInfoScanner: Adding new season {}, multipart episode {} [{}]",
                      episode.iSeason, episode.iEpisode,
                      CServiceBroker::GetSettingsComponent()
                          ->GetAdvancedSettings()
                          ->m_tvshowMultiPartEnumRegExp);

            episodeList.push_back(episode);
            remainder = reg.GetMatch(3);
            offset = 0;
          }
          else if (((regexp2pos < regexppos) && regexp2pos != -1) ||
                   (regexp2pos >= 0 && regexppos == -1))
          {
            episode.iEpisode = atoi(reg2.GetMatch(1).c_str());
            CLog::Log(LOGDEBUG, "VideoInfoScanner: Adding multipart episode {} [{}]",
                      episode.iEpisode,
                      CServiceBroker::GetSettingsComponent()
                          ->GetAdvancedSettings()
                          ->m_tvshowMultiPartEnumRegExp);
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

  bool CVideoInfoScanner::GetEpisodeTitleFromRegExp(CRegExp& reg, EPISODE& episodeInfo)
  {
    std::string param1(reg.GetMatch(1));

    if (!param1.empty())
    {
      episodeInfo.strTitle = param1;
      return true;
    }
    return false;
  }

  long CVideoInfoScanner::AddVideo(CFileItem *pItem, const CONTENT_TYPE &content, bool videoFolder /* = false */, bool useLocal /* = true */, const CVideoInfoTag *showInfo /* = NULL */, bool libraryImport /* = false */)
  {
    // ensure our database is open (this can get called via other classes)
    if (!m_database.Open())
      return -1;

    if (!libraryImport)
      GetArtwork(pItem, content, videoFolder, useLocal && !pItem->IsPlugin(), showInfo ? showInfo->m_strPath : "");

    // ensure the art map isn't completely empty by specifying an empty thumb
    std::map<std::string, std::string> art = pItem->GetArt();
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
      strTitle = StringUtils::Format("{} - {}x{} - {}", showInfo->m_strTitle,
                                     movieDetails.m_iSeason, movieDetails.m_iEpisode, strTitle);
    }

    if (CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
            CSettings::SETTING_MYVIDEOS_EXTRACTFLAGS) &&
        CDVDFileInfo::GetFileStreamDetails(pItem))
      CLog::Log(LOGDEBUG, "VideoInfoScanner: Extracted filestream details from video file {}",
                CURL::GetRedacted(pItem->GetPath()));

    CLog::Log(LOGDEBUG, "VideoInfoScanner: Adding new item to {}:{}", TranslateContent(content), CURL::GetRedacted(pItem->GetPath()));
    long lResult = -1;

    if (content == CONTENT_MOVIES)
    {
      // find local trailer first
      std::string strTrailer = pItem->FindTrailer();
      if (!strTrailer.empty())
        movieDetails.m_strTrailer = strTrailer;

      lResult = m_database.SetDetailsForMovie(movieDetails, art);
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
          CLog::Log(LOGDEBUG, "VideoInfoScanner: Failed to link movie {} to show {}",
                    movieDetails.m_strTitle, movieDetails.m_showLink[i]);
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
        std::vector<std::string> multipath;
        if (!URIUtils::IsMultiPath(pItem->GetPath()) || !CMultiPathDirectory::GetPaths(pItem->GetPath(), multipath))
          multipath.push_back(pItem->GetPath());
        std::vector<std::pair<std::string, std::string> > paths;
        for (std::vector<std::string>::const_iterator i = multipath.begin(); i != multipath.end(); ++i)
          paths.emplace_back(*i, URIUtils::GetParentPath(*i));

        std::map<int, std::map<std::string, std::string> > seasonArt;

        if (!libraryImport)
          GetSeasonThumbs(movieDetails, seasonArt, CVideoThumbLoader::GetArtTypes(MediaTypeSeason), useLocal && !pItem->IsPlugin());

        lResult = m_database.SetDetailsForTvShow(paths, movieDetails, art, seasonArt);
        movieDetails.m_iDbId = lResult;
        movieDetails.m_type = MediaTypeTvShow;
      }
      else
      {
        // we add episode then set details, as otherwise set details will delete the
        // episode then add, which breaks multi-episode files.
        int idShow = showInfo ? showInfo->m_iDbId : -1;
        int idEpisode = m_database.AddNewEpisode(idShow, movieDetails);
        lResult = m_database.SetDetailsForEpisode(movieDetails, art, idShow, idEpisode);
        movieDetails.m_iDbId = lResult;
        movieDetails.m_type = MediaTypeEpisode;
        movieDetails.m_strShowTitle = showInfo ? showInfo->m_strTitle : "";
        if (movieDetails.m_EpBookmark.timeInSeconds > 0)
        {
          movieDetails.m_strFileNameAndPath = pItem->GetPath();
          movieDetails.m_EpBookmark.seasonNumber = movieDetails.m_iSeason;
          movieDetails.m_EpBookmark.episodeNumber = movieDetails.m_iEpisode;
          m_database.AddBookMarkForEpisode(movieDetails, movieDetails.m_EpBookmark);
        }
      }
    }
    else if (content == CONTENT_MUSICVIDEOS)
    {
      lResult = m_database.SetDetailsForMusicVideo(movieDetails, art);
      movieDetails.m_iDbId = lResult;
      movieDetails.m_type = MediaTypeMusicVideo;
    }

    if (!pItem->m_bIsFolder)
    {
      const auto advancedSettings = CServiceBroker::GetSettingsComponent()->GetAdvancedSettings();
      if ((libraryImport || advancedSettings->m_bVideoLibraryImportWatchedState) &&
          (movieDetails.IsPlayCountSet() || movieDetails.m_lastPlayed.IsValid()))
        m_database.SetPlayCount(*pItem, movieDetails.GetPlayCount(), movieDetails.m_lastPlayed);

      if ((libraryImport || advancedSettings->m_bVideoLibraryImportResumePoint) &&
          movieDetails.GetResumePoint().IsSet())
        m_database.AddBookMarkToFile(pItem->GetPath(), movieDetails.GetResumePoint(), CBookmark::RESUME);
    }

    m_database.Close();

    CFileItemPtr itemCopy = std::make_shared<CFileItem>(*pItem);
    CVariant data;
    data["added"] = true;
    if (m_bRunning)
      data["transaction"] = true;
    CServiceBroker::GetAnnouncementManager()->Announce(ANNOUNCEMENT::VideoLibrary, "OnUpdate",
                                                       itemCopy, data);
    return lResult;
  }

  std::string ContentToMediaType(CONTENT_TYPE content, bool folder)
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

  std::string CVideoInfoScanner::GetMovieSetInfoFolder(const std::string& setTitle)
  {
    if (setTitle.empty())
      return "";
    std::string path = CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(
        CSettings::SETTING_VIDEOLIBRARY_MOVIESETSFOLDER);
    if (path.empty())
      return "";
    path = URIUtils::AddFileToFolder(path, CUtil::MakeLegalFileName(setTitle, LEGAL_WIN32_COMPAT));
    URIUtils::AddSlashAtEnd(path);
    CLog::Log(LOGDEBUG,
        "VideoInfoScanner: Looking for local artwork for movie set '{}' in folder '{}'",
        setTitle,
        CURL::GetRedacted(path));
    return CDirectory::Exists(path) ? path : "";
  }

  void CVideoInfoScanner::AddLocalItemArtwork(CGUIListItem::ArtMap& itemArt,
    const std::vector<std::string>& wantedArtTypes, const std::string& itemPath,
    bool addAll, bool exactName)
  {
    std::string path = URIUtils::GetDirectory(itemPath);
    if (path.empty())
      return;

    CFileItemList availableArtFiles;
    CDirectory::GetDirectory(path, availableArtFiles,
        CServiceBroker::GetFileExtensionProvider().GetPictureExtensions(),
        DIR_FLAG_NO_FILE_DIRS | DIR_FLAG_READ_CACHE | DIR_FLAG_NO_FILE_INFO);

    std::string baseFilename = URIUtils::GetFileName(itemPath);
    if (!baseFilename.empty())
    {
      URIUtils::RemoveExtension(baseFilename);
      baseFilename.append("-");
    }

    for (const auto& artFile : availableArtFiles)
    {
      std::string candidate = URIUtils::GetFileName(artFile->GetPath());

      bool matchesFilename =
        !baseFilename.empty() && StringUtils::StartsWith(candidate, baseFilename);
      if (!baseFilename.empty() && !matchesFilename)
        continue;

      if (matchesFilename)
        candidate.erase(0, baseFilename.length());
      URIUtils::RemoveExtension(candidate);
      StringUtils::ToLower(candidate);

      // move 'folder' to thumb / poster / banner based on aspect ratio
      // if such artwork doesn't already exist
      if (!matchesFilename && StringUtils::EqualsNoCase(candidate, "folder") &&
        !CVideoThumbLoader::IsArtTypeInWhitelist("folder", wantedArtTypes, exactName))
      {
        // cache the image to determine sizing
        CTextureDetails details;
        if (CServiceBroker::GetTextureCache()->CacheImage(artFile->GetPath(), details))
        {
          candidate = GetArtTypeFromSize(details.width, details.height);
          if (itemArt.find(candidate) != itemArt.end())
            continue;
        }
      }

      if ((addAll && CVideoThumbLoader::IsValidArtType(candidate)) ||
        CVideoThumbLoader::IsArtTypeInWhitelist(candidate, wantedArtTypes, exactName))
      {
        itemArt[candidate] = artFile->GetPath();
      }
    }
  }

  void CVideoInfoScanner::GetArtwork(CFileItem *pItem, const CONTENT_TYPE &content, bool bApplyToDir, bool useLocal, const std::string &actorArtPath)
  {
    int artLevel = CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(
        CSettings::SETTING_VIDEOLIBRARY_ARTWORK_LEVEL);
    if (artLevel == CSettings::VIDEOLIBRARY_ARTWORK_LEVEL_NONE)
      return;

    CVideoInfoTag &movieDetails = *pItem->GetVideoInfoTag();
    movieDetails.m_fanart.Unpack();
    movieDetails.m_strPictureURL.Parse();

    CGUIListItem::ArtMap art = pItem->GetArt();

    // get and cache thumb images
    std::string mediaType = ContentToMediaType(content, pItem->m_bIsFolder);
    std::vector<std::string> artTypes = CVideoThumbLoader::GetArtTypes(mediaType);
    bool moviePartOfSet = content == CONTENT_MOVIES && !movieDetails.m_set.title.empty();
    std::vector<std::string> movieSetArtTypes;
    if (moviePartOfSet)
    {
      movieSetArtTypes = CVideoThumbLoader::GetArtTypes(MediaTypeVideoCollection);
      for (const std::string& artType : movieSetArtTypes)
        artTypes.push_back("set." + artType);
    }
    bool addAll = artLevel == CSettings::VIDEOLIBRARY_ARTWORK_LEVEL_ALL;
    bool exactName = artLevel == CSettings::VIDEOLIBRARY_ARTWORK_LEVEL_BASIC;
    // find local art
    if (useLocal)
    {
      if (!pItem->SkipLocalArt())
      {
        if (bApplyToDir && (content == CONTENT_MOVIES || content == CONTENT_MUSICVIDEOS))
        {
          std::string filename = pItem->GetLocalArtBaseFilename();
          std::string directory = URIUtils::GetDirectory(filename);
          if (filename != directory)
            AddLocalItemArtwork(art, artTypes, directory, addAll, exactName);
        }
        AddLocalItemArtwork(art, artTypes, pItem->GetLocalArtBaseFilename(), addAll, exactName);
      }

      if (moviePartOfSet)
      {
        std::string movieSetInfoPath = GetMovieSetInfoFolder(movieDetails.m_set.title);
        if (!movieSetInfoPath.empty())
        {
          CGUIListItem::ArtMap movieSetArt;
          AddLocalItemArtwork(movieSetArt, movieSetArtTypes, movieSetInfoPath, addAll, exactName);
          for (const auto& artItem : movieSetArt)
          {
            art["set." + artItem.first] = artItem.second;
          }
        }
      }
    }

    // find embedded art
    if (pItem->HasVideoInfoTag() && !pItem->GetVideoInfoTag()->m_coverArt.empty())
    {
      for (auto& it : pItem->GetVideoInfoTag()->m_coverArt)
      {
        if ((addAll || CVideoThumbLoader::IsArtTypeInWhitelist(it.m_type, artTypes, exactName)) &&
          art.find(it.m_type) == art.end())
        {
          std::string thumb = CTextureUtils::GetWrappedImageURL(pItem->GetPath(),
                                                                "video_" + it.m_type);
          art.insert(std::make_pair(it.m_type, thumb));
        }
      }
    }

    // add online fanart (treated separately due to it being stored in m_fanart)
    if ((addAll || CVideoThumbLoader::IsArtTypeInWhitelist("fanart", artTypes, exactName)) &&
      art.find("fanart") == art.end())
    {
      std::string fanart = pItem->GetVideoInfoTag()->m_fanart.GetImageURL();
      if (!fanart.empty())
        art.insert(std::make_pair("fanart", fanart));
    }

    // add online art
    for (const auto& url : pItem->GetVideoInfoTag()->m_strPictureURL.GetUrls())
    {
      if (url.m_type != CScraperUrl::UrlType::General)
        continue;
      std::string aspect = url.m_aspect;
      if (aspect.empty())
        // Backward compatibility with Kodi 11 Eden NFO files
        aspect = mediaType == MediaTypeEpisode ? "thumb" : "poster";

      if ((addAll || CVideoThumbLoader::IsArtTypeInWhitelist(aspect, artTypes, exactName)) &&
        art.find(aspect) == art.end())
      {
        std::string image = GetImage(url, pItem->GetPath());
        if (!image.empty())
          art.insert(std::make_pair(aspect, image));
      }
    }

    if (art.find("thumb") == art.end() &&
        CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
            CSettings::SETTING_MYVIDEOS_EXTRACTTHUMB) &&
        CDVDFileInfo::CanExtract(*pItem))
    {
      art["thumb"] = CVideoThumbLoader::GetEmbeddedThumbURL(*pItem);
    }

    for (const auto& artType : artTypes)
    {
      if (art.find(artType) != art.end())
        CServiceBroker::GetTextureCache()->BackgroundCacheImage(art[artType]);
    }

    pItem->SetArt(art);

    // parent folder to apply the thumb to and to search for local actor thumbs
    std::string parentDir = URIUtils::GetBasePath(pItem->GetPath());
    if (CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_VIDEOLIBRARY_ACTORTHUMBS))
      FetchActorThumbs(movieDetails.m_cast, actorArtPath.empty() ? parentDir : actorArtPath);
    if (bApplyToDir)
      ApplyThumbToFolder(parentDir, art["thumb"]);
  }

  std::string CVideoInfoScanner::GetImage(const CScraperUrl::SUrlEntry &image, const std::string& itemPath)
  {
    std::string thumb = CScraperUrl::GetThumbUrl(image);
    if (!thumb.empty() && thumb.find('/') == std::string::npos &&
        thumb.find('\\') == std::string::npos)
    {
      std::string strPath = URIUtils::GetDirectory(itemPath);
      thumb = URIUtils::AddFileToFolder(strPath, thumb);
    }
    return thumb;
  }

  CInfoScanner::INFO_RET
  CVideoInfoScanner::OnProcessSeriesFolder(EPISODELIST& files,
                                           const ADDON::ScraperPtr &scraper,
                                           bool useLocal,
                                           const CVideoInfoTag& showInfo,
                                           CGUIDialogProgress* pDlgProgress /* = NULL */)
  {
    if (pDlgProgress)
    {
      pDlgProgress->SetLine(1, CVariant{20361}); // Loading episode details
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
      if (pDlgProgress)
      {
        pDlgProgress->SetLine(1, CVariant{20361}); // Loading episode details
        pDlgProgress->SetLine(2, StringUtils::Format("{} {}", g_localizeStrings.Get(20373),
                                                     file->iSeason)); // Season x
        pDlgProgress->SetLine(3, StringUtils::Format("{} {}", g_localizeStrings.Get(20359),
                                                     file->iEpisode)); // Episode y
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
      if (file->item)
        item = *file->item;
      else
      {
        item.SetPath(file->strPath);
        item.GetVideoInfoTag()->m_iEpisode = file->iEpisode;
      }

      // handle .nfo files
      CInfoScanner::INFO_TYPE result=CInfoScanner::NO_NFO;
      CScraperUrl scrUrl;
      const ScraperPtr& info(scraper);
      std::unique_ptr<IVideoInfoTagLoader> loader;
      if (useLocal)
      {
        loader.reset(CVideoInfoTagLoaderFactory::CreateLoader(item, info, false));
        if (loader)
        {
          // no reset here on purpose
          result = loader->Load(*item.GetVideoInfoTag(), false);
        }
      }
      if (result == CInfoScanner::FULL_NFO)
      {
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
          url.ParseAndAppendUrlsFromEpisodeGuide(showInfo.m_strEpisodeGuide);

          if (pDlgProgress)
          {
            pDlgProgress->SetLine(1, CVariant{20354}); // Fetching episode guide
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
        CLog::Log(LOGERROR,
                  "VideoInfoScanner: Asked to lookup episode {}"
                  " online, but we have no episode guide. Check your tvshow.nfo and make"
                  " sure the <episodeguide> tag is in place.",
                  CURL::GetRedacted(file->strPath));
        continue;
      }

      EPISODE key(file->iSeason, file->iEpisode, file->iSubepisode);
      EPISODE backupkey(file->iSeason, file->iEpisode, 0);
      bool bFound = false;
      EPISODELIST::iterator guide = episodes.begin();
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
        if (!guide->cScraperUrl.GetTitle().empty() &&
            StringUtils::EqualsNoCase(guide->cScraperUrl.GetTitle(), file->strTitle))
        {
          bFound = true;
          break;
        }
        if (!guide->strTitle.empty() && StringUtils::EqualsNoCase(guide->strTitle, file->strTitle))
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
          CLog::Log(LOGDEBUG, "VideoInfoScanner: analyzing parsed title '{}'", file->strTitle);
          double minscore = 0; // Default minimum score is 0 to find whatever is the best match.

          EPISODELIST *candidates;
          if (matches.empty()) // No matches found using earlier criteria. Use fuzzy match on titles across all episodes.
          {
            minscore = 0.8; // 80% should ensure a good match.
            candidates = &episodes;
          }
          else // Multiple matches found. Use fuzzy match on the title with already matched episodes to pick the best.
            candidates = &matches;

          std::vector<std::string> titles;
          for (guide = candidates->begin(); guide != candidates->end(); ++guide)
          {
            auto title = guide->cScraperUrl.GetTitle();
            if (title.empty())
            {
              title = guide->strTitle;
            }
            StringUtils::ToLower(title);
            guide->cScraperUrl.SetTitle(title);
            titles.push_back(title);
          }

          double matchscore;
          std::string loweredTitle(file->strTitle);
          StringUtils::ToLower(loweredTitle);
          int index = StringUtils::FindBestMatch(loweredTitle, titles, matchscore);
          if (index >= 0 && matchscore >= minscore)
          {
            guide = candidates->begin() + index;
            bFound = true;
            CLog::Log(LOGDEBUG,
                      "{} fuzzy title match for show: '{}', title: '{}', match: '{}', score: {:f} "
                      ">= {:f}",
                      __FUNCTION__, showInfo.m_strTitle, file->strTitle, titles[index], matchscore,
                      minscore);
          }
        }
      }

      if (bFound)
      {
        CVideoInfoDownloader imdb(scraper);
        CFileItem item;
        item.SetPath(file->strPath);
        if (!imdb.GetEpisodeDetails(guide->cScraperUrl, *item.GetVideoInfoTag(), pDlgProgress))
          return INFO_NOT_FOUND; //! @todo should we just skip to the next episode?

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
        CLog::Log(
            LOGDEBUG,
            "{} - no match for show: '{}', season: {}, episode: {}.{}, airdate: '{}', title: '{}'",
            __FUNCTION__, showInfo.m_strTitle, file->iSeason, file->iEpisode, file->iSubepisode,
            file->cDate.GetAsLocalizedDate(), file->strTitle);
      }
    }
    return INFO_ADDED;
  }

  bool CVideoInfoScanner::GetDetails(CFileItem *pItem, CScraperUrl &url,
                                     const ScraperPtr& scraper,
                                     IVideoInfoTagLoader* loader,
                                     CGUIDialogProgress* pDialog /* = NULL */)
  {
    CVideoInfoTag movieDetails;

    if (m_handle && !url.GetTitle().empty())
      m_handle->SetText(url.GetTitle());

    CVideoInfoDownloader imdb(scraper);
    bool ret = imdb.GetDetails(url, movieDetails, pDialog);

    if (ret)
    {
      if (loader)
        loader->Load(movieDetails, true);

      if (m_handle && url.GetTitle().empty())
        m_handle->SetText(movieDetails.m_strTitle);

      if (pDialog)
      {
        if (!pDialog->HasText())
          pDialog->SetLine(0, CVariant{movieDetails.m_strTitle});
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
    CDigest digest{CDigest::Type::MD5};
    int count = 0;
    for (int i = 0; i < items.Size(); ++i)
    {
      const CFileItemPtr pItem = items[i];
      digest.Update(pItem->GetPath());
      if (pItem->IsPlugin())
      {
        // allow plugin to calculate hash itself using strings rather than binary data for size and date
        // according to ListItem.setInfo() documentation date format should be "d.m.Y"
        if (pItem->m_dwSize)
          digest.Update(std::to_string(pItem->m_dwSize));
        if (pItem->m_dateTime.IsValid())
          digest.Update(StringUtils::Format("{:02}.{:02}.{:04}", pItem->m_dateTime.GetDay(),
                                            pItem->m_dateTime.GetMonth(),
                                            pItem->m_dateTime.GetYear()));
      }
      else
      {
        digest.Update(&pItem->m_dwSize, sizeof(pItem->m_dwSize));
        KODI::TIME::FileTime time = pItem->m_dateTime;
        digest.Update(&time, sizeof(KODI::TIME::FileTime));
      }
      if (pItem->IsVideo() && !pItem->IsPlayList() && !pItem->IsNFO())
        count++;
    }
    hash = digest.Finalize();
    return count;
  }

  bool CVideoInfoScanner::CanFastHash(const CFileItemList &items, const std::vector<std::string> &excludes) const
  {
    if (!CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_bVideoLibraryUseFastHash || items.IsPlugin())
      return false;

    for (int i = 0; i < items.Size(); ++i)
    {
      if (items[i]->m_bIsFolder && !CUtil::ExcludeFileOrFolder(items[i]->GetPath(), excludes))
        return false;
    }
    return true;
  }

  std::string CVideoInfoScanner::GetFastHash(const std::string &directory,
      const std::vector<std::string> &excludes) const
  {
    CDigest digest{CDigest::Type::MD5};

    if (excludes.size())
      digest.Update(StringUtils::Join(excludes, "|"));

    struct __stat64 buffer;
    if (XFILE::CFile::Stat(directory, &buffer) == 0)
    {
      int64_t time = buffer.st_mtime;
      if (!time)
        time = buffer.st_ctime;
      if (time)
      {
        digest.Update((unsigned char *)&time, sizeof(time));
        return digest.Finalize();
      }
    }
    return "";
  }

  std::string CVideoInfoScanner::GetRecursiveFastHash(const std::string &directory,
      const std::vector<std::string> &excludes) const
  {
    CFileItemList items;
    items.Add(std::make_shared<CFileItem>(directory, true));
    CUtil::GetRecursiveDirsListing(directory, items, DIR_FLAG_NO_FILE_DIRS | DIR_FLAG_NO_FILE_INFO);

    CDigest digest{CDigest::Type::MD5};

    if (excludes.size())
      digest.Update(StringUtils::Join(excludes, "|"));

    int64_t time = 0;
    for (int i=0; i < items.Size(); ++i)
    {
      int64_t stat_time = 0;
      struct __stat64 buffer;
      if (XFILE::CFile::Stat(items[i]->GetPath(), &buffer) == 0)
      {
        //! @todo some filesystems may return the mtime/ctime inline, in which case this is
        //! unnecessarily expensive. Consider supporting Stat() in our directory cache?
        stat_time = buffer.st_mtime ? buffer.st_mtime : buffer.st_ctime;
        time += stat_time;
      }

      if (!stat_time)
        return "";
    }

    if (time)
    {
      digest.Update((unsigned char *)&time, sizeof(time));
      return digest.Finalize();
    }
    return "";
  }

  void CVideoInfoScanner::GetSeasonThumbs(const CVideoInfoTag &show,
      std::map<int, std::map<std::string, std::string>> &seasonArt, const std::vector<std::string> &artTypes, bool useLocal)
  {
    int artLevel = CServiceBroker::GetSettingsComponent()->GetSettings()->
      GetInt(CSettings::SETTING_VIDEOLIBRARY_ARTWORK_LEVEL);
    bool addAll = artLevel == CSettings::VIDEOLIBRARY_ARTWORK_LEVEL_ALL;
    bool exactName = artLevel == CSettings::VIDEOLIBRARY_ARTWORK_LEVEL_BASIC;
    if (useLocal)
    {
      // find the maximum number of seasons we have local thumbs for
      int maxSeasons = 0;
      CFileItemList items;
      std::string extensions = CServiceBroker::GetFileExtensionProvider().GetPictureExtensions();
      if (!show.m_strPath.empty())
      {
        CDirectory::GetDirectory(show.m_strPath, items, extensions,
                                 DIR_FLAG_NO_FILE_DIRS | DIR_FLAG_READ_CACHE |
                                     DIR_FLAG_NO_FILE_INFO);
      }
      extensions.erase(std::remove(extensions.begin(), extensions.end(), '.'), extensions.end());
      CRegExp reg;
      if (items.Size() && reg.RegComp("season([0-9]+)(-[a-z0-9]+)?\\.(" + extensions + ")"))
      {
        for (const auto& item : items)
        {
          std::string name = URIUtils::GetFileName(item->GetPath());
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
        std::map<int, std::map<std::string, std::string>>::const_iterator it = seasonArt.find(season);
        if (it != seasonArt.end() && !it->second.empty())
          continue;

        std::map<std::string, std::string> art;
        std::string basePath;
        if (season == -1)
          basePath = "season-all";
        else if (season == 0)
          basePath = "season-specials";
        else
          basePath = StringUtils::Format("season{:02}", season);

        AddLocalItemArtwork(art, artTypes,
          URIUtils::AddFileToFolder(show.m_strPath, basePath),
          addAll, exactName);

        seasonArt[season] = art;
      }
    }
    // add online art
    for (const auto& url : show.m_strPictureURL.GetUrls())
    {
      if (url.m_type != CScraperUrl::UrlType::Season)
        continue;
      std::string aspect = url.m_aspect;
      if (aspect.empty())
        aspect = "thumb";
      std::map<std::string, std::string>& art = seasonArt[url.m_season];
      if ((addAll || CVideoThumbLoader::IsArtTypeInWhitelist(aspect, artTypes, exactName)) &&
        art.find(aspect) == art.end())
      {
        std::string image = CScraperUrl::GetThumbUrl(url);
        if (!image.empty())
          art.insert(std::make_pair(aspect, image));
      }
    }
  }

  void CVideoInfoScanner::FetchActorThumbs(std::vector<SActorInfo>& actors, const std::string& strPath)
  {
    CFileItemList items;
    // don't try to fetch anything local with plugin source
    if (!URIUtils::IsPlugin(strPath))
    {
      std::string actorsDir = URIUtils::AddFileToFolder(strPath, ".actors");
      if (CDirectory::Exists(actorsDir))
        CDirectory::GetDirectory(actorsDir, items, ".png|.jpg|.tbn", DIR_FLAG_NO_FILE_DIRS |
                                 DIR_FLAG_NO_FILE_INFO);
    }
    for (std::vector<SActorInfo>::iterator i = actors.begin(); i != actors.end(); ++i)
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
        if (i->thumb.empty() && !i->thumbUrl.GetFirstUrlByType().m_url.empty())
          i->thumb = CScraperUrl::GetThumbUrl(i->thumbUrl.GetFirstUrlByType());
        if (!i->thumb.empty())
          CServiceBroker::GetTextureCache()->BackgroundCacheImage(i->thumb);
      }
    }
  }

  bool CVideoInfoScanner::DownloadFailed(CGUIDialogProgress* pDialog)
  {
    if (CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_bVideoScannerIgnoreErrors)
      return true;

    if (pDialog)
    {
      HELPERS::ShowOKDialogText(CVariant{20448}, CVariant{20449});
      return false;
    }
    return HELPERS::ShowYesNoDialogText(CVariant{20448}, CVariant{20450}) ==
           DialogResponse::CHOICE_YES;
  }

  bool CVideoInfoScanner::ProgressCancelled(CGUIDialogProgress* progress, int heading, const std::string &line1)
  {
    if (progress)
    {
      progress->SetHeading(CVariant{heading});
      progress->SetLine(0, CVariant{line1});
      progress->Progress();
      return progress->IsCanceled();
    }
    return m_bStop;
  }

  int CVideoInfoScanner::FindVideo(const std::string &title, int year, const ScraperPtr &scraper, CScraperUrl &url, CGUIDialogProgress *progress)
  {
    MOVIELIST movielist;
    CVideoInfoDownloader imdb(scraper);
    int returncode = imdb.FindMovie(title, year, movielist, progress);
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
