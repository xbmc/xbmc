/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DatabaseManager.h"
#include "FileItem.h"
#include "FileItemList.h"
#include "MediaSource.h"
#include "ServiceBroker.h"
#include "TextureCache.h"
#include "URL.h"
#include "addons/AddonManager.h"
#include "addons/AddonVersion.h"
#include "addons/IAddon.h"
#include "addons/Scraper.h"
#include "events/EventLog.h"
#include "filesystem/BlurayDiscCache.h"
#include "filesystem/Directory.h"
#include "filesystem/DirectoryCache.h"
#include "filesystem/File.h"
#include "filesystem/PluginDirectory.h"
#include "filesystem/SpecialProtocol.h"
#include "interfaces/AnnouncementManager.h"
#include "interfaces/generic/ILanguageInvocationHandler.h"
#include "interfaces/generic/ILanguageInvoker.h"
#include "interfaces/generic/ScriptInvocationManager.h"
#include "jobs/JobManager.h"
#include "language/LanguageLoader.h"
#include "resources/LocalizeStrings.h"
#include "resources/ResourcesComponent.h"
#include "settings/AdvancedSettings.h"
#include "settings/MediaSourceSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "test/TestUtils.h"
#include "utils/Artwork.h"
#include "utils/JSONVariantParser.h"
#include "utils/StreamDetails.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "utils/XBMCTinyXML.h"
#include "utils/XMLUtils.h"
#include "video/FilenameAttributes.h"
#include "video/SetInfoTag.h"
#include "video/VideoDatabase.h"
#include "video/VideoInfoScanner.h"
#include "video/VideoInfoTag.h"
#include "video/VideoManagerTypes.h"
#include "video/jobs/VideoLibraryRefreshingJob.h"
#include "video/jobs/VideoLibraryRefreshingSourceJob.h"
#include "video/jobs/VideoLibraryScanningJob.h"
#include "video/tags/IVideoInfoTagLoader.h"

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <functional>
#include <future>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_set>
#include <utility>
#include <vector>

#ifdef TARGET_WINDOWS
#include <windows.h>
#endif

#include <gtest/gtest.h>

using namespace KODI::VIDEO;

namespace
{
constexpr const char* DB_NAME = "TestVideoInfoScanner";

constexpr const char* SCRAPER_ID = "metadata.test.videoinfoscanner";

constexpr const char* SCRAPER_ADDON = R"(<?xml version="1.0" encoding="UTF-8"?>
<addon id="metadata.test.videoinfoscanner" name="Test scraper" version="1.0.0"
       provider-name="Team Kodi">
  <requires>
    <import addon="xbmc.metadata" version="2.1.0"/>
  </requires>
  <extension point="xbmc.metadata.scraper.movies" library="scraper.py"/>
  <extension point="xbmc.metadata.scraper.tvshows" library="scraper.py"/>
  <extension point="xbmc.addon.metadata">
    <platform>all</platform>
  </extension>
</addon>)";

constexpr const char* OTHER_SCRAPER_ID = "metadata.alternative.videoinfoscanner";

constexpr const char* OTHER_SCRAPER_ADDON = R"(<?xml version="1.0" encoding="UTF-8"?>
<addon id="metadata.alternative.videoinfoscanner" name="Other test scraper" version="1.0.0"
       provider-name="Team Kodi">
  <requires>
    <import addon="xbmc.metadata" version="2.1.0"/>
  </requires>
  <extension point="xbmc.metadata.scraper.movies" library="scraper.py"/>
  <extension point="xbmc.metadata.scraper.tvshows" library="scraper.py"/>
  <extension point="xbmc.addon.metadata">
    <platform>all</platform>
  </extension>
</addon>)";

constexpr std::string_view OTHER_URL{"http://other.invalid/"};

constexpr const char* SITES = "special://temp/TestVideoInfoScanner/sites/";

std::string SiteFolder(const std::string& scraper)
{
  return URIUtils::AddFileToFolder(CSpecialProtocol::TranslatePath(SITES), scraper);
}

struct SRuns
{
  std::mutex lock;
  std::vector<int> threads;
};

/*!
 \brief Stands in for the python interpreter running a test scraper, answering each call from the
 pages of the scraper's site:
 - a search for a title: the page search-<title>, a <results> element holding an <entity> with the
   <title> and <url> of each video found. A search without a page finds nothing.
 - details: the page the url names, a <details> element holding them as an nfo would. Details
   asked for by ids, rather than by a url, are read from the page <type>-<id> of the first id that
   has one.
 - an episode guide: the page the url names, an <episodeguide> element holding an <episode> with
   the <title>, <season>, <epnum> and <url> of each episode.
 - an nfo: other-<n> for the url other.invalid/<n> in it, which only the other scraper recognises.
 A page holding <error/> fails the call, as a site that doesn't answer does; so does asking for
 details or an episode guide that has no page.
 */
class CSiteInvoker : public ILanguageInvoker
{
public:
  CSiteInvoker(ILanguageInvocationHandler* handler, SRuns& runs)
    : ILanguageInvoker(handler),
      m_runs(runs)
  {
  }

protected:
  bool execute(const std::string& script, const std::vector<std::string>& arguments) override
  {
    setState(InvokerStateRunning);
    {
      std::unique_lock lock(m_runs.lock);
      m_runs.threads.push_back(GetId());
    }

    const CURL call{arguments[0] + arguments[2]};
    const int handle{std::stoi(arguments[1])};
    const std::string site{SiteFolder(call.GetHostName())};
    const std::string action{call.GetOption("action")};
    if (action == "find")
      Find(handle, site, call.GetOption("title"));
    else if (action == "getdetails" || action == "getepisodedetails")
      Details(handle, site, call.GetOption("url"), call.GetOption("uniqueIDs"));
    else if (action == "getepisodelist")
      Guide(handle, site, call.GetOption("url"));
    else if (action == "NfoUrl")
      NfoUrl(handle, call.GetHostName(), call.GetOption("nfo"));
    else
      XFILE::CPluginDirectory::EndOfDirectory(handle, false, false, false);

    setState(InvokerStateScriptDone);
    return true;
  }

  bool stop(bool abort) override { return true; }

private:
  static bool Page(const std::string& site, const std::string& url, CXBMCTinyXML& page)
  {
    const std::string path{URIUtils::AddFileToFolder(site, url)};
    return !url.empty() && XFILE::CFile::Exists(path) && page.LoadFile(path) &&
           page.RootElement() != nullptr;
  }

  static bool IsError(const CXBMCTinyXML& page)
  {
    return page.RootElement()->ValueStr() == "error";
  }

  static void Find(int handle, const std::string& site, const std::string& title)
  {
    CXBMCTinyXML page;
    const bool found{Page(site, "search-" + title, page)};
    if (found && !IsError(page))
    {
      for (const TiXmlElement* entity = page.RootElement()->FirstChildElement("entity"); entity;
           entity = entity->NextSiblingElement("entity"))
      {
        std::string name;
        std::string url;
        XMLUtils::GetString(entity, "title", name);
        XMLUtils::GetString(entity, "url", url);
        CFileItem result{url, false};
        result.SetLabel(name);
        XFILE::CPluginDirectory::AddItem(handle, &result, 0);
      }
    }
    XFILE::CPluginDirectory::EndOfDirectory(handle, !found || !IsError(page), false, false);
  }

  static void Details(int handle,
                      const std::string& site,
                      const std::string& url,
                      const std::string& ids)
  {
    CXBMCTinyXML page;
    bool found{Page(site, url, page)};
    CVariant uniqueIDs;
    if (url.empty() && CJSONVariantParser::Parse(ids, uniqueIDs))
    {
      for (auto it = uniqueIDs.begin_map(); !found && it != uniqueIDs.end_map(); ++it)
        found = Page(site, it->first + "-" + it->second.asString(), page);
    }

    CFileItem details{url, false};
    if (!found || IsError(page))
    {
      XFILE::CPluginDirectory::SetResolvedUrl(handle, false, &details);
      return;
    }
    details.GetVideoInfoTag()->Load(page.RootElement(), false);
    XFILE::CPluginDirectory::SetResolvedUrl(handle, true, &details);
  }

  static void Guide(int handle, const std::string& site, const std::string& url)
  {
    CXBMCTinyXML page;
    if (!Page(site, url, page) || IsError(page))
    {
      XFILE::CPluginDirectory::EndOfDirectory(handle, false, false, false);
      return;
    }
    for (const TiXmlElement* episode = page.RootElement()->FirstChildElement("episode"); episode;
         episode = episode->NextSiblingElement("episode"))
    {
      std::string title;
      std::string episodeUrl;
      CFileItem item;
      CVideoInfoTag& tag{*item.GetVideoInfoTag()};
      XMLUtils::GetString(episode, "title", title);
      XMLUtils::GetString(episode, "url", episodeUrl);
      XMLUtils::GetInt(episode, "season", tag.m_iSeason);
      XMLUtils::GetInt(episode, "epnum", tag.m_iEpisode);
      tag.SetTitle(title);
      item.SetPath(episodeUrl);
      XFILE::CPluginDirectory::AddItem(handle, &item, 0);
    }
    XFILE::CPluginDirectory::EndOfDirectory(handle, true, false, false);
  }

  static void NfoUrl(int handle, const std::string& scraper, const std::string& nfo)
  {
    if (const size_t at{nfo.find(OTHER_URL)};
        scraper == OTHER_SCRAPER_ID && at != std::string::npos)
    {
      CFileItem url{"other-" + nfo.substr(at + OTHER_URL.size()), false};
      XFILE::CPluginDirectory::AddItem(handle, &url, 1);
    }
    XFILE::CPluginDirectory::EndOfDirectory(handle, true, false, false);
  }

  SRuns& m_runs;
};

class CSiteHandler : public ILanguageInvocationHandler
{
public:
  std::shared_ptr<ILanguageInvoker> CreateInvoker() override
  {
    return std::make_shared<CSiteInvoker>(this, m_runs);
  }

  SRuns m_runs;
};

//! The test's database, named as CVideoDatabase::Open() names the one the settings give it
class TestDatabase : public CVideoDatabase
{
public:
  using CDatabase::GetSchemaVersion;
};

//! A refresh without its GUI, of the test's database
class TestRefreshingJob : public CVideoLibraryRefreshingJob
{
public:
  using CVideoLibraryRefreshingJob::CVideoLibraryRefreshingJob;
  using CVideoLibraryRefreshingJob::Work;
};

//! A refresh of a source of the test's database, whose movies are each refreshed with a database
//! of their own
class TestRefreshingSourceJob : public CVideoLibraryRefreshingSourceJob
{
public:
  using CVideoLibraryRefreshingSourceJob::CVideoLibraryRefreshingSourceJob;
  using CVideoLibraryRefreshingSourceJob::Work;
};

//! A scan without its GUI: Start() needs one, so the folders are scanned with DoScan()
class TestScanner : public CVideoInfoScanner
{
public:
  explicit TestScanner(const DatabaseSettings& settings,
                       bool ignoreVideoExtras = false,
                       bool clean = false)
  {
    m_database.Connect(settings.name, settings, false);
    m_similarVideoAction = SimilarVideoScanAction::NONE;
    m_ignoreVideoExtras = ignoreVideoExtras;
    m_bClean = clean;
  }

  using CVideoInfoScanner::CanLookUpMovieAlongside;

  //! Looks the movies of \p folders beside each other up together, rather than of 64
  void LimitFoldersLookedUpTogether(size_t folders) { m_foldersLookedUpTogether = folders; }

  //! The folders a scan would plan with \p directory, itself included, \p budget of them below it
  size_t FoldersPlannedWith(const std::string& directory, size_t budget)
  {
    const std::unique_ptr<DirectoryScan> scan{PlanDirectory(directory)};
    if (!scan || !CanScanAlongside(*scan))
      return 0;
    PlanAhead(*scan, budget);
    return CountPlanned(*scan);
  }

  //! The progress shown once \p directory, which the scan was asked to scan, is listed and \p share
  //! of the first folder it holds is done
  float ProgressOnceListed(const std::string& directory, float share)
  {
    m_progress.Start({directory}, directory);
    const std::unique_ptr<DirectoryScan> scan{PlanDirectory(directory)};
    return m_progress.Percentage(share);
  }

  using CVideoInfoScanner::FolderTime;
  using CVideoInfoScanner::ScannedSubfolders;
  using CVideoInfoScanner::Snapshot;

  //! As Start() and Process() do, but for the clean where \p stoppedBeforeTheClean
  void Scan(const std::string& directory, bool stoppedBeforeTheClean = false)
  {
    m_pathsToScan.insert(directory);
    std::vector<std::pair<int, std::string>> subPaths;
    m_database.GetSubPaths(directory, subPaths);
    for (const auto& [_, path] : subPaths)
      m_pathsToScan.insert(path);

    while (!m_pathsToScan.empty())
    {
      // A copy, as DoScan() first removes the path from the set
      const std::string folder{*m_pathsToScan.begin()};
      m_progress.Start(m_pathsToScan, folder);
      DoScan(folder);
      std::erase_if(m_pathsToScan,
                    [this, &folder](const std::string& path)
                    {
                      if (!path.starts_with(folder))
                        return false;
                      if (m_bClean)
                        m_pathsToClean.insert(m_database.GetPathId(path));
                      return true;
                    });
    }

    if (m_bClean && !stoppedBeforeTheClean)
      m_database.CleanDatabase(nullptr, m_pathsToClean, false);
  }

  //! As a scan adds the main playlists found on a disc, with grouping of similar videos on
  InfoRet AddPlaylists(const CFileItemList& playlists)
  {
    m_similarVideoAction = SimilarVideoScanAction::AUTO;
    return AddBlurayPlaylistVersions(playlists, nullptr, true, false);
  }

  //! Groups the similar videos it finds without asking
  void GroupSimilarVideos() { m_similarVideoAction = SimilarVideoScanAction::AUTO; }

  //! As a scan does with a movie it has added, with grouping of similar videos on
  std::pair<VersionConversionResult, int> GroupWithSimilar(int dbId)
  {
    GroupSimilarVideos();
    return ProcessVideoVersion(VideoDbContentType::MOVIES, dbId);
  }

  using CVideoInfoScanner::CollectShowFiles;
  using CVideoInfoScanner::EnumerateSeriesFolder;
  using CVideoInfoScanner::FolderSettings;
  using CVideoInfoScanner::GetMovieIdentifier;
  using CVideoInfoScanner::SourceName;
  using CScraperCalls = CVideoInfoScanner::CScraperCalls;
  using EpisodeResult = CVideoInfoScanner::EpisodeResult;
  using ScanSummary = CVideoInfoScanner::ScanSummary;

  const ScanSummary& Summary() const { return m_summary; }

  CVideoDatabase& Database() { return m_database; }

  float Progress() { return m_progress.Percentage(); }
};

class TestVideoInfoScanner : public ::testing::Test
{
protected:
  void SetUp() override
  {
    // A folder of each test's own, as the directory cache may still hold another test's listing
    m_root = URIUtils::AddFileToFolder(
        CSpecialProtocol::TranslatePath("special://temp/TestVideoInfoScanner/"),
        ::testing::UnitTest::GetInstance()->current_test_info()->name());
    URIUtils::AddSlashAtEnd(m_root);
    XFILE::CDirectory::RemoveRecursive(m_root);
    ASSERT_TRUE(XFILE::CDirectory::Create(m_root));

    // A refresh opens the database the settings name, as does the scanner it adds the video again
    // with, once the database manager has updated the profile's databases. The settings name the
    // test's database, which Open() gives its version.
    if (!CServiceBroker::GetDatabaseManager().CanOpen("MyVideos"))
    {
      ASSERT_TRUE(CServiceBroker::GetDatabaseManager().Initialize());
    }
    m_settings.type = "sqlite3";
    m_settings.name = DB_NAME;
    m_settings.host = CSpecialProtocol::TranslatePath("special://temp/");
    auto& videoDatabase{
        CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_databaseVideo};
    m_videoDatabase = videoDatabase;
    videoDatabase = m_settings;
    m_settings.name += std::to_string(m_db.GetSchemaVersion());
    ASSERT_EQ(CDatabase::ConnectionState::STATE_CONNECTED,
              m_db.Connect(m_settings.name, m_settings, true));

    // The test environment has no announcement manager. An unstarted one only queues them.
    m_previousAnnouncementManager = CServiceBroker::GetAnnouncementManager();
    CServiceBroker::RegisterAnnouncementManager(
        std::make_shared<ANNOUNCEMENT::CAnnouncementManager>());
    // Neither has it the cache of what discs hold, which reading a disc's playlists fills
    m_previousDiscCache = CServiceBroker::GetBlurayDiscCache();
    CServiceBroker::RegisterBlurayDiscCache(std::make_shared<XFILE::CBlurayDiscCache>());

    // A movie source using folder names, whose movies are read from their nfos. The clean looks
    // for each file's video source among the sources.
    CMediaSource source;
    source.FromNameAndPaths(m_root, {m_root});
    CMediaSourceSettings::GetInstance()
        .GetSources(KODI::MEDIA::MediaSection::VIDEO)
        .push_back(source);
    ASSERT_TRUE(m_db.ExecuteQuery(m_db.PrepareSQL(
        "UPDATE path SET strContent='movies', strScraper='metadata.local', useFolderNames=1, "
        "scanRecursive=2147483647, noUpdate=0 WHERE idPath=%i",
        m_db.AddPath(m_root))));
  }

  void TearDown() override
  {
    std::erase_if(CMediaSourceSettings::GetInstance().GetSources(KODI::MEDIA::MediaSection::VIDEO),
                  [this](const CMediaSource& source) { return source.strPath == m_root; });
    CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_bVideoLibraryUseFastHash =
        m_fastHash;
    CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_moviesExcludeFromScanRegExps =
        m_excludes;
    CServiceBroker::GetSettingsComponent()->GetSettings()->SetBool(
        CSettings::SETTING_MYVIDEOS_EXTRACTFLAGS, m_extractFlags);
    if (m_standIn)
    {
      std::vector<int> threads;
      {
        std::unique_lock lock(m_handler.m_runs.lock);
        threads = m_handler.m_runs.threads;
      }
      const auto end{std::chrono::steady_clock::now() + std::chrono::seconds{5}};
      for (const int id : threads)
      {
        while (Scripts().IsRunning(id) && std::chrono::steady_clock::now() < end)
          std::this_thread::sleep_for(std::chrono::milliseconds{1});
        EXPECT_FALSE(Scripts().IsRunning(id));
      }
      Scripts().UnregisterLanguageInvocationHandler(&m_handler);
    }
    for (const auto& [id, folder] : m_installedScrapers)
    {
      CServiceBroker::GetAddonMgr().UnloadAddon(id);
      XFILE::CDirectory::RemoveRecursive(folder);
    }
    XFILE::CDirectory::RemoveRecursive(CSpecialProtocol::TranslatePath(SITES));
    if (m_similarVideoAction >= 0)
      CServiceBroker::GetSettingsComponent()->GetSettings()->SetInt(
          CSettings::SETTING_VIDEOLIBRARY_SIMILARVIDEOACTION, m_similarVideoAction);

    CServiceBroker::UnregisterAnnouncementManager();
    if (m_previousAnnouncementManager)
      CServiceBroker::RegisterAnnouncementManager(m_previousAnnouncementManager);
    if (m_previousDiscCache)
      CServiceBroker::RegisterBlurayDiscCache(m_previousDiscCache);
    else
      CServiceBroker::UnregisterBlurayDiscCache();

    if (m_videoDatabase)
      CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_databaseVideo =
          *m_videoDatabase;
    m_db.Close();
    XFILE::CFile::Delete(m_settings.host + m_settings.name + ".db");
    XFILE::CDirectory::RemoveRecursive(m_root);
  }

  static std::string Folder(const std::string& parent, const std::string& name)
  {
    std::string folder{URIUtils::AddFileToFolder(parent, name)};
    URIUtils::AddSlashAtEnd(folder);
    EXPECT_TRUE(XFILE::CDirectory::Create(folder));
    return folder;
  }

  static std::string File(const std::string& folder,
                          const std::string& name,
                          const std::string& content = {})
  {
    const std::string path{URIUtils::AddFileToFolder(folder, name)};
    XFILE::CFile file;
    EXPECT_TRUE(file.OpenForWrite(path, true));
    if (!content.empty())
      file.Write(content.data(), content.size());
    file.Close();
    return path;
  }

  //! A full nfo, so the movie is added without a scraper
  static void Nfo(const std::string& folder, const std::string& name, const std::string& title)
  {
    File(folder, name, "<movie><title>" + title + "</title></movie>");
  }

  //! A movie in a folder of its own, named by its folder's nfo
  static std::string FolderMovie(const std::string& folder, const std::string& file)
  {
    Nfo(folder, "movie.nfo", URIUtils::GetFileName(folder.substr(0, folder.size() - 1)));
    return File(folder, file);
  }

  //! A dvd folder rip
  //! \return the rip's VIDEO_TS.IFO
  static std::string DvdRip(const std::string& folder)
  {
    const std::string videoTs{Folder(folder, "VIDEO_TS")};
    const std::string disc{File(videoTs, "VIDEO_TS.IFO")};
    File(videoTs, "VTS_01_0.IFO");
    File(videoTs, "VTS_01_1.VOB");
    return disc;
  }

  //! What a scan makes of a disc rip in a folder depends on whether its folder was scanned before,
  //! which the hash of the folder holding it doesn't, so that the next scan doesn't look at it again
  void FolderHoldingADiscRipKeepsItsHash(bool dvd)
  {
    const std::string folder{Folder(m_root, "Film (2010)")};
    Nfo(folder, "movie.nfo", "Film");
    if (dvd)
      DvdRip(folder);
    else
      BlurayRip(folder);
    Scan();
    ASSERT_EQ(1, CountMovies());
    const std::string first{Record(m_root)};

    Scan();

    // What the listing held. The folder's time is read again where the file system updated it late.
    const std::string second{Record(m_root)};
    EXPECT_EQ(StringUtils::Split(first, '|')[3], StringUtils::Split(second, '|')[3]);
    EXPECT_EQ(StringUtils::Split(first, '|')[1], StringUtils::Split(second, '|')[1]);
  }

