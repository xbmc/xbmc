/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "MediaSource.h"
#include "ServiceBroker.h"
#include "TextureCache.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "filesystem/SpecialProtocol.h"
#include "interfaces/AnnouncementManager.h"
#include "jobs/JobManager.h"
#include "settings/AdvancedSettings.h"
#include "settings/MediaSourceSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/URIUtils.h"
#include "video/VideoDatabase.h"
#include "video/VideoInfoScanner.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

using namespace KODI::VIDEO;

namespace
{
constexpr const char* DB_NAME = "TestVideoInfoScannerArt";

//! A scan without its GUI: Start() needs one, so the folders are scanned with DoScan()
class TestScanner : public CVideoInfoScanner
{
public:
  explicit TestScanner(const DatabaseSettings& settings)
  {
    m_database.Connect(DB_NAME, settings, false);
    m_similarVideoAction = SimilarVideoScanAction::NONE;
  }

  //! As Start() and Process() do
  void Scan(const std::string& directory)
  {
    m_pathsToScan.insert(directory);
    while (!m_pathsToScan.empty())
    {
      // A copy, as DoScan() first removes the path from the set
      const std::string folder{*m_pathsToScan.begin()};
      DoScan(folder);
      std::erase_if(m_pathsToScan,
                    [&folder](const std::string& path) { return path.starts_with(folder); });
    }
  }
};

class TestVideoInfoScannerArt : public ::testing::Test
{
protected:
  void SetUp() override
  {
    // A folder of each test's own, as the directory cache may still hold another test's listing
    m_root = URIUtils::AddFileToFolder(
        CSpecialProtocol::TranslatePath("special://temp/TestVideoInfoScannerArt/"),
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

    // The art a scan finds is cached, in a texture cache of the test's own
    m_previousJobManager = CServiceBroker::GetJobManager();
    CServiceBroker::RegisterJobManager(std::make_shared<CJobManager>());
    CServiceBroker::RegisterTextureCache(std::make_shared<CTextureCache>());
    // ..but not a thumbnail generated from the video
    Settings()->SetBool(CSettings::SETTING_MYVIDEOS_EXTRACTTHUMB, false);

    // A movie source using folder names, whose movies are read from their nfos
    CMediaSource source;
    source.FromNameAndPaths(m_root, {m_root});
    CMediaSourceSettings::GetInstance().GetSources("video")->push_back(source);
    ASSERT_TRUE(m_db.ExecuteQuery(m_db.PrepareSQL(
        "UPDATE path SET strContent='movies', strScraper='metadata.local', useFolderNames=1, "
        "scanRecursive=2147483647, noUpdate=0 WHERE idPath=%i",
        m_db.AddPath(m_root))));
  }

  void TearDown() override
  {
    std::erase_if(*CMediaSourceSettings::GetInstance().GetSources("video"),
                  [this](const CMediaSource& source) { return source.strPath == m_root; });
    Settings()->SetBool(CSettings::SETTING_MYVIDEOS_EXTRACTTHUMB, m_extractThumb);

    // First, as its jobs are cancelled with the job manager
    CServiceBroker::UnregisterTextureCache();
    CServiceBroker::GetJobManager()->CancelJobs();
    CServiceBroker::GetJobManager()->Restart();
    CServiceBroker::UnregisterJobManager();
    if (m_previousJobManager)
      CServiceBroker::RegisterJobManager(m_previousJobManager);

    CServiceBroker::UnregisterAnnouncementManager();
    if (m_previousAnnouncementManager)
      CServiceBroker::RegisterAnnouncementManager(m_previousAnnouncementManager);

    m_db.Close();
    XFILE::CFile::Delete(m_settings.host + DB_NAME + ".db");
    XFILE::CDirectory::RemoveRecursive(m_root);
  }

  static std::shared_ptr<CSettings> Settings()
  {
    return CServiceBroker::GetSettingsComponent()->GetSettings();
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

  //! A bluray folder rip whose nfo, as exported, holds a version for each of playlists 800 and 801
  //! \return the folder holding BDMV
  std::string BlurayRipWithTwoVersions()
  {
    const std::string folder{Folder(m_root, "M3GAN")};
    const std::string bdmv{Folder(folder, "BDMV")};
    File(bdmv, "index.bdmv");
    File(bdmv, "MovieObject.bdmv");
    const std::string playlists{Folder(bdmv, "PLAYLIST")};
    File(playlists, "00800.mpls");
    File(playlists, "00801.mpls");
    File(Folder(bdmv, "STREAM"), "00000.m2ts");
    // With the streamdetails an export writes, which spare reading the playlist from the disc
    const std::string streamDetails{"<fileinfo><streamdetails><video><codec>h264</codec>"
                                    "<durationinseconds>6000</durationinseconds></video>"
                                    "</streamdetails></fileinfo>"};
    File(bdmv, "index.nfo",
         "<movies>"
         "<movie><title>M3GAN</title><playlist>800</playlist>"
         "<hasvideoversions>true</hasvideoversions>" +
             streamDetails +
             "</movie>"
             "<movie><title>M3GAN</title><playlist>801</playlist>"
             "<videoassettitle>Extended</videoassettitle>" +
             streamDetails +
             "</movie>"
             "</movies>");
    return folder;
  }

  void Scan()
  {
    TestScanner scanner{m_settings};
    scanner.Scan(m_root);
  }

  //! The art of the movie, which is the version of playlist 800
  std::string MovieArt(const std::string& type)
  {
    return m_db.GetSingleValue(m_db.PrepareSQL(
        "SELECT url FROM art WHERE media_type='movie' AND type='%s'", type.c_str()));
  }

  //! The art of the movie's other version, of playlist 801
  std::string VersionArt(const std::string& type)
  {
    return m_db.GetSingleValue(m_db.PrepareSQL(
        "SELECT art.url FROM art "
        "  JOIN videoversion ON art.media_id = videoversion.idFile "
        "  JOIN movie ON videoversion.idMedia = movie.idMovie "
        "WHERE art.media_type='videoversion' AND art.type='%s' AND videoversion.idFile <> "
        "movie.idFile",
        type.c_str()));
  }

  std::string m_root;
  DatabaseSettings m_settings;
  CVideoDatabase m_db;
  std::shared_ptr<ANNOUNCEMENT::CAnnouncementManager> m_previousAnnouncementManager;
  std::shared_ptr<CJobManager> m_previousJobManager;
  const bool m_extractThumb{Settings()->GetBool(CSettings::SETTING_MYVIDEOS_EXTRACTTHUMB)};
};
} // namespace

TEST_F(TestVideoInfoScannerArt, EachVersionOfABlurayRipHasItsOwnArtBesideBDMV)
{
  const std::string folder{BlurayRipWithTwoVersions()};
  const std::string poster800{File(folder, "index-00800-poster.jpg")};
  const std::string poster801{File(folder, "index-00801-poster.jpg")};

  Scan();

  ASSERT_EQ(1, m_db.GetSingleValueInt("SELECT COUNT(*) FROM movie"));
  EXPECT_EQ(poster800, MovieArt("poster"));
  EXPECT_EQ(poster801, VersionArt("poster"));
}

TEST_F(TestVideoInfoScannerArt, AVersionsArtCanBeNamedByItsPlaylistAlone)
{
  const std::string folder{BlurayRipWithTwoVersions()};
  const std::string poster800{File(folder, "00800-poster.jpg")};
  const std::string poster801{File(folder, "00801-poster.jpg")};

  Scan();

  EXPECT_EQ(poster800, MovieArt("poster"));
  EXPECT_EQ(poster801, VersionArt("poster"));
}

TEST_F(TestVideoInfoScannerArt, AVersionsIndexNamedArtWinsOverAnyOther)
{
  const std::string folder{BlurayRipWithTwoVersions()};
  const std::string poster800{File(folder, "index-00800-poster.jpg")};
  File(folder, "00800-poster.jpg");
  File(folder, "poster.jpg");

  Scan();

  EXPECT_EQ(poster800, MovieArt("poster"));
}

TEST_F(TestVideoInfoScannerArt, AVersionsPlaylistNamedArtWinsOverThePlainArt)
{
  const std::string folder{BlurayRipWithTwoVersions()};
  const std::string poster801{File(folder, "00801-poster.jpg")};
  File(folder, "poster.jpg");

  Scan();

  EXPECT_EQ(poster801, VersionArt("poster"));
}

TEST_F(TestVideoInfoScannerArt, AVersionWithoutArtOfItsOwnHasThePlainArtBesideBDMV)
{
  const std::string folder{BlurayRipWithTwoVersions()};
  File(folder, "index-00800-poster.jpg");
  const std::string poster{File(folder, "poster.jpg")};
  const std::string fanart{File(folder, "fanart.jpg")};

  Scan();

  EXPECT_EQ(fanart, MovieArt("fanart"));
  EXPECT_EQ(poster, VersionArt("poster"));
  EXPECT_EQ(fanart, VersionArt("fanart"));
}

namespace
{
class TestVideoInfoScannerArtTvShows : public TestVideoInfoScannerArt
{
protected:
  void SetUp() override
  {
    TestVideoInfoScannerArt::SetUp();

    // A tv show source, each of whose folders holds a show
    ASSERT_TRUE(m_db.ExecuteQuery(m_db.PrepareSQL(
        "UPDATE path SET strContent='tvshows', useFolderNames=0, scanRecursive=0 WHERE idPath=%i",
        m_db.GetPathId(m_root))));
  }

