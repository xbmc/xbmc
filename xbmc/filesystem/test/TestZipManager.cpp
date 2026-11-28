/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "filesystem/File.h"
#include "filesystem/ZipManager.h"
#include "test/TestUtils.h"
#include "URL.h"
#include "utils/RegExp.h"
#include "utils/URIUtils.h"

#include <vector>

#include <gtest/gtest.h>

TEST(TestZipManager, TruncatedFileRejected)
{
  XFILE::CFile* file{XBMC_CREATETEMPFILE(".zip")};
  ASSERT_NE(nullptr, file);
  file->Close();
  ASSERT_TRUE(file->OpenForWrite(XBMC_TEMPFILEPATH(file), true));
  const char contents[ECDREC_SIZE - 1]{'P', 'K', 3, 4};
  ASSERT_EQ(static_cast<ssize_t>(sizeof(contents)),
            file->Write(contents, sizeof(contents)));
  file->Close();

  const CURL zipUrl{URIUtils::CreateArchivePath("zip", CURL(XBMC_TEMPFILEPATH(file)), "")};
  std::vector<SZipEntry> items;
  EXPECT_FALSE(g_ZipManager.GetZipList(zipUrl, items));

  EXPECT_TRUE(XBMC_DELETETEMPFILE(file));
}

TEST(TestZipManager, PathTraversal)
{
  CRegExp pathTraversal;
  pathTraversal.RegComp(PATH_TRAVERSAL);

  ASSERT_TRUE(pathTraversal.RegFind("..") >= 0);
  ASSERT_TRUE(pathTraversal.RegFind("../test.txt") >= 0);
  ASSERT_TRUE(pathTraversal.RegFind("..\\test.txt") >= 0);
  ASSERT_TRUE(pathTraversal.RegFind("test/../test.txt") >= 0);
  ASSERT_TRUE(pathTraversal.RegFind("test\\../test.txt") >= 0);
  ASSERT_TRUE(pathTraversal.RegFind("test\\..\\test.txt") >= 0);

  ASSERT_FALSE(pathTraversal.RegFind("...") >= 0);
  ASSERT_FALSE(pathTraversal.RegFind("..test.txt") >= 0);
  ASSERT_FALSE(pathTraversal.RegFind("test.txt..") >= 0);
  ASSERT_FALSE(pathTraversal.RegFind("test..test.txt") >= 0);
}