  //! A bluray folder rip, with the backup copy of its index that a BDMV folder holds
  //! \return the rip's index.bdmv
  static std::string BlurayRip(const std::string& folder)
  {
    const std::string bdmv{Folder(folder, "BDMV")};
    const std::string index{File(bdmv, "index.bdmv")};
    File(bdmv, "MovieObject.bdmv");
    File(Folder(bdmv, "BACKUP"), "index.bdmv");
    File(Folder(bdmv, "PLAYLIST"), "00000.mpls");
    File(Folder(bdmv, "STREAM"), "00000.m2ts");
    return index;
  }

  //! An AVCHD folder rip, with the backup copies that its BDMV folder holds. Its index, movie
  //! object, playlists and streams all have video extensions.
  //! \return the rip's INDEX.BDM
  static std::string AvchdRip(const std::string& folder)
  {
    const std::string bdmv{Folder(folder, "BDMV")};
    const std::string index{File(bdmv, "INDEX.BDM")};
    File(bdmv, "MOVIEOBJ.BDM");
    File(Folder(bdmv, "PLAYLIST"), "00000.MPL");
    File(Folder(bdmv, "STREAM"), "00001.MTS");
    const std::string backup{Folder(bdmv, "BACKUP")};
    File(backup, "INDEX.BDM");
    File(backup, "MOVIEOBJ.BDM");
    File(Folder(backup, "PLAYLIST"), "00000.MPL");
    return index;
  }

  //! Moves a folder's modified time on, as adding or removing a file does, a second or more later.
  //! A day on, as an hour on in the hour before the clocks go back is the same local time, which
  //! the hash of the folder holding it uses.
  static void Touch(const std::string& folder)
  {
    const std::filesystem::path path{
        std::u8string{reinterpret_cast<const char8_t*>(folder.data()), folder.size()}};
    std::filesystem::last_write_time(path,
                                     std::filesystem::last_write_time(path) + std::chrono::days{1});
  }

  //! Sets a folder's modified time explicitly, a day back. Windows gives the time a folder's parent
  //! lists it with from the parent's index, which it updates late after a file is added, but at
  //! once when the time is set.
  static void Settle(const std::string& folder)
  {
    const std::filesystem::path path{
        std::u8string{reinterpret_cast<const char8_t*>(folder.data()), folder.size()}};
    std::filesystem::last_write_time(path,
                                     std::filesystem::last_write_time(path) - std::chrono::days{1});
  }

  //! Makes a change in a folder and puts the folder's modified time back, as it was before the
  //! change, so that the folder looks as it did to a scan deciding from its time alone
  static void KeepingItsTime(const std::string& folder, const std::function<void()>& change)
  {
    const std::filesystem::path path{
        std::u8string{reinterpret_cast<const char8_t*>(folder.data()), folder.size()}};
    const auto time{std::filesystem::last_write_time(path)};
    change();
    std::filesystem::last_write_time(path, time);
  }

  //! What the scan recorded for a folder
  std::string Record(const std::string& folder)
  {
    return m_db.GetSingleValue(
        m_db.PrepareSQL("SELECT strHash FROM path WHERE strPath='%s'", folder.c_str()));
  }

  //! Records something else for a folder
  void SetRecord(const std::string& folder, const std::string& record)
  {
    ASSERT_TRUE(m_db.SetPathHash(folder, record));
  }

  //! The record of a folder with one of its fields replaced
  std::string RecordWithField(const std::string& folder, size_t field, const std::string& value)
  {
    std::vector<std::string> fields{StringUtils::Split(Record(folder), '|')};
    EXPECT_EQ(5U, fields.size());
    if (field < fields.size())
      fields[field] = value;
    return StringUtils::Join(fields, "|");
  }

  //! Scans the source holding a movie, makes \p change, adds a second movie without moving the
  //! source's time, and scans again
  //! \return the second movie's file
  std::string MovieAddedKeepingItsTime(const std::function<void()>& change = {})
  {
    Nfo(m_root, "First.nfo", "First");
    File(m_root, "First.mkv");
    Settle(m_root);
    Scan();
    EXPECT_EQ(1, CountMovies());
    EXPECT_TRUE(Record(m_root).starts_with("2|"));
    if (change)
      change();

    std::string second;
    KeepingItsTime(m_root,
                   [this, &second]
                   {
                     Nfo(m_root, "Second.nfo", "Second");
                     second = File(m_root, "Second.mkv");
                   });
    Scan();
    return second;
  }

  //! The folder holding .nomedia is the only folder of its parent. Deleting the file changes the
  //! folder's time, not its parent's.
  void NomediaFileDeleted(bool fastHash)
  {
    CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_bVideoLibraryUseFastHash =
        fastHash;
    const std::string folder{Folder(m_root, "Film (2010)")};
    FolderMovie(folder, "Film (2010).mkv");
    const std::string noMedia{File(folder, ".nomedia")};
    Scan();
    ASSERT_EQ(0, CountMovies());

    KeepingItsTime(m_root,
                   [&noMedia, &folder]
                   {
                     ASSERT_TRUE(XFILE::CFile::Delete(noMedia));
                     Touch(folder);
                   });
    Scan();

    EXPECT_EQ(1, CountMovies());
  }

  //! \return What the scan did
  TestScanner::ScanSummary Scan(bool ignoreVideoExtras = false,
                                bool clean = false,
                                const std::string& path = {})
  {
    TestScanner scanner{m_settings, ignoreVideoExtras, clean};
    if (m_similarVideoAction >= 0)
      scanner.GroupSimilarVideos();
    scanner.Scan(path.empty() ? m_root : path);
    return scanner.Summary();
  }

  static CScriptInvocationManager& Scripts() { return CScriptInvocationManager::GetInstance(); }

  void InstallScraper(const std::string& id, const std::string& addonXml)
  {
    if (!m_standIn)
    {
      ASSERT_FALSE(Scripts().HasLanguageInvoker("scraper.py"))
          << "python scripts are run by python";
      Scripts().RegisterLanguageInvocationHandler(&m_handler, ".py");
      m_standIn = true;
    }

    const std::string addons{CSpecialProtocol::TranslatePath("special://home/addons/")};
    static_cast<void>(XFILE::CDirectory::Create(addons));
    const std::string folder{Folder(addons, id)};
    File(folder, "addon.xml", addonXml);
    File(folder, "scraper.py");
    m_installedScrapers.emplace_back(id, folder);
    // Found rather than loaded, as loading publishes an event through the job manager a test
    // doesn't have
    ASSERT_TRUE(CServiceBroker::GetAddonMgr().FindAddon(id, ADDON::ORIGIN_SYSTEM,
                                                        ADDON::CAddonVersion{"1.0.0"}));
  }

  //! Looks the source's videos up with the test scraper, rather than reading them only from their
  //! nfos
  void UseTestScraper()
  {
    InstallScraper(SCRAPER_ID, SCRAPER_ADDON);
    ASSERT_TRUE(m_db.ExecuteQuery(
        m_db.PrepareSQL("UPDATE path SET strScraper='%s', strSettings='' WHERE idPath=%i",
                        SCRAPER_ID, m_db.GetPathId(m_root))));
  }

  void InstallOtherScraper() { InstallScraper(OTHER_SCRAPER_ID, OTHER_SCRAPER_ADDON); }

  static void Page(const std::string& url,
                   const std::string& content,
                   const std::string& scraper = SCRAPER_ID)
  {
    const std::string site{SiteFolder(scraper)};
    static_cast<void>(XFILE::CDirectory::Create(CSpecialProtocol::TranslatePath(SITES)));
    static_cast<void>(XFILE::CDirectory::Create(site));
    File(site, url, content);
  }

  static void Found(const std::string& title)
  {
    Page("search-" + title, "<results><entity><title>" + title + "</title><url>details-" + title +
                                "</url></entity></results>");
  }

  //! Searching for the title fails, as when the site can't be reached
  static void SearchFails(const std::string& title) { Page("search-" + title, "<error/>"); }

  //! The title's details, once found
  static void Details(const std::string& title, const std::string& details)
  {
    Page("details-" + title, details);
  }

  //! The title is found, but its details can't be retrieved
  static void DetailsFail(const std::string& title)
  {
    Found(title);
    Details(title, "<error/>");
  }

  //! The title is found, with details titling it "<title> from the scraper"
  static void FoundFromTheScraper(const std::string& title)
  {
    Found(title);
    Details(title, "<details><title>" + title + " from the scraper</title></details>");
  }

  bool IsExtraOf(const std::string& path, int idMovie)
  {
    const VideoAssetInfo asset{m_db.GetVideoVersionInfo(path)};
    return asset.m_assetType == VideoAssetType::EXTRA && asset.m_idMedia == idMovie;
  }

  int CountExtras()
  {
    return m_db.GetSingleValueInt(m_db.PrepareSQL(
        "SELECT COUNT(*) FROM videoversion WHERE itemType=%i", VideoAssetType::EXTRA));
  }

  int CountMovies() { return m_db.GetSingleValueInt("SELECT COUNT(*) FROM movie"); }

  //! Searching for Film finds it, with its tmdb id, which a refresh looks it up by
  static void FilmFound()
  {
    static constexpr const char* FILM{
        R"(<details><title>Film</title><uniqueid type="tmdb" default="true">100</uniqueid></details>)"};
    Found("Film");
    Details("Film", FILM);
    Page("tmdb-100", FILM);
  }

  //! Refreshes a movie as the information dialog does, or as a refresh of a source does each of
  //! its movies, from the movie as it was listed
  bool Refresh(const CVideoInfoTag& listed)
  {
    TestRefreshingJob refresh{std::make_shared<CFileItem>(listed), true, false};
    return refresh.Work(m_db);
  }

  //! "(Disc n)" is a localised string, which a test naming a movie by its disc loads
  struct Strings
  {
    Strings() : strings{CServiceBroker::GetResourcesComponent().GetLocalizeStrings()}
    {
      loaded = strings.Load(KODI::LANGUAGE::CLanguageLoader::GetLanguagePath(),
                            "resource.language.en_gb");
    }
    ~Strings() { strings.Clear(); }

    CLocalizeStrings& strings;
    bool loaded{false};
  };

  //! A movie of its own of the same film, as one disc of it added before grouping. An imdb id
  //! is given when it has one as well as its tmdb id.
  int AddFilm(const std::string& file, const std::string& imdb = {})
  {
    CVideoInfoTag tag;
    tag.m_strTitle = "Film";
    tag.m_strFileNameAndPath = file;
    tag.m_strPath = URIUtils::GetDirectory(file);
    tag.SetUniqueID("100", "tmdb", true);
    if (!imdb.empty())
      tag.SetUniqueID(imdb, "imdb");
    return m_db.SetDetailsForMovie(tag, KODI::ART::Artwork{});
  }

  //! Groups the similar videos found without asking, as the setting's default does
  void GroupSimilarVideosAutomatically()
  {
    const auto settings{CServiceBroker::GetSettingsComponent()->GetSettings()};
    m_similarVideoAction = settings->GetInt(CSettings::SETTING_VIDEOLIBRARY_SIMILARVIDEOACTION);
    settings->SetInt(CSettings::SETTING_VIDEOLIBRARY_SIMILARVIDEOACTION,
                     static_cast<int>(SimilarVideoScanAction::AUTO));
  }

  //! A file added or deleted below an extras folder changes only the folder holding it. The movie
  //! is a file in its folder, or a disc structure the folder holds.
  void ScanChangesBelowTheExtras(bool disc = false)
  {
    const std::string folder{Folder(m_root, "Film (2010)")};
    if (disc)
    {
      Nfo(folder, "movie.nfo", "Film");
      BlurayRip(folder);
    }
    else
      FolderMovie(folder, "Film (2010).mkv");
    const std::string extras{Folder(folder, "Extras")};
    const std::string featurettes{Folder(extras, "Featurettes")};
    File(featurettes, "Trailer.mkv");
    Settle(extras);
    Scan();
    ASSERT_EQ(1, CountMovies());
    ASSERT_EQ(1, CountExtras());
    const int idMovie{m_db.GetSingleValueInt("SELECT idMovie FROM movie")};

    const std::string interview{File(featurettes, "Interview.mkv")};
    Touch(featurettes);
    Scan();
    EXPECT_TRUE(IsExtraOf(interview, idMovie));
    EXPECT_EQ(2, CountExtras());

    ASSERT_TRUE(XFILE::CFile::Delete(interview));
    Touch(featurettes);
    Scan(false, true);
    EXPECT_EQ(1, CountExtras());
    EXPECT_EQ(-1, m_db.GetVideoVersionInfo(interview).m_assetTypeId);
  }

  //! The extras a bonus disc names are its playlists, as a refresh adds them, so its folder is
  //! known only by them. A video added beside the disc changes neither the movie's folder nor the
  //! library. The disc is an image, or a folder rip.
  void VideoAddedBesideABonusDisc(bool rip)
  {
    const std::string folder{Folder(m_root, "Film (2010)")};
    const std::string movie{FolderMovie(folder, "Film (2010).mkv")};
    const std::string bonus{Folder(folder, "Bonus Disc")};
    Scan();
    const int idMovie{m_db.GetMovieId(movie)};
    ASSERT_GT(idMovie, 0);

    ASSERT_TRUE(
        m_db.ExecuteQuery(m_db.PrepareSQL("DELETE FROM path WHERE strPath='%s'", bonus.c_str())));
    const std::string disc{rip ? URIUtils::AddFileToFolder(bonus, "BDMV", "index.bdmv")
                               : URIUtils::AddFileToFolder(bonus, "BONUS.iso")};
    CFileItem playlist{URIUtils::GetBlurayPlaylistPath(disc, 1), false};
    ASSERT_TRUE(
        m_db.AddVideoAsset(VideoDbContentType::MOVIES, idMovie,
                           m_db.AddOrValidateVideoVersionType("Featurette", VideoAssetType::EXTRA),
                           VideoAssetType::EXTRA, playlist));

    const std::string interview{File(bonus, "Interview.mkv")};
    Scan();

    EXPECT_TRUE(IsExtraOf(interview, idMovie));
  }

  //! A refresh removes a movie before adding it again, so one whose details can't be read is
  //! kept. The movie has no nfo, or one that overrides some of the details it is looked up for.
  void RefreshThatCantReadTheDetails(bool overrideNfo)
  {
    UseTestScraper();
    const std::string folder{Folder(m_root, "Afilm")};
    const std::string film{File(folder, "Afilm.mkv")};
    if (overrideNfo)
      File(folder, "Afilm.nfo", "<movie override=\"true\"><title>Afilm</title></movie>");
    Found("Afilm");
    Details("Afilm", "<details><title>Afilm</title></details>");
    Scan();
    const int idMovie{m_db.GetMovieId(film)};
    ASSERT_GT(idMovie, 0);

    DetailsFail("Afilm");
    CVideoInfoTag tag;
    ASSERT_TRUE(m_db.GetMovieInfo({}, tag, idMovie));
    TestRefreshingJob refresh{std::make_shared<CFileItem>(tag), true, false};
    refresh.Work(m_db);

    EXPECT_EQ(idMovie, m_db.GetMovieId(film));
    EXPECT_FALSE(refresh.RemovedItem());
  }

  std::string m_root;
  DatabaseSettings m_settings;
  std::optional<DatabaseSettings> m_videoDatabase;
  TestDatabase m_db;
  std::shared_ptr<ANNOUNCEMENT::CAnnouncementManager> m_previousAnnouncementManager;
  std::shared_ptr<XFILE::CBlurayDiscCache> m_previousDiscCache;
  const bool m_fastHash{
      CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_bVideoLibraryUseFastHash};
  const std::vector<std::string> m_excludes{CServiceBroker::GetSettingsComponent()
                                                ->GetAdvancedSettings()
                                                ->m_moviesExcludeFromScanRegExps};
  const bool m_extractFlags{CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
      CSettings::SETTING_MYVIDEOS_EXTRACTFLAGS)};
  CSiteHandler m_handler;
  bool m_standIn{false};
  std::vector<std::pair<std::string, std::string>> m_installedScrapers;
  int m_similarVideoAction{-1};
};
} // namespace

// The backup copy of the index is matched too by a pattern found anywhere in the path, as an air
// date is
TEST_F(TestVideoInfoScanner, ABlurayRipOfADatedEpisodeIsOneEpisode)
{
  const std::string show{Folder(m_root, "Show")};
  BlurayRip(Folder(show, "Show.2023-11-05"));

  TestScanner scanner{m_settings};
  CFileItem item{show, true};
  EPISODELIST episodes;
  EXPECT_EQ(TestScanner::EpisodeResult::FOUND_EPISODES,
            scanner.EnumerateSeriesFolder(&item, episodes));

  EXPECT_EQ(1U, episodes.size());
}

// The rip's index and movie object are both dated by its folder
TEST_F(TestVideoInfoScanner, AnAvchdRipOfADatedEpisodeIsOneEpisode)
{
  const std::string show{Folder(m_root, "Show")};
  AvchdRip(Folder(show, "Show.2023-11-05"));

  TestScanner scanner{m_settings};
  CFileItem item{show, true};
  EPISODELIST episodes;
  EXPECT_EQ(TestScanner::EpisodeResult::FOUND_EPISODES,
            scanner.EnumerateSeriesFolder(&item, episodes));

  ASSERT_EQ(1U, episodes.size());
  EXPECT_EQ("2023-11-05", episodes[0].cDate.GetAsDBDate());
}

TEST_F(TestVideoInfoScanner, AddingASetLeavesTheDatabaseAsItWas)
{
  TestScanner scanner{m_settings};
  CVideoDatabase& database{scanner.Database()};
  ASSERT_TRUE(database.IsOpen());

  CSetInfoTag set;
  set.SetTitle("Collection");
  EXPECT_TRUE(scanner.AddSet(set));

  // Close() only closes once it has been called as often as Open()
  database.Close();
  EXPECT_FALSE(database.IsOpen());
}

TEST_F(TestVideoInfoScanner, ACancelledScanDoesNotHideTheSameScanAskedForAgain)
{
  CVideoLibraryScanningJob cancelled{m_root};
  const CVideoLibraryScanningJob again{m_root};
  ASSERT_TRUE(cancelled.Equals(&again));

  cancelled.Cancel();

  EXPECT_FALSE(cancelled.Equals(&again));
}

// Movies

TEST_F(TestVideoInfoScanner, AMovieIsAddedFromItsNfo)
{
  const std::string folder{Folder(m_root, "Film (2010)")};
  const std::string movie{FolderMovie(folder, "Film (2010).mkv")};

  Scan();

  EXPECT_GT(m_db.GetMovieId(movie), 0);
  EXPECT_EQ("Film (2010)", m_db.GetSingleValue("SELECT c00 FROM movie"));
}

// A measurement of a file's picture that read nothing is stored with its movie too, so that the
// file isn't measured again
TEST_F(TestVideoInfoScanner, AMeasurementOfAMoviesPictureThatReadNothingIsStored)
{
  const auto settings{CServiceBroker::GetSettingsComponent()->GetSettings()};
  const bool measure{settings->GetBool(CSettings::SETTING_VIDEOSCREEN_EXTRACTCONTENTGEOMETRY)};
  const bool onScan{settings->GetBool(CSettings::SETTING_VIDEOSCREEN_CONTENTGEOMETRYONSCAN)};
  settings->SetBool(CSettings::SETTING_VIDEOSCREEN_EXTRACTCONTENTGEOMETRY, true);
  settings->SetBool(CSettings::SETTING_VIDEOSCREEN_CONTENTGEOMETRYONSCAN, true);
  const std::string movie{FolderMovie(Folder(m_root, "Film (2010)"), "Film (2010).mkv")};

  Scan();
  settings->SetBool(CSettings::SETTING_VIDEOSCREEN_EXTRACTCONTENTGEOMETRY, measure);
  settings->SetBool(CSettings::SETTING_VIDEOSCREEN_CONTENTGEOMETRYONSCAN, onScan);

  const int idFile{m_db.GetFileId(CFileItem{movie, false})};
  ASSERT_GT(idFile, 0);
  const auto record{m_db.GetStoredContentGeometry(idFile)};
  ASSERT_TRUE(record.has_value());
  EXPECT_FALSE(record->HasReading());
}

TEST_F(TestVideoInfoScanner, AMovieAddedToAScannedFolderIsFound)
{
  Nfo(m_root, "First.nfo", "First");
  File(m_root, "First.mkv");
  Scan();
  ASSERT_EQ(1, CountMovies());

  Nfo(m_root, "Second.nfo", "Second");
  const std::string second{File(m_root, "Second.mkv")};
  Touch(m_root);
  const auto summary{Scan()};

  EXPECT_GT(m_db.GetMovieId(second), 0);
  EXPECT_EQ(2, CountMovies());
  ASSERT_EQ(1u, summary.added.size());
  EXPECT_EQ(TestScanner::ScanSummary::Type::MOVIE, summary.added[0].type);
  ASSERT_EQ(1u, summary.existing.size());
  EXPECT_EQ(TestScanner::ScanSummary::Type::MOVIE, summary.existing[0].type);
}

TEST_F(TestVideoInfoScanner, AnUnchangedFolderAddsNothingAgain)
{
  FolderMovie(Folder(m_root, "Film (2010)"), "Film (2010).mkv");
  Scan();

  Scan();

  EXPECT_EQ(1, CountMovies());
  EXPECT_EQ(1, m_db.GetSingleValueInt("SELECT COUNT(*) FROM files"));
}

// A folder without folders to scan is decided by its modified time, without listing it
TEST_F(TestVideoInfoScanner, AFolderWhoseTimeHasNotMovedIsNotListedAgain)
{
  EXPECT_EQ(-1, m_db.GetMovieId(MovieAddedKeepingItsTime()));
}

TEST_F(TestVideoInfoScanner, AFolderHashedBeforeRecordsIsListedAgain)
{
  const std::string second{
      MovieAddedKeepingItsTime([this] { SetRecord(m_root, "0123456789abcdef0123456789abcdef"); })};

  EXPECT_GT(m_db.GetMovieId(second), 0);
  EXPECT_TRUE(Record(m_root).starts_with("2|"));
}

