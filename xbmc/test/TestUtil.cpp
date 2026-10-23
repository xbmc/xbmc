/*
 *  Copyright (C) 2005-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DatabaseManager.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "Util.h"
#include "cores/VideoPlayer/Interface/StreamInfo.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "filesystem/SpecialProtocol.h"
#include "resources/LocalizeStrings.h"
#include "resources/ResourcesComponent.h"
#include "settings/AdvancedSettings.h"
#include "settings/MediaSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "test/TestUtils.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "video/FilenameAttributes.h"

#include <algorithm>
#include <string>
#include <vector>

#include <gtest/gtest-param-test.h>
#include <gtest/gtest.h>

using ::testing::Test;
using ::testing::ValuesIn;
using ::testing::WithParamInterface;

using namespace KODI::VIDEO;

TEST(TestUtil, GetQualifiedFilename)
{
  std::string file = "../foo";
  CUtil::GetQualifiedFilename("smb://", file);
  EXPECT_EQ(file, "foo");
  file = "C:\\foo\\bar";
  CUtil::GetQualifiedFilename("smb://", file);
  EXPECT_EQ(file, "C:\\foo\\bar");
  file = "../foo/./bar";
  CUtil::GetQualifiedFilename("smb://my/path", file);
  EXPECT_EQ(file, "smb://my/foo/bar");
  file = "smb://foo/bar/";
  CUtil::GetQualifiedFilename("upnp://", file);
  EXPECT_EQ(file, "smb://foo/bar/");
}

TEST(TestUtil, IsInPlaylistsFolderFollowsThePlaylistsSetting)
{
  using KODI::MEDIA::MediaSection;
  const auto settings = CServiceBroker::GetSettingsComponent()->GetSettings();
  const std::string original{settings->GetString(CSettings::SETTING_SYSTEM_PLAYLISTSPATH)};
  settings->SetString(CSettings::SETTING_SYSTEM_PLAYLISTSPATH, "special://temp/lists/");

  EXPECT_TRUE(CUtil::IsInPlaylistsFolder("special://temp/lists/video/a.m3u", MediaSection::VIDEO));
  EXPECT_FALSE(CUtil::IsInPlaylistsFolder("special://temp/lists/video/a.m3u", MediaSection::MUSIC));
  EXPECT_TRUE(CUtil::IsInPlaylistsFolder("special://temp/lists/mixed/a.m3u", MediaSection::MUSIC));
  EXPECT_TRUE(CUtil::IsInPlaylistsFolder("special://musicplaylists/a.xsp", MediaSection::MUSIC));
  EXPECT_FALSE(
      CUtil::IsInPlaylistsFolder("special://profile/playlists/video/a.m3u", MediaSection::VIDEO));
  EXPECT_FALSE(CUtil::IsInPlaylistsFolder("special://temp/lists/video/a.m3u", MediaSection::FILES));

  settings->SetString(CSettings::SETTING_SYSTEM_PLAYLISTSPATH, original);
}

TEST(TestUtil, MakeLegalPath)
{
  std::string path;
#ifdef TARGET_WINDOWS
  path = "C:\\foo\\bar";
  EXPECT_EQ(CUtil::MakeLegalPath(path), "C:\\foo\\bar");
  path = "C:\\foo:\\bar\\";
  EXPECT_EQ(CUtil::MakeLegalPath(path), "C:\\foo_\\bar\\");
#else
  path = "/foo/bar/";
  EXPECT_EQ(CUtil::MakeLegalPath(path),"/foo/bar/");
  path = "/foo?/bar";
  EXPECT_EQ(CUtil::MakeLegalPath(path),"/foo_/bar");
#endif
  path = "smb://foo/bar";
  EXPECT_EQ(CUtil::MakeLegalPath(path), "smb://foo/bar");
  path = "smb://foo/bar?/";
  EXPECT_EQ(CUtil::MakeLegalPath(path), "smb://foo/bar_/");
}

TEST(TestUtil, MakeShortenPath)
{
  std::string result;
  EXPECT_EQ(true, CUtil::MakeShortenPath("smb://test/string/is/long/and/very/much/so", result, 10));
  EXPECT_EQ("smb:/../so", result);

  EXPECT_EQ(true, CUtil::MakeShortenPath("smb://test/string/is/long/and/very/much/so", result, 30));
  EXPECT_EQ("smb://../../../../../../../so", result);

  EXPECT_EQ(true, CUtil::MakeShortenPath("smb://test//string/is/long/and/very//much/so", result, 30));
  EXPECT_EQ("smb:/../../../../../so", result);

  EXPECT_EQ(true, CUtil::MakeShortenPath("//test//string/is/long/and/very//much/so", result, 30));
  EXPECT_EQ("/../../../../../so", result);
}

TEST(TestUtil, ValidatePath)
{
  std::string path;
#ifdef TARGET_WINDOWS
  for (const std::string drive : {"C", "c", "D", "x"})
  {
    SCOPED_TRACE(drive);
    EXPECT_EQ(drive + R"(:\\Movies\movie.mkv)", CUtil::ValidatePath(drive + "://Movies/movie.mkv"));
    EXPECT_EQ(drive + R"(:\Movies\movie.mkv)",
              CUtil::ValidatePath(drive + "://Movies/movie.mkv", true));
  }
  EXPECT_EQ(R"(C:\file)", CUtil::ValidatePath("C:/file"));
  EXPECT_EQ("C:relative", CUtil::ValidatePath("C:relative"));
  path = "C:\\\\foo\\\\bar\\";
  EXPECT_EQ(CUtil::ValidatePath(path, true), "C:\\foo\\bar\\");
  path = "\\\\foo\\\\bar\\";
  EXPECT_EQ(CUtil::ValidatePath(path, true), "\\\\foo\\bar\\");
#else
  path = "\\foo\\bar\\";
  EXPECT_EQ(CUtil::ValidatePath(path), "/foo/bar/");
  path = "/foo//bar/";
  EXPECT_EQ(CUtil::ValidatePath(path, true), "/foo/bar/");
#endif
  path = "C:\\foo\\bar\\";
  EXPECT_EQ(CUtil::ValidatePath(path), "C:\\foo\\bar\\");
  path = "smb://foo/bar/";
  EXPECT_EQ(CUtil::ValidatePath(path), "smb://foo/bar/");
  path = "smb://foo//bar/";
  EXPECT_EQ(CUtil::ValidatePath(path, true), "smb://foo/bar/");
  path = "smb:\\\\foo\\\\bar\\";
  EXPECT_EQ(CUtil::ValidatePath(path, true), "smb://foo/bar/");
}

struct TestUtilCleanStringData
{
  std::string input;
  bool first;
  std::string expTitle;
  std::string expTitleYear;
  std::string expYear;
  std::string expIdentifierType{};
  std::string expIdentifier{};
};

std::ostream& operator<<(std::ostream& os, const TestUtilCleanStringData& rhs)
{
  return os << "(input: " << rhs.input << "; first: " << (rhs.first ? "true" : "false")
            << "; expTitle: " << rhs.expTitle << "; expTitleYear: " << rhs.expTitleYear
            << "; expYear: " << rhs.expYear << ")";
}

class TestUtilCleanString : public Test, public WithParamInterface<TestUtilCleanStringData>
{
public:
  static void SetUpTestSuite()
  {
    // Inject list of known metadata sources for reliable results
    const std::shared_ptr<CAdvancedSettings> advancedSettings =
        CServiceBroker::GetSettingsComponent()->GetAdvancedSettings();
    ASSERT_TRUE(advancedSettings != nullptr);
    advancedSettings->m_videoScannerMetadataSources = {"tmdb", "imdb"};
  }
};

TEST_P(TestUtilCleanString, GetFilenameIdentifier)
{
  std::string identifierType;
  std::string identifier;
  CFilenameAttributes(GetParam().input, nullptr).GetIdentifier(identifierType, identifier);
  EXPECT_EQ(identifierType, GetParam().expIdentifierType);
  EXPECT_EQ(identifier, GetParam().expIdentifier);
}

TEST_P(TestUtilCleanString, CleanString)
{
  std::string title, titleYear, year;
  CUtil::CleanString(GetParam().input, title, titleYear, year, true, GetParam().first);
  EXPECT_EQ(title, GetParam().expTitle);
  EXPECT_EQ(titleYear, GetParam().expTitleYear);
  EXPECT_EQ(year, GetParam().expYear);
}
const TestUtilCleanStringData values[] = {
    {"Some.BDRemux.mkv", true, "Some", "Some", ""},
    {"SomeMovie.2018.UHD.BluRay.2160p.HEVC.TrueHD.Atmos.7.1-BeyondHD", true, "SomeMovie",
     "SomeMovie (2018)", "2018"},
    {"SOME_UHD_HDR10+_DV_2022_remux_dub atmos_soundmovie", true, "SOME", "SOME (2022)", "2022"},
    // no result because of the . and spaces in movie name
    {"Some Movie.Some Story.2013.BDRemux.1080p", true, "Some Movie.Some Story",
     "Some Movie.Some Story (2013)", "2013"},
    {"Movie.Some.Story.2017.2160p.BDRemux.IMAX.HDR.DV.IVA(ENG.RUS).ExKinoRay", true,
     "Movie Some Story", "Movie Some Story (2017)", "2017"},
    {"Some.Movie.1954.BDRip.1080p.mkv", true, "Some Movie", "Some Movie (1954)", "1954"},
    {"Some «Movie».2021.WEB-DL.2160p.HDR.mkv", true, "Some «Movie»", "Some «Movie» (2021)", "2021"},
    {"Some Movie (2013).mp4", true, "Some Movie", "Some Movie (2013)", "2013"},
    {"Some Movie (2013) [imdbid-tt123].mp4", true, "Some Movie", "Some Movie (2013)", "2013",
     "imdb", "tt123"},
    {"Some Movie (2013) {tmdb-123}.mp4", true, "Some Movie", "Some Movie (2013)", "2013", "tmdb",
     "123"},
    {"Some Movie (2013) {tmdb=123}.mp4", true, "Some Movie", "Some Movie (2013)", "2013", "tmdb",
     "123"},
    // no result because of the text (Director Cut), it can also a be a movie translation
    {"Some (Director Cut).BDRemux.mkv", true, "Some (Director Cut)", "Some (Director Cut)", ""}};

INSTANTIATE_TEST_SUITE_P(URL, TestUtilCleanString, ValuesIn(values));

struct TestUtilSplitParamsData
{
  std::string input;
  std::vector<std::string> expectedResult;
};

std::ostream& operator<<(std::ostream& os, const TestUtilSplitParamsData& rhs)
{
  os << "(input: " << rhs.input << "; expCount: " << rhs.expectedResult.size();
  for (unsigned int i = 0; i < rhs.expectedResult.size(); ++i)
    os << "; expParam " << i + 1 << ": " << rhs.expectedResult[i];

  return os;
}

class TestUtilSplitParams : public Test, public WithParamInterface<TestUtilSplitParamsData>
{
};

TEST_P(TestUtilSplitParams, SplitParams)
{
  std::vector<std::string> actual;
  CUtil::SplitParams(GetParam().input, actual);
  EXPECT_EQ(actual.size(), GetParam().expectedResult.size());
  EXPECT_EQ(actual, GetParam().expectedResult);
}

const TestUtilSplitParamsData valuesSplitParams[] = {
    {"", {}},
    {"one", {"one"}},
    {" one", {"one"}},
    {"one ", {"one"}},
    {" one ", {"one"}},
    // quotes " removed only when at beginning and end of param (after trimming)
    {R"(" one")", {R"( one)"}},
    {R"("one ")", {R"(one )"}},
    {R"("on"e )", {R"("on"e)"}},
    {R"(o"n"e )", {R"(o"n"e)"}},
    {R"(o" n"e )", {R"(o" n"e)"}},
    {R"( "one" )", {R"(one)"}},
    {R"( " one")", {R"( one)"}},
    {R"( " one " )", {R"( one )"}},
    {R"(o" n"e )", {R"(o" n"e)"}},
    {R"(o"ne" )", {R"(o"ne")"}},
    {R"(o\"ne")", {R"(o"ne")"}},
    {R"(\" one")", {R"( one)"}},
    {R"(\" one\")", {R"( one)"}},
    {R"("one " one)", {R"("one " one)"}},
    // function 1 parameter
    {"fun(p1)", {"fun(p1)"}},
    {" fun(p1)", {"fun(p1)"}},
    {"fun(p1) ", {"fun(p1)"}},
    {"fun(p1 p1) ", {"fun(p1 p1)"}},
    {"fun( p1 ) ", {"fun( p1 )"}}, // no trim: likely omission of the code?
    // 2 parameters
    {"one,", {"one", ""}},
    {"one,two", {"one", "two"}},
    {"one ,two", {"one", "two"}},
    {"one,two ", {"one", "two"}},
    {"one, two", {"one", "two"}},
    {"one,two two", {"one", "two two"}},
    {"one, two two", {"one", "two two"}},
    {"one,two two ", {"one", "two two"}},
    {R"(\" one\", two)", {R"( one)", "two"}},
    //{R"(\" one", two)", {R"( one)", "two"}}, // mixing \" & " not supported for multi param
    {R"(one, \" two\")", {"one", R"( two)"}},
    //{R"(one, \" two")", {"one", R"( two)"}},  // mixing \" & " not supported for multi param

    // function 2 parameters
    {"fun(p1,p2)", {"fun(p1,p2)"}},
    {"fun(p1 ,p2)", {"fun(p1 ,p2)"}}, // no trim: omission in the code?
    {"fun(p1, p2)", {"fun(p1, p2)"}}, // no trim: omission in the code?
    {"fun(p1 p1 ,p2)", {"fun(p1 p1 ,p2)"}}, // no trim: omission in the code?
    {"fun(p1,p2), two", {"fun(p1,p2)", "two"}},
    {"\"fun(p1,p2)\",two", {"fun(p1,p2)", "two"}},
    {"\"fun(p1,p2)\",\" two\"", {"fun(p1,p2)", " two"}},
    {"fun(p1,p2),\" two\"", {"fun(p1,p2)", " two"}},
    {"\\\"fun(p1,p2)\\\",two", {"fun(p1,p2)", "two"}},
    {"fun(p1,p2),\\\"two\\\"", {"fun(p1,p2)", "two"}},
    {"fun(fun2(p1,p2),p3),two", {"fun(fun2(p1,p2),p3)", "two"}},
    {"fun\"ction(p1,p2\",p3,p4)\"", {"fun\"ction(p1,p2\"", "p3", "p4)\""}},
    // 3 parameters
    {"one,two,three", {"one", "two", "three"}},
    {"one,two two,three", {"one", "two two", "three"}},
    {"one, two, three", {"one", "two", "three"}},
    {"one, two two, three", {"one", "two two", "three"}},
    // \ escaping
    {R"(D:\foo\bar\baz.m2ts)", {R"(D:\foo\bar\baz.m2ts)"}},
    {R"(D:\foo\bar\)", {R"(D:\foo\bar\)"}},
    {R"(D:\\foo\bar\)", {R"(D:\foo\bar\)"}},
    {R"(D:\foo\\bar\)", {R"(D:\foo\bar\)"}},
    {R"(D:\foo\bar\\)", {R"(D:\foo\bar\)"}},
    {R"(D:\\\foo\bar\)", {R"(D:\\foo\bar\)"}},
    {R"(D:\foo\\\bar\)", {R"(D:\foo\\bar\)"}},
    {R"(D:\foo\bar\\\)", {R"(D:\foo\bar\\)"}},
    {R"(D:\\\\foo\bar\)", {R"(D:\\foo\bar\)"}},
    {R"(D:\foo\\\\bar\)", {R"(D:\foo\\bar\)"}},
    {R"(D:\foo\bar\\\\)", {R"(D:\foo\bar\\)"}},
    {R"("D:\foo\\bar\\\baz.m2ts\\\\")", {R"(D:\foo\bar\\baz.m2ts\\)"}},
    {R"(" D:\foo\\bar\\\baz.m2ts\\\\")", {R"( D:\foo\bar\\baz.m2ts\\)"}},
    {R"(\"D:\foo\\bar\\\baz.m2ts\\\\\")", {R"(D:\foo\bar\\baz.m2ts\\)"}},
    {R"(\" D:\foo\\bar\\\baz.m2ts\\\\\")", {R"( D:\foo\bar\\baz.m2ts\\)"}},
    {R"(123,D:\foo\\bar\\\baz.m2ts\\\\,abc)", {"123", R"(D:\foo\bar\\baz.m2ts\\)", "abc"}},
    {R"(123,"D:\foo\\bar\\\baz.m2ts\\\\",abc)", {"123", R"(D:\foo\bar\\baz.m2ts\\)", "abc"}},
    {R"(123," D:\foo\\bar\\\baz.m2ts\\\\",abc)", {"123", R"( D:\foo\bar\\baz.m2ts\\)", "abc"}},
    {R"(123,\"D:\foo\\bar\\\baz.m2ts\\\\\",abc)", {"123", R"(D:\foo\bar\\baz.m2ts\\)", "abc"}},
    {R"(123,\" D:\foo\\bar\\\baz.m2ts\\\\\",abc)", {"123", R"( D:\foo\bar\\baz.m2ts\\)", "abc"}},
    // name="value" parameter form
    {R"(name="value")", {"name=value"}},
    {R"(name="value1 value2")", {"name=value1 value2"}},
    {R"(name="value1,value2")", {"name=value1,value2"}},
    {R"(name="value1, value2")", {"name=value1, value2"}},
    {R"(name="value1 "value2 " value3")", {R"(name=value1 "value2 " value3)"}},
    {R"(name="value1 \"value2 \" value 3")", {R"(name=value1 "value2 " value 3)"}},
    {R"(name=\"value\")", {R"(name=value)"}},
    {R"("name=\"value\"")", {R"(name="value")"}},
    {R"(name=value1,value2)", {"name=value1", "value2"}},
    {R"(foo=bar=value1,value2)", {"foo=bar=value1", "value2"}},
    {R"(foo=bar="value1,value2")", {"foo=bar=value1,value2"}},
    {R"("name=value1 value2")", {"name=value1 value2"}},
    {R"(abc, name="value", cde)", {"abc", "name=value", "cde"}},
};

INSTANTIATE_TEST_SUITE_P(SP, TestUtilSplitParams, ValuesIn(valuesSplitParams));

struct TestDiscData
{
  const char* file;
  const char* number;
};

class TestDiscNumbers : public Test, public WithParamInterface<TestDiscData>
{
};

const TestDiscData BaseFiles[] = {
    // Linux path tests
    {"/home/user/movies/movie/video_ts/VIDEO_TS.IFO", ""},
    {"/home/user/movies/movie/disc 1/video_ts/VIDEO_TS.IFO", "1"},
    {"/home/user/movies/movie/BDMV/index.bdmv", ""},
    {"/home/user/movies/movie/disc 1/BDMV/index.bdmv", "1"},
    {"/home/user/movies/movie.iso", ""},
    {"/home/user/movies/movie/file.iso", ""},
    {"/home/user/movies/disc 1/movie.iso", "1"},
    {"/home/user/movies/movie/disc 1/file.iso", "1"},
    {"/home/user/movies/movie.avi", ""},
    {"/home/user/movies/movie/file.avi", ""},
    {"/home/user/movies/movie/disc 1/file.avi", "1"},
    {"/home/user/movies/movie/disk 1/file.avi", "1"},
    {"/home/user/movies/movie/cd 1/file.avi", "1"},
    {"/home/user/movies/movie/dvd 1/file.avi", "1"},
    // DOS path tests
    {"D:\\Movies\\Movie\\video_ts\\VIDEO_TS.IFO", ""},
    {"D:\\Movies\\Movie\\disc 1\\video_ts\\VIDEO_TS.IFO", "1"},
    {"D:\\Movies\\Movie\\BDMV\\index.bdmv", ""},
    {"D:\\Movies\\Movie\\disc 1\\BDMV\\index.bdmv", "1"},
    {"D:\\Movies\\Movie\\movie.iso", ""},
    {"D:\\Movies\\Movie\\file.iso", ""},
    {"D:\\Movies\\Movie\\disc 1\\movie.iso", "1"},
    {"D:\\Movies\\Movie\\disc 1\\file.iso", "1"},
    {"D:\\Movies\\Movie\\movie.avi", ""},
    {"D:\\Movies\\Movie\\file.avi", ""},
    {"D:\\Movies\\Movie\\disc 1\\file.avi", "1"},
    {"D:\\Movies\\Movie\\disk 1\\file.avi", "1"},
    {"D:\\Movies\\Movie\\cd 1\\file.avi", "1"},
    {"D:\\Movies\\Movie\\dvd 1\\file.avi", "1"},
    // Server path tests
    {"\\\\Server\\Movies\\Movie\\video_ts\\VIDEO_TS.IFO", ""},
    {"\\\\Server\\Movies\\Movie\\disc 1\\video_ts\\VIDEO_TS.IFO", "1"},
    {"\\\\Server\\Movies\\Movie\\BDMV\\index.bdmv", ""},
    {"\\\\Server\\Movies\\Movie\\disc 1\\BDMV\\index.bdmv", "1"},
    {"\\\\Server\\Movies\\Movie\\movie.iso", ""},
    {"\\\\Server\\Movies\\Movie\\file.iso", ""},
    {"\\\\Server\\Movies\\Movie\\disc 1\\movie.iso", "1"},
    {"\\\\Server\\Movies\\Movie\\disc 1\\file.iso", "1"},
    {"\\\\Server\\Movies\\Movie\\movie.avi", ""},
    {"\\\\Server\\Movies\\Movie\\file.avi", ""},
    {"\\\\Server\\Movies\\Movie\\disc 1\\file.avi", "1"},
    {"\\\\Server\\Movies\\Movie\\disk 1\\file.avi", "1"},
    {"\\\\Server\\Movies\\Movie\\cd 1\\file.avi", "1"},
    {"\\\\Server\\Movies\\Movie\\dvd 1\\file.avi", "1"},
    // URL path tests with smb://
    {"smb://home/user/movies/movie/video_ts/VIDEO_TS.IFO", ""},
    {"smb://home/user/movies/movie/disc 1/video_ts/VIDEO_TS.IFO", "1"},
    {"smb://home/user/movies/movie/BDMV/index.bdmv", ""},
    {"smb://home/user/movies/movie/disc 1/BDMV/index.bdmv", "1"},
    {"smb://home/user/movies/movie.iso", ""},
    {"smb://home/user/movies/movie/file.iso", ""},
    {"smb://home/user/movies/disc 1/movie.iso", "1"},
    {"smb://home/user/movies/movie/disc 1/file.iso", "1"},
    {"smb://home/user/movies/movie.avi", ""},
    {"smb://home/user/movies/movie/file.avi", ""},
    {"smb://home/user/movies/movie/disc 1/file.avi", "1"},
    {"smb://home/user/movies/movie/disk 1/file.avi", "1"},
    {"smb://home/user/movies/movie/cd 1/file.avi", "1"},
    {"smb://home/user/movies/movie/dvd 1/file.avi", "1"},
    // Embedded linux path tests
    {"bluray://udf%3a%2f%2f%252fsomepath%252fpath%252fdisc%25201%252fmovie.iso%2f/"
     "BDMV/PLAYLIST/00800.mpls",
     "1"},
    {"zip://%2fsomepath%2fpath%2fdisc%202%2fmovie.zip/BDMV/index.bdmv", "2"},
    {"rar://%2fsomepath%2fpath%2fdisc%203%2fmovie.rar/BDMV/index.bdmv", "3"},
    {"archive://%2fsomepath%2fpath%2fdisc%204%2fmovie.tar.gz/VIDEO_TS/VIDEO_TS.IFO", "4"},
    // Embedded DOS path tests
    {"bluray://udf%3a%2f%2fD%253a%255csomepath%255cdisc%25201%255cmovie.iso%2f/"
     "BDMV/PLAYLIST/00800.mpls",
     "1"},
    {"zip://D%3a%5csomepath%5cdisc%202%5cmovie.zip/BDMV/index.bdmv", "2"},
    {"rar://D%3a%5csomepath%5cdisc%203%5cmovie.rar/BDMV/index.bdmv", "3"},
    {"archive://D%3a%5csomepath%5cdisc%204%5cmovie.tar.gz/VIDEO_TS/VIDEO_TS.IFO", "4"},
    // Embedded windows server path tests
    {"bluray://udf%3a%2f%2f%255c%255cServer%255cMovie%255cdisc%25201%255cmovie.iso%2f/"
     "BDMV/PLAYLIST/00800.mpls",
     "1"},
    {"zip://%5c%5cServer%5cMovie%5cdisc%202%5cmovie.zip/BDMV/index.bdmv", "2"},
    {"rar://%5c%5cServer%5cMovie%5cdisc%203%5cmovie.rar/BDMV/index.bdmv", "3"},
    {"archive://%5c%5cServer%5cMovie%5cdisc%204%5cmovie.tar.gz/VIDEO_TS/VIDEO_TS.IFO", "4"},
    // Embedded URL tests with smb://
    {"bluray://udf%3a%2f%2fsmb%253a%252f%252fsomepath%252fpath%252fdisc%25201%252fmovie.iso%2f/"
     "BDMV/PLAYLIST/00800.mpls",
     "1"},
    {"zip://smb%3a%2f%2fsomepath%2fpath%2fdisc%202%2fmovie.zip/BDMV/index.bdmv", "2"},
    {"rar://smb%3a%2f%2fsomepath%2fpath%2fdisc%203%2fmovie.rar/BDMV/index.bdmv", "3"},
    {"archive://smb%3a%2f%2fsomepath%2fpath%2fdisc%204%2fmovie.tar.gz/VIDEO_TS/VIDEO_TS.IFO", "4"}};

TEST_P(TestDiscNumbers, GetDiscNumbers)
{
  const std::string discNum = CUtil::GetPartNumberFromPath(GetParam().file);
  EXPECT_EQ(GetParam().number, discNum);
}

INSTANTIATE_TEST_SUITE_P(DiscNumbers, TestDiscNumbers, ValuesIn(BaseFiles));

struct TestRemoveData
{
  const char* path;
  const char* result;
};

class TestRemoveDiscNumbers : public Test, public WithParamInterface<TestRemoveData>
{
};

const TestRemoveData BasePaths[] = {
    // Linux path tests
    {"/home/user/movies/movie/video_ts/", "/home/user/movies/movie/"},
    {"/home/user/movies/movie/disc 1/video_ts/", "/home/user/movies/movie/"},
    {"/home/user/movies/movie/BDMV/", "/home/user/movies/movie/"},
    {"/home/user/movies/movie/disc 1/BDMV/index.bdmv", "/home/user/movies/movie/"},
    {"/home/user/movies/movie/file.iso", "/home/user/movies/movie/"},
    {"/home/user/movies/movie/disc 1/file.iso", "/home/user/movies/movie/"},
    {"/home/user/movies/movie/file.avi", "/home/user/movies/movie/"},
    {"/home/user/movies/movie/disc 1/file.avi", "/home/user/movies/movie/"},
    {"/home/user/movies/movie/disk 1/file.avi", "/home/user/movies/movie/"},
    {"/home/user/movies/movie/cd 1/file.avi", "/home/user/movies/movie/"},
    {"/home/user/movies/movie/dvd 1/file.avi", "/home/user/movies/movie/"},
    // DOS path tests
    {"D:\\movies\\movie\\video_ts\\", "D:\\movies\\movie\\"},
    {"D:\\movies\\movie\\disc 1\\video_ts\\", "D:\\movies\\movie\\"},
    {"D:\\movies\\movie\\BDMV\\", "D:\\movies\\movie\\"},
    {"D:\\movies\\movie\\disc 1\\BDMV\\index.bdmv", "D:\\movies\\movie\\"},
    {"D:\\movies\\movie\\file.iso", "D:\\movies\\movie\\"},
    {"D:\\movies\\movie\\disc 1\\file.iso", "D:\\movies\\movie\\"},
    {"D:\\movies\\movie\\file.avi", "D:\\movies\\movie\\"},
    {"D:\\movies\\movie\\disc 1\\file.avi", "D:\\movies\\movie\\"},
    {"D:\\movies\\movie\\disk 1\\file.avi", "D:\\movies\\movie\\"},
    {"D:\\movies\\movie\\cd 1\\file.avi", "D:\\movies\\movie\\"},
    {"D:\\movies\\movie\\dvd 1\\file.avi", "D:\\movies\\movie\\"},
    // Server path tests
    {"\\\\Server\\Movies\\movie\\video_ts\\", "\\\\Server\\Movies\\movie\\"},
    {"\\\\Server\\Movies\\movie\\disc 1\\video_ts\\", "\\\\Server\\Movies\\movie\\"},
    {"\\\\Server\\Movies\\movie\\BDMV\\", "\\\\Server\\Movies\\movie\\"},
    {"\\\\Server\\Movies\\movie\\disc 1\\BDMV\\index.bdmv", "\\\\Server\\Movies\\movie\\"},
    {"\\\\Server\\Movies\\movie\\file.iso", "\\\\Server\\Movies\\movie\\"},
    {"\\\\Server\\Movies\\movie\\disc 1\\file.iso", "\\\\Server\\Movies\\movie\\"},
    {"\\\\Server\\Movies\\movie\\file.avi", "\\\\Server\\Movies\\movie\\"},
    {"\\\\Server\\Movies\\movie\\disc 1\\file.avi", "\\\\Server\\Movies\\movie\\"},
    {"\\\\Server\\Movies\\movie\\disk 1\\file.avi", "\\\\Server\\Movies\\movie\\"},
    {"\\\\Server\\Movies\\movie\\cd 1\\file.avi", "\\\\Server\\Movies\\movie\\"},
    {"\\\\Server\\Movies\\movie\\dvd 1\\file.avi", "\\\\Server\\Movies\\movie\\"},
    // URL path tests with smb://
    {"smb://home/user/movies/movie/video_ts/", "smb://home/user/movies/movie/"},
    {"smb://home/user/movies/movie/disc 1/video_ts/", "smb://home/user/movies/movie/"},
    {"smb://home/user/movies/movie/BDMV/", "smb://home/user/movies/movie/"},
    {"smb://home/user/movies/movie/disc 1/BDMV/index.bdmv", "smb://home/user/movies/movie/"},
    {"smb://home/user/movies/movie/file.iso", "smb://home/user/movies/movie/"},
    {"smb://home/user/movies/movie/disc 1/file.iso", "smb://home/user/movies/movie/"},
    {"smb://home/user/movies/movie/file.avi", "smb://home/user/movies/movie/"},
    {"smb://home/user/movies/movie/disc 1/file.avi", "smb://home/user/movies/movie/"},
    {"smb://home/user/movies/movie/disk 1/file.avi", "smb://home/user/movies/movie/"},
    {"smb://home/user/movies/movie/cd 1/file.avi", "smb://home/user/movies/movie/"},
    {"smb://home/user/movies/movie/dvd 1/file.avi", "smb://home/user/movies/movie/"},
    // Embedded linux path tests
    {"bluray://udf%3a%2f%2f%252fsomepath%252fpath%252fdisc%25201%252fmovie.iso%2f/"
     "BDMV/PLAYLIST/00800.mpls",
     "/somepath/path/"},
    {"zip://%2fsomepath%2fpath%2fdisc%202%2fmovie.zip/BDMV/index.bdmv", "/somepath/path/"},
    {"rar://%2fsomepath%2fpath%2fdisc%203%2fmovie.rar/BDMV/index.bdmv", "/somepath/path/"},
    {"archive://%2fsomepath%2fpath%2fdisc%204%2fmovie.tar.gz/VIDEO_TS/VIDEO_TS.IFO",
     "/somepath/path/"},
    // Embedded DOS path tests
    {"bluray://udf%3a%2f%2fD%253a%255csomepath%255cdisc%25201%255cmovie.iso%2f/"
     "BDMV/PLAYLIST/00800.mpls",
     "D:\\somepath\\"},
    {"zip://D%3a%5csomepath%5cdisc%202%5cmovie.zip/BDMV/index.bdmv", "D:\\somepath\\"},
    {"rar://D%3a%5csomepath%5cdisc%203%5cmovie.rar/BDMV/index.bdmv", "D:\\somepath\\"},
    {"archive://D%3a%5csomepath%5cdisc%204%5cmovie.tar.gz/VIDEO_TS/VIDEO_TS.IFO", "D:\\somepath\\"},
    // Embedded Windows server path tests
    {"bluray://udf%3a%2f%2f%255c%255cServer%255cMovie%255cdisc%25201%255cmovie.iso%2f/"
     "BDMV/PLAYLIST/00800.mpls",
     "\\\\Server\\Movie\\"},
    {"zip://%5c%5cServer%5cMovie%5cdisc%202%5cmovie.zip/BDMV/index.bdmv", "\\\\Server\\Movie\\"},
    {"rar://%5c%5cServer%5cMovie%5cdisc%203%5cmovie.rar/BDMV/index.bdmv", "\\\\Server\\Movie\\"},
    {"archive://%5c%5cServer%5cMovie%5cdisc%204%5cmovie.tar.gz/VIDEO_TS/VIDEO_TS.IFO",
     "\\\\Server\\Movie\\"},
    // Embedded URL tests with smb://
    {"bluray://udf%3a%2f%2fsmb%253a%252f%252fsomepath%252fpath%252fdisc%25201%252fmovie.iso%2f/"
     "BDMV/PLAYLIST/00800.mpls",
     "smb://somepath/path/"},
    {"zip://smb%3a%2f%2fsomepath%2fpath%2fdisc%202%2fmovie.zip/BDMV/index.bdmv",
     "smb://somepath/path/"},
    {"rar://smb%3a%2f%2fsomepath%2fpath%2fdisc%203%2fmovie.rar/BDMV/index.bdmv",
     "smb://somepath/path/"},
    {"archive://smb%3a%2f%2fsomepath%2fpath%2fdisc%204%2fmovie.tar.gz/VIDEO_TS/VIDEO_TS.IFO",
     "smb://somepath/path/"}};

TEST_P(TestRemoveDiscNumbers, RemoveDiscNumbers)
{
  const std::string path = CUtil::RemoveTrailingPartNumberSegmentFromPath(
      GetParam().path, CUtil::PreserveFileName::REMOVE);
  EXPECT_EQ(GetParam().result, path);
}

INSTANTIATE_TEST_SUITE_P(RemoveDiscNumbers, TestRemoveDiscNumbers, ValuesIn(BasePaths));

struct TestBaseData
{
  const char* path;
  const char* base;
  const char* file;
};

class TestVideoBasePathAndFileName : public Test, public WithParamInterface<TestBaseData>
{
};

const TestBaseData Paths[] = {
    // Linux path tests
    {"/home/user/movies/movie/video_ts/video_ts.ifo", "/home/user/movies/movie/", "movie"},
    {"/home/user/movies/movie/disc 1/video_ts/video_ts.ifo", "/home/user/movies/movie/disc 1/",
     "movie"},
    {"/home/user/movies/movie/BDMV/index.bdmv", "/home/user/movies/movie/", "movie"},
    {"/home/user/movies/movie/disc 1/BDMV/index.bdmv", "/home/user/movies/movie/disc 1/", "movie"},
    {"/home/user/movies/movie/file.iso", "/home/user/movies/movie/", "file"},
    {"/home/user/movies/movie/disc 1/file.iso", "/home/user/movies/movie/disc 1/", "file"},
    // DOS path tests
    {"D:\\movies\\movie\\video_ts\\video_ts.ifo", "D:\\movies\\movie\\", "movie"},
    {"D:\\movies\\movie\\disc 1\\video_ts\\video_ts.ifo", "D:\\movies\\movie\\disc 1\\", "movie"},
    {"D:\\movies\\movie\\BDMV\\index.bdmv", "D:\\movies\\movie\\", "movie"},
    {"D:\\movies\\movie\\disc 1\\BDMV\\index.bdmv", "D:\\movies\\movie\\disc 1\\", "movie"},
    {"D:\\movies\\movie\\file.iso", "D:\\movies\\movie\\", "file"},
    {"D:\\movies\\movie\\disc 1\\file.iso", "D:\\movies\\movie\\disc 1\\", "file"},
    // Windows server path tests
    {"\\\\Server\\Movies\\movie\\video_ts\\video_ts.ifo", "\\\\Server\\Movies\\movie\\", "movie"},
    {"\\\\Server\\Movies\\movie\\disc 1\\video_ts\\video_ts.ifo",
     "\\\\Server\\Movies\\movie\\disc 1\\", "movie"},
    {"\\\\Server\\Movies\\movie\\BDMV\\index.bdmv", "\\\\Server\\Movies\\movie\\", "movie"},
    {"\\\\Server\\Movies\\movie\\disc 1\\BDMV\\index.bdmv", "\\\\Server\\Movies\\movie\\disc 1\\",
     "movie"},
    {"\\\\Server\\Movies\\movie\\file.iso", "\\\\Server\\Movies\\movie\\", "file"},
    {"\\\\Server\\Movies\\movie\\disc 1\\file.iso", "\\\\Server\\Movies\\movie\\disc 1\\", "file"},
    // URL path tests with smb://
    {"smb://home/user/movies/movie/video_ts/video_ts.ifo", "smb://home/user/movies/movie/",
     "movie"},
    {"smb://home/user/movies/movie/disc 1/video_ts/video_ts.ifo",
     "smb://home/user/movies/movie/disc 1/", "movie"},
    {"smb://home/user/movies/movie/BDMV/index.bdmv", "smb://home/user/movies/movie/", "movie"},
    {"smb://home/user/movies/movie/disc 1/BDMV/index.bdmv", "smb://home/user/movies/movie/disc 1/",
     "movie"},
    {"smb://home/user/movies/movie/file.iso", "smb://home/user/movies/movie/", "file"},
    {"smb://home/user/movies/movie/disc 1/file.iso", "smb://home/user/movies/movie/disc 1/",
     "file"},
    // Embedded URL tests with smb://
    {"bluray://udf%3a%2f%2fsmb%253a%252f%252fsomepath%252fmovie.iso%2f/BDMV/PLAYLIST/00800.mpls",
     "smb://somepath/", "movie"},
    {"bluray://udf%3a%2f%2fsmb%253a%252f%252fsomepath%252fdisc%25201%252fmovie.iso%2f/BDMV/"
     "PLAYLIST/"
     "00800.mpls",
     "smb://somepath/disc 1/", "movie"},
    {"bluray://smb%3a%2f%2fsomepath%2fmovie%2f/BDMV/PLAYLIST/00800.mpls", "smb://somepath/movie/",
     "movie"},
    {"bluray://smb%3a%2f%2fsomepath%2fmovie%2fdisc%201%2f/BDMV/PLAYLIST/00800.mpls",
     "smb://somepath/movie/disc 1/", "movie"},
    {"zip://smb%3a%2f%2fsomepath%2fmovie%2fmovie.zip/BDMV/index.BDMV", "smb://somepath/movie/",
     "movie"},
    {"zip://smb%3a%2f%2fsomepath%2fmovie%2fdisc%201%2fmovie.zip/file.mkv",
     "smb://somepath/movie/disc 1/", "file"},
    {"rar://smb%3a%2f%2fsomepath%2fmovie%2fmovie.rar/BDMV/index.BDMV", "smb://somepath/movie/",
     "movie"},
    {"rar://smb%3a%2f%2fsomepath%2fmovie%2fdisc%201%2fmovie.rar/file.mkv",
     "smb://somepath/movie/disc 1/", "file"},
    {"archive://smb%3a%2f%2fsomepath%2fmovie%2fmovie.tar.gz/BDMV/index.BDMV",
     "smb://somepath/movie/", "movie"},
    {"archive://smb%3a%2f%2fsomepath%2fmovie%2fdisc%201%2fmovie.tar.gz/file.mkv",
     "smb://somepath/movie/disc 1/", "file"},
    // Embedded linux path tests
    {"bluray://udf%3a%2f%2f%252fsomepath%252fdisc%25201%252fmovie.iso%2f/BDMV/"
     "PLAYLIST/"
     "00800.mpls",
     "/somepath/disc 1/", "movie"},
    {"bluray://%2fsomepath%2fmovie%2f/BDMV/PLAYLIST/00800.mpls", "/somepath/movie/", "movie"},
    {"bluray://%2fsomepath%2fmovie%2fdisc%201%2f/BDMV/PLAYLIST/00800.mpls",
     "/somepath/movie/disc 1/", "movie"},
    {"zip://%2fsomepath%2fmovie%2fmovie.zip/BDMV/index.BDMV", "/somepath/movie/", "movie"},
    {"zip://smb%3a%2f%2fsomepath%2fmovie%2fdisc%201%2fmovie.zip/file.mkv",
     "smb://somepath/movie/disc 1/", "file"},
    {"rar://%2fsomepath%2fmovie%2fmovie.rar/BDMV/index.BDMV", "/somepath/movie/", "movie"},
    {"rar://%2fsomepath%2fmovie%2fdisc%201%2fmovie.rar/file.mkv", "/somepath/movie/disc 1/",
     "file"},
    {"archive://%2fsomepath%2fmovie%2fmovie.tar.gz/BDMV/index.BDMV", "/somepath/movie/", "movie"},
    {"archive://%2fsomepath%2fmovie%2fdisc%201%2fmovie.tar.gz/file.mkv", "/somepath/movie/disc 1/",
     "file"},
    // Embedded DOS path tests
    {"bluray://udf%3a%2f%2fD%253a%255csomepath%255cmovie%255cdisc%25201%255cmovie.iso%2f/BDMV/"
     "PLAYLIST/"
     "00800.mpls",
     "D:\\somepath\\movie\\disc 1\\", "movie"},
    {"bluray://D%3a%5csomepath%5cmovie%5c/BDMV/PLAYLIST/00800.mpls", "D:\\somepath\\movie\\",
     "movie"},
    {"bluray://D%3a%5csomepath%5cmovie%5cdisc%201%5c/BDMV/PLAYLIST/00800.mpls",
     "D:\\somepath\\movie\\disc 1\\", "movie"},
    {"zip://D%3a%5csomepath%5cmovie%5cmovie.zip/BDMV/index.BDMV", "D:\\somepath\\movie\\", "movie"},
    {"zip://D%3a%5csomepath%5cmovie%5cdisc%201%5cmovie.zip/file.mkv",
     "D:\\somepath\\movie\\disc 1\\", "file"},
    {"rar://D%3a%5csomepath%5cmovie%5cmovie.rar/BDMV/index.BDMV", "D:\\somepath\\movie\\", "movie"},
    {"rar://D%3a%5csomepath%5cmovie%5cdisc%201%5cmovie.rar/file.mkv",
     "D:\\somepath\\movie\\disc 1\\", "file"},
    {"archive://D%3a%5csomepath%5cmovie%5cmovie.tar.gz/BDMV/index.BDMV", "D:\\somepath\\movie\\",
     "movie"},
    {"archive://D%3a%5csomepath%5cmovie%5cdisc%201%5cmovie.tar.gz/file.mkv",
     "D:\\somepath\\movie\\disc 1\\", "file"},
    // Embedded windows server path tests
    {"bluray://udf%3a%2f%2f%255c%255cServer%255cMovies%255cmovie%255cdisc%25201%255cmovie.iso%2f/"
     "BDMV/"
     "PLAYLIST/"
     "00800.mpls",
     "\\\\Server\\Movies\\movie\\disc 1\\", "movie"},
    {"bluray://%5c%5cServer%5cMovies%5cmovie%5cdisc%201%5c/BDMV/PLAYLIST/00800.mpls",
     "\\\\Server\\Movies\\movie\\disc 1\\", "movie"},
    {"zip://%5c%5cServer%5cMovies%5cmovie%5cmovie.zip/BDMV/index.BDMV",
     "\\\\Server\\Movies\\movie\\", "movie"},
    {"zip://%5c%5cServer%5cMovies%5cmovie%5cdisc%201%5cmovie.zip/file.mkv",
     "\\\\Server\\Movies\\movie\\disc 1\\", "file"},
    {"rar://%5c%5cServer%5cMovies%5cmovie%5cmovie.rar/BDMV/index.BDMV",
     "\\\\Server\\Movies\\movie\\", "movie"},
    {"rar://%5c%5cServer%5cMovies%5cmovie%5cdisc%201%5cmovie.rar/file.mkv",
     "\\\\Server\\Movies\\movie\\disc 1\\", "file"},
    {"archive://%5c%5cServer%5cMovies%5cmovie%5cmovie.tar.gz/BDMV/index.BDMV",
     "\\\\Server\\Movies\\movie\\", "movie"},
    {"archive://%5c%5cServer%5cMovies%5cmovie%5cdisc%201%5cmovie.tar.gz/file.mkv",
     "\\\\Server\\Movies\\movie\\disc 1\\", "file"},
    {"file:/videos/movie.mkv?cache=1#part", "file:/videos/", "movie"},
    {"http:/videos/movie.mkv?cache=1#part", "http:/videos/", "movie"},
    {"http://host/videos/movie.mkv?cache=1#part|auth=token", "http://host/videos/|auth=token",
     "movie"},
    {"magnet:/videos/movie.mkv?xt=urn:btih:hash&tr=one&tr=two#part",
     "magnet:/videos/?xt=urn:btih:hash&tr=one&tr=two#part", "movie"},
    // HTTP URL with query string
    {"http://192.168.1.1/movie.mkv?session=abc", "http://192.168.1.1/", "movie"},
    {"http://192.168.1.1/path/movie.mkv?session=abc", "http://192.168.1.1/path/", "movie"},
    // Endpoint-style URL where the query selects the media
    {"http://192.168.1.1/stream?file=movie.mkv", "http://192.168.1.1/", "stream"},
    {"http://192.168.1.1/dir/stream?file=movie.mkv", "http://192.168.1.1/dir/", "stream"},
    // HTTP URL with multi-param query string
    {"http://192.168.0.110:80/immich/album-2025/VID_20250624_114200.mp4?index=1&play",
     "http://192.168.0.110:80/immich/album-2025/", "VID_20250624_114200"}};

TEST_P(TestVideoBasePathAndFileName, GetVideoBasePathAndFileName)
{
  std::string base;
  std::string file;
  CUtil::GetVideoBasePathAndFileName(GetParam().path, base, file);
  EXPECT_EQ(GetParam().base, base);
  EXPECT_EQ(GetParam().file, file);
}

INSTANTIATE_TEST_SUITE_P(GetVideoBasePathAndFileName,
                         TestVideoBasePathAndFileName,
                         ValuesIn(Paths));

struct SourceData
{
  const char* name;
  const char* path;
};

struct TestMatchingSourceData
{
  const char* path;
  int matchingSource{-1};
};

class TestMatchingSource : public Test, public WithParamInterface<TestMatchingSourceData>
{
};

constexpr SourceData Sources[] = {{"Movies", "smb://somepath/Movies/"},
                                  {"TV Shows", "smb://somepath/TV Shows/"},
                                  {"Documentaries", "smb://somepath/Documentaries/"},
                                  {"Movies", "/somepath/Movies/"},
                                  {"TV Shows", "/somepath/TV Shows/"},
                                  {"Documentaries", "/somepath/Documentaries/"},
                                  {"Movies", "D:\\somepath\\Movies\\"},
                                  {"TV Shows", "D:\\somepath\\TV Shows\\"},
                                  {"Documentaries", "D:\\somepath\\Documentaries\\"},
                                  {"Movies", "\\\\Server\\Movies\\"},
                                  {"TV Shows", "\\\\Server\\TV Shows\\"},
                                  {"Documentaries", "\\\\Server\\Documentaries\\"}};

constexpr TestMatchingSourceData SourcesToMatch[] = {
    // URL path tests with smb://
    {"smb://somepath/Movies/Alien (1979)/ALIEN.ISO", 0},
    {"smb://somepath/Movies/Alien (1979)/Disc 1/ALIEN.ISO", 0},
    {"smb://somepath/Movies/Alien (1979)/Disc 1/BDMV/index.bdmv", 0},
    {"bluray://"
     "udf%3a%2f%2fsmb%253a%252f%252fsomepath%252fMovies%252fAlien%2520(1979)%252fALIEN.ISO%2f/"
     "BDMV/"
     "PLAYLIST/"
     "00800.mpls",
     0},
    {"bluray://"
     "udf%3a%2f%2fsmb%253a%252f%252fsomepath%252fMovies%252fAlien%2520(1979)%252fDisc%25201%252f"
     "ALIEN.ISO%2f/BDMV/PLAYLIST/"
     "00800.mpls",
     0},
    {"bluray://smb%3a%2f%2fsomepath%2fMovies%2fAlien%2520(1979)/BDMV/PLAYLIST/00800.mpls", 0},
    {"bluray://smb%3a%2f%2fsomepath%2fMovies%2fAlien%2520(1979)%2fDisc%201/BDMV/PLAYLIST/"
     "00800.mpls",
     0},
    {"smb://somepath/TV Shows/A Perfect Planet (2021)/", 1},
    {"smb://somepath/Other/Something Else/", -1},
    {"zip://smb%3a%2f%2fsomepath%2fMovies%2fmovie.zip/BDMV/index.BDMV", 0},
    {"zip://smb%3a%2f%2fsomepath%2fTV%20Shows%2fShowf%2fdisc%201%2fshow.rar/file.mkv", 1},
    {"rar://smb%3a%2f%2fsomepath%2fMovies%2fmovie.rar/BDMV/index.BDMV", 0},
    {"rar://smb%3a%2f%2fsomepath%2fTV%20Shows%2fShowf%2fdisc%201%2fshow.rar/file.mkv", 1},
    {"archive://smb%3a%2f%2fsomepath%2fMovies%2fmovie.tar.gz/BDMV/index.BDMV", 0},
    {"archive://smb%3a%2f%2fsomepath%2fTV%20Shows%2fShowf%2fdisc%201%2fshow.tar.gz/file.mkv", 1},
    {"stack://smb://somepath/Documentaries/other/part 1.mkv , "
     "smb://somepath/Documentaries/other/part 2.mkv",
     2},
    {"stack://smb://somepath/Documentaries/other/other part 1/file.mkv , "
     "smb://somepath/Documentaries/other/other part 2/file.mkv",
     2},
    // Linux path tests
    {"/somepath/Movies/Alien (1979)/ALIEN.ISO", 3},
    {"/somepath/Movies/Alien (1979)/Disc 1/ALIEN.ISO", 3},
    {"/somepath/Movies/Alien (1979)/Disc 1/BDMV/index.bdmv", 3},
    {"bluray://"
     "udf%3a%2f%2f%252fsomepath%252fMovies%252fAlien%2520(1979)%252fALIEN.ISO%2f/BDMV/"
     "PLAYLIST/"
     "00800.mpls",
     3},
    {"bluray://"
     "udf%3a%2f%2f%252fsomepath%252fMovies%252fAlien%2520(1979)%252fDisc%25201%252f"
     "ALIEN.ISO%2f/BDMV/PLAYLIST/"
     "00800.mpls",
     3},
    {"bluray://%2fsomepath%2fMovies%2fAlien%2520(1979)/BDMV/PLAYLIST/00800.mpls", 3},
    {"bluray://%2fsomepath%2fMovies%2fAlien%2520(1979)%2fDisc%201/BDMV/PLAYLIST/"
     "00800.mpls",
     3},
    {"/somepath/TV Shows/A Perfect Planet (2021)/", 4},
    {"/somepath/Other/Something Else/", -1},
    {"zip://%2fsomepath%2fMovies%2fmovie.zip/BDMV/index.BDMV", 3},
    {"zip://%2fsomepath%2fTV%20Shows%2fShowf%2fdisc%201%2fshow.rar/file.mkv", 4},
    {"rar://%2fsomepath%2fMovies%2fmovie.rar/BDMV/index.BDMV", 3},
    {"rar://%2fsomepath%2fTV%20Shows%2fShowf%2fdisc%201%2fshow.rar/file.mkv", 4},
    {"archive://%2fsomepath%2fMovies%2fmovie.tar.gz/BDMV/index.BDMV", 3},
    {"archive://%2fsomepath%2fTV%20Shows%2fShowf%2fdisc%201%2fshow.tar.gz/file.mkv", 4},
    {"stack:///somepath/Documentaries/other/part 1.mkv , "
     "/somepath/Documentaries/other/part 2.mkv",
     5},
    {"stack:///somepath/Documentaries/other/other part 1/file.mkv , "
     "/somepath/Documentaries/other/other part 2/file.mkv",
     5},
    // DOS path tests
    {"D:\\somepath\\Movies\\Alien (1979)\\ALIEN.ISO", 6},
    {"D:\\somepath\\Movies\\Alien (1979)\\Disc 1\\ALIEN.ISO", 6},
    {"D:\\somepath\\Movies\\Alien (1979)\\Disc 1\\BDMV\\index.bdmv", 6},
    {"bluray://udf%3a%2f%2fD%253a%255csomepath%255cMovies%255cAlien%2520(1979)%255cmovie.iso%2f/"
     "BDMV/"
     "PLAYLIST/"
     "00800.mpls",
     6},
    {"bluray://"
     "udf%3a%2f%2fD%253a%255csomepath%255cMovies%255cAlien%2520(1979)%255cdisc%25201%255cmovie.iso%"
     "2f/BDMV/"
     "PLAYLIST/"
     "00800.mpls",
     6},
    {"bluray://"
     "D%3a%5csomepath%5cMovies%5cAlien%20(1979)%5c/BDMV/PLAYLIST/"
     "00800.mpls",
     6},
    {"bluray://"
     "D%3a%5csomepath%5cMovies%5cAlien%20(1979)%5cdisc%201%5c/BDMV/PLAYLIST/"
     "00800.mpls",
     6},
    {"D:\\somepath\\TV Shows\\A Perfect Planet (2021)\\", 7},
    {"D:\\somepath\\Other/Something Else\\", -1},
    {"zip://D%3a%5csomepath%5cMovies%5cmovie.zip/BDMV/index.BDMV", 6},
    {"zip://D%3a%5csomepath%5cTV%20Shows%5cdisc%201%5cshow.rar/file.mkv", 7},
    {"rar://D%3a%5csomepath%5cMovies%5cmovie.rar/BDMV/index.BDMV", 6},
    {"rar://D%3a%5csomepath%5cTV%20Shows%5cdisc%201%5cshow.rar/file.mkv", 7},
    {"archive://D%3a%5csomepath%5cMovies%5cmovie.tar.gz/BDMV/index.BDMV", 6},
    {"archive://D%3a%5csomepath%5cTV%20Shows%5cdisc%201%2fshow.tar.gz/file.mkv", 7},
    {"stack://D:\\somepath\\Documentaries\\other\\part 1.mkv , "
     "D:\\somepath\\Documentaries\\other\\part 2.mkv",
     8},
    {"stack://D:\\somepath\\Documentaries\\other\\other part 1\\file.mkv , "
     "D:\\somepath\\Documentaries\\other\\other part 2\\file.mkv",
     8},
    // Windows server path tests
    {"\\\\Server\\Movies\\Alien (1979)\\ALIEN.ISO", 9},
    {"\\\\Server\\Movies\\Alien (1979)\\Disc 1\\ALIEN.ISO", 9},
    {"\\\\Server\\Movies\\Alien (1979)\\Disc 1\\BDMV\\index.bdmv", 9},
    {"bluray://udf%3a%2f%2f%255c%255cServer%255cMovies%255cAlien%2520(1979)%255cmovie.iso%2f/"
     "BDMV/"
     "PLAYLIST/"
     "00800.mpls",
     9},
    {"bluray://"
     "udf%3a%2f%2f%255c%255cServer%255cMovies%255cAlien%2520(1979)%255cdisc%25201%255cmovie.iso%"
     "2f/BDMV/"
     "PLAYLIST/"
     "00800.mpls",
     9},
    {"bluray://"
     "%5c%5cServer%5cMovies%5cAlien%20(1979)%5c/BDMV/PLAYLIST/"
     "00800.mpls",
     9},
    {"bluray://"
     "%5c%5cServer%5cMovies%5cAlien%20(1979)%5cdisc%201%5c/BDMV/PLAYLIST/"
     "00800.mpls",
     9},
    {"\\\\Server\\TV Shows\\A Perfect Planet (2021)\\", 10},
    {"\\\\Server\\Other\\Something Else\\", -1},
    {"zip://%5c%5cServer%5cMovies%5cmovie.zip/BDMV/index.BDMV", 9},
    {"zip://%5c%5cServer%5cTV%20Shows%5cdisc%201%5cshow.rar/file.mkv", 10},
    {"rar://%5c%5cServer%5cMovies%5cmovie.rar/BDMV/index.BDMV", 9},
    {"rar://%5c%5cServer%5cTV%20Shows%5cdisc%201%5cshow.rar/file.mkv", 10},
    {"archive://%5c%5cServer%5cMovies%5cmovie.tar.gz/BDMV/index.BDMV", 9},
    {"archive://%5c%5cServer%5cTV%20Shows%5cdisc%201%2fshow.tar.gz/file.mkv", 10},
    {"stack://\\\\Server\\Documentaries\\other\\part 1.mkv , "
     "\\\\Server\\Documentaries\\other\\part 2.mkv",
     11},
    {"stack://\\\\Server\\Documentaries\\other\\other part 1\\file.mkv , "
     "\\\\Server\\Documentaries\\other\\other part 2\\file.mkv",
     11},
};

TEST_P(TestMatchingSource, GetMatchingSource)
{
  char dosDriveLetter{'D'};

#if defined(TARGET_WINDOWS_DESKTOP)
  // D: is often an optical drive. Use a non-optical letter so this fixture tests source matching
  // rather than the intentional optical-source shortcut.
  if (URIUtils::IsOnDVD("D:\\"))
  {
    for (char candidate = 'C'; candidate <= 'Z'; ++candidate)
    {
      std::string driveRoot{"C:\\"};
      driveRoot.front() = candidate;
      if (!URIUtils::IsOnDVD(driveRoot))
      {
        dosDriveLetter = candidate;
        break;
      }
    }
  }
  ASSERT_FALSE(URIUtils::IsOnDVD(std::string{dosDriveLetter} + ":\\"));
#endif

  const auto replaceOpticalTestDrive = [dosDriveLetter](const char* value)
  {
    std::string path{value};
    const std::string drive{dosDriveLetter};
    StringUtils::Replace(path, "D:\\", drive + ":\\");
    StringUtils::Replace(path, "D%3a", drive + "%3a");
    StringUtils::Replace(path, "D%253a", drive + "%253a");
    return path;
  };

  // Generate sources
  std::vector<CMediaSource> sources;
  for (const auto& source : Sources)
  {
    const std::string sourcePath{replaceOpticalTestDrive(source.path)};
    CMediaSource mediaSource;
    mediaSource.strName = source.name;
    mediaSource.strPath = sourcePath;
    mediaSource.m_iDriveType = SourceType::REMOTE;
    mediaSource.vecPaths = {sourcePath};
    sources.emplace_back(mediaSource);
  }

  bool isSourceName{false};
  int source{
      CUtil::GetMatchingSource(replaceOpticalTestDrive(GetParam().path), sources, isSourceName)};
  EXPECT_EQ(source, GetParam().matchingSource);
}

INSTANTIATE_TEST_SUITE_P(GetMatchingSource, TestMatchingSource, ValuesIn(SourcesToMatch));

struct TestExternalStreamData
{
  std::string videoPath;
  std::string associatedFile;
  std::string language;
  unsigned int flag;
};

// clang-format off
const TestExternalStreamData ExternalStreams[] = {
    // The language is a BCP 47 tag, which uses the alpha-2 code where one exists, whichever form
    // the filename used
    {"/movies/BigBuckBunny.mkv", "/movies/BigBuckBunny.en.srt", "en", StreamFlags::FLAG_NONE},
    {"/movies/BigBuckBunny.mkv", "/movies/BigBuckBunny.eng.srt", "en", StreamFlags::FLAG_NONE},
    // A language with no ISO 639-1 code keeps its alpha-3 form
    {"/movies/BigBuckBunny.mkv", "/movies/BigBuckBunny.ady.srt", "ady", StreamFlags::FLAG_NONE},
    // _ stands in for the BCP 47 - subtag separator, since - separates filename tokens
    {"/movies/BigBuckBunny.mkv", "/movies/BigBuckBunny.en_AU.srt", "en-AU", StreamFlags::FLAG_NONE},
    {"/movies/BigBuckBunny.mkv", "/movies/BigBuckBunny.pt_BR.srt", "pt-BR", StreamFlags::FLAG_NONE},
    // Flags, before and after the language
    {"/movies/BigBuckBunny.mkv", "/movies/BigBuckBunny.forced.en.srt", "en", StreamFlags::FLAG_FORCED},
    {"/movies/BigBuckBunny.mkv", "/movies/BigBuckBunny.en.forced.srt", "en", StreamFlags::FLAG_FORCED},
    {"/movies/BigBuckBunny.mkv", "/movies/BigBuckBunny.en.default.srt", "en", StreamFlags::FLAG_DEFAULT},
    {"/movies/BigBuckBunny.mkv", "/movies/BigBuckBunny.en.original.srt", "en", StreamFlags::FLAG_ORIGINAL},
    {"/movies/BigBuckBunny.mkv", "/movies/BigBuckBunny.en.impaired.srt", "en", StreamFlags::FLAG_HEARING_IMPAIRED},
    // No language in the filename
    {"/movies/BigBuckBunny.mkv", "/movies/BigBuckBunny.srt", "", StreamFlags::FLAG_NONE},
    // The language nearest the extension is the stream's, and anything before it is its name
    {"/movies/BigBuckBunny.mkv", "/movies/BigBuckBunny.director.en.srt", "en", StreamFlags::FLAG_NONE},
    {"/movies/BigBuckBunny.mkv", "/movies/BigBuckBunny.en.fr.srt", "fr", StreamFlags::FLAG_NONE},
};
// clang-format on

class TestExternalStreamDetails : public Test, public WithParamInterface<TestExternalStreamData>
{
};

TEST_P(TestExternalStreamDetails, GetExternalStreamDetailsFromFilename)
{
  const ExternalStreamInfo info =
      CUtil::GetExternalStreamDetailsFromFilename(GetParam().videoPath, GetParam().associatedFile);

  EXPECT_EQ(info.language.AsBcp47(), GetParam().language);
  EXPECT_EQ(info.flag, GetParam().flag);
}

INSTANTIATE_TEST_SUITE_P(GetExternalStreamDetailsFromFilename,
                         TestExternalStreamDetails,
                         ValuesIn(ExternalStreams));

//! A movie folder, M3GAN, holding a bluray of several versions (playlists)
class TestVersionSubtitles : public Test
{
protected:
  void SetUp() override
  {
    XFILE::CDirectory::RemoveRecursive(m_root);
    for (const std::string& path : {m_root, m_folder, URIUtils::AddFileToFolder(m_folder, "BDMV")})
      ASSERT_TRUE(XFILE::CDirectory::Create(path));
  }

  void TearDown() override { XFILE::CDirectory::RemoveRecursive(m_root); }

  //! A folder in the movie's
  void Folder(const std::string& name) const
  {
    ASSERT_TRUE(XFILE::CDirectory::Create(URIUtils::AddFileToFolder(m_folder, name)));
  }

  std::string File(const std::string& name) const
  {
    const std::string path{URIUtils::AddFileToFolder(m_folder, name)};
    XFILE::CFile output;
    EXPECT_TRUE(output.OpenForWrite(path, true));
    output.Close();
    return path;
  }

  //! The subtitles found for playlist 800 of the disc, sorted
  static std::vector<std::string> SubtitlesOf800(const std::string& disc)
  {
    std::vector<std::string> subtitles;
    CUtil::ScanForExternalSubtitles(URIUtils::GetBlurayPlaylistPath(disc, 800), subtitles);
    std::ranges::sort(subtitles);
    return subtitles;
  }

  const std::string m_root{URIUtils::AddFileToFolder(
      CSpecialProtocol::TranslatePath("special://temp/"), "TestVersionSubtitles")};
  const std::string m_folder{URIUtils::AddFileToFolder(m_root, "M3GAN")};
};

// The version of a folder rip has the subtitles named after the folder holding BDMV, and those
// beside BDMV named as its art is, but not those of another version
TEST_F(TestVersionSubtitles, AFolderRipsVersionHasItsOwnSubtitles)
{
  const std::string disc{File(URIUtils::AddFileToFolder("BDMV", "index.bdmv"))};
  std::vector<std::string> expected{File("M3GAN.eng.srt"), File("M3GAN-00800.eng.srt"),
                                    File("index-00800.eng.ass"), File("00800.fre.srt")};
  File("M3GAN-00801.eng.srt");
  File("index-00801.eng.ass");
  File("00801.fre.srt");

  std::ranges::sort(expected);
  EXPECT_EQ(expected, SubtitlesOf800(disc));
}

// The version of an ISO has the subtitles named after the ISO, but not those of another version
TEST_F(TestVersionSubtitles, AnIsosVersionHasItsOwnSubtitles)
{
  const std::string disc{File("M3GAN.iso")};
  std::vector<std::string> expected{File("M3GAN.eng.srt"), File("M3GAN-00800.eng.srt"),
                                    File("M3GAN-2022.eng.srt")};
  File("M3GAN-00801.eng.srt");

  std::ranges::sort(expected);
  EXPECT_EQ(expected, SubtitlesOf800(disc));
}

// A disc in a folder of its own within the movie's has the subtitles beside it, named after the
// movie's folder or as its art is
TEST_F(TestVersionSubtitles, AFolderRipInADiscFolderHasItsOwnSubtitles)
{
  Folder("disc 1");
  Folder(URIUtils::AddFileToFolder("disc 1", "BDMV"));
  const std::string disc{File(URIUtils::AddFileToFolder("disc 1", "BDMV", "index.bdmv"))};
  std::vector<std::string> expected{File(URIUtils::AddFileToFolder("disc 1", "M3GAN.eng.srt")),
                                    File(URIUtils::AddFileToFolder("disc 1", "index-00800.ass")),
                                    File(URIUtils::AddFileToFolder("disc 1", "00800.fre.srt"))};
  File(URIUtils::AddFileToFolder("disc 1", "index-00801.ass"));
  File(URIUtils::AddFileToFolder("disc 1", "disc 1.eng.srt"));
  File("M3GAN.ger.srt");

  std::ranges::sort(expected);
  EXPECT_EQ(expected, SubtitlesOf800(disc));
}

TEST_F(TestVersionSubtitles, AnIsoInADiscFolderHasItsOwnSubtitles)
{
  Folder("disc 1");
  const std::string disc{File(URIUtils::AddFileToFolder("disc 1", "M3GAN.iso"))};
  std::vector<std::string> expected{File(URIUtils::AddFileToFolder("disc 1", "M3GAN.eng.srt")),
                                    File(URIUtils::AddFileToFolder("disc 1", "M3GAN-00800.srt"))};
  File(URIUtils::AddFileToFolder("disc 1", "M3GAN-00801.srt"));

  std::ranges::sort(expected);
  EXPECT_EQ(expected, SubtitlesOf800(disc));
}

// A subtitles folder beside the disc is the disc's too
TEST_F(TestVersionSubtitles, AVersionsSubtitlesCanBeInASubtitlesFolder)
{
  Folder("subs");
  const std::string disc{File(URIUtils::AddFileToFolder("BDMV", "index.bdmv"))};
  std::vector<std::string> expected{File(URIUtils::AddFileToFolder("subs", "index-00800.ass")),
                                    File(URIUtils::AddFileToFolder("subs", "00800.fre.srt"))};
  File(URIUtils::AddFileToFolder("subs", "index-00801.ass"));

  std::ranges::sort(expected);
  EXPECT_EQ(expected, SubtitlesOf800(disc));
}

// The custom subtitle folder can be shared by many discs, so a version's name says nothing there
TEST_F(TestVersionSubtitles, AVersionsSubtitlesAreNotTakenFromTheCustomFolder)
{
  const std::string custom{URIUtils::AddFileToFolder(m_root, "custom")};
  ASSERT_TRUE(XFILE::CDirectory::Create(custom));
  const auto settings{CServiceBroker::GetSettingsComponent()->GetSettings()};
  const std::string customPath{settings->GetString(CSettings::SETTING_SUBTITLES_CUSTOMPATH)};
  settings->SetString(CSettings::SETTING_SUBTITLES_CUSTOMPATH, custom);
  CMediaSettings::GetInstance().SetAdditionalSubtitleDirectoryChecked(0);

  const std::string disc{File(URIUtils::AddFileToFolder("BDMV", "index.bdmv"))};
  const std::vector<std::string> expected{URIUtils::AddFileToFolder(custom, "M3GAN.eng.srt")};
  for (const char* name : {"M3GAN.eng.srt", "index-00800.ass", "00800.fre.srt"})
  {
    XFILE::CFile output;
    EXPECT_TRUE(output.OpenForWrite(URIUtils::AddFileToFolder(custom, name), true));
  }
  const std::vector<std::string> subtitles{SubtitlesOf800(disc)};

  settings->SetString(CSettings::SETTING_SUBTITLES_CUSTOMPATH, customPath);
  CMediaSettings::GetInstance().SetAdditionalSubtitleDirectoryChecked(0);
  EXPECT_EQ(expected, subtitles);
}

// Several ISOs can share a folder: each version has only what is named after its own ISO, and
// nothing named by a playlist alone
TEST_F(TestVersionSubtitles, EachOfSeveralIsosInAFolderHasOnlyItsOwnSubtitles)
{
  const std::string disc{File("A.iso")};
  File("B.iso");
  std::vector<std::string> expected{File("A.eng.srt"), File("A-00800.eng.srt")};
  File("A-00801.eng.srt");
  File("B.eng.srt");
  File("B-00800.eng.srt");
  File("00800.eng.srt");
  File("index-00800.eng.srt");

  std::ranges::sort(expected);
  EXPECT_EQ(expected, SubtitlesOf800(disc));
}

// The subtitles in another version's archive aren't offered, though named for no version
TEST_F(TestVersionSubtitles, AnotherVersionsArchivedSubtitlesAreNotOffered)
{
  const std::string disc{File("M3GAN.iso")};
  const std::string archive{XBMC_REF_FILE_PATH("xbmc/filesystem/test/reffile.txt.zip")};
  for (const char* name : {"M3GAN-00800.zip", "M3GAN-00801.zip"})
    ASSERT_TRUE(XFILE::CFile::Copy(archive, URIUtils::AddFileToFolder(m_folder, name)));

  // reffile.txt, a subtitle by its extension, from each archive offered
  std::vector<std::string> archives;
  for (const std::string& subtitle : SubtitlesOf800(disc))
    archives.emplace_back(URIUtils::GetFileName(CURL(subtitle).GetHostName()));
  EXPECT_EQ(std::vector<std::string>{"M3GAN-00800.zip"}, archives);
}

// A disc played as itself, rather than a playlist, is no version: it has only the subtitles named
// after the movie
TEST_F(TestVersionSubtitles, AFolderRipPlayedWithoutAPlaylistHasTheMoviesSubtitles)
{
  const std::string disc{File(URIUtils::AddFileToFolder("BDMV", "index.bdmv"))};
  const std::vector<std::string> expected{File("M3GAN.eng.srt")};
  File("index-00800.eng.srt");
  File("00800.eng.srt");

  std::vector<std::string> subtitles;
  CUtil::ScanForExternalSubtitles(disc, subtitles);
  EXPECT_EQ(expected, subtitles);
}

TEST_F(TestVersionSubtitles, ADvdFolderRipHasTheMoviesSubtitles)
{
  Folder("VIDEO_TS");
  const std::string disc{File(URIUtils::AddFileToFolder("VIDEO_TS", "VIDEO_TS.IFO"))};
  const std::vector<std::string> expected{File("M3GAN.eng.srt")};

  std::vector<std::string> subtitles;
  CUtil::ScanForExternalSubtitles(disc, subtitles);
  EXPECT_EQ(expected, subtitles);
}

// A video file is no version of a bluray, so nothing named for a playlist is left out
TEST_F(TestVersionSubtitles, AVideoFileHasAllItsSubtitles)
{
  const std::string video{File("Film.mkv")};
  std::vector<std::string> expected{File("Film.eng.srt"), File("Film-00801.eng.srt")};

  std::vector<std::string> subtitles;
  CUtil::ScanForExternalSubtitles(video, subtitles);
  std::ranges::sort(subtitles);
  std::ranges::sort(expected);
  EXPECT_EQ(expected, subtitles);
}

TEST_F(TestVersionSubtitles, AnotherVersionsArchivedSubtitlesOfAFolderRipAreNotOffered)
{
  const std::string disc{File(URIUtils::AddFileToFolder("BDMV", "index.bdmv"))};
  const std::string archive{XBMC_REF_FILE_PATH("xbmc/filesystem/test/reffile.txt.zip")};
  for (const char* name : {"M3GAN-00800.zip", "M3GAN-00801.zip"})
    ASSERT_TRUE(XFILE::CFile::Copy(archive, URIUtils::AddFileToFolder(m_folder, name)));

  std::vector<std::string> archives;
  for (const std::string& subtitle : SubtitlesOf800(disc))
    archives.emplace_back(URIUtils::GetFileName(CURL(subtitle).GetHostName()));
  EXPECT_EQ(std::vector<std::string>{"M3GAN-00800.zip"}, archives);
}

TEST(TestUtil, GetVersionFileNames)
{
  using Names = std::vector<std::string>;
  const auto namesOf800 = [](const std::string& disc)
  { return CUtil::GetVersionFileNames(URIUtils::GetBlurayPlaylistPath(disc, 800)); };

  EXPECT_EQ((Names{"index-00800", "00800"}), namesOf800("/movies/M3GAN/BDMV/index.bdmv"));
  EXPECT_EQ((Names{"index-00800", "00800"}), namesOf800("/movies/M3GAN/disc 1/BDMV/index.bdmv"));
  EXPECT_EQ((Names{"M3GAN-00800"}), namesOf800("/movies/M3GAN/M3GAN.iso"));
  EXPECT_EQ((Names{"The.Matrix.1999-00800"}), namesOf800("/movies/The.Matrix.1999.iso"));
  // Not a version of a bluray
  EXPECT_EQ(Names{}, CUtil::GetVersionFileNames("/movies/M3GAN/BDMV/index.bdmv"));
  EXPECT_EQ(Names{}, CUtil::GetVersionFileNames("/movies/M3GAN/M3GAN.mkv"));
  EXPECT_EQ(Names{}, CUtil::GetVersionFileNames("/movies/M3GAN/VIDEO_TS/VIDEO_TS.IFO"));
}

TEST(TestUtil, IsAnotherVersionsFile)
{
  const std::string video{URIUtils::GetBlurayPlaylistPath("/movies/M3GAN/BDMV/index.bdmv", 800)};
  const auto isAnothers = [&video](const std::string& file)
  { return CUtil::IsAnotherVersionsFile("/movies/M3GAN/" + file, "M3GAN", video); };

  EXPECT_TRUE(isAnothers("M3GAN-00801.eng.srt"));
  EXPECT_TRUE(isAnothers("index-00801.eng.srt"));
  EXPECT_TRUE(isAnothers("INDEX-00801.eng.srt"));
  EXPECT_TRUE(isAnothers("00801.eng.srt"));
  EXPECT_FALSE(isAnothers("M3GAN-00800.eng.srt"));
  EXPECT_FALSE(isAnothers("index-00800.eng.srt"));
  EXPECT_FALSE(isAnothers("00800.eng.srt"));
  EXPECT_FALSE(isAnothers("M3GAN.eng.srt"));
  // A playlist number is five digits
  EXPECT_FALSE(isAnothers("M3GAN-2022.eng.srt"));
  EXPECT_FALSE(isAnothers("M3GAN-008011.eng.srt"));
  // A video named by five digits keeps its own files
  EXPECT_FALSE(CUtil::IsAnotherVersionsFile(
      "/movies/10000 BC/10000 BC.eng.srt", "10000 BC",
      URIUtils::GetBlurayPlaylistPath("/movies/10000 BC/BDMV/index.bdmv", 800)));
  // What is in another version's archive is that version's, whatever its own name, also when in an
  // archive in it
  const auto isAnothersInArchive = [&video](const std::string& archive, const std::string& file)
  {
    return CUtil::IsAnotherVersionsFile(
        URIUtils::CreateArchivePath("zip", CURL(archive), file).Get(), "M3GAN", video);
  };
  EXPECT_TRUE(isAnothersInArchive("/movies/M3GAN/M3GAN-00801.zip", "English.srt"));
  EXPECT_FALSE(isAnothersInArchive("/movies/M3GAN/M3GAN-00800.zip", "English.srt"));
  EXPECT_TRUE(isAnothersInArchive(
      URIUtils::CreateArchivePath("zip", CURL("/movies/M3GAN/M3GAN-00801.zip"), "subs.zip").Get(),
      "English.srt"));
  // Nothing is another version's for what is no version
  EXPECT_FALSE(CUtil::IsAnotherVersionsFile("/movies/M3GAN/M3GAN-00801.eng.srt", "M3GAN",
                                            "/movies/M3GAN/M3GAN.mkv"));
}

//! The external audio found for playlist 800 of the disc, sorted
class TestVersionAudio : public TestVersionSubtitles
{
protected:
  void SetUp() override
  {
    // The scan asks the video database whether to use all the audio beside the video
    if (!CServiceBroker::GetDatabaseManager().CanOpen("MyVideos"))
    {
      ASSERT_TRUE(CServiceBroker::GetDatabaseManager().Initialize());
    }
    TestVersionSubtitles::SetUp();
  }

  static std::vector<std::string> AudioOf800(const std::string& disc)
  {
    std::vector<std::string> audio;
    CUtil::ScanForExternalAudio(URIUtils::GetBlurayPlaylistPath(disc, 800), audio);
    std::ranges::sort(audio);
    return audio;
  }
};

// The version of a folder rip has the audio named after the folder holding BDMV, and that beside
// BDMV named as its art is, but not another version's
TEST_F(TestVersionAudio, AFolderRipsVersionHasItsOwnAudio)
{
  const std::string disc{File(URIUtils::AddFileToFolder("BDMV", "index.bdmv"))};
  std::vector<std::string> expected{File("M3GAN.ac3"), File("M3GAN-00800.ac3"),
                                    File("index-00800.mka"), File("00800.dts")};
  File("M3GAN-00801.ac3");
  File("index-00801.mka");
  File("00801.dts");

  std::ranges::sort(expected);
  EXPECT_EQ(expected, AudioOf800(disc));
}

// The version of an ISO has the audio named after the ISO, but not another version's
TEST_F(TestVersionAudio, AnIsosVersionHasItsOwnAudio)
{
  const std::string disc{File("M3GAN.iso")};
  std::vector<std::string> expected{File("M3GAN.ac3"), File("M3GAN-00800.ac3")};
  File("M3GAN-00801.ac3");

  std::ranges::sort(expected);
  EXPECT_EQ(expected, AudioOf800(disc));
}

TEST_F(TestVersionAudio, EachOfSeveralIsosInAFolderHasOnlyItsOwnAudio)
{
  const std::string disc{File("A.iso")};
  File("B.iso");
  std::vector<std::string> expected{File("A.ac3"), File("A-00800.ac3")};
  File("A-00801.ac3");
  File("B.ac3");
  File("B-00800.ac3");
  File("00800.ac3");

  std::ranges::sort(expected);
  EXPECT_EQ(expected, AudioOf800(disc));
}

// A video file is no version of a bluray, so nothing named for a playlist is left out
TEST_F(TestVersionAudio, AVideoFileHasAllItsAudio)
{
  const std::string video{File("Film.mkv")};
  std::vector<std::string> expected{File("Film.ac3"), File("Film-00801.ac3")};

  std::vector<std::string> audio;
  CUtil::ScanForExternalAudio(video, audio);
  std::ranges::sort(audio);
  std::ranges::sort(expected);
  EXPECT_EQ(expected, audio);
}

TEST_F(TestVersionAudio, AFolderRipInADiscFolderHasItsOwnAudio)
{
  Folder("disc 1");
  Folder(URIUtils::AddFileToFolder("disc 1", "BDMV"));
  const std::string disc{File(URIUtils::AddFileToFolder("disc 1", "BDMV", "index.bdmv"))};
  std::vector<std::string> expected{File(URIUtils::AddFileToFolder("disc 1", "M3GAN.ac3")),
                                    File(URIUtils::AddFileToFolder("disc 1", "index-00800.mka")),
                                    File(URIUtils::AddFileToFolder("disc 1", "00800.dts"))};
  File(URIUtils::AddFileToFolder("disc 1", "index-00801.mka"));
  File(URIUtils::AddFileToFolder("disc 1", "disc 1.ac3"));

  std::ranges::sort(expected);
  EXPECT_EQ(expected, AudioOf800(disc));
}

TEST_F(TestVersionAudio, AnIsoInADiscFolderHasItsOwnAudio)
{
  Folder("disc 1");
  const std::string disc{File(URIUtils::AddFileToFolder("disc 1", "M3GAN.iso"))};
  std::vector<std::string> expected{File(URIUtils::AddFileToFolder("disc 1", "M3GAN.ac3")),
                                    File(URIUtils::AddFileToFolder("disc 1", "M3GAN-00800.ac3"))};
  File(URIUtils::AddFileToFolder("disc 1", "M3GAN-00801.ac3"));

  std::ranges::sort(expected);
  EXPECT_EQ(expected, AudioOf800(disc));
}

// An audio folder beside the disc is the disc's too
TEST_F(TestVersionAudio, AVersionsAudioCanBeInAnAudioFolder)
{
  Folder("audio");
  const std::string disc{File(URIUtils::AddFileToFolder("BDMV", "index.bdmv"))};
  std::vector<std::string> expected{File(URIUtils::AddFileToFolder("audio", "index-00800.mka")),
                                    File(URIUtils::AddFileToFolder("audio", "00800.dts"))};
  File(URIUtils::AddFileToFolder("audio", "index-00801.mka"));

  std::ranges::sort(expected);
  EXPECT_EQ(expected, AudioOf800(disc));
}

// A version's own subtitle is not given the version's name, as one named after the video is not
TEST(TestUtil, ExternalStreamDetailsOfABlurayVersionsSubtitle)
{
  const std::string external{
      CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(21602)};
  for (const auto& [disc, subtitle] :
       {std::pair{"/movies/M3GAN/BDMV/index.bdmv", "/movies/M3GAN/index-00800.eng.forced.ass"},
        std::pair{"/movies/M3GAN/BDMV/index.bdmv", "/movies/M3GAN/00800.eng.forced.ass"},
        std::pair{"/movies/M3GAN/M3GAN.iso", "/movies/M3GAN/M3GAN-00800.eng.forced.ass"}})
  {
    const ExternalStreamInfo info{CUtil::GetExternalStreamDetailsFromFilename(
        URIUtils::GetBlurayPlaylistPath(disc, 800), subtitle)};
    EXPECT_EQ(info.language.AsBcp47(), "en") << subtitle;
    EXPECT_EQ(info.flag, StreamFlags::FLAG_FORCED) << subtitle;
    EXPECT_EQ(info.name, external) << subtitle;
  }
}

/*!
 * A percent-encoded path reaches here from any VFS that escapes its names - WebDAV among them.
 * Hiding the extension must not hand back the escaped form.
 */
