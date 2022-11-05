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

#include <gtest/gtest.h>

TEST(TestFileUtils, DeleteItem_CFileItemPtr)
{
  XFILE::CFile *tmpfile;
  std::string tmpfilepath;

  ASSERT_NE(nullptr, (tmpfile = XBMC_CREATETEMPFILE("")));
  tmpfilepath = XBMC_TEMPFILEPATH(tmpfile);

  CFileItemPtr item(new CFileItem(tmpfilepath));
  item->SetPath(tmpfilepath);
  item->m_bIsFolder = false;
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
