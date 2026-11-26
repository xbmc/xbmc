/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FileItem.h"
#include "FileItemList.h"
#include "test/TestLibrary.h"
#include "test/TestUtils.h"
#include "utils/URIUtils.h"
#include "video/VideoDatabase.h"
#include "video/VideoInfoScannerArt.h"
#include "video/VideoInfoScannerExtras.h"
#include "video/VideoInfoTag.h"
#include "video/VideoManagerTypes.h"
#include "video/test/VideoDatabaseTestBase.h"

#include <memory>
#include <string>

#include <gtest/gtest.h>

using KODI::VIDEO::FindExtraOnAnotherDisc;

// The same extra on two discs of a movie is found by its name and length, and one playing all of a
// kind by its length alone, as each disc may code the kind differently
TEST(TestVideoInfoScannerExtras, AnExtraOnAnotherDiscIsFound)
{
  const CScopedLanguageStrings strings;
  ASSERT_TRUE(strings.IsLoaded());

  const auto extra{[](const std::string& title, int seconds)
                   {
                     auto item{std::make_shared<CFileItem>("extra.mkv", false)};
                     item->GetVideoInfoTag()->GetAssetInfo().SetTitle(title);
                     item->GetVideoInfoTag()->SetDuration(seconds);
                     return item;
                   }};
  CFileItemList existing;
  existing.Add(extra("Gag Reel", 300));
  existing.Add(extra("DA (play all)", 1200));

  EXPECT_EQ(existing[0].get(), FindExtraOnAnotherDisc(existing, *extra("Gagreel", 301)));
  EXPECT_EQ(nullptr, FindExtraOnAnotherDisc(existing, *extra("Gag Reel", 310)));
  EXPECT_EQ(nullptr, FindExtraOnAnotherDisc(existing, *extra("Making Of", 300)));
  EXPECT_EQ(existing[1].get(), FindExtraOnAnotherDisc(existing, *extra("SE (play all)", 1201)));
  EXPECT_EQ(existing[1].get(), FindExtraOnAnotherDisc(existing, *extra("Play all", 1200)));
  EXPECT_EQ(nullptr, FindExtraOnAnotherDisc(existing, *extra("SE (play all)", 900)));
  EXPECT_EQ(nullptr, FindExtraOnAnotherDisc(existing, *extra("Deleted scenes", 1200)));
}

namespace
{
//! The extras the scanner adds to the movies of a library of the test's own
class TestVideoInfoScannerExtrasOfMovies : public VideoDatabaseTestBase
{
protected:
  TestVideoInfoScannerExtrasOfMovies()
    : VideoDatabaseTestBase("TestVideoInfoScannerExtras", "/movies/")
  {
  }

  int AddMovie(const std::string& fileAndPath) { return KODI::TEST::AddMovie(m_db, fileAndPath); }
};
} // namespace

// A movie restored from its nfos has only the extras they record, so a video without an nfo in its
// extras folder is left out during that scan, and added by a later one
TEST_F(TestVideoInfoScannerExtrasOfMovies, ARestoredMovieHasOnlyTheExtrasOfItsNfos)
{
  const std::string folder{Folder(m_root, "Extras")};
  const std::string video{File(folder, "Interview.mkv")};

  const int idMovie{AddMovie(URIUtils::AddFileToFolder(m_root, "Movie.mkv"))};
  ASSERT_GT(idMovie, 0);
  KODI::VIDEO::CVideoInfoScannerArt art;
  {
    KODI::VIDEO::CVideoInfoScannerExtras restoringScan{m_db, art};
    restoringScan.SetRestoredFromNfo(idMovie);
    restoringScan.AddVideoExtras(idMovie, folder);
    EXPECT_EQ(m_db.GetVideoVersionInfo(video).m_assetTypeId, -1);
  }

  KODI::VIDEO::CVideoInfoScannerExtras laterScan{m_db, art};
  laterScan.AddVideoExtras(idMovie, folder);
  EXPECT_EQ(m_db.GetVideoVersionInfo(video).m_assetType, VideoAssetType::EXTRA);
}