class TestTitleFromPath : public Test
{
protected:
  void SetUp() override
  {
    const auto settings = CServiceBroker::GetSettingsComponent()->GetSettings();
    m_showExtensions = settings->GetBool(CSettings::SETTING_FILELISTS_SHOWEXTENSIONS);
    settings->SetBool(CSettings::SETTING_FILELISTS_SHOWEXTENSIONS, false);
  }

  void TearDown() override
  {
    CServiceBroker::GetSettingsComponent()->GetSettings()->SetBool(
        CSettings::SETTING_FILELISTS_SHOWEXTENSIONS, m_showExtensions);
  }

private:
  bool m_showExtensions{true};
};

TEST_F(TestTitleFromPath, DecodesAnEscapedNameWhileHidingTheExtension)
{
  EXPECT_EQ("file name", CUtil::GetTitleFromPath("davs://server/files/file%20name.mkv"));
  EXPECT_EQ("file_name", CUtil::GetTitleFromPath("davs://server/files/file_name.mkv"));
}

TEST_F(TestTitleFromPath, DecodesLosslessURIPathNames)
{
  EXPECT_EQ("C++ movie%20",
            CUtil::GetTitleFromPath("file:/videos/C++%20movie%2520.mkv?title=ignored#part"));
  EXPECT_EQ("Video clips",
            CUtil::GetTitleFromPath("http:/Video%20clips/?title=ignored#part", true));
}