TEST_F(TestVideoInfoScanner, AFolderScannedWithOtherSettingsIsListedAgain)
{
  auto& excludes{CServiceBroker::GetSettingsComponent()
                     ->GetAdvancedSettings()
                     ->m_moviesExcludeFromScanRegExps};
  std::string before;
  const std::string second{MovieAddedKeepingItsTime(
      [this, &before, &excludes]
      {
        before = Record(m_root);
        excludes.emplace_back("matches-nothing-in-this-test");
      })};

  EXPECT_GT(m_db.GetMovieId(second), 0);
  EXPECT_NE(StringUtils::Split(before, '|')[1], StringUtils::Split(Record(m_root), '|')[1]);
}

TEST_F(TestVideoInfoScanner, AFolderRecordedWithoutItsTimeIsListedAgain)
{
  const std::string second{
      MovieAddedKeepingItsTime([this] { SetRecord(m_root, RecordWithField(m_root, 2, "?")); })};

  EXPECT_GT(m_db.GetMovieId(second), 0);
  EXPECT_NE("?", StringUtils::Split(Record(m_root), '|')[2]);
}

// A folder is recorded before the folder holding it, and is linked to it all the same
TEST_F(TestVideoInfoScanner, AFolderIsLinkedToTheFolderHoldingIt)
{
  const std::string collection{Folder(m_root, "Collection")};
  const std::string empty{Folder(collection, "Empty")};

  Scan();

  const int idCollection{m_db.GetPathId(collection)};
  ASSERT_GT(idCollection, 0);
  ASSERT_GT(m_db.GetPathId(empty), 0);
  EXPECT_EQ(idCollection, m_db.GetSingleValueInt(m_db.PrepareSQL(
                              "SELECT idParentPath FROM path WHERE strPath='%s'", empty.c_str())));
}

TEST_F(TestVideoInfoScanner, AMovieDeletedIsCleanedOnUpdate)
{
  const std::string folder{Folder(m_root, "Film (2010)")};
  const std::string movie{FolderMovie(folder, "Film (2010).mkv")};
  Scan();
  ASSERT_EQ(1, CountMovies());

  ASSERT_TRUE(XFILE::CFile::Delete(movie));
  Touch(folder);
  Scan(false, true);

  EXPECT_EQ(0, CountMovies());
}

// So that it is looked at again once it can be, rather than passed over by its time
TEST_F(TestVideoInfoScanner, AFolderThatCantBeListedLosesItsRecord)
{
  const std::string folder{Folder(m_root, "Film (2010)")};
  FolderMovie(folder, "Film (2010).mkv");
  Scan();
  ASSERT_TRUE(Record(folder).starts_with("2|"));

  ASSERT_TRUE(XFILE::CDirectory::RemoveRecursive(folder));
  Scan(false, false, folder);

  EXPECT_EQ("", Record(folder));
}

// A folder's time is the one read before it was listed, so that its record never holds a later
// time than its listing: a video added between the two would otherwise be passed over by its time
TEST_F(TestVideoInfoScanner, AFoldersTimeIsTheOneReadBeforeItWasListed)
{
  const std::string folder{Folder(m_root, "Film (2010)")};
  Settle(folder);
  const std::optional<int64_t> before{ReadModifiedTime(folder)};
  ASSERT_TRUE(before);

  TestScanner scanner{m_settings};
  ASSERT_TRUE(scanner.Snapshot(folder));
  Touch(folder);
  // Nor is it the time a listing of its parent taken since gives
  const auto parent{scanner.Snapshot(m_root)};
  ASSERT_TRUE(parent);
  scanner.ScannedSubfolders(*parent, {});

  EXPECT_EQ(before, scanner.FolderTime(folder));
}

// A movie deleted from a folder, leaving it without a video, is cleaned by the scan after one
// stopped before its clean
TEST_F(TestVideoInfoScanner, AMovieDeletedIsCleanedByTheScanAfterOneStoppedBeforeItsClean)
{
  const std::string folder{Folder(m_root, "Film (2010)")};
  const std::string movie{FolderMovie(folder, "Film (2010).mkv")};
  const std::string other{Folder(m_root, "Other (2011)")};
  FolderMovie(other, "Other (2011).mkv");
  Scan();
  ASSERT_EQ(2, CountMovies());

  ASSERT_TRUE(XFILE::CFile::Delete(movie));
  Touch(folder);
  {
    TestScanner stopped{m_settings, false, true};
    stopped.Scan(m_root, true);
  }
  ASSERT_EQ(2, CountMovies());

  // A change in another folder, so that the clean is of the folders the scan queued, not of all
  File(other, "Notes.txt");
  Touch(other);
  Scan(false, true);
  EXPECT_EQ(1, CountMovies());
}

TEST_F(TestVideoInfoScanner, AFileStackIsOneMovie)
{
  const std::string folder{Folder(m_root, "Film (2010)")};
  FolderMovie(folder, "Film (2010) cd1.mkv");
  File(folder, "Film (2010) cd2.mkv");

  Scan();

  ASSERT_EQ(1, CountMovies());
  EXPECT_TRUE(m_db.GetSingleValue("SELECT strFileName FROM movie_view").starts_with("stack://"));
}

TEST_F(TestVideoInfoScanner, AFolderStackIsOneMovie)
{
  const std::string folder{Folder(m_root, "Film (2010)")};
  Nfo(folder, "movie.nfo", "Film");
  File(Folder(folder, "Film CD1"), "Film.mkv");
  File(Folder(folder, "Film CD2"), "Film.mkv");

  Scan();

  ASSERT_EQ(1, CountMovies());
  EXPECT_TRUE(m_db.GetSingleValue("SELECT strFileName FROM movie_view").starts_with("stack://"));
}

#ifdef HAVE_LIBBLURAY
TEST_F(TestVideoInfoScanner, AFolderHoldingADiscRipKeepsItsHash)
{
  FolderHoldingADiscRipKeepsItsHash(false);
}
#endif

TEST_F(TestVideoInfoScanner, AFolderHoldingADvdRipKeepsItsHash)
{
  FolderHoldingADiscRipKeepsItsHash(true);
}

// A folder holding nothing but an archive, whose movie the scraper didn't know, isn't recorded, so
// that the next scan looks the movie up again, as it does a movie file the scraper didn't know
TEST_F(TestVideoInfoScanner, AMovieInAnArchiveTheScraperDidntKnowIsLookedUpAgain)
{
  UseTestScraper();
  Page("search-Film", "<results></results>");
  const std::string folder{Folder(m_root, "Film")};
  // A zip of an empty Film.mkv, compressed: an archive of one stored video is listed as the video
  constexpr unsigned char zip[]{
      0x50, 0x4b, 0x03, 0x04, 0x14, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x21, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00,
      0x46, 0x69, 0x6c, 0x6d, 0x2e, 0x6d, 0x6b, 0x76, 0x03, 0x00, 0x50, 0x4b, 0x01, 0x02, 0x14,
      0x00, 0x14, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x21, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x01, 0x00, 0x00, 0x00, 0x00, 0x46, 0x69, 0x6c, 0x6d,
      0x2e, 0x6d, 0x6b, 0x76, 0x50, 0x4b, 0x05, 0x06, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01,
      0x00, 0x36, 0x00, 0x00, 0x00, 0x28, 0x00, 0x00, 0x00, 0x00, 0x00};
  File(folder, "Film.zip", std::string(reinterpret_cast<const char*>(zip), sizeof(zip)));
  Settle(folder);
  Scan();
  ASSERT_EQ(0, CountMovies());

  Found("Film");
  Details("Film", "<details><title>Film</title></details>");
  Scan();

  EXPECT_EQ(1, CountMovies());
}

// A disc joining the movie of another disc of it makes the movie span the discs, so it isn't
// named by the one it was added from
TEST_F(TestVideoInfoScanner, AMovieADiscJoinsIsNotNamedByItsOtherDisc)
{
  const Strings strings;
  ASSERT_TRUE(strings.loaded);
  UseTestScraper();
  GroupSimilarVideosAutomatically();
  FilmFound();
  const std::string folder{Folder(m_root, "Film (2010)")};
  const std::string first{File(Folder(folder, "Disc 1"), "Film.mkv")};
  Scan();
  const int idMovie{m_db.GetMovieId(first)};
  ASSERT_GT(idMovie, 0);
  ASSERT_EQ("Film (Disc 1)", m_db.GetMovieTitle(idMovie));

  const std::string second{File(Folder(folder, "Disc 2"), "Film.mkv")};
  Touch(folder);
  Scan();

  EXPECT_EQ(idMovie, m_db.GetMovieId(second));
  EXPECT_EQ("Film", m_db.GetMovieTitle(idMovie));
}

#ifdef HAVE_LIBBLURAY
TEST_F(TestVideoInfoScanner, ABlurayFolderRipIsOneMovie)
{
  const std::string folder{Folder(m_root, "Film (2010)")};
  Nfo(folder, "movie.nfo", "Film");
  const std::string disc{BlurayRip(folder)};

  Scan();

  EXPECT_EQ(1, CountMovies());
  EXPECT_GT(m_db.GetMovieId(disc), 0);
}
#endif

// A bluray's streams are read from its playlist as the scan chooses it, so the disc isn't probed.
// Its subtitles are looked for all the same, as the player looks for them: beside the folder
// holding the disc, named by that folder.
TEST_F(TestVideoInfoScanner, ABlurayRipHasTheSubtitleBesideIt)
{
  CServiceBroker::GetSettingsComponent()->GetSettings()->SetBool(
      CSettings::SETTING_MYVIDEOS_EXTRACTFLAGS, true);
  const std::string folder{Folder(m_root, "Film (2010)")};
  const std::string disc{BlurayRip(folder)};
  File(folder, "Film (2010).en.srt", "1\n00:00:00,000 --> 00:00:01,000\nHello\n");
  // The disc's main playlist, with the streams the scan has read from it
  const auto playlist{std::make_shared<CFileItem>(URIUtils::GetBlurayPlaylistPath(disc, 0), false)};
  playlist->GetVideoInfoTag()->SetTitle("Film");
  auto* video{new CStreamDetailVideo()};
  video->m_iWidth = 1920;
  playlist->GetVideoInfoTag()->m_streamDetails.AddStream(video);
  CFileItemList playlists;
  playlists.Add(playlist);

  TestScanner scanner{m_settings};
  ASSERT_EQ(CInfoScanner::InfoRet::ADDED, scanner.AddPlaylists(playlists));

  CVideoInfoTag tag;
  ASSERT_TRUE(m_db.GetMovieInfo({}, tag, m_db.GetMovieId(playlist->GetDynPath())));
  ASSERT_TRUE(m_db.GetStreamDetails(tag));
  EXPECT_EQ(1920, tag.m_streamDetails.GetVideoWidth());
  EXPECT_EQ(1, tag.m_streamDetails.GetSubtitleStreamCount());
  EXPECT_EQ("eng", tag.m_streamDetails.GetSubtitleLanguage(1).AsIso6392B());
}

// A stack of bluray rips has the streams of its first disc's playlist, and the subtitle the player
// loads as it plays that disc: beside the folder holding the disc, named by that folder
TEST_F(TestVideoInfoScanner, ABlurayStackHasItsFirstDiscsSubtitle)
{
  CServiceBroker::GetSettingsComponent()->GetSettings()->SetBool(
      CSettings::SETTING_MYVIDEOS_EXTRACTFLAGS, true);
  const std::string first{Folder(m_root, "Film (2010) Disc 1")};
  const std::string second{Folder(m_root, "Film (2010) Disc 2")};
  const std::string firstDisc{BlurayRip(first)};
  const std::string secondDisc{BlurayRip(second)};
  File(first, "Film (2010) Disc 1.en.srt", "1\n00:00:00,000 --> 00:00:01,000\nHello\n");
  File(second, "Film (2010) Disc 2.de.srt", "1\n00:00:00,000 --> 00:00:01,000\nHallo\n");
  // The stack of the discs' main playlists, with the streams the scan has read from the first
  const std::string playlists{"stack://" + URIUtils::GetBlurayPlaylistPath(firstDisc, 0) + " , " +
                              URIUtils::GetBlurayPlaylistPath(secondDisc, 0)};
  const auto stack{std::make_shared<CFileItem>(playlists, false)};
  stack->GetVideoInfoTag()->SetTitle("Film");
  auto* video{new CStreamDetailVideo()};
  video->m_iWidth = 1920;
  stack->GetVideoInfoTag()->m_streamDetails.AddStream(video);
  CFileItemList stacks;
  stacks.Add(stack);

  TestScanner scanner{m_settings};
  ASSERT_EQ(CInfoScanner::InfoRet::ADDED, scanner.AddPlaylists(stacks));

  CVideoInfoTag tag;
  ASSERT_TRUE(m_db.GetMovieInfo({}, tag, m_db.GetMovieId(stack->GetDynPath())));
  ASSERT_TRUE(m_db.GetStreamDetails(tag));
  EXPECT_EQ(1920, tag.m_streamDetails.GetVideoWidth());
  EXPECT_EQ(1, tag.m_streamDetails.GetSubtitleStreamCount());
  EXPECT_EQ("eng", tag.m_streamDetails.GetSubtitleLanguage(1).AsIso6392B());
}

TEST_F(TestVideoInfoScanner, PlaylistsOfADiscAlreadyInTheLibraryKeepTheirMovie)
{
  const std::string disc{File(Folder(m_root, "Film (2010)"), "FILM.iso")};
  const std::string main{URIUtils::GetBlurayPlaylistPath(disc, 801)};
  const std::string other{URIUtils::GetBlurayPlaylistPath(disc, 800)};

  CVideoInfoTag tag;
  tag.m_strTitle = "Film";
  tag.m_strFileNameAndPath = main;
  tag.m_strPath = URIUtils::GetDirectory(main);
  const int idMovie{m_db.SetDetailsForMovie(tag, KODI::ART::Artwork{})};
  ASSERT_GT(idMovie, 0);
  CFileItem version{other, false};
  ASSERT_TRUE(m_db.AddVideoAsset(VideoDbContentType::MOVIES, idMovie, VIDEO_VERSION_ID_DEFAULT,
                                 VideoAssetType::VERSION, version));

  CFileItemList playlists;
  playlists.Add(std::make_shared<CFileItem>(main, false));
  playlists.Add(std::make_shared<CFileItem>(other, false));
  TestScanner scanner{m_settings};
  EXPECT_EQ(CInfoScanner::InfoRet::HAVE_ALREADY, scanner.AddPlaylists(playlists));

  EXPECT_EQ(1, CountMovies());
  EXPECT_EQ(idMovie, m_db.GetMovieId(main));
  EXPECT_EQ(idMovie, m_db.GetMovieId(other));
}

// The discs of a movie, added as movies of their own, become one as one of them is added again
TEST_F(TestVideoInfoScanner, DiscsOfAMovieInItsFolderBecomeOneMovie)
{
  GroupSimilarVideosAutomatically();
  const std::string folder{URIUtils::AddFileToFolder(m_root, "Film (2010)")};
  const std::string first{URIUtils::AddFileToFolder(folder, "Disc 1", "Film.mkv")};
  const std::string second{URIUtils::AddFileToFolder(folder, "Disc 2", "Film.mkv")};
  const std::string third{URIUtils::AddFileToFolder(folder, "Disc 3", "Film.mkv")};
  const int idFirst{AddFilm(first)};
  ASSERT_GT(idFirst, 0);
  ASSERT_GT(AddFilm(third), 0);
  const int idSecond{AddFilm(second)};
  ASSERT_GT(idSecond, 0);

  TestScanner scanner{m_settings};
  EXPECT_EQ(VersionConversionResult::SUCCESS, scanner.GroupWithSimilar(idSecond).first);

  EXPECT_EQ(1, CountMovies());
  EXPECT_EQ(idFirst, m_db.GetMovieId(first));
  EXPECT_EQ(idFirst, m_db.GetMovieId(second));
  EXPECT_EQ(idFirst, m_db.GetMovieId(third));
}

// The movie of the lowest disc of those already in the library is kept, though the disc added is
// lower, as they may hold what the user set on them
TEST_F(TestVideoInfoScanner, ALowerDiscAddedJoinsTheMovieOfTheLowestDiscAlreadyThere)
{
  GroupSimilarVideosAutomatically();
  const std::string folder{URIUtils::AddFileToFolder(m_root, "Film (2010)")};
  const std::string first{URIUtils::AddFileToFolder(folder, "Disc 1", "Film.mkv")};
  const std::string second{URIUtils::AddFileToFolder(folder, "Disc 2", "Film.mkv")};
  const std::string third{URIUtils::AddFileToFolder(folder, "Disc 3", "Film.mkv")};
  ASSERT_GT(AddFilm(third), 0);
  const int idSecond{AddFilm(second)};
  ASSERT_GT(idSecond, 0);
  const int idFirst{AddFilm(first)};
  ASSERT_GT(idFirst, 0);

  TestScanner scanner{m_settings};
  EXPECT_EQ(VersionConversionResult::SUCCESS, scanner.GroupWithSimilar(idFirst).first);

  EXPECT_EQ(1, CountMovies());
  EXPECT_EQ(idSecond, m_db.GetMovieId(first));
  EXPECT_EQ(idSecond, m_db.GetMovieId(second));
  EXPECT_EQ(idSecond, m_db.GetMovieId(third));
}

// A copy elsewhere may be kept apart on purpose, so only the disc beside the movie joins it
TEST_F(TestVideoInfoScanner, ACopyOfAMovieInAnotherFolderStaysApart)
{
  GroupSimilarVideosAutomatically();
  const std::string folder{URIUtils::AddFileToFolder(m_root, "Film (2010)")};
  const std::string first{URIUtils::AddFileToFolder(folder, "Disc 1", "Film.mkv")};
  const std::string second{URIUtils::AddFileToFolder(folder, "Disc 2", "Film.mkv")};
  const std::string copy{URIUtils::AddFileToFolder(m_root, "Film Copy (2010)", "Film.mkv")};
  const int idFirst{AddFilm(first)};
  ASSERT_GT(idFirst, 0);
  const int idCopy{AddFilm(copy)};
  ASSERT_GT(idCopy, 0);
  const int idSecond{AddFilm(second)};
  ASSERT_GT(idSecond, 0);

  TestScanner scanner{m_settings};
  EXPECT_EQ(VersionConversionResult::SUCCESS, scanner.GroupWithSimilar(idSecond).first);

  EXPECT_EQ(2, CountMovies());
  EXPECT_EQ(idFirst, m_db.GetMovieId(second));
  EXPECT_EQ(idCopy, m_db.GetMovieId(copy));
}

// Without a movie beside it, there is no telling which of several similar movies it belongs to
TEST_F(TestVideoInfoScanner, AMovieWithSeveralSimilarMoviesElsewhereStaysApart)
{
  GroupSimilarVideosAutomatically();
  const int idMovie{AddFilm(URIUtils::AddFileToFolder(m_root, "Film (2010)", "Film.mkv"))};
  ASSERT_GT(idMovie, 0);
  ASSERT_GT(AddFilm(URIUtils::AddFileToFolder(m_root, "Film Copy (2010)", "Film.mkv")), 0);
  ASSERT_GT(AddFilm(URIUtils::AddFileToFolder(m_root, "Film Other (2010)", "Film.mkv")), 0);

  TestScanner scanner{m_settings};
  EXPECT_EQ(VersionConversionResult::CANCELLED, scanner.GroupWithSimilar(idMovie).first);

  EXPECT_EQ(3, CountMovies());
}

// A stack plays the discs of a movie in turn, so isn't one of them, and stays apart as they become
// one movie
TEST_F(TestVideoInfoScanner, AStackOfTheDiscsOfAMovieStaysApartAsTheyBecomeOne)
{
  GroupSimilarVideosAutomatically();
  const std::string folder{URIUtils::AddFileToFolder(m_root, "Film (2010)")};
  const std::string first{URIUtils::AddFileToFolder(folder, "Disc 1", "Film.mkv")};
  const std::string second{URIUtils::AddFileToFolder(folder, "Disc 2", "Film.mkv")};
  const std::string stack{"stack://" + first + " , " + second};
  const int idFirst{AddFilm(first)};
  ASSERT_GT(idFirst, 0);
  const int idSecond{AddFilm(second)};
  ASSERT_GT(idSecond, 0);
  const int idStack{AddFilm(stack)};
  ASSERT_GT(idStack, 0);

  TestScanner scanner{m_settings};
  EXPECT_EQ(VersionConversionResult::SUCCESS, scanner.GroupWithSimilar(idFirst).first);

  EXPECT_EQ(2, CountMovies());
  EXPECT_EQ(idSecond, m_db.GetMovieId(first));
  EXPECT_EQ(idStack, m_db.GetMovieId(stack));
}

// The discs beside a movie are compared with each other as well as with it, as it may lack the
// kind of id they differ by
TEST_F(TestVideoInfoScanner, DiscsBesideAMovieWithConflictingIdsStayApart)
{
  GroupSimilarVideosAutomatically();
  const std::string folder{URIUtils::AddFileToFolder(m_root, "Film (2010)")};
  ASSERT_GT(AddFilm(URIUtils::AddFileToFolder(folder, "Disc 1", "Film.mkv"), "tt0000001"), 0);
  ASSERT_GT(AddFilm(URIUtils::AddFileToFolder(folder, "Disc 2", "Film.mkv"), "tt0000002"), 0);
  const int idThird{AddFilm(URIUtils::AddFileToFolder(folder, "Disc 3", "Film.mkv"))};
  ASSERT_GT(idThird, 0);

  TestScanner scanner{m_settings};
  EXPECT_EQ(VersionConversionResult::CANCELLED, scanner.GroupWithSimilar(idThird).first);

  EXPECT_EQ(3, CountMovies());
}

