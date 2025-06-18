/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FileItem.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "settings/lib/SettingsManager.h"

#include <gtest/gtest.h>

using ::testing::Test;
using ::testing::WithParamInterface;
using ::testing::ValuesIn;

struct TestFileData
{
  const char *file;
  bool use_folder;
  const char *base;
};

class AdvancedSettingsResetBase : public Test
{
public:
  AdvancedSettingsResetBase();
};

AdvancedSettingsResetBase::AdvancedSettingsResetBase()
{
  // Force all advanced settings to be reset to defaults
  const auto settings = CServiceBroker::GetSettingsComponent();
  CSettingsManager* settingsMgr = settings->GetSettings()->GetSettingsManager();
  settings->GetAdvancedSettings()->Uninitialize(*settingsMgr);
  settings->GetAdvancedSettings()->Initialize(*settingsMgr);
}

class TestFileItemBasePath : public AdvancedSettingsResetBase,
                             public WithParamInterface<TestFileData>
{
};

TEST_P(TestFileItemBasePath, GetBaseMoviePath)
{
  CFileItem item;
  item.SetPath(GetParam().file);
  std::string path = CURL(item.GetBaseMoviePath(GetParam().use_folder)).Get();
  std::string compare = CURL(GetParam().base).Get();
  EXPECT_EQ(compare, path);
}

const TestFileData BaseMovies[] = {{ "c:\\dir\\filename.avi", false, "c:\\dir\\filename.avi" },
                                   { "c:\\dir\\filename.avi", true,  "c:\\dir\\" },
                                   { "/dir/filename.avi", false, "/dir/filename.avi" },
                                   { "/dir/filename.avi", true,  "/dir/" },
                                   { "smb://somepath/file.avi", false, "smb://somepath/file.avi" },
                                   { "smb://somepath/file.avi", true, "smb://somepath/" },
                                   { "stack:///path/to/movie_name/cd1/some_file1.avi , /path/to/movie_name/cd2/some_file2.avi", false, "stack:///path/to/movie_name/cd1/some_file1.avi , /path/to/movie_name/cd2/some_file2.avi" },
                                   { "stack:///path/to/movie_name/cd1/some_file1.avi , /path/to/movie_name/cd2/some_file2.avi", true,  "/path/to/movie_name/" },
                                   { "/home/user/TV Shows/Dexter/S1/1x01.avi", false, "/home/user/TV Shows/Dexter/S1/1x01.avi" },
                                   { "/home/user/TV Shows/Dexter/S1/1x01.avi", true, "/home/user/TV Shows/Dexter/S1/" },
                                   { "zip://g%3a%5cmultimedia%5cmovies%5cSphere%2ezip/Sphere.avi", true, "g:\\multimedia\\movies\\" },
                                   { "/home/user/movies/movie_name/video_ts/VIDEO_TS.IFO", false, "/home/user/movies/movie_name/" },
                                   { "/home/user/movies/movie_name/video_ts/VIDEO_TS.IFO", true, "/home/user/movies/movie_name/" },
                                   { "/home/user/movies/movie_name/BDMV/index.bdmv", false, "/home/user/movies/movie_name/" },
                                   { "/home/user/movies/movie_name/BDMV/index.bdmv", true, "/home/user/movies/movie_name/" }};

INSTANTIATE_TEST_SUITE_P(BaseNameMovies, TestFileItemBasePath, ValuesIn(BaseMovies));

TEST(TestFileItem, TestSimplePathSet)
{
  CFileItem item;
  item.SetPath("/local/path/regular/file.txt");
  item.SetDynPath("/local/path/dynamic/file.txt");

  EXPECT_EQ("/local/path/regular/file.txt", item.GetURL().Get());
  EXPECT_EQ("/local/path/dynamic/file.txt", item.GetDynURL().Get());
  EXPECT_EQ("/local/path/regular/file.txt", item.GetURLRef().Get());
  EXPECT_EQ("/local/path/dynamic/file.txt", item.GetDynURLRef().Get());
}