/*! A local path is not escaped, so decoding one would corrupt every name holding a plus. */
TEST_F(TestTitleFromPath, LeavesALocalNameAlone)
{
  EXPECT_EQ("C++ Media", CUtil::GetTitleFromPath("/path/to/C++ Media.mkv"));
  EXPECT_EQ("100% proof", CUtil::GetTitleFromPath("/path/to/100% proof.mkv"));
}

TEST_F(TestTitleFromPath, KeepsAPlusInTheName)
{
  EXPECT_EQ("C++ Collection", CUtil::GetTitleFromPath("smb://server/share/C++ Collection.mkv"));
  EXPECT_EQ("C++ Collection", CUtil::GetTitleFromPath("davs://server/files/C++ Collection.mkv"));
}

TEST_F(TestTitleFromPath, DecodesAnEscapedPercent)
{
  EXPECT_EQ("100% proof", CUtil::GetTitleFromPath("davs://server/files/100%25%20proof.mkv"));

  // a name that itself contains the characters "%20"
  EXPECT_EQ("100%20proof", CUtil::GetTitleFromPath("davs://server/files/100%2520proof.mkv"));

  // the name is taken off the path before the decode, so an escaped separator stays in the leaf
  EXPECT_EQ("file/name", CUtil::GetTitleFromPath("davs://server/files/file%2Fname.mkv"));
}

