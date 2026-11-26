/*
 *  Copyright (C) 2005-2026 Team Kodi
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
#include "URL.h"
#include "Util.h"
#include "addons/AddonVersion.h"
#include "addons/Scraper.h"
#include "cores/VideoPlayer/DVDFileInfo.h"
#include "dialogs/GUIDialogExtendedProgressBar.h"
#include "dialogs/GUIDialogProgress.h"
#include "events/EventLog.h"
#include "events/MediaLibraryEvent.h"
#include "events/NotificationEvent.h"
#include "filesystem/Directory.h"
#include "filesystem/DirectoryFactory.h"
#include "filesystem/DiscDirectoryHelper.h"
#include "filesystem/File.h"
#include "filesystem/MultiPathDirectory.h"
#include "filesystem/PluginDirectory.h"
#include "filesystem/StackDirectory.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "interfaces/AnnouncementManager.h"
#include "interfaces/generic/ScriptRunner.h"
#include "messaging/helpers/DialogOKHelper.h"
#include "playlists/PlayListFileItemClassify.h"
#include "resources/LocalizeStrings.h"
#include "resources/ResourcesComponent.h"
#include "settings/AdvancedSettings.h"
#include "settings/MediaSourceSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "tags/VideoInfoTagLoaderFactory.h"
#include "tags/VideoTagLoaderNFO.h"
#include "utils/ArtTypes.h"
#include "utils/ArtUtils.h"
#include "utils/Digest.h"
#include "utils/DiscsUtils.h"
#include "utils/EpisodeUtils.h"
#include "utils/FileExtensionProvider.h"
#include "utils/ItemProperties.h"
#include "utils/RegExp.h"
#include "utils/ScopeGuard.h"
#include "utils/StreamDetails.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "utils/log.h"
#include "video/FilenameAttributes.h"
#include "video/VideoFileItemClassify.h"
#include "video/VideoInfoScannerLookups.h"
#include "video/VideoInfoTag.h"
#include "video/VideoLibrary.h"
#include "video/VideoManagerTypes.h"
#include "video/VideoThumbLoader.h"
#include "video/VideoUtils.h"
#include "video/dialogs/GUIDialogVideoManagerVersions.h"
#include "video/geometry/ContentGeometryScanner.h"
#include "video/geometry/GeometrySettings.h"

#include <algorithm>
#include <chrono>
#include <deque>
#include <iterator>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <ranges>
#include <set>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>

using namespace XFILE;
using namespace ADDON;
using namespace KODI::MESSAGING;
using namespace KODI;

using KODI::UTILITY::CDigest;

namespace
{
//! The most folders whose movies are looked up together, so that a first scan of a large source
//! doesn't list and hold every folder before its first movie is added
constexpr size_t FOLDERS_LOOKED_UP_TOGETHER{64};
//! Shows looked up ahead of the one being added, so that the scraper has calls to make while a
//! show's episodes are probed and added
constexpr size_t SHOWS_LOOKED_UP_AHEAD{8};
//! The savepoint a video of a batch is written after (CWriteBatch)
constexpr const char* SAVEPOINT{"video"};
//! How long a batch leaves the database to a writer that waited for its transaction, which tries
//! again every 100 ms
constexpr std::chrono::milliseconds LETTING_OTHERS_IN{120};

/*!
 * \brief Get the video extras folder directly below \p directory that \p path belongs to.
 *        A disc (BDMV/VIDEO_TS) in such a folder is listed as a playable file below it, so the
 *        whole path has to be considered and not only the name of the listed item.
 * \param[in] directory The directory being scanned. Must end with a directory separator
 * \param[in] path Path of an item listed in \p directory
 * \return Path of the video extras folder (ending with a directory separator), or empty if
 *         \p path doesn't belong to one
 */
std::string GetExtrasFolder(std::string_view directory, const std::string& path)
{
  std::string current{path};
  URIUtils::RemoveSlashAtEnd(current);

  // Walk up until the direct child of the scanned directory is reached
  while (!current.empty())
  {
    std::string parent{URIUtils::GetParentPath(current)};
    if (parent.empty() || URIUtils::PathEquals(parent, current, true))
      break;

    if (URIUtils::PathEquals(parent, std::string{directory}, true))
    {
      if (!VIDEO::IsVideoExtrasFolderName(URIUtils::GetFileOrFolderName(current)))
        break;

      URIUtils::AddSlashAtEnd(current);
      return current;
    }

    current = std::move(parent);
    URIUtils::RemoveSlashAtEnd(current);
  }
  return {};
}

//! Whether a folder is a show's disc structure folder holding the disc's index, below which are
//! only the disc's playlists and streams. An AVCHD disc's index is INDEX.BDM.
bool HoldsDiscIndex(const VIDEO::CDirectorySnapshot& snapshot)
{
  return VIDEO::IsDiscStructureFolder(URIUtils::GetFileOrFolderName(snapshot.Path())) &&
         (snapshot.Find("index.bdmv") || snapshot.Find("index.bdm"));
}

//! \brief The grouping of similar videos setting, for logging
const char* SimilarVideoScanActionToStr(SimilarVideoScanAction action)
{
  switch (action)
  {
    using enum SimilarVideoScanAction;
    case NONE:
      return "off";
    case ASK:
      return "ask";
    case AUTO:
      return "automatic";
    default:
      return "unknown";
  }
}

/*! \brief Retrieve the art type for an image from the given size.
 \param width the width of the image.
 \param height the height of the image.
 \return "poster" if the aspect ratio is at most 4:5, "banner" if the aspect ratio
         is at least 1:4, "thumb" otherwise.
 */

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
  : m_foldersLookedUpTogether(FOLDERS_LOOKED_UP_TOGETHER),
    m_advancedSettings(CServiceBroker::GetSettingsComponent()->GetAdvancedSettings())
{
  m_scanAll = false;
  // What a scan writes comes from NFO files, or the record an item had before
  m_database.SetAnnounceFileChanges(false);

  const auto settings = CServiceBroker::GetSettingsComponent()->GetSettings();

  m_similarVideoAction = static_cast<SimilarVideoScanAction>(
      settings->GetInt(CSettings::SETTING_VIDEOLIBRARY_SIMILARVIDEOACTION));
  m_ignoreVideoExtras = settings->GetBool(CSettings::SETTING_VIDEOLIBRARY_IGNOREVIDEOEXTRAS);
}

