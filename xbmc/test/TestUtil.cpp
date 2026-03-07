/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Util.h"

#include <gtest/gtest-param-test.h>
#include <gtest/gtest.h>

using ::testing::Test;
using ::testing::ValuesIn;
using ::testing::WithParamInterface;

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
};

TEST_P(TestUtilCleanString, GetFilenameIdentifier)
{
  std::string identifierType;
  std::string identifier;
  CUtil::GetFilenameIdentifier(GetParam().input, identifierType, identifier);
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
     "\\\\Server\\Movies\\movie\\disc 1\\", "file"}};

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
  // Generate sources
  std::vector<CMediaSource> sources;
  for (const auto& source : Sources)
  {
    CMediaSource mediaSource{
        source.name,   "",    "",  source.path, SourceType::REMOTE, KODI::UTILS::CLockInfo{}, "",
        {source.path}, false, true};
    sources.emplace_back(mediaSource);
  }

  bool isSourceName{false};
  int source{CUtil::GetMatchingSource(GetParam().path, sources, isSourceName)};
  EXPECT_EQ(source, GetParam().matchingSource);
}

INSTANTIATE_TEST_SUITE_P(GetMatchingSource, TestMatchingSource, ValuesIn(SourcesToMatch));
