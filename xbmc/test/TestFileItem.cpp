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

const TestFileData BaseMovies[] = {
    {"c:\\dir\\filename.avi", false, "c:\\dir\\filename.avi"},
    {"c:\\dir\\filename.avi", true, "c:\\dir\\"},
    {"/dir/filename.avi", false, "/dir/filename.avi"},
    {"/dir/filename.avi", true, "/dir/"},
    {"smb://somepath/file.avi", false, "smb://somepath/file.avi"},
    {"smb://somepath/file.avi", true, "smb://somepath/"},
    {"stack:///path/to/movie_name/cd1/some_file1.avi , /path/to/movie_name/cd2/some_file2.avi",
     false,
     "stack:///path/to/movie_name/cd1/some_file1.avi , /path/to/movie_name/cd2/some_file2.avi"},
    {"stack:///path/to/movie_name/cd1/some_file1.avi , /path/to/movie_name/cd2/some_file2.avi",
     true, "/path/to/movie_name/"},
    {"/home/user/TV Shows/Dexter/S1/1x01.avi", false, "/home/user/TV Shows/Dexter/S1/1x01.avi"},
    {"/home/user/TV Shows/Dexter/S1/1x01.avi", true, "/home/user/TV Shows/Dexter/S1/"},
    {"zip://g%3a%5cmultimedia%5cmovies%5cSphere%2ezip/Sphere.avi", true,
     "g:\\multimedia\\movies\\"},
    {"/home/user/movies/movie_name/video_ts/VIDEO_TS.IFO", false, "/home/user/movies/movie_name/"},
    {"/home/user/movies/movie_name/video_ts/VIDEO_TS.IFO", true, "/home/user/movies/movie_name/"},
    {"/home/user/movies/movie_name/BDMV/index.bdmv", false, "/home/user/movies/movie_name/"},
    {"/home/user/movies/movie_name/BDMV/index.bdmv", true, "/home/user/movies/movie_name/"},
    {"/home/user/movies/movie_name/disc 1/BDMV/index.bdmv", false, "/home/user/movies/movie_name/"},
    {"/home/user/movies/movie_name/disc 1/BDMV/index.bdmv", true, "/home/user/movies/movie_name/"},
    {"bluray://udf%3a%2f%2fsmb%253a%252f%252fsomepath%252fmovie.iso%2f/BDMV/PLAYLIST/00800.mpls",
     true, "smb://somepath/"},
    {"bluray://smb%3a%2f%2fsomepath%2fDisc%201%2f/BDMV/PLAYLIST/00800.mpls", true,
     "smb://somepath/"},
    {"zip://smb%3a%2f%2fsomepath%2fmovie.zip/BDMV/index.bdmv", true, "smb://somepath/"}};

INSTANTIATE_TEST_SUITE_P(BaseNameMovies, TestFileItemBasePath, ValuesIn(BaseMovies));
