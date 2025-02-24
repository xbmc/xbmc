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
using ::testing::ValuesIn;
using ::testing::WithParamInterface;

struct TestFileData
{
  const char* file;
  bool use_folder;
  const char* base;
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

class TestFileItemLocalMetadataPath : public AdvancedSettingsResetBase,
                                      public WithParamInterface<TestFileData>
{
};

const TestFileData BaseMovies[] = {
    // Linux path tests
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
    {"stack:///path/to/movie_name/cd1/some_file1.avi , /path/to/movie_name/cd2/some_file2.avi",
     false,
     "stack:///path/to/movie_name/cd1/some_file1.avi , /path/to/movie_name/cd2/some_file2.avi"},
    {"stack:///path/to/movie_name/cd1/some_file1.avi , /path/to/movie_name/cd2/some_file2.avi",
     true, "/path/to/movie_name/"},
    // DOS path tests
    {"D:\\Movies\\Movie\\video_ts\\VIDEO_TS.IFO", false, "D:\\Movies\\Movie\\"},
    {"D:\\Movies\\Movie\\video_ts\\VIDEO_TS.IFO", true, "D:\\Movies\\Movie\\"},
    {"D:\\Movies\\Movie\\disc 1\\video_ts\\VIDEO_TS.IFO", false, "D:\\Movies\\Movie\\"},
    {"D:\\Movies\\Movie\\disc 1\\video_ts\\VIDEO_TS.IFO", true, "D:\\Movies\\Movie\\"},
    {"D:\\Movies\\Movie\\BDMV\\index.bdmv", false, "D:\\Movies\\Movie\\"},
    {"D:\\Movies\\Movie\\BDMV\\index.bdmv", true, "D:\\Movies\\Movie\\"},
    {"D:\\Movies\\Movie\\disc 1\\BDMV\\index.bdmv", false, "D:\\Movies\\Movie\\"},
    {"D:\\Movies\\Movie\\disc 1\\BDMV\\index.bdmv", true, "D:\\Movies\\Movie\\"},
    {"D:\\Movies\\Movie\\movie.iso", false, "D:\\Movies\\Movie\\movie.iso"},
    {"D:\\Movies\\Movie\\file.iso", true, "D:\\Movies\\Movie\\"},
    {"D:\\Movies\\Movie\\disc 1\\movie.iso", false, "D:\\Movies\\Movie\\disc 1\\movie.iso"},
    {"D:\\Movies\\Movie\\disc 1\\file.iso", true, "D:\\Movies\\Movie\\"},
    {"stack://D:\\Movies\\Movie\\Movie - part 1\\some_file1.avi , D:\\Movies\\Movie\\Movie - part "
     "2\\some_file2.avi",
     false,
     "stack://D:\\Movies\\Movie\\Movie - part 1\\some_file1.avi , D:\\Movies\\Movie\\Movie - part "
     "2\\some_file2.avi"},
    {"stack://D:\\Movies\\Movie\\Movie - part 1\\some_file1.avi , D:\\Movies\\Movie\\Movie - part "
     "2\\some_file2.avi",
     true, "D:\\Movies\\Movie\\"},
    // Windows server path tests
    {"\\\\Server\\Movies\\Movie\\video_ts\\VIDEO_TS.IFO", false, "\\\\Server\\Movies\\Movie\\"},
    {"\\\\Server\\Movies\\Movie\\video_ts\\VIDEO_TS.IFO", true, "\\\\Server\\Movies\\Movie\\"},
    {"\\\\Server\\Movies\\Movie\\disc 1\\video_ts\\VIDEO_TS.IFO", false,
     "\\\\Server\\Movies\\Movie\\"},
    {"\\\\Server\\Movies\\Movie\\disc 1\\video_ts\\VIDEO_TS.IFO", true,
     "\\\\Server\\Movies\\Movie\\"},
    {"\\\\Server\\Movies\\Movie\\BDMV\\index.bdmv", false, "\\\\Server\\Movies\\Movie\\"},
    {"\\\\Server\\Movies\\Movie\\BDMV\\index.bdmv", true, "\\\\Server\\Movies\\Movie\\"},
    {"\\\\Server\\Movies\\Movie\\disc 1\\BDMV\\index.bdmv", false, "\\\\Server\\Movies\\Movie\\"},
    {"\\\\Server\\Movies\\Movie\\disc 1\\BDMV\\index.bdmv", true, "\\\\Server\\Movies\\Movie\\"},
    {"\\\\Server\\Movies\\Movie\\movie.iso", false, "\\\\Server\\Movies\\Movie\\movie.iso"},
    {"\\\\Server\\Movies\\Movie\\file.iso", true, "\\\\Server\\Movies\\Movie\\"},
    {"\\\\Server\\Movies\\Movie\\disc 1\\movie.iso", false,
     "\\\\Server\\Movies\\Movie\\disc 1\\movie.iso"},
    {"\\\\Server\\Movies\\Movie\\disc 1\\file.iso", true, "\\\\Server\\Movies\\Movie\\"},
    {"stack://\\\\Server\\Movies\\Movie\\Movie - part 1\\some_file1.avi , "
     "\\\\Server\\Movies\\Movie\\Movie - part "
     "2\\some_file2.avi",
     false,
     "stack://\\\\Server\\Movies\\Movie\\Movie - part 1\\some_file1.avi , "
     "\\\\Server\\Movies\\Movie\\Movie - part "
     "2\\some_file2.avi"},
    {"stack://\\\\Server\\Movies\\Movie\\Movie - part 1\\some_file1.avi , "
     "\\\\Server\\Movies\\Movie\\Movie - part "
     "2\\some_file2.avi",
     true, "\\\\Server\\Movies\\Movie\\"},
    // URL path tests with smb://
    {"smb://home/user/movies/movie/video_ts/VIDEO_TS.IFO", false, "smb://home/user/movies/movie/"},
    {"smb://home/user/movies/movie/video_ts/VIDEO_TS.IFO", true, "smb://home/user/movies/movie/"},
    {"smb://home/user/movies/movie/disc 1/video_ts/VIDEO_TS.IFO", false,
     "smb://home/user/movies/movie/"},
    {"smb://home/user/movies/movie/disc 1/video_ts/VIDEO_TS.IFO", true,
     "smb://home/user/movies/movie/"},
    {"smb://home/user/movies/movie/BDMV/index.bdmv", false, "smb://home/user/movies/movie/"},
    {"smb://home/user/movies/movie/BDMV/index.bdmv", true, "smb://home/user/movies/movie/"},
    {"smb://home/user/movies/movie/disc 1/BDMV/index.bdmv", false, "smb://home/user/movies/movie/"},
    {"smb://home/user/movies/movie/disc 1/BDMV/index.bdmv", true, "smb://home/user/movies/movie/"},
    {"smb://home/user/movies/movie/movie.iso", false, "smb://home/user/movies/movie/movie.iso"},
    {"smb://home/user/movies/movie/file.iso", true, "smb://home/user/movies/movie/"},
    {"smb://home/user/movies/movie/disc 1/movie.iso", false,
     "smb://home/user/movies/movie/disc 1/movie.iso"},
    {"smb://home/user/movies/movie/disc 1/file.iso", true, "smb://home/user/movies/movie/"},
    {"stack://smb://path/to/movie_name/cd1/some_file1.avi , "
     "smb://path/to/movie_name/cd2/some_file2.avi",
     false,
     "stack://smb://path/to/movie_name/cd1/some_file1.avi , "
     "smb://path/to/movie_name/cd2/some_file2.avi"},
    {"stack://smb://path/to/movie_name/cd1/some_file1.avi , "
     "smb://path/to/movie_name/cd2/some_file2.avi",
     true, "smb://path/to/movie_name/"},
    // Embedded smb:// path tests
    {"bluray://udf%3a%2f%2fsmb%253a%252f%252fsomepath%252fmovie.iso%2f/BDMV/PLAYLIST/"
     "00800.mpls",
     false, "smb://somepath/movie.iso"},
    {"bluray://udf%3a%2f%2fsmb%253a%252f%252fsomepath%252fmovie.iso%2f/BDMV/PLAYLIST/"
     "00800.mpls",
     true, "smb://somepath/"},
    {"bluray://udf%3a%2f%2fsmb%253a%252f%252fsomepath%252fDisc%25201%252fmovie.iso%2f/BDMV/"
     "PLAYLIST/"
     "00800.mpls",
     true, "smb://somepath/"},
    {"bluray://smb%3a%2f%2fsomepath%2f/BDMV/PLAYLIST/00800.mpls", false, "smb://somepath/"},
    {"bluray://smb%3a%2f%2fsomepath%2f/BDMV/PLAYLIST/00800.mpls", true, "smb://somepath/"},
    {"bluray://smb%3a%2f%2fsomepath%2fDisc%201%2f/BDMV/PLAYLIST/00800.mpls", true,
     "smb://somepath/"},
    {"zip://smb%3a%2f%2fsomepath%2fmovie.zip/BDMV/index.bdmv", false, "smb://somepath/movie.zip"},
    {"zip://smb%3a%2f%2fsomepath%2fmovie.zip/BDMV/index.bdmv", true, "smb://somepath/"},
    {"zip://smb%3a%2f%2fsomepath%2fmovie%2fdisc%201%2fmovie_disc.zip/BDMV/index.bdmv", true,
     "smb://somepath/movie/"},
    {"rar://smb%3a%2f%2fsomepath%2fmovie.rar/BDMV/index.bdmv", false, "smb://somepath/movie.rar"},
    {"rar://smb%3a%2f%2fsomepath%2fmovie.rar/BDMV/index.bdmv", true, "smb://somepath/"},
    {"rar://smb%3a%2f%2fsomepath%2fmovie%2fdisc%201%2fmovie_disc.rar/BDMV/index.bdmv", true,
     "smb://somepath/movie/"},
    {"archive://smb%3a%2f%2fsomepath%2fmovie.tar.gz/BDMV/index.bdmv", false,
     "smb://somepath/movie.tar.gz"},
    {"archive://smb%3a%2f%2fsomepath%2fmovie.tar.gz/BDMV/index.bdmv", true, "smb://somepath/"},
    {"archive://smb%3a%2f%2fsomepath%2fmovie%2fdisc%201%2fmovie_disc.tar.gz/BDMV/index.bdmv", true,
     "smb://somepath/movie/"},
    // Embedded linux path tests
    {"bluray://udf%3a%2f%2f%252fsomepath%252fmovie.iso%2f/BDMV/PLAYLIST/00800.mpls", false,
     "/somepath/movie.iso"},
    {"bluray://udf%3a%2f%2f%252fsomepath%252fmovie.iso%2f/BDMV/PLAYLIST/00800.mpls", true,
     "/somepath/"},
    {"bluray://udf%3a%2f%2f%252fsomepath%252fDisc%25201%252fmovie.iso%2f/BDMV/"
     "PLAYLIST/"
     "00800.mpls",
     true, "/somepath/"},
    {"bluray://%2fsomepath%2f/BDMV/PLAYLIST/00800.mpls", false, "/somepath/"},
    {"bluray://%2fsomepath%2f/BDMV/PLAYLIST/00800.mpls", true, "/somepath/"},
    {"bluray://%2fsomepath%2fDisc%201%2f/BDMV/PLAYLIST/00800.mpls", true, "/somepath/"},
    {"zip://%2fsomepath%2fmovie.zip/BDMV/index.bdmv", false, "/somepath/movie.zip"},
    {"zip://%2fsomepath%2fmovie.zip/BDMV/index.bdmv", true, "/somepath/"},
    {"zip://%2fsomepath%2fdisc%201%2fmovie.zip/BDMV/index.bdmv", true, "/somepath/"},
    {"rar://%2fsomepath%2fmovie.rar/BDMV/index.bdmv", false, "/somepath/movie.rar"},
    {"rar://%2fsomepath%2fmovie.rar/BDMV/index.bdmv", true, "/somepath/"},
    {"rar://%2fsomepath%2fdisc%201%2fmovie_disc.rar/BDMV/index.bdmv", true, "/somepath/"},
    {"archive://%2fsomepath%2fmovie.tar.gz/BDMV/index.bdmv", false, "/somepath/movie.tar.gz"},
    {"archive://%2fsomepath%2fmovie.tar.gz/BDMV/index.bdmv", true, "/somepath/"},
    {"archive://%2fsomepath%2fdisc%201%2fmovie.tar.gz/BDMV/index.bdmv", true, "/somepath/"},
    // Embedded DOS path tests
    {"bluray://udf%3a%2f%2fD%253a%255cmovies%255cmovie.iso%2f/BDMV/PLAYLIST/00800.mpls", false,
     "D:\\movies\\movie.iso"},
    {"bluray://udf%3a%2f%2fD%253a%255cmovies%255cmovie.iso%2f/BDMV/PLAYLIST/00800.mpls", true,
     "D:\\movies\\"},
    {"bluray://udf%3a%2f%2fD%253a%255cmovies%255cDisc%25201%255cmovie.iso%2f/BDMV/"
     "PLAYLIST/"
     "00800.mpls",
     true, "D:\\movies\\"},
    {"bluray://D%3a%5cmovie%5c/BDMV/PLAYLIST/00800.mpls", false, "D:\\movie\\"},
    {"bluray://D%3a%5cmovie%5c/BDMV/PLAYLIST/00800.mpls", true, "D:\\movie\\"},
    {"bluray://D%3a%5cmovie%5cDisc%201%5c/BDMV/PLAYLIST/00800.mpls", true, "D:\\movie\\"},
    {"zip://D%3a%5cmovies%5cmovie.zip/BDMV/index.bdmv", false, "D:\\movies\\movie.zip"},
    {"zip://D%3a%5cmovies%5cmovie.zip/BDMV/index.bdmv", true, "D:\\movies\\"},
    {"zip://D%3a%5cmovies%5cmovie%5cdisc%201%5cmovie.zip/BDMV/index.bdmv", true,
     "D:\\movies\\movie\\"},
    {"rar://D%3a%5cmovies%5cmovie.rar/BDMV/index.bdmv", false, "D:\\movies\\movie.rar"},
    {"rar://D%3a%5cmovies%5cmovie.rar/BDMV/index.bdmv", true, "D:\\movies\\"},
    {"rar://D%3a%5cmovies%5cmovie%5cdisc%201%5cmovie.rar/BDMV/index.bdmv", true,
     "D:\\movies\\movie\\"},
    {"archive://D%3a%5cmovies%5cmovie.tar.gz/BDMV/index.bdmv", false, "D:\\movies\\movie.tar.gz"},
    {"archive://D%3a%5cmovies%5cmovie.tar.gz/BDMV/index.bdmv", true, "D:\\movies\\"},
    {"archive://D%3a%5cmovies%5cmovie%5cdisc%201%5cmovie.tar.gz/BDMV/index.bdmv", true,
     "D:\\movies\\movie\\"},
    // Embedded Windows server path tests
    {"bluray://udf%3a%2f%2f%255c%255cServer%255cMovies%255cmovie.iso%2f/BDMV/PLAYLIST/00800.mpls",
     false, "\\\\Server\\Movies\\movie.iso"},
    {"bluray://udf%3a%2f%2f%255c%255cServer%255cMovies%255cmovie.iso%2f/BDMV/PLAYLIST/00800.mpls",
     true, "\\\\Server\\Movies\\"},
    {"bluray://udf%3a%2f%2f%255c%255cServer%255cMovies%255cMovie%255cDisc%25201%255cmovie.iso%2f/"
     "BDMV/"
     "PLAYLIST/"
     "00800.mpls",
     true, "\\\\Server\\Movies\\Movie\\"},
    {"bluray://%5c%5cServer%5cMovies%5cMovie%5c/BDMV/PLAYLIST/00800.mpls", false,
     "\\\\Server\\Movies\\Movie\\"},
    {"bluray://%5c%5cServer%5cMovies%5cMovie%5c/BDMV/PLAYLIST/00800.mpls", true,
     "\\\\Server\\Movies\\Movie\\"},
    {"bluray://%5c%5cServer%5cMovies%5cMovie%5cDisc%201%5c/BDMV/PLAYLIST/00800.mpls", true,
     "\\\\Server\\Movies\\Movie\\"},
    {"zip://%5c%5cServer%5cMovies%5cmovie.zip/BDMV/index.bdmv", false,
     "\\\\Server\\Movies\\movie.zip"},
    {"zip://%5c%5cServer%5cMovies%5cmovie.zip/BDMV/index.bdmv", true, "\\\\Server\\Movies\\"},
    {"zip://%5c%5cServer%5cMovies%5cMovie%5cdisc%201%5cmovie.zip/BDMV/index.bdmv", true,
     "\\\\Server\\Movies\\Movie\\"},
    {"rar://%5c%5cServer%5cMovies%5cmovie.rar/BDMV/index.bdmv", false,
     "\\\\Server\\Movies\\movie.rar"},
    {"rar://%5c%5cServer%5cMovies%5cmovie.rar/BDMV/index.bdmv", true, "\\\\Server\\Movies\\"},
    {"rar://%5c%5cServer%5cMovies%5cMovie%5cdisc%201%5cmovie.rar/BDMV/index.bdmv", true,
     "\\\\Server\\Movies\\Movie\\"},
    {"archive://%5c%5cServer%5cMovies%5cmovie.tar.gz/BDMV/index.bdmv", false,
     "\\\\Server\\Movies\\movie.tar.gz"},
    {"archive://%5c%5cServer%5cMovies%5cmovie.tar.gz/BDMV/index.bdmv", true,
     "\\\\Server\\Movies\\"},
    {"archive://%5c%5cServer%5cMovies%5cMovie%5cdisc%201%5cmovie.tar.gz/BDMV/index.bdmv", true,
     "\\\\Server\\Movies\\Movie\\"}};

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
    // Linux path tests
    {"/home/user/movies/movie/video_ts/VIDEO_TS.IFO", false, "movie"},
    {"/home/user/movies/movie/video_ts/VIDEO_TS.IFO", true, "movie"},
    {"/home/user/movies/movie/disc 1/video_ts/VIDEO_TS.IFO", false, "movie"},
    {"/home/user/movies/movie/disc 1/video_ts/VIDEO_TS.IFO", true, "movie"},
    {"/home/user/movies/movie/BDMV/index.bdmv", false, "movie"},
    {"/home/user/movies/movie/BDMV/index.bdmv", true, "movie"},
    {"/home/user/movies/movie/disc 1/BDMV/index.bdmv", false, "movie"},
    {"/home/user/movies/movie/disc 1/BDMV/index.bdmv", true, "movie"},
    {"/home/user/movies/movie/movie.iso", false, "movie"},
    {"/home/user/movies/movie/file.iso", true, "movie"},
    {"/home/user/movies/movie/disc 1/movie.iso", false, "movie"},
    {"/home/user/movies/movie/disc 1/file.iso", true, "movie"},
    {"stack:///path/to/movie_folder/cd1/movie_file1.avi , "
     "/path/to/movie_folder/cd2/movie_file2.avi",
     false, "movie"},
    {"stack:///path/to/movie/cd1/some_file1.avi , /path/to/movie/cd2/some_file2.avi", true,
     "movie"},
    // DOS path tests
    {"D:\\Movies\\Movie\\video_ts\\VIDEO_TS.IFO", false, "Movie"},
    {"D:\\Movies\\Movie\\video_ts\\VIDEO_TS.IFO", true, "Movie"},
    {"D:\\Movies\\Movie\\disc 1\\video_ts\\VIDEO_TS.IFO", false, "Movie"},
    {"D:\\Movies\\Movie\\disc 1\\video_ts\\VIDEO_TS.IFO", true, "Movie"},
    {"D:\\Movies\\Movie\\BDMV\\index.bdmv", false, "Movie"},
    {"D:\\Movies\\Movie\\BDMV\\index.bdmv", true, "Movie"},
    {"D:\\Movies\\Movie\\disc 1\\BDMV\\index.bdmv", false, "Movie"},
    {"D:\\Movies\\Movie\\disc 1\\BDMV\\index.bdmv", true, "Movie"},
    {"D:\\Movies\\Movie\\movie.iso", false, "movie"},
    {"D:\\Movies\\Movie\\file.iso", true, "Movie"},
    {"D:\\Movies\\Movie\\disc 1\\movie.iso", false, "movie"},
    {"D:\\Movies\\Movie\\disc 1\\file.iso", true, "Movie"},
    {"stack://D:\\Movies\\Movie\\movie_file1.avi , D:\\Movies\\Movie\\Movie - part "
     "2\\movie_file2.avi",
     false, "movie"},
    {"stack://D:\\Movies\\Movie\\Movie - part 1\\some_file1.avi , D:\\Movies\\Movie\\Movie - part "
     "2\\some_file2.avi",
     true, "Movie"},
    // Windows server path tests
    {"\\\\Server\\Movies\\Movie\\video_ts\\VIDEO_TS.IFO", false, "Movie"},
    {"\\\\Server\\Movies\\Movie\\video_ts\\VIDEO_TS.IFO", true, "Movie"},
    {"\\\\Server\\Movies\\Movie\\disc 1\\video_ts\\VIDEO_TS.IFO", false, "Movie"},
    {"\\\\Server\\Movies\\Movie\\disc 1\\video_ts\\VIDEO_TS.IFO", true, "Movie"},
    {"\\\\Server\\Movies\\Movie\\BDMV\\index.bdmv", false, "Movie"},
    {"\\\\Server\\Movies\\Movie\\BDMV\\index.bdmv", true, "Movie"},
    {"\\\\Server\\Movies\\Movie\\disc 1\\BDMV\\index.bdmv", false, "Movie"},
    {"\\\\Server\\Movies\\Movie\\disc 1\\BDMV\\index.bdmv", true, "Movie"},
    {"\\\\Server\\Movies\\Movie\\movie.iso", false, "movie"},
    {"\\\\Server\\Movies\\Movie\\file.iso", true, "Movie"},
    {"\\\\Server\\Movies\\Movie\\disc 1\\movie.iso", false, "movie"},
    {"\\\\Server\\Movies\\Movie\\disc 1\\file.iso", true, "Movie"},
    {"stack://\\\\Server\\Movies\\Movie\\movie_file1.avi , "
     "\\\\Server\\Movies\\Movie\\movie_file2.avi",
     false, "movie"},
    {"stack://\\\\Server\\Movies\\Movie\\Movie - part 1\\some_file1.avi , "
     "\\\\Server\\Movies\\Movie\\Movie - part "
     "2\\some_file2.avi",
     true, "Movie"},
    // URL path tests with smb://
    {"smb://home/user/movies/movie/video_ts/VIDEO_TS.IFO", false, "movie"},
    {"smb://home/user/movies/movie/video_ts/VIDEO_TS.IFO", true, "movie"},
    {"smb://home/user/movies/movie/disc 1/video_ts/VIDEO_TS.IFO", false, "movie"},
    {"smb://home/user/movies/movie/disc 1/video_ts/VIDEO_TS.IFO", true, "movie"},
    {"smb://home/user/movies/movie/BDMV/index.bdmv", false, "movie"},
    {"smb://home/user/movies/movie/BDMV/index.bdmv", true, "movie"},
    {"smb://home/user/movies/movie/disc 1/BDMV/index.bdmv", false, "movie"},
    {"smb://home/user/movies/movie/disc 1/BDMV/index.bdmv", true, "movie"},
    {"smb://home/user/movies/movie/movie.iso", false, "movie"},
    {"smb://home/user/movies/movie/file.iso", true, "movie"},
    {"smb://home/user/movies/movie/disc 1/movie.iso", false, "movie"},
    {"smb://home/user/movies/movie/disc 1/file.iso", true, "movie"},
    {"stack://smb://path/to/movie_name/cd1/movie_file1.avi , "
     "smb://path/to/movie_name/cd2/movie_file2.avi",
     false, "movie"},
    {"stack://smb://path/to/movie/cd1/some_file1.avi , "
     "smb://path/to/movie/cd2/some_file2.avi",
     true, "movie"},
    // Embedded smb:// path tests
    {"bluray://udf%3a%2f%2fsmb%253a%252f%252fsomepath%252fmovie.iso%2f/BDMV/PLAYLIST/"
     "00800.mpls",
     false, "movie"},
    {"bluray://udf%3a%2f%2fsmb%253a%252f%252fsomepath%252fmovie%252ffilm.iso%2f/BDMV/PLAYLIST/"
     "00800.mpls",
     true, "movie"},
    {"bluray://udf%3a%2f%2fsmb%253a%252f%252fsomepath%252fmovie%252fDisc%25201%252ffilm.iso%2f/"
     "BDMV/"
     "PLAYLIST/"
     "00800.mpls",
     true, "movie"},
    {"bluray://smb%3a%2f%2fsomepath%2fmovie%2f/BDMV/PLAYLIST/00800.mpls", false, "movie"},
    {"bluray://smb%3a%2f%2fsomepath%2fmovie%2f/BDMV/PLAYLIST/00800.mpls", true, "movie"},
    {"bluray://smb%3a%2f%2fsomepath%2fmovie%2fDisc%201%2f/BDMV/PLAYLIST/00800.mpls", true, "movie"},
    {"zip://smb%3a%2f%2fsomepath%2fmovie.zip/film.avi", false, "movie"},
    {"zip://smb%3a%2f%2fsomepath%2fmovie%2ffilm.zip/film.avi", true, "movie"},
    {"zip://smb%3a%2f%2fsomepath%2fmovie%2fdisc%201%2ffilm.zip/BDMV/index.bdmv", true, "movie"},
    {"rar://smb%3a%2f%2fsomepath%2fmovie.rar/film.avi", false, "movie"},
    {"rar://smb%3a%2f%2fsomepath%2fmovie%2ffilm.rar/film.avi", true, "movie"},
    {"rar://smb%3a%2f%2fsomepath%2fmovie%2fdisc%201%2ffilm.rar/BDMV/index.bdmv", true, "movie"},
    {"archive://smb%3a%2f%2fsomepath%2fmovie.tar.gz/film.avi", false, "movie"},
    {"archive://smb%3a%2f%2fsomepath%2fmovie%2ffilm.tar.gz/film.avi", true, "movie"},
    {"archive://smb%3a%2f%2fsomepath%2fmovie%2fdisc%201%2ffilm.tar.gz/BDMV/index.bdmv", true,
     "movie"},
    // Embedded linux path tests
    {"bluray://udf%3a%2f%2f%252fsomepath%252fmovie.iso%2f/BDMV/PLAYLIST/00800.mpls", false,
     "movie"},
    {"bluray://udf%3a%2f%2f%252fsomepath%252fmovie%252ffile.iso%2f/BDMV/PLAYLIST/00800.mpls", true,
     "movie"},
    {"bluray://udf%3a%2f%2f%252fsomepath%252fmovie%252fDisc%25201%252ffilm.iso%2f/BDMV/"
     "PLAYLIST/"
     "00800.mpls",
     true, "movie"},
    {"bluray://%2fsomepath%2fmovie%2f/BDMV/PLAYLIST/00800.mpls", false, "movie"},
    {"bluray://%2fsomepath%2fmovie%2f/BDMV/PLAYLIST/00800.mpls", true, "movie"},
    {"bluray://%2fsomepath%2fmovie%2fDisc%201%2f/BDMV/PLAYLIST/00800.mpls", true, "movie"},
    {"zip://%2fsomepath%2fmovie.zip/film.avi", false, "movie"},
    {"zip://%2fsomepath%2fmovie%2ffilm.zip/film.avi", true, "movie"},
    {"zip://%2fsomepath%2fmovie%2fmovie%2fdisc%201%2ffilm.zip/BDMV/index.bdmv", true, "movie"},
    {"rar://%2fsomepath%2fmovie.rar/film.avi", false, "movie"},
    {"rar://%2fsomepath%2fmovie%2ffilm.rar/BDMV/index.bdmv", true, "movie"},
    {"rar://%2fsomepath%2fmovie%2fdisc%201%2ffilm.rar/BDMV/index.bdmv", true, "movie"},
    {"archive://%2fsomepath%2fmovie.tar.gz/film.avi", false, "movie"},
    {"archive://%2fsomepath%2fmovie%2ffilm.tar.gz/film.avi", true, "movie"},
    {"archive://%2fsomepath%2fmovie%2fdisc%201%2ffilm.tar.gz/BDMV/index.bdmv", true, "movie"},
    // Embedded DOS path tests
    {"bluray://udf%3a%2f%2fD%253a%255cmovies%255cmovie.iso%2f/BDMV/PLAYLIST/00800.mpls", false,
     "movie"},
    {"bluray://udf%3a%2f%2fD%253a%255cmovies%255cmovie%255cfilm.iso%2f/BDMV/PLAYLIST/00800.mpls",
     true, "movie"},
    {"bluray://udf%3a%2f%2fD%253a%255cmovies%255cmovie%255cDisc%25201%255cfilm.iso%2f/BDMV/"
     "PLAYLIST/"
     "00800.mpls",
     true, "movie"},
    {"bluray://D%3a%5csomepath%5cmovie%5c/BDMV/PLAYLIST/00800.mpls", false, "movie"},
    {"bluray://D%3a%5csomepath%5cmovie%5c/BDMV/PLAYLIST/00800.mpls", true, "movie"},
    {"bluray://D%3a%5csomepath%5cmovie%5cDisc%201%5c/BDMV/PLAYLIST/00800.mpls", true, "movie"},
    {"zip://C%3a%5cmovies%5cmovie.zip/film.avi", false, "movie"},
    {"zip://C%3a%5cmovies%5cmovie%5cfilm.zip/film.avi", true, "movie"},
    {"zip://C%3a%5cmovies%5cmovie%5cdisc%201%5cfilm.zip/BDMV/index.bdmv", true, "movie"},
    {"rar://C%3a%5cmovies%5cmovie.rar/film.avi", false, "movie"},
    {"rar://C%3a%5cmovies%5cmovie%5cfilm.rar/film.avi", true, "movie"},
    {"rar://C%3a%5cmovies%5cmovie%5cdisc%201%5cfilm.rar/BDMV/index.bdmv", true, "movie"},
    {"archive://C%3a%5cmovies%5cmovie.tar.gz/film.avi", false, "movie"},
    {"archive://C%3a%5cmovies%5cmovie%5cfilm.tar.gz/film.avi", true, "movie"},
    {"archive://C%3a%5cmovies%5cmovie%5cdisc%201%5cfilm.tar.gz/BDMV/index.bdmv", true, "movie"},
    // Embedded Windows server path tests
    {"bluray://udf%3a%2f%2f%255c%255cServer%255cMovies%255cmovie.iso%2f/BDMV/PLAYLIST/00800.mpls",
     false, "movie"},
    {"bluray://udf%3a%2f%2f%255c%255cServer%255cMovies%255cmovie%255cfilm.iso%2f/BDMV/PLAYLIST/"
     "00800.mpls",
     true, "movie"},
    {"bluray://udf%3a%2f%2f%255c%255cServer%255cMovies%255cmovie%255cDisc%25201%255cmovie.iso%2f/"
     "BDMV/"
     "PLAYLIST/"
     "00800.mpls",
     true, "movie"},
    {"bluray://%5c%5csomepath%5cmovie%5c/BDMV/PLAYLIST/00800.mpls", false, "movie"},
    {"bluray://%5c%5csomepath%5cmovie%5c/BDMV/PLAYLIST/00800.mpls", true, "movie"},
    {"bluray://%5c%5csomepath%5cmovie%5cDisc%201%5c/BDMV/PLAYLIST/00800.mpls", true, "movie"},
    {"zip://%5c%5cServer%5cMovies%5cmovie.zip/film.avi", false, "movie"},
    {"zip://%5c%5cServer%5cMovies%5cmovie%5cfilm.zip/film.avi", true, "movie"},
    {"zip://%5c%5cServer%5cMovies%5cmovie%5cdisc%201%5cfilm.zip/BDMV/index.bdmv", true, "movie"},
    {"rar://%5c%5cServer%5cMovies%5cmovie.rar/film.avi", false, "movie"},
    {"rar://%5c%5cServer%5cMovies%5cmovie%5cfilm.rar/film.avi", true, "movie"},
    {"rar://%5c%5cServer%5cMovies%5cmovie%5cdisc%201%5cfilm.rar/BDMV/index.bdmv", true, "movie"},
    {"archive://%5c%5cServer%5cMovies%5cmovie.tar.gz/film.avi", false, "movie"},
    {"archive://%5c%5cServer%5cMovies%5cmovie%5cfilm.tar.gz/film.avi", true, "movie"},
    {"archive://%5c%5cServer%5cMovies%5cmovie%5cdisc%201%5cfilm.tar.gz/BDMV/index.bdmv", true,
     "movie"}};