TEST(TestFileItem, TestLabel)
{
  CFileItem item("My Item Label");
  item.SetPath("/local/path/file.txt");

  EXPECT_EQ("My Item Label", item.GetLabel());
  EXPECT_EQ("/local/path/file.txt", item.GetURL().Get());
  EXPECT_EQ("/local/path/file.txt", item.GetDynURL().Get());
  EXPECT_EQ("/local/path/file.txt", item.GetURLRef().Get());
  EXPECT_EQ("/local/path/file.txt", item.GetDynURLRef().Get());
}

TEST(TestFileItem, TestCURLConstructor)
{
  CFileItem folderItem(CURL("/local/path/file.txt"), true);

  EXPECT_EQ("", folderItem.GetLabel());
  EXPECT_EQ("/local/path/file.txt/", folderItem.GetURL().Get());
  EXPECT_EQ("/local/path/file.txt/", folderItem.GetDynURL().Get());
  EXPECT_EQ("/local/path/file.txt/", folderItem.GetURLRef().Get());
  EXPECT_EQ("/local/path/file.txt/", folderItem.GetDynURLRef().Get());

  CFileItem fileItem(CURL("/local/path/file.txt"), false);

  EXPECT_EQ("", fileItem.GetLabel());
  EXPECT_EQ("/local/path/file.txt", fileItem.GetURL().Get());
  EXPECT_EQ("/local/path/file.txt", fileItem.GetDynURL().Get());
  EXPECT_EQ("/local/path/file.txt", fileItem.GetURLRef().Get());
  EXPECT_EQ("/local/path/file.txt", fileItem.GetDynURLRef().Get());
}

TEST(TestFileItem, TestAssignment)
{
  CFileItem itemToCopy("My Item Label");
  itemToCopy.SetPath("/local/path/file.txt");

  CFileItem item("Alternative Label");
  item = itemToCopy;

  EXPECT_EQ("My Item Label", item.GetLabel());
  EXPECT_EQ("/local/path/file.txt", item.GetURL().Get());
  EXPECT_EQ("/local/path/file.txt", item.GetDynURL().Get());
  EXPECT_EQ("/local/path/file.txt", item.GetURLRef().Get());
  EXPECT_EQ("/local/path/file.txt", item.GetDynURLRef().Get());

  item.Reset();
  EXPECT_EQ("", item.GetLabel());
  EXPECT_EQ("", item.GetURL().Get());
  EXPECT_EQ("", item.GetDynURL().Get());
  EXPECT_EQ("", item.GetURLRef().Get());
  EXPECT_EQ("", item.GetDynURLRef().Get());
}

TEST(TestFileItem, MimeType)
{
  CFileItem item("Internet Movies List");
  item.SetPath("http://testdomain.com/api/movies");

  item.FillInMimeType(true);
  EXPECT_EQ("Internet Movies List", item.GetLabel());
  EXPECT_EQ("http://testdomain.com/api/movies", item.GetURL().Get());
  EXPECT_EQ("http://testdomain.com/api/movies", item.GetDynURL().Get());
  EXPECT_EQ("http://testdomain.com/api/movies", item.GetURLRef().Get());
  EXPECT_EQ("http://testdomain.com/api/movies", item.GetDynURLRef().Get());

  item.FillInMimeType(false);
  EXPECT_EQ("Internet Movies List", item.GetLabel());
  EXPECT_EQ("http://testdomain.com/api/movies", item.GetURL().Get());
  EXPECT_EQ("http://testdomain.com/api/movies", item.GetDynURL().Get());
  EXPECT_EQ("http://testdomain.com/api/movies", item.GetURLRef().Get());
  EXPECT_EQ("http://testdomain.com/api/movies", item.GetDynURLRef().Get());
}
