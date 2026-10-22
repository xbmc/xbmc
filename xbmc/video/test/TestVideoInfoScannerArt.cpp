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
#include "video/VideoManagerTypes.h"

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

  //! An nfo, as exported, holding a version for each of playlists 800 and 801
  static std::string TwoVersionsNfo()
  {
    // With the streamdetails an export writes, which spare reading the playlist from the disc
    const std::string streamDetails{"<fileinfo><streamdetails><video><codec>h264</codec>"
                                    "<durationinseconds>6000</durationinseconds></video>"
                                    "</streamdetails></fileinfo>"};
    return "<movies>"
           "<movie><title>M3GAN</title><playlist>800</playlist>"
           "<hasvideoversions>true</hasvideoversions>" +
           streamDetails +
           "</movie>"
           "<movie><title>M3GAN</title><playlist>801</playlist>"
           "<videoassettitle>Extended</videoassettitle>" +
           streamDetails +
           "</movie>"
           "</movies>";
  }

  //! An nfo, as exported, holding the movie (playlist 800) and an extra of it (playlist 805)
  static std::string MovieAndExtraNfo()
  {
    const std::string streamDetails{"<fileinfo><streamdetails><video><codec>h264</codec>"
                                    "<durationinseconds>600</durationinseconds></video>"
                                    "</streamdetails></fileinfo>"};
    return "<movies>"
           "<movie><title>M3GAN</title><playlist>800</playlist>" +
           streamDetails +
           "</movie>"
           "<movie><title>M3GAN</title><playlist>805</playlist>"
           "<videoassettitle>Featurette</videoassettitle><videoassettype>2</videoassettype>" +
           streamDetails +
           "</movie>"
           "</movies>";
  }

  //! The art of the movie's extra
  std::string ExtraArt(const std::string& type)
  {
    return m_db.GetSingleValue(m_db.PrepareSQL(
        "SELECT art.url FROM art JOIN videoversion ON art.media_id = videoversion.idFile "
        "WHERE art.media_type='videoversion' AND art.type='%s' AND videoversion.itemType=%i",
        type.c_str(), static_cast<int>(VideoAssetType::EXTRA)));
  }

  //! A bluray folder rip of two versions, in the movie's folder or in a folder of its own there
  //! \return the folder holding BDMV
  std::string BlurayRipWithTwoVersions(const std::string& discFolder = {})
  {
    std::string folder{Folder(m_root, "M3GAN")};
    if (!discFolder.empty())
      folder = Folder(folder, discFolder);
    const std::string bdmv{Folder(folder, "BDMV")};
    File(bdmv, "index.bdmv");
    File(bdmv, "MovieObject.bdmv");
    const std::string playlists{Folder(bdmv, "PLAYLIST")};
    File(playlists, "00800.mpls");
    File(playlists, "00801.mpls");
    File(Folder(bdmv, "STREAM"), "00000.m2ts");
    File(bdmv, "index.nfo", TwoVersionsNfo());
    return folder;
  }

  //! A bluray ISO of two versions, M3GAN.iso
  //! \return the folder holding the ISO
  std::string IsoWithTwoVersions()
  {
    const std::string folder{Folder(m_root, "M3GAN")};
    File(folder, "M3GAN.iso");
    File(folder, "M3GAN.nfo", TwoVersionsNfo());
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
  const bool m_retrieveAllArt{
      Settings()->GetBool(CSettings::SETTING_VIDEOLIBRARY_RETRIEVEALLARTONEXPORT)};
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

TEST_F(TestVideoInfoScannerArt, EachVersionOfABlurayRipInADiscFolderHasItsOwnArtBesideBDMV)
{
  const std::string folder{BlurayRipWithTwoVersions("disc 1")};
  const std::string poster800{File(folder, "index-00800-poster.jpg")};
  const std::string poster801{File(folder, "00801-poster.jpg")};

  Scan();

  ASSERT_EQ(1, m_db.GetSingleValueInt("SELECT COUNT(*) FROM movie"));
  EXPECT_EQ(poster800, MovieArt("poster"));
  EXPECT_EQ(poster801, VersionArt("poster"));
}

// A test's ISO can't be read as a disc, so its second version isn't added. The first, the movie,
// is the version of playlist 800 all the same.
TEST_F(TestVideoInfoScannerArt, AVersionOfAnIsoHasItsOwnArtBesideIt)
{
  const std::string folder{IsoWithTwoVersions()};
  const std::string poster800{File(folder, "M3GAN-00800-poster.jpg")};
  File(folder, "M3GAN-00801-poster.jpg");

  Scan();

  ASSERT_EQ(1, m_db.GetSingleValueInt("SELECT COUNT(*) FROM movie"));
  EXPECT_EQ(poster800, MovieArt("poster"));
}

// Several ISOs can share a folder, so the playlist alone can't say which one the art is for
TEST_F(TestVideoInfoScannerArt, AnIsosVersionsArtIsNotNamedByItsPlaylistAlone)
{
  const std::string folder{IsoWithTwoVersions()};
  File(folder, "00800-poster.jpg");

  Scan();

  ASSERT_EQ(1, m_db.GetSingleValueInt("SELECT COUNT(*) FROM movie"));
  EXPECT_EQ("", MovieArt("poster"));
}

// With movies not in folders of their own, several ISOs can share one: each has only the art
// named after it, and none named by a playlist alone, nor the folder's plain art
TEST_F(TestVideoInfoScannerArt, EachOfSeveralIsosInAFolderHasOnlyTheArtNamedAfterIt)
{
  ASSERT_TRUE(m_db.ExecuteQuery(
      m_db.PrepareSQL("UPDATE path SET useFolderNames=0 WHERE idPath=%i", m_db.GetPathId(m_root))));
  // A and B have versions (of which a test's ISO gives only the first), C has none
  const std::string streamDetails{"<fileinfo><streamdetails><video><codec>h264</codec>"
                                  "<durationinseconds>6000</durationinseconds></video>"
                                  "</streamdetails></fileinfo>"};
  for (const std::string title : {"A", "B"})
  {
    File(m_root, title + ".iso");
    File(m_root, title + ".nfo",
         "<movies><movie><title>" + title +
             "</title><playlist>800</playlist><hasvideoversions>true</hasvideoversions>" +
             streamDetails + "</movie><movie><title>" + title + "</title><playlist>801</playlist>" +
             streamDetails + "</movie></movies>");
  }
  File(m_root, "C.iso");
  File(m_root, "C.nfo", "<movie><title>C</title></movie>");
  File(m_root, "D.iso");
  File(m_root, "D.nfo", "<movie><title>D</title></movie>");

  const std::string posterA{File(m_root, "A-00800-poster.jpg")};
  const std::string posterB{File(m_root, "B-00800-poster.jpg")};
  const std::string posterC{File(m_root, "C-poster.jpg")};
  File(m_root, "00800-poster.jpg");
  File(m_root, "poster.jpg");

  Scan();

  const auto posterOf = [this](const std::string& title)
  {
    return m_db.GetSingleValue(
        m_db.PrepareSQL("SELECT art.url FROM art JOIN movie ON art.media_id = movie.idMovie "
                        "WHERE art.media_type='movie' AND art.type='poster' AND movie.c00='%s'",
                        title.c_str()));
  };
  ASSERT_EQ(4, m_db.GetSingleValueInt("SELECT COUNT(*) FROM movie"));
  EXPECT_EQ(posterA, posterOf("A"));
  EXPECT_EQ(posterB, posterOf("B"));
  EXPECT_EQ(posterC, posterOf("C"));
  EXPECT_EQ("", posterOf("D"));
}

// A disc without versions has the plain art beside BDMV, and none named for a version
TEST_F(TestVideoInfoScannerArt, ABlurayRipWithoutVersionsHasThePlainArtBesideBDMV)
{
  const std::string folder{Folder(m_root, "M3GAN")};
  const std::string bdmv{Folder(folder, "BDMV")};
  File(bdmv, "index.bdmv");
  File(bdmv, "index.nfo", "<movie><title>M3GAN</title></movie>");
  const std::string poster{File(folder, "poster.jpg")};
  File(folder, "index-00800-poster.jpg");
  File(folder, "00800-fanart.jpg");

  Scan();

  ASSERT_EQ(1, m_db.GetSingleValueInt("SELECT COUNT(*) FROM movie"));
  EXPECT_EQ(poster, MovieArt("poster"));
  EXPECT_EQ("", MovieArt("fanart"));
}

TEST_F(TestVideoInfoScannerArt, AVersionOfAnIsoInADiscFolderHasItsOwnArtBesideIt)
{
  const std::string folder{Folder(Folder(m_root, "M3GAN"), "disc 1")};
  File(folder, "M3GAN.iso");
  File(folder, "M3GAN.nfo", TwoVersionsNfo());
  const std::string poster800{File(folder, "M3GAN-00800-poster.jpg")};

  Scan();

  ASSERT_EQ(1, m_db.GetSingleValueInt("SELECT COUNT(*) FROM movie"));
  EXPECT_EQ(poster800, MovieArt("poster"));
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

// An extra of the movie on its disc, such as a featurette, has its own art beside BDMV, but not
// the plain art there, which is the movie's
TEST_F(TestVideoInfoScannerArt, AnExtraOfABlurayRipHasOnlyItsOwnArt)
{
  const std::string folder{Folder(m_root, "M3GAN")};
  const std::string bdmv{Folder(folder, "BDMV")};
  File(bdmv, "index.bdmv");
  File(bdmv, "MovieObject.bdmv");
  const std::string playlists{Folder(bdmv, "PLAYLIST")};
  File(playlists, "00800.mpls");
  File(playlists, "00805.mpls");
  File(Folder(bdmv, "STREAM"), "00000.m2ts");
  File(bdmv, "index.nfo", MovieAndExtraNfo());
  const std::string poster{File(folder, "poster.jpg")};
  const std::string fanart{File(folder, "fanart.jpg")};
  const std::string extraPoster{File(folder, "index-00805-poster.jpg")};

  Scan();

  ASSERT_EQ(1, m_db.GetSingleValueInt("SELECT COUNT(*) FROM movie"));
  EXPECT_EQ(poster, MovieArt("poster"));
  EXPECT_EQ(fanart, MovieArt("fanart"));
  EXPECT_EQ(extraPoster, ExtraArt("poster"));
  EXPECT_EQ("", ExtraArt("fanart"));
}

// A movie whose disc holds an extra but no other version is exported with the plain art a scan
// reads, and the extra with art named for its playlist, so each has its own art again when the
// library is scanned anew from the export
TEST_F(TestVideoInfoScannerArt, AMovieAndItsExtraOnADiscHaveTheirArtAgainAfterAnExport)
{
  const std::string folder{Folder(m_root, "M3GAN")};
  const std::string bdmv{Folder(folder, "BDMV")};
  File(bdmv, "index.bdmv");
  File(bdmv, "MovieObject.bdmv");
  const std::string playlists{Folder(bdmv, "PLAYLIST")};
  File(playlists, "00800.mpls");
  File(playlists, "00805.mpls");
  File(Folder(bdmv, "STREAM"), "00000.m2ts");
  File(bdmv, "index.nfo", MovieAndExtraNfo());
  Scan();
  ASSERT_EQ(1, m_db.GetSingleValueInt("SELECT COUNT(*) FROM movie"));
  const int idMovie{m_db.GetSingleValueInt("SELECT idMovie FROM movie")};
  const int idExtra{
      m_db.GetSingleValueInt(m_db.PrepareSQL("SELECT idFile FROM videoversion WHERE itemType=%i",
                                             static_cast<int>(VideoAssetType::EXTRA)))};
  ASSERT_GT(idExtra, 0);

  // Art chosen for each from elsewhere: a 2x2 image
  const std::vector<unsigned char> pngBytes{
      0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x0d, 0x49, 0x48, 0x44,
      0x52, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x02, 0x08, 0x02, 0x00, 0x00, 0x00, 0xfd,
      0xd4, 0x9a, 0x73, 0x00, 0x00, 0x00, 0x0f, 0x49, 0x44, 0x41, 0x54, 0x78, 0x9c, 0x63, 0x60,
      0x60, 0xf8, 0x0f, 0x46, 0x60, 0x0a, 0x00, 0x17, 0xf6, 0x03, 0xfd, 0x7e, 0xeb, 0x37, 0xf3,
      0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4e, 0x44, 0xae, 0x42, 0x60, 0x82};
  const std::string png(pngBytes.begin(), pngBytes.end());
  const std::string images{Folder(m_root, "images")};
  ASSERT_TRUE(m_db.SetArtForItem(idMovie, KODI::MEDIA::TYPE::MOVIE, "poster",
                                 File(images, "movie.png", png)));
  ASSERT_TRUE(m_db.SetArtForItem(idExtra, KODI::MEDIA::TYPE::VIDEO_VERSION, "poster",
                                 File(images, "extra.png", png)));

  Settings()->SetBool(CSettings::SETTING_VIDEOLIBRARY_RETRIEVEALLARTONEXPORT, true);
  m_db.ExportToXML(m_root, false, true, false, true);
  Settings()->SetBool(CSettings::SETTING_VIDEOLIBRARY_RETRIEVEALLARTONEXPORT, m_retrieveAllArt);

  // The image exported, as a .png copied or a .jpg cached
  const auto exported = [&folder](const std::string& name)
  {
    for (const char* extension : {".png", ".jpg"})
    {
      if (const std::string image{URIUtils::AddFileToFolder(folder, name + extension)};
          XFILE::CFile::Exists(image))
        return image;
    }
    return std::string{};
  };
  const std::string moviePoster{exported("poster")};
  const std::string extraPoster{exported("index-00805-poster")};
  EXPECT_NE("", moviePoster);
  EXPECT_NE("", extraPoster);
  EXPECT_EQ("", exported("index-00800-poster"));

  // The library scanned anew, from the exported nfo and art
  ASSERT_TRUE(m_db.DeleteMovie(idMovie));
  XFILE::CDirectory::RemoveRecursive(images);
  Scan();

  ASSERT_EQ(1, m_db.GetSingleValueInt("SELECT COUNT(*) FROM movie"));
  EXPECT_EQ(moviePoster, MovieArt("poster"));
  EXPECT_EQ(extraPoster, ExtraArt("poster"));
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

  //! A show holding a dvd folder rip of episodes 3 and 4 of season 1, read from the disc's nfo
  //! \return the folder holding VIDEO_TS
  std::string DvdRipOfTwoEpisodes()
  {
    const std::string show{Folder(m_root, "Show")};
    File(show, "tvshow.nfo", "<tvshow><title>Show</title></tvshow>");
    const std::string folder{Folder(show, "Show.S01E03E04")};
    File(Folder(folder, "VIDEO_TS"), "VIDEO_TS.IFO");
    File(URIUtils::AddFileToFolder(folder, "VIDEO_TS"), "VIDEO_TS.nfo",
         "<episodedetails><title>Third</title><season>1</season><episode>3</episode>"
         "</episodedetails>"
         "<episodedetails><title>Fourth</title><season>1</season><episode>4</episode>"
         "</episodedetails>");
    return folder;
  }

  //! A show holding an ISO of episodes 5 and 6 of season 1, read from the ISO's nfo
  //! \return the folder holding the ISO
  std::string IsoOfTwoEpisodes()
  {
    const std::string show{Folder(m_root, "Show")};
    File(show, "tvshow.nfo", "<tvshow><title>Show</title></tvshow>");
    File(show, "Show.S01E05E06.iso");
    File(show, "Show.S01E05E06.nfo",
         "<episodedetails><title>Fifth</title><season>1</season><episode>5</episode>"
         "</episodedetails>"
         "<episodedetails><title>Sixth</title><season>1</season><episode>6</episode>"
         "</episodedetails>");
    return show;
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

TEST_F(TestVideoInfoScannerArtTvShows, EachEpisodeOfADvdRipHasItsOwnArtBesideVIDEO_TS)
{
  const std::string folder{DvdRipOfTwoEpisodes()};
  const std::string thumb3{File(folder, "VIDEO_TS-S01E03-thumb.jpg")};
  const std::string thumb4{File(folder, "S01E04-thumb.jpg")};
  File(folder, "thumb.jpg");

  Scan();

  ASSERT_EQ(2, m_db.GetSingleValueInt("SELECT COUNT(*) FROM episode"));
  EXPECT_EQ(thumb3, EpisodeArt(3, "thumb"));
  EXPECT_EQ(thumb4, EpisodeArt(4, "thumb"));
}

// Several ISOs can share a folder, so the episode alone can't say which one the art is for
TEST_F(TestVideoInfoScannerArtTvShows, EachEpisodeOfAnIsoHasItsOwnArtBesideIt)
{
  const std::string folder{IsoOfTwoEpisodes()};
  const std::string thumb5{File(folder, "Show.S01E05E06-S01E05-thumb.jpg")};
  File(folder, "S01E06-thumb.jpg");

  Scan();

  ASSERT_EQ(2, m_db.GetSingleValueInt("SELECT COUNT(*) FROM episode"));
  EXPECT_EQ(thumb5, EpisodeArt(5, "thumb"));
  EXPECT_EQ("", EpisodeArt(6, "thumb"));
}

// A disc of one episode has the plain art beside BDMV, as it always had
TEST_F(TestVideoInfoScannerArtTvShows, ABlurayRipOfOneEpisodeHasThePlainArtBesideBDMV)
{
  const std::string show{Folder(m_root, "Show")};
  File(show, "tvshow.nfo", "<tvshow><title>Show</title></tvshow>");
  const std::string folder{Folder(show, "Show.S01E09")};
  const std::string bdmv{Folder(folder, "BDMV")};
  File(bdmv, "index.bdmv");
  File(bdmv, "index.nfo",
       "<episodedetails><title>Ninth</title><season>1</season><episode>9</episode>"
       "</episodedetails>");
  const std::string thumb{File(folder, "thumb.jpg")};

  Scan();

  ASSERT_EQ(1, m_db.GetSingleValueInt("SELECT COUNT(*) FROM episode"));
  EXPECT_EQ(thumb, EpisodeArt(9, "thumb"));
}

// Episodes of several in a video file have the art named after the file and the episode, and
// none named by the episode alone, as several files can share the folder
TEST_F(TestVideoInfoScannerArtTvShows, EachEpisodeOfAVideoFileHasItsOwnArtBesideIt)
{
  const std::string show{Folder(m_root, "Show")};
  File(show, "tvshow.nfo", "<tvshow><title>Show</title></tvshow>");
  File(show, "Show.S01E07E08.mkv");
  File(show, "Show.S01E07E08.nfo",
       "<episodedetails><title>Seventh</title><season>1</season><episode>7</episode>"
       "</episodedetails>"
       "<episodedetails><title>Eighth</title><season>1</season><episode>8</episode>"
       "</episodedetails>");
  const std::string thumb7{File(show, "Show.S01E07E08-S01E07-thumb.jpg")};
  File(show, "S01E08-thumb.jpg");

  Scan();

  ASSERT_EQ(2, m_db.GetSingleValueInt("SELECT COUNT(*) FROM episode"));
  EXPECT_EQ(thumb7, EpisodeArt(7, "thumb"));
  EXPECT_EQ("", EpisodeArt(8, "thumb"));
}