TEST_F(TestVideoInfoScanner, ADvdFolderRipIsOneMovie)
{
  const std::string folder{Folder(m_root, "Film (2010)")};
  Nfo(folder, "movie.nfo", "Film");
  const std::string videoTs{Folder(folder, "VIDEO_TS")};
  const std::string disc{File(videoTs, "VIDEO_TS.IFO")};
  File(videoTs, "VTS_01_0.IFO");
  File(videoTs, "VTS_01_1.VOB");

  Scan();

  EXPECT_EQ(1, CountMovies());
  EXPECT_GT(m_db.GetMovieId(disc), 0);
}

TEST_F(TestVideoInfoScanner, AFolderHoldingANomediaFileIsSkipped)
{
  const std::string folder{Folder(m_root, "Film (2010)")};
  FolderMovie(folder, "Film (2010).mkv");
  File(folder, ".nomedia");

  Scan();

  EXPECT_EQ(0, CountMovies());
}

TEST_F(TestVideoInfoScanner, AFolderWhoseNomediaFileIsDeletedIsScanned)
{
  NomediaFileDeleted(true);
}

TEST_F(TestVideoInfoScanner, AFolderWhoseNomediaFileIsDeletedIsScannedWithFastHashingOff)
{
  NomediaFileDeleted(false);
}

TEST_F(TestVideoInfoScanner, ATrailerIsTheMoviesTrailerNotAMovie)
{
  const std::string folder{Folder(m_root, "Film (2010)")};
  FolderMovie(folder, "Film (2010).mkv");
  const std::string trailer{File(folder, "Film (2010)-trailer.mkv")};

  Scan();

  EXPECT_EQ(1, CountMovies());
  EXPECT_EQ(trailer, m_db.GetSingleValue("SELECT c19 FROM movie"));
}

TEST_F(TestVideoInfoScanner, ASampleIsNotAMovie)
{
  const std::string folder{Folder(m_root, "Film (2010)")};
  FolderMovie(folder, "Film (2010).mkv");
  File(Folder(folder, "Sample"), "sample.mkv");

  Scan();

  EXPECT_EQ(1, CountMovies());
}

TEST_F(TestVideoInfoScanner, AMovieIsAddedToTheSetItsNfoNames)
{
  const std::string folder{Folder(m_root, "Film (2010)")};
  File(folder, "movie.nfo", "<movie><title>Film</title><set><name>Films</name></set></movie>");
  File(folder, "Film (2010).mkv");

  Scan();

  EXPECT_EQ(1, m_db.GetSingleValueInt("SELECT COUNT(*) FROM movie JOIN sets ON sets.idSet = "
                                      "movie.idSet WHERE sets.strSet = 'Films'"));
}

TEST_F(TestVideoInfoScanner, AnEditionInTheFileNameNamesTheMoviesVersion)
{
  const std::string folder{Folder(m_root, "Film (2010)")};
  const std::string movie{FolderMovie(folder, "Film (2010) [edition=Director's Cut].mkv")};

  Scan();

  EXPECT_EQ("Director's Cut", m_db.GetVideoVersionInfo(movie).m_assetTypeName);
}

// The identifier a movie is looked up by. Only a python scraper is given one, which the test
// scraper isn't, so the identifier is asked for directly.

namespace
{
class TestVideoInfoScannerMovieIdentifier : public TestVideoInfoScanner
{
protected:
  void SetUp() override
  {
    TestVideoInfoScanner::SetUp();
    m_metadataSources = AdvancedSettings()->m_videoScannerMetadataSources;
    AdvancedSettings()->m_videoScannerMetadataSources = {"tmdb", "imdb"};
  }

  void TearDown() override
  {
    AdvancedSettings()->m_videoScannerMetadataSources = m_metadataSources;
    TestVideoInfoScanner::TearDown();
  }

  static std::shared_ptr<CAdvancedSettings> AdvancedSettings()
  {
    return CServiceBroker::GetSettingsComponent()->GetAdvancedSettings();
  }

  bool GetMovieIdentifier(const std::string& path, bool bDirNames)
  {
    const TestScanner scanner{m_settings};
    return scanner.GetMovieIdentifier(CFileItem{path, false}, bDirNames,
                                      CFilenameAttributes{URIUtils::GetFileName(path), nullptr},
                                      m_identifierType, m_identifier);
  }

  std::string m_identifierType;
  std::string m_identifier;

private:
  std::unordered_set<std::string> m_metadataSources;
};
} // namespace

TEST_F(TestVideoInfoScannerMovieIdentifier, TheFolderNameGivesTheIdentifierWhenFolderNamesAreUsed)
{
  const std::string movie{URIUtils::AddFileToFolder(m_root, "Film (2010) {tmdb=603}", "ww.mkv")};

  ASSERT_TRUE(GetMovieIdentifier(movie, true));
  EXPECT_EQ("tmdb", m_identifierType);
  EXPECT_EQ("603", m_identifier);
}

TEST_F(TestVideoInfoScannerMovieIdentifier, TheFolderNameGivesNoIdentifierWhenFolderNamesAreNotUsed)
{
  const std::string movie{URIUtils::AddFileToFolder(m_root, "Film (2010) {tmdb=603}", "ww.mkv")};

  EXPECT_FALSE(GetMovieIdentifier(movie, false));
}

TEST_F(TestVideoInfoScannerMovieIdentifier, TheFileNameGivesTheIdentifierBeforeTheFolderName)
{
  const std::string movie{URIUtils::AddFileToFolder(m_root, "Film (2010) {tmdb=603}",
                                                    "Film (2010) {imdb=tt0133093}.mkv")};

  ASSERT_TRUE(GetMovieIdentifier(movie, true));
  EXPECT_EQ("imdb", m_identifierType);
  EXPECT_EQ("tt0133093", m_identifier);
}

// Movies looked up with a scraper

TEST_F(TestVideoInfoScanner, AMovieIsAddedFromTheScraper)
{
  UseTestScraper();
  Found("Film");
  Details("Film", "<details><title>Film from the scraper</title></details>");
  const std::string movie{File(m_root, "Film.mkv")};

  Scan();

  EXPECT_GT(m_db.GetMovieId(movie), 0);
  EXPECT_EQ("Film from the scraper", m_db.GetSingleValue("SELECT c00 FROM movie"));
}

// The movies of a folder are looked up alongside each other, and each is added with its own
// details, one whose search failed among them
TEST_F(TestVideoInfoScanner, TheMoviesOfAFolderAreEachAddedWithTheirDetails)
{
  UseTestScraper();
  std::vector<std::string> movies;
  constexpr int count{8};
  for (int movie = 1; movie <= count; ++movie)
  {
    const std::string name{"Film" + std::to_string(movie)};
    if (movie == 4)
      SearchFails(name);
    else
      FoundFromTheScraper(name);
    movies.emplace_back(File(m_root, name + ".mkv"));
  }

  Scan();

  EXPECT_EQ(count - 1, CountMovies());
  for (int movie = 1; movie <= count; ++movie)
  {
    const int idMovie{m_db.GetMovieId(movies[movie - 1])};
    if (movie == 4)
    {
      EXPECT_EQ(-1, idMovie);
      continue;
    }
    ASSERT_GT(idMovie, 0) << movie;
    EXPECT_EQ("Film" + std::to_string(movie) + " from the scraper", m_db.GetMovieTitle(idMovie));
  }
  EXPECT_FALSE(Record(m_root).starts_with("2|"));
}

TEST_F(TestVideoInfoScanner, ADiscIsLookedUpAlongsideOtherMovies)
{
  const TestScanner scanner{m_settings};

  EXPECT_TRUE(
      scanner.CanLookUpMovieAlongside(CFileItem{DvdRip(Folder(m_root, "Film (2010)")), false}));
  EXPECT_TRUE(scanner.CanLookUpMovieAlongside(
      CFileItem{URIUtils::AddFileToFolder(m_root, "Other (2011).iso"), false}));
}

TEST_F(TestVideoInfoScanner, TheDiscsOfFoldersBesideEachOtherAreEachAddedWithTheirDetails)
{
  UseTestScraper();
  std::vector<std::string> folders;
  std::vector<std::string> discs;
  constexpr int count{4};
  for (int movie = 1; movie <= count; ++movie)
  {
    const std::string name{"Film" + std::to_string(movie)};
    folders.emplace_back(Folder(m_root, name));
    if (movie == 2)
      SearchFails(name);
    else
      FoundFromTheScraper(name);
    discs.emplace_back(DvdRip(folders.back()));
  }

  Scan();

  EXPECT_EQ(count - 1, CountMovies());
  for (int movie = 1; movie <= count; ++movie)
  {
    const int idMovie{m_db.GetMovieId(discs[movie - 1])};
    if (movie == 2)
    {
      EXPECT_EQ(-1, idMovie);
      EXPECT_FALSE(Record(folders[movie - 1]).starts_with("2|"));
      continue;
    }
    ASSERT_GT(idMovie, 0) << movie;
    EXPECT_EQ("Film" + std::to_string(movie) + " from the scraper", m_db.GetMovieTitle(idMovie));
  }
}

// Folders beside each other are looked up in bounded batches, so that a first scan of a large
// source doesn't hold every folder before its first movie is added
TEST_F(TestVideoInfoScanner, TheMoviesOfMoreFoldersThanAreLookedUpTogetherAreAllAdded)
{
  constexpr int count{70};
  std::vector<std::string> folders;
  for (int movie = 1; movie <= count; ++movie)
  {
    const std::string name{"Film" + std::to_string(movie)};
    folders.emplace_back(Folder(m_root, name));
    FolderMovie(folders.back(), name + ".mkv");
  }

  Scan();

  EXPECT_EQ(count, CountMovies());
  for (int movie = 1; movie <= count; ++movie)
    EXPECT_TRUE(Record(folders[movie - 1]).starts_with("2|")) << movie;
}

// The movies of the folders beside each other are looked up alongside each other, each folder is
// recorded once its own movie is added, and a folder that holds another is scanned in its turn
TEST_F(TestVideoInfoScanner, TheMoviesOfFoldersBesideEachOtherAreEachAddedWithTheirDetails)
{
  UseTestScraper();
  std::vector<std::string> folders;
  std::vector<std::string> movies;
  constexpr int count{6};
  for (int movie = 1; movie <= count; ++movie)
  {
    const std::string name{"Film" + std::to_string(movie)};
    folders.emplace_back(Folder(m_root, name));
    if (movie == 3)
      SearchFails(name);
    else
      FoundFromTheScraper(name);
    movies.emplace_back(File(folders.back(), name + ".mkv"));
  }
  // A folder holding another folder of movies, scanned once those before it are
  const std::string collection{Folder(folders[3], "Collection")};
  Nfo(collection, "Inner.nfo", "Inner");
  const std::string inner{File(collection, "Inner.mkv")};
  // A movie folder with extras is still one holding no folder to scan
  const std::string trailer{File(Folder(folders[4], "Extras"), "Trailer.mkv")};

  Scan();

  EXPECT_EQ(count, CountMovies());
  EXPECT_GT(m_db.GetMovieId(inner), 0);
  for (int movie = 1; movie <= count; ++movie)
  {
    const int idMovie{m_db.GetMovieId(movies[movie - 1])};
    if (movie == 3)
    {
      EXPECT_EQ(-1, idMovie);
      EXPECT_FALSE(Record(folders[movie - 1]).starts_with("2|"));
      continue;
    }
    ASSERT_GT(idMovie, 0) << movie;
    EXPECT_EQ("Film" + std::to_string(movie) + " from the scraper", m_db.GetMovieTitle(idMovie));
    EXPECT_TRUE(Record(folders[movie - 1]).starts_with("2|")) << movie;
  }
  EXPECT_TRUE(IsExtraOf(trailer, m_db.GetMovieId(movies[4])));
}

// The movies of the folders a movie folder holds are looked up with those of the folders beside it,
// and added in their turn, so that the library is written to in the order of the folders
TEST_F(TestVideoInfoScanner, TheMoviesOfTheFoldersInAFolderAreEachAddedWithTheirDetails)
{
  UseTestScraper();
  const std::string collection{Folder(m_root, "Film2")};
  const std::vector<std::pair<std::string, std::string>> movies{
      {File(Folder(m_root, "Film1"), "Film1.mkv"), "Film1"},
      {File(Folder(collection, "Alpha"), "Alpha.mkv"), "Alpha"},
      {File(Folder(collection, "Beta"), "Beta.mkv"), "Beta"},
      {File(Folder(m_root, "Film3"), "Film3.mkv"), "Film3"}};
  for (const auto& [movie, name] : movies)
    FoundFromTheScraper(name);

  Scan();

  int previous{0};
  for (const auto& [movie, name] : movies)
  {
    const int idMovie{m_db.GetMovieId(movie)};
    ASSERT_GT(idMovie, previous) << movie;
    EXPECT_EQ(name + " from the scraper", m_db.GetMovieTitle(idMovie));
    previous = idMovie;
  }
}

// A folder planned ahead with the folder holding it isn't planned again in its turn, which would
// look the movies below it up twice: an update finds each folder of a tree unchanged once
TEST_F(TestVideoInfoScanner, AnUpdateFindsEachFolderOfATreeUnchangedOnce)
{
  const std::string collection{Folder(m_root, "Collection")};
  const std::string series{Folder(collection, "Series")};
  const std::string film{Folder(series, "Film (2010)")};
  FolderMovie(film, "Film (2010).mkv");
  // A folder to scan, so that the update lists the film's folder too
  const std::string other{Folder(film, "Other")};
  for (const std::string& folder : {other, film, series, collection})
    Settle(folder);
  Scan();

  const auto summary{Scan()};

  EXPECT_EQ(1, CountMovies());
  // The source, the collection, the series, the film's folder and the folder it holds
  EXPECT_EQ(5u, summary.unchangedDirectories);
}

TEST_F(TestVideoInfoScanner, TheProgressOfAScanEndsAtItsEnd)
{
  for (const std::string name : {"Film1", "Film2", "Film3"})
    FolderMovie(Folder(m_root, name), name + ".mkv");
  FolderMovie(Folder(Folder(m_root, "Collection"), "Film4"), "Film4.mkv");

  for (int scan = 0; scan < 2; ++scan)
  {
    TestScanner scanner{m_settings};
    scanner.Scan(m_root);
    EXPECT_FLOAT_EQ(100.f, scanner.Progress()) << scan;
  }
  EXPECT_EQ(4, CountMovies());
}

TEST_F(TestVideoInfoScanner, AFolderOfFoldersTakesNoShareOfTheProgress)
{
  const std::string collection{Folder(m_root, "Collection")};
  File(collection, "Film1 (2001).mkv");
  File(collection, "Film2 (2002).mkv");

  EXPECT_FLOAT_EQ(50.f, TestScanner{m_settings}.ProgressOnceListed(m_root, 0.5f));
}

TEST_F(TestVideoInfoScanner, AFailedSearchDoesNotStopTheScan)
{
  UseTestScraper();
  SearchFails("Afilm");
  File(m_root, "Afilm.mkv");
  Nfo(m_root, "Bfilm.nfo", "Bfilm");
  const std::string second{File(m_root, "Bfilm.mkv")};

  Scan();

  EXPECT_GT(m_db.GetMovieId(second), 0);
  EXPECT_EQ(1, CountMovies());
}

TEST_F(TestVideoInfoScanner, AnUnchangedFolderIsLookedUpAgainAfterASearchFailed)
{
  UseTestScraper();
  SearchFails("Afilm");
  const std::string first{File(m_root, "Afilm.mkv")};
  Nfo(m_root, "Bfilm.nfo", "Bfilm");
  File(m_root, "Bfilm.mkv");
  Scan();
  ASSERT_EQ(1, CountMovies());

  Found("Afilm");
  Details("Afilm", "<details><title>Afilm</title></details>");
  Scan();

  EXPECT_GT(m_db.GetMovieId(first), 0);
  EXPECT_EQ(2, CountMovies());
}

TEST_F(TestVideoInfoScanner, AnUnchangedFolderIsLookedUpAgainAfterDetailsFailed)
{
  UseTestScraper();
  DetailsFail("Afilm");
  const std::string first{File(m_root, "Afilm.mkv")};
  Nfo(m_root, "Bfilm.nfo", "Bfilm");
  File(m_root, "Bfilm.mkv");
  Scan();
  ASSERT_EQ(1, CountMovies());

  Details("Afilm", "<details><title>Afilm</title></details>");
  Scan();

  EXPECT_GT(m_db.GetMovieId(first), 0);
  EXPECT_EQ(2, CountMovies());
}

// A refresh removes a movie before adding it again, so one whose details can't be read is kept
TEST_F(TestVideoInfoScanner, ARefreshThatCantReadTheDetailsKeepsTheMovie)
{
  RefreshThatCantReadTheDetails(false);
}

// An override nfo gives some of the details, and the rest are still looked up
TEST_F(TestVideoInfoScanner, ARefreshWithAnOverrideNfoThatCantReadTheDetailsKeepsTheMovie)
{
  RefreshThatCantReadTheDetails(true);
}

// A refresh adds the movie again for the same files, so the next scan doesn't look at its folders
TEST_F(TestVideoInfoScanner, ARefreshKeepsTheHashesOfTheMoviesFolders)
{
  UseTestScraper();
  const std::string folder{Folder(m_root, "Afilm")};
  const std::string film{File(folder, "Afilm.mkv")};
  Found("Afilm");
  Details("Afilm", "<details><title>Afilm</title></details>");
  Scan();
  CVideoInfoTag tag;
  ASSERT_TRUE(m_db.GetMovieInfo({}, tag, m_db.GetMovieId(film)));
  const std::string root{Record(m_root)};
  ASSERT_FALSE(root.empty());

  ASSERT_TRUE(Refresh(tag));

  EXPECT_GT(m_db.GetMovieId(film), 0);
  EXPECT_EQ(root, Record(m_root));
}

// A refresh, which hasn't listed the movie's folder itself, lists it afresh to find the movie's
// subtitles, though the cache holds the listing the scan took. The movie has an nfo, which is found
// by its name: without one, a listing of the folder is taken to look for any nfo in it.
TEST_F(TestVideoInfoScanner, ARefreshFindsASubtitleAddedSinceTheScan)
{
  CServiceBroker::GetSettingsComponent()->GetSettings()->SetBool(
      CSettings::SETTING_MYVIDEOS_EXTRACTFLAGS, true);
  const std::string folder{Folder(m_root, "Afilm")};
  Nfo(folder, "movie.nfo", "Afilm");
  const std::string film{URIUtils::AddFileToFolder(folder, "Afilm.mkv")};
  ASSERT_TRUE(XFILE::CFile::Copy(
      XBMC_REF_FILE_PATH("xbmc/cores/VideoPlayer/test/testdata/h264_8bit_320x240.mkv"), film));
  Scan();
  CVideoInfoTag tag;
  ASSERT_TRUE(m_db.GetMovieInfo({}, tag, m_db.GetMovieId(film)));
  ASSERT_TRUE(m_db.GetStreamDetails(tag));
  ASSERT_EQ(0, tag.m_streamDetails.GetSubtitleStreamCount());
  // Added as another program adds it, unseen by the directory cache
  const std::string subtitle{URIUtils::AddFileToFolder(folder, "Afilm.en.srt")};
  {
    std::ofstream file{std::filesystem::path{
        std::u8string{reinterpret_cast<const char8_t*>(subtitle.data()), subtitle.size()}}};
    file << "1\n00:00:00,000 --> 00:00:01,000\nHello\n";
    ASSERT_TRUE(file.good());
  }

  ASSERT_TRUE(Refresh(tag));

  CVideoInfoTag refreshed;
  ASSERT_TRUE(m_db.GetMovieInfo({}, refreshed, m_db.GetMovieId(film)));
  ASSERT_TRUE(m_db.GetStreamDetails(refreshed));
  EXPECT_EQ(1, refreshed.m_streamDetails.GetSubtitleStreamCount());
}

// A refresh lists the movie's folder afresh to look for an nfo of any name in it, though the cache
// holds the listing the scan took without it
TEST_F(TestVideoInfoScanner, ARefreshFindsAnNfoAddedSinceTheScan)
{
  UseTestScraper();
  const std::string folder{Folder(m_root, "Afilm")};
  const std::string film{File(folder, "Afilm.mkv")};
  Found("Afilm");
  Details("Afilm", "<details><title>Afilm</title></details>");
  Scan();
  CVideoInfoTag tag;
  ASSERT_TRUE(m_db.GetMovieInfo({}, tag, m_db.GetMovieId(film)));
  // Added as another program adds it, unseen by the directory cache
  const std::string nfo{URIUtils::AddFileToFolder(folder, "info.nfo")};
  {
    std::ofstream file{std::filesystem::path{
        std::u8string{reinterpret_cast<const char8_t*>(nfo.data()), nfo.size()}}};
    file << "<movie><title>Other</title></movie>";
    ASSERT_TRUE(file.good());
  }

  ASSERT_TRUE(Refresh(tag));

  EXPECT_EQ("Other", m_db.GetMovieTitle(m_db.GetMovieId(film)));
}

// A refresh adds a disc of a movie again, which is titled with its disc, and gives it back the
// movie's other disc
TEST_F(TestVideoInfoScanner, ARefreshOfADiscGivesTheMovieBackItsOtherDisc)
{
  const Strings strings;
  ASSERT_TRUE(strings.loaded);
  UseTestScraper();
  GroupSimilarVideosAutomatically();
  FilmFound();
  const std::string folder{Folder(m_root, "Film (2010)")};
  const std::string first{File(Folder(folder, "Disc 1"), "Film.mkv")};
  Scan();
  const int idMovie{m_db.GetMovieId(first)};
  ASSERT_GT(idMovie, 0);
  const std::string second{File(Folder(folder, "Disc 2"), "Film.mkv")};
  Touch(folder);
  Scan();
  ASSERT_EQ(idMovie, m_db.GetMovieId(second));
  CVideoInfoTag tag;
  ASSERT_TRUE(m_db.GetMovieInfo({}, tag, idMovie));

  ASSERT_TRUE(Refresh(tag));

  EXPECT_EQ(1, CountMovies());
  const int idRefreshed{m_db.GetMovieId(first)};
  ASSERT_GT(idRefreshed, 0);
  EXPECT_EQ(idRefreshed, m_db.GetMovieId(second));
  EXPECT_EQ("Film", m_db.GetMovieTitle(idRefreshed));
}

