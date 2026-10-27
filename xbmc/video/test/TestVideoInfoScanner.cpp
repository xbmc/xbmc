/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FileItem.h"
#include "FileItemList.h"
#include "MediaSource.h"
#include "ServiceBroker.h"
#include "TextureCache.h"
#include "addons/AddonManager.h"
#include "addons/AddonVersion.h"
#include "addons/IAddon.h"
#include "addons/Scraper.h"
#include "addons/addoninfo/AddonInfo.h"
#include "addons/addoninfo/AddonInfoBuilder.h"
#include "addons/addoninfo/AddonType.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "filesystem/SpecialProtocol.h"
#include "interfaces/AnnouncementManager.h"
#include "jobs/JobManager.h"
#include "settings/AdvancedSettings.h"
#include "settings/MediaSourceSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/Artwork.h"
#include "utils/URIUtils.h"
#include "video/SetInfoTag.h"
#include "video/VideoDatabase.h"
#include "video/VideoInfoScanner.h"
#include "video/VideoInfoTag.h"
#include "video/VideoManagerTypes.h"
#include "video/jobs/VideoLibraryRefreshingJob.h"
#include "video/jobs/VideoLibraryScanningJob.h"

#include <chrono>
#include <filesystem>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

using namespace KODI::VIDEO;

namespace
{
constexpr const char* DB_NAME = "TestVideoInfoScanner";

constexpr const char* SCRAPER_ID = "metadata.test.videoinfoscanner";

//! A movie and tv show scraper whose search results, details and episode guides are files a test
//! writes. Each url it gives names a cache file, which is read in place of fetching the url.
constexpr const char* SCRAPER_ADDON = R"(<?xml version="1.0" encoding="UTF-8"?>
<addon id="metadata.test.videoinfoscanner" name="Test scraper" version="1.0.0"
       provider-name="Team Kodi">
  <requires>
    <import addon="xbmc.metadata" version="2.1.0"/>
  </requires>
  <extension point="xbmc.metadata.scraper.movies" library="scraper.xml"
             cachepersistence="01:00"/>
  <extension point="xbmc.metadata.scraper.tvshows" library="scraper.xml"
             cachepersistence="01:00"/>
  <extension point="xbmc.addon.metadata">
    <platform>all</platform>
  </extension>
</addon>)";

//! Searching for a title reads search-<title>.xml, and each other function returns the content of
//! the file its url names
constexpr const char* SCRAPER_XML = R"(<?xml version="1.0" encoding="UTF-8"?>
<scraper framework="1.1">
  <CreateSearchUrl dest="3">
    <RegExp input="$$1" output="&lt;url cache=&quot;search-$$1.xml&quot;&gt;http://scraper.invalid/$$1&lt;/url&gt;" dest="3">
      <expression/>
    </RegExp>
  </CreateSearchUrl>
  <GetSearchResults dest="3">
    <RegExp input="$$1" output="$$1" dest="3"><expression/></RegExp>
  </GetSearchResults>
  <GetDetails dest="3">
    <RegExp input="$$1" output="$$1" dest="3"><expression/></RegExp>
  </GetDetails>
  <GetEpisodeList dest="3">
    <RegExp input="$$1" output="$$1" dest="3"><expression/></RegExp>
  </GetEpisodeList>
  <GetEpisodeDetails dest="3">
    <RegExp input="$$1" output="$$1" dest="3"><expression/></RegExp>
  </GetEpisodeDetails>
</scraper>)";

constexpr const char* OTHER_SCRAPER_ID = "metadata.alternative.videoinfoscanner";

//! Another movie and tv show scraper, which the add-on manager is given. It recognises a url to
//! other.invalid, whose content is read from other-<id>.xml. Its id sorts before metadata.local,
//! where the search for an nfo's url once stopped, so that it would be asked.
constexpr const char* OTHER_SCRAPER_ADDON = R"(<?xml version="1.0" encoding="UTF-8"?>
<addon id="metadata.alternative.videoinfoscanner" name="Other test scraper" version="1.0.0"
       provider-name="Team Kodi">
  <requires>
    <import addon="xbmc.metadata" version="2.1.0"/>
  </requires>
  <extension point="xbmc.metadata.scraper.movies" library="scraper.xml"/>
  <extension point="xbmc.metadata.scraper.tvshows" library="scraper.xml"/>
  <extension point="xbmc.addon.metadata">
    <platform>all</platform>
  </extension>
