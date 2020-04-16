/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "URL.h"
#include "filesystem/NFSFile.h"
#include "test/TestUtils.h"

#include <errno.h>
#include <string>

#include <gtest/gtest.h>

using ::testing::Test;
using ::testing::WithParamInterface;
using ::testing::ValuesIn;

struct SplitPath
{
  std::string url;
  std::string exportPath;
  std::string relativePath;
  bool expectedResultExport;
  bool expectedResultPath;
} g_TestData[] = {
                   {"nfs://192.168.0.1:2049/srv/test/tvmedia/foo.txt", "/srv/test", "//tvmedia/foo.txt", true, true},
                   {"nfs://192.168.0.1/srv/test/tv/media/foo.txt", "/srv/test/tv", "//media/foo.txt", true, true},
                   {"nfs://192.168.0.1:2049/srv/test/tvmedia", "/srv/test", "//tvmedia", true, true},
                   {"nfs://192.168.0.1:2049/srv/test/tvmedia/", "/srv/test", "//tvmedia/", true, true},
                   {"nfs://192.168.0.1:2049/srv/test/tv/media", "/srv/test/tv", "//media", true, true},
                   {"nfs://192.168.0.1:2049/srv/test/tv/media/", "/srv/test/tv", "//media/", true, true},
                   {"nfs://192.168.0.1:2049/srv/test/tv", "/srv/test/tv", "//", true, true},
                   {"nfs://192.168.0.1:2049/srv/test/", "/srv/test", "//", true, true},
                   {"nfs://192.168.0.1:2049/", "/", "//", true, true},
                   {"nfs://192.168.0.1:2049/notexported/foo.txt", "/", "//notexported/foo.txt", true, true},

                   {"nfs://192.168.0.1:2049/notexported/foo.txt", "/notexported", "//foo.txt", false, false},
                 };

class TestNfs : public Test,
                public WithParamInterface<SplitPath>
{
};

class ExportList
{
  public:
    std::list<std::string> data;

    ExportList()
    {
      data.emplace_back("/srv/test");
      data.emplace_back("/srv/test/tv");
      data.emplace_back("/");
      data.sort();
      data.reverse();
    }
};

static ExportList exportList;

TEST_P(TestNfs, splitUrlIntoExportAndPath)
{
  CURL url(GetParam().url);
  std::string exportPath;
  std::string relativePath;
  gNfsConnection.splitUrlIntoExportAndPath(url, exportPath, relativePath, exportList.data);

  if (GetParam().expectedResultExport)
    EXPECT_STREQ(GetParam().exportPath.c_str(), exportPath.c_str());
  else
    EXPECT_STRNE(GetParam().exportPath.c_str(), exportPath.c_str());

  if (GetParam().expectedResultPath)
    EXPECT_STREQ(GetParam().relativePath.c_str(), relativePath.c_str());
  else
    EXPECT_STRNE(GetParam().relativePath.c_str(), relativePath.c_str());
}

INSTANTIATE_TEST_SUITE_P(NfsFile, TestNfs, ValuesIn(g_TestData));