// The movie refreshed is as it was listed, as by the information dialog, which a scan may have
// added extras to since. The database is asked which assets it has.
TEST_F(TestVideoInfoScanner, ARefreshKeepsTheExtrasAddedSinceTheMovieWasListed)
{
  UseTestScraper();
  const std::string folder{Folder(m_root, "Film (2010)")};
  const std::string movie{FolderMovie(folder, "Film (2010).mkv")};
  Scan();
  const int idMovie{m_db.GetMovieId(movie)};
  ASSERT_GT(idMovie, 0);
  CVideoInfoTag listed;
  ASSERT_TRUE(m_db.GetMovieInfo({}, listed, idMovie));
  const std::string trailer{File(Folder(folder, "Extras"), "Trailer.mkv")};
  Touch(folder);
  Scan();
  ASSERT_TRUE(IsExtraOf(trailer, idMovie));

  ASSERT_TRUE(Refresh(listed));

  const int idRefreshed{m_db.GetMovieId(movie)};
  ASSERT_GT(idRefreshed, 0);
  EXPECT_TRUE(IsExtraOf(trailer, idRefreshed));
  EXPECT_EQ(1, CountExtras());
}

// The refresh of a disc added as a movie of its own merges it into the movie of the lowest disc,
// where it becomes the default version. That movie is still refreshed from its own disc.
TEST_F(TestVideoInfoScanner, ARefreshOfASourceRefreshesEachDiscOfAMovie)
{
  UseTestScraper();
  GroupSimilarVideosAutomatically();
  FilmFound();
  const std::string folder{Folder(m_root, "Film (2010)")};
  const std::string first{File(Folder(folder, "Disc 1"), "Film.mkv")};
  const std::string second{File(Folder(folder, "Disc 2"), "Film.mkv")};
  // listed in the order they are added
  ASSERT_GT(AddFilm(second), 0);
  ASSERT_GT(AddFilm(first), 0);

  TestRefreshingSourceJob refresh{m_root};
  ASSERT_TRUE(refresh.Work(m_db));

  EXPECT_EQ(1, CountMovies());
  const int idMovie{m_db.GetMovieId(first)};
  EXPECT_EQ(idMovie, m_db.GetMovieId(second));
  CVideoInfoTag tag;
  ASSERT_TRUE(m_db.GetMovieInfo({}, tag, idMovie));
  EXPECT_EQ(first, tag.m_strFileNameAndPath);
}

// Merging the discs of a film as an earlier disc is refreshed makes the movie of a later one a
// version, which is still refreshed from its own disc. The disc refreshed last is the default.
TEST_F(TestVideoInfoScanner, ARefreshOfASourceRefreshesADiscMergedBeforeItsTurn)
{
  const Strings strings;
  ASSERT_TRUE(strings.loaded);
  UseTestScraper();
  GroupSimilarVideosAutomatically();
  FilmFound();
  const std::string folder{Folder(m_root, "Film (2010)")};
  const std::string first{File(Folder(folder, "Disc 1"), "Film.mkv")};
  const std::string second{File(Folder(folder, "Disc 2"), "Film.mkv")};
  const std::string third{File(Folder(folder, "Disc 3"), "Film.mkv")};
  // listed in the order they are added: refreshing Disc 2 merges Disc 3 into Disc 1 first
  ASSERT_GT(AddFilm(second), 0);
  ASSERT_GT(AddFilm(first), 0);
  ASSERT_GT(AddFilm(third), 0);
  // only a refresh of Disc 3 reads this
  File(URIUtils::GetDirectory(third), "Film.nfo",
       R"(<movie override="true"><title>Third refreshed</title></movie>)");

  TestRefreshingSourceJob refresh{m_root};
  ASSERT_TRUE(refresh.Work(m_db));

  EXPECT_EQ(1, CountMovies());
  const int idMovie{m_db.GetMovieId(first)};
  EXPECT_EQ(idMovie, m_db.GetMovieId(second));
  EXPECT_EQ(idMovie, m_db.GetMovieId(third));
  EXPECT_EQ("Third refreshed", m_db.GetMovieTitle(idMovie));
  CVideoInfoTag tag;
  ASSERT_TRUE(m_db.GetMovieInfo({}, tag, idMovie));
  EXPECT_EQ(third, tag.m_strFileNameAndPath);
}

// An nfo naming the movie's url gives none of its details, which the refresh reads before it
// removes the movie
TEST_F(TestVideoInfoScanner, ARefreshWithAUrlNfoThatCantReadTheDetailsKeepsTheMovie)
{
  UseTestScraper();
  InstallOtherScraper();
  ASSERT_TRUE(m_db.ExecuteQuery(m_db.PrepareSQL("UPDATE path SET strScraper='%s' WHERE idPath=%i",
                                                OTHER_SCRAPER_ID, m_db.GetPathId(m_root))));
  const std::string folder{Folder(m_root, "Afilm")};
  const std::string film{File(folder, "Afilm.mkv")};
  // A url that the path's scraper recognises, but whose details it can't fetch
  File(folder, "Afilm.nfo", "http://other.invalid/1");
  CVideoInfoTag tag;
  tag.m_strTitle = "Afilm";
  tag.m_strFileNameAndPath = film;
  tag.m_strPath = folder;
  const int idMovie{m_db.SetDetailsForMovie(tag, KODI::ART::Artwork{})};
  ASSERT_GT(idMovie, 0);

  ASSERT_TRUE(m_db.GetMovieInfo({}, tag, idMovie));
  TestRefreshingJob refresh{std::make_shared<CFileItem>(tag), true, false};
  refresh.Work(m_db);

  EXPECT_EQ(idMovie, m_db.GetMovieId(film));
}

// A url is only taken from the scraper chosen for the path, which is the one it is fetched with
TEST_F(TestVideoInfoScanner, AMovieIsLookedUpWithItsPathsScraperWhenItsNfoNamesAnother)
{
  UseTestScraper();
  InstallOtherScraper();
  Found("Film");
  Details("Film", "<details><title>Film from the path's scraper</title></details>");
  // A url that only the other scraper recognises
  File(m_root, "Film.nfo", "http://other.invalid/1");
  const std::string movie{File(m_root, "Film.mkv")};

  Scan();

  EXPECT_GT(m_db.GetMovieId(movie), 0);
  EXPECT_EQ("Film from the path's scraper", m_db.GetSingleValue("SELECT c00 FROM movie"));
}

// The nfo gives the set no overview, so its movie's path is asked for a scraper to look it up with
TEST_F(TestVideoInfoScanner, ARefreshOfASetPassesOverAMovieWithAScraperKodiNoLongerHas)
{
  const std::string folder{Folder(m_root, "Film (2010)")};
  File(folder, "movie.nfo", "<movie><title>Film</title><set><name>Films</name></set></movie>");
  File(folder, "Film (2010).mkv");
  Scan();
  ASSERT_TRUE(m_db.ExecuteQuery(
      m_db.PrepareSQL("UPDATE path SET strScraper='metadata.gone.videoinfoscanner' WHERE idPath=%i",
                      m_db.GetPathId(m_root))));
  const auto set{std::make_shared<CFileItem>()};
  CVideoInfoTag tag;
  ASSERT_TRUE(m_db.GetSetInfo(m_db.GetSingleValueInt("SELECT idSet FROM sets"), tag, set.get()));

  TestRefreshingJob refresh{set, true, false};
  EXPECT_TRUE(refresh.Work(m_db));
}

// Movie extras

TEST_F(TestVideoInfoScanner, AMovieFolderGetsItsExtras)
{
  const std::string folder{Folder(m_root, "Film (2010)")};
  const std::string movie{FolderMovie(folder, "Film (2010).mkv")};
  const std::string trailer{File(Folder(folder, "Extras"), "Trailer.mkv")};

  Scan();

  const int idMovie{m_db.GetMovieId(movie)};
  ASSERT_GT(idMovie, 0);
  EXPECT_TRUE(IsExtraOf(trailer, idMovie));

  // An update with nothing changed adds and removes nothing
  Scan();
  EXPECT_EQ(1, CountMovies());
  EXPECT_EQ(1, CountExtras());
}

// The extras folder is recorded as added with the movie, so the next scan doesn't add it again
TEST_F(TestVideoInfoScanner, AnExtrasFolderAddedWithItsMovieIsHashed)
{
  const std::string folder{Folder(m_root, "Film (2010)")};
  FolderMovie(folder, "Film (2010).mkv");
  const std::string extras{Folder(folder, "Extras")};
  File(extras, "Trailer.mkv");

  Scan();

  EXPECT_FALSE(Record(extras).empty());
}

TEST_F(TestVideoInfoScanner, AVideoAddedBesideTheExtrasOfABonusDiscIsAdded)
{
  VideoAddedBesideABonusDisc(false);
}

TEST_F(TestVideoInfoScanner, AVideoAddedBesideTheExtrasOfABonusDiscRipIsAdded)
{
  VideoAddedBesideABonusDisc(true);
}

// The extras folder belongs to no movie where its folder holds several
TEST_F(TestVideoInfoScanner, ExtrasBesideTwoMoviesGoToNeither)
{
  Nfo(m_root, "First.nfo", "First");
  const std::string first{File(m_root, "First.mkv")};
  Nfo(m_root, "Second.nfo", "Second");
  const std::string second{File(m_root, "Second.mkv")};
  const std::string trailer{File(Folder(m_root, "Extras"), "Trailer.mkv")};

  Scan();

  EXPECT_GT(m_db.GetMovieId(first), 0);
  EXPECT_GT(m_db.GetMovieId(second), 0);
  EXPECT_EQ(-1, m_db.GetVideoVersionInfo(trailer).m_assetTypeId);
  EXPECT_EQ(-1, m_db.GetMovieId(trailer));
  EXPECT_EQ(0, CountExtras());
}

TEST_F(TestVideoInfoScanner, TheExtrasOfAStackGoToItsMovie)
{
  const std::string folder{Folder(m_root, "Film (2010)")};
  FolderMovie(folder, "Film (2010) cd1.mkv");
  File(folder, "Film (2010) cd2.mkv");
  const std::string trailer{File(Folder(folder, "Extras"), "Trailer.mkv")};

  Scan();

  ASSERT_EQ(1, CountMovies());
  const int idMovie{m_db.GetSingleValueInt("SELECT idMovie FROM movie")};
  EXPECT_TRUE(IsExtraOf(trailer, idMovie));
}

// The movie is found once the folders beside the extras folder have been scanned
TEST_F(TestVideoInfoScanner, ExtrasBesideTheFolderOfTheirMovieGoToIt)
{
  const std::string folder{Folder(m_root, "Film (2010)")};
  const std::string movie{FolderMovie(Folder(folder, "Disc 1"), "Film (2010).mkv")};
  const std::string trailer{File(Folder(folder, "Extras"), "Trailer.mkv")};

  Scan();

  const int idMovie{m_db.GetMovieId(movie)};
  ASSERT_GT(idMovie, 0);
  EXPECT_TRUE(IsExtraOf(trailer, idMovie));
}

// A video in an extras folder is an extra, even with a full nfo of its own
TEST_F(TestVideoInfoScanner, AnExtrasFolderIsNotScannedForMovies)
{
  const std::string folder{Folder(m_root, "Film (2010)")};
  const std::string movie{FolderMovie(folder, "Film (2010).mkv")};
  const std::string extras{Folder(folder, "Extras")};
  Nfo(extras, "Making Of.nfo", "Making Of");
  const std::string makingOf{File(extras, "Making Of.mkv")};

  Scan();

  EXPECT_EQ(1, CountMovies());
  EXPECT_TRUE(IsExtraOf(makingOf, m_db.GetMovieId(movie)));
}

TEST_F(TestVideoInfoScanner, IgnoredExtrasAreNeitherExtrasNorMovies)
{
  const std::string folder{Folder(m_root, "Film (2010)")};
  FolderMovie(folder, "Film (2010).mkv");
  const std::string extras{Folder(folder, "Extras")};
  Nfo(extras, "Making Of.nfo", "Making Of");
  File(extras, "Making Of.mkv");

  Scan(true);

  EXPECT_EQ(1, CountMovies());
  EXPECT_EQ(0, CountExtras());
}

TEST_F(TestVideoInfoScanner, AVideoAddedToTheExtrasOfAnUnchangedFolderIsAdded)
{
  const std::string folder{Folder(m_root, "Film (2010)")};
  const std::string movie{FolderMovie(folder, "Film (2010).mkv")};
  const std::string extras{Folder(folder, "Extras")};
  File(extras, "Trailer.mkv");
  Scan();
  ASSERT_EQ(1, CountExtras());

  const std::string interview{File(extras, "Interview.mkv")};
  Touch(extras);
  Scan();

  EXPECT_TRUE(IsExtraOf(interview, m_db.GetMovieId(movie)));
  EXPECT_EQ(2, CountExtras());
}

TEST_F(TestVideoInfoScanner, AnExtraDeletedIsCleanedOnUpdate)
{
  const std::string folder{Folder(m_root, "Film (2010)")};
  FolderMovie(folder, "Film (2010).mkv");
  const std::string extras{Folder(folder, "Extras")};
  const std::string trailer{File(extras, "Trailer.mkv")};
  const std::string interview{File(extras, "Interview.mkv")};
  Scan();
  ASSERT_EQ(2, CountExtras());

  ASSERT_TRUE(XFILE::CFile::Delete(interview));
  Touch(extras);
  Scan(false, true);

  EXPECT_EQ(1, CountExtras());
  EXPECT_EQ(-1, m_db.GetVideoVersionInfo(interview).m_assetTypeId);
}

// A scan of the movie's own folder takes its extras folder out of the scan, so queues it for the
// clean
TEST_F(TestVideoInfoScanner, AnExtraDeletedIsCleanedByAScanOfItsMovieFolder)
{
  const std::string folder{Folder(m_root, "Film (2010)")};
  FolderMovie(folder, "Film (2010).mkv");
  const std::string extras{Folder(folder, "Extras")};
  File(extras, "Trailer.mkv");
  const std::string interview{File(extras, "Interview.mkv")};
  Scan();
  ASSERT_EQ(2, CountExtras());

  ASSERT_TRUE(XFILE::CFile::Delete(interview));
  Touch(extras);
  Scan(false, true, folder);

  EXPECT_EQ(1, CountExtras());
}

TEST_F(TestVideoInfoScanner, ExtrasAreAddedWithFastHashingOff)
{
  CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_bVideoLibraryUseFastHash = false;
  const std::string folder{Folder(m_root, "Film (2010)")};
  const std::string movie{FolderMovie(folder, "Film (2010).mkv")};
  const std::string extras{Folder(folder, "Extras")};
  const std::string trailer{File(extras, "Trailer.mkv")};

  Scan();
  const int idMovie{m_db.GetMovieId(movie)};
  ASSERT_GT(idMovie, 0);
  EXPECT_TRUE(IsExtraOf(trailer, idMovie));

  const std::string interview{File(extras, "Interview.mkv")};
  Touch(extras);
  Scan();
  EXPECT_TRUE(IsExtraOf(interview, idMovie));
  EXPECT_EQ(1, CountMovies());
}

TEST_F(TestVideoInfoScanner, AVideoAddedOrDeletedBelowTheExtrasOfAnUnchangedFolderIsSeen)
{
  ScanChangesBelowTheExtras();
}

TEST_F(TestVideoInfoScanner, AVideoAddedOrDeletedBelowTheExtrasIsSeenWithFastHashingOff)
{
  CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_bVideoLibraryUseFastHash = false;
  ScanChangesBelowTheExtras();
}

// A folder of the extras that a scan for movies leaves out still holds extras
TEST_F(TestVideoInfoScanner, AVideoAddedOrDeletedInAnExcludedFolderOfTheExtrasIsSeen)
{
  CServiceBroker::GetSettingsComponent()
      ->GetAdvancedSettings()
      ->m_moviesExcludeFromScanRegExps.emplace_back("[/\\\\]Featurettes[/\\\\]");
  ScanChangesBelowTheExtras();
}

// An extras folder a scan for movies leaves out still holds extras, so a video added to it is seen
TEST_F(TestVideoInfoScanner, AVideoAddedToAnExcludedExtrasFolderIsSeen)
{
  CServiceBroker::GetSettingsComponent()
      ->GetAdvancedSettings()
      ->m_moviesExcludeFromScanRegExps.emplace_back("[/\\\\]Extras[/\\\\]");
  const std::string folder{Folder(m_root, "Film (2010)")};
  FolderMovie(folder, "Film (2010).mkv");
  const std::string extras{Folder(folder, "Extras")};
  File(extras, "Trailer.mkv");
  Settle(folder);
  Scan();
  ASSERT_EQ(1, CountExtras());

  const std::string interview{File(extras, "Interview.mkv")};
  Touch(extras);
  Scan();
  EXPECT_TRUE(IsExtraOf(interview, m_db.GetSingleValueInt("SELECT idMovie FROM movie")));
}

#ifdef HAVE_LIBBLURAY
TEST_F(TestVideoInfoScanner, AnExcludedExtrasFolderBesideADiscGoesToItsMovie)
{
  CServiceBroker::GetSettingsComponent()
      ->GetAdvancedSettings()
      ->m_moviesExcludeFromScanRegExps.emplace_back("[/\\\\]Extras[/\\\\]");
  const std::string folder{Folder(m_root, "Film (2010)")};
  Nfo(folder, "movie.nfo", "Film");
  BlurayRip(folder);
  const std::string trailer{File(Folder(folder, "Extras"), "Trailer.mkv")};

  Scan();

  EXPECT_TRUE(IsExtraOf(trailer, m_db.GetSingleValueInt("SELECT idMovie FROM movie")));
}

// A folder holding a disc structure is listed as the disc's file, so its extras folders are found
// beside the disc
TEST_F(TestVideoInfoScanner, ExtrasBesideADiscGoToItsMovie)
{
  const std::string folder{Folder(m_root, "Film (2010)")};
  Nfo(folder, "movie.nfo", "Film");
  BlurayRip(folder);
  const std::string extras{Folder(folder, "Extras")};
  const std::string trailer{File(extras, "Trailer.mkv")};

  Scan();

  ASSERT_EQ(1, CountMovies());
  const int idMovie{m_db.GetSingleValueInt("SELECT idMovie FROM movie")};
  EXPECT_TRUE(IsExtraOf(trailer, idMovie));

  // Nor does adding one change the disc's folder
  const std::string interview{File(extras, "Interview.mkv")};
  Touch(extras);
  Scan();
  EXPECT_TRUE(IsExtraOf(interview, idMovie));
  EXPECT_EQ(1, CountMovies());
}

TEST_F(TestVideoInfoScanner, AVideoAddedOrDeletedBelowTheExtrasBesideADiscIsSeen)
{
  ScanChangesBelowTheExtras(true);
}

// Its playlists and streams are not extras of their own, nor is the disc a movie
TEST_F(TestVideoInfoScanner, ADiscInAnExtrasFolderIsOneExtra)
{
  const std::string folder{Folder(m_root, "Film (2010)")};
  const std::string movie{FolderMovie(folder, "Film (2010).mkv")};
  const std::string disc{BlurayRip(Folder(folder, "Extras"))};

  Scan();

  EXPECT_EQ(1, CountMovies());
  EXPECT_TRUE(IsExtraOf(disc, m_db.GetMovieId(movie)));
  EXPECT_EQ(1, CountExtras());
}
#endif

// A folder with content set on it is a source of its own, whatever its name
TEST_F(TestVideoInfoScanner, AnExtrasFolderThatIsASourceIsScannedForMovies)
{
  const std::string folder{Folder(m_root, "Film (2010)")};
  FolderMovie(folder, "Film (2010).mkv");
  const std::string extras{Folder(folder, "Extras")};
  Nfo(extras, "Other.nfo", "Other");
  const std::string other{File(extras, "Other.mkv")};
  ASSERT_TRUE(m_db.ExecuteQuery(m_db.PrepareSQL(
      "UPDATE path SET strContent='movies', strScraper='metadata.local', useFolderNames=0, "
      "scanRecursive=0, noUpdate=0 WHERE idPath=%i",
      m_db.AddPath(extras))));

  Scan();

  EXPECT_GT(m_db.GetMovieId(other), 0);
  EXPECT_EQ(2, CountMovies());
  EXPECT_EQ(0, CountExtras());
}

// A movie restored from its export has only the extras the export records. The others beside them
// were there before, so a later scan doesn't add them either.
TEST_F(TestVideoInfoScanner, AnExtraAnExportLeftOutIsNotAddedByTheNextScan)
{
  const std::string folder{Folder(m_root, "Film (2010)")};
  File(folder, "movie.nfo",
       "<movie><title>Film (2010)</title><videoassettype>1</videoassettype></movie>");
  const std::string movie{File(folder, "Film (2010).mkv")};
  const std::string extras{Folder(folder, "Extras")};
  const std::string trailer{File(extras, "Trailer.mkv")};
  File(extras, "Trailer.nfo",
       "<movie><title>Film (2010)</title><videoassettitle>Trailer</videoassettitle>"
       "<videoassettype>2</videoassettype></movie>");
  const std::string featurette{File(extras, "Featurette.mkv")};
  Scan();
  const int idMovie{m_db.GetMovieId(movie)};
  ASSERT_GT(idMovie, 0);
  ASSERT_TRUE(IsExtraOf(trailer, idMovie));
  ASSERT_FALSE(IsExtraOf(featurette, idMovie));

  Scan();

  EXPECT_FALSE(IsExtraOf(featurette, idMovie));
  EXPECT_EQ(1, CountExtras());
}

