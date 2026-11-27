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
#include "ServiceBroker.h"
#include "URL.h"
#include "filesystem/Directory.h"
#include "filesystem/DirectoryCache.h"
#include "filesystem/File.h"
#include "filesystem/SpecialProtocol.h"
#include "interfaces/AnnouncementManager.h"
#include "utils/Artwork.h"
#include "utils/URIUtils.h"
#include "video/VideoDatabase.h"
#include "video/VideoInfoTag.h"
#include "video/VideoManagerTypes.h"

#include <memory>
#include <string>

#include <gtest/gtest.h>

namespace
{
class TestVideoDatabaseFile : public ::testing::Test
{
protected:
  void SetUp() override
  {
    XFILE::CDirectory::RemoveRecursive(m_root);
    ASSERT_TRUE(XFILE::CDirectory::Create(m_root));
    m_initialisedDatabases = !CServiceBroker::GetDatabaseManager().CanOpen("MyVideos");
    if (m_initialisedDatabases)
    {
      ASSERT_TRUE(CServiceBroker::GetDatabaseManager().Initialize());
    }
    ASSERT_TRUE(m_db.Open());

    // The test environment has no announcement manager, and DeleteMovie() announces.
    // An unstarted one only queues the announcements.
    m_previousAnnouncementManager = CServiceBroker::GetAnnouncementManager();
    CServiceBroker::RegisterAnnouncementManager(
        std::make_shared<ANNOUNCEMENT::CAnnouncementManager>());
  }

  void TearDown() override
  {
    if (m_idMovie > 0)
      m_db.DeleteMovie(m_idMovie);
    CServiceBroker::UnregisterAnnouncementManager();
    if (m_previousAnnouncementManager)
      CServiceBroker::RegisterAnnouncementManager(m_previousAnnouncementManager);
    m_db.Close();
    // Left as found: the scanner's tests rely on the library being unavailable
    if (m_initialisedDatabases)
      CServiceBroker::GetDatabaseManager().Deinitialize();
    XFILE::CDirectory::RemoveRecursive(m_root);
  }

  std::string NewFile(const std::string& name) const
  {
    const std::string file{URIUtils::AddFileToFolder(m_root, name)};
    XFILE::CFile out;
    EXPECT_TRUE(out.OpenForWrite(file, true));
    return file;
  }

  //! A movie whose own file is missing, with a version whose file exists
  //! \return the file id of the version
  int AddMovieWithVersion()
  {
    CVideoInfoTag movie;
    movie.m_strTitle = "Movie";
    movie.m_strFileNameAndPath = URIUtils::AddFileToFolder(m_root, "Movie.mkv");
    movie.m_strPath = m_root;
    m_idMovie = m_db.SetDetailsForMovie(movie, KODI::ART::Artwork{});
    EXPECT_GT(m_idMovie, 0);

    CFileItem version{NewFile("Movie Extended.mkv"), false};
    EXPECT_TRUE(m_db.AddVideoAsset(
        VideoDbContentType::MOVIES, m_idMovie,
        m_db.AddVideoVersionType("Extended", VideoAssetTypeOwner::USER, VideoAssetType::VERSION),
        VideoAssetType::VERSION, version));
    CVideoInfoTag tag;
    EXPECT_TRUE(m_db.GetFileInfo(version.GetPath(), tag));
    EXPECT_GT(tag.m_iFileId, 0);
    return tag.m_iFileId;
  }

  const std::string m_root{CSpecialProtocol::TranslatePath("special://temp/VideoDatabaseFile/")};
  CVideoDatabase m_db;
  bool m_initialisedDatabases{false};
  int m_idMovie{-1};
  std::shared_ptr<ANNOUNCEMENT::CAnnouncementManager> m_previousAnnouncementManager;
};
} // namespace

// The path of a version ends in its file id, not the movie's id
TEST_F(TestVideoDatabaseFile, VersionPathFindsItsOwnFile)
{
  const int idFile{AddMovieWithVersion()};
  ASSERT_GT(idFile, 0);

  const std::string path{"videodb://movies/titles/" + std::to_string(m_idMovie) + "/-2/" +
                         std::to_string(idFile) + "?assetType=-2"};
  EXPECT_TRUE(XFILE::CFile::Exists(path, false));
}

// A library listing is not kept in the directory cache
TEST_F(TestVideoDatabaseFile, LibraryListingIsNotCached)
{
  ASSERT_GT(AddMovieWithVersion(), 0);

  const std::string path{"videodb://movies/titles/" + std::to_string(m_idMovie) + "/-2/"};
  CFileItemList items;
  ASSERT_TRUE(XFILE::CDirectory::GetDirectory(path, items, "", XFILE::DIR_FLAG_DEFAULTS));
  ASSERT_FALSE(items.IsEmpty());

  bool inCache{true};
  g_directoryCache.FileExists(CURL(items[0]->GetPath()), inCache);
  EXPECT_FALSE(inCache);
}