TEST_P(TestFileItemMovieName, GetMovieName)
{
  CFileItem item;
  item.SetPath(GetParam().file);
  std::string path = CURL(item.GetMovieName(GetParam().use_folder)).Get();
  std::string compare = CURL(GetParam().name).Get();
  EXPECT_EQ(compare, path);
}

INSTANTIATE_TEST_SUITE_P(NameMovies, TestFileItemMovieName, ValuesIn(BaseNames));

const TestFileData BasePaths[] = {
    // Linux path tests
    {"/home/user/movies/movie/", true, "/home/user/movies/movie/"},
    {"/home/user/movies/movie/video_ts/VIDEO_TS.IFO", false, "/home/user/movies/movie/"},
    {"/home/user/movies/movie/disc 1/video_ts/VIDEO_TS.IFO", false,
     "/home/user/movies/movie/disc 1/"},
    {"/home/user/movies/movie/BDMV/index.bdmv", false, "/home/user/movies/movie/"},
    {"/home/user/movies/movie/disc 1/BDMV/index.bdmv", false, "/home/user/movies/movie/disc 1/"},
    {"/home/user/movies/movie/movie.iso", false, "/home/user/movies/movie/"},
    {"/home/user/movies/movie/disc 1/movie.iso", false, "/home/user/movies/movie/disc 1/"},
    {"stack:///path/to/movie_name/cd1/some_file1.avi , /path/to/movie_name/cd2/some_file2.avi",
     false, "/path/to/movie_name/"},
    // DOS path tests
    {"D:\\Movies\\Movie\\", true, "D:\\Movies\\Movie\\"},
    {"D:\\Movies\\Movie\\video_ts\\VIDEO_TS.IFO", false, "D:\\Movies\\Movie\\"},
    {"D:\\Movies\\Movie\\disc 1\\video_ts\\VIDEO_TS.IFO", false, "D:\\Movies\\Movie\\disc 1\\"},
    {"D:\\Movies\\Movie\\BDMV\\index.bdmv", false, "D:\\Movies\\Movie\\"},
    {"D:\\Movies\\Movie\\disc 1\\BDMV\\index.bdmv", false, "D:\\Movies\\Movie\\disc 1\\"},
    {"D:\\Movies\\Movie\\movie.iso", false, "D:\\Movies\\Movie\\"},
    {"D:\\Movies\\Movie\\disc 1\\movie.iso", false, "D:\\Movies\\Movie\\disc 1\\"},
    {"stack://D:\\Movies\\Movie\\Movie - part 1\\some_file1.avi , D:\\Movies\\Movie\\Movie - part "
     "2\\some_file2.avi",
     false, "D:\\Movies\\Movie\\"},
    // Windows server path tests
    {"\\\\Server\\Movies\\Movie\\", true, "\\\\Server\\Movies\\Movie\\"},
    {"\\\\Server\\Movies\\Movie\\video_ts\\VIDEO_TS.IFO", false, "\\\\Server\\Movies\\Movie\\"},
    {"\\\\Server\\Movies\\Movie\\disc 1\\video_ts\\VIDEO_TS.IFO", false,
     "\\\\Server\\Movies\\Movie\\disc 1\\"},
    {"\\\\Server\\Movies\\Movie\\BDMV\\index.bdmv", false, "\\\\Server\\Movies\\Movie\\"},
    {"\\\\Server\\Movies\\Movie\\disc 1\\BDMV\\index.bdmv", false,
     "\\\\Server\\Movies\\Movie\\disc 1\\"},
    {"\\\\Server\\Movies\\Movie\\movie.iso", false, "\\\\Server\\Movies\\Movie\\"},
    {"\\\\Server\\Movies\\Movie\\disc 1\\movie.iso", false, "\\\\Server\\Movies\\Movie\\disc 1\\"},
    {"stack://\\\\Server\\Movies\\Movie\\Movie - part 1\\some_file1.avi , "
     "\\\\Server\\Movies\\Movie\\Movie - part "
     "2\\some_file2.avi",
     false, "\\\\Server\\Movies\\Movie\\"},
    // URL path tests with smb://
    {"smb://home/user/movies/movie/", true, "smb://home/user/movies/movie/"},
    {"smb://home/user/movies/movie/video_ts/VIDEO_TS.IFO", false, "smb://home/user/movies/movie/"},
    {"smb://home/user/movies/movie/disc 1/video_ts/VIDEO_TS.IFO", false,
     "smb://home/user/movies/movie/disc 1/"},
    {"smb://home/user/movies/movie/BDMV/index.bdmv", false, "smb://home/user/movies/movie/"},
    {"smb://home/user/movies/movie/disc 1/BDMV/index.bdmv", false,
     "smb://home/user/movies/movie/disc 1/"},
    {"smb://home/user/movies/movie/movie.iso", false, "smb://home/user/movies/movie/"},
    {"smb://home/user/movies/movie/disc 1/movie.iso", false,
     "smb://home/user/movies/movie/disc 1/"},
    {"stack://smb://path/to/movie_name/cd1/some_file1.avi , "
     "smb://path/to/movie_name/cd2/some_file2.avi",
     false, "smb://path/to/movie_name/"},
    // Embedded smb:// path tests
    {"bluray://udf%3a%2f%2fsmb%253a%252f%252fsomepath%252fmovie.iso%2f/BDMV/PLAYLIST/"
     "00800.mpls",
     false, "smb://somepath/"},
    {"bluray://udf%3a%2f%2fsmb%253a%252f%252fsomepath%252fdisc%25201%252fmovie.iso%2f/BDMV/"
     "PLAYLIST/"
     "00800.mpls",
     false, "smb://somepath/disc 1/"},
    {"bluray://smb%3a%2f%2fsomepath%2f/BDMV/PLAYLIST/00800.mpls", false, "smb://somepath/"},
    {"bluray://smb%3a%2f%2fsomepath%2fdisc%201%2f/BDMV/PLAYLIST/00800.mpls", false,
     "smb://somepath/disc 1/"},
    {"zip://smb%3a%2f%2fsomepath%2fmovie.zip/BDMV/index.bdmv", false, "smb://somepath/"},
    {"zip://smb%3a%2f%2fsomepath%2fmovie%2fdisc%201%2ffilm.zip/BDMV/index.bdmv", false,
     "smb://somepath/movie/disc 1/"},
    {"rar://smb%3a%2f%2fsomepath%2fmovie.rar/BDMV/index.bdmv", false, "smb://somepath/"},
    {"rar://smb%3a%2f%2fsomepath%2fmovie%2fdisc%201%2ffilm.rar/BDMV/index.bdmv", false,
     "smb://somepath/movie/disc 1/"},
    {"archive://smb%3a%2f%2fsomepath%2fmovie.tar.gz/BDMV/index.bdmv", false, "smb://somepath/"},
    {"archive://smb%3a%2f%2fsomepath%2fmovie%2fdisc%201%2ffilm.tar.gz/BDMV/index.bdmv", false,
     "smb://somepath/movie/disc 1/"},
    // Embedded linux path tests
    {"bluray://udf%3a%2f%2f%252fsomepath%252fmovie.iso%2f/BDMV/PLAYLIST/00800.mpls", false,
     "/somepath/"},
    {"bluray://udf%3a%2f%2f%252fsomepath%252fdisc%25201%252fmovie.iso%2f/BDMV/"
     "PLAYLIST/"
     "00800.mpls",
     false, "/somepath/disc 1/"},
    {"bluray://%2fsomepath%2f/BDMV/PLAYLIST/00800.mpls", false, "/somepath/"},
    {"bluray://%2fsomepath%2fdisc%201%2f/BDMV/PLAYLIST/00800.mpls", false, "/somepath/disc 1/"},
    {"zip://%2fsomepath%2fmovie.zip/BDMV/index.bdmv", false, "/somepath/"},
    {"zip://%2fsomepath%2fdisc%201%2fmovie.zip/BDMV/index.bdmv", false, "/somepath/disc 1/"},
    {"rar://%2fsomepath%2fmovie.rar/BDMV/index.bdmv", false, "/somepath/"},
    {"rar://%2fsomepath%2fdisc%201%2fmovie_disc.rar/BDMV/index.bdmv", false, "/somepath/disc 1/"},
    {"archive://%2fsomepath%2fmovie.tar.gz/BDMV/index.bdmv", false, "/somepath/"},
    {"archive://%2fsomepath%2fdisc%201%2fmovie.tar.gz/BDMV/index.bdmv", true, "/somepath/disc 1/"},
    // Embedded DOS path tests
    {"bluray://udf%3a%2f%2fD%253a%255cmovies%255cmovie.iso%2f/BDMV/PLAYLIST/00800.mpls", false,
     "D:\\movies\\"},
    {"bluray://udf%3a%2f%2fD%253a%255cmovies%255cdisc%25201%255cmovie.iso%2f/BDMV/"
     "PLAYLIST/"
     "00800.mpls",
     false, "D:\\movies\\disc 1\\"},
    {"bluray://D%3a%5cmovie%5c/BDMV/PLAYLIST/00800.mpls", false, "D:\\movie\\"},
    {"bluray://D%3a%5cmovie%5cdisc%201%5c/BDMV/PLAYLIST/00800.mpls", false, "D:\\movie\\disc 1\\"},
    {"zip://D%3a%5cmovies%5cmovie.zip/BDMV/index.bdmv", false, "D:\\movies\\"},
    {"zip://D%3a%5cmovies%5cmovie%5cdisc%201%5cmovie.zip/BDMV/index.bdmv", false,
     "D:\\movies\\movie\\disc 1\\"},
    {"rar://D%3a%5cmovies%5cmovie.rar/BDMV/index.bdmv", false, "D:\\movies\\"},
    {"rar://D%3a%5cmovies%5cmovie%5cdisc%201%5cmovie.rar/BDMV/index.bdmv", false,
     "D:\\movies\\movie\\disc 1\\"},
    {"archive://D%3a%5cmovies%5cmovie.tar.gz/BDMV/index.bdmv", false, "D:\\movies\\"},
    {"archive://D%3a%5cmovies%5cmovie%5cdisc%201%5cmovie.tar.gz/BDMV/index.bdmv", false,
     "D:\\movies\\movie\\disc 1\\"},
    // Embedded Windows server path tests
    {"bluray://udf%3a%2f%2f%255c%255cServer%255cMovies%255cmovie.iso%2f/BDMV/PLAYLIST/00800.mpls",
     false, "\\\\Server\\Movies\\"},
    {"bluray://udf%3a%2f%2f%255c%255cServer%255cMovies%255cMovie%255cdisc%25201%255cmovie.iso%2f/"
     "BDMV/"
     "PLAYLIST/"
     "00800.mpls",
     false, "\\\\Server\\Movies\\Movie\\disc 1\\"},
    {"bluray://%5c%5cServer%5cMovies%5cMovie%5c/BDMV/PLAYLIST/00800.mpls", false,
     "\\\\Server\\Movies\\Movie\\"},
    {"bluray://%5c%5cServer%5cMovies%5cMovie%5cdisc%201%5c/BDMV/PLAYLIST/00800.mpls", false,
     "\\\\Server\\Movies\\Movie\\disc 1\\"},
    {"zip://%5c%5cServer%5cMovies%5cmovie.zip/BDMV/index.bdmv", false, "\\\\Server\\Movies\\"},
    {"zip://%5c%5cServer%5cMovies%5cMovie%5cdisc%201%5cmovie.zip/BDMV/index.bdmv", false,
     "\\\\Server\\Movies\\Movie\\disc 1\\"},
    {"rar://%5c%5cServer%5cMovies%5cmovie.rar/BDMV/index.bdmv", false, "\\\\Server\\Movies\\"},
    {"rar://%5c%5cServer%5cMovies%5cMovie%5cdisc%201%5cmovie.rar/BDMV/index.bdmv", false,
     "\\\\Server\\Movies\\Movie\\disc 1\\"},
    {"archive://%5c%5cServer%5cMovies%5cmovie.tar.gz/BDMV/index.bdmv", false,
     "\\\\Server\\Movies\\"},
    {"archive://%5c%5cServer%5cMovies%5cMovie%5cdisc%201%5cmovie.tar.gz/BDMV/index.bdmv", false,
     "\\\\Server\\Movies\\Movie\\disc 1\\"}};