// An exported nfo names the movie's asset type. A scan restoring the movie from it takes only
// the extras its nfos record; a refresh reads the discs and extras folders as well
TEST_F(TestVideoInfoScanner, ARefreshOfAMovieWithAnExportedNfoAddsItsExtras)
{
  const std::string folder{Folder(m_root, "Film (2010)")};
  File(folder, "movie.nfo",
       "<movie><title>Film (2010)</title><videoassettype>1</videoassettype></movie>");
  const std::string movie{File(folder, "Film (2010).mkv")};
  const std::string trailer{File(Folder(folder, "Extras"), "Trailer.mkv")};

  Scan();
  const int idMovie{m_db.GetMovieId(movie)};
  ASSERT_GT(idMovie, 0);
  EXPECT_EQ(0, CountExtras());

  // As the refresh job does: the movie is deleted, read again and then given its extras
  ASSERT_TRUE(m_db.DeleteMovie(idMovie, DeleteMovieCascadeAction::DEFAULT_VERSION));
  TestScanner refresh{m_settings};
  refresh.DeferMovieExtras();
  CFileItemList items;
  items.SetPath(folder);
  items.Add(std::make_shared<CFileItem>(movie, false));
  ASSERT_TRUE(refresh.RetrieveVideoInfo(items, true, ADDON::ContentType::MOVIES));
  const int idRefreshed{m_db.GetMovieId(movie)};
  ASSERT_GT(idRefreshed, 0);
  refresh.AddMovieExtras(idRefreshed, true);

  EXPECT_TRUE(IsExtraOf(trailer, idRefreshed));
}

TEST_F(TestVideoInfoScanner, AFolderIsNamedAfterTheSourceHoldingIt)
{
  std::vector<CMediaSource>& sources{
      CMediaSourceSettings::GetInstance().GetSources(KODI::MEDIA::MediaSection::VIDEO)};
  const auto source{std::ranges::find(sources, m_root, &CMediaSource::strPath)};
  ASSERT_NE(sources.end(), source);
  source->strName = "Test Films";

  EXPECT_EQ("Test Films", TestScanner::SourceName(m_root));
  EXPECT_EQ("Test Films", TestScanner::SourceName(URIUtils::AddFileToFolder(m_root, "Film/")));
  // A folder of no source is named after itself
  EXPECT_EQ("Elsewhere", TestScanner::SourceName("smb://host/Elsewhere/"));
}

// Showing hidden files changes which are scanned, though not the listings they are in
TEST_F(TestVideoInfoScanner, ShowingHiddenFilesIsPartOfAFoldersSettings)
{
  const auto settings{CServiceBroker::GetSettingsComponent()->GetSettings()};
  const bool showHidden{settings->GetBool(CSettings::SETTING_FILELISTS_SHOWHIDDEN)};
  const std::string hidden{TestScanner{m_settings}.FolderSettings(m_root)};
  settings->SetBool(CSettings::SETTING_FILELISTS_SHOWHIDDEN, !showHidden);
  const std::string shown{TestScanner{m_settings}.FolderSettings(m_root)};
  settings->SetBool(CSettings::SETTING_FILELISTS_SHOWHIDDEN, showHidden);

  EXPECT_FALSE(hidden.empty());
  EXPECT_NE(hidden, shown);
}

#ifdef TARGET_WINDOWS
// A file that stops being hidden, its size and time unchanged, changes the listing of its folder,
// which an update without the fast hash then looks at again
TEST_F(TestVideoInfoScanner, AMovieThatStopsBeingHiddenIsAddedOnUpdate)
{
  CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_bVideoLibraryUseFastHash = false;
  Nfo(m_root, "First.nfo", "First");
  File(m_root, "First.mkv");
  Nfo(m_root, "Second.nfo", "Second");
  const std::string movie{File(m_root, "Second.mkv")};
  const std::filesystem::path path{
      std::u8string{reinterpret_cast<const char8_t*>(movie.data()), movie.size()}};
  ASSERT_TRUE(SetFileAttributesW(path.c_str(), FILE_ATTRIBUTE_HIDDEN));

  const auto settings{CServiceBroker::GetSettingsComponent()->GetSettings()};
  const bool showHidden{settings->GetBool(CSettings::SETTING_FILELISTS_SHOWHIDDEN)};
  settings->SetBool(CSettings::SETTING_FILELISTS_SHOWHIDDEN, false);
  Scan();
  EXPECT_EQ(-1, m_db.GetMovieId(movie));
  EXPECT_TRUE(SetFileAttributesW(path.c_str(), FILE_ATTRIBUTE_NORMAL));
  Scan();
  settings->SetBool(CSettings::SETTING_FILELISTS_SHOWHIDDEN, showHidden);

  EXPECT_GT(m_db.GetMovieId(movie), 0);
}
#endif

// A folder whose folders, and those below them, make more than are looked up together has only
// as many planned ahead; the others are scanned in their turn
TEST_F(TestVideoInfoScanner, AllTheMoviesOfATreeOfMoreFoldersThanAreLookedUpTogetherAreAdded)
{
  const std::string collection{Folder(m_root, "Collection")};
  std::vector<std::string> movies;
  for (int series = 1; series <= 8; ++series)
  {
    const std::string folder{Folder(collection, StringUtils::Format("Series {}", series))};
    for (int film = 1; film <= 8; ++film)
    {
      const std::string title{StringUtils::Format("Film {} {} (2010)", series, film)};
      movies.push_back(FolderMovie(Folder(folder, title), title + ".mkv"));
    }
  }

  // 8 series and 56 of their movies
  EXPECT_EQ(1U + 64U, TestScanner{m_settings}.FoldersPlannedWith(collection, 64));

  Scan();

  EXPECT_EQ(64, CountMovies());
  for (const std::string& movie : movies)
    EXPECT_GT(m_db.GetMovieId(movie), 0) << movie;
}

// A folder that isn't scanned alongside those beside it, here one of music videos, is added once the
// movies of every batch of folders before it are, though the next batch is looked up before the one
// before it is added
TEST_F(TestVideoInfoScanner, AFolderScannedOnItsOwnWaitsForEveryBatchBeforeIt)
{
  for (const std::string name :
       {"A Film (2001)", "B Film (2002)", "C Film (2003)", "D Film (2004)", "E Film (2005)"})
    FolderMovie(Folder(m_root, name), name + ".mkv");
  const std::string videos{Folder(m_root, "F Videos")};
  File(videos, "Video.nfo", "<musicvideo><title>Video</title><artist>Artist</artist></musicvideo>");
  File(videos, "Video.mkv");
  ASSERT_TRUE(m_db.ExecuteQuery(m_db.PrepareSQL(
      "UPDATE path SET strContent='musicvideos', strScraper='metadata.local', useFolderNames=0, "
      "scanRecursive=0, noUpdate=0 WHERE idPath=%i",
      m_db.AddPath(videos))));
  FolderMovie(Folder(m_root, "G Film (2007)"), "G Film (2007).mkv");

  TestScanner scanner{m_settings};
  scanner.LimitFoldersLookedUpTogether(2);
  scanner.Scan(m_root);

  std::vector<std::string> added;
  for (const TestScanner::ScanSummary::Item& item : scanner.Summary().added)
    added.push_back(URIUtils::GetFileName(item.path));
  EXPECT_EQ((std::vector<std::string>{"A Film (2001).mkv", "B Film (2002).mkv", "C Film (2003).mkv",
                                      "D Film (2004).mkv", "E Film (2005).mkv", "Video.mkv",
                                      "G Film (2007).mkv"}),
            added);
}

// However many folders are looked up together, the library is written in the same order, so is the
// same. The source holds a disc, extras, a folder whose few folders are planned ahead, one of them
// with .nomedia, and one whose many are scanned in their turn.
TEST_F(TestVideoInfoScanner, TheLibraryIsTheSameHoweverManyFoldersAreLookedUpTogether)
{
  for (const std::string name : {"A Film (2001)", "B Film (2002)", "D Film (2004)", "F Film (2006)",
                                 "G Film (2007)", "H Film (2008)"})
    FolderMovie(Folder(m_root, name), name + ".mkv");
  File(Folder(URIUtils::AddFileToFolder(m_root, "B Film (2002)"), "Extras"), "Trailer.mkv");
  const std::string disc{Folder(m_root, "C Film (2003)")};
  Nfo(disc, "movie.nfo", "C Film");
  DvdRip(disc);
  const std::string few{Folder(m_root, "E Collection")};
  for (const std::string name : {"Part 1 (2001)", "Part 2 (2002)"})
    FolderMovie(Folder(few, name), name + ".mkv");
  File(Folder(few, "Hidden (2005)"), ".nomedia");
  const std::string many{Folder(m_root, "I Many")};
  for (int film = 1; film <= 9; ++film)
  {
    const std::string name{StringUtils::Format("Many {} (2010)", film)};
    FolderMovie(Folder(many, name), name + ".mkv");
  }
  Nfo(m_root, "Root Film (2009).nfo", "Root Film");
  File(m_root, "Root Film (2009).mkv");

  // A database of its own for each scan, as SetUp() makes the test's
  const auto scanned{[this](size_t together)
                     {
                       DatabaseSettings settings{m_settings};
                       settings.name += "Together" + std::to_string(together);
                       TestDatabase db;
                       EXPECT_EQ(CDatabase::ConnectionState::STATE_CONNECTED,
                                 db.Connect(settings.name, settings, true));
                       EXPECT_TRUE(db.ExecuteQuery(db.PrepareSQL(
                           "UPDATE path SET strContent='movies', strScraper='metadata.local', "
                           "useFolderNames=1, scanRecursive=2147483647, noUpdate=0 WHERE idPath=%i",
                           db.AddPath(m_root))));
                       std::vector<std::string> library;
                       {
                         TestScanner scanner{settings};
                         scanner.LimitFoldersLookedUpTogether(together);
                         scanner.Scan(m_root);
                         for (const TestScanner::ScanSummary::Item& item : scanner.Summary().added)
                           library.push_back(item.path + "|" + item.title);
                       }
                       // The movies and extras in the order written, and the folders' records
                       library.push_back(db.GetSingleValue(
                           "SELECT GROUP_CONCAT(entry, char(10)) FROM (SELECT movie.c00 || '|' || "
                           "path.strPath || files.strFilename AS entry FROM movie JOIN files ON "
                           "files.idFile=movie.idFile JOIN path ON path.idPath=files.idPath ORDER "
                           "BY movie.idMovie)"));
                       library.push_back(db.GetSingleValue(
                           "SELECT GROUP_CONCAT(entry, char(10)) FROM (SELECT path.strPath || "
                           "files.strFilename || '|' || videoversion.idMedia || '|' || "
                           "videoversion.itemType AS entry FROM videoversion JOIN files ON "
                           "files.idFile=videoversion.idFile JOIN path ON "
                           "path.idPath=files.idPath ORDER BY videoversion.idFile)"));
                       library.push_back(db.GetSingleValue(
                           "SELECT GROUP_CONCAT(entry, char(10)) FROM (SELECT strPath || '|' || "
                           "IFNULL(strHash, '') AS entry FROM path ORDER BY strPath)"));
                       db.Close();
                       XFILE::CFile::Delete(settings.host + settings.name + ".db");
                       return library;
                     }};

  const std::vector<std::string> together{scanned(64)};
  // Every movie, and the extra
  ASSERT_GE(together.size(), 3U);
  EXPECT_EQ(19, std::ranges::count(together[together.size() - 3], '\n') + 1);
  EXPECT_FALSE(together[together.size() - 2].empty());
  EXPECT_EQ(together, scanned(2));
  EXPECT_EQ(together, scanned(1));
}

// A scraper call waits while another is being made
TEST(TestVideoInfoScannerScraperCalls, ACallWaitsForAPlace)
{
  TestScanner::CScraperCalls calls;
  std::optional<TestScanner::CScraperCalls::CRegistration> made{std::in_place, calls};
  auto waiting{std::async(std::launch::async, [&calls]
                          { const TestScanner::CScraperCalls::CRegistration call{calls}; })};

  const bool waited{waiting.wait_for(std::chrono::milliseconds(200)) ==
                    std::future_status::timeout};
  made.reset();
  const bool placed{waiting.wait_for(std::chrono::seconds(5)) == std::future_status::ready};
  // Lets it through, should it still be waiting, so that the test ends
  calls.CancelAll();
  EXPECT_TRUE(waited);
  EXPECT_TRUE(placed);
}

// A stopped scan lets the calls waiting for a place go ahead, to be given up at once
TEST(TestVideoInfoScannerScraperCalls, AStopLetsTheWaitingCallsThrough)
{
  TestScanner::CScraperCalls calls;
  std::optional<TestScanner::CScraperCalls::CRegistration> made{std::in_place, calls};
  auto waiting{std::async(std::launch::async, [&calls]
                          { const TestScanner::CScraperCalls::CRegistration call{calls}; })};

  calls.CancelAll();
  const bool through{waiting.wait_for(std::chrono::seconds(5)) == std::future_status::ready};
  // Frees its place, should it still be waiting, so that the test ends
  made.reset();
  EXPECT_TRUE(through);
}

// Of the calls waiting, the one for the video being added goes first, then those of the videos
// looked up ahead of it in the order the scan adds them
TEST(TestVideoInfoScannerScraperCalls, TheCallForTheVideoNeededSoonestGoesFirst)
{
  TestScanner::CScraperCalls calls;
  std::optional<TestScanner::CScraperCalls::CRegistration> made{std::in_place, calls};
  std::mutex lock;
  std::vector<std::string> turns;
  const auto call{[&calls, &lock, &turns](std::optional<size_t> place, std::string name)
                  {
                    return std::async(std::launch::async,
                                      [&calls, &lock, &turns, place, name]
                                      {
                                        std::optional<TestScanner::CScraperCalls::CPlace> ahead;
                                        if (place)
                                          ahead.emplace(*place);
                                        const TestScanner::CScraperCalls::CRegistration turn{calls};
                                        std::unique_lock guard(lock);
                                        turns.push_back(name);
                                      });
                  }};
  std::vector<std::future<void>> waiting;
  waiting.push_back(call(5, "sixth"));
  waiting.push_back(call(std::nullopt, "added"));
  waiting.push_back(call(1, "second"));
  const auto deadline{std::chrono::steady_clock::now() + std::chrono::seconds(5)};
  while (calls.Waiting() < waiting.size() && std::chrono::steady_clock::now() < deadline)
    std::this_thread::sleep_for(std::chrono::milliseconds(1));

  made.reset();
  bool through{true};
  for (std::future<void>& call : waiting)
    through = call.wait_for(std::chrono::seconds(5)) == std::future_status::ready && through;
  // Lets them through, should any still be waiting, and lets them finish before their turns are
  // read
  calls.CancelAll();
  for (std::future<void>& call : waiting)
    call.wait();

  EXPECT_TRUE(through);
  EXPECT_EQ((std::vector<std::string>{"added", "second", "sixth"}), turns);
}

// Music videos

TEST_F(TestVideoInfoScanner, AFolderOfAMusicVideoSourceIsNotReportedAsNotFound)
{
  ASSERT_TRUE(m_db.ExecuteQuery(
      m_db.PrepareSQL("UPDATE path SET strContent='musicvideos', useFolderNames=0 WHERE idPath=%i",
                      m_db.GetPathId(m_root))));
  const std::string folder{Folder(m_root, "Artist")};
  File(folder, "Video.nfo", "<musicvideo><title>Video</title><artist>Artist</artist></musicvideo>");
  File(folder, "Video.mkv");

  const auto summary{Scan()};

  EXPECT_EQ(1, m_db.GetSingleValueInt("SELECT COUNT(*) FROM musicvideo"));
  ASSERT_EQ(1u, summary.added.size());
  EXPECT_EQ(TestScanner::ScanSummary::Type::MUSIC_VIDEO, summary.added[0].type);
  EXPECT_TRUE(summary.notFound.empty());
}

// TV shows

namespace
{
class TestVideoInfoScannerTvShows : public TestVideoInfoScanner
{
protected:
  void SetUp() override
  {
    TestVideoInfoScanner::SetUp();

    // A tv show source, each of whose folders holds a show
    ASSERT_TRUE(m_db.ExecuteQuery(m_db.PrepareSQL(
        "UPDATE path SET strContent='tvshows', useFolderNames=0, scanRecursive=0 WHERE idPath=%i",
        m_db.AddPath(m_root))));

    // An episode is given a thumbnail generated from the video, and a test has no texture cache to
    // cache it in
    Settings()->SetBool(CSettings::SETTING_MYVIDEOS_EXTRACTTHUMB, false);
  }

  void TearDown() override
  {
    if (m_textureCache)
    {
      // First, as its jobs are cancelled with the job manager
      CServiceBroker::UnregisterTextureCache();
      CServiceBroker::GetJobManager()->CancelJobs();
      CServiceBroker::GetJobManager()->Restart();
      CServiceBroker::UnregisterJobManager();
      if (m_previousJobManager)
        CServiceBroker::RegisterJobManager(m_previousJobManager);
    }
    Settings()->SetBool(CSettings::SETTING_MYVIDEOS_EXTRACTTHUMB, m_extractThumb);
    TestVideoInfoScanner::TearDown();
  }

  static std::shared_ptr<CSettings> Settings()
  {
    return CServiceBroker::GetSettingsComponent()->GetSettings();
  }

  //! A show with a full nfo, so it is added without a scraper
  std::string Show(const std::string& name)
  {
    const std::string show{Folder(m_root, name)};
    File(show, "tvshow.nfo", "<tvshow><title>" + name + "</title></tvshow>");
    return show;
  }

  //! An episode with a full nfo
  static std::string Episode(const std::string& folder, const std::string& name)
  {
    File(folder, URIUtils::ReplaceExtension(name, ".nfo"),
         "<episodedetails><title>" + name + "</title></episodedetails>");
    return File(folder, name);
  }

  //! Caches the art a scan finds, as the cast's thumbs are, in a texture cache of the test's own
  void UseTextureCache()
  {
    m_previousJobManager = CServiceBroker::GetJobManager();
    CServiceBroker::RegisterJobManager(std::make_shared<CJobManager>());
    CServiceBroker::RegisterTextureCache(std::make_shared<CTextureCache>());
    m_textureCache = true;
  }

  //! A refresh of a show with all its episodes removes it before adding it again, so one whose
  //! details can't be read is kept. The show has no nfo, or one that overrides some of the
  //! details it is looked up for.
  void RefreshThatCantReadTheDetails(bool overrideNfo)
  {
    UseTestScraper();
    Found("Show");
    Details("Show", "<details><title>Show</title></details>");
    const std::string show{Folder(m_root, "Show")};
    const std::string episode{Episode(Folder(show, "Season 1"), "Show S01E01.mkv")};
    Scan();
    const int idShow{m_db.GetSingleValueInt("SELECT idShow FROM tvshow")};
    ASSERT_GT(idShow, 0);
    ASSERT_GT(m_db.GetEpisodeId(episode, 1, 1), 0);

    if (overrideNfo)
      File(show, "tvshow.nfo", R"(<tvshow override="true"><title>Show</title></tvshow>)");
    DetailsFail("Show");
    CVideoInfoTag tag;
    ASSERT_TRUE(m_db.GetTvShowInfo({}, tag, idShow));
    TestRefreshingJob refresh{std::make_shared<CFileItem>(tag), true, true};
    refresh.Work(m_db);

    EXPECT_EQ(idShow, m_db.GetSingleValueInt("SELECT idShow FROM tvshow"));
    EXPECT_EQ(1, CountShows());
    EXPECT_GT(m_db.GetEpisodeId(episode, 1, 1), 0);
  }

  int CountShows() { return m_db.GetSingleValueInt("SELECT COUNT(*) FROM tvshow"); }
  int CountEpisodes() { return m_db.GetSingleValueInt("SELECT COUNT(*) FROM episode"); }

  const bool m_extractThumb{Settings()->GetBool(CSettings::SETTING_MYVIDEOS_EXTRACTTHUMB)};
  bool m_textureCache{false};
  std::shared_ptr<CJobManager> m_previousJobManager;
};
} // namespace

TEST_F(TestVideoInfoScannerTvShows, AShowIsAddedWithItsEpisodes)
{
  const std::string season{Folder(Show("Show"), "Season 1")};
  const std::string first{Episode(season, "Show S01E01.mkv")};
  const std::string second{Episode(season, "Show S01E02.mkv")};

  Scan();

  EXPECT_EQ(1, CountShows());
  EXPECT_GT(m_db.GetEpisodeId(first, 1, 1), 0);
  EXPECT_GT(m_db.GetEpisodeId(second, 2, 1), 0);
  EXPECT_EQ(2, CountEpisodes());
}

// The local scraper has no episode guide to look an episode up in
TEST_F(TestVideoInfoScannerTvShows, AnEpisodeWithoutAnNfoIsNotAdded)
{
  const std::string season{Folder(Show("Show"), "Season 1")};
  Episode(season, "Show S01E01.mkv");
  File(season, "Show S01E02.mkv");

  Scan();

  EXPECT_EQ(1, CountEpisodes());
}

TEST_F(TestVideoInfoScannerTvShows, AnEpisodeAddedToAScannedSeasonIsFound)
{
  const std::string season{Folder(Show("Show"), "Season 1")};
  Episode(season, "Show S01E01.mkv");
  Scan();
  ASSERT_EQ(1, CountEpisodes());

  const std::string second{Episode(season, "Show S01E02.mkv")};
  Touch(season);
  const auto summary{Scan()};

  EXPECT_GT(m_db.GetEpisodeId(second, 2, 1), 0);
  EXPECT_EQ(2, CountEpisodes());
  ASSERT_EQ(1u, summary.added.size());
  EXPECT_EQ(TestScanner::ScanSummary::Type::EPISODE, summary.added[0].type);
  ASSERT_EQ(1u, summary.existing.size());
  EXPECT_EQ(TestScanner::ScanSummary::Type::EPISODE, summary.existing[0].type);
}

TEST_F(TestVideoInfoScannerTvShows, AnUnchangedShowAddsNothingAgain)
{
  const std::string season{Folder(Show("Show"), "Season 1")};
  Episode(season, "Show S01E01.mkv");
  Episode(season, "Show S01E02.mkv");
  Scan();

  const auto summary{Scan()};

  EXPECT_EQ(1, CountShows());
  EXPECT_EQ(2, CountEpisodes());
  EXPECT_EQ(1u, summary.unchangedDirectories);
  EXPECT_TRUE(summary.existing.empty());
  EXPECT_TRUE(summary.added.empty());
}

