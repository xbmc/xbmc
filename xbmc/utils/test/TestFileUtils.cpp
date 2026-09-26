/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FileItem.h"
#include "filesystem/File.h"
#include "test/TestUtils.h"
#include "utils/FileUtils.h"
#include "utils/URIUtils.h"

#include <gtest/gtest.h>

TEST(TestFileUtils, DeleteItem_CFileItemPtr)
{
  XFILE::CFile *tmpfile;
  std::string tmpfilepath;

  ASSERT_NE(nullptr, (tmpfile = XBMC_CREATETEMPFILE("")));
  tmpfilepath = XBMC_TEMPFILEPATH(tmpfile);

  CFileItemPtr item(new CFileItem(tmpfilepath));
  item->SetPath(tmpfilepath);
  item->SetFolder(false);
  item->Select(true);
  tmpfile->Close();  //Close tmpfile before we try to delete it
  EXPECT_TRUE(CFileUtils::DeleteItem(item));
  EXPECT_FALSE(XBMC_DELETETEMPFILE(tmpfile));
}

TEST(TestFileUtils, DeleteItemString)
{
  XFILE::CFile *tmpfile;

  ASSERT_NE(nullptr, (tmpfile = XBMC_CREATETEMPFILE("")));
  tmpfile->Close();  //Close tmpfile before we try to delete it
  EXPECT_TRUE(CFileUtils::DeleteItem(XBMC_TEMPFILEPATH(tmpfile)));
  EXPECT_FALSE(XBMC_DELETETEMPFILE(tmpfile));
}

/* Executing RenameFile() requires input from the user */
// static bool RenameFile(const std::string &strFile);

TEST(TestFileUtils, GetModificationDateOfDiscImagePlaylist)
{
  // A playlist within a disc image, alone or as the first part of a stack, has the image's date
  const std::string image{XBMC_REF_FILE_PATH(
      "xbmc/video/test/testdata/moviestack_blurayiso/Movie_(2001)/Movie_(2001)_part1.iso")};
  const std::string playlist{URIUtils::GetBlurayPlaylistPath(image, 1003)};
  const std::string otherPlaylist{URIUtils::GetBlurayPlaylistPath(
      XBMC_REF_FILE_PATH(
          "xbmc/video/test/testdata/moviestack_blurayiso/Movie_(2001)/Movie_(2001)_part2.iso"),
      1003)};
  ASSERT_TRUE(URIUtils::IsBlurayPath(playlist));

  const CDateTime imageDate{CFileUtils::GetModificationDate(0, image)};
  ASSERT_TRUE(imageDate.IsValid());
  EXPECT_EQ(CFileUtils::GetModificationDate(0, playlist), imageDate);
  EXPECT_EQ(CFileUtils::GetModificationDate(0, "stack://" + playlist + " , " + otherPlaylist),
            imageDate);
}
