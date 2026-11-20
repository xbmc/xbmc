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

#include <chrono>
#include <filesystem>
#include <fstream>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_set>
#include <utility>
#include <vector>

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

  //! As Start() and Process() do
  void Scan(const std::string& directory)
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

    if (m_bClean)
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

  using CVideoInfoScanner::EnumerateSeriesFolder;
  using CVideoInfoScanner::GetMovieIdentifier;
  using EpisodeResult = CVideoInfoScanner::EpisodeResult;
  using ScanSummary = CVideoInfoScanner::ScanSummary;

  const ScanSummary& Summary() const { return m_summary; }

  CVideoDatabase& Database() { return m_database; }
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
    std::string first;
    ASSERT_TRUE(m_db.GetPathHash(m_root, first));

    Scan();

    std::string second;
    ASSERT_TRUE(m_db.GetPathHash(m_root, second));
    EXPECT_EQ(first, second);
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
    const std::string featurettes{Folder(Folder(folder, "Extras"), "Featurettes")};
    File(featurettes, "Trailer.mkv");
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
  }

  std::string m_root;
  DatabaseSettings m_settings;
  std::optional<DatabaseSettings> m_videoDatabase;
  TestDatabase m_db;
  std::shared_ptr<ANNOUNCEMENT::CAnnouncementManager> m_previousAnnouncementManager;
  std::shared_ptr<XFILE::CBlurayDiscCache> m_previousDiscCache;
  const bool m_fastHash{
      CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_bVideoLibraryUseFastHash};
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

// The backup copy of the index is matched too by the date in its path, so it is passed over as a
// Blu-ray's is
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
  std::string root;
  ASSERT_TRUE(m_db.GetPathHash(m_root, root));
  ASSERT_FALSE(root.empty());

  ASSERT_TRUE(Refresh(tag));

  EXPECT_GT(m_db.GetMovieId(film), 0);
  std::string hash;
  EXPECT_TRUE(m_db.GetPathHash(m_root, hash));
  EXPECT_EQ(root, hash);
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

  std::string hash;
  EXPECT_TRUE(m_db.GetPathHash(extras, hash));
  EXPECT_FALSE(hash.empty());
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

#ifdef HAVE_LIBBLURAY
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

TEST_F(TestVideoInfoScannerTvShows, AFileOfTwoEpisodesIsTwoEpisodes)
{
  const std::string episodes{Episode(Folder(Show("Show"), "Season 1"), "Show S01E01E02.mkv")};

  Scan();

  EXPECT_GT(m_db.GetEpisodeId(episodes, 1, 1), 0);
  EXPECT_GT(m_db.GetEpisodeId(episodes, 2, 1), 0);
  EXPECT_EQ(1, m_db.GetSingleValueInt("SELECT COUNT(DISTINCT idFile) FROM episode"));
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

// Read file by file, the rip's index and movie object were each the episode, and its playlists and
// stream were specials
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
  std::string hash;
  ASSERT_TRUE(m_db.GetPathHash(show, hash));
  ASSERT_FALSE(hash.empty());

  CVideoInfoTag tag;
  ASSERT_TRUE(m_db.GetEpisodeInfo({}, tag, idEpisode));
  TestRefreshingJob refresh{std::make_shared<CFileItem>(tag), true, false};
  ASSERT_TRUE(refresh.Work(m_db));

  EXPECT_GT(m_db.GetEpisodeId(episode, 1, 1), 0);
  std::string after;
  EXPECT_TRUE(m_db.GetPathHash(show, after));
  EXPECT_EQ(hash, after);
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