// A season is decided by its own time, not the one its show's listing gives it, which Windows
// updates late after a file is added
TEST_F(TestVideoInfoScannerTvShows, AnEpisodeIsFoundWhileItsShowListsTheSeasonsOldTime)
{
  const std::string show{Show("Show")};
  const std::string season{Folder(show, "Season 1")};
  Episode(season, "Show S01E01.mkv");
  Settle(season);
  Scan();
  CFileItemList stale;
  ASSERT_TRUE(XFILE::CDirectory::GetDirectory(show, stale, "", XFILE::DIR_FLAG_GET_HIDDEN));

  Episode(season, "Show S01E02.mkv");
  Touch(season);
  g_directoryCache.SetDirectory(CURL{show}, stale, XFILE::CacheType::ALWAYS);
  Scan();
  g_directoryCache.ClearDirectory(CURL{show});

  EXPECT_EQ(2, CountEpisodes());
}

// Each folder of a show is recorded, as is the source holding the shows. A season is decided by its
// own time rather than by listing it.
TEST_F(TestVideoInfoScannerTvShows, ASeasonWhoseTimeHasNotMovedIsNotListedAgain)
{
  const std::string show{Show("Show")};
  const std::string season{Folder(show, "Season 1")};
  Episode(season, "Show S01E01.mkv");
  Settle(season);
  Scan();
  ASSERT_EQ(1, CountEpisodes());
  EXPECT_TRUE(Record(m_root).starts_with("2|"));
  EXPECT_TRUE(Record(show).starts_with("2|"));
  EXPECT_TRUE(Record(season).starts_with("2|"));

  std::string second;
  KeepingItsTime(season, [&second, &season] { second = Episode(season, "Show S01E02.mkv"); });
  Scan();

  EXPECT_EQ(-1, m_db.GetEpisodeId(second, 2, 1));
  EXPECT_EQ(1, CountEpisodes());
}

// An episode deleted from a show, leaving it without an episode, is cleaned by the scan after one
// stopped before its clean
TEST_F(TestVideoInfoScannerTvShows, AnEpisodeDeletedIsCleanedByTheScanAfterOneStoppedBeforeItsClean)
{
  const std::string show{Show("Show")};
  const std::string episode{Episode(show, "Show S01E01.mkv")};
  File(show, "Interview.mkv");
  const std::string other{Show("Other")};
  Episode(other, "Other S01E01.mkv");
  Scan();
  ASSERT_EQ(2, CountEpisodes());

  ASSERT_TRUE(XFILE::CFile::Delete(episode));
  Touch(show);
  {
    TestScanner stopped{m_settings, false, true};
    stopped.Scan(m_root, true);
  }
  ASSERT_EQ(2, CountEpisodes());

  // A change in the other show, so that the clean is of the folders the scan queued, not of all
  File(other, "Notes.txt");
  Touch(other);
  Scan(false, true);
  EXPECT_EQ(1, CountEpisodes());
}

TEST_F(TestVideoInfoScannerTvShows, AShowFolderWithoutEpisodesIsNotListedAgain)
{
  const std::string show{Show("Show")};
  File(show, "Interview.mkv");
  Settle(show);
  Scan();
  ASSERT_EQ(0, CountEpisodes());
  EXPECT_TRUE(Record(show).starts_with("2|"));

  std::string episode;
  KeepingItsTime(show, [&episode, &show] { episode = Episode(show, "Show S01E01.mkv"); });
  Scan();

  EXPECT_EQ(-1, m_db.GetEpisodeId(episode, 1, 1));
  EXPECT_EQ(0, CountEpisodes());
}

// A source a scan stopped in keeps no record, so that the next scan looks at it again
TEST_F(TestVideoInfoScannerTvShows, ASourceAStoppedScanLeftIsLookedAtAgain)
{
  Episode(Show("Show"), "Show S01E01.mkv");
  TestScanner scanner{m_settings};
  scanner.Stop();

  scanner.Scan(m_root);

  EXPECT_EQ("", Record(m_root));
}

// As a Blu-ray rip's, an AVCHD rip's playlists, streams and backup copies are below its index, so
// aren't listed. Its episodes don't show it, as the rip is one episode on its index either way.
TEST_F(TestVideoInfoScannerTvShows, AnAvchdRipInAShowIsNotListedBelowItsIndex)
{
  const std::string show{Show("Show")};
  const std::string index{AvchdRip(Folder(show, "Show.S01E01"))};

  TestScanner scanner{m_settings};
  CFileItemList files;
  std::vector<std::pair<std::string, FolderRecord>> records;
  scanner.CollectShowFiles(show, files, records);

  ASSERT_FALSE(files.IsEmpty());
  for (const auto& file : files)
    EXPECT_EQ(URIUtils::GetDirectory(index), URIUtils::GetDirectory(file->GetPath()));
}

TEST_F(TestVideoInfoScannerTvShows, AFileOfTwoEpisodesIsTwoEpisodes)
{
  const std::string episodes{Episode(Folder(Show("Show"), "Season 1"), "Show S01E01E02.mkv")};

  Scan();

  EXPECT_GT(m_db.GetEpisodeId(episodes, 1, 1), 0);
  EXPECT_GT(m_db.GetEpisodeId(episodes, 2, 1), 0);
  EXPECT_EQ(1, m_db.GetSingleValueInt("SELECT COUNT(DISTINCT idFile) FROM episode"));
}

// A file named with the same episode twice lists it twice, and it is added once
TEST_F(TestVideoInfoScannerTvShows, ARepeatedEpisodeNumberIsAddedOnlyOnce)
{
  const std::string episode{Episode(Folder(Show("Show"), "Season 1"), "Show S01E01E01.mkv")};

  Scan();

  EXPECT_GT(m_db.GetEpisodeId(episode, 1, 1), 0);
  EXPECT_EQ(1, CountEpisodes());
}

// The file is probed for its first episode only; the second has the same streams, and its subtitles
// are still looked for
TEST_F(TestVideoInfoScannerTvShows, TheEpisodesOfAFileHaveItsStreamsAndSubtitles)
{
  Settings()->SetBool(CSettings::SETTING_MYVIDEOS_EXTRACTFLAGS, true);
  const std::string season{Folder(Show("Show"), "Season 1")};
  const std::string episodes{URIUtils::AddFileToFolder(season, "Show S01E01E02.mkv")};
  ASSERT_TRUE(XFILE::CFile::Copy(
      XBMC_REF_FILE_PATH("xbmc/cores/VideoPlayer/test/testdata/h264_8bit_320x240.mkv"), episodes));
  // Each episode of a file of several has an nfo of its own, named by its season and episode
  File(season, "Show S01E01E02-S01E01.nfo", "<episodedetails><title>1</title></episodedetails>");
  File(season, "Show S01E01E02-S01E02.nfo", "<episodedetails><title>2</title></episodedetails>");
  File(season, "Show S01E01E02.en.srt", "1\n00:00:00,000 --> 00:00:01,000\nHello\n");

  Scan();

  for (const int episode : {1, 2})
  {
    CVideoInfoTag tag;
    ASSERT_TRUE(m_db.GetEpisodeInfo(episodes, tag, m_db.GetEpisodeId(episodes, episode, 1)));
    EXPECT_EQ(std::to_string(episode), tag.m_strTitle);
    ASSERT_TRUE(m_db.GetStreamDetails(tag));
    EXPECT_EQ(320, tag.m_streamDetails.GetVideoWidth()) << episode;
    EXPECT_EQ(1, tag.m_streamDetails.GetSubtitleStreamCount()) << episode;
  }
}

TEST_F(TestVideoInfoScannerTvShows, ASpecialIsInSeasonZero)
{
  const std::string special{Episode(Folder(Show("Show"), "Specials"), "Show S00E01.mkv")};

  Scan();

  EXPECT_GT(m_db.GetEpisodeId(special, 1, 0), 0);
}

TEST_F(TestVideoInfoScannerTvShows, ABlurayRipInAShowIsOneEpisode)
{
  const std::string show{Show("Show")};
  BlurayRip(Folder(show, "Show.S01E01"));

  TestScanner scanner{m_settings};
  CFileItem item{show, true};
  EPISODELIST episodes;
  EXPECT_EQ(TestScanner::EpisodeResult::FOUND_EPISODES,
            scanner.EnumerateSeriesFolder(&item, episodes));

  ASSERT_EQ(1U, episodes.size());
  EXPECT_EQ(1, episodes[0].iSeason);
  EXPECT_EQ(1, episodes[0].iEpisode);
}

// Below the rip's BDMV folder only its index and movie object are listed, and each was the episode
TEST_F(TestVideoInfoScannerTvShows, AnAvchdRipOfAnEpisodeIsOneEpisode)
{
  const std::string show{Show("Show")};
  const std::string index{AvchdRip(Folder(show, "Show.S01E01"))};

  TestScanner scanner{m_settings};
  CFileItem item{show, true};
  EPISODELIST episodes;
  EXPECT_EQ(TestScanner::EpisodeResult::FOUND_EPISODES,
            scanner.EnumerateSeriesFolder(&item, episodes));

  ASSERT_EQ(1U, episodes.size());
  EXPECT_EQ(1, episodes[0].iSeason);
  EXPECT_EQ(1, episodes[0].iEpisode);
  EXPECT_EQ(index, episodes[0].strPath);
}

// A disc put in a disc structure folder already there changes the time of that folder only
TEST_F(TestVideoInfoScannerTvShows, ADiscPutInADiscFolderOfAScannedShowIsFound)
{
  const std::string show{Show("Show")};
  Episode(show, "Show S01E01.mkv");
  const std::string rip{Folder(show, "Show.S01E02")};
  const std::string bdmv{Folder(rip, "BDMV")};
  Settle(rip);
  Scan();
  ASSERT_TRUE(Record(rip).starts_with("2|"));

  const std::string index{BlurayRip(rip)};
  Touch(bdmv);
  TestScanner scanner{m_settings};
  CFileItem item{show, true};
  EPISODELIST episodes;
  ASSERT_EQ(TestScanner::EpisodeResult::FOUND_EPISODES,
            scanner.EnumerateSeriesFolder(&item, episodes));

  EXPECT_NE(episodes.end(), std::ranges::find(episodes, index, &EPISODE::strPath));
}

TEST_F(TestVideoInfoScannerTvShows, AShowsCastHaveTheThumbsInItsActorsFolder)
{
  UseTextureCache();
  // A show's item is its folder, rather than a file in the folder
  const std::string show{Folder(m_root, "Show")};
  File(show, "tvshow.nfo",
       "<tvshow><title>Show</title><actor><name>Actor Name</name></actor></tvshow>");
  const std::string thumb{File(Folder(show, ".actors"), "Actor_Name.jpg")};
  Episode(Folder(show, "Season 1"), "Show S01E01.mkv");

  Scan();

  EXPECT_EQ(thumb, m_db.GetSingleValue(
                       "SELECT url FROM art WHERE media_type = 'actor' AND type = 'thumb'"));
}

TEST_F(TestVideoInfoScannerTvShows, EachShowsCastAndGuestsHaveTheThumbsInItsActorsFolder)
{
  UseTextureCache();
  std::map<std::string, std::string> thumbs;
  for (const std::string name : {"A", "B", "C", "D"})
  {
    const std::string show{Folder(m_root, "Show " + name)};
    File(show, "tvshow.nfo",
         "<tvshow><title>Show " + name + "</title><actor><name>Star " + name +
             "</name></actor></tvshow>");
    const std::string actors{Folder(show, ".actors")};
    thumbs["Star " + name] = File(actors, "Star_" + name + ".jpg");
    thumbs["Guest " + name] = File(actors, "Guest_" + name + ".jpg");
    const std::string season{Folder(show, "Season 1")};
    for (int episode = 1; episode <= 4; ++episode)
    {
      const std::string file{"Show " + name + " S01E0" + std::to_string(episode) + ".mkv"};
      File(season, URIUtils::ReplaceExtension(file, ".nfo"),
           "<episodedetails><title>" + file + "</title><actor><name>Guest " + name +
               "</name></actor></episodedetails>");
      File(season, file);
    }
  }

  Scan();

  for (const auto& [actor, thumb] : thumbs)
  {
    EXPECT_EQ(thumb, m_db.GetSingleValue(m_db.PrepareSQL(
                         "SELECT url FROM art JOIN actor ON art.media_id = actor.actor_id WHERE "
                         "art.media_type = 'actor' AND art.type = 'thumb' AND actor.name = '%s'",
                         actor.c_str())))
        << actor;
  }
}

// TV shows looked up with a scraper

TEST_F(TestVideoInfoScannerTvShows, AShowWhoseDetailsFailedIsLookedUpAgain)
{
  UseTestScraper();
  DetailsFail("Show");
  const std::string episode{Episode(Folder(Folder(m_root, "Show"), "Season 1"), "Show S01E01.mkv")};
  CEventLog* eventLog{CServiceBroker::GetEventLog()};
  ASSERT_NE(nullptr, eventLog);
  const size_t warnings{eventLog->Get(EventLevel::Warning).size()};
  const auto summary{Scan()};
  ASSERT_EQ(0, CountShows());
  EXPECT_EQ(0, CountEpisodes());
  ASSERT_EQ(1u, summary.failed.size());
  EXPECT_EQ(TestScanner::ScanSummary::Type::TV_SHOW, summary.failed[0].type);
  EXPECT_TRUE(summary.notFound.empty());
  EXPECT_EQ(warnings + 1, eventLog->Get(EventLevel::Warning).size());

  Details("Show", "<details><title>Show</title></details>");
  Scan();

  EXPECT_EQ(1, CountShows());
  EXPECT_GT(m_db.GetEpisodeId(episode, 1, 1), 0);
}

// A refresh of a show with all its episodes removes it before adding it again, so one whose
// details can't be read is kept
TEST_F(TestVideoInfoScannerTvShows, ARefreshThatCantReadTheDetailsKeepsTheShow)
{
  RefreshThatCantReadTheDetails(false);
}

// An override nfo gives some of the details, and the rest are still looked up
TEST_F(TestVideoInfoScannerTvShows, ARefreshWithAnOverrideNfoThatCantReadTheDetailsKeepsTheShow)
{
  RefreshThatCantReadTheDetails(true);
}

// A refresh of an episode adds it again for the same file, so the next scan doesn't look at its
// show's folder again
TEST_F(TestVideoInfoScannerTvShows, ARefreshOfAnEpisodeKeepsTheHashOfItsShowsFolder)
{
  UseTestScraper();
  Found("Show");
  Details("Show", "<details><title>Show</title></details>");
  const std::string show{Folder(m_root, "Show")};
  const std::string episode{Episode(Folder(show, "Season 1"), "Show S01E01.mkv")};
  Scan();
  const int idEpisode{m_db.GetEpisodeId(episode, 1, 1)};
  ASSERT_GT(idEpisode, 0);
  const std::string hash{Record(show)};
  ASSERT_FALSE(hash.empty());

  CVideoInfoTag tag;
  ASSERT_TRUE(m_db.GetEpisodeInfo({}, tag, idEpisode));
  TestRefreshingJob refresh{std::make_shared<CFileItem>(tag), true, false};
  ASSERT_TRUE(refresh.Work(m_db));

  EXPECT_GT(m_db.GetEpisodeId(episode, 1, 1), 0);
  EXPECT_EQ(hash, Record(show));
}

TEST_F(TestVideoInfoScannerTvShows, AnEpisodeWithAnNfoIsAddedWhenTheEpisodeGuideFails)
{
  UseTestScraper();
  const std::string show{Folder(m_root, "Show")};
  File(show, "tvshow.nfo",
       "<tvshow><title>Show</title><episodeguide>guide</episodeguide></tvshow>");
  Page("guide", "<error/>");
  const std::string season{Folder(show, "Season 1")};
  const std::string first{File(season, "Show S01E01.mkv")};
  const std::string second{Episode(season, "Show S01E02.mkv")};
  Scan();
  EXPECT_GT(m_db.GetEpisodeId(second, 2, 1), 0);
  ASSERT_EQ(1, CountEpisodes());

  Page("guide", "<episodeguide><episode><title>First</title><url>first</url>"
                "<epnum>1</epnum><season>1</season></episode></episodeguide>");
  Page("first", "<details><title>First</title></details>");
  Scan();

  EXPECT_GT(m_db.GetEpisodeId(first, 1, 1), 0);
  EXPECT_EQ(2, CountEpisodes());
}

TEST_F(TestVideoInfoScannerTvShows, AnEpisodeWhoseDetailsFailedIsLookedUpAgain)
{
  UseTestScraper();
  const std::string show{Folder(m_root, "Show")};
  File(show, "tvshow.nfo",
       "<tvshow><title>Show</title><episodeguide>guide</episodeguide></tvshow>");
  Page("guide", "<episodeguide><episode><title>First</title><url>first</url>"
                "<epnum>1</epnum><season>1</season></episode></episodeguide>");
  Page("first", "<error/>");
  const std::string season{Folder(show, "Season 1")};
  const std::string first{File(season, "Show S01E01.mkv")};
  const std::string second{Episode(season, "Show S01E02.mkv")};
  Scan();
  EXPECT_GT(m_db.GetEpisodeId(second, 2, 1), 0);
  ASSERT_EQ(1, CountEpisodes());

  Page("first", "<details><title>First</title></details>");
  Scan();

  EXPECT_GT(m_db.GetEpisodeId(first, 1, 1), 0);
  EXPECT_EQ(2, CountEpisodes());
}

// The episodes of a show are looked up alongside each other, and each is added with its own details
TEST_F(TestVideoInfoScannerTvShows, TheEpisodesOfAShowAreEachAddedWithTheirDetails)
{
  UseTestScraper();
  const std::string show{Folder(m_root, "Show")};
  File(show, "tvshow.nfo",
       "<tvshow><title>Show</title><episodeguide>guide</episodeguide></tvshow>");
  const std::string season{Folder(show, "Season 1")};
  std::string guide{"<episodeguide>"};
  std::vector<std::string> files;
  constexpr int episodes{8};
  for (int episode = 1; episode <= episodes; ++episode)
  {
    const std::string name{"episode" + std::to_string(episode)};
    guide += "<episode><title>" + name + "</title><url>" + name + "</url><epnum>" +
             std::to_string(episode) + "</epnum><season>1</season></episode>";
    Page(name, "<details><title>Title " + std::to_string(episode) + "</title></details>");
    files.emplace_back(File(season, "Show S01E0" + std::to_string(episode) + ".mkv"));
  }
  Page("guide", guide + "</episodeguide>");

  Scan();

  ASSERT_EQ(episodes, CountEpisodes());
  for (int episode = 1; episode <= episodes; ++episode)
  {
    CVideoInfoTag tag;
    ASSERT_TRUE(m_db.GetEpisodeInfo(files[episode - 1], tag,
                                    m_db.GetEpisodeId(files[episode - 1], episode, 1)));
    EXPECT_EQ("Title " + std::to_string(episode), tag.m_strTitle);
  }
}

// A show's episodes are looked up in the episode guide stored when it was added, which only the
// scraper that added it understands
TEST_F(TestVideoInfoScannerTvShows, AnExistingShowKeepsItsScraperWhenItsNfoNamesAnother)
{
  UseTestScraper();
  InstallOtherScraper();
  const std::string show{Folder(m_root, "Show")};
  File(show, "tvshow.nfo",
       "<tvshow><title>Show</title><episodeguide>guide</episodeguide></tvshow>");
  Page("guide", "<episodeguide>"
                "<episode><title>First</title><url>first</url><epnum>1</epnum><season>1</season>"
                "</episode>"
                "<episode><title>Second</title><url>second</url><epnum>2</epnum><season>1</season>"
                "</episode>"
                "</episodeguide>");
  Page("first", "<details><title>First</title></details>");
  Page("second", "<details><title>Second</title></details>");
  const std::string season{Folder(show, "Season 1")};
  File(season, "Show S01E01.mkv");
  Scan();
  ASSERT_EQ(1, CountEpisodes());

  // A url that only the other scraper recognises
  File(show, "tvshow.nfo", "http://other.invalid/1");
  const std::string second{File(season, "Show S01E02.mkv")};
  Touch(season);
  Scan();

  EXPECT_GT(m_db.GetEpisodeId(second, 2, 1), 0);
  EXPECT_EQ(2, CountEpisodes());
}

namespace
{
constexpr const char* PYTHON_SCRAPER_ID = "metadata.test.videoinfoscanner.python";

constexpr const char* PYTHON_SCRAPER_ADDON = R"(<?xml version="1.0" encoding="UTF-8"?>
<addon id="metadata.test.videoinfoscanner.python" name="Test python scraper" version="1.0.0"
       provider-name="Team Kodi">
  <requires>
    <import addon="xbmc.metadata" version="2.1.0"/>
  </requires>
  <extension point="xbmc.metadata.scraper.movies" library="scraper.py"/>
  <extension point="xbmc.addon.metadata">
    <reuselanguageinvoker>true</reuselanguageinvoker>
    <platform>all</platform>
  </extension>
</addon>)";

//! What the runs of the python scraper share: how many have started, the threads they ran on, and
//! whether they may end
struct SPythonRuns
{
  std::mutex lock;
  std::condition_variable changed;
  int started{0};
  std::vector<int> threads;
  bool end{false};
  //! The options each run was given, as "?action=find&title=..."
  std::vector<std::string> calls;
  //! How a run answers once it may end, with its handle and its call's options; else it finds
  //! nothing
  std::function<void(int, const CURL&)> answer;
};

/*!
 \brief Stands in for the python interpreter running a scraper whose every call waits on a server
 that doesn't answer, as a blocking socket read does: the call ends, finding nothing, only once the
 test lets it
 */
