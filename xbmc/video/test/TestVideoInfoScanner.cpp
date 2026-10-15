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
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "filesystem/SpecialProtocol.h"
#include "interfaces/AnnouncementManager.h"
#include "settings/AdvancedSettings.h"
#include "settings/MediaSourceSettings.h"
#include "settings/SettingsComponent.h"
#include "utils/URIUtils.h"
#include "video/VideoDatabase.h"
#include "video/VideoInfoScanner.h"
#include "video/VideoManagerTypes.h"

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

  //! Moves a folder's modified time on, as adding or removing a file does, a second or more later
  static void Touch(const std::string& folder)
  {
    const std::filesystem::path path{
        std::u8string{reinterpret_cast<const char8_t*>(folder.data()), folder.size()}};
    std::filesystem::last_write_time(path, std::filesystem::last_write_time(path) +
                                               std::chrono::hours{1});
  }

  void Scan(bool ignoreVideoExtras = false, bool clean = false, const std::string& path = {})
  {
    TestScanner scanner{m_settings, ignoreVideoExtras, clean};
    scanner.Scan(path.empty() ? m_root : path);
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

  std::string m_root;
  DatabaseSettings m_settings;
  CVideoDatabase m_db;
  std::shared_ptr<ANNOUNCEMENT::CAnnouncementManager> m_previousAnnouncementManager;
  const bool m_fastHash{
      CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_bVideoLibraryUseFastHash};
};
} // namespace

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
