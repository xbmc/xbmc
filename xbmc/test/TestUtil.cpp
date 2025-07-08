/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Util.h"

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
  path = "C:/foo/bar/";
  EXPECT_EQ(CUtil::ValidatePath(path), "C:\\foo\\bar\\");
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

const TestDiscData BaseFiles[] = {{"/home/user/movies/movie/video_ts/VIDEO_TS.IFO", ""},
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
                                  {"smb://home/user/movies/movie/disc 1/file.avi", "1"}};

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
    {"smb://home/user/movies/movie/disc 1/file.avi", "smb://home/user/movies/movie/"}};

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
    {"bluray://smb%3a%2f%2fsomepath%2fmovie%2f/BDMV/PLAYLIST/00800.mpls", "smb://somepath/movie/",
     "movie"},
    {"c:\\dir\\movie.avi", "c:\\dir\\", "movie"},
    {"/dir/movie.avi", "/dir/", "movie"},
    {"smb://somepath/movie.avi", "smb://somepath/", "movie"},
    {"smb://somepath/disc 1/movie.avi", "smb://somepath/disc 1/", "movie"},
    {"/home/user/movies/movie/video_ts/video_ts.ifo", "/home/user/movies/movie/", "movie"},
    {"/home/user/movies/movie/disc 1/video_ts/video_ts.ifo", "/home/user/movies/movie/disc 1/",
     "movie"},
    {"/home/user/movies/movie/BDMV/index.bdmv", "/home/user/movies/movie/", "movie"},
    {"/home/user/movies/movie/disc 1/BDMV/index.bdmv", "/home/user/movies/movie/disc 1/", "movie"},
    {"/home/user/movies/movie/file.iso", "/home/user/movies/movie/", "file"},
    {"/home/user/movies/movie/disc 1/file.iso", "/home/user/movies/movie/disc 1/", "file"},
    {"bluray://udf%3a%2f%2fsmb%253a%252f%252fsomepath%252fmovie.iso%2f/BDMV/PLAYLIST/00800.mpls",
     "smb://somepath/", "movie"},
    {"bluray://udf%3a%2f%2fsmb%253a%252f%252fsomepath%252fdisc%201%252fmovie.iso%2f/BDMV/PLAYLIST/"
     "00800.mpls",
     "smb://somepath/disc 1/", "movie"},
    {"bluray://smb%3a%2f%2fsomepath%2fmovie%2f/BDMV/PLAYLIST/00800.mpls", "smb://somepath/movie/",
     "movie"},
    {"bluray://smb%3a%2f%2fsomepath%2fmovie%2fdisc%201%2f/BDMV/PLAYLIST/00800.mpls",
     "smb://somepath/movie/disc 1/", "movie"}};

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
                                  {"Documentaries", "/somepath/Documentaries/"}};

constexpr TestMatchingSourceData SourcesToMatch[] = {
    {"smb://somepath/Movies/Alien (1979)/ALIEN.ISO", 0},
    {"smb://somepath/Movies/Alien (1979)/Disc 1/ALIEN.ISO", 0},
    {"smb://somepath/Movies/Alien (1979)/Disc 1/BDMV/index.bdmv", 0},
    {"bluray://"
     "udf%3a%2f%2fsmb%253a%252f%252fsomepath%252fMovies%252fAlien%20(1979)%252fALIEN.ISO%2f/BDMV/"
     "PLAYLIST/"
     "00800.mpls",
     0},
    {"bluray://"
     "udf%3a%2f%2fsmb%253a%252f%252fsomepath%252fMovies%252fAlien%20(1979)%252fDisc%201%252f"
     "ALIEN.ISO%2f/BDMV/PLAYLIST/"
     "00800.mpls",
     0},
    {"bluray://smb%3a%2f%2fsomepath%2fMovies%2fAlien%20(1979)/BDMV/PLAYLIST/00800.mpls", 0},
    {"bluray://smb%3a%2f%2fsomepath%2fMovies%2fAlien%20(1979)%2fDisc%201/BDMV/PLAYLIST/00800.mpls",
     0},
    {"stack:///somepath/Documentaries/other/part 1.mkv,/somepath/Documentaries/other/part 2.mkv",
     2},
    {"smb://somepath/TV Shows/A Perfect Planet (2021)/", 1},
    {"smb://somepath/Other/Something Else/", -1}};

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