TEST_F(TestTitleFromPath, LeavesAMalformedEscapeAlone)
{
  EXPECT_EQ("file%2", CUtil::GetTitleFromPath("davs://server/files/file%2.mkv"));
  EXPECT_EQ("file%zz", CUtil::GetTitleFromPath("davs://server/files/file%zz.mkv"));
  EXPECT_EQ("file%", CUtil::GetTitleFromPath("davs://server/files/file%.mkv"));
  EXPECT_EQ("100% off", CUtil::GetTitleFromPath("davs://server/files/100% off.mkv"));
}

TEST_F(TestTitleFromPath, LeavesANameOnANonEscapingProtocolAlone)
{
  EXPECT_EQ("file name", CUtil::GetTitleFromPath("smb://server/share/file name.mkv"));
  EXPECT_EQ("file name", CUtil::GetTitleFromPath("nfs://server/export/file name.mkv"));
  EXPECT_EQ("file name", CUtil::GetTitleFromPath("ftp://server/pub/file name.mkv"));
  EXPECT_EQ("100% proof", CUtil::GetTitleFromPath("smb://server/share/100% proof.mkv"));
}

TEST_F(TestTitleFromPath, DecodesAnArchiveNameOnlyWhenItsParentIsAUrl)
{
  EXPECT_EQ(
      "100% proof.zip",
      CUtil::GetTitleFromPath("zip://davs%3a%2f%2fserver%2ffiles%2f100%2525%2520proof.zip/", true));
  EXPECT_EQ("100%20proof.zip",
            CUtil::GetTitleFromPath("zip://%2fmedia%2f100%2520proof.zip/", true));
}