</addon>)";

constexpr const char* OTHER_SCRAPER_XML = R"(<?xml version="1.0" encoding="UTF-8"?>
<scraper framework="1.1">
  <NfoUrl dest="3">
    <RegExp input="$$1" output="&lt;url cache=&quot;other-\1.xml&quot;&gt;http://other.invalid/\1&lt;/url&gt;" dest="3">
      <expression>other\.invalid/([0-9]+)</expression>
    </RegExp>
  </NfoUrl>
  <CreateSearchUrl dest="3">
    <RegExp input="$$1" output="&lt;url&gt;http://other.invalid/search&lt;/url&gt;" dest="3">
      <expression/>
    </RegExp>
  </CreateSearchUrl>
  <GetEpisodeList dest="3">
    <RegExp input="$$1" output="$$1" dest="3"><expression/></RegExp>
  </GetEpisodeList>
</scraper>)";

//! A refresh without its GUI, of the test's database
class TestRefreshingJob : public CVideoLibraryRefreshingJob
{
public:
  using CVideoLibraryRefreshingJob::CVideoLibraryRefreshingJob;
  using CVideoLibraryRefreshingJob::Work;
};

//! A scan without its GUI: Start() needs one, so the folders are scanned with DoScan()
class TestScanner : public CVideoInfoScanner
{
public:
  explicit TestScanner(const DatabaseSettings& settings,
                       bool ignoreVideoExtras = false,
                       bool clean = false)
  {
    m_database.Connect(DB_NAME, settings, false);
    m_similarVideoAction = SimilarVideoScanAction::NONE;
    m_ignoreVideoExtras = ignoreVideoExtras;
    m_bClean = clean;
  }

  //! Looks the videos up with this scraper, which the add-on manager doesn't know of. The path's
  //! scraper is looked for here before it is asked.
  void UseScraper(const ADDON::ScraperPtr& scraper)
  {
    m_scraperCache.try_emplace(scraper->ID() + ADDON::TranslateContent(scraper->Content()),
                               scraper);
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

  using CVideoInfoScanner::EnumerateSeriesFolder;
  using EpisodeResult = CVideoInfoScanner::EpisodeResult;

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

    m_settings.type = "sqlite3";
    m_settings.name = DB_NAME;
    m_settings.host = CSpecialProtocol::TranslatePath("special://temp/");
    ASSERT_EQ(CDatabase::ConnectionState::STATE_CONNECTED, m_db.Connect(DB_NAME, m_settings, true));

    // The test environment has no announcement manager. An unstarted one only queues them.
    m_previousAnnouncementManager = CServiceBroker::GetAnnouncementManager();
    CServiceBroker::RegisterAnnouncementManager(
        std::make_shared<ANNOUNCEMENT::CAnnouncementManager>());

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
    CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_bVideoScannerIgnoreErrors =
        m_ignoreErrors;
    if (!m_installedScraper.empty())
    {
      CServiceBroker::GetAddonMgr().UnloadAddon(SCRAPER_ID);
      XFILE::CDirectory::RemoveRecursive(m_installedScraper);
    }
    if (!m_otherScraper.empty())
    {
      CServiceBroker::GetAddonMgr().UnloadAddon(OTHER_SCRAPER_ID);
      XFILE::CDirectory::RemoveRecursive(m_otherScraper);
      XFILE::CDirectory::RemoveRecursive(ScraperCacheFolder(OTHER_SCRAPER_ID));
    }
    if (m_scraperInfo)
    {
      CServiceBroker::GetSettingsComponent()->GetSettings()->SetBool(
          CSettings::SETTING_EVENTLOG_ENABLED, m_eventLog);
      XFILE::CDirectory::RemoveRecursive(m_scraperInfo->Path());
      XFILE::CDirectory::RemoveRecursive(ScraperCacheFolder());
    }

    CServiceBroker::UnregisterAnnouncementManager();
    if (m_previousAnnouncementManager)
      CServiceBroker::RegisterAnnouncementManager(m_previousAnnouncementManager);

    m_db.Close();
    XFILE::CFile::Delete(m_settings.host + DB_NAME + ".db");
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

  void Scan(bool ignoreVideoExtras = false, bool clean = false, const std::string& path = {})
  {
    TestScanner scanner{m_settings, ignoreVideoExtras, clean};
    if (m_scraperInfo)
      scanner.UseScraper(std::make_shared<ADDON::CScraper>(m_scraperInfo, m_scraperType));
    scanner.Scan(path.empty() ? m_root : path);
  }

  //! Looks the source's videos up with the test scraper, rather than reading them only from their
  //! nfos. A failed lookup is passed over, as it would otherwise ask whether to carry on.
  void UseTestScraper(ADDON::AddonType type)
  {
    const std::string addon{Folder(
        CSpecialProtocol::TranslatePath("special://temp/TestVideoInfoScanner/"), SCRAPER_ID)};
    File(addon, "addon.xml", SCRAPER_ADDON);
    File(addon, "scraper.xml", SCRAPER_XML);
    m_scraperInfo = ADDON::CAddonInfoBuilder::Generate(addon);
    ASSERT_TRUE(m_scraperInfo);
    m_scraperType = type;

    ASSERT_TRUE(m_db.ExecuteQuery(
        m_db.PrepareSQL("UPDATE path SET strScraper='%s', strSettings='' WHERE idPath=%i",
                        SCRAPER_ID, m_db.GetPathId(m_root))));
    CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_bVideoScannerIgnoreErrors =
        true;
    // A video with no information found is logged as an event, which is sent to the GUI a test
    // doesn't have
    m_eventLog = CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
        CSettings::SETTING_EVENTLOG_ENABLED);
    CServiceBroker::GetSettingsComponent()->GetSettings()->SetBool(
        CSettings::SETTING_EVENTLOG_ENABLED, false);
  }