CVideoInfoScanner::~CVideoInfoScanner()
{
}

  void CVideoInfoScanner::Process()
  {
    // A scraper's call in flight is given up once the scan stops
    const CScriptRunner::CAbandonWhen abandonOnStop{m_bStop};
    const CScraperCalls::CInterpreter interpreter{m_scraperCalls};
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
        if (!m_bStop)
        {
          std::set<int> paths;
          m_database.CleanDatabase(m_handle, paths, false, &m_bStop);
        }

        if (m_handle)
          m_handle->MarkFinished();
        m_handle = NULL;

        m_bRunning = false;

        return;
      }

      auto start = std::chrono::steady_clock::now();
      const CScanProbe::CScanning scanning;

      m_database.Open();

      m_bCanInterrupt = true;

      CLog::Log(LOGINFO, "VideoInfoScanner: Starting scan .. (grouping of similar videos is {})",
                SimilarVideoScanActionToStr(m_similarVideoAction));
      CLog::Log(LOGINFO,
                "VideoInfoScanner: Looking videos up on {} threads, at most {} probing at once",
                LookUpThreads(), CScanProbe::ProbesAtOnce());
      CServiceBroker::GetAnnouncementManager()->Announce(
          ANNOUNCEMENT::VideoLibraryEvent{ANNOUNCEMENT::EVENT::LIBRARY::ScanStarted{}});

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
        m_progress.Start(m_pathsToScan, directory);
        if (m_bStop)
        {
          bCancelled = true;
        }
        else if (!CDirectory::Exists(directory))
        {
          /*
           * Note that this will skip clean (if m_bClean is enabled) for the directory and its
           * sub-directories if the directory really doesn't exist rather than a NAS being switched
           * off. A manual clean from settings will still pick up and remove it though.
           */
          CLog::LogF(LOGWARNING, "directory '{}' does not exist - skipping scan{}.",
                     CURL::GetRedacted(directory), m_bClean ? " and clean" : "");
          m_pathsToScan.erase(directory);
          m_progress.Finished();

          SkipRelatedDirectories(directory);
        }
        else
        {
          if ([[maybe_unused]] const auto [scanComplete, foundContent] = DoScan(directory);
              scanComplete == ScanComplete::Stopped)
          {
            bCancelled = true;
          }
          else
          {
            // Skip the individual scans of the sub directories still queued under the path to
            // scan. Not all of them are missing - disc folders are consumed by Stack() and
            // season folders are not recursed into.
            // Happens mostly for TV Shows that are in the library and were deleted from a sub
            // directory of a defined source.
            size_t missing{0};
            // Checking each folder is only worth its cost when the count is logged
            const bool countMissing{CServiceBroker::GetLogging().IsLogLevelLogged(LOGDEBUG)};
            auto f{[this, &missing, countMissing](const std::string& dir)
                   {
                     m_progress.Finished();
                     if (m_bClean)
                       m_pathsToClean.insert(m_database.GetPathId(dir));

                     // CPluginDirectory::Exists always says yes, so treat a plugin path as missing
                     if (!countMissing || (!URIUtils::IsPlugin(dir) && CDirectory::Exists(dir)))
                       return;

                     ++missing;
                   }};

            RemoveSubDirectories(m_pathsToScan, directory, f);
            if (missing)
              CLog::Log(LOGDEBUG, "VideoInfoScanner: Skipped {} missing sub directories of {}.",
                        missing, directory);
          }
        }
      }

      m_discReadAhead.Stop();

      if (!bCancelled)
      {
        // An empty set cleans the whole library, which a scan of part of it must not do
        if (m_bClean && !(m_scanSubtree && m_pathsToClean.empty()))
          m_database.CleanDatabase(m_handle, m_pathsToClean, false, &m_bStop);
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

      // A scan stopped, by Stop() or the user's answer to a failed search, is no finished one
      if (m_bStop)
        CLog::Log(LOGINFO, "VideoInfoScanner: Stopped scan. Scanning for video info took {} ms",
                  duration.count());
      else
        CLog::Log(LOGINFO, "VideoInfoScanner: Finished scan. Scanning for video info took {} ms",
                  duration.count());
      ReportSummary();

      // Deliberately after the scan is timed and the library is browsable, so that fetching the
      // art doesn't hold either back
      m_art.FlushDeferred();
    }
    catch (...)
    {
      CLog::Log(LOGERROR, "VideoInfoScanner: Exception while scanning.");
    }

    m_bRunning = false;
    CServiceBroker::GetAnnouncementManager()->Announce(
        ANNOUNCEMENT::VideoLibraryEvent{ANNOUNCEMENT::EVENT::LIBRARY::ScanFinished{}});

    if (m_handle)
      m_handle->MarkFinished();
    m_handle = NULL;
  }

  void CVideoInfoScanner::Start(const std::string& strDirectory, bool scanAll)
  {
    m_scanAll = scanAll;
    m_scanSubtree = !strDirectory.empty();
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

    if (!m_bStop)
      CLog::Log(LOGINFO, "VideoInfoScanner: Stopping the scan");
    m_bStop = true;
    m_scraperCalls.CancelAll();
    m_discReadAhead.Abandon();
  }

  void CVideoInfoScanner::CScraperCalls::CancelAll()
  {
    {
      std::unique_lock lock(m_lock);
      m_cancelled = true;
    }
    m_freed.notify_all();
  }

  size_t CVideoInfoScanner::CScraperCalls::Waiting()
  {
    std::unique_lock lock(m_lock);
    return m_waiting.size();
  }

  CVideoInfoScanner::CScraperCalls::CRegistration::CRegistration(CScraperCalls& calls)
    : m_scraperCalls(calls),
      m_interpreter(calls)
  {
    std::unique_lock lock(m_scraperCalls.m_lock);
    const size_t place{CVideoLookups::Place()};
    const auto waiting{m_scraperCalls.m_waiting.insert(place)};
    m_scraperCalls.m_freed.wait(lock,
                                [this, place]
                                {
                                  return (!m_scraperCalls.m_calling &&
                                          place <= *m_scraperCalls.m_waiting.begin()) ||
                                         m_scraperCalls.m_cancelled;
                                });
    m_scraperCalls.m_waiting.erase(waiting);
    if (!m_scraperCalls.m_cancelled)
    {
      m_scraperCalls.m_calling = true;
      m_placed = true;
    }
  }

  CVideoInfoScanner::CScraperCalls::CRegistration::~CRegistration()
  {
    {
      std::unique_lock lock(m_scraperCalls.m_lock);
      if (m_placed)
        m_scraperCalls.m_calling = false;
    }
    // All, as only the call next in line may go
    m_scraperCalls.m_freed.notify_all();
  }

  std::pair<CInfoScanner::ScanComplete, CInfoScanner::ContentFound> CVideoInfoScanner::DoScan(
      const std::string& strDirectory)
  {
    // What the scan read of this folder and of those below it is of no use once it has left them
    const KODI::UTILS::CScopeGuard<CVideoInfoScanner*, nullptr, void(CVideoInfoScanner*)> leaving(
        [&strDirectory](CVideoInfoScanner* scanner)
        {
          scanner->ForgetListings(strDirectory);
          scanner->ForgetLookUps(strDirectory);
        },
        this);

    const std::unique_ptr<DirectoryScan> scan{PlanDirectory(strDirectory)};
    if (!scan)
    {
      FolderFinished();
      return std::make_pair(ScanComplete::Completed, ContentFound::None);
    }

    RetrieveDirectory(*scan);
    return FinishDirectory(*scan);
  }

  std::unique_ptr<CVideoInfoScanner::DirectoryScan> CVideoInfoScanner::PlanDirectory(
      const std::string& strDirectory)
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
    m_pathsToScan.erase(strDirectory);

    m_pathTable->Load(strDirectory);

    // load subfolder
    auto scan{std::make_unique<DirectoryScan>()};
    scan->path = strDirectory;
    CFileItemList& items{scan->items};
    bool& foundDirectly{scan->foundDirectly};
    bool& bSkip{scan->skip};

    SScanSettings& settings{scan->settings};
    ScraperPtr& info{scan->info};
    info = ScraperForPath(strDirectory, settings, foundDirectly);
    ContentType& content{scan->content};
    content = info ? info->Content() : ContentType::NONE;

    // exclude folders that match our exclude regexps
    const std::vector<std::string>& regexps =
        content == ContentType::TVSHOWS ? m_advancedSettings->m_tvshowExcludeFromScanRegExps
                                        : m_advancedSettings->m_moviesExcludeFromScanRegExps;

    if (CUtil::ExcludeFileOrFolder(strDirectory, regexps, &m_regexpCache))
      return nullptr;

    const bool isPlugin{scan->isPlugin = URIUtils::IsPlugin(strDirectory)};

    bool ignoreFolder = !m_scanAll && settings.noupdate;
    if (content == ContentType::NONE || ignoreFolder)
      return nullptr;

    // A video extras folder (eg. "Extras", "Bonus Disc") holds no movie of its own. Skip it and
    // anything below it when video extras are ignored.
    // A path with content set on it (a source root) is never skipped.
    if (m_ignoreVideoExtras && content == ContentType::MOVIES && !foundDirectly &&
        IsVideoExtrasFolderName(URIUtils::GetFileOrFolderName(strDirectory)))
    {
      CLog::Log(LOGDEBUG, "VideoInfoScanner: Skipping extras dir '{}'",
                CURL::GetRedacted(strDirectory));
      m_progress.Finished(RemoveSubDirectories(m_pathsToScan, strDirectory, {}));
      return nullptr;
    }

    if (isPlugin &&
        !CPluginDirectory::IsMediaLibraryScanningAllowed(TranslateContent(content), strDirectory))
    {
      CLog::Log(
          LOGINFO,
          "VideoInfoScanner: Plugin '{}' does not support media library scanning for '{}' content",
          CURL::GetRedacted(strDirectory), content);
      return nullptr;
    }

    // An archive (eg. rar://) has no record of its own: it is a file of the folder holding it,
    // which is listed again whenever the archive changes
    const bool isArchive{scan->isArchive = URIUtils::IsArchive(CURL(strDirectory))};
    const bool fastHash{m_advancedSettings->m_bVideoLibraryUseFastHash};
    const std::string cfg{FolderSettings(strDirectory)};
    const std::optional<FolderRecord> record{isArchive ? std::nullopt
                                                       : m_pathTable->Record(strDirectory)};
    std::optional<FolderRecord>& newRecord{scan->newRecord};

    if (content == ContentType::MOVIES || content == ContentType::MUSICVIDEOS)
    {
      if (m_handle)
      {
        int str = content == ContentType::MOVIES ? 20317 : 20318;
        m_handle->SetTitle(StringUtils::Format(
            CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(str), info->Name(),
            SourceName(strDirectory)));
      }

      bool unchanged{false};
      if (isPlugin)
      {
        CDirectory::GetDirectory(strDirectory, items,
                                 CServiceBroker::GetFileExtensionProvider().GetVideoExtensions(),
                                 DIR_FLAG_DEFAULTS);
        items.Stack();

        // force sorting consistency to avoid hash mismatch between platforms
        // sort by filename as always present for any files, but keep case sensitivity
        items.Sort(SortBy::FILE, SortOrder::ASCENDING, SortAttributeNone);

        std::string hash;
        UTILS::GetPathHash(items, hash);
        if (!hash.empty())
        {
          newRecord = FolderRecord{.cfg = cfg, .mtime = std::nullopt, .digest = hash};
          unchanged = record && record->UnchangedListing(cfg, hash);
        }
      }
      else
      {
        const std::optional<int64_t> time{FolderTime(strDirectory)};
        if (record && record->UnchangedUnlisted(cfg, time, fastHash))
        {
          // Nothing below it is looked at either, as it holds no folder to scan
          CLog::Log(LOGDEBUG, "VideoInfoScanner: Skipping dir '{}' due to no change (fasthash)",
                    CURL::GetRedacted(strDirectory));
          ++m_summary.unchangedDirectories;
          return nullptr;
        }

        if (const auto snapshot{Snapshot(strDirectory)}; snapshot)
        {
          if (snapshot->HasNoMedia())
          {
            if (!isArchive)
              m_pathTable->Store(strDirectory, FolderRecord{.cfg = cfg,
                                                            .mtime = time,
                                                            .digest = snapshot->Digest(),
                                                            .noMedia = true});
            return nullptr;
          }

          newRecord = FolderRecord{.cfg = cfg,
                                   .mtime = time,
                                   .digest = snapshot->Digest(),
                                   .hasSubfolders = HasScannedSubfolders(*snapshot, regexps)};
          unchanged = record && record->UnchangedListing(cfg, snapshot->Digest());

          // An archive is looked inside only once it, or another file beside it, has changed
          snapshot->GetItems(items, CServiceBroker::GetFileExtensionProvider().GetVideoExtensions(),
                             !unchanged);

          // mark subfolders whose record shows them unchanged since their parent listed them;
          // the disc structure probes in Stack() and the recursion loop (which also drops them
          // from m_pathsToScan) skip marked items. A folder that has folders of its own to scan
          // is never marked, so is still visited.
          std::vector<CRegExp> stackRegExps{m_advancedSettings->m_folderStackRegExps};
          for (int i = items.Size() - 1; i >= 0; --i)
          {
            if (!items[i]->IsFolder())
              continue;
            const std::string& subfolder{items[i]->GetPath()};
            if (URIUtils::IsArchive(CURL(subfolder)))
              continue;
            // An extras folder is decided by its own time, which this listing may give late
            if (auto& time{m_listedTimes[subfolder]}; !time && !IsVideoExtrasFolder(*items[i]))
              time = ListedModifiedTime(*items[i]);

            // a marked folder-stack member (label like "cd1") stays a folder in
            // Stack() and its stack cannot form; leave such folders unmarked
            std::string label = StringUtils::ToLower(items[i]->GetLabel());
            URIUtils::RemoveSlashAtEnd(label);
            const bool stackMember{std::ranges::any_of(stackRegExps, [&label](CRegExp& re)
                                                       { return re.RegFind(label) != -1; })};
            if (!stackMember && FolderUnchangedUnlisted(subfolder))
              items[i]->SetProperty(PROPERTY_UNCHANGED, true);
            else if (CUtil::ExcludeFileOrFolder(subfolder, regexps, &m_regexpCache))
              items[i]->SetProperty(PROPERTY_DISC_PATH, "");
            // Listed now rather than probed by Stack(), as it is about to be scanned or stacked
            else if (FolderHasNoMedia(subfolder, true))
              items.Remove(i);
            else if (const auto listing{Snapshot(subfolder)}; listing)
              items[i]->SetProperty(PROPERTY_DISC_PATH, listing->GetDiscPath());
          }
        }
        else
          CLog::Log(LOGDEBUG, "VideoInfoScanner: Unable to list dir '{}'",
                    CURL::GetRedacted(strDirectory));

        items.Stack();

        // force sorting consistency to avoid hash mismatch between platforms
        // sort by filename as always present for any files, but keep case sensitivity
        items.Sort(SortBy::FILE, SortOrder::ASCENDING, SortAttributeNone);
      }

      if (!newRecord)
      { // directory empty or non-existent - add to clean list and skip
        CLog::Log(LOGDEBUG,
                  "VideoInfoScanner: Skipping dir '{}' as it's empty or doesn't exist - adding to "
                  "clean list",
                  CURL::GetRedacted(strDirectory));
        if (m_bClean)
          AddPathToClean(strDirectory);
        bSkip = true;
        // Its record would otherwise let a later scan pass it over by its time alone
        if (record && !isArchive)
          m_pathTable->Clear(strDirectory);
      }
      else if (unchanged)
      { // listing matches - skipping
        CLog::Log(LOGDEBUG, "VideoInfoScanner: Skipping dir '{}' due to no change",
                  CURL::GetRedacted(strDirectory));
        bSkip = true;
        ++m_summary.unchangedDirectories;
        // Only the time it was listed with has moved
        if (!isArchive && record->mtime != newRecord->mtime)
          m_pathTable->Store(strDirectory, *newRecord);
      }
      else if (!record)
      { // new folder - scan
        CLog::Log(LOGDEBUG, "VideoInfoScanner: Scanning dir '{}' as not in the database",
                  CURL::GetRedacted(strDirectory));
      }
      else
      { // listing changed - rescan
        CLog::Log(LOGDEBUG, "VideoInfoScanner: Rescanning dir '{}' due to change",
                  CURL::GetRedacted(strDirectory));
      }
    }
    else if (content == ContentType::TVSHOWS)
    {
      if (m_handle)
        m_handle->SetTitle(StringUtils::Format(
            CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(20319), info->Name(),
            SourceName(strDirectory)));

      if (foundDirectly && !settings.parent_name_root)
      {
        // Each show is a folder of the source, which decides from its own record whether it has
        // changed, so the source's listing is always looked at
        if (isPlugin)
          CDirectory::GetDirectory(strDirectory, items,
                                   CServiceBroker::GetFileExtensionProvider().GetVideoExtensions(),
                                   DIR_FLAG_DEFAULTS);
        else if (const auto snapshot{Snapshot(strDirectory)}; snapshot)
        {
          if (snapshot->HasNoMedia())
            return nullptr;

          newRecord = FolderRecord{.cfg = cfg,
                                   .mtime = FolderTime(strDirectory),
                                   .digest = snapshot->Digest(),
                                   .hasSubfolders = HasScannedSubfolders(*snapshot, {})};
          snapshot->GetItems(items, CServiceBroker::GetFileExtensionProvider().GetVideoExtensions(),
                             true);
        }
        items.SetPath(strDirectory);

        // force sorting consistency to avoid hash mismatch between platforms
        // sort by filename as always present for any files, but keep case sensitivity
        items.Sort(SortBy::FILE, SortOrder::ASCENDING, SortAttributeNone);
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

    // Drop everything belonging to a video extras folder (eg. "Extras", "Bonus Disc") when video
    // extras are ignored. Stack() has already turned such a folder into a playable file item when
    // it holds a disc structure (BDMV/VIDEO_TS), so the path of the item is matched and not only
    // its name. Done after the listing was recorded, so that the record still describes all of it.
    if (m_ignoreVideoExtras && content == ContentType::MOVIES)
    {
      for (int i = items.Size() - 1; i >= 0; --i)
      {
        const std::string extrasFolder{GetExtrasFolder(strDirectory, items[i]->GetPath())};
        if (extrasFolder.empty())
          continue;

        // Leave a folder with content set on it (a source root) alone
        if (IsContentRoot(extrasFolder))
          continue;

        CLog::Log(LOGDEBUG, "VideoInfoScanner: Ignoring extras item '{}'",
                  CURL::GetRedacted(items[i]->GetPath()));
        m_progress.Finished(RemoveSubDirectories(m_pathsToScan, extrasFolder, {}));
        items.Remove(i);
      }
    }

    // Video extras folders whose movie is found once this listing has been scanned. Stack() lists
    // an extras folder holding a disc structure as the disc's file, which is taken out here rather
    // than scraped as a movie.
    if (!m_ignoreVideoExtras && content == ContentType::MOVIES && settings.parent_name)
    {
      for (int i = items.Size() - 1; i >= 0; --i)
      {
        if (items[i]->IsFolder())
          continue;

        const std::string extrasFolder{GetExtrasFolder(strDirectory, items[i]->GetPath())};
        if (extrasFolder.empty())
          continue;

        if (IsContentRoot(extrasFolder))
          continue;

        RemoveExtrasFolder(extrasFolder);
        AddExtrasFolder(*scan, extrasFolder);
        items.Remove(i);
      }
    }

    CountFoldersFound(*scan);
    return scan;
  }

  void CVideoInfoScanner::CountFoldersFound(DirectoryScan& scan)
  {
    if (scan.content != ContentType::TVSHOWS && scan.settings.recurse <= 0)
      return;

    size_t listed{0};
    size_t counted{0};
    for (const auto& item : scan.items)
    {
      const std::string& path{item->GetPath()};
      if (!item->IsParentFolder())
        ++listed;
      if (!item->IsFolder() || item->IsParentFolder() || PLAYLIST::IsPlayList(*item) ||
          path == scan.path)
        continue;
      // An extras folder is added with its movie, rather than scanned
      if (scan.content == ContentType::MOVIES && scan.settings.parent_name &&
          !m_ignoreVideoExtras && IsVideoExtrasFolder(*item))
        continue;

      ++counted;
      if (!m_pathsToScan.contains(path))
        m_progress.Found();
      if (scan.content == ContentType::TVSHOWS)
        m_progressShows.insert(path);
    }

    // A tv source's folders are shows, which it looks up itself
    if (scan.content != ContentType::TVSHOWS && counted > 0 && counted == listed)
    {
      scan.leftOut = true;
      m_progress.LeaveOut();
    }
  }

  void CVideoInfoScanner::FolderFinished()
  {
    m_progress.Finished();
    ReportProgress();
  }

  void CVideoInfoScanner::ReportProgress(float share /* = 0.f */)
  {
    const float percentage{m_progress.Percentage(share)};
    if (m_handle)
      m_handle->SetPercentage(percentage);
  }

  void CVideoInfoScanner::RetrieveDirectory(DirectoryScan& scan)
  {
    if (scan.skip)
      return;

    // parent_name_root, passed as bDirNames below, is false for a source's own root even when
    // folder names are used
    m_useFolderNames = scan.settings.parent_name;

    const unsigned int failedLookups{m_failedLookups};
    m_database.Open();
    ItemLookups batch;
    SetUpLookups(scan, batch);
    PlanLookups(batch);
    ReadDiscsAhead(batch);
    const size_t window{scan.content == ContentType::TVSHOWS ? SHOWS_LOOKED_UP_AHEAD + 1 : 0};
    CVideoLookups::Run(
        static_cast<size_t>(scan.items.Size()), LookUpThreads(),
        [this, &batch](size_t i) { LookUpItem(batch, i, i); },
        [this, &batch](size_t i) { return HandBackItem(batch, i); }, m_bStop, window);
    EndLookups(batch);
    m_database.Close();
    EndRetrieve(scan, batch.foundSomeInfo, failedLookups);
  }

  void CVideoInfoScanner::SetUpLookups(const DirectoryScan& scan, ItemLookups& batch)
  {
    batch.items = &scan.items;
    batch.bDirNames = scan.settings.parent_name_root;
    batch.content = scan.content;
    batch.useFolderNames = scan.settings.parent_name;
  }

  void CVideoInfoScanner::EndRetrieve(DirectoryScan& scan,
                                      bool foundSomething,
                                      unsigned int failedLookups)
  {
    const std::string& strDirectory{scan.path};
    const ContentType content{scan.content};
    scan.foundSomething = foundSomething;
    // The directory's record stands for its own items and the archives in it, so it isn't stored
    // where a lookup failed in either, and the next scan tries them again
    scan.lookupFailed = m_failedLookups != failedLookups;
    if (foundSomething)
    {
      if (!m_bStop && (content == ContentType::MOVIES || content == ContentType::MUSICVIDEOS))
      {
        if (m_bClean)
          AddPathToClean(strDirectory);
        CLog::Log(LOGDEBUG, "VideoInfoScanner: Finished adding information from dir {}",
                  CURL::GetRedacted(strDirectory));
      }
    }
    else
    {
      if (m_bClean)
        AddPathToClean(strDirectory);
      CLog::Log(LOGDEBUG, "VideoInfoScanner: No (new) information was found in dir {}",
                CURL::GetRedacted(strDirectory));
    }
  }

  std::pair<CInfoScanner::ScanComplete, CInfoScanner::ContentFound> CVideoInfoScanner::
      FinishDirectory(DirectoryScan& scan)
  {
    const std::string& strDirectory{scan.path};
    CFileItemList& items{scan.items};
    const SScanSettings& settings{scan.settings};
    const ContentType content{scan.content};
    const bool isPlugin{scan.isPlugin};
    const bool isArchive{scan.isArchive};
    const bool bSkip{scan.skip};
    const bool fastHash{m_advancedSettings->m_bVideoLibraryUseFastHash};
    const std::optional<FolderRecord>& newRecord{scan.newRecord};
    std::vector<std::string>& extrasFolders{scan.extrasFolders};
    bool& extrasAddedAgain{scan.extrasAddedAgain};
    const bool foundSomething{scan.foundSomething};
    bool& lookupFailed{scan.lookupFailed};

    if (m_handle)
      OnDirectoryScanned(strDirectory);

    bool foundSomethingInArchive = false;
    // Folders beside each other, planned and waiting to be looked up together
    std::vector<std::unique_ptr<DirectoryScan>> queued;
    // The batches of them being looked up, in the order they are added, and the run looking them
    // up, made with the first. Declared after the batches, so that its look-ups end first.
    std::deque<AlongsideBatch> batches;
    std::optional<CVideoLookups::CRun> run;
    // The folders queued are looked up as a batch after those before them, and the batches but the
    // last \p keep are added. A folder being added that holds folders scans them in a run of its own,
    // as those of this run wait for it.
    const auto lookUpQueued{[this, &queued, &batches, &run](size_t keep)
                            {
                              if (!queued.empty())
                              {
                                if (!run)
                                  run.emplace(LookUpThreads(), m_bStop);
                                AlongsideBatch& batch{batches.emplace_back()};
                                batch.folders = std::move(queued);
                                queued.clear();
                                AddAlongside(*run, batch);
                              }
                              while (batches.size() > keep)
                              {
                                if (!HandBackAlongside(*run, batches.front()))
                                {
                                  // The run looks up no more, so none of its batches is added
                                  run.reset();
                                  batches.clear();
                                  return;
                                }
                                batches.pop_front();
                              }
                            }};
    for (int i = 0; i < items.Size(); ++i)
    {
      CFileItemPtr pItem = items[i];

      if (m_bStop)
        break;

      // add video extras to library. An extras folder holds no movie of its own, so it is never
      // scanned for one.
      const bool addAsExtras{AddsAsExtras(scan, *pItem)};

      // A movie folder is scanned with the others beside it, its movie looked up alongside theirs.
      // Anything else waits for them, so that the library is written to in the same order.
      const bool recurse{content != ContentType::TVSHOWS && settings.recurse > 0 &&
                         pItem->IsFolder() && !pItem->IsParentFolder() &&
                         !PLAYLIST::IsPlayList(*pItem)};
      const bool unchangedFolder{recurse && pItem->GetProperty(PROPERTY_UNCHANGED).asBoolean()};
      const bool alongside{IsScannedAlongside(scan, *pItem, addAsExtras)};
      const bool plainFile{!pItem->IsFolder() && !URIUtils::IsOpticalMediaFile(pItem->GetPath())};
      if (!alongside && !unchangedFolder && !plainFile && (!queued.empty() || !batches.empty()))
      {
        lookUpQueued(0);
        if (m_bStop)
          break;
      }

      if (addAsExtras)
      {
        RemoveExtrasFolder(pItem->GetPath());
        // Added once the folders beside it (eg. Disc 1, Disc 2) are scanned, to the one movie there
        AddExtrasFolder(scan, pItem->GetPath());

        // no further processing required
        continue;
      }

      // Disc rips are anchored in files/path several ways: BD subfolder rips
      // under the raw BDMV/ row (older imports or failed playlist detection)
      // or under a bluray:// playlist row (current imports); DVD rips under
      // VIDEO_TS/; flat rips with the structure file directly in the movie
      // folder. In every form the movie folder itself, the item the scanner
      // lists, has no row of its own, so each parent rescan would re-import the
      // movie from NFO. Record the folder here; GetMovieId resolves all anchor
      // forms. ISOs are recorded as plain files of their folder and never reach this block.
      if (content == ContentType::MOVIES && !isPlugin && !pItem->IsFolder() &&
          !URIUtils::IsStack(pItem->GetPath()) && URIUtils::IsOpticalMediaFile(pItem->GetPath()))
      {
        std::string discFolder = URIUtils::RemoveDiscPath(pItem->GetPath());
        URIUtils::AddSlashAtEnd(discFolder);
        if (!URIUtils::PathEquals(discFolder, strDirectory, true))
        {
          const std::string discCfg{FolderSettings(discFolder)};
          const std::optional<FolderRecord> discRecord{m_pathTable->Record(discFolder)};
          const std::optional<int64_t> discTime{FolderTime(discFolder)};
          std::shared_ptr<const CDirectorySnapshot> discListing;
          bool unchanged{discRecord && discRecord->UnchangedUnlisted(discCfg, discTime, fastHash)};
          if (!unchanged)
          {
            discListing = Snapshot(discFolder);
            unchanged = discRecord && discListing &&
                        discRecord->UnchangedListing(discCfg, discListing->Digest());
          }

          // An extra deleted beside the disc changes its extras folder, or the disc's folder
          bool extrasAdded{false};
          if (settings.parent_name && !m_ignoreVideoExtras && discListing)
            extrasAdded = AddExtrasBesideDisc(*discListing);
          if ((extrasAdded || !unchanged) && m_bClean)
            AddPathToClean(discFolder);

          if (!unchanged && discListing && m_database.HasMovieInfo(pItem->GetDynPath()))
            m_pathTable->Store(
                discFolder, FolderRecord{.cfg = discCfg,
                                         .mtime = discTime,
                                         .digest = discListing->Digest(),
                                         .hasSubfolders = HasScannedSubfolders(
                                             *discListing,
                                             m_advancedSettings->m_moviesExcludeFromScanRegExps)});
        }
      }

      // if we have a directory item (non-playlist) we then recurse into that folder
      // do not recurse for tv shows - we have already looked recursively for episodes
      if (recurse)
      {
        if (unchangedFolder)
        {
          CLog::Log(LOGDEBUG, "VideoInfoScanner: Skipping dir '{}' due to no change (fasthash)",
                    CURL::GetRedacted(pItem->GetPath()));
          m_pathsToScan.erase(pItem->GetPath());
          ++m_summary.unchangedDirectories;
          FolderFinished();
          continue;
        }
        if (alongside)
        {
          std::unique_ptr<DirectoryScan> subfolder;
          const auto planned{std::ranges::find_if(scan.plannedAhead, [&pItem](const auto& folder)
                                                  { return folder.first == pItem->GetPath(); })};
          if (planned != scan.plannedAhead.end())
            subfolder = std::move(planned->second);
          else
            subfolder = PlanDirectory(pItem->GetPath());
          if (!subfolder)
          {
            ForgetListings(pItem->GetPath());
            FolderFinished();
            continue;
          }
          if (CanScanAlongside(*subfolder))
          {
            // One planned ahead had the folders it holds planned with it
            if (subfolder->plannedAhead.empty())
            {
              size_t budget{m_foldersLookedUpTogether};
              PlanAhead(*subfolder, budget);
            }
            queued.emplace_back(std::move(subfolder));
            // Bounded, so that a first scan of a large source doesn't list and hold every folder
            // before its first movie is added
            size_t planned{0};
            for (const auto& folder : queued)
              planned += CountPlanned(*folder);
            if (planned >= m_foldersLookedUpTogether)
            {
              // Looked up while the batch before them is added, so that the scraper has their
              // calls to make meanwhile
              lookUpQueued(1);
              if (m_bStop)
                break;
            }
            continue;
          }

          // Scanned as DoScan() does, once the folders before it are
          if (!queued.empty() || !batches.empty())
          {
            lookUpQueued(0);
            if (m_bStop)
            {
              ForgetListings(pItem->GetPath());
              break;
            }
          }
          RetrieveDirectory(*subfolder);
          const auto [scanComplete, foundContent] = FinishDirectory(*subfolder);
          ForgetListings(pItem->GetPath());
          if (scanComplete == ScanComplete::Stopped)
            m_bStop = true;
          continue;
        }

        const unsigned int failedLookups{m_failedLookups};
        if (const auto [scanComplete, foundContentOnRecursion] = DoScan(pItem->GetPath());
            scanComplete == ScanComplete::Stopped)
        {
          m_bStop = true;
        }
        // If this subdirectory is an archive and content was found inside it,
        // store the hash for the physical parent directory instead of the archive (eg. rar://) path.
        // This ensures change detection works correctly on the real filesystem path.
        else if (!foundSomething && !m_bStop &&
                 foundContentOnRecursion == ContentFound::NewContentFound &&
                 (content == ContentType::MOVIES || content == ContentType::MUSICVIDEOS) &&
                 URIUtils::IsArchive(CURL(pItem->GetPath())))
        {
          foundSomethingInArchive = true;
        }
        if (URIUtils::IsArchive(CURL(pItem->GetPath())) && m_failedLookups != failedLookups)
          lookupFailed = true;
      }
    }

    lookUpQueued(0);
    run.reset();

    for (const std::string& extrasFolder : extrasFolders)
    {
      if (m_bStop)
        break;

      const CDiscReadAhead::CTaken discs{m_discReadAhead, extrasFolder};
      const int dbId{m_database.GetMovieIdInFolder(strDirectory, extrasFolder)};
      if (dbId < 0)
      {
        CLog::Log(LOGDEBUG, "VideoInfoScanner: No single movie found for video extras {}",
                  CURL::GetRedacted(extrasFolder));
        continue;
      }

      // Recorded only once all of it could be listed, so that a later scan adds what couldn't be
      if (m_extras.AddVideoExtras(dbId, extrasFolder))
        StoreFolderRecords(extrasFolder, {});
      CLog::Log(LOGDEBUG, "VideoInfoScanner: Finished adding video extras from dir {}",
                CURL::GetRedacted(extrasFolder));
    }

    if (extrasAddedAgain && m_bClean)
      AddPathToClean(strDirectory);

    // Content found directly or in an archive subfolder is recorded by the physical directory,
    // rather than by the archive's (eg. rar://) virtual path, once every lookup of it has been
    // made. A listing of nothing but folders has nothing of its own to find, but is recorded only
    // where a later scan lists it: passed over by its time, an archive it holds would not be looked
    // in again for a movie not found there, or changed inside it. An empty one isn't recorded where
    // an update cleans, so that a scan stopped before its clean leaves the folder to the next.
    const bool onlyFolders{
        std::ranges::all_of(items, [](const auto& item) { return item->IsFolder(); }) &&
        ((items.IsEmpty() && !m_bClean) || !newRecord || newRecord->hasSubfolders ||
         !newRecord->mtime)};
    if (!bSkip && !m_bStop && !lookupFailed && newRecord && !isArchive &&
        (foundSomething || foundSomethingInArchive || onlyFolders ||
         content == ContentType::TVSHOWS))
    {
      m_pathTable->Store(strDirectory, *newRecord);
      if (foundSomethingInArchive)
        CLog::LogF(LOGDEBUG,
                   "Recorded physical dir '{}' after finding content in archive subfolder",
                   CURL::GetRedacted(strDirectory));
    }

    if (!scan.leftOut)
      FolderFinished();
    return std::make_pair(m_bStop ? ScanComplete::Stopped : ScanComplete::Completed,
                          foundSomething || foundSomethingInArchive ? ContentFound::NewContentFound
                                                                    : ContentFound::None);
  }

  bool CVideoInfoScanner::CanScanAlongside(const DirectoryScan& scan) const
  {
    // One holding folders to scan scans them as it finishes, in its turn
    return scan.content == ContentType::MOVIES && !scan.isPlugin && !scan.isArchive;
  }

  bool CVideoInfoScanner::AddsAsExtras(const DirectoryScan& scan, const CFileItem& item)
  {
    if (scan.content != ContentType::MOVIES || !scan.settings.parent_name || m_ignoreVideoExtras ||
        !IsVideoExtrasFolder(item))
      return false;
    // One that is a source of its own is scanned for movies
    return !IsContentRoot(item.GetPath());
  }

  bool CVideoInfoScanner::IsScannedAlongside(const DirectoryScan& scan,
                                             const CFileItem& item,
                                             bool addAsExtras) const
  {
    return scan.content == ContentType::MOVIES && scan.settings.recurse > 0 && item.IsFolder() &&
           !item.IsParentFolder() && !PLAYLIST::IsPlayList(item) && !addAsExtras &&
           !item.GetProperty(PROPERTY_UNCHANGED).asBoolean() &&
           !URIUtils::IsArchive(CURL(item.GetPath()));
  }

  void CVideoInfoScanner::PlanAhead(DirectoryScan& scan, size_t& budget)
  {
    std::vector<std::string> subfolders;
    for (const auto& item : scan.items)
    {
      if (IsScannedAlongside(scan, *item, AddsAsExtras(scan, *item)))
        subfolders.emplace_back(item->GetPath());
    }
    // One holding many, as a collection does, scans them in batches of its own, as does one
    // whose folders and those below them would make more than a batch
    if (subfolders.size() > 8 || subfolders.size() > budget)
      return;
    budget -= subfolders.size();

    for (std::string& path : subfolders)
    {
      std::unique_ptr<DirectoryScan> subfolder{PlanDirectory(path)};
      if (subfolder && CanScanAlongside(*subfolder))
        PlanAhead(*subfolder, budget);
      scan.plannedAhead.emplace_back(std::move(path), std::move(subfolder));
    }
  }

  size_t CVideoInfoScanner::CountPlanned(const DirectoryScan& scan)
  {
    size_t count{1};
    for (const auto& [path, planned] : scan.plannedAhead)
    {
      if (planned)
        count += CountPlanned(*planned);
    }
    return count;
  }

  void CVideoInfoScanner::AddAlongside(CVideoLookups::CRun& run, AlongsideBatch& batch)
  {
    using Entry = AlongsideBatch::Entry;
    std::vector<std::unique_ptr<DirectoryScan>>& folders{batch.folders};
    std::vector<Entry>& entries{batch.entries};
    const auto setUp{[this](DirectoryScan& scan)
                     {
                       scan.lookups = std::make_unique<ItemLookups>();
                       SetUpLookups(scan, *scan.lookups);
                       PlanLookups(*scan.lookups);
                       ReadDiscsAhead(*scan.lookups);
                     }};
    // Not where its folders are scanned as it finishes: their discs, queued later, would pass these by
    const auto readExtrasAhead{
        [this](DirectoryScan& scan)
        {
          if (!scan.plannedAhead.empty() ||
              std::ranges::none_of(
                  scan.items, [this, &scan](const auto& item)
                  { return IsScannedAlongside(scan, *item, AddsAsExtras(scan, *item)); }))
            ReadExtrasDiscsAhead(scan);
        }};
    const std::function<void(DirectoryScan&)> lookUpAhead{
        [this, &entries, &setUp, &readExtrasAhead, &lookUpAhead](DirectoryScan& scan)
        {
          for (auto& [path, planned] : scan.plannedAhead)
          {
            // A folder of many movies is looked up in its turn, so that they aren't all held till then
            if (!planned || !CanScanAlongside(*planned) || planned->items.Size() > 8)
              continue;
            if (!planned->skip && !planned->lookups)
            {
              setUp(*planned);
              for (size_t item = 0; item < static_cast<size_t>(planned->items.Size()); ++item)
                entries.push_back({planned.get(), std::nullopt, item});
            }
            lookUpAhead(*planned);
            readExtrasAhead(*planned);
          }
        }};
    for (size_t folder = 0; folder < folders.size(); ++folder)
    {
      DirectoryScan& scan{*folders[folder]};
      if (!scan.skip)
      {
        if (!scan.lookups)
          setUp(scan);
        for (size_t item = 0; item < static_cast<size_t>(scan.items.Size()); ++item)
          entries.push_back({&scan, folder, item});
      }
      lookUpAhead(scan);
      readExtrasAhead(scan);
      entries.push_back({&scan, folder, std::nullopt});
    }

    batch.failedLookups.resize(folders.size());
    // Looked up in the order of the run, the batches before first
    batch.first = run.Add(entries.size(),
                          [this, &entries](size_t e, size_t place)
                          {
                            if (const Entry & entry{entries[e]}; entry.item)
                              LookUpItem(*entry.scan->lookups, *entry.item, place);
                          });
  }

  bool CVideoInfoScanner::HandBackAlongside(CVideoLookups::CRun& run, AlongsideBatch& batch)
  {
    m_database.Open();
    const bool handedBack{
        run.HandBack(batch.first + batch.entries.size(),
                     [this, &batch](size_t index)
                     {
                       const AlongsideBatch::Entry& entry{batch.entries[index - batch.first]};
                       if (!entry.folder)
                         return true;

                       DirectoryScan& scan{*entry.scan};
                       const size_t folder{*entry.folder};
                       std::optional<unsigned int>& failedLookups{batch.failedLookups[folder]};
                       if (!failedLookups)
                         failedLookups = m_failedLookups;
                       if (entry.item)
                         return HandBackItem(*scan.lookups, *entry.item);

                       if (!scan.skip)
                       {
                         EndLookups(*scan.lookups);
                         EndRetrieve(scan, scan.lookups->foundSomeInfo, *failedLookups);
                       }
                       const auto [scanComplete, foundContent] = FinishDirectory(scan);
                       ForgetListings(scan.path);
                       ForgetLookUps(scan.path);
                       if (scanComplete == ScanComplete::Stopped)
                       {
                         m_bStop = true;
                         return false;
                       }
                       return true;
                     })};
    m_folderMovies = nullptr;
    m_database.Close();
    return handedBack;
  }

  ScraperPtr CVideoInfoScanner::ScraperForPath(const std::string& path,
                                               SScanSettings& settings,
                                               bool& foundDirectly)
  {
    return m_pathTable->Resolve(path, settings, foundDirectly, &m_scraperCache);
  }

  bool CVideoInfoScanner::IsContentRoot(const std::string& folder)
  {
    SScanSettings settings;
    bool foundDirectly{false};
    ScraperForPath(folder, settings, foundDirectly);
    return foundDirectly;
  }

  void CVideoInfoScanner::RemoveExtrasFolder(const std::string& folder)
  {
    // As Process() does for the folders a scan leaves
    RemoveSubDirectories(m_pathsToScan, folder,
                         [this](const std::string& dir)
                         {
                           m_progress.Finished();
                           if (m_bClean)
                             m_pathsToClean.insert(m_database.GetPathId(dir));
                         });
  }

  void CVideoInfoScanner::AddExtrasFolder(DirectoryScan& scan, const std::string& folder)
  {
    // An extras folder of a listing that has changed is looked at again, as its movie may be new.
    // One of an unchanged listing is looked at only where something below it has changed. Its
    // extras are added from every folder below it, whatever a scan for movies excludes.
    if (!scan.skip)
      scan.extrasFolders.emplace_back(folder);
    else if (FolderChanged(folder, {}))
    {
      scan.extrasFolders.emplace_back(folder);
      scan.extrasAddedAgain = true;
    }
  }

  std::string CVideoInfoScanner::FolderSettings(const std::string& folder)
  {
    if (const auto known{m_folderSettings.find(folder)}; known != m_folderSettings.end())
      return known->second;

    SScanSettings settings;
    bool foundDirectly{false};
    const ScraperPtr scraper{ScraperForPath(folder, settings, foundDirectly)};
    std::string fingerprint;
    if (scraper)
    {
      const bool tvShows{scraper->Content() == ContentType::TVSHOWS};
      std::string text{StringUtils::Format(
          "{}|{}|{}|{}|{}|{}|{}|{}|{}|{}", TranslateContent(scraper->Content()), scraper->ID(),
          scraper->Version().asString(), scraper->GetPathSettings(), settings.parent_name,
          settings.parent_name_root, settings.recurse,
          m_advancedSettings->m_bVideoLibraryUseFastHash, m_ignoreVideoExtras,
          CServiceBroker::GetFileExtensionProvider().GetVideoExtensions())};
      // Whether hidden files are shown changes which are scanned, but not the listing they are in
      const bool showHidden{CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
          CSettings::SETTING_FILELISTS_SHOWHIDDEN)};
      text += showHidden ? "|1" : "|0";
      text += "|" + StringUtils::Join(tvShows ? m_advancedSettings->m_tvshowExcludeFromScanRegExps
                                              : m_advancedSettings->m_moviesExcludeFromScanRegExps,
                                      "|");
      if (tvShows)
      {
        for (const TVShowRegexp& regexp : m_advancedSettings->m_tvshowEnumRegExps)
          text += StringUtils::Format("|{}{}{}{}", regexp.byDate, regexp.byTitle,
                                      regexp.defaultSeason, regexp.regexp);
        text += "|" + m_advancedSettings->m_tvshowMultiPartEnumRegExp;
      }
      else
      {
        for (const CRegExp& regexp : m_advancedSettings->m_videoStackRegExps)
          text += "|" + regexp.GetPattern();
        for (const CRegExp& regexp : m_advancedSettings->m_folderStackRegExps)
          text += "|" + regexp.GetPattern();
      }
      CDigest digest{CDigest::Type::MD5};
      digest.Update(text);
      fingerprint = digest.Finalize();
    }
    return m_folderSettings.try_emplace(folder, std::move(fingerprint)).first->second;
  }

  std::shared_ptr<const CDirectorySnapshot> CVideoInfoScanner::Snapshot(const std::string& folder)
  {
    if (const auto taken{m_snapshots.find(folder)}; taken != m_snapshots.end())
      return taken->second;

    // A folder listed before its parent is left is taken again where it has been let go of. A cold
    // scan of a large folder lists every folder in it before visiting them. Two batches of folders
    // looked up together, and the folders they hold, are listed at once.
    constexpr size_t MAX_SNAPSHOTS{1024};
    while (m_snapshots.size() >= MAX_SNAPSHOTS && !m_snapshotOrder.empty())
    {
      m_snapshots.erase(m_snapshotOrder.front());
      m_snapshotOrder.pop_front();
    }
    m_snapshotOrder.emplace_back(folder);
    // Read before the folder is listed, so that its record never holds a later time than its
    // listing
    FolderTime(folder);
    auto snapshot{CDirectorySnapshot::Take(folder)};
    // Its listing is now in the directory cache, where a video's subtitles are looked for
    if (snapshot)
    {
      std::unique_lock lock(m_listedFoldersLock);
      m_listedFolders.insert(folder);
    }
    return m_snapshots.try_emplace(folder, std::move(snapshot)).first->second;
  }

  std::optional<int64_t> CVideoInfoScanner::FolderTime(const std::string& folder)
  {
    if (const auto listed{m_listedTimes.find(folder)};
        listed != m_listedTimes.end() && listed->second)
      return listed->second;
    if (URIUtils::IsPlugin(folder))
      return std::nullopt;
    return m_listedTimes.insert_or_assign(folder, ReadModifiedTime(folder)).first->second;
  }

  bool CVideoInfoScanner::HasScannedSubfolders(const CDirectorySnapshot& snapshot,
                                               const std::vector<std::string>& excludes)
  {
    return !ScannedSubfolders(snapshot, excludes).empty();
  }

  std::vector<std::string> CVideoInfoScanner::ScannedSubfolders(
      const CDirectorySnapshot& snapshot, const std::vector<std::string>& excludes, bool tvShow)
  {
    std::vector<std::string> subfolders;
    // Nothing below a folder holding .nomedia is scanned, nor below a disc's index in a show
    if (snapshot.HasNoMedia() || (tvShow && HoldsDiscIndex(snapshot)))
      return subfolders;

    const bool showHidden{CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
        CSettings::SETTING_FILELISTS_SHOWHIDDEN)};
    for (const DirectoryEntry& entry : snapshot.Entries())
    {
      if (!entry.isFolder || (entry.hidden && !showHidden) ||
          (!tvShow && IsDiscStructureFolder(entry.name)))
        continue;
      std::string subfolder{URIUtils::AddFileToFolder(snapshot.Path(), entry.name)};
      URIUtils::AddSlashAtEnd(subfolder);
      // An extras folder's extras are added whatever a scan for movies excludes
      if ((tvShow || !IsVideoExtrasFolderName(entry.name)) &&
          CUtil::ExcludeFileOrFolder(subfolder, excludes, &m_regexpCache))
        continue;
      subfolders.emplace_back(std::move(subfolder));
    }
    return subfolders;
  }

  bool CVideoInfoScanner::FolderUnchangedUnlisted(const std::string& folder)
  {
    const std::optional<FolderRecord> record{m_pathTable->Record(folder)};
    return record && record->UnchangedUnlisted(FolderSettings(folder), FolderTime(folder),
                                               m_advancedSettings->m_bVideoLibraryUseFastHash);
  }

  bool CVideoInfoScanner::FolderHasNoMedia(const std::string& folder, bool list)
  {
    if (URIUtils::IsPlugin(folder) || URIUtils::IsArchive(CURL(folder)))
      return HasNoMedia(folder);

    const std::optional<FolderRecord> record{m_pathTable->Record(folder)};
    if (record && record->UnchangedUnlisted(FolderSettings(folder), FolderTime(folder),
                                            m_advancedSettings->m_bVideoLibraryUseFastHash))
      return record->noMedia;

    // A folder the scan hasn't listed yet is looked into rather than listed, so that the listings
    // of a folder's subfolders aren't all held until each is visited
    const auto taken{m_snapshots.find(folder)};
    if (!list && taken == m_snapshots.end())
      return HasNoMedia(folder);

    const auto snapshot{taken != m_snapshots.end() ? taken->second : Snapshot(folder)};
    if (!snapshot || !snapshot->HasNoMedia())
      return false;

    // So that the next scan knows without listing it again
    m_pathTable->Store(folder, FolderRecord{.cfg = FolderSettings(folder),
                                            .mtime = FolderTime(folder),
                                            .digest = snapshot->Digest(),
                                            .noMedia = true});
    return true;
  }

  bool CVideoInfoScanner::FolderChanged(const std::string& folder,
                                        const std::vector<std::string>& excludes,
                                        bool tvShow)
  {
    const std::string cfg{FolderSettings(folder)};
    const std::optional<FolderRecord> record{m_pathTable->Record(folder)};
    const std::optional<int64_t> time{FolderTime(folder)};
    if (record &&
        record->UnchangedUnlisted(cfg, time, m_advancedSettings->m_bVideoLibraryUseFastHash))
      return false;

    const auto snapshot{Snapshot(folder)};
    if (!snapshot || !record || !record->UnchangedListing(cfg, snapshot->Digest()))
      return true;

    // Only the time it was listed with has moved
    if (record->mtime != time)
    {
      FolderRecord moved{*record};
      moved.mtime = time;
      m_pathTable->Store(folder, moved);
    }

    return std::ranges::any_of(ScannedSubfolders(*snapshot, excludes, tvShow),
                               [this, &excludes, tvShow](const std::string& subfolder)
                               { return FolderChanged(subfolder, excludes, tvShow); });
  }

  void CVideoInfoScanner::StoreFolderRecords(const std::string& folder,
                                             const std::vector<std::string>& excludes)
  {
    StoreRecords(ListFolderRecords(folder, excludes));
  }

  void CVideoInfoScanner::StoreRecords(
      const std::vector<std::pair<std::string, FolderRecord>>& records)
  {
    if (records.empty())
      return;

    // The table has them only once committed
    CScanPathTable::CStoring storing{*m_pathTable};
    const bool ownTransaction{!m_database.InTransaction()};
    if (ownTransaction)
      m_database.BeginTransaction();
    for (const auto& [folder, record] : records)
      m_pathTable->Store(folder, record);
    if (!ownTransaction || m_database.CommitTransaction())
    {
      storing.Keep();
      return;
    }

    if (m_database.InTransaction())
      m_database.RollbackTransaction();
    CLog::Log(LOGERROR,
              "VideoInfoScanner: The records of {} and the folders below it couldn't be stored",
              CURL::GetRedacted(records.front().first));
  }

  std::vector<std::pair<std::string, FolderRecord>> CVideoInfoScanner::ListFolderRecords(
      const std::string& folder, const std::vector<std::string>& excludes)
  {
    std::vector<std::pair<std::string, FolderRecord>> records;
    const auto snapshot{Snapshot(folder)};
    if (!snapshot)
      return records;

    const std::vector<std::string> subfolders{ScannedSubfolders(*snapshot, excludes)};
    records.emplace_back(folder, FolderRecord{.cfg = FolderSettings(folder),
                                              .mtime = FolderTime(folder),
                                              .digest = snapshot->Digest(),
                                              .hasSubfolders = !subfolders.empty(),
                                              .noMedia = snapshot->HasNoMedia()});
    for (const std::string& subfolder : subfolders)
      std::ranges::move(ListFolderRecords(subfolder, excludes), std::back_inserter(records));
    return records;
  }

  bool CVideoInfoScanner::AddExtrasBesideDisc(const CDirectorySnapshot& discFolder)
  {
    // A new extras folder changes the disc's folder, and a file added below one changes the folder
    // holding it, each of which its record shows
    bool added{false};
    const std::vector<std::string>& excludes{m_advancedSettings->m_moviesExcludeFromScanRegExps};
    for (const std::string& folder : ScannedSubfolders(discFolder, excludes))
    {
      if (!IsVideoExtrasFolderName(URIUtils::GetFileOrFolderName(folder)))
        continue;

      // Leave a folder with content set on it (a source root) alone
      if (IsContentRoot(folder))
        continue;

      if (FolderHasNoMedia(folder, true) || !FolderChanged(folder, {}))
        continue;

      const int dbId{m_database.GetMovieIdInFolder(discFolder.Path(), folder)};
      if (dbId < 0)
      {
        CLog::Log(LOGDEBUG, "VideoInfoScanner: No single movie found for video extras {}",
                  CURL::GetRedacted(folder));
        continue;
      }

      if (m_extras.AddVideoExtras(dbId, folder))
        StoreFolderRecords(folder, {});
      added = true;
      CLog::Log(LOGDEBUG, "VideoInfoScanner: Finished adding video extras from dir {}",
                CURL::GetRedacted(folder));
    }
    return added;
  }

  void CVideoInfoScanner::ForgetListings(const std::string& folder)
  {
    std::erase_if(m_snapshots,
                  [&folder](const auto& snapshot) { return snapshot.first.starts_with(folder); });
    std::erase_if(m_snapshotOrder,
                  [&folder](const std::string& path) { return path.starts_with(folder); });
    std::erase_if(m_listedTimes,
                  [&folder](const auto& time) { return time.first.starts_with(folder); });
  }

  void CVideoInfoScanner::ForgetLookUps(const std::string& folder)
  {
    m_art.ForgetFolder(folder);
    m_subtitleFolders.Forget(folder);
  }

  void CVideoInfoScanner::CollectShowFiles(
      const std::string& folder,
      CFileItemList& files,
      std::vector<std::pair<std::string, FolderRecord>>& records,
      bool recorded)
  {
    const auto snapshot{Snapshot(folder)};
    if (!snapshot)
      return;

    if (recorded)
      records.emplace_back(
          folder, FolderRecord{.cfg = FolderSettings(folder),
                               .mtime = FolderTime(folder),
                               .digest = snapshot->Digest(),
                               .hasSubfolders = !ScannedSubfolders(*snapshot, {}, true).empty(),
                               .noMedia = snapshot->HasNoMedia()});

    // As a recursive listing of the show, which leaves out what is below a .nomedia file
    if (snapshot->HasNoMedia())
      return;

    CFileItemList listing;
    snapshot->GetItems(listing, CServiceBroker::GetFileExtensionProvider().GetVideoExtensions(),
                       true);
    for (const auto& item : listing)
    {
      if (!item->IsFolder())
        files.Add(item);
    }

    // A disc's playlists and streams are below its index, which is all that is kept of it
    if (HoldsDiscIndex(*snapshot))
      return;

    // What is inside an archive is never looked at on its own, so isn't recorded. A disc structure
    // folder is, as a file added to it changes only its own time.
    for (const auto& item : listing)
    {
      if (item->IsFolder())
        CollectShowFiles(item->GetPath(), files, records,
                         recorded && !URIUtils::IsArchive(CURL(item->GetPath())));
    }
  }

  void CVideoInfoScanner::StoreShowRecords(const std::string& show)
  {
    const auto records{m_showRecords.find(show)};
    if (records == m_showRecords.end())
      return;
    StoreRecords(records->second);
    m_showRecords.erase(records);
  }

  bool CVideoInfoScanner::UpdateSetInTag(CVideoInfoTag& tag)
  {
    return tag.m_set.HasTitle() &&
           UpdateSetInTag(tag, CVideoInfoScannerArt::ReadMovieSetFolder(tag.m_set.GetTitle()));
  }

  bool CVideoInfoScanner::UpdateMovieSet(CVideoInfoTag& tag) const
  {
    return tag.m_set.HasTitle() &&
           UpdateSetInTag(tag, m_art.GetMovieSetFolder(tag.m_set.GetTitle()));
  }

  bool CVideoInfoScanner::UpdateSetInTag(CVideoInfoTag& tag,
                                         const CVideoInfoScannerArt::MovieSetFolder& folder)
  {
    // Uses the set information in m_set from a tag of a movie
    // If there is a set, see if the details need to be updated from the
    // Movie Set Information Folder
    if (!tag.m_set.HasTitle())
      return false;

    bool setUpdated{false};
    ART::Artwork movieSetArt;
    ART::Artwork setNfoArt;

    // Wherever the set's art comes from, it follows the artwork level, as a movie's does
    const int artLevel{CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(
        CSettings::SETTING_VIDEOLIBRARY_ARTWORK_LEVEL)};
    const bool addAll{artLevel == CSettings::VIDEOLIBRARY_ARTWORK_LEVEL_ALL};
    const bool exactName{artLevel == CSettings::VIDEOLIBRARY_ARTWORK_LEVEL_BASIC};
    const std::vector<std::string> movieSetArtTypes{
        CVideoThumbLoader::GetArtTypes(MEDIA::TYPE::VIDEO_COLLECTION)};
    const auto isWanted = [addAll, exactName, &movieSetArtTypes](const std::string& artType) {
      return addAll ||
             CVideoThumbLoader::IsArtTypeInWhitelist(artType, movieSetArtTypes, exactName);
    };

    const std::string& movieSetInfoPath{folder.path};
    if (!movieSetInfoPath.empty())
    {
      // From set.nfo
      if (folder.nfo)
      {
        const CSetInfoTag& setTag{*folder.nfo};
        tag.m_set.SetOriginalTitle(tag.m_set.GetTitle());
        if (!setTag.GetTitle().empty())
          tag.m_set.SetTitle(setTag.GetTitle());
        if (setTag.HasSortTitle())
          tag.m_set.SetSortTitle(setTag.GetSortTitle());
        if (!setTag.GetOverview().empty())
          tag.m_set.SetOverview(setTag.GetOverview());
        setNfoArt = setTag.GetArt();
        setUpdated = true;
      }

      // Now look for art
      // Look for local art files first
      if (artLevel != CSettings::VIDEOLIBRARY_ARTWORK_LEVEL_NONE)
        ART::AddLocalItemArtwork(movieSetArt, movieSetArtTypes, movieSetInfoPath, addAll, exactName,
                                 true);

      // If art specified in set.nfo use that next
      if (movieSetArt.empty())
        for (const auto& art : setNfoArt)
          if (isWanted(art.first))
            movieSetArt.insert(art);
    }
    else
    {
      // No MSIF, so use the set art from the scraper (already in tag)
      tag.m_strPictureURL.Parse();
      for (const auto& url : tag.m_strPictureURL.GetUrls())
        if (StringUtils::StartsWith(url.m_aspect, "set.") && isWanted(url.m_aspect.substr(4)))
          movieSetArt.try_emplace(url.m_aspect.substr(4), url.m_url);
    }
    if (!movieSetArt.empty())
    {
      tag.m_set.SetArt(movieSetArt);
      setUpdated = true;
    }

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

    ItemLookups batch;
    batch.items = &items;
    batch.bDirNames = bDirNames;
    batch.content = content;
    batch.useLocal = useLocal;
    batch.pURL = pURL;
    batch.fetchEpisodes = fetchEpisodes;
    batch.pDlgProgress = pDlgProgress;
    batch.useFolderNames = m_useFolderNames;
    PlanLookups(batch);

    // A dialog is updated from this thread, so each item is looked up in turn
    CVideoLookups::Run(
        static_cast<size_t>(items.Size()), pDlgProgress ? 1 : LookUpThreads(),
        [this, &batch](size_t i) { LookUpItem(batch, i, i); },
        [this, &batch](size_t i) { return HandBackItem(batch, i); }, m_bStop);

    EndLookups(batch);
    if (pDlgProgress)
      pDlgProgress->ShowProgressBar(false);

    m_database.Close();
    return batch.foundSomeInfo;
  }

  void CVideoInfoScanner::PlanLookups(ItemLookups& batch)
  {
    const CFileItemList& items{*batch.items};
    const ContentType content{batch.content};
    const CScraperUrl* const pURL{batch.pURL};
    const CGUIDialogProgress* const pDlgProgress{batch.pDlgProgress};

    if (content == ContentType::MOVIES)
    {
      batch.folderMovies = m_database.GetFolderMovies(items.GetPath());
      // Read before any movie is looked up, which then only reads them
      LoadVideoVersionTypes();
    }

    std::vector<std::optional<ItemLookups::Candidate>>& candidates{batch.candidates};
    candidates.resize(items.Size());
    batch.lookups.resize(items.Size());
    batch.shows.resize(items.Size());
    bool first{true};
    for (int i = 0; i < items.Size(); ++i)
    {
      const CFileItemPtr& pItem{items[i]};

      if (pItem->IsFolder() && pItem->GetProperty(PROPERTY_UNCHANGED).asBoolean())
        continue;

      // we do this since we may have a override per dir
      ScraperPtr info2 = ScraperForPath(pItem->IsFolder() ? pItem->GetPath() : items.GetPath());
      if (!info2) // skip
        continue;

      // Discard all .nomedia folders
      if (pItem->IsFolder() && FolderHasNoMedia(pItem->GetPath()))
        continue;

      // Discard all exclude files defined by regExExclude
      if (CUtil::ExcludeFileOrFolder(pItem->GetPath(),
                                     (content == ContentType::TVSHOWS)
                                         ? m_advancedSettings->m_tvshowExcludeFromScanRegExps
                                         : m_advancedSettings->m_moviesExcludeFromScanRegExps,
                                     &m_regexpCache))
        continue;

      // A movie is looked up alongside others unless it is given a url, as the first item is by a
      // refresh, or a dialog shows each in turn
      ItemLookups::Candidate& candidate{candidates[i].emplace()};
      candidate.lookUpAlongside =
          info2->Content() == ContentType::MOVIES && !pDlgProgress && !(first && pURL) &&
          CanLookUpMovieAlongside(*pItem) &&
          m_database.GetMovieId(pItem->GetDynPath(), batch.folderMovies) <= 0;
      // Not a show folder recorded without episodes, which its record decides
      candidate.lookUpShow =
          info2->Content() == ContentType::TVSHOWS && !pDlgProgress && !(first && pURL) &&
          m_uniqueIDs.empty() && batch.fetchEpisodes && pItem->IsFolder() && !pItem->IsPlugin() &&
          m_database.GetTvShowId(pItem->GetPath()) < 0 && !m_pathTable->Record(pItem->GetPath());
      candidate.info2 = std::move(info2);
      first = false;
    }
  }

  void CVideoInfoScanner::LookUpItem(ItemLookups& batch, size_t i, size_t place) const
  {
    const std::optional<ItemLookups::Candidate>& candidate{batch.candidates[i]};
    if (!candidate || m_bStop)
      return;

    const CScraperCalls::CInterpreter interpreter{m_scraperCalls};
    const CVideoLookups::CPlace turn{place};
    if (candidate->lookUpAlongside && !batch.lookups[i].done)
    {

      LookUpMovie(*(*batch.items)[static_cast<int>(i)], batch.bDirNames, candidate->info2,
                  batch.useLocal, batch.useFolderNames, batch.lookups[i]);
      batch.lookups[i].done = true;
    }
    else if (candidate->lookUpShow && !batch.shows[i].done)
    {
      LookUpShow(*(*batch.items)[static_cast<int>(i)], batch.bDirNames, candidate->info2,
                 batch.useLocal, batch.shows[i]);
      batch.shows[i].done = true;
    }
  }

  bool CVideoInfoScanner::HandBackItem(ItemLookups& batch, size_t index)
  {
    if (!batch.candidates[index])
      return true;

    const CFileItemList& items{*batch.items};
    const int i{static_cast<int>(index)};
    m_useFolderNames = batch.useFolderNames;
    m_folderMovies = &batch.folderMovies;

    CFileItemPtr pItem = items[i];
    ScraperPtr info2{batch.candidates[index]->info2};
    MovieLookup* const lookup{batch.lookups[index].done ? &batch.lookups[index] : nullptr};

    if (info2->Content() == ContentType::MOVIES || info2->Content() == ContentType::MUSICVIDEOS)
      ReportProgress(static_cast<float>(i) / static_cast<float>(items.Size()));

    // A movie already in the library had its discs read when added, or by a refresh
    bool knownMovie{false};
    const size_t added{m_summary.added.size()};
    const unsigned int unchangedDirectories{m_summary.unchangedDirectories};
    const unsigned int failedLookups{m_failedLookups};
    ScanSummary::Type type{ScanSummary::Type::MOVIE};
    if (info2->Content() == ContentType::TVSHOWS)
      type = ScanSummary::Type::TV_SHOW;
    else if (info2->Content() == ContentType::MUSICVIDEOS)
      type = ScanSummary::Type::MUSIC_VIDEO;
    // Its disc read ahead, held until its extras are added too
    std::optional<CDiscReadAhead::CTaken> disc;
    InfoRet ret = InfoRet::CANCELLED;
    if (info2->Content() == ContentType::TVSHOWS)
    {
      ShowLookup* const show{
          index < batch.shows.size() && batch.shows[index].done ? &batch.shows[index] : nullptr};
      // A show looked up ahead has its discs read while it is added, and then those of the next
      // show looked up ahead, so that the readers keep ahead of the scan from show to show. A show
      // after another looked up ahead was listed already, as that one's next.
      if (show)
      {
        if ((show->found || show->nfoResult == InfoType::FULL) &&
            !(index > 0 && batch.shows[index - 1].done))
          ReadEpisodeDiscsAhead(ShowDiscs(pItem->GetPath()));
        if (index + 1 < batch.candidates.size() && batch.candidates[index + 1] &&
            batch.candidates[index + 1]->lookUpShow)
          ReadEpisodeDiscsAhead(ShowDiscs(items[i + 1]->GetPath()));
      }
      ret = RetrieveInfoForTvShow(pItem.get(), batch.bDirNames, info2, batch.useLocal, batch.pURL,
                                  batch.fetchEpisodes, batch.pDlgProgress, show);
      ForgetLookUps(pItem->GetPath());
      // Its details, the item's now, aren't held for the rest of the batch
      if (show)
      {
        show->guide.reset();
        show->item = CFileItem{};
      }
      if (m_progressShows.erase(pItem->GetPath()))
      {
        m_progress.Finished(RemoveSubDirectories(m_pathsToScan, pItem->GetPath(),
                                                 [this](const std::string& dir)
                                                 {
                                                   if (m_bClean)
                                                     m_pathsToClean.insert(
                                                         m_database.GetPathId(dir));
                                                 }));
        FolderFinished();
      }
    }
    else if (info2->Content() == ContentType::MOVIES)
    {
      disc.emplace(m_discReadAhead, pItem->GetPath());
      knownMovie = m_database.GetMovieId(pItem->GetDynPath(), batch.folderMovies) > 0;
      ret = RetrieveInfoForMovie(pItem.get(), batch.bDirNames, info2, batch.useLocal, batch.pURL,
                                 batch.pDlgProgress, lookup);
      if (lookup)
      {
        lookup->item = CFileItem{};
        lookup->prepared.reset();
      }
    }
    else if (info2->Content() == ContentType::MUSICVIDEOS)
      ret = RetrieveInfoForMusicVideo(pItem.get(), batch.bDirNames, info2, batch.useLocal,
                                      batch.pURL, batch.pDlgProgress);
    else
    {
      CLog::Log(LOGERROR, "VideoInfoScanner: Unknown content type {} ({})", info2->Content(),
                CURL::GetRedacted(pItem->GetPath()));
      batch.foundSomeInfo = false;
      return false;
    }
    if (ret == InfoRet::CANCELLED || ret == InfoRet::INFO_ERROR)
    {
      // A look-up given up, as the scan stopped, is no error
      CLog::Log(ret == InfoRet::CANCELLED ? LOGDEBUG : LOGWARNING,
                "VideoInfoScanner: Error {} occurred while retrieving"
                "information for {}.",
                static_cast<int>(ret), CURL::GetRedacted(pItem->GetPath()));
      if (ret == InfoRet::CANCELLED)
      {
        batch.foundSomeInfo = false;
        return false;
      }
      // The other items are still added, and the directory is tried again by the next scan
      ++m_failedLookups;
      NotAdded(type, pItem->GetPath(), true);
    }
    else if (ret == InfoRet::ADDED || ret == InfoRet::HAVE_ALREADY)
    {
      batch.foundSomeInfo = true;
      // A movie added as a version of one already in the library counts as added, and an
      // unchanged show as an unchanged folder
      if (ret == InfoRet::HAVE_ALREADY && m_summary.added.size() == added &&
          m_summary.unchangedDirectories == unchangedDirectories)
      {
        // A disc without a file of its own holds its movie's default version
        if (type == ScanSummary::Type::MOVIE)
        {
          const int idFile{m_database.GetVideoVersionInfo(pItem->GetDynPath()).m_idFile};
          if (idFile > -1 && !m_database.IsDefaultVideoVersion(idFile))
            type = ScanSummary::Type::VERSION;
        }
        m_summary.existing.push_back({type, pItem->GetPath(), {}});
      }
      if (info2->Content() == ContentType::MOVIES && !m_ignoreVideoExtras && !m_deferMovieExtras &&
          !knownMovie)
        m_extras.AddMovieDiscExtras(*pItem);
    }
    else if (ret == InfoRet::NOT_FOUND)
    {
      CLog::Log(LOGWARNING, "No information found for item '{}', it won't be added to the library.",
                CURL::GetRedacted(pItem->GetPath()));

      // Nothing found by a lookup that failed isn't the same as nothing to find
      NotAdded(type, pItem->GetPath(), m_failedLookups != failedLookups);
    }

    batch.pURL = nullptr;

    // Keep track of directories we've seen
    if (m_bClean && pItem->IsFolder())
    {
      // A show added by the scan is known to the database only
      const int idPath{m_pathTable->PathId(pItem->GetPath())};
      batch.seenPaths.push_back(idPath >= 0 ? idPath : m_database.GetPathId(pItem->GetPath()));
    }
    return true;
  }

  void CVideoInfoScanner::EndLookups(ItemLookups& batch)
  {
    m_folderMovies = nullptr;

    // A show left out, or not reached as the scan stopped, is finished too
    for (const auto& item : *batch.items)
    {
      if (m_progressShows.erase(item->GetPath()))
        FolderFinished();
    }

    if (batch.content == ContentType::TVSHOWS && !batch.seenPaths.empty())
    {
      std::vector<std::pair<int, std::string>> libPaths;
      m_database.GetSubPaths(batch.items->GetPath(), libPaths);
      for (const auto& [idPath, path] : libPaths)
      {
        if (std::ranges::find(batch.seenPaths, idPath) == batch.seenPaths.end())
          m_pathsToClean.insert(idPath);
      }
    }
  }

  void CVideoInfoScanner::AddMovieExtras(int dbId, bool useFolderNames)
  {
    if (m_ignoreVideoExtras)
      return;

    m_database.Open();
    m_extras.AddMovieExtras(dbId, useFolderNames);
    m_database.Close();
  }

  CInfoScanner::InfoRet CVideoInfoScanner::RetrieveInfoForTvShow(CFileItem* pItem,
                                                                 bool bDirNames,
                                                                 ScraperPtr& info2,
                                                                 bool useLocal,
                                                                 CScraperUrl* pURL,
                                                                 bool fetchEpisodes,
                                                                 CGUIDialogProgress* pDlgProgress,
                                                                 ShowLookup* lookup /* = nullptr */)
  {
    const bool isSeason =
        pItem->HasVideoInfoTag() && pItem->GetVideoInfoTag()->GetMediaType() == MEDIA::TYPE::SEASON;

    // Enumerate episodes here as this compares the records of folders containing episodes from
    // this show, and we want to do this before processing nfo files. A show folder whose episodes
    // are wanted is looked at before the library is asked about it, as nothing is needed from it
    // where the folder hasn't changed.
    EPISODELIST files;
    std::optional<EpisodeResult> epResult;
    if (pItem->IsFolder() && fetchEpisodes)
    {
      epResult = EnumerateSeriesFolder(pItem, files);
      if (*epResult == EpisodeResult::NOT_CHANGED)
        return InfoRet::HAVE_ALREADY;
      // Its files hold no episode until a folder of it changes, or the settings that name
      // episodes do, either of which changes its records. A show in the library isn't recorded
      // where an update cleans, so that a scan stopped before its clean leaves the show to the next.
      if (*epResult == EpisodeResult::NO_EPISODES &&
          (!m_bClean || m_database.GetTvShowId(pItem->GetPath()) < 0))
        StoreShowRecords(pItem->GetPath());
      if (*epResult == EpisodeResult::NO_MEDIA || *epResult == EpisodeResult::NO_EPISODES)
        return InfoRet::NOT_NEEDED;
    }

    int idTvShow = -1;
    std::string strPath = pItem->GetPath();
    if (pItem->IsFolder())
    {
      idTvShow = m_database.GetTvShowId(strPath);
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
    }

    // If we only want to refresh show details (and not the episodes), the
    // episode-folder hash comparison below is irrelevant: it exists purely to decide
    // whether episodes need to be re-scanned.
    const bool refreshShowInfoOnly =
        idTvShow > -1 && pItem->IsFolder() && !isSeason && !fetchEpisodes;

    if (!refreshShowInfoOnly)
    {
      if (!epResult)
      {
        epResult = EnumerateSeriesFolder(pItem, files);
        if (*epResult == EpisodeResult::NOT_CHANGED)
          return InfoRet::HAVE_ALREADY;
        if (*epResult == EpisodeResult::NO_EPISODES && (!m_bClean || idTvShow < 0))
          StoreShowRecords(pItem->GetPath());
        if (*epResult == EpisodeResult::NO_MEDIA || *epResult == EpisodeResult::NO_EPISODES)
          return InfoRet::NOT_NEEDED;
      }
      if (isSeason && idTvShow < 0)
        return InfoRet::NOT_NEEDED;
    }

    // An episode whose lookup failed leaves the show's folders unrecorded, so the next scan tries
    // it again
    const auto setFolderHash = [this, pItem, failedLookups = m_failedLookups]
    {
      if (pItem->IsFolder() && m_failedLookups == failedLookups)
        StoreShowRecords(pItem->GetPath());
    };

    if (ProgressCancelled(pDlgProgress, pItem->IsFolder() ? 20353 : 20361,
                          pItem->IsFolder() ? pItem->GetVideoInfoTag()->m_strShowTitle
                                            : pItem->GetVideoInfoTag()->m_strTitle))
      return InfoRet::CANCELLED;

    if (m_handle)
      m_handle->SetText(pItem->GetMovieName(bDirNames));

    // A show already in the library only gets its new episodes. The show is taken from the library,
    // so its nfo isn't read.
    if (idTvShow > -1 && (fetchEpisodes || !pItem->IsFolder()))
    {
      const InfoRet ret = files.empty() ? InfoRet::NOT_NEEDED
                                        : RetrieveInfoForEpisodes(pItem, idTvShow, files, info2,
                                                                  useLocal, pDlgProgress);
      if (ret == InfoRet::ADDED || ret == InfoRet::NOT_NEEDED)
        setFolderHash();
      return ret;
    }

    InfoType result = InfoType::NONE;
    CScraperUrl scrUrl;
    // handle .nfo files
    std::unique_ptr<IVideoInfoTagLoader> loader;
    if (lookup)
    {
      result = lookup->nfoResult;
      loader = std::move(lookup->loader);
      if (result == InfoType::FULL)
        *pItem = std::move(lookup->item);
    }
    else if (useLocal)
      std::tie(result, loader) = ReadInfoTag(*pItem, info2, bDirNames, true);

    if (result == InfoType::FULL && (idTvShow < 0 || refreshShowInfoOnly))
    {
      const long lResult = AddVideo(pItem, info2, bDirNames, useLocal);
      if (lResult < 0)
        return InfoRet::INFO_ERROR;
      if (fetchEpisodes && !files.empty())
      {
        const InfoRet ret =
            RetrieveInfoForEpisodes(pItem, lResult, files, info2, useLocal, pDlgProgress);
        if (ret == InfoRet::ADDED)
          setFolderHash();
        return ret;
      }
      if (files.empty())
        setFolderHash();
      return InfoRet::ADDED;
    }

    if (lookup)
    {
      if (!lookup->found)
        return LookupNotFound(*lookup, pDlgProgress);

      *pItem = std::move(lookup->item);
      const long showId{AddVideo(pItem, info2, false, useLocal)};
      if (showId < 0)
        return InfoRet::INFO_ERROR;
      if (fetchEpisodes && !files.empty())
      {
        const InfoRet ret{RetrieveInfoForEpisodes(pItem, showId, files, info2, useLocal,
                                                  pDlgProgress, true,
                                                  lookup->guide ? &*lookup->guide : nullptr)};
        if (ret == InfoRet::ADDED)
          setFolderHash();
      }
      else if (files.empty())
      {
        setFolderHash();
      }
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
    long lResult = -1;
    ADDON::CScraper::UniqueIDs uniqueIDs;
    // The ids a refresh looks the show up by are those it has, which an identifier in its name
    // may contradict, and are those its details were already read by
    if (!m_uniqueIDs.empty() && !(pURL && pURL->HasUrls()))
      uniqueIDs = m_uniqueIDs;
    else if (CFilenameAttributes(movieTitle, &m_regexpCache)
                 .GetIdentifier(identifierType, identifier))
      uniqueIDs = {{identifierType, identifier}};
    if (!uniqueIDs.empty())
    {
      if (GetDetails(pItem, uniqueIDs, url, info2,
                     (result == InfoType::COMBINED || result == InfoType::OVERRIDE) ? loader.get()
                                                                                    : nullptr,
                     pDlgProgress))
      {
        if ((lResult = AddVideo(pItem, info2, false, useLocal)) < 0)
          return InfoRet::INFO_ERROR;

        if (fetchEpisodes && !files.empty())
        {
          const InfoRet ret =
              RetrieveInfoForEpisodes(pItem, lResult, files, info2, useLocal, pDlgProgress, true);
          if (ret == InfoRet::ADDED)
          {
            setFolderHash();
            return InfoRet::ADDED;
          }
        }
        else if (files.empty())
        {
          setFolderHash();
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

    // Without the show's details there is nothing to add its episodes to. The folder isn't hashed,
    // so that the next scan tries again.
    if (!GetDetails(pItem, {}, url, info2,
                    (result == InfoType::COMBINED || result == InfoType::OVERRIDE) ? loader.get()
                                                                                   : nullptr,
                    pDlgProgress))
    {
      // A call a stop gave up isn't a failed look-up
      if (m_bStop)
        return InfoRet::CANCELLED;
      ++m_failedLookups; // found, but its details couldn't be retrieved
      return InfoRet::NOT_FOUND;
    }

    if ((lResult = AddVideo(pItem, info2, false, useLocal)) < 0)
      return InfoRet::INFO_ERROR;

    if (fetchEpisodes && !files.empty())
    {
      const InfoRet ret =
          RetrieveInfoForEpisodes(pItem, lResult, files, info2, useLocal, pDlgProgress, true);
      if (ret == InfoRet::ADDED)
        setFolderHash();
    }
    else if (files.empty())
    {
      setFolderHash();
    }
    return InfoRet::ADDED;
  }

  namespace
  {
  void ResolveBlurayPlaylist(CFileItem* item, CFileItemList& items); // Forward declaration

  // A bluray needing playlist resolution, ie. a disc image holding a BDMV or a BDMV folder itself
  bool IsBluray(const std::string& path)
  {
    return ::UTILS::DISCS::IsBlurayDiscImage(path) || URIUtils::IsBDFile(path);
  }

  void ReadExtrasPlaylists(const std::string& path)
  {
    CFileItem disc{path, false};
    disc.GetVideoInfoTag()->GetAssetInfo().SetType(VideoAssetType::EXTRA);
    CFileItemList named;
    CDiscDirectoryHelper::GetOrShowPlaylistSelection(disc, named, MenuDecision::SILENT);
  }

  // A part without a playlist has one chosen for it, unless choosePlaylists is false
  bool ResolveBlurayStack(CFileItem* item, bool choosePlaylists = true)
  {
    const std::string originalPath{item->GetDynPath()};

    std::vector<std::string> paths;
    if (!CStackDirectory::GetPaths(originalPath, paths) || paths.empty())
      return false;

    std::vector<std::string> playlistPaths;
    playlistPaths.reserve(paths.size());
    std::vector<size_t> fileParts; // Indices of the non-bluray members, durations retrieved below
    std::vector<std::chrono::milliseconds> partDurations(paths.size(),
                                                         std::chrono::milliseconds{0});
    CStreamDetails streamDetails;
    int totalDuration{0};
    size_t blurays{0};

    for (size_t partIndex{0}; const std::string& path : paths)
    {
      const size_t part{partIndex++};

      // Only resolve blurays
      const bool playlistChosen{URIUtils::GetBlurayPlaylistFromPath(path) > -1};
      if (!IsBluray(path) && !playlistChosen)
      {
        fileParts.emplace_back(part);
        playlistPaths.emplace_back(path);
        continue;
      }
      if (!playlistChosen && !choosePlaylists)
      {
        playlistPaths.emplace_back(path);
        continue;
      }

      CFileItem partItem{*item};
      partItem.SetPath(path);
      partItem.SetDynPath(path);

      partItem.GetVideoInfoTag()->m_streamDetails.Reset();
      bool resolved;
      if (playlistChosen)
        resolved = CDiscDirectoryHelper::ReadResolvedPlaylist(partItem); // Refresh
      else
      {
        CFileItemList partItems;
        ResolveBlurayPlaylist(&partItem, partItems);
        resolved = !partItems.IsEmpty();
      }

      if (!resolved)
      {
        CLog::LogF(LOGERROR, "Unable to resolve a bluray playlist for {} of {}",
                   CURL::GetRedacted(path), CURL::GetRedacted(originalPath));
        playlistPaths.emplace_back(path);
        continue;
      }

      const CStreamDetails& partDetails{partItem.GetVideoInfoTag()->m_streamDetails};

      // Take the first streamdetails but update the duration
      if (blurays == 0)
        streamDetails = partDetails;
      const int partDuration{partDetails.GetVideoDuration()}; // seconds
      totalDuration += partDuration;
      partDurations[part] = std::chrono::seconds{partDuration};

      ++blurays;
      playlistPaths.emplace_back(partItem.GetDynPath());
    }

    // Not a stack of blurays at all, so there is nothing to do
    if (blurays == 0)
      return false;

    // Durations of any plain file members
    for (const size_t part : fileParts)
    {
      int duration{0}; // milliseconds
      if (CScanProbe::GetFileDuration(paths[part], duration) && duration > 0)
      {
        totalDuration += duration / 1000;
        partDurations[part] = std::chrono::milliseconds{duration};
      }
      else
        CLog::LogF(LOGDEBUG, "Unable to get the duration of {} of {}",
                   CURL::GetRedacted(paths[part]), CURL::GetRedacted(originalPath));
    }

    const size_t knownDurations{static_cast<size_t>(
        std::ranges::count_if(partDurations, [](const std::chrono::milliseconds duration)
                              { return duration > std::chrono::milliseconds{0}; }))};

    std::string stackPath;
    if (!CStackDirectory::ConstructStackPath(playlistPaths, stackPath))
      return false;

    item->SetDynPath(stackPath);
    CVideoInfoTag* tag{item->GetVideoInfoTag()};
    tag->SetFileNameAndPath(stackPath);

    // Where each part ends, ie. the stack times. Logged per part as the order matters and a wrong
    // boundary is otherwise only visible as mis-seeking during playback
    std::vector<std::chrono::milliseconds> times;
    times.reserve(partDurations.size());
    std::chrono::milliseconds endTime{0};
    for (size_t part{0}; part < partDurations.size(); ++part)
    {
      endTime += partDurations[part];
      times.emplace_back(endTime);
      CLog::LogF(LOGDEBUG, "Part {} of {} ({}) lasts {}ms and ends at {}ms", part + 1, paths.size(),
                 CURL::GetRedacted(paths[part]), partDurations[part].count(), endTime.count());
    }

    // Only describe the stack when every part contributed a duration
    if (knownDurations == paths.size())
    {
      if (totalDuration > 0)
        streamDetails.SetVideoDuration(0, totalDuration);

      // Nfo streamdetails describe the stack as a whole, so they are what it keeps
      if (!tag->HasNFOStreamDetails())
        tag->m_streamDetails = streamDetails;

      // Record where each part ends, so that playback does not have to derive the durations again
      // (a resolved bluray:// playlist cannot be demuxed for its duration)
      LIBRARY::SetStackTimes(stackPath, times);
    }
    else
      CLog::LogF(LOGDEBUG,
                 "Only {} of {} parts of {} have a duration, so neither the streamdetails nor the "
                 "stack times can be recorded",
                 knownDurations, paths.size(), CURL::GetRedacted(originalPath));

    CLog::LogF(LOGDEBUG, "Resolved {} of {} parts of {} to {} ({}s total)", blurays, paths.size(),
               CURL::GetRedacted(originalPath), CURL::GetRedacted(stackPath), totalDuration);

    item->SetProperty(ITEM::PROPERTY::ORIGINAL_LISTITEM_URL, originalPath);
    return true;
  }

  // Rebuilds a stack from the playlists an nfo recorded for its parts, matching on the part paths
  // rather than their order so that a part that has been added, removed or renamed cannot end up
  // with another part's playlist. Returns whether any playlist was applied
  bool ApplyStackParts(CFileItem* item, const std::vector<XFILE::StackPartPlaylist>& stackParts)
  {
    if (stackParts.empty())
      return false;

    const std::string originalPath{item->GetDynPath()};

    std::vector<std::string> paths;
    if (!CStackDirectory::GetPaths(originalPath, paths))
      return false;

    std::vector<std::string> playlistPaths;
    playlistPaths.reserve(paths.size());
    size_t applied{0};

    for (const std::string& path : paths)
    {
      const auto part{std::ranges::find_if(stackParts,
                                           [&path](const XFILE::StackPartPlaylist& stackPart)
                                           { return URIUtils::PathEquals(stackPart.file, path); })};
      if (part == stackParts.end() || !IsBluray(path))
      {
        CLog::LogF(LOGDEBUG, "No playlist to apply to {} of {}", CURL::GetRedacted(path),
                   CURL::GetRedacted(originalPath));
        playlistPaths.emplace_back(path);
        continue;
      }

      playlistPaths.emplace_back(URIUtils::GetBlurayPlaylistPath(path, part->playlist));
      ++applied;
    }

    std::string stackPath;
    if (applied == 0 || !CStackDirectory::ConstructStackPath(playlistPaths, stackPath))
      return false;

    item->SetDynPath(stackPath);
    item->GetVideoInfoTag()->SetFileNameAndPath(stackPath);

    CLog::LogF(LOGDEBUG, "Applied {} of {} recorded playlists to {}", applied, paths.size(),
               CURL::GetRedacted(originalPath));
    return true;
  }

  // Populates CFileItemList items with every candidate (version) bluray playlist found for item (if any).
  // item is updated in place to the first (main) playlist; any further items are additional playlists presumed to
  // be other versions of the same movie (only populated when returned by CDiscDirectoryHelper when
  // not SimilarVideoScanAction::NONE).
  void ResolveBlurayPlaylist(CFileItem* item, CFileItemList& items)
  {
    // Resolve each part of stack individually
    if (URIUtils::IsStack(item->GetDynPath()))
    {
      ResolveBlurayStack(item);
      return;
    }

    if (IsBluray(item->GetPath()))
    {
      if (CDiscDirectoryHelper::GetOrShowPlaylistSelection(*item, items, MenuDecision::SILENT) &&
          !items.IsEmpty())
        *item = *items[0];
    }
  }

  // An edition extracted from the filename is applied only when an asset title hasn't been set yet (NFO wins).
  void ApplyEdition(CFileItem* item, const std::string& editionFromFilename)
  {
    if (editionFromFilename.empty())
      return;

    // Creation of a tag if one doesn't exist yet is on purpose
    CVideoInfoTag* tag{item->GetVideoInfoTag()};
    if (tag && tag->GetAssetInfo().GetTitle().empty())
      tag->GetAssetInfo().SetTitle(editionFromFilename);
  }

  // Whether a path points at a disc in any of the forms one takes - an image, a BDMV/VIDEO_TS
  // structure or a bluray:// playlist.
  bool IsDisc(const std::string& path)
  {
    return URIUtils::IsDiscImage(path) || URIUtils::IsOpticalMediaFile(path) ||
           URIUtils::IsBlurayPath(path);
  }

  // Whether a movie is a disc, or a stack holding one, whose playlists are chosen with those the
  // library already has
  bool IsDiscMovie(const std::string& path)
  {
    return IsDisc(path) || (URIUtils::IsStack(path) && CStackDirectory::HasDiscPart(path));
  }

  } // unnamed namespace

  void CVideoInfoScanner::LoadVideoVersionTypes()
  {
    // Editions known to the library (both the built-in ones and any the user has added) are the
    // only ones worth looking for, and they don't change during a scan, so the list is cached.
    if (m_videoVersionTypesCached)
      return;
    m_videoVersionTypesCached = true;

    CFileItemList types;
    if (m_database.GetVideoVersionTypes(VideoDbContentType::MOVIES, VideoAssetType::VERSION, types))
    {
      const std::string standardEdition{
          CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(
              VIDEO_VERSION_ID_DEFAULT)};

      for (const auto& type : types)
      {
        // The default edition is what an unrecognised movie gets anyway
        if (const std::string & name{type->GetLabel()}; !name.empty() && name != standardEdition)
          m_videoVersionTypes.emplace_back(name);
      }
    }
  }

  std::string CVideoInfoScanner::FindEditionInFolderName(const std::string& folderName) const
  {
    if (folderName.empty())
      return {};

    const std::string edition{UTILS::FindEditionInName(folderName, m_videoVersionTypes)};
    if (edition.empty())
      CLog::LogF(LOGDEBUG, "No edition recognised in folder '{}' (of the {} known)", folderName,
                 m_videoVersionTypes.size());
    else
      CLog::LogF(LOGDEBUG, "Derived edition '{}' from folder '{}'", edition, folderName);

    return edition;
  }

  bool CVideoInfoScanner::GetMovieIdentifier(const CFileItem& item,
                                             bool dirNames,
                                             const CFilenameAttributes& filenameAttributes,
                                             std::string& identifierType,
                                             std::string& identifier) const
  {
    return filenameAttributes.GetIdentifier(identifierType, identifier) ||
           CFilenameAttributes(item.GetMovieName(dirNames), &m_regexpCache)
               .GetIdentifier(identifierType, identifier);
  }

  CInfoScanner::InfoRet CVideoInfoScanner::RetrieveInfoForMovie(CFileItem* pItem,
                                                                bool bDirNames,
                                                                ScraperPtr& info2,
                                                                bool useLocal,
                                                                CScraperUrl* pURL,
                                                                CGUIDialogProgress* pDlgProgress,
                                                                MovieLookup* lookup /* = nullptr */)
  {
    if (pItem->IsFolder() || !IsVideo(*pItem) || pItem->IsNFO() ||
        (PLAYLIST::IsPlayList(*pItem) && !URIUtils::HasExtension(pItem->GetPath(), ".strm")))
      return InfoRet::NOT_NEEDED;

    if (ProgressCancelled(pDlgProgress, 198, pItem->GetLabel()))
      return InfoRet::CANCELLED;

    // A movie already in the library keeps its details.
    // This means adding a version to an existing nfo is not processed but
    // this doesn't cause a hash change anyway. A refresh will re-read.
    if ((m_folderMovies ? m_database.GetMovieId(pItem->GetDynPath(), *m_folderMovies)
                        : m_database.GetMovieId(pItem->GetDynPath())) > 0)
      return InfoRet::HAVE_ALREADY;

    if (m_handle)
      m_handle->SetText(pItem->GetMovieName(bDirNames));

    const CFilenameAttributes filenameAttributes(URIUtils::GetFileName(pItem->GetPath()),
                                                 &m_regexpCache);

    LoadVideoVersionTypes();
    const std::string editionFromFilename{
        lookup ? lookup->edition
               : MovieEdition(*pItem, bDirNames, m_useFolderNames, m_regexpCache)};

    // Handle sets, filename-derived edition, bluray playlist(s) (if any) and add to the library
    const auto HandleMovieSetAndVersions = [this, pItem, &info2, bDirNames, useLocal,
                                            &editionFromFilename, lookup]() -> InfoRet
    {
      // A look-up has read the set and the movie's art and streams already
      const bool prepared{lookup && lookup->prepared};
      if ((prepared ? lookup->setUpdated : UpdateMovieSet(*pItem->GetVideoInfoTag())) &&
          !AddSet(pItem->GetVideoInfoTag()->m_set))
        return InfoRet::INFO_ERROR;

      ApplyEdition(pItem, editionFromFilename);

      // Determine bluray playlist(s) (if possible)
      // Also populates streamdetails if playlist(s) found
      CFileItemList blurayItems;
      if (!prepared)
        ResolveBlurayPlaylist(pItem, blurayItems);
      if (!blurayItems.IsEmpty())
        return AddBlurayPlaylistVersions(blurayItems, info2, bDirNames, useLocal);

      const int movieDbId{static_cast<int>(
          prepared ? WriteVideo(pItem, *lookup->prepared, bDirNames, nullptr, false)
                   : AddVideo(pItem, info2, bDirNames, useLocal))};
      if (movieDbId < 0)
        return InfoRet::INFO_ERROR;

      if ((m_similarVideoAction == SimilarVideoScanAction::ASK ||
           m_similarVideoAction == SimilarVideoScanAction::AUTO))
      {
        [[maybe_unused]] const auto [result, targetMovieDbId] =
            ProcessVideoVersion(VideoDbContentType::MOVIES, movieDbId);
        switch (result)
        {
          case VersionConversionResult::SUCCESS:
            RemovePartNumberFromTitle(targetMovieDbId, VideoDbContentType::MOVIES, m_database);
            // Added as a movie, it is now a version of another
            m_summary.added.back().type = ScanSummary::Type::VERSION;
            return InfoRet::HAVE_ALREADY;
          case VersionConversionResult::NOT_NEEDED:
          case VersionConversionResult::CANCELLED:
          case VersionConversionResult::NOT_ALLOWED:
            return InfoRet::ADDED;
          case VersionConversionResult::FAILED:
            return InfoRet::INFO_ERROR;
        }
      }

      // No version processing
      return InfoRet::ADDED;
    };

    InfoType result = InfoType::NONE;
    CScraperUrl scrUrl;

    // handle .nfo files
    std::unique_ptr<IVideoInfoTagLoader> loader;
    CFileItem item{*pItem};
    if (lookup)
    {
      result = lookup->nfoResult;
      loader = std::move(lookup->loader);
      if (result == InfoType::FULL)
        item = std::move(lookup->item);
    }
    else if (useLocal)
      std::tie(result, loader) = ReadInfoTag(item, info2, bDirNames, true);
    if (result == InfoType::FULL)
    {
      // Add the movie entry
      int movieId{-1};
      bool mergedIntoExistingMovie{false};
      item.SetProperty("from_nfo", true);

      // Only an export's nfo adding the movie is the record of its extras; a refresh reads it again
      const bool restoring{!m_deferMovieExtras &&
                           item.GetVideoInfoTag()->GetAssetInfo().GetType() !=
                               VideoAssetType::UNKNOWN};

      // Refreshing the recorded playlists gives the durations, and so the stack times, that the
      // nfo does not hold. A part the nfo records no playlist for stays the disc.
      if (URIUtils::IsStack(item.GetDynPath()) && ApplyStackParts(&item, loader->GetStackParts()))
        ResolveBlurayStack(&item, false);

      CVideoInfoTag* tag{item.GetVideoInfoTag()};
      if (tag->HasVideoVersions())
      {
        // See if movie already in library
        CFileItemList items;
        using enum CVideoDatabase::MatchingMask;
        m_database.GetSameVideoItems(item, items, UniqueId | (bDirNames ? Path : None));
        erase_if(items, [tag](const std::shared_ptr<CFileItem>& current)
                 { return tag->HasConflictingUniqueID(*current->GetVideoInfoTag()); });
        if (!items.IsEmpty())
        {
          // Movie already exists
          movieId = items[0]->GetVideoInfoTag()->m_iDbId;
          mergedIntoExistingMovie = true;
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
        ApplyEdition(&item, editionFromFilename);

        movieId =
            static_cast<int>(lookup && lookup->prepared
                                 ? WriteVideo(&item, *lookup->prepared, bDirNames, nullptr, false)
                                 : AddVideo(&item, info2, bDirNames, true));
        if (movieId < 0)
          return InfoRet::INFO_ERROR;

        // Deal with set
        if (UpdateMovieSet(*item.GetVideoInfoTag()) && !AddSet(item.GetVideoInfoTag()->m_set))
          return InfoRet::INFO_ERROR;
      }

      // Look for default version
      int defaultVersionFileId{-1};
      if (tag->IsDefaultVideoVersion())
        defaultVersionFileId = tag->m_iFileId; // Updated in AddMovie()

      // Look for versions and extras (ie. subsequent <movie> entries in the .nfo file), each
      // marked by its <videoassettype>. Reuse the loader.
      int index{1};
      while (true)
      {
        // The item is reused for every entry, so must not keep the art found for the one before
        tag->Reset();
        item.ClearArt();
        const InfoType versionResult{loader->LoadVersion(++index, *tag)};
        if (versionResult == InfoType::NONE)
          break; // No further <movie> entries
        if (versionResult != InfoType::FULL)
          continue; // Entry cannot stand alone as a version - skip it, but keep looking

        // An entry holds the movie's art, which an extra shows none of. Its own is beside it.
        if (tag->GetAssetInfo().GetType() == VideoAssetType::EXTRA)
        {
          tag->m_strPictureURL.Clear();
          tag->m_fanart.Clear();
        }

        // A <playlist> nfo element identifies the disc playlist the info belongs to.
        // The item is reused for every entry, so a version without one must not inherit the
        // playlist of the entry before it
        if (const int playlist{loader->GetBlurayPlaylist()}; playlist > -1)
          item.SetProperty(ITEM::PROPERTY::BLURAY_PLAYLIST, playlist);
        else
          item.ClearProperty(ITEM::PROPERTY::BLURAY_PLAYLIST);

        // Keep properties only if advancedsettings.xml says so
        if (!m_advancedSettings->m_bVideoLibraryImportWatchedState)
          tag->ResetPlayCount();
        if (!m_advancedSettings->m_bVideoLibraryImportResumePoint)
          tag->SetResumePoint(CBookmark());

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

      // Set default version
      if (defaultVersionFileId > -1)
        m_database.SetDefaultVideoVersion(VideoDbContentType::MOVIES, movieId,
                                          defaultVersionFileId);

      // For the rest of the scan the movie's extras are those its nfos record
      if (restoring)
        m_extras.SetRestoredFromNfo(movieId);

      return mergedIntoExistingMovie ? InfoRet::HAVE_ALREADY : InfoRet::ADDED;
    }

    if (lookup)
    {
      if (lookup->found)
      {
        *pItem = std::move(lookup->item);
        return HandleMovieSetAndVersions();
      }
      return LookupNotFound(*lookup, pDlgProgress);
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
      if (item.HasVideoInfoTag())
      {
        const CVideoInfoTag* tag = item.GetVideoInfoTag();
        movieTitle = tag->GetTitle();
        movieYear = tag->GetYear(); // movieYear is expected to be >= 0
      }
      else
      {
        CLog::Log(LOGWARNING,
                  "VideoInfoScanner: RetrieveInfoForMovie/ReadInfoTag did not create a video info "
                  "tag for {}",
                  CURL::GetRedacted(item.GetDynPath()));
      }
    }

    std::string identifierType;
    std::string identifier;
    // The ids a refresh looks the movie up by come first: an identifier in the file or folder
    // name may name another film, and the ids are those its details were already read by
    if (m_uniqueIDs.empty() &&
        GetMovieIdentifier(*pItem, bDirNames, filenameAttributes, identifierType, identifier))
    {
      const ADDON::CScraper::UniqueIDs uniqueIDs{{identifierType, identifier}};
      if (GetDetails(pItem, uniqueIDs, url, info2,
                     (result == InfoType::COMBINED || result == InfoType::OVERRIDE) ? loader.get()
                                                                                    : nullptr,
                     pDlgProgress))
        return HandleMovieSetAndVersions();
    }

    if (!m_uniqueIDs.empty() && !(pURL && pURL->HasUrls()) &&
        GetDetails(pItem, m_uniqueIDs, url, info2,
                   (result == InfoType::COMBINED || result == InfoType::OVERRIDE) ? loader.get()
                                                                                  : nullptr,
                   pDlgProgress))
      return HandleMovieSetAndVersions();

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
      return HandleMovieSetAndVersions();

    if (m_bStop)
      return InfoRet::CANCELLED;
    //! @todo This is not strictly correct as we could fail to download information here or error, or be cancelled
    ++m_failedLookups; // found, but its details couldn't be retrieved
    return InfoRet::NOT_FOUND;
  }

  void CVideoInfoScanner::ReadDiscsAhead(const ItemLookups& batch)
  {
    if (batch.content != ContentType::MOVIES)
      return;

    // A disc already in the library is not read, nor one a refresh looks up in turn
    for (size_t i = 0; i < batch.candidates.size(); ++i)
    {
      if (const auto& candidate{batch.candidates[i]}; candidate && candidate->lookUpAlongside)
      {
        const CFileItem& item{*(*batch.items)[static_cast<int>(i)]};
        // Held until its look-up has read the movie's nfo, which may give all the scan needs
        if (IsDiscByName(item.GetPath()))
          m_discReadAhead.Queue(item.GetPath(), [this, item] { ReadMovieDisc(item); }, true);
      }
    }
  }

  void CVideoInfoScanner::ReadExtrasDiscsAhead(DirectoryScan& scan)
  {
    // Once, as the folder holding it queues them before its turn, and not for an unchanged
    // listing, whose extras are added only where something below them has changed
    if (std::exchange(scan.extrasReadAhead, true) || scan.skip ||
        !m_advancedSettings->m_parseBlurayProjectFile)
      return;

    for (const auto& item : scan.items)
    {
      if (AddsAsExtras(scan, *item))
        m_discReadAhead.Queue(item->GetPath(),
                              [this, folder = item->GetPath()] { ReadExtrasDiscs(folder); });
    }
  }

  void CVideoInfoScanner::ReadMovieDisc(const CFileItem& movie) const
  {
    if (!IsBluray(movie.GetPath()))
      return;

    // As RetrieveInfoForMovie() and AddMovieDiscExtras() do
    CFileItemList playlists;
    if (!CDiscDirectoryHelper::GetOrShowPlaylistSelection(movie, playlists, MenuDecision::SILENT))
      return;

    for (const auto& playlist : playlists)
    {
      CFileItem chosen{*playlist};
      CDiscDirectoryHelper::ReadResolvedPlaylist(chosen);
    }
    if (!m_ignoreVideoExtras && !m_deferMovieExtras && !playlists.IsEmpty() &&
        m_advancedSettings->m_parseBlurayProjectFile)
    {
      ReadExtrasPlaylists(playlists[0]->GetDynPath());
    }
  }

  void CVideoInfoScanner::ReadExtrasDiscs(const std::string& folder) const
  {
    UTILS::EnumerateVideoExtras(folder,
                                [](const std::shared_ptr<CFileItem>& item)
                                {
                                  if (IsBluray(item->GetPath()))
                                    ReadExtrasPlaylists(item->GetPath());
                                });
  }

  void CVideoInfoScanner::ReadEpisodeDiscsAhead(const std::vector<std::string>& discs)
  {
    for (const std::string& disc : discs)
      m_discReadAhead.Queue(disc, [this, disc] { ReadEpisodeDisc(disc); });
  }

  std::vector<std::string> CVideoInfoScanner::ShowDiscs(const std::string& show)
  {
    // A show scanned before may be unchanged, so isn't listed until its turn
    if (m_pathTable->Record(show))
      return {};

    CFileItemList items;
    std::vector<std::pair<std::string, FolderRecord>> records;
    CollectShowFiles(show, items, records, false);
    // Most shows have no disc, so their files aren't parsed for episodes here
    if (std::ranges::none_of(items, [](const auto& item) { return IsDiscByName(item->GetPath()); }))
      return {};

    EPISODELIST files;
    EnumerateShowFiles(items, files);
    std::vector<const EPISODE*> episodes;
    std::ranges::transform(files, std::back_inserter(episodes),
                           [](const EPISODE& file) { return &file; });
    return EpisodeDiscs(episodes);
  }

  void CVideoInfoScanner::ReadEpisodeDisc(const std::string& disc) const
  {
    if (!IsBluray(disc))
      return;

    // Listed without CDirectory, as a disc on which no episode is found is no error
    const CURL episodes{URIUtils::GetBlurayAllEpisodesPath(disc)};
    if (const std::unique_ptr<IDirectory> directory{CDirectoryFactory::Create(episodes)}; directory)
    {
      CFileItemList playlists;
      directory->GetDirectory(episodes, playlists);
    }
  }

  std::string CVideoInfoScanner::MovieEdition(const CFileItem& item,
                                              bool bDirNames,
                                              bool useFolderNames,
                                              KODI::REGEXP::RegExpCache& regexps) const
  {
    //! @todo: do something about edition values slightly different from db due to
    //! available filesystem characters. ex. "directors cut" in file name vs "Director's Cut"
    std::string edition{
        CFilenameAttributes(URIUtils::GetFileName(item.GetPath()), &regexps).GetEdition()};

    // Neither a disc nor a stack may have a filename that says anything about the movie, so when
    // each movie is in its own folder the edition is taken from the folder name instead - either
    // as an attribute pair (ex. "[edition=Director's Cut]") or, failing that, as an edition name
    // spelt out in the folder name itself
    if (const std::string & path{item.GetPath()}; edition.empty() &&
                                                  (bDirNames || useFolderNames) &&
                                                  (URIUtils::IsStack(path) || IsDisc(path)))
    {
      if (const std::string folderName{item.GetMovieName(true)}; !folderName.empty())
      {
        const CFilenameAttributes folderAttributes(folderName, &regexps);
        edition = folderAttributes.GetEdition();
        if (edition.empty())
          edition = FindEditionInFolderName(folderName);
      }
    }
    return edition;
  }

  bool CVideoInfoScanner::CanLookUpMovieAlongside(const CFileItem& item) const
  {
    // A refresh's ids are looked up by in turn
    return !item.IsFolder() && IsVideo(item) && !item.IsNFO() && !PLAYLIST::IsPlayList(item) &&
           !item.IsPlugin() && m_uniqueIDs.empty();
  }

  void CVideoInfoScanner::LookUpMovie(const CFileItem& source,
                                      bool bDirNames,
                                      const ScraperPtr& info2,
                                      bool useLocal,
                                      bool useFolderNames,
                                      MovieLookup& lookup) const
  {
    KODI::REGEXP::RegExpCache regexps;
    std::optional<std::pair<std::string, CStreamDetails>> lastProbed;
    lookup.edition = MovieEdition(source, bDirNames, useFolderNames, regexps);

    CFileItem item{source};
    if (useLocal)
      std::tie(lookup.nfoResult, lookup.loader) = ReadInfoTag(item, info2, bDirNames, true);

    if (lookup.nfoResult == InfoType::FULL)
      m_discReadAhead.Drop(source.GetPath());
    else
      m_discReadAhead.Allow(source.GetPath());

    if (lookup.nfoResult == InfoType::FULL)
    {
      item.SetProperty("from_nfo", true);
      // A movie the nfo records versions of may join one already in the library
      if (!item.GetVideoInfoTag()->HasVideoVersions() && !URIUtils::IsStack(item.GetDynPath()) &&
          !IsDiscMovie(source.GetDynPath()))
      {
        ApplyEdition(&item, lookup.edition);
        lookup.prepared = PrepareVideo(&item, info2, bDirNames, true, nullptr, false,
                                       ContentType::NONE, regexps, lastProbed);
      }
      lookup.item = std::move(item);
      return;
    }

    CFileItem movie{source};
    if (!FindVideoAhead(source, item, IdentifierIn::FILE_NAME, bDirNames, info2, lookup, movie,
                        regexps))
      return;

    lookup.found = true;
    if (IsDiscMovie(source.GetDynPath()))
    {
      lookup.item = std::move(movie);
      return;
    }
    lookup.setUpdated = UpdateMovieSet(*movie.GetVideoInfoTag());
    ApplyEdition(&movie, lookup.edition);
    lookup.prepared = PrepareVideo(&movie, info2, bDirNames, useLocal, nullptr, false,
                                   ContentType::NONE, regexps, lastProbed);
    lookup.item = std::move(movie);
  }

  bool CVideoInfoScanner::FindVideoAhead(const CFileItem& source,
                                         const CFileItem& item,
                                         IdentifierIn identifierIn,
                                         bool bDirNames,
                                         const ScraperPtr& info2,
                                         VideoLookup& lookup,
                                         CFileItem& found,
                                         KODI::REGEXP::RegExpCache& regexps) const
  {
    CScraperUrl scrUrl;
    const CScraperUrl* pURL{nullptr};
    if (lookup.nfoResult == InfoType::URL || lookup.nfoResult == InfoType::COMBINED)
    {
      scrUrl = lookup.loader->ScraperUrl();
      pURL = &scrUrl;
    }
    IVideoInfoTagLoader* const overlay{lookup.nfoResult == InfoType::COMBINED ||
                                               lookup.nfoResult == InfoType::OVERRIDE
                                           ? lookup.loader.get()
                                           : nullptr};

    std::string title{source.GetMovieName(bDirNames)};
    int year{-1};
    if (lookup.nfoResult == InfoType::TITLE && item.HasVideoInfoTag())
    {
      title = item.GetVideoInfoTag()->GetTitle();
      year = item.GetVideoInfoTag()->GetYear();
    }

    CScraperUrl url;
    std::string identifierType;
    std::string identifier;
    const std::string named{
        identifierIn == IdentifierIn::TITLE ? title : URIUtils::GetFileName(source.GetPath())};
    // As GetMovieIdentifier()
    if ((CFilenameAttributes(named, &regexps).GetIdentifier(identifierType, identifier) ||
         (identifierIn == IdentifierIn::FILE_NAME &&
          CFilenameAttributes(source.GetMovieName(bDirNames), &regexps)
              .GetIdentifier(identifierType, identifier))) &&
        LookUpDetails(found, {{identifierType, identifier}}, url, info2, overlay))
      return true;

    if (pURL && pURL->HasUrls())
      url = *pURL;
    else
    {
      std::vector<CScraperUrl> results;
      try
      {
        const CScraperCalls::CRegistration call{m_scraperCalls};
        results = info2->FindMovie(title, year);
        lookup.searchResult = 1;
      }
      catch (const CScraperError&)
      {
        lookup.searchResult = -1; // failed, rather than finding nothing
      }
      lookup.searchFound = !results.empty();
      if (!lookup.searchFound)
        return false;
      url = results[0];
    }

    CLog::Log(LOGDEBUG, "VideoInfoScanner: Fetching url '{}' using {} scraper (content: '{}')",
              url.GetFirstThumbUrl(), info2->Name(), info2->Content());
    return LookUpDetails(found, {}, url, info2, overlay);
  }

  CInfoScanner::InfoRet CVideoInfoScanner::LookupNotFound(const VideoLookup& lookup,
                                                          CGUIDialogProgress* pDlgProgress)
  {
    if (const std::optional<int> searched{lookup.searchResult}; searched)
    {
      if (*searched <= 0)
      {
        if (m_bStop || !DownloadFailed(pDlgProgress))
        {
          m_bStop = true;
          return InfoRet::CANCELLED;
        }
        ++m_failedLookups;
        return InfoRet::NOT_FOUND;
      }
      if (!lookup.searchFound)
        return InfoRet::NOT_FOUND;
    }

    ++m_failedLookups;
    return InfoRet::NOT_FOUND;
  }

  void CVideoInfoScanner::LookUpShow(const CFileItem& source,
                                     bool bDirNames,
                                     const ScraperPtr& info2,
                                     bool useLocal,
                                     ShowLookup& lookup) const
  {
    KODI::REGEXP::RegExpCache regexps;

    CFileItem item{source};
    if (useLocal)
      std::tie(lookup.nfoResult, lookup.loader) = ReadInfoTag(item, info2, bDirNames, true);
    if (lookup.nfoResult == InfoType::FULL)
    {
      lookup.item = std::move(item);
      return;
    }

    CFileItem show{source};
    if (!FindVideoAhead(source, item, IdentifierIn::TITLE, bDirNames, info2, lookup, show, regexps))
      return;

    lookup.found = true;
    // A guide that can't be fetched now is fetched by the show's episodes, which report it
    if (const std::string & episodeGuide{show.GetVideoInfoTag()->m_strEpisodeGuide};
        !episodeGuide.empty() && !info2->IsLocalInformationOnly() && !m_bStop)
    {
      CScraperUrl guideUrl;
      guideUrl.ParseAndAppendUrlsFromEpisodeGuide(episodeGuide);
      const CScraperCalls::CRegistration call{m_scraperCalls};
      if (EPISODELIST episodes{info2->GetEpisodeList(guideUrl)}; !episodes.empty())
        lookup.guide = std::move(episodes);
    }
    lookup.item = std::move(show);
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
      return InfoRet::NOT_NEEDED;

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
      pItem->SetProperty("from_nfo", true);
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
    if (CFilenameAttributes(movieTitle, &m_regexpCache).GetIdentifier(identifierType, identifier))
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
    if (m_bStop)
      return InfoRet::CANCELLED;
    //! @todo This is not strictly correct as we could fail to download information here or error, or be cancelled
    ++m_failedLookups; // found, but its details couldn't be retrieved
    return InfoRet::NOT_FOUND;
  }

  CInfoScanner::InfoRet CVideoInfoScanner::RetrieveInfoForEpisodes(
      CFileItem* item,
      long showID,
      EPISODELIST& files,
      const ADDON::ScraperPtr& scraper,
      bool useLocal,
      CGUIDialogProgress* progress,
      bool alreadyHasArt /* = false */,
      const EPISODELIST* guide /* = nullptr */)
  {
    if (m_bStop || (progress && progress->IsCanceled()))
      return InfoRet::CANCELLED;

    CVideoInfoTag showInfo;
    m_database.GetTvShowInfo("", showInfo, showID);

    // A show can have several folders, and the database returns only one of them. Local
    // season art and actor thumbs belong to the folder being scanned.
    std::vector<std::string> showPaths;
    if (item->IsFolder() && m_database.GetPathsLinkedToTvShow(static_cast<int>(showID), showPaths))
    {
      const auto showPath =
          std::ranges::find_if(showPaths, [item](const std::string& path)
                               { return URIUtils::PathEquals(path, item->GetPath(), true); });
      if (showPath != showPaths.end())
        showInfo.m_strPath = *showPath;
    }

    InfoRet ret{OnProcessSeriesFolder(files, scraper, useLocal, showInfo, progress, guide)};

    // A show just added was written with its season art
    if (ret == InfoRet::ADDED && !alreadyHasArt)
    {
      KODI::ART::SeasonsArtwork seasonArt;
      m_database.GetTvShowSeasonArt(showID, seasonArt);

      // Only a season of the show's episodes asks the scraper for art again: the row for all
      // seasons, and a season without episodes, often have none to be had
      EpisodeFileMap episodes;
      m_database.GetEpisodeMap(static_cast<int>(showID), episodes);
      std::set<int> seasonsOfEpisodes;
      for (const auto& [_, episode] : episodes)
        seasonsOfEpisodes.insert(episode.season);
      if (std::ranges::any_of(seasonArt, [](const auto& i) { return i.second.empty(); }))
      {
        const bool seasonOfEpisodesWithoutArt{std::ranges::any_of(
            seasonArt, [&seasonsOfEpisodes](const auto& i)
            { return i.second.empty() && seasonsOfEpisodes.contains(i.first); })};
        if (!item->IsPlugin() && seasonOfEpisodesWithoutArt)
        {
          const CScraperCalls::CRegistration call{m_scraperCalls};
          CVideoInfoTag tag{showInfo};
          scraper->GetArtwork(tag); // Can alter other fields in the tag
          showInfo.m_strPictureURL = tag.m_strPictureURL; // We only want artwork
        }
        const CVideoInfoScannerArt::UseRemoteArtWithLocalScraper useRemoteArt{
            scraper->IsLocalInformationOnly() && m_advancedSettings->m_bNoRemoteArtWithLocalScraper
                ? CVideoInfoScannerArt::UseRemoteArtWithLocalScraper::NO
                : CVideoInfoScannerArt::UseRemoteArtWithLocalScraper::YES};
        CVideoInfoScannerArt::GetSeasonThumbs(
            showInfo, seasonArt, CVideoThumbLoader::GetArtTypes(MEDIA::TYPE::SEASON),
            useLocal && !item->IsPlugin(), useRemoteArt, &m_regexpCache);
        for (const auto& art : seasonArt | std::views::values)
          m_art.Cache(art);
        // The seasons in one transaction
        const bool ownTransaction{!m_database.InTransaction()};
        if (ownTransaction)
          m_database.BeginTransaction();
        for (const auto& [season, art] : seasonArt)
        {
          const int seasonID{m_database.AddSeason(static_cast<int>(showID), season)};
          m_database.SetArtForItem(seasonID, MEDIA::TYPE::SEASON, art);
        }
        if (ownTransaction && !m_database.CommitTransaction())
        {
          if (m_database.InTransaction())
            m_database.RollbackTransaction();
          CLog::Log(LOGERROR, "VideoInfoScanner: The season art of {} couldn't be stored",
                    CURL::GetRedacted(item->GetPath()));
        }
      }
    }
    return ret;
  }

  CVideoInfoScanner::EpisodeResult CVideoInfoScanner::EnumerateSeriesFolder(
      CFileItem* item, EPISODELIST& episodeList)
  {
    CFileItemList items;

    if (item->IsFolder())
    {
      const std::string& path{item->GetPath()};

      /*
       * Note: DoScan() will not remove this path as it's not recursing for tvshows.
       * Remove this path from the list we're processing in order to avoid hitting
       * it twice in the main loop.
       */
      m_pathsToScan.erase(path);

      const std::string cfg{FolderSettings(path)};
      const std::optional<FolderRecord> record{m_pathTable->Record(path)};
      bool unchanged{false};
      if (item->IsPlugin())
      {
        // if plugin has already calculated a hash for directory contents - use it
        // in this case we don't need to get directory listing from plugin for hash checking
        std::string hash;
        if (item->HasProperty("hash"))
        {
          hash = item->GetProperty("hash").asString();
          // A record's fields are separated by '|'
          StringUtils::Replace(hash, '|', '_');
        }
        unchanged = record && record->UnchangedListing(cfg, hash);
        if (!unchanged)
        {
          CDirectory::EnumerateDirectory(
              path, [&items](const std::shared_ptr<CFileItem>& item) { items.Add(item); },
              [](const std::shared_ptr<CFileItem>& folder)
              { return !HasNoMedia(folder->GetPath()); }, true,
              CServiceBroker::GetFileExtensionProvider().GetVideoExtensions(),
              hash.empty() ? DIR_FLAG_DEFAULTS : DIR_FLAG_NO_FILE_INFO);

          if (hash.empty())
          {
            // force sorting consistency to avoid hash mismatch between platforms
            // sort by filename as always present for any files, but keep case sensitivity
            items.Sort(SortBy::FILE, SortOrder::ASCENDING, SortAttributeNone);
            UTILS::GetPathHash(items, hash);
            unchanged = record && record->UnchangedListing(cfg, hash);
          }
        }
        m_showRecords.insert_or_assign(
            path, std::vector<std::pair<std::string, FolderRecord>>{
                      {path, FolderRecord{.cfg = cfg, .mtime = std::nullopt, .digest = hash}}});
      }
      else
      {
        if (record && record->UnchangedUnlisted(cfg, FolderTime(path),
                                                m_advancedSettings->m_bVideoLibraryUseFastHash))
        {
          if (record->noMedia)
            return EpisodeResult::NO_MEDIA;
          unchanged = true;
        }
        else if (FolderHasNoMedia(path, true))
        {
          ForgetListings(path);
          return EpisodeResult::NO_MEDIA;
        }
        else
          unchanged = !FolderChanged(path, {}, true);
      }

      if (unchanged)
      {
        CLog::Log(LOGDEBUG, "VideoInfoScanner: Skipping dir '{}' due to no change",
                  CURL::GetRedacted(path));
        ForgetListings(path);
        ++m_summary.unchangedDirectories;
        // update our dialog with our progress
        if (m_handle)
          OnDirectoryScanned(path);
        return EpisodeResult::NOT_CHANGED;
      }

      if (!record)
        CLog::Log(LOGDEBUG, "VideoInfoScanner: Scanning dir '{}' as not in the database",
                  CURL::GetRedacted(path));
      else
        CLog::Log(LOGDEBUG, "VideoInfoScanner: Rescanning dir '{}' due to change",
                  CURL::GetRedacted(path));

      if (m_bClean)
      {
        m_pathsToClean.insert(m_database.GetPathId(path));
        m_database.GetPathsForTvShow(m_database.GetTvShowId(path), m_pathsToClean);
      }

      if (!item->IsPlugin())
      {
        // Every file of the show, from a listing of each of its folders, which are recorded once
        // its episodes are added
        std::vector<std::pair<std::string, FolderRecord>> records;
        CollectShowFiles(path, items, records);
        m_showRecords.insert_or_assign(path, std::move(records));
        ForgetListings(path);
      }
    }
    else
    {
      CFileItemPtr newItem(new CFileItem(*item));
      items.Add(newItem);
    }

    if (!EnumerateShowFiles(items, episodeList))
      return EpisodeResult::NO_FILES;
    return episodeList.empty() ? EpisodeResult::NO_EPISODES : EpisodeResult::FOUND_EPISODES;
  }

  bool CVideoInfoScanner::EnumerateShowFiles(CFileItemList& items, EPISODELIST& episodeList)
  {
    const std::vector<std::string>& regexps = m_advancedSettings->m_tvshowExcludeFromScanRegExps;

    /*
    stack down any dvd folders
    need to sort using the full path since this is a collapsed recursive listing of all subdirs
    video_ts.ifo files should sort at the top of a dvd folder in ascending order

    /foo/bar/video_ts.ifo
    /foo/bar/vts_x_y.ifo
    /foo/bar/vts_x_y.vob
    */

    // since we're doing this now anyway, should other items be stacked?
    items.Sort(SortBy::PATH, SortOrder::ASCENDING);

    // If found VIDEO_TS.IFO, INDEX.BDMV or INDEX.BDM then we are dealing with DVD, Blu-ray or
    // AVCHD files on disc somewhere in the directory tree. Assume that all other files/folders in
    // the same folder with VIDEO_TS or BDMV can be ignored.
    // THere can be a BACKUP/INDEX.BDMV which needs to be ignored (and broke the old while loop here)

    // A Blu-ray's index is INDEX.BDMV, and an AVCHD disc's INDEX.BDM
    const auto isIndex = [](const std::string& upperFile)
    { return upperFile == "INDEX.BDMV" || upperFile == "INDEX.BDM"; };

    // A local Windows path is separated by backslashes. INDEX.BDM is a prefix of both indexes.
    const auto isBackupIndex = [](const std::string& upperPath)
    {
      return upperPath.find("BACKUP/INDEX.BDM") != std::string::npos ||
             upperPath.find("BACKUP\\INDEX.BDM") != std::string::npos;
    };

    // Get folders to remove
    std::vector<std::string> foldersToRemove;
    for (const auto& item : items)
    {
      const std::string file = StringUtils::ToUpper(item->GetPath());
      if (file.find("VIDEO_TS.IFO") != std::string::npos)
        foldersToRemove.emplace_back(StringUtils::ToUpper(URIUtils::GetDirectory(file)));
      if (isIndex(URIUtils::GetFileName(file)) && !isBackupIndex(file))
        foldersToRemove.emplace_back(
            StringUtils::ToUpper(URIUtils::GetParentPath(URIUtils::GetDirectory(file))));
    }

    // Remove folders
    erase_if(items,
             [&](const std::shared_ptr<CFileItem>& i)
             {
               const std::string fileAndPath(StringUtils::ToUpper(i->GetPath()));
               std::string file;
               std::string path;
               URIUtils::Split(fileAndPath, path, file);
               return (std::count_if(foldersToRemove.begin(), foldersToRemove.end(),
                                     [&](const std::string& removePath)
                                     { return path.rfind(removePath, 0) == 0; }) > 0) &&
                      file != "VIDEO_TS.IFO" && (!isIndex(file) || isBackupIndex(fileAndPath));
             });

    bool hasEpisodeCandidates = false;

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
      if (CUtil::ExcludeFileOrFolder(items[i]->GetPath(), regexps, &m_regexpCache))
        continue;

      hasEpisodeCandidates = true;

      /*
       * Check if the media source has already set the season and episode or original air date in
       * the VideoInfoTag. If it has, do not try to parse any of them from the file path to avoid
       * any false positive matches.
       */
      if (ProcessItemByVideoInfoTag(items[i].get(), episodeList))
        continue;

      if (!CEpisodeUtils::EnumerateEpisodeItem(items[i].get(), episodeList, &m_regexpCache))
        CLog::Log(LOGDEBUG, "VideoInfoScanner: Could not enumerate file {}",
                  CURL::GetRedacted(items[i]->GetPath()));
    }
    return hasEpisodeCandidates;
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

  bool CVideoInfoScanner::AddSet(const CSetInfoTag& set)
  {
    // ensure our database is open (this can get called via other classes)
    if (!m_database.Open())
      return false;

    CLog::LogF(LOGDEBUG, "Adding new set {}", set.GetTitle());

    // Create set
    const int idSet{m_database.AddSet(set.GetTitle(), set.GetOverview(), set.GetOriginalTitle(),
                                      set.GetSortTitle())};

    // Assume art in set
    const bool added{idSet > 0 &&
                     m_database.SetArtForItem(idSet, MEDIA::TYPE::VIDEO_COLLECTION, set.GetArt())};

    m_database.Close();
    return added;
  }

  long CVideoInfoScanner::AddVideo(CFileItem* pItem,
                                   const ScraperPtr& scraper,
                                   bool videoFolder,
                                   bool useLocal,
                                   const CVideoInfoTag* showInfo,
                                   bool libraryImport,
                                   ContentType contentOverride)
  {
    const PreparedVideo video{PrepareVideo(pItem, scraper, videoFolder, useLocal, showInfo,
                                           libraryImport, contentOverride, m_regexpCache,
                                           m_lastProbed)};
    return WriteVideo(pItem, video, videoFolder, showInfo, libraryImport);
  }

  CVideoInfoScanner::PreparedVideo CVideoInfoScanner::PrepareVideo(
      CFileItem* pItem,
      const ScraperPtr& scraper,
      bool videoFolder,
      bool useLocal,
      const CVideoInfoTag* showInfo,
      bool libraryImport,
      ContentType contentOverride,
      KODI::REGEXP::RegExpCache& regexps,
      std::optional<std::pair<std::string, CStreamDetails>>& lastProbed) const
  {
    PreparedVideo video;
    video.content =
        !scraper || contentOverride != ContentType::NONE ? contentOverride : scraper->Content();
    const ContentType content{video.content};
    const bool usingLocalScraper{!scraper || scraper->IsLocalInformationOnly()};
    video.useRemoteArt = usingLocalScraper && m_advancedSettings->m_bNoRemoteArtWithLocalScraper
                             ? CVideoInfoScannerArt::UseRemoteArtWithLocalScraper::NO
                             : CVideoInfoScannerArt::UseRemoteArtWithLocalScraper::YES;
    const CVideoInfoScannerArt::UseRemoteArtWithLocalScraper useRemoteArt{video.useRemoteArt};

    std::string path{pItem->GetDynPath()};
    const int playlist{pItem->GetProperty(ITEM::PROPERTY::BLURAY_PLAYLIST).asInteger32(-1)};

    // The versions loop in RetrieveInfoForMovie() reuses one item for every <movie> in the nfo, so
    // the dynpath may still hold the previous version's playlist.
    if (playlist > -1 &&
        (::UTILS::DISCS::IsBlurayDiscImage(path) || URIUtils::IsBDFile(path) ||
         (URIUtils::IsBlurayPath(path) && URIUtils::GetBlurayPlaylistFromPath(path) != playlist)))
    {
      path = URIUtils::GetBlurayPlaylistPath(path, playlist);
      pItem->SetDynPath(path);
    }

    // A bluray:// path means a playlist has been chosen, so get details here (if not present)
    // An episode is matched against the disc, as its playlist may hold other episodes too
    if (!libraryImport && URIUtils::IsBlurayPath(path) &&
        !pItem->GetVideoInfoTag()->HasStreamDetails())
    {
      const bool read{content == ContentType::TVSHOWS && pItem->GetVideoInfoTag()->m_iEpisode > -1
                          ? CDiscDirectoryHelper::ReadEpisodePlaylist(*pItem)
                          : CDiscDirectoryHelper::ReadResolvedPlaylist(*pItem)};
      if (read)
        path = pItem->GetDynPath();
    }

    if (!libraryImport)
      m_art.GetArtwork(pItem, content, videoFolder, useLocal && !pItem->IsPlugin(),
                       showInfo ? URIUtils::AddFileToFolder(showInfo->m_strPath, ".actors") : "",
                       useRemoteArt);

    // ensure the art map isn't completely empty by specifying an empty thumb
    video.art = pItem->GetArt();
    if (video.art.empty())
      video.art[ART::TYPE::THUMB] = "";

    CVideoInfoTag &movieDetails = *pItem->GetVideoInfoTag();
    if (movieDetails.m_basePath.empty())
      movieDetails.m_basePath = pItem->GetBaseMoviePath(videoFolder);

    movieDetails.m_strFileNameAndPath = path;

    if (pItem->IsFolder())
      movieDetails.m_strPath = path;

    if (CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
            CSettings::SETTING_MYVIDEOS_EXTRACTFLAGS))
    {
      // At this stage potential sources of stream details are:
      // NFO - Local NFO (CVideoTagLoaderNFO)
      // NFO - NFO embedded in MKV (CVideoTagLoaderFFmpeg::LoadMKV() → CNfoFile::GetDetails())
      // NFO - Library import (ImportFromXML)
      // MEDIA - Bluray (ResolveBlurayPlaylist())
      // EXTERNAL - Plugin content (CVideoTagLoaderPlugin)

      // The folder the subtitles are looked for in is read from the cache where the scan has
      // listed it itself, but not that of a video it is given, as a refresh is
      std::string subtitleFolder;
      std::string videoName;
      CUtil::GetVideoBasePathAndFileName(pItem->GetSubtitleAnchorPath(), subtitleFolder, videoName);
      const int listingFlags{ListedFolder(subtitleFolder) ? DIR_FLAG_READ_CACHE
                                                          : DIR_FLAG_DEFAULTS};
      if (!movieDetails.HasStreamDetails())
      {
        // The episodes of a file are added one after the other, and the file is probed for the
        // first of them only: each still has its own external subtitles.
        if (lastProbed && lastProbed->first == path)
        {
          CDVDFileInfo::AddExternalSubtitles(*pItem, lastProbed->second, listingFlags,
                                             &m_subtitleFolders);
          CLog::LogF(LOGDEBUG, "Filestream details of video file {} already extracted",
                     CURL::GetRedacted(path));
        }
        else if (CStreamDetails probed;
                 CScanProbe::GetFileStreamDetails(pItem, listingFlags, &probed, &m_subtitleFolders))
        {
          lastProbed.emplace(path, std::move(probed));
          CLog::LogF(LOGDEBUG, "Extracted filestream details from video file {}",
                     CURL::GetRedacted(path));
        }
        else
          CLog::LogF(LOGDEBUG, "No filestream details extracted from video file {}",
                     CURL::GetRedacted(path));
      }
      else if (movieDetails.HasNFOStreamDetails())
        CLog::LogF(LOGDEBUG, "Filestream details from NFO file for {}", CURL::GetRedacted(path));
      else if (URIUtils::IsBlurayPath(path) || URIUtils::IsBDFile(path))
      {
        // A bluray's streams are read from its playlist, so it isn't probed, but the player loads
        // its external subtitles too. A dvd's aren't looked for, as the player loads none.
        CDVDFileInfo::AddExternalSubtitles(*pItem, movieDetails.m_streamDetails, listingFlags,
                                           &m_subtitleFolders);
        CLog::LogF(LOGDEBUG, "Filestream details from the playlist of bluray {}",
                   CURL::GetRedacted(path));
      }
      else if (const std::string firstPart{URIUtils::IsStack(path)
                                               ? CStackDirectory::GetFirstStackedFile(path)
                                               : std::string{}};
               URIUtils::IsBlurayPath(firstPart) || URIUtils::IsBDFile(firstPart))
      {
        // A stack of blurays has the streams of its first disc's playlist, and the subtitles the
        // player loads as it plays that disc
        CFileItem firstDisc{*pItem};
        firstDisc.SetDynPath(firstPart);
        CUtil::GetVideoBasePathAndFileName(firstDisc.GetSubtitleAnchorPath(), subtitleFolder,
                                           videoName);
        CDVDFileInfo::AddExternalSubtitles(firstDisc, movieDetails.m_streamDetails,
                                           ListedFolder(subtitleFolder) ? DIR_FLAG_READ_CACHE
                                                                        : DIR_FLAG_DEFAULTS,
                                           &m_subtitleFolders);
        movieDetails.m_streamDetails = firstDisc.GetVideoInfoTag()->m_streamDetails;
        CLog::LogF(LOGDEBUG, "Filestream details from the playlists of bluray stack {}",
                   CURL::GetRedacted(path));
      }
      else
        CLog::LogF(LOGDEBUG, "Filestream details already present for {}", CURL::GetRedacted(path));
    }

    // A measurement that found nothing is stored too, once the file is added, so the sweep
    // does not measure the file again.
    if (GEOMETRY::ContentGeometryNonLiveFromSettings() && !movieDetails.HasContentGeometry())
    {
      if (const std::optional<GEOMETRY::FileIdentity> identity{
              GEOMETRY::MeasurableIdentity(*pItem)})
      {
        // It reads frames from across the file, so takes its turn with the scan's probes
        std::optional<GEOMETRY::ContentGeometryRecord> geometry;
        CScanProbe::Run(pItem->GetDynPath(),
                        [&]
                        {
                          geometry = GEOMETRY::MeasureContentGeometry(
                              *pItem, *identity, GEOMETRY::SamplingDepth::Normal,
                              [this]() { return m_bStop.load(); });
                          return geometry.has_value();
                        });

        if (geometry && geometry->HasReading())
          movieDetails.m_contentGeometry.Set(*geometry);
        else if (geometry)
          video.unreadGeometry = *geometry;
      }
    }

    if (content == ContentType::MOVIES)
    {
      // find local trailer first
      std::string strTrailer = UTILS::FindTrailer(*pItem, videoFolder, &regexps);
      if (!strTrailer.empty())
        movieDetails.m_strTrailer = strTrailer;
    }

    if (content == ContentType::TVSHOWS && pItem->IsFolder() && !libraryImport)
    {
      CVideoInfoScannerArt::GetSeasonThumbs(movieDetails, video.seasonArt,
                                            CVideoThumbLoader::GetArtTypes(MEDIA::TYPE::SEASON),
                                            useLocal && !pItem->IsPlugin(), useRemoteArt, &regexps);
      for (const auto& seasonArtwork : video.seasonArt | std::views::values)
        m_art.Cache(seasonArtwork);
    }

    // A file's date added may be read from the file itself, and a show's from its folders, where
    // nothing gives one
    if (!movieDetails.m_dateAdded.IsValid())
    {
      if (!pItem->IsFolder())
        video.datesAdded.emplace_back(path, CVideoDatabase::ReadDateAdded(path));
      else if (content == ContentType::TVSHOWS)
      {
        std::vector<std::string> showPaths;
        if (!URIUtils::IsMultiPath(path) || !CMultiPathDirectory::GetPaths(path, showPaths))
          showPaths.push_back(path);
        for (const std::string& showPath : showPaths)
          video.datesAdded.emplace_back(showPath, CVideoDatabase::ReadDateAdded(showPath));
      }
    }

    video.path = std::move(path);
    return video;
  }

  long CVideoInfoScanner::WriteVideo(CFileItem* pItem,
                                     const PreparedVideo& video,
                                     bool videoFolder,
                                     const CVideoInfoTag* showInfo,
                                     bool libraryImport,
                                     CWriteBatch* batch /* = nullptr */)
  {
    // ensure our database is open (this can get called via other classes)
    if (!m_database.Open())
      return -1;

    // A batch's after a savepoint of its own, which a video that can't be written is undone to
    if (batch && !batch->BeginVideo())
    {
      m_database.Close();
      return -1;
    }

    const ContentType content{video.content};
    const CVideoInfoScannerArt::UseRemoteArtWithLocalScraper useRemoteArt{video.useRemoteArt};
    const std::string& path{video.path};
    KODI::ART::Artwork art{video.art};
    const KODI::ART::SeasonsArtwork& seasonArt{video.seasonArt};
    CVideoInfoTag& movieDetails = *pItem->GetVideoInfoTag();
    for (const auto& [datedPath, dateAdded] : video.datesAdded)
      m_database.PrepareDateAdded(datedPath, dateAdded);

    std::string strTitle(movieDetails.m_strTitle);
    if (showInfo && content == ContentType::TVSHOWS)
    {
      strTitle = StringUtils::Format("{} - {}x{} - {}", showInfo->m_strTitle,
                                     movieDetails.m_iSeason, movieDetails.m_iEpisode, strTitle);
    }

    CLog::Log(LOGDEBUG, "VideoInfoScanner: Adding new item to {}:{}", content,
              CURL::GetRedacted(path));
    long lResult = -1;

    // Written in one transaction, with nothing left to wait on but the database. A caller writing
    // more with it has its own open.
    const bool ownTransaction{!batch && !m_database.InTransaction()};
    if (ownTransaction)
      m_database.BeginTransaction();

    movieDetails.m_parentPathID =
        m_database.AddPath(URIUtils::GetParentPath(movieDetails.m_basePath));

    if (content == ContentType::MOVIES)
    {
      // Remove remote set art (if need)
      if (useRemoteArt == CVideoInfoScannerArt::UseRemoteArtWithLocalScraper::NO)
        std::erase_if(art,
                      [](const auto& artItem)
                      {
                        const auto& [type, url] = artItem;
                        return StringUtils::StartsWith(type, "set.") && URIUtils::IsRemote(url);
                      });

      // Art in SetInfoTag trumps set.* art in VideoInfoTag
      std::erase_if(art,
                    [&pItem](const auto& artItem)
                    {
                      const auto& [type, url] = artItem;
                      if (StringUtils::StartsWith(type, "set."))
                        // Remove set art from VideoInfoTag if same type art in SetInfoTag
                        return pItem->GetVideoInfoTag()->m_set.GetArt().contains(type.substr(4));
                      return false;
                    });

      // Deal with 'Disc n' subdirectories
      // Unless dealing with a full nfo or a library import, in which case the title and set are
      // taken from there already. Only occurs on first addition of the movie.
      const bool newMovie{m_database.GetMovieId(path) < 0};
      if (newMovie && !libraryImport && !pItem->GetProperty("from_nfo").asBoolean(false) &&
          !pItem->IsStack())
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
              m_database.SetArtForItem(idSet, MEDIA::TYPE::VIDEO_COLLECTION, art);
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
      movieDetails.SetMediaType(MEDIA::TYPE::MOVIE);

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

          // An nfo or an export records whether the asset is an extra of the movie
          const VideoAssetType assetType{tag->GetAssetInfo().GetType() == VideoAssetType::EXTRA
                                             ? VideoAssetType::EXTRA
                                             : VideoAssetType::VERSION};

          // Need to look up asset title in current table as, if importing, it may have a different id (primary key)
          const std::string assetTitle{tag->GetAssetInfo().GetTitle()};
          const int assetId{m_database.AddOrValidateVideoVersionType(assetTitle, assetType)};

          lResult = m_database.AddVideoAsset(VideoDbContentType::MOVIES, idMovie, assetId,
                                             assetType, *pItem)
                        ? tag->m_iFileId
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

        lResult = m_database.SetDetailsForTvShow(multipath, movieDetails, art, seasonArt);
        movieDetails.m_iDbId = lResult;
        movieDetails.SetMediaType(MEDIA::TYPE::TV_SHOW);
      }
      else
      {
        // we add episode then set details, as otherwise set details will delete the
        // episode then add, which breaks multi-episode files.
        int idShow = showInfo ? showInfo->m_iDbId : -1;
        int idEpisode = m_database.AddNewEpisode(idShow, movieDetails);
        lResult = m_database.SetDetailsForEpisode(movieDetails, art, idShow, idEpisode);
        movieDetails.m_iDbId = lResult;
        movieDetails.SetMediaType(MEDIA::TYPE::EPISODE);
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
      movieDetails.SetMediaType(MEDIA::TYPE::MUSIC_VIDEO);
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

    if (video.unreadGeometry && lResult > -1)
      m_database.SetContentGeometry(m_database.GetFileId(*pItem), *video.unreadGeometry);

    ScanSummary::Type type{ScanSummary::Type::MOVIE};
    if (content == ContentType::MOVIE_VERSIONS)
      type = ScanSummary::Type::VERSION;
    else if (content == ContentType::TVSHOWS)
      type = showInfo ? ScanSummary::Type::EPISODE : ScanSummary::Type::TV_SHOW;
    else if (content == ContentType::MUSICVIDEOS)
      type = ScanSummary::Type::MUSIC_VIDEO;

    // An episode added without its details leaves no episode behind, and a video whose transaction
    // couldn't be committed isn't added. A batch's is counted and announced once committed.
    if (ownTransaction)
    {
      if (lResult < 0)
        m_database.RollbackTransaction();
      else if (!m_database.CommitTransaction())
      {
        if (m_database.InTransaction())
          m_database.RollbackTransaction();
        lResult = -1;
        movieDetails.m_iDbId = -1;
      }
    }
    else if (batch && !batch->EndVideo(lResult > -1, {type, path, strTitle}, *pItem))
    {
      lResult = -1;
      movieDetails.m_iDbId = -1;
    }
    m_database.ForgetPreparedDatesAdded();

    m_database.Close();

    if (lResult > -1 && !batch)
      Added({type, path, strTitle}, *pItem);
    return lResult;
  }

  void CVideoInfoScanner::Added(ScanSummary::Item item, const CFileItem& video)
  {
    m_summary.added.push_back(std::move(item));

    // Announce() copies the item before it returns, so it is lent here rather than copied twice
    CServiceBroker::GetAnnouncementManager()->Announce(
        ANNOUNCEMENT::VideoLibraryEvent{ANNOUNCEMENT::EVENT::LIBRARY::Update{
            .item = std::shared_ptr<const CFileItem>(&video, [](const CFileItem*) {}),
            .transaction = m_bRunning,
            .added = true}});
  }

  CVideoInfoScanner::CWriteBatch::CWriteBatch(CVideoInfoScanner& scanner) : m_scanner(scanner)
  {
    // Held open, as a connection closed rolls back its transaction
    m_scanner.m_database.Open();
  }

  CVideoInfoScanner::CWriteBatch::~CWriteBatch()
  {
    Commit();
    if (m_transactions > 0)
      CLog::Log(LOGDEBUG, "VideoInfoScanner: Wrote {} videos in {} transactions", m_committed,
                m_transactions);
    m_scanner.m_database.Close();
  }

  bool CVideoInfoScanner::CWriteBatch::BeginVideo()
  {
    if (m_failed)
      return false;

    CVideoDatabase& database{m_scanner.m_database};
    if (!m_open)
    {
      if (!database.BeginTransaction())
      {
        if (database.InTransaction())
          database.RollbackTransaction();
        m_failed = true;
        return false;
      }
      m_open = true;
      m_begun = std::chrono::steady_clock::now();
    }

    if (!database.BeginSavepoint(SAVEPOINT))
    {
      Abandon();
      return false;
    }
    return true;
  }

  bool CVideoInfoScanner::CWriteBatch::EndVideo(bool written,
                                                ScanSummary::Item item,
                                                const CFileItem& video)
  {
    CVideoDatabase& database{m_scanner.m_database};
    if (!written)
    {
      // Those written before it are kept
      if (!database.RollbackToSavepoint(SAVEPOINT))
        Abandon();
      return false;
    }
    if (!database.ReleaseSavepoint(SAVEPOINT))
    {
      Abandon();
      return false;
    }

    m_written.emplace_back(std::move(item), std::make_shared<const CFileItem>(video));
    if (m_written.size() >= m_scanner.m_batchLimit.videos ||
        std::chrono::steady_clock::now() - m_begun >= m_scanner.m_batchLimit.time)
      return Commit();
    return true;
  }

  bool CVideoInfoScanner::CWriteBatch::Commit()
  {
    if (!m_open)
      return !m_failed;

    m_open = false;
    ++m_transactions;
    ++m_scanner.m_batchTransactions;
    std::vector<std::pair<ScanSummary::Item, std::shared_ptr<const CFileItem>>> written;
    written.swap(m_written);
    CVideoDatabase& database{m_scanner.m_database};
    if (!database.CommitTransaction())
    {
      if (database.InTransaction())
        database.RollbackTransaction();
      CLog::Log(LOGERROR, "VideoInfoScanner: The {} videos written last couldn't be committed",
                written.size());
      m_failed = true;
      return false;
    }

    m_committed += written.size();
    for (auto& [item, video] : written)
      m_scanner.Added(std::move(item), *video);

    // A writer that waited for the transaction to end gets in before the next is begun. It tries
    // again every 100 ms.
    if (database.OthersWaitedSince(m_begun))
      std::this_thread::sleep_for(LETTING_OTHERS_IN);
    return true;
  }

  void CVideoInfoScanner::CWriteBatch::Abandon()
  {
    CVideoDatabase& database{m_scanner.m_database};
    if (database.InTransaction())
      database.RollbackTransaction();
    CLog::Log(LOGERROR,
              "VideoInfoScanner: The {} videos written last were lost with their "
              "transaction",
              m_written.size());
    m_written.clear();
    m_open = false;
    m_failed = true;
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
                  bool resetTag) const
  {
    // A folder the scan listed itself isn't listed again to look for an nfo in it
    if (std::unique_ptr<IVideoInfoTagLoader> loader{CVideoInfoTagLoaderFactory::CreateLoader(
            item, scraper, lookInFolder, false,
            [this](const std::string& folder) { return ListedFolder(folder); })};
        loader)
    {
      CVideoInfoTag& infoTag = *item.GetVideoInfoTag();
      if (resetTag)
        infoTag.Reset();
      auto result = loader->Load(infoTag, false);

      // A <playlist> nfo element identifies the disc playlist the info belongs to
      if (const int playlist{loader->GetBlurayPlaylist()}; playlist > -1)
        item.SetProperty(ITEM::PROPERTY::BLURAY_PLAYLIST, playlist);

      // keep some properties only if advancedsettings.xml says so
      if (!m_advancedSettings->m_bVideoLibraryImportWatchedState)
        infoTag.ResetPlayCount();
      if (!m_advancedSettings->m_bVideoLibraryImportResumePoint)
        infoTag.SetResumePoint(CBookmark());

      return {result, std::move(loader)};
    }
    return {InfoType::NONE, nullptr};
  }

  std::string CVideoInfoScanner::SourceName(const std::string& folder)
  {
    std::vector<CMediaSource>& sources{
        CMediaSourceSettings::GetInstance().GetSources(MEDIA::MediaSection::VIDEO)};
    bool isSourceName{false};
    if (const int source{CUtil::GetMatchingSource(folder, sources, isSourceName)}; source >= 0)
      return sources[source].strName;

    std::string name{folder};
    URIUtils::RemoveSlashAtEnd(name);
    return URIUtils::GetFileName(name);
  }

  std::string CVideoInfoScanner::GetMovieSetInfoFolder(const std::string& setTitle)
  {
    if (setTitle.empty())
      return "";
    std::string path = CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(
        CSettings::SETTING_VIDEOLIBRARY_MOVIESETSFOLDER);
    if (path.empty())
      return "";
    path = URIUtils::AddFileToFolderMatchingEncoding(
        path, CUtil::MakeLegalFileName(setTitle, LegalPath::WIN32_COMPAT));
    URIUtils::AddSlashAtEnd(path);
    CLog::Log(LOGDEBUG,
        "VideoInfoScanner: Looking for local artwork for movie set '{}' in folder '{}'",
        setTitle,
        CURL::GetRedacted(path));
    return CDirectory::Exists(path) ? path : "";
  }

  CInfoScanner::InfoRet CVideoInfoScanner::OnProcessSeriesFolder(
      EPISODELIST& files,
      const ADDON::ScraperPtr& scraper,
      bool useLocal,
      const CVideoInfoTag& showInfo,
      CGUIDialogProgress* pDlgProgress /* = NULL */,
      const EPISODELIST* fetchedGuide /* = nullptr */)
  {
    if (pDlgProgress)
    {
      pDlgProgress->SetLine(1, CVariant{20361}); // Loading episode details
      pDlgProgress->SetPercentage(0);
      pDlgProgress->ShowProgressBar(true);
      pDlgProgress->Progress();
    }

    EpisodeFileMap knownEpisodes;
    if (showInfo.m_iDbId > 0)
      m_database.GetEpisodeMap(showInfo.m_iDbId, knownEpisodes);
    const auto isKnownEpisode{
        [&knownEpisodes](const EPISODE& file)
        {
          const auto [first, last]{knownEpisodes.equal_range(file.strPath)};
          return std::any_of(first, last,
                             [&file](const EpisodeFileMapEntry& known)
                             {
                               return (file.iEpisode == -1 ||
                                       known.second.episode == file.iEpisode) &&
                                      (file.iSeason == -1 || known.second.season == file.iSeason);
                             });
        }};

    std::unordered_map<std::string, int> episodeMap;
    for (const auto& file : files)
      episodeMap[file.strPath]++;

    // The episodes to look up, those of a file together, as the file's streams are read once for
    // all of them
    std::vector<std::vector<EPISODELIST::const_iterator>> lookups;
    for (EPISODELIST::const_iterator file = files.begin(); file != files.end(); ++file)
    {
      if ((pDlgProgress && pDlgProgress->IsCanceled()) || m_bStop)
        return InfoRet::CANCELLED;

      if (isKnownEpisode(*file) ||
          m_database.GetEpisodeId(file->strPath, file->iEpisode, file->iSeason) > -1)
      {
        if (m_handle)
          m_handle->SetText(
              CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(20415));
        m_summary.existing.push_back({ScanSummary::Type::EPISODE, file->strPath, {}});
        continue;
      }

      if (!lookups.empty() && lookups.back().front()->strPath == file->strPath)
        lookups.back().emplace_back(file);
      else
        lookups.push_back({file});
    }

    if (!pDlgProgress)
    {
      std::vector<const EPISODE*> toAdd;
      for (const auto& lookup : lookups)
        std::ranges::transform(lookup, std::back_inserter(toAdd),
                               [](const EPISODELIST::const_iterator& file) { return &*file; });
      ReadEpisodeDiscsAhead(EpisodeDiscs(toAdd));
    }

    // All seasons (empty) if an episode is matched by date or title
    std::set<int> seasons;
    for (const auto& lookup : lookups)
      for (const auto& file : lookup)
        seasons.insert(file->iSeason);
    if (!seasons.empty() && *seasons.begin() < 0)
      seasons.clear();

    // The show's episode guide, fetched by the first episode that needs it, unless it was with the
    // show
    struct
    {
      std::mutex lock;
      bool fetched{false};
      bool failed{false};
      EPISODELIST episodes;
    } guide;
    if (fetchedGuide)
    {
      guide.fetched = true;
      guide.episodes = *fetchedGuide;
    }

    struct FoundEpisode
    {
      //! The listed episode it was found for, which, as a reference, can't be left out
      const EPISODE& source;
      CFileItem item;
      bool isFolder{false};
      bool useLocal{true};
      //! Whether its playlist is chosen when it is added, after those of the episodes before it
      bool resolvePlaylist{false};
      std::optional<PreparedVideo> prepared{};
    };
    struct Lookup
    {
      std::vector<FoundEpisode> found;
      unsigned int failedLookups{0};
      //! The files of the episodes not added, and whether as their lookup failed
      std::vector<std::pair<std::string, bool>> notAdded;
    };
    std::vector<Lookup> results(lookups.size());

    const auto lookUpFile{
        [&](size_t index)
        {
          Lookup& lookup{results[index]};
          KODI::REGEXP::RegExpCache regexps;
          std::optional<std::pair<std::string, CStreamDetails>> lastProbed;
          for (const EPISODELIST::const_iterator& file : lookups[index])
          {
            if (m_bStop)
              return;

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
            std::unique_ptr<IVideoInfoTagLoader> loader;
            if (useLocal)
              std::tie(result, loader) = ReadInfoTag(item, scraper, false, false);
            if (result == InfoType::FULL)
            {
              item.SetProperty("from_nfo", true);

              // override with episode and season number from file if available
              if (file->iEpisode > -1)
              {
                item.GetVideoInfoTag()->m_iEpisode = file->iEpisode;
                item.GetVideoInfoTag()->m_iSeason = file->iSeason;

                // Add flag if multi-episode file
                // <archive>-SxxEyy.nfo is only exported for an archive holding several episodes
                const auto* nfoLoader{dynamic_cast<const CVideoTagLoaderNFO*>(loader.get())};
                if (episodeMap.at(file->strPath) > 1 ||
                    (URIUtils::IsInArchive(file->strPath) && nfoLoader &&
                     URIUtils::PathEquals(
                         nfoLoader->GetNFOPath(),
                         URIUtils::ReplaceExtension(
                             ART::GetTBNFile(item, file->iSeason, file->iEpisode), ".nfo"))))
                  item.SetProperty(MULTIPLE_EPISODES, true);
              }
              lookup.found.push_back(FoundEpisode{.source = *file,
                                                  .item = std::move(item),
                                                  .isFolder = file->isFolder,
                                                  .useLocal = true});
              continue;
            }

            std::unique_lock guideLock(guide.lock);
            if (!guide.fetched)
            {
              // fetch episode guide
              if (!showInfo.m_strEpisodeGuide.empty() && !scraper->IsLocalInformationOnly())
              {
                CScraperUrl url;
                url.ParseAndAppendUrlsFromEpisodeGuide(showInfo.m_strEpisodeGuide);

                if (pDlgProgress)
                {
                  pDlgProgress->SetLine(1, CVariant{20354}); // Fetching episode guide
                  pDlgProgress->Progress();
                }

                const CScraperCalls::CRegistration call{m_scraperCalls};
                guide.episodes = scraper->GetEpisodeList(url, seasons);
                // A scraper may list none of the seasons named, which isn't a failure
                if (guide.episodes.empty() && !seasons.empty())
                  guide.episodes = scraper->GetEpisodeList(url);
                if (guide.episodes.empty())
                {
                  CLog::Log(LOGWARNING,
                            "VideoInfoScanner: Unable to fetch the episode guide of '{}'",
                            showInfo.m_strTitle);
                  ++lookup.failedLookups;
                  guide.failed = true;
                }

                guide.fetched = true;
              }
            }
            const bool hasEpisodeGuide{guide.fetched};
            const bool guideFailed{guide.failed};
            guideLock.unlock();
            const EPISODELIST& episodes{guide.episodes};

            if (episodes.empty())
            {
              lookup.notAdded.emplace_back(file->strPath, guideFailed);

              // An episode guide that couldn't be fetched has been reported already
              if (!hasEpisodeGuide)
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
            EPISODELIST::const_iterator guideEntry = episodes.begin();
            EPISODELIST matches;

            for (; guideEntry != episodes.end(); ++guideEntry)
            {
              if ((file->iEpisode != -1) && (file->iSeason != -1))
              {
                if (key == *guideEntry)
                {
                  bFound = true;
                  break;
                }
                else if ((file->iSubepisode != 0) && (backupkey == *guideEntry))
                {
                  matches.push_back(*guideEntry);
                  continue;
                }
              }
              if (file->cDate.IsValid() && guideEntry->cDate.IsValid() &&
                  file->cDate == guideEntry->cDate)
              {
                matches.push_back(*guideEntry);
                continue;
              }
              if (!guideEntry->strTitle.empty() &&
                  StringUtils::EqualsNoCase(guideEntry->strTitle, file->strTitle))
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
                guideEntry = matches.begin();
                bFound = true;
              }
              else if (!file->strTitle.empty())
              {
                CLog::Log(LOGDEBUG, "VideoInfoScanner: analyzing parsed title '{}'",
                          file->strTitle);
                double minscore =
                    0; // Default minimum score is 0 to find whatever is the best match.

                const EPISODELIST* candidates;
                if (matches
                        .empty()) // No matches found using earlier criteria. Use fuzzy match on titles across all episodes.
                {
                  minscore = 0.8; // 80% should ensure a good match.
                  candidates = &episodes;
                }
                else // Multiple matches found. Use fuzzy match on the title with already matched episodes to pick the best.
                  candidates = &matches;

                std::vector<std::string> titles;
                for (auto candidate = candidates->begin(); candidate != candidates->end();
                     ++candidate)
                {
                  std::string title{candidate->strTitle};
                  StringUtils::ToLower(title);
                  titles.push_back(title);
                }

                double matchscore;
                std::string loweredTitle(file->strTitle);
                StringUtils::ToLower(loweredTitle);
                int index = StringUtils::FindBestMatch(loweredTitle, titles, matchscore);
                if (index >= 0 && matchscore >= minscore)
                {
                  guideEntry = candidates->begin() + index;
                  bFound = true;
                  CLog::Log(LOGDEBUG,
                            "{} fuzzy title match for show: '{}', title: '{}', match: '{}', score: "
                            "{:f} >= {:f}",
                            __FUNCTION__, showInfo.m_strTitle, file->strTitle, titles[index],
                            matchscore, minscore);
                }
              }
            }

            if (!bFound)
            {
              CLog::Log(LOGDEBUG,
                        "{} - no match for show: '{}', season: {}, episode: {}.{}, airdate: '{}', "
                        "title: '{}'",
                        __FUNCTION__, showInfo.m_strTitle, file->iSeason, file->iEpisode,
                        file->iSubepisode, file->cDate.GetAsLocalizedDate(), file->strTitle);
              lookup.notAdded.emplace_back(file->strPath, false);
              continue;
            }

            CFileItem scraperItem;
            scraperItem.SetPath(file->strPath);
            bool fetched{false};
            if (guideEntry->item && guideEntry->item->HasVideoInfoTag())
            {
              *scraperItem.GetVideoInfoTag() = *guideEntry->item->GetVideoInfoTag();
              scraperItem.GetVideoInfoTag()->m_streamDetails.Reset();
              fetched = true;
            }
            else
            {
              const CScraperCalls::CRegistration call{m_scraperCalls};
              fetched = scraper->GetVideoDetails({}, guideEntry->cScraperUrl, false /*fMovie*/,
                                                 *scraperItem.GetVideoInfoTag());
            }
            if (!fetched)
            {
              CLog::Log(LOGWARNING, "VideoInfoScanner: Unable to fetch the details of episode {}",
                        CURL::GetRedacted(file->strPath));
              ++lookup.failedLookups;
              lookup.notAdded.emplace_back(file->strPath, true);
              continue;
            }

            if (result == InfoType::COMBINED || result == InfoType::OVERRIDE)
              scraperItem.GetVideoInfoTag()->Merge(*item.GetVideoInfoTag());

            // Only set season/epnum from filename when it is not already set by a scraper
            if (scraperItem.GetVideoInfoTag()->m_iSeason == -1)
              scraperItem.GetVideoInfoTag()->m_iSeason = guideEntry->iSeason;
            if (scraperItem.GetVideoInfoTag()->m_iEpisode == -1)
              scraperItem.GetVideoInfoTag()->m_iEpisode = guideEntry->iEpisode;

            // Resolved as an episode whether or not the scraper gave the media type
            scraperItem.GetVideoInfoTag()->SetMediaType(MEDIA::TYPE::EPISODE);
            lookup.found.push_back(FoundEpisode{.source = *file,
                                                .item = std::move(scraperItem),
                                                .isFolder = file->isFolder,
                                                .useLocal = useLocal,
                                                .resolvePlaylist = true});
          }

          // A disc's playlists are chosen as its episodes are added, each taking one those before it
          // haven't, so a disc is prepared then
          for (FoundEpisode& episode : lookup.found)
          {
            if (!IsDisc(episode.item.GetDynPath()))
              episode.prepared =
                  PrepareVideo(&episode.item, scraper, episode.isFolder, episode.useLocal,
                               &showInfo, false, ContentType::TVSHOWS, regexps, lastProbed);
          }
        }};

    const auto lookUp{[&](size_t index)
                      {
                        const CScraperCalls::CInterpreter interpreter{m_scraperCalls};
                        try
                        {
                          lookUpFile(index);
                        }
                        catch (...)
                        {
                          // The episodes not reached are looked up again by the next scan
                          CLog::LogF(LOGERROR, "Looking up the episodes of {} failed",
                                     CURL::GetRedacted(lookups[index].front()->strPath));
                          ++results[index].failedLookups;
                        }
                      }};

    bool cancelled{false};
    bool failed{false};
    // The episodes handed back one after another are written in one transaction, but where a dialog
    // is up, which the database isn't held for, or a caller writes them in a transaction of its own
    std::optional<CWriteBatch> batch;
    if (!pDlgProgress && !m_database.InTransaction())
      batch.emplace(*this);
    const auto commit{[&]
                      {
                        if (batch && !batch->Commit())
                          failed = true;
                        return !failed;
                      }};

    const auto handBack{
        [&](size_t index)
        {
          const EPISODE& first{*lookups[index].front()};
          if (pDlgProgress)
          {
            pDlgProgress->SetLine(1, CVariant{20361}); // Loading episode details
            pDlgProgress->SetLine(
                2, StringUtils::Format(
                       "{} {}",
                       CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(20373),
                       first.iSeason)); // Season x
            pDlgProgress->SetLine(
                3, StringUtils::Format(
                       "{} {}",
                       CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(20359),
                       first.iEpisode)); // Episode y
            pDlgProgress->SetPercentage(static_cast<int>(100.f * (index + 1) / lookups.size()));
            pDlgProgress->Progress();
          }
          ReportProgress(static_cast<float>(index + 1) / static_cast<float>(lookups.size()));

          if ((pDlgProgress && pDlgProgress->IsCanceled()) || m_bStop)
          {
            cancelled = true;
            return false;
          }

          // Taken, so that its details, once written, aren't held for the rest of the show
          Lookup lookup{std::move(results[index])};
          m_failedLookups += lookup.failedLookups;
          for (const auto& [path, lookupFailed] : lookup.notAdded)
            NotAdded(ScanSummary::Type::EPISODE, path, lookupFailed);
          // A disc is waited for here, with the database not held for it
          if (IsDisc(first.strPath) && !commit())
            return false;
          // Its disc, where read ahead, is held until its episodes are added
          const CDiscReadAhead::CTaken disc{m_discReadAhead, lookups[index].front()->strPath};
          for (FoundEpisode& episode : lookup.found)
          {
            // The library was looked at for each episode before any was looked up, so an episode
            // a file lists twice, as Show S01E01E01.mkv does, is found again as it is written
            const EPISODE& source{episode.source};
            if (episodeMap.at(source.strPath) > 1 &&
                m_database.GetEpisodeId(source.strPath, source.iEpisode, source.iSeason) > -1)
            {
              m_summary.existing.push_back({ScanSummary::Type::EPISODE, source.strPath, {}});
              continue;
            }
            if (!episode.prepared)
            {
              // An episode of a disc has its playlists chosen from those the library's episodes of
              // the disc don't have, read through a connection of its own, which sees only what is
              // committed: the episodes written before it are, and the database isn't held while
              // the disc is read. Choosing a stack's writes through a connection of its own.
              if (!commit())
                return false;

              // Determine bluray playlist(s) (if possible)
              // Also populates streamdetails if playlist(s) found
              if (episode.resolvePlaylist)
              {
                CFileItemList blurayItems;
                ResolveBlurayPlaylist(&episode.item, blurayItems);
              }
              episode.prepared =
                  PrepareVideo(&episode.item, scraper, episode.isFolder, episode.useLocal,
                               &showInfo, false, ContentType::TVSHOWS, m_regexpCache, m_lastProbed);
            }
            if (WriteVideo(&episode.item, *episode.prepared, episode.isFolder, &showInfo, false,
                           batch ? &*batch : nullptr) < 0)
            {
              failed = true;
              return false;
            }
          }
          return true;
        }};

    // The database isn't held while the scan waits for an episode
    const auto beforeWaiting{[&] { return !m_commitBeforeWaiting || commit(); }};

    CVideoLookups::Run(lookups.size(), pDlgProgress ? 1 : LookUpThreads(), lookUp, handBack,
                       m_bStop, 0, beforeWaiting);

    // The episodes written are kept, those written before a stop too
    commit();

    if (failed)
      return InfoRet::INFO_ERROR;
    if (cancelled || m_bStop)
      return InfoRet::CANCELLED;
    return InfoRet::ADDED;
  }

  bool CVideoInfoScanner::GetDetails(CFileItem* pItem,
                                     const ADDON::CScraper::UniqueIDs& uniqueIDs,
                                     CScraperUrl& url,
                                     const ScraperPtr& scraper,
                                     IVideoInfoTagLoader* loader,
                                     CGUIDialogProgress* pDialog /* = NULL */)
  {
    if (m_handle && !url.GetTitle().empty())
      m_handle->SetText(url.GetTitle());

    if (!LookUpDetails(*pItem, uniqueIDs, url, scraper, loader, pDialog))
      return false; // no info found, or cancelled

    const CVideoInfoTag& movieDetails{*pItem->GetVideoInfoTag()};
    if (m_handle && url.GetTitle().empty())
      m_handle->SetText(movieDetails.m_strTitle);

    if (pDialog)
    {
      if (!pDialog->HasText())
        pDialog->SetLine(0, CVariant{movieDetails.m_strTitle});
      pDialog->Progress();
    }
    return true;
  }

  bool CVideoInfoScanner::LookUpDetails(CFileItem& item,
                                        const ADDON::CScraper::UniqueIDs& uniqueIDs,
                                        CScraperUrl& url,
                                        const ScraperPtr& scraper,
                                        IVideoInfoTagLoader* loader,
                                        CGUIDialogProgress* pDialog /* = nullptr */) const
  {
    CVideoInfoTag movieDetails;
    {
      const CScraperCalls::CRegistration call{m_scraperCalls};
      if (!scraper->GetVideoDetails(uniqueIDs, url, true /*fMovie*/, movieDetails))
        return false;
    }

    if (loader)
      loader->Load(movieDetails, true);

    *item.GetVideoInfoTag() = movieDetails;

    // A <playlist> nfo element identifies the disc playlist the info belongs to
    if (const int playlist{loader ? loader->GetBlurayPlaylist() : -1}; playlist > -1)
      item.SetProperty(ITEM::PROPERTY::BLURAY_PLAYLIST, playlist);

    return true;
  }

  void CVideoInfoScanner::AddPathToClean(const std::string& directory)
  {
    m_pathsToClean.insert(m_database.GetPathId(directory));

    // Pick up the base paths of directory's disc rips and archives, and its video extras folders
    std::vector<std::pair<int, std::string>> subPaths;
    m_database.GetSubPaths(directory, subPaths, false);
    for (const auto& [idPath, path] : subPaths)
    {
      if (!URIUtils::PathHasParent(path, directory) || URIUtils::IsDiscPath(path) ||
          !GetExtrasFolder(directory, path).empty())
        m_pathsToClean.insert(idPath);
    }
  }

  bool CVideoInfoScanner::DownloadFailed(CGUIDialogProgress* pDialog)
  {
    // A scan in the background carries on without asking: the failure is counted, has its entry in
    // the event log and its place in the notification at the end, and its folder is looked up again
    // by the next scan
    if (!pDialog ||
        CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_bVideoScannerIgnoreErrors)
      return true;

    HELPERS::ShowOKDialogText(CVariant{20448}, CVariant{20449});
    return false;
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
    std::vector<CScraperUrl> found;
    try
    {
      const CScraperCalls::CRegistration call{m_scraperCalls};
      found = scraper->FindMovie(title, year);
    }
    catch (const CScraperError&)
    {
      if (m_bStop || !DownloadFailed(progress))
      { // the user wants to cancel the scan
        m_bStop = true;
        return -1; // cancelled
      }
      ++m_failedLookups; // carried on past a search that failed, rather than one that found nothing
    }
    if (!found.empty())
    {
      url = found[0];
      return 1;  // found a movie
    }
    return 0;    // didn't find anything
  }

  CInfoScanner::InfoRet CVideoInfoScanner::AddBlurayPlaylistVersions(
      const CFileItemList& blurayItems, const ScraperPtr& scraper, bool bDirNames, bool useLocal)
  {
    if (blurayItems.IsEmpty())
    {
      CLog::LogF(LOGDEBUG, "No bluray playlist versions to add for the movie");
      return InfoRet::NOT_NEEDED;
    }

    bool added{false};
    bool versioned{false};
    bool known{false};
    int targetDbId{-1};
    for (const auto& item : blurayItems)
    {
      // The playlists are ordered with the disc's main title first, so only that one may become
      // the default version - otherwise each playlist added after it displaces it in turn, leaving
      // the shortest of them as the default
      const bool isMainPlaylist{item == blurayItems.Get(0)};

      // A playlist already in the library keeps its movie, which the disc's new playlists join.
      // Adding it again would return that movie, which a declined merge below would then delete.
      if (const int knownDbId{m_database.GetMovieId(item->GetDynPath())}; knownDbId >= 0)
      {
        known = true;
        if (isMainPlaylist)
          targetDbId = knownDbId;
        continue;
      }

      const int newMovieDbId{static_cast<int>(
          AddVideo(item.get(), scraper, bDirNames, useLocal, nullptr, false, ContentType::MOVIES))};
      if (newMovieDbId < 0)
      {
        CLog::LogF(LOGERROR, "Failed to add bluray playlist '{}' for movie",
                   CURL::GetRedacted(item->GetDynPath()));
        continue;
      }
      added = true;

      // Each playlist is added as a movie in its own right first, then merged as a version of the
      // one before it. Nothing to merge into if versioning is turned off, so they stay separate
      if (m_similarVideoAction != SimilarVideoScanAction::ASK &&
          m_similarVideoAction != SimilarVideoScanAction::AUTO)
        continue;

      const auto [result, chosenTargetMovieDbId] = ProcessVideoVersion(
          ContentToVideoDbType(ContentType::MOVIES), newMovieDbId, targetDbId, isMainPlaylist);
      if (result == VersionConversionResult::SUCCESS)
      {
        CLog::LogF(LOGDEBUG, "Added bluray playlist '{}' as a version of movie id {}",
                   CURL::GetRedacted(item->GetDynPath()), chosenTargetMovieDbId);
        m_summary.added.back().type = ScanSummary::Type::VERSION;
        targetDbId = chosenTargetMovieDbId;
        versioned = true;
      }
      else if (isMainPlaylist)
      {
        // Not merged into another movie, so the disc's main title stays one and the other
        // playlists become versions of it
        targetDbId = newMovieDbId;
      }
      else if (result == VersionConversionResult::FAILED ||
               result == VersionConversionResult::CANCELLED)
      {
        // Declined, or merging was not possible
        if (m_database.DeleteMovie(newMovieDbId))
        {
          m_database.DeleteFile(item->GetVideoInfoTag()->m_iFileId);
          m_summary.added.pop_back();
        }
        CLog::LogF(LOGDEBUG,
                   "Not adding bluray playlist '{}' as a version - declined or merge not possible",
                   CURL::GetRedacted(item->GetDynPath()));
      }
    }

    if (versioned)
      RemovePartNumberFromTitle(targetDbId, VideoDbContentType::MOVIES, m_database);
    if (!added)
      return known ? InfoRet::HAVE_ALREADY : InfoRet::INFO_ERROR;
    return versioned ? InfoRet::HAVE_ALREADY : InfoRet::ADDED;
  }

  std::pair<VersionConversionResult, int> CVideoInfoScanner::ProcessVideoVersion(
      VideoDbContentType itemType,
      int dbId,
      int targetDbId /* = -1 */,
      bool canBecomeDefault /* = true */)
  {
    if (!m_database.Open())
      return {VersionConversionResult::FAILED, -1};
    const auto result{CGUIDialogVideoManagerVersions::ProcessVideoVersion(
        itemType, dbId, m_database, targetDbId, canBecomeDefault)};
    m_database.Close();
    return result;
  }

  void CVideoInfoScanner::RemovePartNumberFromTitle(int dbId,
                                                    VideoDbContentType itemType,
                                                    CVideoDatabase& db)
  {
    return CGUIDialogVideoManagerVersions::RemovePartNumberFromTitle(dbId, itemType, db);
  }

  void CVideoInfoScanner::NotAdded(ScanSummary::Type type, const std::string& path, bool failed)
  {
    (failed ? m_summary.failed : m_summary.notFound).push_back({type, path, {}});

    auto* eventLog{CServiceBroker::GetEventLog()};
    if (!eventLog)
      return;

    MEDIA::TYPE mediaType{MEDIA::TYPE::MOVIE};
    if (type == ScanSummary::Type::TV_SHOW)
      mediaType = MEDIA::TYPE::TV_SHOW;
    else if (type == ScanSummary::Type::EPISODE)
      mediaType = MEDIA::TYPE::EPISODE;
    else if (type == ScanSummary::Type::MUSIC_VIDEO)
      mediaType = MEDIA::TYPE::MUSIC_VIDEO;

    const std::string name{type == ScanSummary::Type::TV_SHOW ? CURL::GetRedacted(path)
                                                              : URIUtils::GetFileName(path)};
    const std::string& text{
        CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(failed ? 24191 : 24147)};
    eventLog->Add(std::make_shared<CMediaLibraryEvent>(
        mediaType, path, 24145, StringUtils::Format(text, MEDIA::NameOf(mediaType), name),
        EventLevel::Warning));
  }

  void CVideoInfoScanner::ReportSummary() const
  {
    using enum ScanSummary::Type;
    const auto count{[](const std::vector<ScanSummary::Item>& items, ScanSummary::Type type)
                     { return std::ranges::count(items, type, &ScanSummary::Item::type); }};
    const auto name{[](ScanSummary::Type type) -> std::string_view
                    {
                      switch (type)
                      {
                        case VERSION:
                          return "version";
                        case TV_SHOW:
                          return "tv show";
                        case EPISODE:
                          return "episode";
                        case MUSIC_VIDEO:
                          return "music video";
                        case MOVIE:
                          break;
                      }
                      return "movie";
                    }};

    const auto& [added, existing, unchangedDirectories, notFound, failed] = m_summary;
    CLog::Log(LOGINFO, "VideoInfoScanner: Scan summary");
    CLog::Log(LOGINFO,
              "VideoInfoScanner:   added - movies: {}, versions: {}, tv shows: {}, episodes: {}, "
              "music videos: {}",
              count(added, MOVIE), count(added, VERSION), count(added, TV_SHOW),
              count(added, EPISODE), count(added, MUSIC_VIDEO));
    CLog::Log(LOGINFO,
              "VideoInfoScanner:   rescanned but already in the library - movies: {}, versions: "
              "{}, tv shows: {}, episodes: {}, music videos: {}",
              count(existing, MOVIE), count(existing, VERSION), count(existing, TV_SHOW),
              count(existing, EPISODE), count(existing, MUSIC_VIDEO));
    CLog::Log(LOGINFO, "VideoInfoScanner:   unchanged directories skipped: {}",
              unchangedDirectories);
    CLog::Log(LOGINFO,
              "VideoInfoScanner:   not found - movies: {}, tv shows: {}, episodes: {}, music "
              "videos: {}",
              count(notFound, MOVIE), count(notFound, TV_SHOW), count(notFound, EPISODE),
              count(notFound, MUSIC_VIDEO));
    CLog::Log(LOGINFO,
              "VideoInfoScanner:   failed - movies: {}, tv shows: {}, episodes: {}, music "
              "videos: {}",
              count(failed, MOVIE), count(failed, TV_SHOW), count(failed, EPISODE),
              count(failed, MUSIC_VIDEO));

    // There can be very many of these on a first scan
    for (const auto& item : added)
      CLog::Log(LOGDEBUG, "VideoInfoScanner: Added {} '{}' as '{}'", name(item.type),
                CURL::GetRedacted(item.path), item.title);

    for (const auto& item : notFound)
      CLog::Log(LOGINFO, "VideoInfoScanner: Not added {} '{}' as no information was found",
                name(item.type), CURL::GetRedacted(item.path));
    for (const auto& item : failed)
      CLog::Log(LOGINFO, "VideoInfoScanner: Not added {} '{}' as its lookup failed",
                name(item.type), CURL::GetRedacted(item.path));

    if (auto* eventLog{CServiceBroker::GetEventLog()}; eventLog)
    {
      // Shown when a lookup failed, as a scan in the background no longer asks about each failure
      eventLog->Add(
          std::make_shared<CNotificationEvent>(
              24145,
              StringUtils::Format(
                  CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(24190),
                  added.size(), existing.size(), unchangedDirectories, notFound.size(),
                  failed.size()),
              notFound.empty() && failed.empty() ? EventLevel::Basic : EventLevel::Warning),
          !failed.empty(), false);
    }
  }

  void CVideoInfoScanner::SkipRelatedDirectories(std::string_view originalDirectory)
  {
    std::string directory{originalDirectory};

    // Look for an existing parent directory
    std::string parentDir = URIUtils::GetParentPath(directory);
    while (!parentDir.empty() && parentDir != directory && !CDirectory::Exists(parentDir))
    {
      directory = parentDir;
      parentDir = URIUtils::GetParentPath(directory);
    }

    if (directory != originalDirectory)
    {
      const std::string& parentMsg =
          parentDir.empty()
              ? "no existing root parent directory found"
              : StringUtils::Format("first existing parent directory is '{}'", parentDir);

      CLog::LogF(LOGWARNING, "{}, skipping '{}' and its sub directories", parentMsg, directory);
    }

    if (auto count = RemoveSubDirectories(m_pathsToScan, directory, nullptr))
    {
      m_progress.Finished(count);
      CLog::LogF(LOGWARNING, "skipped {} sub directories.", count);
    }
  }

  size_t CVideoInfoScanner::RemoveSubDirectories(std::set<std::string, std::less<>>& directories,
                                                 std::string_view directory,
                                                 std::function<void(const std::string&)> f)
  {
    size_t count{0};

    if (!URIUtils::HasSlashAtEnd(directory))
    {
      CLog::LogF(LOGWARNING, "The parameter '{}' doesn't end with a path separator. Skipping...",
                 directory);
      return count;
    }

    auto it{directories.lower_bound(directory)};

    while (it != directories.end() && (*it).starts_with(directory))
    {
      if (f)
        f(*it);

      it = directories.erase(it);
      ++count;
    }
    return count;
  }
  } // namespace KODI::VIDEO