TEST_P(TestFileItemLocalMetadataPath, GetLocalMetadataPath)
{
  CFileItem item;
  item.SetPath(GetParam().file);
  item.SetFolder(GetParam().use_folder);
  EXPECT_EQ(GetParam().base, item.GetLocalMetadataPath());
}

INSTANTIATE_TEST_SUITE_P(NameMovies, TestFileItemLocalMetadataPath, ValuesIn(BasePaths));

TEST(TestFileItem, TestSimplePathSet)
{
  CFileItem item;
  item.SetPath("/local/path/regular/file.txt");
  item.SetDynPath("/local/path/dynamic/file.txt");

  EXPECT_EQ("/local/path/regular/file.txt", item.GetURL().Get());
  EXPECT_EQ("/local/path/dynamic/file.txt", item.GetDynURL().Get());
}

TEST(TestFileItem, TestLabel)
{
  CFileItem item("My Item Label");
  item.SetPath("/local/path/file.txt");

  EXPECT_EQ("My Item Label", item.GetLabel());
  EXPECT_EQ("/local/path/file.txt", item.GetURL().Get());
  EXPECT_EQ("/local/path/file.txt", item.GetDynURL().Get());
}

TEST(TestFileItem, TestCURLConstructor)
{
  CFileItem folderItem(CURL("/local/path/file.txt"), true);

  EXPECT_EQ("", folderItem.GetLabel());
  EXPECT_EQ("/local/path/file.txt/", folderItem.GetURL().Get());
  EXPECT_EQ("/local/path/file.txt/", folderItem.GetDynURL().Get());

  CFileItem fileItem(CURL("/local/path/file.txt"), false);

  EXPECT_EQ("", fileItem.GetLabel());
  EXPECT_EQ("/local/path/file.txt", fileItem.GetURL().Get());
  EXPECT_EQ("/local/path/file.txt", fileItem.GetDynURL().Get());
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
}

TEST(TestFileItem, MimeType)
{
  CFileItem item("Internet Movies List");
  item.SetPath("http://testdomain.com/api/movies");

  item.FillInMimeType(true);
  EXPECT_EQ("Internet Movies List", item.GetLabel());
  EXPECT_EQ("http://testdomain.com/api/movies", item.GetURL().Get());
  EXPECT_EQ("http://testdomain.com/api/movies", item.GetDynURL().Get());

  item.FillInMimeType(false);
  EXPECT_EQ("Internet Movies List", item.GetLabel());
  EXPECT_EQ("http://testdomain.com/api/movies", item.GetURL().Get());
  EXPECT_EQ("http://testdomain.com/api/movies", item.GetDynURL().Get());
}
