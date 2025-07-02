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

struct TestNameData
{
  const char* file;
  bool use_folder;
  const char* name;
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

class TestFileItemMovieName : public AdvancedSettingsResetBase,
                              public WithParamInterface<TestNameData>
{
};

const TestFileData BaseMovies[] = {
    {"/home/user/movies/movie/disc 1/video_ts/VIDEO_TS.IFO", false, "/home/user/movies/movie/"},
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
    {"/home/user/movies/movie/video_ts/VIDEO_TS.IFO", false, "/home/user/movies/movie/"},
    {"/home/user/movies/movie/video_ts/VIDEO_TS.IFO", true, "/home/user/movies/movie/"},
    {"/home/user/movies/movie/disc 1/video_ts/VIDEO_TS.IFO", false, "/home/user/movies/movie/"},
    {"/home/user/movies/movie/disc 1/video_ts/VIDEO_TS.IFO", true, "/home/user/movies/movie/"},
    {"/home/user/movies/movie/BDMV/index.bdmv", false, "/home/user/movies/movie/"},
    {"/home/user/movies/movie/BDMV/index.bdmv", true, "/home/user/movies/movie/"},
    {"/home/user/movies/movie/disc 1/BDMV/index.bdmv", false, "/home/user/movies/movie/"},
    {"/home/user/movies/movie/disc 1/BDMV/index.bdmv", true, "/home/user/movies/movie/"},
    {"/home/user/movies/movie/movie.iso", false, "/home/user/movies/movie/movie.iso"},
    {"/home/user/movies/movie/file.iso", true, "/home/user/movies/movie/"},
    {"/home/user/movies/movie/disc 1/movie.iso", false, "/home/user/movies/movie/disc 1/movie.iso"},
    {"/home/user/movies/movie/disc 1/file.iso", true, "/home/user/movies/movie/"},
    {"bluray://udf%3a%2f%2fsmb%253a%252f%252fsomepath%252fmovie.iso%2f/BDMV/PLAYLIST/00800.mpls",
     true, "smb://somepath/"},
    {"bluray://udf%3a%2f%2fsmb%253a%252f%252fsomepath%252fDisc%201%252fmovie.iso%2f/BDMV/PLAYLIST/"
     "00800.mpls",
     true, "smb://somepath/"},
    {"bluray://smb%3a%2f%2fsomepath%2f/BDMV/PLAYLIST/00800.mpls", true, "smb://somepath/"},
    {"bluray://smb%3a%2f%2fsomepath%2fDisc%201%2f/BDMV/PLAYLIST/00800.mpls", true,
     "smb://somepath/"},
    {"zip://smb%3a%2f%2fsomepath%2fmovie.zip/BDMV/index.bdmv", true, "smb://somepath/"}};

TEST_P(TestFileItemBasePath, GetBaseMoviePath)
{
  CFileItem item;
  item.SetPath(GetParam().file);
  std::string path = CURL(item.GetBaseMoviePath(GetParam().use_folder)).Get();
  std::string compare = CURL(GetParam().base).Get();
  EXPECT_EQ(compare, path);
}

INSTANTIATE_TEST_SUITE_P(BaseNameMovies, TestFileItemBasePath, ValuesIn(BaseMovies));

const TestNameData BaseNames[] = {
    {"c:\\dir\\movie.avi", false, "movie"},
    {"c:\\movie\\filename.avi", true, "movie"},
    {"/dir/movie.avi", false, "movie"},
    {"/movie/filename.avi", true, "movie"},
    {"smb://somepath/movie.avi", false, "movie"},
    {"smb://somepath/movie/file.avi", true, "movie"},
    {"/home/user/movies/movie/video_ts/VIDEO_TS.IFO", false, "movie"},
    {"/home/user/movies/movie/video_ts/VIDEO_TS.IFO", true, "movie"},
    {"/home/user/movies/movie/disc 1/video_ts/VIDEO_TS.IFO", false, "movie"},
    {"/home/user/movies/movie/disc 1/video_ts/VIDEO_TS.IFO", true, "movie"},
    {"/home/user/movies/movie/BDMV/index.bdmv", false, "movie"},
    {"/home/user/movies/movie/BDMV/index.bdmv", true, "movie"},
    {"/home/user/movies/movie/disc 1/BDMV/index.bdmv", false, "movie"},
    {"/home/user/movies/movie/disc 1/BDMV/index.bdmv", true, "movie"},
    {"/home/user/movies/movie.iso", false, "movie"},
    {"/home/user/movies/movie/file.iso", true, "movie"},
    {"/home/user/movies/disc 1/movie.iso", false, "movie"},
    {"/home/user/movies/movie/disc 1/file.iso", true, "movie"}};

TEST_P(TestFileItemMovieName, GetMovieName)
{
  CFileItem item;
  item.SetPath(GetParam().file);
  std::string path = CURL(item.GetMovieName(GetParam().use_folder)).Get();
  std::string compare = CURL(GetParam().name).Get();
  EXPECT_EQ(compare, path);
}

INSTANTIATE_TEST_SUITE_P(NameMovies, TestFileItemMovieName, ValuesIn(BaseNames));