TEST(TestUtil, FoundationNativeAndNestedNormalization)
{
  EXPECT_EQ("relative?name#part+100%.mkv", CUtil::ValidatePath("relative?name#part+100%.mkv"));
  EXPECT_EQ("C:relative", CUtil::ValidatePath("C:relative"));
  EXPECT_EQ("./notes:2026", CUtil::ValidatePath("./notes:2026"));
  EXPECT_EQ("zip://%2ftmp%2fa.zip/dir//a.mkv",
            CUtil::ValidatePath("zip://%2ftmp%2fa.zip/dir//a.mkv", true));
  EXPECT_EQ("http://server/a%2Fb//c", CUtil::ValidatePath("http://server/a%2Fb//c", true));
}

namespace
{
//! The custom subtitle folder is checked once per session, the first time subtitles are looked for
class TestCustomSubtitleFolder : public Test
{
protected:
  void SetUp() override
  {
    XFILE::CDirectory::RemoveRecursive(m_folder);
    ASSERT_TRUE(XFILE::CDirectory::Create(m_folder));
    XFILE::CFile file;
    ASSERT_TRUE(file.OpenForWrite(m_video, true));
    file.Close();
    CMediaSettings::GetInstance().SetAdditionalSubtitleDirectoryChecked(0);
  }