TEST_F(TestVideoInfoScannerExtrasOfMovies, ExtrasBesideADiscNoticeANewVideo)
{
  const std::string folder{Folder(m_root, "Extras")};

  const int idMovie{AddMovie(URIUtils::AddFileToFolder(m_root, "Movie.mkv"))};
  ASSERT_GT(idMovie, 0);
  const std::string first{File(folder, "First.mkv")};
  ASSERT_GT(AddMovie(first), 0);

  KODI::VIDEO::CVideoInfoScannerArt art;
  KODI::VIDEO::CVideoInfoScannerExtras scanner{m_db, art};
  EXPECT_TRUE(scanner.AddVideoExtrasBesideDisc(m_root));
  EXPECT_EQ(m_db.GetVideoVersionInfo(first).m_assetType, VideoAssetType::EXTRA);

  const int renamed{
      m_db.AddVideoVersionType("Renamed", VideoAssetTypeOwner::USER, VideoAssetType::EXTRA)};
  m_db.SetVideoVersion(m_db.GetVideoVersionInfo(first).m_idFile, renamed);

  // A new video is made an extra; one already there keeps its renamed type
  const std::string second{File(folder, "Second.mkv")};
  EXPECT_GT(AddMovie(second), 0);
  EXPECT_TRUE(scanner.AddVideoExtrasBesideDisc(m_root));
  EXPECT_EQ(m_db.GetVideoVersionInfo(second).m_assetType, VideoAssetType::EXTRA);
  EXPECT_EQ(m_db.GetVideoVersionInfo(first).m_assetTypeId, renamed);
  CFileItemList extras;
  m_db.GetVideoVersions(VideoDbContentType::MOVIES, idMovie, extras, VideoAssetType::EXTRA);
  EXPECT_EQ(extras.Size(), 2);
}

// A video in an extras folder made a version of the movie stays one when the folder is scanned
// again, and the movie keeps its own file
TEST_F(TestVideoInfoScannerExtrasOfMovies, AVersionInAnExtrasFolderStaysAVersion)
{
  const std::string folder{Folder(m_root, "Extras")};
  const std::string video{File(folder, "Director's Cut.mkv")};

  const std::string movie{URIUtils::AddFileToFolder(m_root, "Movie.mkv")};
  const int idMovie{AddMovie(movie)};
  ASSERT_GT(idMovie, 0);
  ASSERT_GT(AddAsset(idMovie, video, VideoAssetType::VERSION, "Director's Cut"), 0);
  const int idType{m_db.GetVideoVersionInfo(video).m_assetTypeId};

  KODI::VIDEO::CVideoInfoScannerArt art;
  KODI::VIDEO::CVideoInfoScannerExtras scanner{m_db, art};
  scanner.AddVideoExtras(idMovie, folder);

  EXPECT_EQ(m_db.GetVideoVersionInfo(video).m_assetTypeId, idType);
  const VideoAssetInfo own{m_db.GetVideoVersionInfo(movie)};
  EXPECT_EQ(own.m_assetType, VideoAssetType::VERSION);
  EXPECT_EQ(own.m_idMedia, idMovie);
}

// A refresh adds the extras of a movie's extras folders, whether or not they have changed since they
// were last added, as in a library scanned before they were recognised. Those of a movie on several
// discs are beside the discs' folders.
TEST_F(TestVideoInfoScannerExtrasOfMovies, ARefreshAddsTheExtrasOfAnUnchangedFolder)
{
  const std::string root{Folder(m_root, "Movies")};
  const std::string folder{Folder(root, "Extras")};
  const std::string video{File(folder, "Interview.mkv")};

  const int idMovie{AddMovie(URIUtils::AddFileToFolder(root, "Disc 1", "Movie.mkv"))};
  ASSERT_GT(idMovie, 0);
  ASSERT_TRUE(m_db.ExecuteQuery(m_db.PrepareSQL(
      "UPDATE path SET strContent='movies', strScraper='metadata.local' WHERE idPath=%i",
      m_db.AddPath(root))));

  // A version outside the sources, which a scan never looks at
  const std::string elsewhere{Folder(m_root, "Elsewhere")};
  const std::string elsewhereExtra{File(Folder(elsewhere, "Extras"), "Other.mkv")};
  CFileItem version{URIUtils::AddFileToFolder(elsewhere, "Movie Extended.mkv"), false};
  ASSERT_TRUE(m_db.AddVideoAsset(VideoDbContentType::MOVIES, idMovie, VIDEO_VERSION_ID_DEFAULT,
                                 VideoAssetType::VERSION, version));

  KODI::VIDEO::CVideoInfoScannerArt art;
  KODI::VIDEO::CVideoInfoScannerExtras scanner{m_db, art};

  // Without folder names a movie has no extras folders
  scanner.AddMovieExtras(idMovie, false);
  EXPECT_EQ(m_db.GetVideoVersionInfo(video).m_assetTypeId, -1);

  scanner.AddMovieExtras(idMovie, true);
  const VideoAssetInfo extra{m_db.GetVideoVersionInfo(video)};
  EXPECT_EQ(extra.m_assetType, VideoAssetType::EXTRA);
  EXPECT_EQ(extra.m_idMedia, idMovie);
  EXPECT_EQ(m_db.GetVideoVersionInfo(elsewhereExtra).m_assetTypeId, -1);
}
