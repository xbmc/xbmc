/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "VideoInfoScanner.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "GUIInfoManager.h"
#include "GUIUserMessages.h"
#include "ServiceBroker.h"
#include "SetInfoTag.h"
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
#include "filesystem/File.h"
#include "filesystem/MultiPathDirectory.h"
#include "filesystem/PluginDirectory.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "imagefiles/ImageFileURL.h"
#include "interfaces/AnnouncementManager.h"
#include "messaging/helpers/DialogHelper.h"
#include "messaging/helpers/DialogOKHelper.h"
#include "playlists/PlayListFileItemClassify.h"
#include "resources/LocalizeStrings.h"
#include "resources/ResourcesComponent.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "tags/SetInfoTagLoaderFactory.h"
#include "tags/VideoInfoTagLoaderFactory.h"
#include "utils/ArtUtils.h"
#include "utils/Digest.h"
#include "utils/FileExtensionProvider.h"
#include "utils/RegExp.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "utils/log.h"
#include "video/VideoFileItemClassify.h"
#include "video/VideoInfoTag.h"
#include "video/VideoManagerTypes.h"
#include "video/VideoThumbLoader.h"
#include "video/VideoUtils.h"
#include "video/dialogs/GUIDialogVideoManagerExtras.h"
#include "video/dialogs/GUIDialogVideoManagerVersions.h"

#include <algorithm>
#include <memory>
#include <ranges>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>

using namespace XFILE;
using namespace ADDON;
using namespace KODI::MESSAGING;
using namespace KODI;

using KODI::MESSAGING::HELPERS::DialogResponse;
using KODI::UTILITY::CDigest;

namespace
{

/**
 * \brief Maximum allowed number of episodes in a single episode range.
 *
 * The value 50 was chosen as a safety limit to prevent accidental processing of
 * extremely large episode ranges, which could be caused by malformed filenames or
 * incorrect regular expression matches. Note this is a per-file number and not the
 * total number of episodes in a season.
 */
constexpr int MAX_EPISODE_RANGE = 50;

// Character following season/episode range must be one of these for range to be valid.
constexpr std::string_view allowed{"-_.esx "};

/*! \brief Perform checks, then add episodes in a given range to the episode list
 \param first first episode in the range to add.
 \param last last episode in the range.
 \param episode the first episode in the range (already added to the list).
 \param episodeList the list (vector) of episodes to add to.
 \param regex the regex used to match the episode range.
 \param remainder the remainder of the filename after the episode range.
*/
void ProcessEpisodeRange(int first,
                         int last,
                         VIDEO::EPISODE& episode,
                         VIDEO::EPISODELIST& episodeList,
                         const std::string& regex,
                         const std::string& remainder)
{
  if (first > last && !episodeList.empty())
  {
    // SxxEaa-SxxEbb or Eaa-Ebb is backwards - bb<aa
    CLog::LogF(LOGDEBUG,
               "VideoInfoScanner: Removing season {}, episode {} as range {}-{} is backwards",
               episode.iSeason, episode.iEpisode, episodeList.back().iEpisode, last);
    episodeList.pop_back();
    return;
  }
  if ((last - first + 1) > MAX_EPISODE_RANGE && !episodeList.empty())
  {
    CLog::LogF(LOGDEBUG,
               "VideoInfoScanner: Removing season {}, episode {} as range is too large {} (maximum "
               "allowed {})",
               episode.iSeason, episode.iEpisode, last - first + 1, MAX_EPISODE_RANGE);
    episodeList.pop_back();
    return;
  }
  if (!remainder.empty() && allowed.find(static_cast<char>(std::tolower(static_cast<unsigned char>(
                                remainder.front())))) == std::string_view::npos)
  {
    CLog::LogF(LOGDEBUG,
               "VideoInfoScanner:Last episode in range {} is not part of an episode string ({}) "
               "- ignoring",
               last, remainder);
    return;
  }
  for (int e = first; e <= last; ++e)
  {
    episode.iEpisode = e;
    CLog::LogF(LOGDEBUG, "VideoInfoScanner: Adding multipart episode {} [{}]", episode.iEpisode,
               regex);
    episodeList.push_back(episode);
  }
}

/*! \brief Retrieve the art type for an image from the given size.
 \param width the width of the image.
 \param height the height of the image.
 \return "poster" if the aspect ratio is at most 4:5, "banner" if the aspect ratio
         is at least 1:4, "thumb" otherwise.
 */
std::string GetArtTypeFromSize(unsigned int width, unsigned int height)
{
  std::string type = "thumb";
  if (width * 5 < height * 4)
    type = "poster";
  else if (width > height * 4)
    type = "banner";
  return type;
}

void AddLocalItemArtwork(KODI::ART::Artwork& itemArt,
                         const std::vector<std::string>& wantedArtTypes,
                         const std::string& itemPath,
                         bool addAll,
                         bool exactName,
                         bool isInFolder)
{
  std::string path = URIUtils::GetDirectory(itemPath);
  if (path.empty())
    return;

  CFileItemList availableArtFiles;
  CDirectory::GetDirectory(path, availableArtFiles,
                           CServiceBroker::GetFileExtensionProvider().GetPictureExtensions(),
                           DIR_FLAG_NO_FILE_DIRS | DIR_FLAG_READ_CACHE | DIR_FLAG_NO_FILE_INFO);

  std::string baseFilename{URIUtils::GetFileName(itemPath)};
  if (!baseFilename.empty())
  {
    URIUtils::RemoveExtension(baseFilename);
    baseFilename.append("-");
  }

  const bool caseSensitive{
      CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_caseSensitiveLocalArtMatch};

  for (const auto& artFile : availableArtFiles)
  {
    std::string candidate{URIUtils::GetFileName(artFile->GetPath())};

    bool matchesFilename{!baseFilename.empty() &&
                         (caseSensitive ? StringUtils::StartsWith(candidate, baseFilename)
                                        : StringUtils::StartsWithNoCase(candidate, baseFilename))};

    if (!baseFilename.empty() && !matchesFilename && !isInFolder)
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
        if (itemArt.contains(candidate))
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

void OnDirectoryScanned(const std::string& strDirectory)
{
  CGUIMessage msg(GUI_MSG_DIRECTORY_SCANNED, 0, 0, 0);
  msg.SetStringParam(strDirectory);
  CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg);
}

} // namespace

namespace KODI::VIDEO
{

CVideoInfoScanner::CVideoInfoScanner()
  : m_advancedSettings(CServiceBroker::GetSettingsComponent()->GetAdvancedSettings())
{
  m_bStop = false;
  m_scanAll = false;

  const auto settings = CServiceBroker::GetSettingsComponent()->GetSettings();

  m_ignoreVideoVersions = settings->GetBool(CSettings::SETTING_VIDEOLIBRARY_IGNOREVIDEOVERSIONS);
  m_ignoreVideoExtras = settings->GetBool(CSettings::SETTING_VIDEOLIBRARY_IGNOREVIDEOEXTRAS);
}

CVideoInfoScanner::~CVideoInfoScanner()
{
  // Clear cache for all used scrapers
  for (auto& [_, scraper] : m_scraperCache)
    scraper->ClearCache();
}