  //! Installs the test scraper, as a refresh finds the scraper of a path through the add-on manager
  void InstallTestScraper()
  {
    const std::string addons{CSpecialProtocol::TranslatePath("special://home/addons/")};
    static_cast<void>(XFILE::CDirectory::Create(addons));
    m_installedScraper = Folder(addons, SCRAPER_ID);
    File(m_installedScraper, "addon.xml", SCRAPER_ADDON);
    File(m_installedScraper, "scraper.xml", SCRAPER_XML);
    ASSERT_TRUE(CServiceBroker::GetAddonMgr().FindAddon(SCRAPER_ID, ADDON::ORIGIN_SYSTEM,
                                                        ADDON::CAddonVersion{"1.0.0"}));
  }

  //! Installs the other scraper. Neither scraper can fetch the url it recognises, nor does the
  //! other one find an episode guide.
  void InstallOtherScraper()
  {
    const std::string addons{CSpecialProtocol::TranslatePath("special://home/addons/")};
    static_cast<void>(XFILE::CDirectory::Create(addons));
    m_otherScraper = Folder(addons, OTHER_SCRAPER_ID);
    File(m_otherScraper, "addon.xml", OTHER_SCRAPER_ADDON);
    File(m_otherScraper, "scraper.xml", OTHER_SCRAPER_XML);
    // Found rather than loaded, as loading publishes an event through the job manager a test
    // doesn't have
    ASSERT_TRUE(CServiceBroker::GetAddonMgr().FindAddon(OTHER_SCRAPER_ID, ADDON::ORIGIN_SYSTEM,
                                                        ADDON::CAddonVersion{"1.0.0"}));
    ScraperFile("guide.xml", "<error/>", OTHER_SCRAPER_ID);
    ScraperFile("other-1.xml", "<error/>", OTHER_SCRAPER_ID);
    ScraperFile("other-1.xml", "<error/>");
  }

  static std::string ScraperCacheFolder(const std::string& scraper = SCRAPER_ID)
  {
    return URIUtils::AddFileToFolder(
        CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_cachePath, "scrapers",
        scraper);
  }

  //! What a test scraper returns for a url naming this file
  static void ScraperFile(const std::string& name,
                          const std::string& content,
                          const std::string& scraper = SCRAPER_ID)
  {
    static_cast<void>(XFILE::CDirectory::Create(ScraperCacheFolder(scraper)));
    File(ScraperCacheFolder(scraper), name, content);
  }