  //! A show holding a bluray folder rip of episodes 1 and 2 of season 1, read from the disc's nfo
  //! \return the folder holding BDMV
  std::string BlurayRipOfTwoEpisodes()
  {
    const std::string show{Folder(m_root, "Show")};
    File(show, "tvshow.nfo", "<tvshow><title>Show</title></tvshow>");
    const std::string folder{Folder(show, "Show.S01E01E02")};
    const std::string bdmv{Folder(folder, "BDMV")};
    File(bdmv, "index.bdmv");
    File(bdmv, "MovieObject.bdmv");
    File(Folder(bdmv, "PLAYLIST"), "00000.mpls");
    File(Folder(bdmv, "STREAM"), "00000.m2ts");
    File(bdmv, "index.nfo",
         "<episodedetails><title>First</title><season>1</season><episode>1</episode>"
         "</episodedetails>"
         "<episodedetails><title>Second</title><season>1</season><episode>2</episode>"
         "</episodedetails>");
    return folder;
  }

  std::string EpisodeArt(int episode, const std::string& type)
  {
    return m_db.GetSingleValue(
        m_db.PrepareSQL("SELECT art.url FROM art JOIN episode ON art.media_id = episode.idEpisode "
                        "WHERE art.media_type='episode' AND art.type='%s' AND episode.c13='%i'",
                        type.c_str(), episode));
  }
};
} // namespace

TEST_F(TestVideoInfoScannerArtTvShows, EachEpisodeOfABlurayRipHasItsOwnArtBesideBDMV)
{
  const std::string folder{BlurayRipOfTwoEpisodes()};
  const std::string thumb1{File(folder, "index-S01E01-thumb.jpg")};
  const std::string thumb2{File(folder, "index-S01E02-thumb.jpg")};
  File(folder, "thumb.jpg");

  Scan();

  ASSERT_EQ(2, m_db.GetSingleValueInt("SELECT COUNT(*) FROM episode"));
  EXPECT_EQ(thumb1, EpisodeArt(1, "thumb"));
  EXPECT_EQ(thumb2, EpisodeArt(2, "thumb"));
}

TEST_F(TestVideoInfoScannerArtTvShows, AnEpisodeOfSeveralOnADiscDoesNotHaveThePlainArtBesideBDMV)
{
  const std::string folder{BlurayRipOfTwoEpisodes()};
  File(folder, "thumb.jpg");

  Scan();

  ASSERT_EQ(2, m_db.GetSingleValueInt("SELECT COUNT(*) FROM episode"));
  EXPECT_EQ("", EpisodeArt(1, "thumb"));
  EXPECT_EQ("", EpisodeArt(2, "thumb"));
}

TEST_F(TestVideoInfoScannerArtTvShows, AnEpisodesArtCanBeNamedByItsEpisodeAlone)
{
  const std::string folder{BlurayRipOfTwoEpisodes()};
  const std::string thumb1{File(folder, "S01E01-thumb.jpg")};
  File(folder, "S01E02-thumb.jpg");
  const std::string thumb2{File(folder, "index-S01E02-thumb.jpg")};

  Scan();

  EXPECT_EQ(thumb1, EpisodeArt(1, "thumb"));
  EXPECT_EQ(thumb2, EpisodeArt(2, "thumb"));
}