  void TearDown() override
  {
    Settings()->SetString(CSettings::SETTING_SUBTITLES_CUSTOMPATH, m_customPath);
    CMediaSettings::GetInstance().SetAdditionalSubtitleDirectoryChecked(m_checked);
    XFILE::CDirectory::RemoveRecursive(m_folder);
  }

  static std::shared_ptr<CSettings> Settings()
  {
    return CServiceBroker::GetSettingsComponent()->GetSettings();
  }

  const std::string m_folder{
      CSpecialProtocol::TranslatePath("special://temp/TestCustomSubtitleFolder/")};
  const std::string m_video{URIUtils::AddFileToFolder(m_folder, "Film.mkv")};
  const std::string m_customPath{Settings()->GetString(CSettings::SETTING_SUBTITLES_CUSTOMPATH)};
  const int m_checked{CMediaSettings::GetInstance().GetAdditionalSubtitleDirectoryChecked()};
};
} // namespace

TEST_F(TestCustomSubtitleFolder, AFolderThatDoesNotExistStaysDisabled)
{
  ASSERT_TRUE(Settings()->SetString(CSettings::SETTING_SUBTITLES_CUSTOMPATH,
                                    URIUtils::AddFileToFolder(m_folder, "Missing\\")));

  std::vector<std::string> subtitles;
  CUtil::ScanForExternalSubtitles(m_video, subtitles);

  EXPECT_EQ(-1, CMediaSettings::GetInstance().GetAdditionalSubtitleDirectoryChecked());
}

TEST_F(TestCustomSubtitleFolder, AFolderThatExistsIsSearched)
{
  const std::string subtitles{URIUtils::AddFileToFolder(m_folder, "AlternateCaptions\\")};
  ASSERT_TRUE(XFILE::CDirectory::Create(subtitles));
  XFILE::CFile file;
  ASSERT_TRUE(file.OpenForWrite(URIUtils::AddFileToFolder(subtitles, "Film.en.srt"), true));
  file.Close();
  ASSERT_TRUE(Settings()->SetString(CSettings::SETTING_SUBTITLES_CUSTOMPATH, subtitles));

  std::vector<std::string> found;
  CUtil::ScanForExternalSubtitles(m_video, found);

  EXPECT_EQ(1, CMediaSettings::GetInstance().GetAdditionalSubtitleDirectoryChecked());
  EXPECT_EQ(1U, found.size());
}
