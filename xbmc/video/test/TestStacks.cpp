/*
 *  Copyright (C) 2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FileItem.h"
#include "FileItemList.h"
#include "URL.h"
#include "filesystem/Directory.h"
#include "filesystem/StackDirectory.h"
#include "test/TestUtils.h"
#include "utils/URIUtils.h"
#include "video/VideoFileItemClassify.h"

#include <string>
#include <vector>

#include <gtest/gtest.h>

using namespace XFILE;

using ::testing::Test;
using ::testing::ValuesIn;
using ::testing::WithParamInterface;

namespace
{
const std::string VIDEO_EXTENSIONS = ".mpg|.mpeg|.mp4|.mkv|.mk3d|.iso";
}

class TestStacks : public ::testing::Test
{
protected:
  TestStacks() = default;
  ~TestStacks() override = default;
};

TEST_F(TestStacks, TestMovieFilesStackFilesAB)
{
  const std::string movieFolder =
      XBMC_REF_FILE_PATH("xbmc/video/test/testdata/moviestack_ab/Movie-(2001)");
  CFileItemList items;
  CDirectory::GetDirectory(movieFolder, items, VIDEO_EXTENSIONS, DIR_FLAG_DEFAULTS);
  // make sure items has 2 items (the two movie parts)
  EXPECT_EQ(items.Size(), 2);
  // stack the items and make sure we end up with a single movie
  items.Stack();
  EXPECT_EQ(items.Size(), 1);
  // check the single item in the stack is a stack://
  if (!items.IsEmpty())
  {
    EXPECT_EQ(items.Get(0)->IsStack(), true);
  }
}

TEST_F(TestStacks, TestMovieFilesStackFilesPart)
{
  const std::string movieFolder =
      XBMC_REF_FILE_PATH("xbmc/video/test/testdata/moviestack_part/Movie_(2001)");
  CFileItemList items;
  CDirectory::GetDirectory(movieFolder, items, VIDEO_EXTENSIONS, DIR_FLAG_DEFAULTS);
  // make sure items has 3 items (the three movie parts)
  EXPECT_EQ(items.Size(), 3);
  // stack the items and make sure we end up with a single movie
  items.Stack();
  EXPECT_EQ(items.Size(), 1);
  // check the single item in the stack is a stack://
  if (!items.IsEmpty())
  {
    EXPECT_EQ(items.Get(0)->IsStack(), true);
  }
}

TEST_F(TestStacks, TestMovieFilesStackDvdIso)
{
  const std::string movieFolder =
      XBMC_REF_FILE_PATH("xbmc/video/test/testdata/moviestack_dvdiso/Movie_(2001)");
  CFileItemList items;
  CDirectory::GetDirectory(movieFolder, items, VIDEO_EXTENSIONS, DIR_FLAG_DEFAULTS);
  // make sure items has 2 items (the two dvd isos)
  EXPECT_EQ(items.Size(), 2);
  // stack the items and make sure we end up with a single movie
  items.Stack();
  EXPECT_EQ(items.Size(), 1);
  // check the single item in the stack is a stack://
  if (!items.IsEmpty())
  {
    EXPECT_EQ(items.Get(0)->IsStack(), true);
  }
}

TEST_F(TestStacks, TestMovieFilesStackBlurayIso)
{
  const std::string movieFolder =
      XBMC_REF_FILE_PATH("xbmc/video/test/testdata/moviestack_blurayiso/Movie_(2001)");
  CFileItemList items;
  CDirectory::GetDirectory(movieFolder, items, VIDEO_EXTENSIONS, DIR_FLAG_DEFAULTS);
  // make sure items has 2 items (the two bluray isos)
  EXPECT_EQ(items.Size(), 2);
  // stack the items and make sure we end up with a single movie
  items.Stack();
  EXPECT_EQ(items.Size(), 1);
  // check the single item in the stack is a stack://
  if (!items.IsEmpty())
  {
    EXPECT_EQ(items.Get(0)->IsStack(), true);
  }
}

TEST_F(TestStacks, TestMovieFilesStackFolderFilesPart)
{
  const std::string movieFolder =
      XBMC_REF_FILE_PATH("xbmc/video/test/testdata/moviestack_subfolder_parts/Movie_(2001)");
  CFileItemList items;
  CDirectory::GetDirectory(movieFolder, items, "", DIR_FLAG_DEFAULTS);
  // make sure items has 3 items (the three movie parts)
  EXPECT_EQ(items.Size(), 3);
  // stack the items and make sure we end up with a single movie
  items.Stack();
  EXPECT_EQ(items.Size(), 1);
  // check the single item in the stack is a stack://
  if (!items.IsEmpty())
  {
    EXPECT_EQ(items.Get(0)->IsStack(), true);
    EXPECT_EQ(items.Get(0)->IsFolder(), false);
  }
}

TEST_F(TestStacks, TestMovieFilesStackFolderFilesPart2)
{
  const std::string movieFolder =
      XBMC_REF_FILE_PATH("xbmc/video/test/testdata/moviestack_subfolder_movie");
  CFileItemList items;
  CDirectory::GetDirectory(movieFolder, items, "", DIR_FLAG_DEFAULTS);
  // make sure items has 2 items (the two movie parts)
  EXPECT_EQ(items.Size(), 2);
  // stack the items and make sure we end up with a single movie
  items.Stack();
  EXPECT_EQ(items.Size(), 1);
  // check the single item in the stack is a stack://
  if (!items.IsEmpty())
  {
    EXPECT_EQ(items.Get(0)->IsStack(), true);
    EXPECT_EQ(items.Get(0)->IsFolder(), false);
  }
}

#ifdef HAVE_LIBBLURAY
// The Blu-ray part is only recognised as a disc folder when libbluray is available.
TEST_F(TestStacks, TestMovieFilesStackFolderFilesDiscPart)
{
  const std::string movieFolder =
      XBMC_REF_FILE_PATH("xbmc/video/test/testdata/moviestack_subfolder_disc_parts/Movie_(2001)");
  CFileItemList items;
  CDirectory::GetDirectory(movieFolder, items, "", DIR_FLAG_DEFAULTS);
  // make sure items has 2 items (the two movie parts)
  EXPECT_EQ(items.Size(), 2);
  // stack the items and make sure we end up with a single movie
  items.Stack();
  EXPECT_EQ(items.Size(), 1);

  // check the single item in the stack is a stack://
  std::shared_ptr<CFileItem> item{items.Get(0)};
  EXPECT_EQ(item->IsStack(), true);
  EXPECT_EQ(item->IsFolder(), false);

  // check bluray/dvd paths
  std::vector<std::string> paths;
  CStackDirectory::GetPaths(item->GetPath(), paths);
  EXPECT_EQ(paths.size(), 2);
  if (paths.size() == 2)
  {
    EXPECT_EQ(URIUtils::IsDVDFile(paths[0]), true);
    EXPECT_EQ(URIUtils::IsBDFile(paths[1]), true);
  }
}
#endif

TEST_F(TestStacks, TestStackIsNotItselfADiscFile)
{
  // stack://<part1> , <part2>\BDMV\index.bdmv should not be taken for a plain Blu-ray file, which can make
  // GetParentPath() truncate the url mid-stack and ultimately produced a bluray:// url rooted at the folder
  // of the last part.
  const std::string bdStack{
      R"(stack://D:\Movies\Movie_PART1\BDMV\index.bdmv , D:\Movies\Movie_PART2\BDMV\index.bdmv)"};
  const std::string dvdStack{
      R"(stack://D:\Movies\Movie_PART1\VIDEO_TS\VIDEO_TS.IFO , D:\Movies\Movie_PART2\VIDEO_TS\VIDEO_TS.IFO)"};

  EXPECT_FALSE(URIUtils::IsBDFile(bdStack));
  EXPECT_FALSE(URIUtils::IsDVDFile(dvdStack));
  EXPECT_FALSE(URIUtils::IsOpticalMediaFile(bdStack));
  EXPECT_FALSE(URIUtils::IsOpticalMediaFile(dvdStack));

  // CURL keeps the whole remainder of a stack:// url in its filename, so it must agree
  EXPECT_FALSE(CURL(bdStack).IsBDFile());
  EXPECT_FALSE(CURL(dvdStack).IsDVDFile());
  EXPECT_FALSE(CURL(bdStack).IsOpticalMediaFile());

  // KODI::VIDEO::IsDVDFile() does not delegate to URIUtils, so it needs its own guard - without
  // it a DVD folder stack takes the disc branch in eg. CFileItem::GetLocalMetadataPath and
  // CGUIDialogVideoInfo::DeleteVideoItemFromDatabase, which then act on the folder of the parts
  EXPECT_FALSE(KODI::VIDEO::IsDVDFile(CFileItem(dvdStack, false)));

  // ..but the individual members still are disc files
  std::vector<std::string> paths;
  CStackDirectory::GetPaths(bdStack, paths);
  ASSERT_EQ(paths.size(), 2U);
  EXPECT_TRUE(URIUtils::IsBDFile(paths[0]));
  EXPECT_TRUE(URIUtils::IsBDFile(paths[1]));

  CStackDirectory::GetPaths(dvdStack, paths);
  ASSERT_EQ(paths.size(), 2U);
  EXPECT_TRUE(KODI::VIDEO::IsDVDFile(CFileItem(paths[0], false)));
  EXPECT_TRUE(KODI::VIDEO::IsDVDFile(CFileItem(paths[1], false)));

  // The stack is unwrapped intact (not truncated at the last separator of the whole url)
  EXPECT_EQ(URIUtils::GetParentPath(bdStack), R"(D:\Movies\)");
}

TEST_F(TestStacks, TestStackIsNotItselfADiscImage)
{
  // A stack of .ISOs must not be classified from its tail either - otherwise it takes the disc
  // branch in eg. CVideoDatabase::GetMovieId/GetVideoVersionsByPath, which asks for a bluray://
  // playlist path that a container cannot have. Use IsDiscImageStack() to spot these instead
  const std::string isoStack{R"(stack://D:\Movies\Movie.CD1.iso , D:\Movies\Movie.CD2.iso)"};

  EXPECT_FALSE(URIUtils::IsDiscImage(isoStack));
  EXPECT_FALSE(CURL(isoStack).IsDiscImage());
  EXPECT_FALSE(CFileItem(isoStack, false).IsDiscImage());

  // The stack as a whole is still recognised as one holding disc parts..
  EXPECT_TRUE(URIUtils::IsDiscImageStack(isoStack));

  // ..and the individual members are still disc images
  std::vector<std::string> paths;
  CStackDirectory::GetPaths(isoStack, paths);
  ASSERT_EQ(paths.size(), 2U);
  EXPECT_TRUE(URIUtils::IsDiscImage(paths[0]));
  EXPECT_TRUE(URIUtils::IsDiscImage(paths[1]));

  // A single .ISO is unaffected
  EXPECT_TRUE(URIUtils::IsDiscImage(R"(D:\Movies\Movie.iso)"));
  EXPECT_TRUE(CURL(R"(D:\Movies\Movie.iso)").IsDiscImage());
}

TEST_F(TestStacks, TestStackHasDiscPart)
{
  // Any stack holding a disc can be held in the library under a path that differs from the one
  // listed, as the playlist of each disc is only known once the discs have been scanned. Which
  // form each part happens to take says nothing about that - a disc image, in particular, names
  // itself in both forms
  EXPECT_TRUE(CStackDirectory::HasDiscPart(
      R"(stack://D:\Movies\Movie_PART1\BDMV\index.bdmv , D:\Movies\Movie_PART2\BDMV\index.bdmv)"));
  EXPECT_TRUE(
      CStackDirectory::HasDiscPart(R"(stack://D:\Movies\Movie.CD1.iso , D:\Movies\Movie.CD2.iso)"));
  EXPECT_TRUE(CStackDirectory::HasDiscPart(
      R"(stack://D:\Movies\Movie_PART1\VIDEO_TS\VIDEO_TS.IFO , D:\Movies\Movie_PART2\VIDEO_TS\VIDEO_TS.IFO)"));
  EXPECT_TRUE(CStackDirectory::HasDiscPart(
      R"(stack://bluray://D%3a%5cMovies%5cMovie_PART1%5c/BDMV/PLAYLIST/00800.mpls , )"
      R"(bluray://D%3a%5cMovies%5cMovie_PART2%5c/BDMV/PLAYLIST/00801.mpls)"));

  // a mixed stack still holds a disc
  EXPECT_TRUE(CStackDirectory::HasDiscPart(
      R"(stack://D:\Movies\Movie_PART1\VIDEO_TS\VIDEO_TS.IFO , D:\Movies\Movie.CD2.iso)"));
  EXPECT_TRUE(
      CStackDirectory::HasDiscPart(R"(stack://D:\Movies\Movie.CD1.mkv , D:\Movies\Movie.CD2.iso)"));

  // A stack of ordinary files holds none, and is stored as it is listed
  EXPECT_FALSE(
      CStackDirectory::HasDiscPart(R"(stack://D:\Movies\Movie.CD1.mkv , D:\Movies\Movie.CD2.mkv)"));
  EXPECT_FALSE(CStackDirectory::HasDiscPart(
      R"(stack://D:\Movies\Movie.part1.mp4 , D:\Movies\Movie.part2.mp4)"));

  // Anything that is not a stack is answered for rather than unwrapped
  EXPECT_FALSE(CStackDirectory::HasDiscPart(R"(D:\Movies\Movie_PART1\BDMV\index.bdmv)"));
  EXPECT_FALSE(CStackDirectory::HasDiscPart("D:\\a.iso"));
  EXPECT_FALSE(CStackDirectory::HasDiscPart(""));
}

TEST_F(TestStacks, TestStackBasePathIsIndependentOfPlaylistResolution)
{
  // A stack of discs is looked up among the rows stored under its base path (see
  // CVideoDatabase::GetDiscStackFileId), so both forms of one have to reduce to the same base
  // path or a stack already in the library is not found and is scraped again
  EXPECT_EQ(
      CStackDirectory::GetBasePath(
          R"(stack://D:\Movies\Movie_PART1\BDMV\index.bdmv , D:\Movies\Movie_PART2\BDMV\index.bdmv)"),
      CStackDirectory::GetBasePath(
          R"(stack://bluray://D%3a%5cMovies%5cMovie_PART1%5c/BDMV/PLAYLIST/00800.mpls , )"
          R"(bluray://D%3a%5cMovies%5cMovie_PART2%5c/BDMV/PLAYLIST/00801.mpls)"));

  // ..a stack of disc images, which resolves through udf://
  EXPECT_EQ(
      CStackDirectory::GetBasePath(R"(stack://D:\Movies\Movie.CD1.iso , D:\Movies\Movie.CD2.iso)"),
      CStackDirectory::GetBasePath(
          R"(stack://bluray://udf%3a%2f%2fD%253a%255cMovies%255cMovie.CD1.iso%2f/BDMV/PLAYLIST/01003.mpls , )"
          R"(bluray://udf%3a%2f%2fD%253a%255cMovies%255cMovie.CD2.iso%2f/BDMV/PLAYLIST/01003.mpls)"));

  // ..a rip held in the movie folder itself rather than a folder per part
  EXPECT_EQ(CStackDirectory::GetBasePath(
                R"(stack://D:\Movies\Movie\BDMV\index.bdmv , D:\Movies\Movie2\BDMV\index.bdmv)"),
            CStackDirectory::GetBasePath(
                R"(stack://bluray://D%3a%5cMovies%5cMovie%5c/BDMV/PLAYLIST/00800.mpls , )"
                R"(bluray://D%3a%5cMovies%5cMovie2%5c/BDMV/PLAYLIST/00800.mpls)"));

  // ..and a stack whose parts are not all held the same way
  EXPECT_EQ(
      CStackDirectory::GetBasePath(
          R"(stack://D:\Movies\Movie_PART1\BDMV\index.bdmv , D:\Movies\Movie.CD2.iso)"),
      CStackDirectory::GetBasePath(
          R"(stack://bluray://D%3a%5cMovies%5cMovie_PART1%5c/BDMV/PLAYLIST/00800.mpls , )"
          R"(bluray://udf%3a%2f%2fD%253a%255cMovies%255cMovie.CD2.iso%2f/BDMV/PLAYLIST/01003.mpls)"));
}

TEST_F(TestStacks, TestSameDiscStack)
{
  const std::string bdStack{
      R"(stack://D:\Movies\Movie_PART1\BDMV\index.bdmv , D:\Movies\Movie_PART2\BDMV\index.bdmv)"};
  const std::string resolvedBdStack{
      R"(stack://bluray://D%3a%5cMovies%5cMovie_PART1%5c/BDMV/PLAYLIST/00800.mpls , )"
      R"(bluray://D%3a%5cMovies%5cMovie_PART2%5c/BDMV/PLAYLIST/00801.mpls)"};

  // A stack listed from disc names no playlist, so it matches the resolved form of itself in the
  // library - the point of the comparison
  EXPECT_TRUE(CStackDirectory::IsSameDiscStack(bdStack, resolvedBdStack));
  EXPECT_TRUE(CStackDirectory::IsSameDiscStack(resolvedBdStack, bdStack));
  EXPECT_TRUE(CStackDirectory::IsSameDiscStack(bdStack, bdStack));
  EXPECT_TRUE(CStackDirectory::IsSameDiscStack(resolvedBdStack, resolvedBdStack));

  // ..but a disc can hold a cut, or a feature, per playlist, so two resolved stacks of the same
  // discs that name different playlists are different movies
  const std::string otherPlaylists{
      R"(stack://bluray://D%3a%5cMovies%5cMovie_PART1%5c/BDMV/PLAYLIST/00900.mpls , )"
      R"(bluray://D%3a%5cMovies%5cMovie_PART2%5c/BDMV/PLAYLIST/00901.mpls)"};
  EXPECT_FALSE(CStackDirectory::IsSameDiscStack(resolvedBdStack, otherPlaylists));

  // one differing part is enough
  const std::string onePlaylistDiffers{
      R"(stack://bluray://D%3a%5cMovies%5cMovie_PART1%5c/BDMV/PLAYLIST/00800.mpls , )"
      R"(bluray://D%3a%5cMovies%5cMovie_PART2%5c/BDMV/PLAYLIST/00901.mpls)"};
  EXPECT_FALSE(CStackDirectory::IsSameDiscStack(resolvedBdStack, onePlaylistDiffers));

  // Different discs, and a different number of them, never match
  EXPECT_FALSE(CStackDirectory::IsSameDiscStack(
      bdStack,
      R"(stack://D:\Movies\Other_PART1\BDMV\index.bdmv , D:\Movies\Other_PART2\BDMV\index.bdmv)"));
  EXPECT_FALSE(CStackDirectory::IsSameDiscStack(
      bdStack, R"(stack://D:\Movies\Movie_PART1\BDMV\index.bdmv , )"
               R"(D:\Movies\Movie_PART2\BDMV\index.bdmv , D:\Movies\Movie_PART3\BDMV\index.bdmv)"));

  // ..nor do the same discs in a different order
  EXPECT_FALSE(CStackDirectory::IsSameDiscStack(
      bdStack,
      R"(stack://D:\Movies\Movie_PART2\BDMV\index.bdmv , D:\Movies\Movie_PART1\BDMV\index.bdmv)"));

  // A stack of disc images behaves the same way through udf://
  const std::string isoStack{R"(stack://D:\Movies\Movie.CD1.iso , D:\Movies\Movie.CD2.iso)"};
  const std::string resolvedIsoStack{
      R"(stack://bluray://udf%3a%2f%2fD%253a%255cMovies%255cMovie.CD1.iso%2f/BDMV/PLAYLIST/01003.mpls , )"
      R"(bluray://udf%3a%2f%2fD%253a%255cMovies%255cMovie.CD2.iso%2f/BDMV/PLAYLIST/01003.mpls)"};
  const std::string resolvedIsoStackOtherPlaylist{
      R"(stack://bluray://udf%3a%2f%2fD%253a%255cMovies%255cMovie.CD1.iso%2f/BDMV/PLAYLIST/01050.mpls , )"
      R"(bluray://udf%3a%2f%2fD%253a%255cMovies%255cMovie.CD2.iso%2f/BDMV/PLAYLIST/01050.mpls)"};
  EXPECT_TRUE(CStackDirectory::IsSameDiscStack(isoStack, resolvedIsoStack));
  EXPECT_FALSE(CStackDirectory::IsSameDiscStack(resolvedIsoStack, resolvedIsoStackOtherPlaylist));

  // A stack of plain files is compared as it is listed, and is not a stack of these discs
  const std::string fileStack{R"(stack://D:\Movies\Movie.CD1.mkv , D:\Movies\Movie.CD2.mkv)"};
  EXPECT_TRUE(CStackDirectory::IsSameDiscStack(fileStack, fileStack));
  EXPECT_FALSE(CStackDirectory::IsSameDiscStack(fileStack, isoStack));
  EXPECT_FALSE(CStackDirectory::IsSameDiscStack(
      fileStack, R"(stack://D:\Movies\Movie.CD1.mkv , D:\Movies\Movie.CD3.mkv)"));

  // Neither side is unwrapped unless it is a stack
  EXPECT_FALSE(
      CStackDirectory::IsSameDiscStack(bdStack, R"(D:\Movies\Movie_PART1\BDMV\index.bdmv)"));
  EXPECT_FALSE(
      CStackDirectory::IsSameDiscStack(R"(D:\Movies\Movie_PART1\BDMV\index.bdmv)", bdStack));
  EXPECT_FALSE(CStackDirectory::IsSameDiscStack("D:\\a.iso", "D:\\a.iso"));
  EXPECT_FALSE(CStackDirectory::IsSameDiscStack("", ""));
}

TEST_F(TestStacks, TestSameDiscStackIgnoresSourceOptions)
{
  // The options of a url are no part of the disc a part is held on, nor of the playlist, so a
  // stack has to keep matching the one in the library when they change
  const std::string stack{
      R"(stack://bluray://smb%3a%2f%2fhost%2fshare%2fMovie_PART1%2f/BDMV/PLAYLIST/00800.mpls , )"
      R"(bluray://smb%3a%2f%2fhost%2fshare%2fMovie_PART2%2f/BDMV/PLAYLIST/00800.mpls)"};
  const std::string withOptions{
      R"(stack://bluray://smb%3a%2f%2fhost%2fshare%2fMovie_PART1%2f/BDMV/PLAYLIST/00800.mpls?opt=1 , )"
      R"(bluray://smb%3a%2f%2fhost%2fshare%2fMovie_PART2%2f/BDMV/PLAYLIST/00800.mpls?opt=1)"};

  EXPECT_TRUE(CStackDirectory::IsSameDiscStack(stack, withOptions));
  EXPECT_TRUE(CStackDirectory::IsSameDiscStack(withOptions, stack));

  // ..while a different playlist of those same discs is still a different movie
  const std::string otherPlaylist{
      R"(stack://bluray://smb%3a%2f%2fhost%2fshare%2fMovie_PART1%2f/BDMV/PLAYLIST/00900.mpls?opt=1 , )"
      R"(bluray://smb%3a%2f%2fhost%2fshare%2fMovie_PART2%2f/BDMV/PLAYLIST/00900.mpls?opt=1)"};
  EXPECT_FALSE(CStackDirectory::IsSameDiscStack(stack, otherPlaylist));

  // A trailing slash on a disc root says nothing about the disc either
  EXPECT_TRUE(CStackDirectory::IsSameDiscStack(
      R"(stack://D:\Movies\Movie_PART1\BDMV\index.bdmv , D:\Movies\Movie_PART2\BDMV\index.bdmv)",
      R"(stack://bluray://D%3a%5cMovies%5cMovie_PART1%5c/BDMV/PLAYLIST/00800.mpls , )"
      R"(bluray://D%3a%5cMovies%5cMovie_PART2%5c/BDMV/PLAYLIST/00800.mpls)"));

  // A part that is an ordinary file is compared as one, so two files in a folder stay apart
  EXPECT_FALSE(CStackDirectory::IsSameDiscStack(
      R"(stack://smb://host/share/Movie.CD1.mkv , smb://host/share/Movie.CD2.mkv)",
      R"(stack://smb://host/share/Movie.CD1.mkv , smb://host/share/Movie.CD3.mkv)"));
}

TEST_F(TestStacks, TestStackHasNoBlurayPath)
{
  // Each member of a stack of disc folders is its own disc, so there is no single disc root to
  // put in a bluray:// url.
  const std::string bdStack{
      R"(stack://D:\Movies\Movie_PART1\BDMV\index.bdmv , D:\Movies\Movie_PART2\BDMV\index.bdmv)"};

  EXPECT_TRUE(URIUtils::GetBlurayMainTitlePath(bdStack).empty());
  EXPECT_TRUE(URIUtils::GetBlurayPlaylistPath(bdStack).empty());
  EXPECT_TRUE(URIUtils::GetBlurayPlaylistPath(bdStack, 800).empty());
  EXPECT_TRUE(URIUtils::GetBlurayTitlesPath(bdStack).empty());
  EXPECT_TRUE(URIUtils::GetBlurayEpisodePath(bdStack, 1, 2).empty());
  EXPECT_TRUE(URIUtils::GetBlurayAllEpisodesPath(bdStack).empty());
  EXPECT_TRUE(URIUtils::GetBlurayMenuPath(bdStack).empty());

  // A single disc folder is unaffected
  EXPECT_EQ(URIUtils::GetBlurayMainTitlePath(R"(D:\Movies\Movie_PART1\BDMV\index.bdmv)"),
            R"(bluray://D%3a%5cMovies%5cMovie_PART1%5c/root/main)");
}

TEST_F(TestStacks, TestMovieFilesStackFolderFilesDiscNPart)
{
  const std::string movieFolder =
      XBMC_REF_FILE_PATH("xbmc/video/test/testdata/moviestack_discn_parts/Movie_(2001)");
  CFileItemList items;
  CDirectory::GetDirectory(movieFolder, items, "", DIR_FLAG_DEFAULTS);
  // make sure items has 2 items (the two movie parts)
  EXPECT_EQ(items.Size(), 2);
  // attempt to stack -> should fail as parts are 'Disc n'
  items.Stack();
  EXPECT_EQ(items.Size(), 2);
}

TEST_F(TestStacks, TestConstructStackPath)
{
  CFileItemList items;
  CFileItem item;

  // File stack

  item.SetPath("smb://somepath/movie_part_1.mkv");
  items.Add(std::make_shared<CFileItem>(item));

  CFileItem item2;
  item2.SetPath("smb://somepath/movie_part_2.mkv");
  items.Add(std::make_shared<CFileItem>(item2));

  std::vector<int> index(2);
  index[0] = 0;
  index[1] = 1;

  std::string path{CStackDirectory::ConstructStackPath(items, index)};
  EXPECT_EQ(path, "stack://smb://somepath/movie_part_1.mkv , smb://somepath/movie_part_2.mkv");

  index[0] = 1;
  index[1] = 0;

  path = CStackDirectory::ConstructStackPath(items, index);
  EXPECT_EQ(path, "stack://smb://somepath/movie_part_2.mkv , smb://somepath/movie_part_1.mkv");

  std::vector<std::string> paths;
  paths.emplace_back("smb://somepath/movie_part_1.mkv");
  EXPECT_EQ(CStackDirectory::ConstructStackPath(paths, path), false);

  paths.emplace_back("smb://somepath/movie_part_2.mkv");
  EXPECT_EQ(CStackDirectory::ConstructStackPath(paths, path), true);
  EXPECT_EQ(path, "stack://smb://somepath/movie_part_1.mkv , smb://somepath/movie_part_2.mkv");

  EXPECT_EQ(CStackDirectory::ConstructStackPath(paths, path, "smb://somepath/movie_part_3.mkv"),
            true);
  EXPECT_EQ(path, "stack://smb://somepath/movie_part_1.mkv , smb://somepath/movie_part_2.mkv , "
                  "smb://somepath/movie_part_3.mkv");

  // Folder stack

  items.Clear();

  item.SetPath("smb://somepath/movie/movie_part_1/file.mkv");
  items.Add(std::make_shared<CFileItem>(item));

  item2.SetPath("smb://somepath/movie/movie_part_2/file.mkv");
  items.Add(std::make_shared<CFileItem>(item2));

  index[0] = 0;
  index[1] = 1;

  path = CStackDirectory::ConstructStackPath(items, index);
  EXPECT_EQ(path, "stack://smb://somepath/movie/movie_part_1/file.mkv , "
                  "smb://somepath/movie/movie_part_2/file.mkv");

  index[0] = 1;
  index[1] = 0;

  path = CStackDirectory::ConstructStackPath(items, index);
  EXPECT_EQ(path, "stack://smb://somepath/movie/movie_part_2/file.mkv , "
                  "smb://somepath/movie/movie_part_1/file.mkv");

  paths.clear();
  paths.emplace_back("smb://somepath/movie/movie_part_1/file.mkv");
  EXPECT_EQ(CStackDirectory::ConstructStackPath(paths, path), false);

  paths.emplace_back("smb://somepath/movie/movie_part_2/file.mkv");
  EXPECT_EQ(CStackDirectory::ConstructStackPath(paths, path), true);
  EXPECT_EQ(path, "stack://smb://somepath/movie/movie_part_1/file.mkv , "
                  "smb://somepath/movie/movie_part_2/file.mkv");

  EXPECT_EQ(CStackDirectory::ConstructStackPath(paths, path,
                                                "smb://somepath/movie/movie_part_3/file.mkv"),
            true);
  EXPECT_EQ(path, "stack://smb://somepath/movie/movie_part_1/file.mkv , "
                  "smb://somepath/movie/movie_part_2/file.mkv , "
                  "smb://somepath/movie/movie_part_3/file.mkv");

  paths.clear();
  paths.emplace_back("smb://somepath/movie/moviepart1/file.mkv");
  EXPECT_EQ(CStackDirectory::ConstructStackPath(paths, path), false);

  paths.emplace_back("smb://somepath/movie/moviepart2/file.mkv");
  EXPECT_EQ(CStackDirectory::ConstructStackPath(paths, path), true);
  EXPECT_EQ(path, "stack://smb://somepath/movie/moviepart1/file.mkv , "
                  "smb://somepath/movie/moviepart2/file.mkv");
}

TEST_F(TestStacks, TestGetParentPath)
{
  std::string path{"stack://smb://somepath/movie_part_1.mkv , smb://somepath/movie_part_2.mkv , "
                   "smb://somepath/movie_part_3.mkv"};
  std::string parent{CStackDirectory::GetParentPath(path)};
  EXPECT_EQ(parent, "smb://somepath/");

  path = "stack://smb://somepath/BDMV/index.bdmv , smb://somepath/VIDEO_TS/VIDEO_TS.IFO";
  parent = CStackDirectory::GetParentPath(path);
  EXPECT_EQ(parent, "smb://somepath/");

  path = "stack://smb://somepath/movie/movie_part_1/file.mkv , "
         "smb://somepath/movie/movie_part_2/file.mkv";
  parent = CStackDirectory::GetParentPath(path);
  EXPECT_EQ(parent, "smb://somepath/movie/");

  path = "stack://smb://somepath/movie/moviepart1/file.mkv , "
         "smb://somepath/movie/moviepart2/file.mkv";
  parent = CStackDirectory::GetParentPath(path);
  EXPECT_EQ(parent, "smb://somepath/movie/");

  path = "stack://smb://somepath/a/b/c/d/e/movie_part_1.mkv , "
         "smb://somepath/a/f/g/h/i/movie_part_2.mkv";
  parent = CStackDirectory::GetParentPath(path);
  EXPECT_EQ(parent, "smb://somepath/a/");

  path = "stack://smb://somepath/a/b/c/d/e/movie_part_1.mkv , "
         "smb://somepath/a/movie_part_2.mkv";
  parent = CStackDirectory::GetParentPath(path);
  EXPECT_EQ(parent, "smb://somepath/a/"); // Asymmetric levels

  path = "stack://smb://somepath/a/b/movie_part_1.mkv , "
         "smb://somepath/a/f/g/h/i/movie_part_2.mkv";
  parent = CStackDirectory::GetParentPath(path);
  EXPECT_EQ(parent, "smb://somepath/a/"); // Asymmetric levels

  path = "stack://smb://somepath/a/b/c/d/e/f/g/h/i/j/k/movie_part_1.mkv , "
         "smb://somepath/a/m/n/o/p/q/r/s/t/u/v/movie_part_2.mkv";
  parent = CStackDirectory::GetParentPath(path);
  EXPECT_EQ(parent, "smb://somepath/a/"); // 10 levels

  path = "stack://smb://somepath/a/b/c/d/e/f/g/h/i/j/k/l/movie_part_1.mkv , "
         "smb://somepath/a/m/n/o/p/q/r/s/t/u/v/w/movie_part_2.mkv";
  parent = CStackDirectory::GetParentPath(path);
  EXPECT_EQ(parent, "/"); // Too many levels
}

struct TestStackData
{
  const char* path;
  const char* basePath;
  const char* firstPath;
};

class TestGetStackedTitlePath : public Test, public WithParamInterface<TestStackData>
{
};

constexpr TestStackData Stacks[] = {
    {.path = "stack://smb://somepath/movie_part_1.mkv , smb://somepath/movie_part_2.mkv",
     .basePath = "smb://somepath/movie.mkv",
     .firstPath = "smb://somepath/movie_part_1.mkv"},
    {.path = "stack://smb://somepath/movie_part_1.iso , smb://somepath/movie_part_2.iso",
     .basePath = "smb://somepath/movie.iso",
     .firstPath = "smb://somepath/movie_part_1.iso"},
    {.path =
         "stack://smb://somepath/movie_part_1/movie.iso , smb://somepath/movie_part_2/movie.iso",
     .basePath = "smb://somepath/movie/",
     .firstPath = "smb://somepath/movie_part_1/movie.iso"},
    {.path = "stack://smb://somepath/movie (2000) part 1/movie.iso , smb://somepath/movie (2000) "
             "part 2/movie.iso",
     .basePath = "smb://somepath/movie (2000)/",
     .firstPath = "smb://somepath/movie (2000) part 1/movie.iso"},
    {.path = "stack://D:\\somepath\\movie (2000) part 1\\movie.iso , D:\\somepath\\movie (2000) "
             "part 2\\movie.iso",
     .basePath = "D:\\somepath\\movie (2000)\\",
     .firstPath = "D:\\somepath\\movie (2000) part 1\\movie.iso"},
    {.path = "stack://smb://somepath/movie - part 1/movie.iso , smb://somepath/movie - part "
             "2/movie.iso",
     .basePath = "smb://somepath/movie/",
     .firstPath = "smb://somepath/movie - part 1/movie.iso"},
    {.path = "stack://smb://somepath/movie (2000) - part 1/movie.iso , smb://somepath/movie (2000) "
             "- part "
             "2/movie.iso",
     .basePath = "smb://somepath/movie (2000)/",
     .firstPath = "smb://somepath/movie (2000) - part 1/movie.iso"},
    {.path = "stack://smb://somepath/movie.(2000).part.1/movie.iso , smb://somepath/movie.(2000)."
             "part.2/movie.iso",
     .basePath = "smb://somepath/movie.(2000)/",
     .firstPath = "smb://somepath/movie.(2000).part.1/movie.iso"},
    {.path = "stack://smb://somepath/movie(2000)part1/movie.iso , smb://somepath/movie(2000)"
             "part2/movie.iso",
     .basePath = "smb://somepath/movie(2000)/",
     .firstPath = "smb://somepath/movie(2000)part1/movie.iso"},
    {.path = "stack://smb://somepath/movie_part_1/BDMV/index.bdmv , "
             "smb://somepath/movie_part_2/VIDEO_TS/VIDEO_TS.IFO",
     .basePath = "smb://somepath/movie/",
     .firstPath = "smb://somepath/movie_part_1/BDMV/index.bdmv"},
    {.path =
         "stack://bluray://"
         "udf%3a%2f%2fsmb%253a%252f%252fsomepath%252fmovie_part_1%252fmovie.iso%2f/BDMV/"
         "PLAYLIST/00800.mpls , "
         "bluray://udf%3a%2f%2fsmb%253a%252f%252fsomepath%252fmovie_part_2%252fmovie.iso%2f/BDMV/"
         "PLAYLIST/00800.mpls",
     .basePath = "smb://somepath/movie/",
     .firstPath =
         "bluray://udf%3a%2f%2fsmb%253a%252f%252fsomepath%252fmovie_part_1%252fmovie.iso%2f/BDMV/"
         "PLAYLIST/00800.mpls"}};

TEST_P(TestGetStackedTitlePath, GetStackedTitlePath)
{
  CFileItem item;
  const std::string path{CStackDirectory::GetStackTitlePath(GetParam().path)};
  EXPECT_EQ(path, GetParam().basePath);
}

INSTANTIATE_TEST_SUITE_P(TestStackDirectory, TestGetStackedTitlePath, ValuesIn(Stacks));

class TestGetFirstStackedFile : public Test, public WithParamInterface<TestStackData>
{
};

TEST_P(TestGetFirstStackedFile, GetFirstStackedFile)
{
  CFileItem item;
  const std::string path{CStackDirectory::GetFirstStackedFile(GetParam().path)};
  EXPECT_EQ(path, GetParam().firstPath);
}

INSTANTIATE_TEST_SUITE_P(TestStackDirectory, TestGetFirstStackedFile, ValuesIn(Stacks));