class CBlockingInvoker : public ILanguageInvoker
{
public:
  CBlockingInvoker(ILanguageInvocationHandler* handler, SPythonRuns& runs)
    : ILanguageInvoker(handler),
      m_runs(runs)
  {
  }

protected:
  bool execute(const std::string& script, const std::vector<std::string>& arguments) override
  {
    setState(InvokerStateRunning);
    // As a plugin is run: its url, handle and options
    const int handle{std::stoi(arguments[1])};
    const std::string options{arguments.size() > 2 ? arguments[2] : std::string{}};
    std::function<void(int, const CURL&)> answer;
    {
      std::unique_lock lock(m_runs.lock);
      ++m_runs.started;
      m_runs.calls.push_back(options);
      if (std::ranges::find(m_runs.threads, GetId()) == m_runs.threads.end())
        m_runs.threads.push_back(GetId());
      m_runs.changed.notify_all();
      m_runs.changed.wait_for(lock, std::chrono::seconds{30}, [this] { return m_runs.end; });
      answer = m_runs.answer;
    }
    if (answer)
      answer(handle, CURL{"plugin://" + std::string{PYTHON_SCRAPER_ID} + "/" + options});
    else
      XFILE::CPluginDirectory::EndOfDirectory(handle, true, false, false);
    setState(InvokerStateScriptDone);
    return true;
  }

  bool stop(bool abort) override { return true; }

private:
  SPythonRuns& m_runs;
};

class CBlockingHandler : public ILanguageInvocationHandler
{
public:
  std::shared_ptr<ILanguageInvoker> CreateInvoker() override
  {
    return std::make_shared<CBlockingInvoker>(this, m_runs);
  }

  SPythonRuns m_runs;
};

//! A movie source looked up with a python scraper, whose scripts a stand-in runs
class TestVideoInfoScannerPython : public TestVideoInfoScanner
{
protected:
  void SetUp() override
  {
    TestVideoInfoScanner::SetUp();
    // Python itself isn't run by the tests, so its scripts can be run by the stand-in
    if (Scripts().HasLanguageInvoker("scraper.py"))
      GTEST_SKIP() << "python scripts are run by python";
    Scripts().RegisterLanguageInvocationHandler(&m_handler, ".py");
    m_registered = true;

    const std::string addons{CSpecialProtocol::TranslatePath("special://home/addons/")};
    static_cast<void>(XFILE::CDirectory::Create(addons));
    m_pythonScraper = Folder(addons, PYTHON_SCRAPER_ID);
    File(m_pythonScraper, "addon.xml", PYTHON_SCRAPER_ADDON);
    File(m_pythonScraper, "scraper.py");
    ASSERT_TRUE(CServiceBroker::GetAddonMgr().FindAddon(PYTHON_SCRAPER_ID, ADDON::ORIGIN_SYSTEM,
                                                        ADDON::CAddonVersion{"1.0.0"}));
    ASSERT_TRUE(m_db.ExecuteQuery(
        m_db.PrepareSQL("UPDATE path SET strScraper='%s', strSettings='' WHERE idPath=%i",
                        PYTHON_SCRAPER_ID, m_db.GetPathId(m_root))));
  }

  void TearDown() override
  {
    if (m_registered)
    {
      LetRunsEnd();
      // The threads kept to run the scraper again go once another script is run
      static_cast<void>(Scripts().GetReusablePluginHandle("another.py"));
      std::vector<int> threads;
      {
        std::unique_lock lock(m_handler.m_runs.lock);
        threads = m_handler.m_runs.threads;
      }
      const auto end{std::chrono::steady_clock::now() + std::chrono::seconds{5}};
      for (const int id : threads)
      {
        while (Scripts().IsRunning(id) && std::chrono::steady_clock::now() < end)
          std::this_thread::sleep_for(std::chrono::milliseconds{1});
        EXPECT_FALSE(Scripts().IsRunning(id));
      }
      Scripts().UnregisterLanguageInvocationHandler(&m_handler);
      CServiceBroker::GetAddonMgr().UnloadAddon(PYTHON_SCRAPER_ID);
      XFILE::CDirectory::RemoveRecursive(m_pythonScraper);
    }
    TestVideoInfoScanner::TearDown();
  }

  static CScriptInvocationManager& Scripts() { return CScriptInvocationManager::GetInstance(); }

  bool RunsStarted(int count)
  {
    std::unique_lock lock(m_handler.m_runs.lock);
    return m_handler.m_runs.changed.wait_for(lock, std::chrono::seconds{5}, [this, count]
                                             { return m_handler.m_runs.started >= count; });
  }

  void LetRunsEnd()
  {
    {
      std::unique_lock lock(m_handler.m_runs.lock);
      m_handler.m_runs.end = true;
    }
    m_handler.m_runs.changed.notify_all();
  }

  CBlockingHandler m_handler;
  bool m_registered{false};
  std::string m_pythonScraper;
};

constexpr const char* PYTHON_SHOW_SCRAPER_ID = "metadata.test.videoinfoscanner.python.tvshows";

constexpr const char* PYTHON_SHOW_SCRAPER_ADDON = R"(<?xml version="1.0" encoding="UTF-8"?>
<addon id="metadata.test.videoinfoscanner.python.tvshows" name="Test python show scraper"
       version="1.0.0" provider-name="Team Kodi">
  <requires>
    <import addon="xbmc.metadata" version="2.1.0"/>
  </requires>
  <extension point="xbmc.metadata.scraper.tvshows" library="scraper.py"/>
  <extension point="xbmc.addon.metadata">
    <reuselanguageinvoker>true</reuselanguageinvoker>
    <platform>all</platform>
  </extension>
</addon>)";

/*!
 \brief Answers as a tv show scraper: a search finds the show, whose guide lists episodes 1 and 2
 of season 1. Asked for the episodes' details with the list, it gives the first's there, marked
 complete; the second's are given only when asked for on their own.
 */
void AnswerAsAShowScraper(int handle, const CURL& call)
{
  const std::string action{call.GetOption("action")};
  if (action == "find")
  {
    CFileItem show{"show", false};
    show.SetLabel("Show");
    XFILE::CPluginDirectory::AddItem(handle, &show, 1);
    XFILE::CPluginDirectory::EndOfDirectory(handle, true, false, false);
  }
  else if (action == "getdetails")
  {
    CFileItem show{"show", false};
    show.GetVideoInfoTag()->SetTitle("Show");
    show.GetVideoInfoTag()->SetEpisodeGuide("guide");
    XFILE::CPluginDirectory::SetResolvedUrl(handle, true, &show);
  }
  else if (action == "getepisodelist")
  {
    for (const int number : {1, 2})
    {
      CFileItem episode{number == 1 ? "first" : "second", false};
      CVideoInfoTag& tag{*episode.GetVideoInfoTag()};
      tag.m_iSeason = 1;
      tag.m_iEpisode = number;
      tag.SetTitle(episode.GetPath());
      if (number == 1 && call.GetOption("complete") == "1")
      {
        tag.SetPlot("Listed with the episodes");
        episode.SetProperty("video.complete", true);
      }
      XFILE::CPluginDirectory::AddItem(handle, &episode, 2);
    }
    XFILE::CPluginDirectory::EndOfDirectory(handle, true, false, false);
  }
  else if (action == "getepisodedetails")
  {
    CFileItem episode{call.GetOption("url"), false};
    episode.GetVideoInfoTag()->SetTitle(call.GetOption("url"));
    episode.GetVideoInfoTag()->SetPlot("Asked for on its own");
    XFILE::CPluginDirectory::SetResolvedUrl(handle, true, &episode);
  }
  else
    XFILE::CPluginDirectory::EndOfDirectory(handle, false, false, false);
}

//! A tv show source looked up with a python scraper, whose scripts a stand-in runs
class TestVideoInfoScannerPythonShows : public TestVideoInfoScannerPython
{
protected:
  void SetUp() override
  {
    TestVideoInfoScannerPython::SetUp();
    if (IsSkipped())
      return;

    const std::string addons{CSpecialProtocol::TranslatePath("special://home/addons/")};
    m_showScraper = Folder(addons, PYTHON_SHOW_SCRAPER_ID);
    File(m_showScraper, "addon.xml", PYTHON_SHOW_SCRAPER_ADDON);
    File(m_showScraper, "scraper.py");
    ASSERT_TRUE(CServiceBroker::GetAddonMgr().FindAddon(
        PYTHON_SHOW_SCRAPER_ID, ADDON::ORIGIN_SYSTEM, ADDON::CAddonVersion{"1.0.0"}));
    // Each of the source's folders holds a show
    ASSERT_TRUE(m_db.ExecuteQuery(m_db.PrepareSQL(
        "UPDATE path SET strContent='tvshows', strScraper='%s', strSettings='', useFolderNames=0, "
        "scanRecursive=0 WHERE idPath=%i",
        PYTHON_SHOW_SCRAPER_ID, m_db.GetPathId(m_root))));
    // An episode is given a thumbnail generated from the video, and a test has no texture cache to
    // cache it in
    Settings()->SetBool(CSettings::SETTING_MYVIDEOS_EXTRACTTHUMB, false);
  }

  void TearDown() override
  {
    if (!m_showScraper.empty())
    {
      Settings()->SetBool(CSettings::SETTING_MYVIDEOS_EXTRACTTHUMB, m_extractThumb);
      CServiceBroker::GetAddonMgr().UnloadAddon(PYTHON_SHOW_SCRAPER_ID);
      XFILE::CDirectory::RemoveRecursive(m_showScraper);
    }
    TestVideoInfoScannerPython::TearDown();
  }

  static std::shared_ptr<CSettings> Settings()
  {
    return CServiceBroker::GetSettingsComponent()->GetSettings();
  }

  //! How many of the scraper's calls hold each of \p parts
  size_t Calls(const std::vector<std::string>& parts)
  {
    std::unique_lock lock(m_handler.m_runs.lock);
    return std::ranges::count_if(m_handler.m_runs.calls,
                                 [&parts](const std::string& call)
                                 {
                                   return std::ranges::all_of(
                                       parts, [&call](const std::string& part)
                                       { return call.find(part) != std::string::npos; });
                                 });
  }

  const bool m_extractThumb{Settings()->GetBool(CSettings::SETTING_MYVIDEOS_EXTRACTTHUMB)};
  std::string m_showScraper;
};
} // namespace

// A python scraper's call can't be cancelled, and the stop of a scan can't interrupt a socket read
// the script waits in. The scan stops waiting for it instead.
TEST_F(TestVideoInfoScannerPython, AStoppedScanStopsWaitingForAScrapersCall)
{
  // Searched for one at a time: one is being searched for, and the other waits its turn
  File(m_root, "Alpha (2001).mkv");
  File(m_root, "Beta (2002).mkv");
  TestScanner scanner{m_settings};
  std::future<void> scan{
      std::async(std::launch::async, [this, &scanner] { scanner.Scan(m_root); })};
  const bool searching{RunsStarted(1)};

  scanner.Stop();
  const bool stopped{scan.wait_for(std::chrono::seconds{10}) == std::future_status::ready};
  LetRunsEnd();
  scan.wait();

  EXPECT_TRUE(searching);
  EXPECT_TRUE(stopped);
  EXPECT_EQ(0, CountMovies());
}

// A music video, looked up on the scan's own thread, whose details call ends in failure as the scan
// is stopped isn't a failed look-up
TEST_F(TestVideoInfoScannerPython, AVideoWhoseCallAStopEndsIsNoFailedLookUp)
{
  const std::string id{"metadata.test.videoinfoscanner.python.musicvideos"};
  struct SAddon
  {
    std::string id;
    std::string folder;
    ~SAddon()
    {
      CServiceBroker::GetAddonMgr().UnloadAddon(id);
      XFILE::CDirectory::RemoveRecursive(folder);
    }
  } const addon{id, Folder(CSpecialProtocol::TranslatePath("special://home/addons/"), id)};
  File(addon.folder, "addon.xml", R"(<?xml version="1.0" encoding="UTF-8"?>
<addon id="metadata.test.videoinfoscanner.python.musicvideos" name="Test" version="1.0.0"
       provider-name="Team Kodi">
  <requires>
    <import addon="xbmc.metadata" version="2.1.0"/>
  </requires>
  <extension point="xbmc.metadata.scraper.musicvideos" library="scraper.py"/>
  <extension point="xbmc.addon.metadata">
    <reuselanguageinvoker>true</reuselanguageinvoker>
    <platform>all</platform>
  </extension>
</addon>)");
  File(addon.folder, "scraper.py");
  ASSERT_TRUE(CServiceBroker::GetAddonMgr().FindAddon(id, ADDON::ORIGIN_SYSTEM,
                                                      ADDON::CAddonVersion{"1.0.0"}));
  ASSERT_TRUE(m_db.ExecuteQuery(m_db.PrepareSQL(
      "UPDATE path SET strContent='musicvideos', strScraper='%s', strSettings='' WHERE idPath=%i",
      id.c_str(), m_db.GetPathId(m_root))));
  File(m_root, "Artist - Song.mkv");
  std::mutex lock;
  std::condition_variable changed;
  bool asked{false};
  bool answer{false};
  {
    std::unique_lock guard(m_handler.m_runs.lock);
    m_handler.m_runs.answer = [&](int handle, const CURL& call)
    {
      if (call.GetOption("action") == "find")
      {
        CFileItem video{"song", false};
        video.SetLabel("Song");
        XFILE::CPluginDirectory::AddItem(handle, &video, 1);
        XFILE::CPluginDirectory::EndOfDirectory(handle, true, false, false);
        return;
      }
      std::unique_lock details(lock);
      asked = true;
      changed.notify_all();
      changed.wait_for(details, std::chrono::seconds{30}, [&answer] { return answer; });
      XFILE::CPluginDirectory::EndOfDirectory(handle, false, false, false);
    };
  }
  LetRunsEnd();
  TestScanner scanner{m_settings};
  std::future<void> scan{
      std::async(std::launch::async, [this, &scanner] { scanner.Scan(m_root); })};
  bool detailsAsked{false};
  {
    std::unique_lock guard(lock);
    detailsAsked = changed.wait_for(guard, std::chrono::seconds{10}, [&asked] { return asked; });
  }

  scanner.Stop();
  {
    std::unique_lock guard(lock);
    answer = true;
  }
  changed.notify_all();
  scan.wait();

  ASSERT_TRUE(detailsAsked);
  EXPECT_TRUE(scanner.Summary().failed.empty());
}

// A scan reads the settings of a folder's scraper as it lists the folder, while a look-up's call to
// the scraper runs: reading them doesn't wait for the call to end
TEST_F(TestVideoInfoScannerPython, AScrapersSettingsAreReadWhileItMakesACall)
{
  ADDON::AddonPtr addon;
  ASSERT_TRUE(CServiceBroker::GetAddonMgr().GetAddon(PYTHON_SCRAPER_ID, addon,
                                                     ADDON::OnlyEnabled::CHOICE_YES));
  const ADDON::ScraperPtr scraper{std::dynamic_pointer_cast<ADDON::CScraper>(addon)};
  ASSERT_TRUE(scraper);
  std::future<void> search{std::async(std::launch::async, [&scraper]
                                      { static_cast<void>(scraper->FindMovie("Alpha", 2001)); })};
  const bool searching{RunsStarted(1)};
  std::future<std::string> settings{
      std::async(std::launch::async, [&scraper] { return scraper->GetPathSettings(); })};

  // Long enough for the settings to be read, were they not waiting for the search
  const bool read{settings.wait_for(std::chrono::seconds{2}) == std::future_status::ready};
  LetRunsEnd();
  search.wait();
  settings.wait();

  EXPECT_TRUE(searching);
  EXPECT_TRUE(read);
}

// A scraper makes one call at a time: a second waits for the first to end
TEST_F(TestVideoInfoScannerPython, AScraperMakesOneCallAtATime)
{
  ADDON::AddonPtr addon;
  ASSERT_TRUE(CServiceBroker::GetAddonMgr().GetAddon(PYTHON_SCRAPER_ID, addon,
                                                     ADDON::OnlyEnabled::CHOICE_YES));
  const ADDON::ScraperPtr scraper{std::dynamic_pointer_cast<ADDON::CScraper>(addon)};
  ASSERT_TRUE(scraper);
  const auto search{[&scraper](const std::string& title)
                    {
                      return std::async(std::launch::async, [&scraper, title]
                                        { static_cast<void>(scraper->FindMovie(title, 2001)); });
                    }};
  std::future<void> first{search("Alpha")};
  const bool searching{RunsStarted(1)};
  std::future<void> second{search("Beta")};
  const bool overlapped{RunsStarted(2)};
  LetRunsEnd();
  first.wait();
  second.wait();

  EXPECT_TRUE(searching);
  EXPECT_FALSE(overlapped);
}

TEST_F(TestVideoInfoScannerPython, AMovieIsLookedUpByTheIdentifierInItsFolderName)
{
  ASSERT_TRUE(m_db.ExecuteQuery(
      m_db.PrepareSQL("UPDATE path SET useFolderNames=1 WHERE idPath=%i", m_db.GetPathId(m_root))));
  File(Folder(m_root, "Film (2010) {tmdb=603}"), "movie.mkv");
  LetRunsEnd();
  // An identifier names one of the metadata sources the scan knows
  auto& sources{
      CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_videoScannerMetadataSources};
  const auto previous{sources};
  sources = {"tmdb", "imdb"};

  TestScanner scanner{m_settings};
  scanner.Scan(m_root);
  sources = previous;

  std::unique_lock lock(m_handler.m_runs.lock);
  EXPECT_TRUE(std::ranges::any_of(m_handler.m_runs.calls,
                                  [](const std::string& call)
                                  {
                                    return call.find("action=getdetails") != std::string::npos &&
                                           call.find("603") != std::string::npos;
                                  }));
}

namespace
{
//! Answers as a movie scraper: a search finds the title searched for, with details of that title.
//! Asked for anything else, such as the url in an nfo, it finds nothing.
void AnswerAsAMovieScraper(int handle, const CURL& call)
{
  const std::string action{call.GetOption("action")};
  if (action == "getdetails")
  {
    CFileItem movie{call.GetOption("url"), false};
    movie.GetVideoInfoTag()->SetTitle(call.GetOption("url"));
    XFILE::CPluginDirectory::SetResolvedUrl(handle, true, &movie);
    return;
  }
  if (action == "find")
  {
    CFileItem movie{call.GetOption("title"), false};
    movie.SetLabel(call.GetOption("title"));
    XFILE::CPluginDirectory::AddItem(handle, &movie, 1);
  }
  XFILE::CPluginDirectory::EndOfDirectory(handle, true, false, false);
}

bool Eventually(const std::function<bool()>& condition)
{
  for (int i = 0; i < 500 && !condition(); ++i)
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  return condition();
}
} // namespace

// A scan stopped while the next batch is looked up, the batch before it added, ends, keeping the
// movies added and the records of their folders, and recording none after them
TEST_F(TestVideoInfoScannerPython, AScanStoppedBetweenBatchesKeepsTheFoldersAddedBeforeIt)
{
  // Only Gamma, the first of the second batch, is searched for, as the others have full nfos, so
  // that its search, which holds the scraper, holds up no other
  std::vector<std::string> folders;
  for (const std::string name : {"Alpha", "Beta", "Gamma", "Kappa"})
  {
    folders.push_back(Folder(m_root, name + " (2001)"));
    if (name == "Gamma")
      File(folders.back(), name + " (2001).mkv");
    else
      FolderMovie(folders.back(), name + " (2001).mkv");
  }
  // Gamma isn't found until the test lets it. Shared, as its call may end after the test.
  struct SSearch
  {
    std::mutex lock;
    std::condition_variable changed;
    bool searching{false};
    bool answer{false};
  };
  const auto gamma{std::make_shared<SSearch>()};
  {
    std::unique_lock guard(m_handler.m_runs.lock);
    m_handler.m_runs.answer = [gamma](int handle, const CURL& call)
    {
      if (call.GetOption("title") == "Gamma")
      {
        std::unique_lock search(gamma->lock);
        gamma->searching = true;
        gamma->changed.notify_all();
        gamma->changed.wait_for(search, std::chrono::seconds(30),
                                [&gamma] { return gamma->answer; });
      }
      AnswerAsAMovieScraper(handle, call);
    };
  }
  LetRunsEnd();

  TestScanner scanner{m_settings};
  scanner.LimitFoldersLookedUpTogether(2);
  std::future<void> scan{
      std::async(std::launch::async, [this, &scanner] { scanner.Scan(m_root); })};
  bool between{false};
  {
    std::unique_lock guard(gamma->lock);
    between = gamma->changed.wait_for(guard, std::chrono::seconds(10),
                                      [&gamma] { return gamma->searching; });
  }
  between =
      between && Eventually([this, &folders] { return Record(folders[1]).starts_with("2|"); });

  scanner.Stop();
  const bool stopped{scan.wait_for(std::chrono::seconds(10)) == std::future_status::ready};
  {
    std::unique_lock guard(gamma->lock);
    gamma->answer = true;
  }
  gamma->changed.notify_all();
  scan.wait();

  EXPECT_TRUE(between);
  EXPECT_TRUE(stopped);
  EXPECT_EQ(2, CountMovies());
  EXPECT_TRUE(Record(folders[0]).starts_with("2|"));
  EXPECT_TRUE(Record(folders[1]).starts_with("2|"));
  EXPECT_FALSE(Record(folders[2]).starts_with("2|"));
  EXPECT_FALSE(Record(folders[3]).starts_with("2|"));
  EXPECT_FALSE(Record(m_root).starts_with("2|"));
}

// A scan's scraper calls keep to one interpreter, those that wait for no turn too: an episode's nfo,
// read once its show has been looked up, is asked for the url it holds on the show's interpreter
TEST_F(TestVideoInfoScannerPythonShows, AnEpisodesNfoIsAskedForItsUrlOnTheScansInterpreter)
{
  const std::string season{Folder(Folder(m_root, "Show"), "Season 1")};
  File(season, "Show S01E01.mkv");
  File(season, "Show S01E01.nfo", "<episodedetails><title>First</title></episodedetails>");
  {
    std::unique_lock lock(m_handler.m_runs.lock);
    m_handler.m_runs.answer = AnswerAsAShowScraper;
  }
  LetRunsEnd();

  TestScanner scanner{m_settings};
  scanner.Scan(m_root);

  ASSERT_EQ(1u, Calls({"action=NfoUrl"}));
  std::unique_lock lock(m_handler.m_runs.lock);
  EXPECT_EQ(1u, m_handler.m_runs.threads.size());
}
