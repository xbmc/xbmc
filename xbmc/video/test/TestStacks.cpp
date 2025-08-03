/*
 *  Copyright (C) 2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FileItem.h"
#include "FileItemList.h"
#include "filesystem/Directory.h"
#include "filesystem/StackDirectory.h"
#include "test/TestUtils.h"
#include "utils/URIUtils.h"

#include <string>

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

TEST_F(TestStacks, TestConstructStackPath)
{
  CFileItemList items;

  CFileItem item;
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

  path = "stack://smb://somepath/a/b/c/d/e/movie_part_1.mkv , "
         "smb://somepath/a/f/g/h/i/movie_part_2.mkv";
  parent = CStackDirectory::GetParentPath(path);
  EXPECT_EQ(parent, "smb://somepath/a/");

  path = "stack://smb://somepath/a/b/c/d/e/f/g/movie_part_1.mkv , "
         "smb://somepath/a/h/i/j/k/l/m/movie_part_2.mkv";
  parent = CStackDirectory::GetParentPath(path);
  EXPECT_EQ(parent, "/");
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
         "PLAYLIST/00800.mpls"},
};

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
