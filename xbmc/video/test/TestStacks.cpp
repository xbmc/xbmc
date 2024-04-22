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
#include "test/TestUtils.h"

#include <string>

#include <gtest/gtest.h>

using namespace XFILE;

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
  EXPECT_EQ(items.Get(0)->IsStack(), true);
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
  EXPECT_EQ(items.Get(0)->IsStack(), true);
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
  EXPECT_EQ(items.Get(0)->IsStack(), true);
}