  //! Searching for the title finds it, with the details in details-<title>.xml
  static void Found(const std::string& title)
  {
    ScraperFile("search-" + title + ".xml",
                "<results><entity><title>" + title + "</title><url cache=\"details-" + title +
                    ".xml\">http://scraper.invalid/details/" + title + "</url></entity></results>");
  }

  //! Searching for the title fails, as when the site can't be reached
  static void SearchFails(const std::string& title)
  {
    ScraperFile("search-" + title + ".xml", "<error><message>Search failed</message></error>");
  }

  //! The title's details, once found
  static void Details(const std::string& title, const std::string& details)
  {
    ScraperFile("details-" + title + ".xml", details);
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

  //! A refresh removes a movie before adding it again, so one whose details can't be read is
  //! kept. The movie has no nfo, or one that overrides some of the details it is looked up for.
  void RefreshThatCantReadTheDetails(bool overrideNfo)
  {
    UseTestScraper(ADDON::AddonType::SCRAPER_MOVIES);
    InstallTestScraper();
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
  CVideoDatabase m_db;
  std::shared_ptr<ANNOUNCEMENT::CAnnouncementManager> m_previousAnnouncementManager;
  const bool m_fastHash{
      CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_bVideoLibraryUseFastHash};
  const bool m_ignoreErrors{
      CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_bVideoScannerIgnoreErrors};
  ADDON::AddonInfoPtr m_scraperInfo;
  ADDON::AddonType m_scraperType{ADDON::AddonType::SCRAPER_MOVIES};
  bool m_eventLog{false};
  std::string m_otherScraper;
  std::string m_installedScraper;
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
  Scan();

  EXPECT_GT(m_db.GetMovieId(second), 0);
  EXPECT_EQ(2, CountMovies());
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

// Movies looked up with a scraper

TEST_F(TestVideoInfoScanner, AMovieIsAddedFromTheScraper)
{
  UseTestScraper(ADDON::AddonType::SCRAPER_MOVIES);
  Found("Film");
  Details("Film", "<details><title>Film from the scraper</title></details>");
  const std::string movie{File(m_root, "Film.mkv")};

  Scan();

  EXPECT_GT(m_db.GetMovieId(movie), 0);
  EXPECT_EQ("Film from the scraper", m_db.GetSingleValue("SELECT c00 FROM movie"));
}

TEST_F(TestVideoInfoScanner, AFailedSearchDoesNotStopTheScan)
{
  UseTestScraper(ADDON::AddonType::SCRAPER_MOVIES);
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
  UseTestScraper(ADDON::AddonType::SCRAPER_MOVIES);
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
  UseTestScraper(ADDON::AddonType::SCRAPER_MOVIES);
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

// A url is only taken from the scraper chosen for the path, which is the one it is fetched with
TEST_F(TestVideoInfoScanner, AMovieIsLookedUpWithItsPathsScraperWhenItsNfoNamesAnother)
{
  UseTestScraper(ADDON::AddonType::SCRAPER_MOVIES);
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
  Scan();

  EXPECT_GT(m_db.GetEpisodeId(second, 2, 1), 0);
  EXPECT_EQ(2, CountEpisodes());
}

TEST_F(TestVideoInfoScannerTvShows, AnUnchangedShowAddsNothingAgain)
{
  const std::string season{Folder(Show("Show"), "Season 1")};
  Episode(season, "Show S01E01.mkv");
  Episode(season, "Show S01E02.mkv");
  Scan();

  Scan();

  EXPECT_EQ(1, CountShows());
  EXPECT_EQ(2, CountEpisodes());
}

TEST_F(TestVideoInfoScannerTvShows, AFileOfTwoEpisodesIsTwoEpisodes)
{
  const std::string episodes{Episode(Folder(Show("Show"), "Season 1"), "Show S01E01E02.mkv")};

  Scan();

  EXPECT_GT(m_db.GetEpisodeId(episodes, 1, 1), 0);
  EXPECT_GT(m_db.GetEpisodeId(episodes, 2, 1), 0);
  EXPECT_EQ(1, m_db.GetSingleValueInt("SELECT COUNT(DISTINCT idFile) FROM episode"));
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
  UseTestScraper(ADDON::AddonType::SCRAPER_TVSHOWS);
  DetailsFail("Show");
  const std::string episode{Episode(Folder(Folder(m_root, "Show"), "Season 1"), "Show S01E01.mkv")};
  Scan();
  ASSERT_EQ(0, CountShows());
  EXPECT_EQ(0, CountEpisodes());

  Details("Show", "<details><title>Show</title></details>");
  Scan();

  EXPECT_EQ(1, CountShows());
  EXPECT_GT(m_db.GetEpisodeId(episode, 1, 1), 0);
}

TEST_F(TestVideoInfoScannerTvShows, AnEpisodeWithAnNfoIsAddedWhenTheEpisodeGuideFails)
{
  UseTestScraper(ADDON::AddonType::SCRAPER_TVSHOWS);
  const std::string show{Folder(m_root, "Show")};
  File(show, "tvshow.nfo",
       "<tvshow><title>Show</title><episodeguide><url cache=\"guide.xml\">"
       "http://scraper.invalid/guide</url></episodeguide></tvshow>");
  ScraperFile("guide.xml", "<error/>");
  const std::string season{Folder(show, "Season 1")};
  const std::string first{File(season, "Show S01E01.mkv")};
  const std::string second{Episode(season, "Show S01E02.mkv")};
  Scan();
  EXPECT_GT(m_db.GetEpisodeId(second, 2, 1), 0);
  ASSERT_EQ(1, CountEpisodes());

  ScraperFile("guide.xml", "<episodeguide><episode><title>First</title><url cache=\"first.xml\">"
                           "http://scraper.invalid/first</url><epnum>1</epnum><season>1</season>"
                           "</episode></episodeguide>");
  ScraperFile("first.xml", "<details><title>First</title></details>");
  Scan();

  EXPECT_GT(m_db.GetEpisodeId(first, 1, 1), 0);
  EXPECT_EQ(2, CountEpisodes());
}

TEST_F(TestVideoInfoScannerTvShows, AnEpisodeWhoseDetailsFailedIsLookedUpAgain)
{
  UseTestScraper(ADDON::AddonType::SCRAPER_TVSHOWS);
  const std::string show{Folder(m_root, "Show")};
  File(show, "tvshow.nfo",
       "<tvshow><title>Show</title><episodeguide><url cache=\"guide.xml\">"
       "http://scraper.invalid/guide</url></episodeguide></tvshow>");
  ScraperFile("guide.xml", "<episodeguide><episode><title>First</title><url cache=\"first.xml\">"
                           "http://scraper.invalid/first</url><epnum>1</epnum><season>1</season>"
                           "</episode></episodeguide>");
  ScraperFile("first.xml", "<error/>");
  const std::string season{Folder(show, "Season 1")};
  const std::string first{File(season, "Show S01E01.mkv")};
  const std::string second{Episode(season, "Show S01E02.mkv")};
  Scan();
  EXPECT_GT(m_db.GetEpisodeId(second, 2, 1), 0);
  ASSERT_EQ(1, CountEpisodes());

  ScraperFile("first.xml", "<details><title>First</title></details>");
  Scan();

  EXPECT_GT(m_db.GetEpisodeId(first, 1, 1), 0);
  EXPECT_EQ(2, CountEpisodes());
}

// A show's episodes are looked up in the episode guide stored when it was added, which only the
// scraper that added it understands
TEST_F(TestVideoInfoScannerTvShows, AnExistingShowKeepsItsScraperWhenItsNfoNamesAnother)
{
  UseTestScraper(ADDON::AddonType::SCRAPER_TVSHOWS);
  InstallOtherScraper();
  const std::string show{Folder(m_root, "Show")};
  File(show, "tvshow.nfo",
       "<tvshow><title>Show</title><episodeguide><url cache=\"guide.xml\">"
       "http://scraper.invalid/guide</url></episodeguide></tvshow>");
  ScraperFile("guide.xml",
              "<episodeguide><episode><title>First</title><url cache=\"first.xml\">"
              "http://scraper.invalid/first</url><epnum>1</epnum><season>1</season></episode>"
              "<episode><title>Second</title><url cache=\"second.xml\">"
              "http://scraper.invalid/second</url><epnum>2</epnum><season>1</season></episode>"
              "</episodeguide>");
  ScraperFile("first.xml", "<details><title>First</title></details>");
  ScraperFile("second.xml", "<details><title>Second</title></details>");
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