  void CVideoInfoScanner::Process()
  {
    m_bStop = false;

    try
    {
      const auto settings = CServiceBroker::GetSettingsComponent()->GetSettings();

      if (m_showDialog && !settings->GetBool(CSettings::SETTING_VIDEOLIBRARY_BACKGROUNDUPDATE))
      {
        CGUIDialogExtendedProgressBar* dialog =
          CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogExtendedProgressBar>(WINDOW_DIALOG_EXT_PROGRESS);
        if (dialog)
          m_handle = dialog->GetHandle(
              CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(314));
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
            m_handle->SetTitle(
                CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(331));
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
    m_bClean = m_advancedSettings->m_bVideoLibraryCleanOnUpdate;

    m_bRunning = true;
    Process();
  }

  void CVideoInfoScanner::Stop()
  {
    if (m_bCanInterrupt)
      m_database.Interrupt();

    m_bStop = true;
  }

  bool CVideoInfoScanner::DoScan(const std::string& strDirectory)
  {
    if (m_handle)
    {
      m_handle->SetText(CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(20415));
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
    ScraperPtr info =
        m_database.GetScraperForPath(strDirectory, settings, foundDirectly, &m_scraperCache);
    ContentType content = info ? info->Content() : ContentType::NONE;

    // exclude folders that match our exclude regexps
    const std::vector<std::string>& regexps =
        content == ContentType::TVSHOWS ? m_advancedSettings->m_tvshowExcludeFromScanRegExps
                                        : m_advancedSettings->m_moviesExcludeFromScanRegExps;

    if (CUtil::ExcludeFileOrFolder(strDirectory, regexps))
      return true;

    if (HasNoMedia(strDirectory))
      return true;

    bool ignoreFolder = !m_scanAll && settings.noupdate;
    if (content == ContentType::NONE || ignoreFolder)
      return true;

    if (URIUtils::IsPlugin(strDirectory) && !CPluginDirectory::IsMediaLibraryScanningAllowed(TranslateContent(content), strDirectory))
    {
      CLog::Log(
          LOGINFO,
          "VideoInfoScanner: Plugin '{}' does not support media library scanning for '{}' content",
          CURL::GetRedacted(strDirectory), content);
      return true;
    }

    std::string hash, dbHash;
    if (content == ContentType::MOVIES || content == ContentType::MUSICVIDEOS)
    {
      if (m_handle)
      {
        int str = content == ContentType::MOVIES ? 20317 : 20318;
        m_handle->SetTitle(StringUtils::Format(
            CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(str), info->Name()));
      }

      std::string fastHash;
      if (m_advancedSettings->m_bVideoLibraryUseFastHash && !URIUtils::IsPlugin(strDirectory))
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
        items.erase(std::remove_if(items.begin(), items.end(), [](const CFileItemPtr& item)
                                   { return item->IsFolder() && HasNoMedia(item->GetPath()); }),
                    items.end());
        items.Stack();

        // force sorting consistency to avoid hash mismatch between platforms
        // sort by filename as always present for any files, but keep case sensitivity
        items.Sort(SortByFile, SortOrder::ASCENDING, SortAttributeNone);

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
    else if (content == ContentType::TVSHOWS)
    {
      if (m_handle)
        m_handle->SetTitle(StringUtils::Format(
            CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(20319), info->Name()));

      if (foundDirectly && !settings.parent_name_root)
      {
        CDirectory::GetDirectory(strDirectory, items, CServiceBroker::GetFileExtensionProvider().GetVideoExtensions(),
                                 DIR_FLAG_DEFAULTS);
        items.SetPath(strDirectory);

        // force sorting consistency to avoid hash mismatch between platforms
        // sort by filename as always present for any files, but keep case sensitivity
        items.Sort(SortByFile, SortOrder::ASCENDING, SortAttributeNone);

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
        item->SetFolder(true);
        items.Add(item);
        items.SetPath(URIUtils::GetParentPath(item->GetPath()));
      }
    }
    bool foundSomething = false;
    if (!bSkip)
    {
      foundSomething = RetrieveVideoInfo(items, settings.parent_name_root, content);
      if (foundSomething)
      {
        if (!m_bStop && (content == ContentType::MOVIES || content == ContentType::MUSICVIDEOS))
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
    else if (!StringUtils::EqualsNoCase(hash, dbHash) &&
             (content == ContentType::MOVIES || content == ContentType::MUSICVIDEOS))
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

      // add video extras to library
      if (foundSomething && content == ContentType::MOVIES && settings.parent_name &&
          !m_ignoreVideoExtras && IsVideoExtrasFolder(*pItem))
      {
        if (AddVideoExtras(items, content, pItem->GetPath()))
        {
          CLog::Log(LOGDEBUG, "VideoInfoScanner: Finished adding video extras from dir {}",
                    CURL::GetRedacted(pItem->GetPath()));
        }

        // no further processing required
        continue;
      }

      // if we have a directory item (non-playlist) we then recurse into that folder
      // do not recurse for tv shows - we have already looked recursively for episodes
      if (content != ContentType::TVSHOWS && settings.recurse > 0 && pItem->IsFolder() &&
          !pItem->IsParentFolder() && !PLAYLIST::IsPlayList(*pItem))
      {
        if (!DoScan(pItem->GetPath()))
        {
          m_bStop = true;
        }
      }
    }
    return !m_bStop;
  }

  bool CVideoInfoScanner::UpdateSetInTag(CVideoInfoTag& tag)
  {
    // Uses the set information in m_set from a tag of a movie
    // If there is a set, see if the details need to be updated from the
    // Movie Set Information Folder
    if (!tag.m_set.HasTitle())
      return false;

    bool setUpdated{false};
    ART::Artwork movieSetArt;
    const std::string movieSetInfoPath{GetMovieSetInfoFolder(tag.m_set.GetTitle())};
    if (!movieSetInfoPath.empty())
    {
      // Look for set.nfo
      if (const std::unique_ptr setLoader{
              CSetInfoTagLoaderFactory::CreateLoader(tag.m_set.GetTitle())};
          setLoader)
      {
        CSetInfoTag setTag;
        InfoType result{setLoader->Load(setTag, false)};
        if (result != InfoType::NONE && !setTag.IsEmpty())
        {
          tag.m_set.SetOriginalTitle(tag.m_set.GetTitle());
          if (!setTag.GetTitle().empty())
            tag.m_set.SetTitle(setTag.GetTitle());
          if (!setTag.GetOverview().empty())
          {
            tag.m_set.SetOverview(setTag.GetOverview());
            tag.SetUpdateSetOverview(true);
          }
          if (setTag.HasArt())
            tag.m_set.SetArt(setTag.GetArt());
          setUpdated = true;
        }
      }

      // Now look for art
      // Look for local art files first
      const std::vector<std::string> movieSetArtTypes =
          CVideoThumbLoader::GetArtTypes(MediaTypeVideoCollection);
      AddLocalItemArtwork(movieSetArt, movieSetArtTypes, movieSetInfoPath, true, false, true);

      // If art specified in set.nfo use that next
      if (movieSetArt.empty() && tag.m_set.HasArt())
        for (auto& art : tag.m_set.GetArt())
          if (CVideoThumbLoader::IsArtTypeInWhitelist(art.first, movieSetArtTypes, false))
            movieSetArt.insert(art);
    }
    else
    {
      // No MSIF, so use the set art from the scraper (already in tag)
      tag.m_strPictureURL.Parse();
      for (const auto& url : tag.m_strPictureURL.GetUrls())
        if (StringUtils::StartsWith(url.m_aspect, "set."))
          movieSetArt.try_emplace(url.m_aspect.substr(4), url.m_url);
    }
    if (!movieSetArt.empty())
      tag.m_set.SetArt(movieSetArt);

    return setUpdated;
  }

  bool CVideoInfoScanner::RetrieveVideoInfo(const CFileItemList& items,
                                            bool bDirNames,
                                            ContentType content,
                                            bool useLocal,
                                            CScraperUrl* pURL,
                                            bool fetchEpisodes,
                                            CGUIDialogProgress* pDlgProgress)
  {
    if (pDlgProgress)
    {
      if (items.Size() > 1 || (items[0]->IsFolder() && fetchEpisodes))
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
    seenPaths.reserve(items.Size());
    for (int i = 0; i < items.Size(); ++i)
    {
      CFileItemPtr pItem = items[i];

      // we do this since we may have a override per dir
      ScraperPtr info2 = m_database.GetScraperForPath(
          pItem->IsFolder() ? pItem->GetPath() : items.GetPath(), &m_scraperCache);
      if (!info2) // skip
        continue;

      // Discard all .nomedia folders
      if (pItem->IsFolder() && HasNoMedia(pItem->GetPath()))
        continue;

      // Discard all exclude files defined by regExExclude
      if (CUtil::ExcludeFileOrFolder(pItem->GetPath(),
                                     (content == ContentType::TVSHOWS)
                                         ? m_advancedSettings->m_tvshowExcludeFromScanRegExps
                                         : m_advancedSettings->m_moviesExcludeFromScanRegExps))
        continue;

      if (info2->Content() == ContentType::MOVIES || info2->Content() == ContentType::MUSICVIDEOS)
      {
        if (m_handle)
          m_handle->SetPercentage(i*100.f/items.Size());
      }

      InfoRet ret = InfoRet::CANCELLED;
      if (info2->Content() == ContentType::TVSHOWS)
        ret = RetrieveInfoForTvShow(pItem.get(), bDirNames, info2, useLocal, pURL, fetchEpisodes, pDlgProgress);
      else if (info2->Content() == ContentType::MOVIES)
        ret = RetrieveInfoForMovie(pItem.get(), bDirNames, info2, useLocal, pURL, pDlgProgress);
      else if (info2->Content() == ContentType::MUSICVIDEOS)
        ret = RetrieveInfoForMusicVideo(pItem.get(), bDirNames, info2, useLocal, pURL, pDlgProgress);
      else
      {
        CLog::Log(LOGERROR, "VideoInfoScanner: Unknown content type {} ({})", info2->Content(),
                  CURL::GetRedacted(pItem->GetPath()));
        FoundSomeInfo = false;
        break;
      }
      if (ret == InfoRet::CANCELLED || ret == InfoRet::INFO_ERROR)
      {
        CLog::Log(LOGWARNING,
                  "VideoInfoScanner: Error {} occurred while retrieving"
                  "information for {}.",
                  static_cast<int>(ret), CURL::GetRedacted(pItem->GetPath()));
        FoundSomeInfo = false;
        break;
      }
      if (ret == InfoRet::ADDED || ret == InfoRet::HAVE_ALREADY)
        FoundSomeInfo = true;
      else if (ret == InfoRet::NOT_FOUND)
      {
        CLog::Log(LOGWARNING,
                  "No information found for item '{}', it won't be added to the library.",
                  CURL::GetRedacted(pItem->GetPath()));

        MediaType mediaType = MediaTypeMovie;
        if (info2->Content() == ContentType::TVSHOWS)
          mediaType = MediaTypeTvShow;
        else if (info2->Content() == ContentType::MUSICVIDEOS)
          mediaType = MediaTypeMusicVideo;

        auto eventLog = CServiceBroker::GetEventLog();
        if (eventLog)
        {
          const std::string itemlogpath = (info2->Content() == ContentType::TVSHOWS)
                                              ? CURL::GetRedacted(pItem->GetPath())
                                              : URIUtils::GetFileName(pItem->GetPath());

          eventLog->Add(EventPtr(new CMediaLibraryEvent(
              mediaType, pItem->GetPath(), 24145,
              StringUtils::Format(
                  CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(24147),
                  mediaType, itemlogpath),
              EventLevel::Warning)));
        }
      }

      pURL = NULL;

      // Keep track of directories we've seen
      if (m_bClean && pItem->IsFolder())
        seenPaths.push_back(m_database.GetPathId(pItem->GetPath()));
    }

    if (content == ContentType::TVSHOWS && !seenPaths.empty())
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

  CInfoScanner::InfoRet CVideoInfoScanner::RetrieveInfoForTvShow(CFileItem* pItem,
                                                                 bool bDirNames,
                                                                 ScraperPtr& info2,
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
    if (pItem->IsFolder())
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

    // Enumerate episodes here as this compares hashes of folders containing episodes from this show
    //  and we want to do this before processing nfo files
    EPISODELIST files;
    if (!EnumerateSeriesFolder(pItem, files))
      return InfoRet::HAVE_ALREADY;
    if (files.empty()) // no update or no files
      return InfoRet::NOT_NEEDED;

    if (ProgressCancelled(pDlgProgress, pItem->IsFolder() ? 20353 : 20361,
                          pItem->IsFolder() ? pItem->GetVideoInfoTag()->m_strShowTitle
                                            : pItem->GetVideoInfoTag()->m_strTitle))
      return InfoRet::CANCELLED;

    if (m_handle)
      m_handle->SetText(pItem->GetMovieName(bDirNames));

    InfoType result = InfoType::NONE;
    CScraperUrl scrUrl;
    // handle .nfo files
    std::unique_ptr<IVideoInfoTagLoader> loader;
    if (useLocal)
      std::tie(result, loader) = ReadInfoTag(*pItem, info2, bDirNames, true);

    if (result == InfoType::FULL && idTvShow < 0)
    {

      long lResult = AddVideo(pItem, info2, bDirNames, useLocal);
      if (lResult < 0)
        return InfoRet::INFO_ERROR;
      if (fetchEpisodes)
      {
        InfoRet ret = RetrieveInfoForEpisodes(pItem, lResult, files, info2, useLocal, pDlgProgress);
        if (ret == InfoRet::ADDED)
          m_database.SetPathHash(pItem->GetPath(), pItem->GetProperty("hash").asString());
        return ret;
      }
      return InfoRet::ADDED;
    }
    if (result == InfoType::URL || result == InfoType::COMBINED)
    {
      scrUrl = loader->ScraperUrl();
      pURL = &scrUrl;
    }

    // Process episodes added later after nfo is scanned in case there is an episode group and parsing url
    if (idTvShow > -1 && (!isSeason || idSeason > -1) && (fetchEpisodes || !pItem->IsFolder()))
    {
      InfoRet ret = RetrieveInfoForEpisodes(pItem, idTvShow, files, info2, useLocal, pDlgProgress);
      if (ret == InfoRet::ADDED)
        m_database.SetPathHash(strPath, pItem->GetProperty("hash").asString());
      return ret;
    }

    CScraperUrl url;
    int retVal = 0;
    std::string movieTitle = pItem->GetMovieName(bDirNames);
    int movieYear = -1; // hint that movie title was not found
    if (result == InfoType::TITLE)
    {
      CVideoInfoTag* tag = pItem->GetVideoInfoTag();
      movieTitle = tag->GetTitle();
      movieYear = tag->GetYear(); // movieYear is expected to be >= 0
    }

    std::string identifierType;
    std::string identifier;
    long lResult = -1;
    if (info2->IsPython() && CUtil::GetFilenameIdentifier(movieTitle, identifierType, identifier))
    {
      const ADDON::CScraper::UniqueIDs uniqueIDs{{identifierType, identifier}};
      if (GetDetails(pItem, uniqueIDs, url, info2,
                     (result == InfoType::COMBINED || result == InfoType::OVERRIDE) ? loader.get()
                                                                                    : nullptr,
                     pDlgProgress))
      {
        if ((lResult = AddVideo(pItem, info2, false, useLocal)) < 0)
          return InfoRet::INFO_ERROR;

        if (fetchEpisodes)
        {
          InfoRet ret =
              RetrieveInfoForEpisodes(pItem, lResult, files, info2, useLocal, pDlgProgress, true);
          if (ret == InfoRet::ADDED)
          {
            m_database.SetPathHash(pItem->GetPath(), pItem->GetProperty("hash").asString());
            return InfoRet::ADDED;
          }
        }
        return InfoRet::ADDED;
      }
    }

    if (pURL && pURL->HasUrls())
      url = *pURL;
    else if ((retVal = FindVideo(movieTitle, movieYear, info2, url, pDlgProgress)) <= 0)
      return retVal < 0 ? InfoRet::CANCELLED : InfoRet::NOT_FOUND;

    CLog::Log(LOGDEBUG, "VideoInfoScanner: Fetching url '{}' using {} scraper (content: '{}')",
              url.GetFirstThumbUrl(), info2->Name(), info2->Content());

    if (GetDetails(pItem, {}, url, info2,
                   (result == InfoType::COMBINED || result == InfoType::OVERRIDE) ? loader.get()
                                                                                  : nullptr,
                   pDlgProgress))
    {
      if ((lResult = AddVideo(pItem, info2, false, useLocal)) < 0)
        return InfoRet::INFO_ERROR;
    }
    if (fetchEpisodes)
    {
      InfoRet ret =
          RetrieveInfoForEpisodes(pItem, lResult, files, info2, useLocal, pDlgProgress, true);
      if (ret == InfoRet::ADDED)
        m_database.SetPathHash(pItem->GetPath(), pItem->GetProperty("hash").asString());
    }
    return InfoRet::ADDED;
  }

  CInfoScanner::InfoRet CVideoInfoScanner::RetrieveInfoForMovie(CFileItem* pItem,
                                                                bool bDirNames,
                                                                ScraperPtr& info2,
                                                                bool useLocal,
                                                                CScraperUrl* pURL,
                                                                CGUIDialogProgress* pDlgProgress)
  {
    if (pItem->IsFolder() || !IsVideo(*pItem) || pItem->IsNFO() ||
        (PLAYLIST::IsPlayList(*pItem) && !URIUtils::HasExtension(pItem->GetPath(), ".strm")))
      return InfoRet::NOT_NEEDED;

    if (ProgressCancelled(pDlgProgress, 198, pItem->GetLabel()))
      return InfoRet::CANCELLED;

    if (m_handle)
      m_handle->SetText(pItem->GetMovieName(bDirNames));

    InfoType result = InfoType::NONE;
    CScraperUrl scrUrl;

    // handle .nfo files
    std::unique_ptr<IVideoInfoTagLoader> loader;
    CFileItem item{*pItem};
    if (useLocal)
      std::tie(result, loader) = ReadInfoTag(item, info2, bDirNames, true);
    if (result == InfoType::FULL)
    {
      // Add the movie entry
      int movieId{-1};
      CVideoInfoTag* tag{item.GetVideoInfoTag()};
      if (tag->HasVideoVersions())
      {
        // See if movie already in library
        CFileItemList items;
        using enum CVideoDatabase::MatchingMask;
        m_database.GetSameVideoItems(item, items, UniqueId | (bDirNames ? Path : None));
        if (!items.IsEmpty())
        {
          // Movie already exists
          movieId = items[0]->GetVideoInfoTag()->m_iDbId;
          CLog::LogF(LOGDEBUG,
                     "Movie '{}' already exists in the library as id {} - adding as version",
                     tag->GetTitle(), movieId);

          // Add as version
          tag->m_iDbId = movieId;
          if (AddVideo(&item, nullptr, bDirNames, true, nullptr, false,
                       ContentType::MOVIE_VERSIONS) < 0)
            return InfoRet::INFO_ERROR;
        }
      }

      // Existing movie cannot be found or is not version - add it
      if (movieId < 0)
      {
        movieId = static_cast<int>(AddVideo(&item, info2, bDirNames, true));
        if (movieId < 0)
          return InfoRet::INFO_ERROR;

        // Deal with set
        if (UpdateSetInTag(*pItem->GetVideoInfoTag()))
          if (!AddSet(pItem->GetVideoInfoTag()->m_set))
            return InfoRet::INFO_ERROR;
      }

      item.SetProperty("from_nfo", true);

      // Look for default version
      int defaultVersionFileId{-1};
      if (tag->IsDefaultVideoVersion())
        defaultVersionFileId = tag->m_iFileId; // Updated in AddMovie()

      // Look for versions (ie. subsequent <movie> entries in the .nfo file)
      // These must be versions
      int index{1};
      do
      {
        index++;
        item.SetProperty("nfo_index", index); // Attempt to read next movie version
        std::tie(result, loader) = ReadInfoTag(item, info2, bDirNames, true);
        if (result == InfoType::FULL)
        {
          // Add the version entry
          tag->m_iDbId = movieId;
          const int versionId{static_cast<int>(AddVideo(&item, nullptr, bDirNames, true, nullptr,
                                                        false, ContentType::MOVIE_VERSIONS))};
          if (versionId < 0)
            return InfoRet::INFO_ERROR;

          // Look for default version
          if (tag->IsDefaultVideoVersion())
            defaultVersionFileId = tag->m_iFileId; // Updated in AddVideoAsset()
        }
      } while (result == InfoType::FULL);

      // Set default version
      if (defaultVersionFileId > -1)
        m_database.SetDefaultVideoVersion(VideoDbContentType::MOVIES, movieId,
                                          defaultVersionFileId);

      return InfoRet::ADDED;
    }

    // If no nfo then return here if movie already in library
    if (m_database.HasMovieInfo(pItem->GetDynPath()))
      return InfoRet::HAVE_ALREADY;

    if (result == InfoType::URL || result == InfoType::COMBINED)
    {
      scrUrl = loader->ScraperUrl();
      pURL = &scrUrl;
    }

    CScraperUrl url;
    int retVal = 0;
    std::string movieTitle = pItem->GetMovieName(bDirNames);
    int movieYear = -1; // hint that movie title was not found
    if (result == InfoType::TITLE)
    {
      CVideoInfoTag* tag = pItem->GetVideoInfoTag();
      movieTitle = tag->GetTitle();
      movieYear = tag->GetYear(); // movieYear is expected to be >= 0
    }

    std::string identifierType;
    std::string identifier;
    if (info2->IsPython() && CUtil::GetFilenameIdentifier(movieTitle, identifierType, identifier))
    {
      const ADDON::CScraper::UniqueIDs uniqueIDs{{identifierType, identifier}};
      if (GetDetails(pItem, uniqueIDs, url, info2,
                     (result == InfoType::COMBINED || result == InfoType::OVERRIDE) ? loader.get()
                                                                                    : nullptr,
                     pDlgProgress))
      {
        if (UpdateSetInTag(*pItem->GetVideoInfoTag()) && !AddSet(pItem->GetVideoInfoTag()->m_set))
          return InfoRet::INFO_ERROR;
        const int dbId{static_cast<int>(AddVideo(pItem, info2, bDirNames, useLocal))};
        if (dbId < 0)
          return InfoRet::INFO_ERROR;
        if (!m_ignoreVideoVersions && ProcessVideoVersion(VideoDbContentType::MOVIES, dbId))
          return InfoRet::HAVE_ALREADY;
        return InfoRet::ADDED;
      }
    }

    if (pURL && pURL->HasUrls())
      url = *pURL;
    else if ((retVal = FindVideo(movieTitle, movieYear, info2, url, pDlgProgress)) <= 0)
      return retVal < 0 ? InfoRet::CANCELLED : InfoRet::NOT_FOUND;

    CLog::Log(LOGDEBUG, "VideoInfoScanner: Fetching url '{}' using {} scraper (content: '{}')",
              url.GetFirstThumbUrl(), info2->Name(), info2->Content());

    if (GetDetails(pItem, {}, url, info2,
                   (result == InfoType::COMBINED || result == InfoType::OVERRIDE) ? loader.get()
                                                                                  : nullptr,
                   pDlgProgress))
    {
      if (UpdateSetInTag(*pItem->GetVideoInfoTag()) && !AddSet(pItem->GetVideoInfoTag()->m_set))
        return InfoRet::INFO_ERROR;
      const int dbId{static_cast<int>(AddVideo(pItem, info2, bDirNames, useLocal))};
      if (dbId < 0)
        return InfoRet::INFO_ERROR;
      if (!m_ignoreVideoVersions && ProcessVideoVersion(VideoDbContentType::MOVIES, dbId))
        return InfoRet::HAVE_ALREADY;
      return InfoRet::ADDED;
    }
    //! @todo This is not strictly correct as we could fail to download information here or error, or be cancelled
    return InfoRet::NOT_FOUND;
  }

  CInfoScanner::InfoRet CVideoInfoScanner::RetrieveInfoForMusicVideo(
      CFileItem* pItem,
      bool bDirNames,
      ScraperPtr& info2,
      bool useLocal,
      CScraperUrl* pURL,
      CGUIDialogProgress* pDlgProgress)
  {
    if (pItem->IsFolder() || !IsVideo(*pItem) || pItem->IsNFO() ||
        (PLAYLIST::IsPlayList(*pItem) && !URIUtils::HasExtension(pItem->GetPath(), ".strm")))
      return InfoRet::NOT_FOUND;

    if (ProgressCancelled(pDlgProgress, 20394, pItem->GetLabel()))
      return InfoRet::CANCELLED;

    if (m_database.HasMusicVideoInfo(pItem->GetPath()))
      return InfoRet::HAVE_ALREADY;

    if (m_handle)
      m_handle->SetText(pItem->GetMovieName(bDirNames));

    InfoType result = InfoType::NONE;
    CScraperUrl scrUrl;
    // handle .nfo files
    std::unique_ptr<IVideoInfoTagLoader> loader;
    if (useLocal)
      std::tie(result, loader) = ReadInfoTag(*pItem, info2, bDirNames, true);
    if (result == InfoType::FULL)
    {
      if (AddVideo(pItem, info2, bDirNames, true) < 0)
        return InfoRet::INFO_ERROR;
      return InfoRet::ADDED;
    }
    if (result == InfoType::URL || result == InfoType::COMBINED)
    {
      scrUrl = loader->ScraperUrl();
      pURL = &scrUrl;
    }

    CScraperUrl url;
    int retVal = 0;
    std::string movieTitle = pItem->GetMovieName(bDirNames);
    int movieYear = -1; // hint that movie title was not found
    if (result == InfoType::TITLE)
    {
      CVideoInfoTag* tag = pItem->GetVideoInfoTag();
      movieTitle = tag->GetTitle();
      movieYear = tag->GetYear(); // movieYear is expected to be >= 0
    }

    std::string identifierType;
    std::string identifier;
    if (info2->IsPython() && CUtil::GetFilenameIdentifier(movieTitle, identifierType, identifier))
    {
      const ADDON::CScraper::UniqueIDs uniqueIDs{{identifierType, identifier}};
      if (GetDetails(pItem, uniqueIDs, url, info2,
                     (result == InfoType::COMBINED || result == InfoType::OVERRIDE) ? loader.get()
                                                                                    : nullptr,
                     pDlgProgress))
      {
        if (AddVideo(pItem, info2, bDirNames, useLocal) < 0)
          return InfoRet::INFO_ERROR;
        return InfoRet::ADDED;
      }
    }

    if (pURL && pURL->HasUrls())
      url = *pURL;
    else if ((retVal = FindVideo(movieTitle, movieYear, info2, url, pDlgProgress)) <= 0)
      return retVal < 0 ? InfoRet::CANCELLED : InfoRet::NOT_FOUND;

    CLog::Log(LOGDEBUG, "VideoInfoScanner: Fetching url '{}' using {} scraper (content: '{}')",
              url.GetFirstThumbUrl(), info2->Name(), info2->Content());

    if (GetDetails(pItem, {}, url, info2,
                   (result == InfoType::COMBINED || result == InfoType::OVERRIDE) ? loader.get()
                                                                                  : nullptr,
                   pDlgProgress))
    {
      if (AddVideo(pItem, info2, bDirNames, useLocal) < 0)
        return InfoRet::INFO_ERROR;
      return InfoRet::ADDED;
    }
    //! @todo This is not strictly correct as we could fail to download information here or error, or be cancelled
    return InfoRet::NOT_FOUND;
  }

  CInfoScanner::InfoRet CVideoInfoScanner::RetrieveInfoForEpisodes(CFileItem* item,
                                                                   long showID,
                                                                   EPISODELIST& files,
                                                                   const ADDON::ScraperPtr& scraper,
                                                                   bool useLocal,
                                                                   CGUIDialogProgress* progress,
                                                                   bool alreadyHasArt /* = false */)
  {
    if (m_bStop || (progress && progress->IsCanceled()))
      return InfoRet::CANCELLED;

    CVideoInfoTag showInfo;
    m_database.GetTvShowInfo("", showInfo, showID);
    InfoRet ret = OnProcessSeriesFolder(files, scraper, useLocal, showInfo, progress);

    if (ret == InfoRet::ADDED)
    {
      KODI::ART::SeasonsArtwork seasonArt;
      m_database.GetTvShowSeasonArt(showID, seasonArt);

      const bool updateSeasonArt{
          alreadyHasArt ||
          std::ranges::any_of(seasonArt, [](const auto& i) { return i.second.empty(); })};
      if (updateSeasonArt)
      {
        if (!alreadyHasArt && !item->IsPlugin() && scraper->ID() != "metadata.local")
        {
          CVideoInfoDownloader loader(scraper);
          CVideoInfoTag tag{showInfo};
          loader.GetArtwork(tag); // Can alter other fields in the tag
          showInfo.m_strPictureURL = tag.m_strPictureURL; // We only want artwork
        }
        const UseRemoteArtWithLocalScraper useRemoteArt{
            scraper->ID() == "metadata.local" && m_advancedSettings->m_bNoRemoteArtWithLocalScraper
                ? UseRemoteArtWithLocalScraper::NO
                : UseRemoteArtWithLocalScraper::YES};
        GetSeasonThumbs(showInfo, seasonArt, CVideoThumbLoader::GetArtTypes(MediaTypeSeason),
                        useLocal && !item->IsPlugin(), useRemoteArt);
        for (const auto& [season, art] : seasonArt)
        {
          const int seasonID{m_database.AddSeason(static_cast<int>(showID), season)};
          m_database.SetArtForItem(seasonID, MediaTypeSeason, art);
        }
      }
    }
    return ret;
  }

  bool CVideoInfoScanner::EnumerateSeriesFolder(CFileItem* item, EPISODELIST& episodeList)
  {
    CFileItemList items;
    const std::vector<std::string>& regexps = m_advancedSettings->m_tvshowExcludeFromScanRegExps;

    bool bSkip = false;

    if (item->IsFolder())
    {
      /*
       * Note: DoScan() will not remove this path as it's not recursing for tvshows.
       * Remove this path from the list we're processing in order to avoid hitting
       * it twice in the main loop.
       */
      std::set<std::string>::iterator it = m_pathsToScan.find(item->GetPath());
      if (it != m_pathsToScan.end())
        m_pathsToScan.erase(it);

      if (HasNoMedia(item->GetPath()))
        return true;

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
      else if (m_advancedSettings->m_bVideoLibraryUseFastHash)
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

        // Listing that ignores files inside and below folders containing .nomedia files.
        CDirectory::EnumerateDirectory(
            item->GetPath(), [&items](const std::shared_ptr<CFileItem>& item) { items.Add(item); },
            [](const std::shared_ptr<CFileItem>& folder) { return !HasNoMedia(folder->GetPath()); },
            true, CServiceBroker::GetFileExtensionProvider().GetVideoExtensions(), flags);

        // fast hash failed - compute slow one
        if (hash.empty())
        {
          // force sorting consistency to avoid hash mismatch between platforms
          // sort by filename as always present for any files, but keep case sensitivity
          items.Sort(SortByFile, SortOrder::ASCENDING, SortAttributeNone);
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
    items.Sort(SortByPath, SortOrder::ASCENDING);

    // If found VIDEO_TS.IFO or INDEX.BDMV then we are dealing with Blu-ray or DVD files on disc
    // somewhere in the directory tree. Assume that all other files/folders in the same folder
    // with VIDEO_TS or BDMV can be ignored.
    // THere can be a BACKUP/INDEX.BDMV which needs to be ignored (and broke the old while loop here)

    // Get folders to remove
    std::vector<std::string> foldersToRemove;
    for (const auto& item : items)
    {
      const std::string file = StringUtils::ToUpper(item->GetPath());
      if (file.find("VIDEO_TS.IFO") != std::string::npos)
        foldersToRemove.emplace_back(StringUtils::ToUpper(URIUtils::GetDirectory(file)));
      if (file.find("INDEX.BDMV") != std::string::npos &&
          file.find("BACKUP/INDEX.BDMV") == std::string::npos)
        foldersToRemove.emplace_back(
            StringUtils::ToUpper(URIUtils::GetParentPath(URIUtils::GetDirectory(file))));
    }

    // Remove folders
    items.erase(
        std::remove_if(items.begin(), items.end(),
                       [&](const CFileItemPtr& i)
                       {
                         const std::string fileAndPath(StringUtils::ToUpper(i->GetPath()));
                         std::string file;
                         std::string path;
                         URIUtils::Split(fileAndPath, path, file);
                         return (std::count_if(foldersToRemove.begin(), foldersToRemove.end(),
                                               [&](const std::string& removePath)
                                               { return path.rfind(removePath, 0) == 0; }) > 0) &&
                                file != "VIDEO_TS.IFO" &&
                                (file != "INDEX.BDMV" ||
                                 fileAndPath.find("BACKUP/INDEX.BDMV") != std::string::npos);
                       }),
        items.end());

    // enumerate
    for (int i=0;i<items.Size();++i)
    {
      if (items[i]->IsFolder())
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
    SETTINGS_TVSHOWLIST expression = m_advancedSettings->m_tvshowEnumRegExps;

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

      const bool disableEpisodeRanges{m_advancedSettings->m_disableEpisodeRanges};

      CRegExp reg2(true, CRegExp::autoUtf8);
      // check the remainder of the string for any further episodes.
      if (!byDate && reg2.RegComp(m_advancedSettings->m_tvshowMultiPartEnumRegExp))
      {
        size_t offset{0};
        int currentSeason{episode.iSeason};
        int currentEpisode{episode.iEpisode};

        // we want "long circuit" OR below so that both offsets are evaluated
        while (static_cast<int>((regexp2pos = reg2.RegFind(remainder.c_str() + offset)) > -1) |
               static_cast<int>((regexppos = reg.RegFind(remainder.c_str() + offset)) > -1))
        {
          if ((regexppos <= regexp2pos && regexppos != -1) || // season (or 'ep') match
              (regexppos >= 0 && regexp2pos == -1))
          {
            GetEpisodeAndSeasonFromRegExp(reg, episode, defaultSeason);
            if (currentSeason == episode.iSeason)
            {
              // Already added SxxEyy now loop (if needed) to SxxEzz
              const int last{episode.iEpisode};
              const int next{
                  disableEpisodeRanges || !remainder.starts_with("-") ? last : currentEpisode + 1};

              ProcessEpisodeRange(next, last, episode, episodeList,
                                  m_advancedSettings->m_tvshowMultiPartEnumRegExp, remainder);

              currentEpisode = episode.iEpisode;
              remainder = reg.GetMatch(3);
            }
            else
            {
              // Two possible scenarios here:
              if (remainder.substr(offset, 1) != "-" || disableEpisodeRanges)
              {
                // (Sxx)Eyy has already been added and we now in a new range (eg. S00E01S01E01....)
                // Add first episode here
                currentSeason = episode.iSeason;
                currentEpisode = episode.iEpisode;
                episodeList.push_back(episode);
                remainder = reg.GetMatch(3);
              }
              else
              {
                // (Sxx)Eyy has already been added as start of range and we now have SaaEbb (eg. S01E01-S02E05)
                //   this is not allowed as scanner cannot determine how many episodes in a season
                if (offset == 0)
                {
                  // Already added first episode of invalid range so remove it
                  episodeList.pop_back();
                  remainder = reg.GetMatch(3);
                  CLog::LogF(
                      LOGDEBUG,
                      "VideoInfoScanner: Removing season {}, episode {} as part of invalid range",
                      episode.iSeason, episode.iEpisode);
                }
                else
                {
                  remainder = remainder.substr(offset);
                }
              }
            }
            offset = 0;
          }
          else if ((regexp2pos < regexppos && regexp2pos != -1) || // episode match
                   (regexp2pos >= 0 && regexppos == -1))
          {
            const std::string result{reg2.GetMatch(2)};
            const int last{std::stoi(result)};
            const std::string prefix{
                offset < remainder.length()
                    ? StringUtils::ToLower(std::string_view(remainder).substr(offset, 2))
                    : std::string{}};
            const int next{(prefix == "-e" || prefix == "-s") && !disableEpisodeRanges
                               ? currentEpisode + 1
                               : last};

            ProcessEpisodeRange(next, last, episode, episodeList,
                                m_advancedSettings->m_tvshowMultiPartEnumRegExp, reg2.GetMatch(3));

            currentEpisode = episode.iEpisode;
            offset += regexp2pos + reg2.GetMatch(1).length() + result.length();
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

  bool CVideoInfoScanner::AddSet(const CSetInfoTag& set)
  {
    // ensure our database is open (this can get called via other classes)
    if (!m_database.Open())
      return false;

    CLog::LogF(LOGDEBUG, "Adding new set {}", set.GetTitle());

    // Create set
    const int idSet{m_database.AddSet(set.GetTitle(), set.GetOverview(), set.GetOriginalTitle())};

    // Assume art in set
    if (idSet > 0)
      return m_database.SetArtForItem(idSet, MediaTypeVideoCollection, set.GetArt());

    return false;
  }

  long CVideoInfoScanner::AddVideo(CFileItem* pItem,
                                   const ScraperPtr& scraper,
                                   bool videoFolder,
                                   bool useLocal,
                                   const CVideoInfoTag* showInfo,
                                   bool libraryImport,
                                   ContentType contentOverride)
  {
    // ensure our database is open (this can get called via other classes)
    if (!m_database.Open())
      return -1;

    const ContentType content{
        !scraper || contentOverride != ContentType::NONE ? contentOverride : scraper->Content()};
    const bool usingLocalScraper{scraper && scraper->ID() == "metadata.local"};
    const UseRemoteArtWithLocalScraper useRemoteArt{
        usingLocalScraper && m_advancedSettings->m_bNoRemoteArtWithLocalScraper
            ? UseRemoteArtWithLocalScraper::NO
            : UseRemoteArtWithLocalScraper::YES};

    std::string path;
    const int playlist = pItem->HasVideoInfoTag() ? pItem->GetVideoInfoTag()->m_iTrack : -1;
    if (playlist > -1 && (content == ContentType::MOVIES || content == ContentType::MOVIE_VERSIONS))
    {
      path = URIUtils::GetBlurayPlaylistPath(pItem->GetPath(), playlist);
      pItem->SetDynPath(path);
    }
    else
      path = pItem->GetPath();

    if (!libraryImport)
      GetArtwork(pItem, content, videoFolder, useLocal && !pItem->IsPlugin(),
                 showInfo ? showInfo->m_strPath : "", useRemoteArt);

    // ensure the art map isn't completely empty by specifying an empty thumb
    KODI::ART::Artwork art = pItem->GetArt();
    if (art.empty())
      art["thumb"] = "";

    CVideoInfoTag &movieDetails = *pItem->GetVideoInfoTag();
    if (movieDetails.m_basePath.empty())
      movieDetails.m_basePath = pItem->GetBaseMoviePath(videoFolder);
    movieDetails.m_parentPathID =
        m_database.AddPath(URIUtils::GetParentPath(movieDetails.m_basePath));

    movieDetails.m_strFileNameAndPath = path;

    if (pItem->IsFolder())
      movieDetails.m_strPath = path;

    std::string strTitle(movieDetails.m_strTitle);

    if (showInfo && content == ContentType::TVSHOWS)
    {
      strTitle = StringUtils::Format("{} - {}x{} - {}", showInfo->m_strTitle,
                                     movieDetails.m_iSeason, movieDetails.m_iEpisode, strTitle);
    }

    /* As HasStreamDetails() returns true for TV shows (because the scraper calls SetVideoInfoTag()
     * directly to set the duration) a better test is just to see if we have any common flag info
     * missing.  If we have already read an nfo file then this data should be populated, otherwise
     * get it from the video file */

    if (CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
            CSettings::SETTING_MYVIDEOS_EXTRACTFLAGS))
    {
      const auto& strmdetails = movieDetails.m_streamDetails;
      if (strmdetails.GetVideoCodec(1).empty() || strmdetails.GetVideoHeight(1) == 0 ||
          strmdetails.GetVideoWidth(1) == 0 || strmdetails.GetVideoDuration(1) == 0)

      {
        CDVDFileInfo::GetFileStreamDetails(pItem);
        CLog::Log(LOGDEBUG, "VideoInfoScanner: Extracted filestream details from video file {}",
                  CURL::GetRedacted(path));
      }
    }

    CLog::Log(LOGDEBUG, "VideoInfoScanner: Adding new item to {}:{}", content,
              CURL::GetRedacted(path));
    long lResult = -1;

    if (content == ContentType::MOVIES)
    {
      // find local trailer first
      std::string strTrailer = UTILS::FindTrailer(*pItem);
      if (!strTrailer.empty())
        movieDetails.m_strTrailer = strTrailer;

      // Remove remote set art (if need)
      if (useRemoteArt == UseRemoteArtWithLocalScraper::NO)
        std::erase_if(art,
                      [](const std::pair<std::string, std::string>& artItem)
                      {
                        const auto& [type, url] = artItem;
                        return StringUtils::StartsWith(type, "set.") && URIUtils::IsRemote(url);
                      });

      // Deal with 'Disc n' subdirectories
      // Unless dealing with a full nfo in which case details are taken from there already
      if (!pItem->GetProperty("from_nfo").asBoolean(false) && !pItem->IsStack())
      {
        const std::string discNum{CUtil::GetPartNumberFromPath(movieDetails.m_strFileNameAndPath)};
        if (!discNum.empty())
        {
          if (!movieDetails.m_set.HasTitle())
          {
            const std::string setName{m_database.GetSetByNameLike(movieDetails.m_strTitle)};
            if (!setName.empty())
            {
              // Add movie to existing set
              movieDetails.SetSet(setName);
            }
            else
            {
              // Create set, then add movie to the set
              const int idSet{m_database.AddSet(movieDetails.m_strTitle)};
              m_database.SetArtForItem(idSet, MediaTypeVideoCollection, art);
              movieDetails.SetSet(movieDetails.m_strTitle);
            }
          }

          // Add '(Disc n)' to title (in local language)
          movieDetails.m_strTitle = StringUtils::Format(
              CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(29995),
              movieDetails.m_strTitle, discNum);
        }
      }

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
    else if (content == ContentType::MOVIE_VERSIONS)
    {
      if (pItem->HasVideoInfoTag())
      {
        const CVideoInfoTag* tag{pItem->GetVideoInfoTag()};
        const int idMovie{
            tag->m_iDbId}; // Set in AddVideo() for movies and RetrieveInfoForMovie() for versions
        if (idMovie != -1)
        {
          pItem->SetArt(art); // May have been filtered above
          CVideoInfoTag* vtag{pItem->GetVideoInfoTag()};

          // Need to look up asset title in current table as, if importing, it may have a different id (primary key)
          const std::string assetTitle{vtag->GetAssetInfo().GetTitle()};
          const int assetId{m_database.AddOrValidateVideoVersionType(assetTitle)};

          lResult = m_database.AddVideoAsset(VideoDbContentType::MOVIES, idMovie, assetId,
                                             VideoAssetType::VERSION, *pItem)
                        ? vtag->m_iFileId
                        : -1;
        }
      }
    }
    else if (content == ContentType::TVSHOWS)
    {
      if (pItem->IsFolder())
      {
        /*
         multipaths are not stored in the database, so in the case we have one,
         we split the paths, and compute the parent paths in each case.
         */
        std::vector<std::string> multipath;
        if (!URIUtils::IsMultiPath(path) || !CMultiPathDirectory::GetPaths(path, multipath))
          multipath.push_back(path);

        KODI::ART::SeasonsArtwork seasonArt;

        if (!libraryImport)
          GetSeasonThumbs(movieDetails, seasonArt, CVideoThumbLoader::GetArtTypes(MediaTypeSeason),
                          useLocal && !pItem->IsPlugin(), useRemoteArt);

        lResult = m_database.SetDetailsForTvShow(multipath, movieDetails, art, seasonArt);
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
          movieDetails.m_strFileNameAndPath = path;
          movieDetails.m_EpBookmark.seasonNumber = movieDetails.m_iSeason;
          movieDetails.m_EpBookmark.episodeNumber = movieDetails.m_iEpisode;
          m_database.AddBookMarkForEpisode(movieDetails, movieDetails.m_EpBookmark);
        }
      }
    }
    else if (content == ContentType::MUSICVIDEOS)
    {
      lResult = m_database.SetDetailsForMusicVideo(movieDetails, art);
      movieDetails.m_iDbId = lResult;
      movieDetails.m_type = MediaTypeMusicVideo;
    }

    if (!pItem->IsFolder())
    {
      if ((libraryImport || m_advancedSettings->m_bVideoLibraryImportWatchedState) &&
          (movieDetails.IsPlayCountSet() || movieDetails.m_lastPlayed.IsValid()))
        m_database.SetPlayCount(*pItem, movieDetails.GetPlayCount(), movieDetails.m_lastPlayed);

      if ((libraryImport || m_advancedSettings->m_bVideoLibraryImportResumePoint) &&
          movieDetails.GetResumePoint().IsSet())
        m_database.AddBookMarkToFile(path, movieDetails.GetResumePoint(), CBookmark::RESUME);
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

  std::string ContentToMediaType(ContentType content, bool folder)
  {
    switch (content)
    {
      using enum ContentType;
      case MOVIES:
        return MediaTypeMovie;
      case MUSICVIDEOS:
        return MediaTypeMusicVideo;
      case TVSHOWS:
        return folder ? MediaTypeTvShow : MediaTypeEpisode;
      default:
        return "";
    }
  }

  VideoDbContentType ContentToVideoDbType(ContentType content)
  {
    switch (content)
    {
      using enum ContentType;
      case MOVIES:
        return VideoDbContentType::MOVIES;
      case MUSICVIDEOS:
        return VideoDbContentType::MUSICVIDEOS;
      case TVSHOWS:
        return VideoDbContentType::EPISODES;
      default:
        return VideoDbContentType::UNKNOWN;
    }
  }

  std::pair<CVideoInfoScanner::InfoType, std::unique_ptr<IVideoInfoTagLoader>> CVideoInfoScanner::
      ReadInfoTag(CFileItem& item,
                  const ADDON::ScraperPtr& scraper,
                  bool lookInFolder,
                  bool resetTag)
  {
    if (std::unique_ptr<IVideoInfoTagLoader> loader{
            CVideoInfoTagLoaderFactory::CreateLoader(item, scraper, lookInFolder)};
        loader)
    {
      CVideoInfoTag& infoTag = *item.GetVideoInfoTag();
      if (resetTag)
        infoTag.Reset();
      auto result = loader->Load(infoTag, false);

      // keep some properties only if advancedsettings.xml says so
      if (!m_advancedSettings->m_bVideoLibraryImportWatchedState)
        infoTag.ResetPlayCount();
      if (!m_advancedSettings->m_bVideoLibraryImportResumePoint)
        infoTag.SetResumePoint(CBookmark());

      return {result, std::move(loader)};
    }
    return {InfoType::NONE, nullptr};
  }

  std::string CVideoInfoScanner::GetMovieSetInfoFolder(const std::string& setTitle)
  {
    if (setTitle.empty())
      return "";
    std::string path = CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(
        CSettings::SETTING_VIDEOLIBRARY_MOVIESETSFOLDER);
    if (path.empty())
      return "";
    path = URIUtils::AddFileToFolder(path,
                                     CUtil::MakeLegalFileName(setTitle, LegalPath::WIN32_COMPAT));
    URIUtils::AddSlashAtEnd(path);
    CLog::Log(LOGDEBUG,
        "VideoInfoScanner: Looking for local artwork for movie set '{}' in folder '{}'",
        setTitle,
        CURL::GetRedacted(path));
    return CDirectory::Exists(path) ? path : "";
  }

  void CVideoInfoScanner::GetArtwork(CFileItem* pItem,
                                     ContentType content,
                                     bool bApplyToDir,
                                     bool useLocal,
                                     const std::string& actorArtPath,
                                     UseRemoteArtWithLocalScraper useRemoteArt /* = yes */) const
  {
    int artLevel = CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(
        CSettings::SETTING_VIDEOLIBRARY_ARTWORK_LEVEL);
    if (artLevel == CSettings::VIDEOLIBRARY_ARTWORK_LEVEL_NONE)
      return;

    CVideoInfoTag &movieDetails = *pItem->GetVideoInfoTag();
    movieDetails.m_fanart.Unpack();
    movieDetails.m_strPictureURL.Parse();

    KODI::ART::Artwork art = pItem->GetArt();

    // get and cache thumb images
    std::string mediaType = ContentToMediaType(content, pItem->IsFolder());
    std::vector<std::string> artTypes = CVideoThumbLoader::GetArtTypes(mediaType);
    bool moviePartOfSet = content == ContentType::MOVIES && movieDetails.m_set.HasTitle();
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
        bool useFolder = false;
        if (bApplyToDir && (content == ContentType::MOVIES || content == ContentType::MUSICVIDEOS))
        {
          std::string filename = ART::GetLocalArtBaseFilename(*pItem, useFolder);
          std::string directory = URIUtils::GetDirectory(filename);
          if (filename != directory)
            AddLocalItemArtwork(art, artTypes, filename, addAll, exactName, bApplyToDir);
        }

        // Reset useFolder to false as GetLocalArtBaseFilename may modify it in
        // the previous call.
        useFolder = false;

        std::string path;
        if (content == ContentType::TVSHOWS)
        {
          path = ART::GetLocalArtBaseFilename(*pItem, useFolder,
                                              pItem->GetProperty(MULTIPLE_EPISODES).asBoolean(false)
                                                  ? ART::AdditionalIdentifiers::SEASON_AND_EPISODE
                                                  : ART::AdditionalIdentifiers::NONE);
        }
        else if (content == ContentType::MOVIE_VERSIONS ||
                 (pItem->HasVideoVersions() && pItem->GetVideoInfoTag()->m_iTrack > -1))
        {
          // Add playlist identifier only when there are multiple versions of the movie on the same disc
          path =
              ART::GetLocalArtBaseFilename(*pItem, useFolder, ART::AdditionalIdentifiers::PLAYLIST);
        }
        else
          path = ART::GetLocalArtBaseFilename(*pItem, useFolder);
        AddLocalItemArtwork(art, artTypes, path, addAll, exactName, bApplyToDir);
      }

      if (moviePartOfSet)
      {
        std::string movieSetInfoPath = GetMovieSetInfoFolder(movieDetails.m_set.GetTitle());
        if (!movieSetInfoPath.empty())
        {
          KODI::ART::Artwork movieSetArt;
          AddLocalItemArtwork(movieSetArt, movieSetArtTypes, movieSetInfoPath, addAll, exactName,
                              true);
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
            !art.contains(it.m_type))
        {
          std::string thumb = IMAGE_FILES::URLFromFile(pItem->GetPath(), "video_" + it.m_type);
          art.insert(std::make_pair(it.m_type, thumb));
        }
      }
    }

    // add online fanart (treated separately due to it being stored in m_fanart)
    if ((addAll || CVideoThumbLoader::IsArtTypeInWhitelist("fanart", artTypes, exactName)) &&
        !art.contains("fanart"))
    {
      std::string fanart = pItem->GetVideoInfoTag()->m_fanart.GetImageURL();
      if (!fanart.empty() &&
          !(useRemoteArt == UseRemoteArtWithLocalScraper::NO && URIUtils::IsRemote(fanart)))
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
          !art.contains(aspect))
      {
        std::string image = GetImage(url, pItem->GetPath());
        if (!image.empty() &&
            !(useRemoteArt == UseRemoteArtWithLocalScraper::NO && URIUtils::IsRemote(image)))
          art.insert(std::make_pair(aspect, image));
      }
    }

    if (!art.contains("thumb") &&
        CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
            CSettings::SETTING_MYVIDEOS_EXTRACTTHUMB) &&
        CDVDFileInfo::CanExtract(*pItem))
    {
      art["thumb"] = CVideoThumbLoader::GetEmbeddedThumbURL(*pItem);
    }

    for (const auto& artType : artTypes)
    {
      if (art.contains(artType))
        CServiceBroker::GetTextureCache()->BackgroundCacheImage(art[artType]);
    }

    pItem->SetArt(art);

    // parent folder to apply the thumb to and to search for local actor thumbs
    std::string parentDir = URIUtils::GetBasePath(pItem->GetPath());
    if (CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
            CSettings::SETTING_VIDEOLIBRARY_ACTORTHUMBS))
      FetchActorThumbs(movieDetails.m_cast, actorArtPath.empty() ? parentDir : actorArtPath,
                       useRemoteArt);
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

  CInfoScanner::InfoRet CVideoInfoScanner::OnProcessSeriesFolder(
      EPISODELIST& files,
      const ADDON::ScraperPtr& scraper,
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

    std::unordered_map<std::string, int> episodeMap;
    for (const auto& file : files)
      episodeMap[file.strPath]++;

    int iMax = files.size();
    int iCurr = 1;
    for (EPISODELIST::iterator file = files.begin(); file != files.end(); ++file)
    {
      if (pDlgProgress)
      {
        pDlgProgress->SetLine(1, CVariant{20361}); // Loading episode details
        pDlgProgress->SetLine(
            2, StringUtils::Format(
                   "{} {}", CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(20373),
                   file->iSeason)); // Season x
        pDlgProgress->SetLine(
            3, StringUtils::Format(
                   "{} {}", CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(20359),
                   file->iEpisode)); // Episode y
        pDlgProgress->SetPercentage((int)((float)(iCurr++)/iMax*100));
        pDlgProgress->Progress();
      }
      if (m_handle)
        m_handle->SetPercentage(100.f*iCurr++/iMax);

      if ((pDlgProgress && pDlgProgress->IsCanceled()) || m_bStop)
        return InfoRet::CANCELLED;

      if (m_database.GetEpisodeId(file->strPath, file->iEpisode, file->iSeason) > -1)
      {
        if (m_handle)
          m_handle->SetText(
              CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(20415));
        continue;
      }

      CFileItem item;
      if (file->item)
        item = *file->item;
      else
      {
        item.SetPath(file->strPath);
        item.GetVideoInfoTag()->m_iSeason = file->iSeason;
        item.GetVideoInfoTag()->m_iEpisode = file->iEpisode;
      }

      // handle .nfo files
      InfoType result = InfoType::NONE;
      CScraperUrl scrUrl;
      const ScraperPtr& info(scraper);
      std::unique_ptr<IVideoInfoTagLoader> loader;
      if (useLocal)
        std::tie(result, loader) = ReadInfoTag(item, info, false, false);
      if (result == InfoType::FULL)
      {
        // override with episode and season number from file if available
        if (file->iEpisode > -1)
        {
          item.GetVideoInfoTag()->m_iEpisode = file->iEpisode;
          item.GetVideoInfoTag()->m_iSeason = file->iSeason;

          // Add flag if multi-episode file
          if (episodeMap[file->strPath] > 1)
            item.SetProperty(MULTIPLE_EPISODES, true);
        }
        if (AddVideo(&item, info, file->isFolder, true, &showInfo, false, ContentType::TVSHOWS) < 0)
          return InfoRet::INFO_ERROR;
        continue;
      }

      if (!hasEpisodeGuide)
      {
        // fetch episode guide
        if (!showInfo.m_strEpisodeGuide.empty() && scraper->ID() != "metadata.local")
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
            return InfoRet::NOT_FOUND;

          hasEpisodeGuide = true;
        }
      }

      if (episodes.empty())
      {
        CLog::Log(LOGERROR,
                  "VideoInfoScanner: Asked to lookup episode {}"
                  " online, but we have either no episode guide or"
                  " we are using the local scraper. Check your tvshow.nfo and make"
                  " sure the <episodeguide> tag is in place and/or use an online"
                  " scraper.",
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
        CFileItem scraperItem;
        scraperItem.SetPath(file->strPath);
        if (!imdb.GetEpisodeDetails(guide->cScraperUrl, *scraperItem.GetVideoInfoTag(),
                                    pDlgProgress))
          return InfoRet::NOT_FOUND; //! @todo should we just skip to the next episode?

        if (result == InfoType::COMBINED || result == InfoType::OVERRIDE)
          scraperItem.GetVideoInfoTag()->Merge(*item.GetVideoInfoTag());

        // Only set season/epnum from filename when it is not already set by a scraper
        if (scraperItem.GetVideoInfoTag()->m_iSeason == -1)
          scraperItem.GetVideoInfoTag()->m_iSeason = guide->iSeason;
        if (scraperItem.GetVideoInfoTag()->m_iEpisode == -1)
          scraperItem.GetVideoInfoTag()->m_iEpisode = guide->iEpisode;

        if (AddVideo(&scraperItem, info, file->isFolder, useLocal, &showInfo, false,
                     ContentType::TVSHOWS) < 0)
          return InfoRet::INFO_ERROR;
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
    return InfoRet::ADDED;
  }

  bool CVideoInfoScanner::GetDetails(CFileItem* pItem,
                                     const ADDON::CScraper::UniqueIDs& uniqueIDs,
                                     CScraperUrl& url,
                                     const ScraperPtr& scraper,
                                     IVideoInfoTagLoader* loader,
                                     CGUIDialogProgress* pDialog /* = NULL */)
  {
    CVideoInfoTag movieDetails;

    if (m_handle && !url.GetTitle().empty())
      m_handle->SetText(url.GetTitle());

    CVideoInfoDownloader imdb(scraper);
    bool ret = imdb.GetDetails(uniqueIDs, url, movieDetails, pDialog);

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
        const int64_t size{pItem->GetSize()};
        if (size)
          digest.Update(std::to_string(size));

        const CDateTime& dateTime{pItem->GetDateTime()};
        if (dateTime.IsValid())
        {
          digest.Update(StringUtils::Format("{:02}.{:02}.{:04}", dateTime.GetDay(),
                                            dateTime.GetMonth(), dateTime.GetYear()));
        }
      }
      else
      {
        const int64_t size{pItem->GetSize()};
        digest.Update(&size, sizeof(size));
        // linux and windows platform don't follow the same output format
        // (linux return a zero value for milliseconds member).
        // for consistency, use less precise format instead which discard
        // milliseconds value.
        // Unless a modification occur during the 1 second window when
        // kodi hash and update this particular file, we are safe.
        time_t tt{};
        pItem->GetDateTime().GetAsTime(tt);
        digest.Update(&tt, sizeof(tt));
      }
      if (IsVideo(*pItem) && !PLAYLIST::IsPlayList(*pItem) && !pItem->IsNFO())
        count++;
    }
    hash = digest.Finalize();
    return count;
  }

  bool CVideoInfoScanner::CanFastHash(const CFileItemList &items, const std::vector<std::string> &excludes) const
  {
    if (!m_advancedSettings->m_bVideoLibraryUseFastHash || items.IsPlugin())
      return false;

    for (int i = 0; i < items.Size(); ++i)
    {
      if (items[i]->IsFolder() && !CUtil::ExcludeFileOrFolder(items[i]->GetPath(), excludes))
        return false;
    }
    return true;
  }

  std::string CVideoInfoScanner::GetFastHash(const std::string &directory,
      const std::vector<std::string> &excludes) const
  {
    CDigest digest{CDigest::Type::MD5};

    if (!excludes.empty())
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

    if (!excludes.empty())
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

  void CVideoInfoScanner::GetSeasonThumbs(const CVideoInfoTag& show,
                                          KODI::ART::SeasonsArtwork& seasonArt,
                                          const std::vector<std::string>& artTypes,
                                          bool useLocal /* = true */,
                                          UseRemoteArtWithLocalScraper useRemoteArt /* = yes */)
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
        // Look for local art irrespective of scraper/existing art as it takes priority
        KODI::ART::Artwork art;
        std::string basePath;
        if (season == -1)
          basePath = "season-all";
        else if (season == 0)
          basePath = "season-specials";
        else
          basePath = StringUtils::Format("season{:02}", season);

        AddLocalItemArtwork(art, artTypes, URIUtils::AddFileToFolder(show.m_strPath, basePath),
                            addAll, exactName, false);

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
      KODI::ART::Artwork& art = seasonArt[url.m_season];
      if ((addAll || CVideoThumbLoader::IsArtTypeInWhitelist(aspect, artTypes, exactName)) &&
          !art.contains(aspect))
      {
        std::string image = CScraperUrl::GetThumbUrl(url);
        if (!image.empty() &&
            !(useRemoteArt == UseRemoteArtWithLocalScraper::NO && URIUtils::IsRemote(image)))
          art.insert(std::make_pair(aspect, image));
      }
    }
  }

  void CVideoInfoScanner::FetchActorThumbs(
      std::vector<SActorInfo>& actors,
      const std::string& strPath,
      UseRemoteArtWithLocalScraper useRemoteArt /* = YES */) const
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
          if (!items[j]->IsFolder() && compare == thumbFile)
          {
            i->thumb = items[j]->GetPath();
            break;
          }
        }
        if (!i->thumbUrl.GetFirstUrlByType().m_url.empty())
        {
          const std::string thumb{CScraperUrl::GetThumbUrl(i->thumbUrl.GetFirstUrlByType())};
          const bool notUsingThisRemoteArt{useRemoteArt == UseRemoteArtWithLocalScraper::NO &&
                                           URIUtils::IsRemote(thumb)};
          if (i->thumb.empty() && !notUsingThisRemoteArt)
            i->thumb = thumb;
          if (notUsingThisRemoteArt)
            i->thumbUrl.Clear();
        }
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
    if (returncode > 0 && !movielist.empty())
    {
      url = movielist[0];
      return 1;  // found a movie
    }
    return 0;    // didn't find anything
  }

  bool CVideoInfoScanner::AddVideoExtras(CFileItemList& items,
                                         ContentType content,
                                         const std::string& path)
  {
    int dbId = -1;

    // get the library item which was added previously with the specified content type
    for (const auto& item : items)
    {
      if (content == ContentType::MOVIES && !item->IsFolder())
      {
        dbId = m_database.GetMovieId(item->GetPath());
        if (dbId != -1)
        {
          break;
        }
      }
    }

    if (dbId == -1)
    {
      CLog::Log(LOGERROR, "VideoInfoScanner: Failed to find the library item for video extras {}",
                CURL::GetRedacted(path));
      return false;
    }

    // No need to check for .nomedia in the current directory, the caller already checked and this
    // function would not have been called if it existed.

    // Add video extras to library
    CDirectory::EnumerateDirectory(
        path,
        [this, content, dbId, path](const std::shared_ptr<CFileItem>& item)
        {
          const std::string extraTypeName =
              CGUIDialogVideoManagerExtras::GenerateVideoExtra(path, item->GetPath());

          const int idVideoAssetType = m_database.AddVideoVersionType(
              extraTypeName, VideoAssetTypeOwner::AUTO, VideoAssetType::EXTRA);

          // the video may have been added to the library as a movie earlier (different settings)
          const int idMovie{m_database.GetMovieId(item->GetPath())};

          if (idMovie <= 0)
          {
            if (CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
                    CSettings::SETTING_MYVIDEOS_EXTRACTFLAGS))
            {
              CDVDFileInfo::GetFileStreamDetails(item.get());
              CLog::Log(LOGDEBUG,
                        "VideoInfoScanner: Extracted filestream details from video file {}",
                        CURL::GetRedacted(item->GetPath()));
            }

            GetArtwork(item.get(), content, true, true, "");

            if (m_database.AddVideoAsset(ContentToVideoDbType(content), dbId, idVideoAssetType,
                                         VideoAssetType::EXTRA, *item.get()))
            {
              CLog::Log(LOGDEBUG, "VideoInfoScanner: Added video extra {}",
                        CURL::GetRedacted(item->GetPath()));
            }
            else
            {
              CLog::Log(LOGERROR, "VideoInfoScanner: Failed to add video extra {}",
                        CURL::GetRedacted(item->GetPath()));
            }
          }
          else
          {
            m_database.ConvertVideoToVersion(ContentToVideoDbType(content), idMovie, dbId,
                                             idVideoAssetType, VideoAssetType::EXTRA,
                                             DeleteMovieCascadeAction::ALL_ASSETS);
          }
        },
        [](const std::shared_ptr<CFileItem>& dirItem) { return !HasNoMedia(dirItem->GetPath()); },
        true, CServiceBroker::GetFileExtensionProvider().GetVideoExtensions(), DIR_FLAG_DEFAULTS);

    return true;
  }

  bool CVideoInfoScanner::ProcessVideoVersion(VideoDbContentType itemType, int dbId)
  {
    return CGUIDialogVideoManagerVersions::ProcessVideoVersion(itemType, dbId);
  }
}
