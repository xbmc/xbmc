/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ServiceBroker.h"
#include "URL.h"
#include "filesystem/File.h"
#include "filesystem/MultiPathDirectory.h"
#include "interfaces/legacy/WindowInterceptor.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "test/TestUtils.h"
#include "utils/URIUtils.h"

#include <array>
#include <utility>

#include <gtest/gtest.h>

using ::testing::Test;
using ::testing::ValuesIn;
using ::testing::WithParamInterface;

using namespace XFILE;

class TestURIUtils : public testing::Test
{
protected:
  TestURIUtils() = default;
  ~TestURIUtils() override
  {
    CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_pathSubstitutions.clear();
  }
};

TEST_F(TestURIUtils, PathHasParent)
{
  EXPECT_TRUE(URIUtils::PathHasParent("/path/to/movie.avi", "/path/to/"));
  EXPECT_FALSE(URIUtils::PathHasParent("/path/to/movie.avi", "/path/2/"));
}

TEST_F(TestURIUtils, GetDirectory)
{
  EXPECT_EQ("/path/to/", URIUtils::GetDirectory("/path/to/movie.avi"));
  EXPECT_EQ("/path/to/", URIUtils::GetDirectory("/path/to/"));
  EXPECT_EQ("/path/to/|option=foo", URIUtils::GetDirectory("/path/to/movie.avi|option=foo"));
  EXPECT_EQ("/path/to/|option=foo", URIUtils::GetDirectory("/path/to/|option=foo"));
  EXPECT_EQ("", URIUtils::GetDirectory("movie.avi"));
  EXPECT_EQ("", URIUtils::GetDirectory("movie.avi|option=foo"));
  EXPECT_EQ("", URIUtils::GetDirectory(""));

  // Make sure it works when assigning to the same str as the reference parameter
  std::string var = "/path/to/movie.avi|option=foo";
  var = URIUtils::GetDirectory(var);
  EXPECT_EQ("/path/to/|option=foo", var);
}

TEST_F(TestURIUtils, GetExtension)
{
  // When testing file names with dots there is no way of telling what is an extension and what isn't
  //  without a list of extensions. This is why movie.name and .movie.name fail and are commented out

  EXPECT_EQ(".avi", URIUtils::GetExtension("/path/to/movie.avi"));
  EXPECT_EQ("", URIUtils::GetExtension("/path/to/movie."));
  EXPECT_EQ("", URIUtils::GetExtension("/path/to/.movie"));
  EXPECT_EQ("", URIUtils::GetExtension("/path/to/movie"));
  EXPECT_EQ(".ext", URIUtils::GetExtension("/path/to/.movie.ext"));
  EXPECT_EQ(".tar.gz", URIUtils::GetExtension("/path/to/movie.tar.gz"));

  EXPECT_EQ(".avi", URIUtils::GetExtension("/path/to/movie.name.avi"));
  EXPECT_EQ("", URIUtils::GetExtension("/path/to/movie.name."));
  //EXPECT_EQ("", URIUtils::GetExtension("/path/to/.movie.name"));
  //EXPECT_EQ("", URIUtils::GetExtension("/path/to/movie.name"));
  EXPECT_EQ(".ext", URIUtils::GetExtension("/path/to/.movie.name.ext"));
  EXPECT_EQ(".tar.gz", URIUtils::GetExtension("/path/to/movie.name.tar.gz"));

  EXPECT_EQ(".avi", URIUtils::GetExtension("D:\\path\\movie.avi"));
  EXPECT_EQ("", URIUtils::GetExtension("D:\\path\\movie."));
  EXPECT_EQ("", URIUtils::GetExtension("D:\\path\\.movie"));
  EXPECT_EQ("", URIUtils::GetExtension("D:\\path\\movie"));
  EXPECT_EQ(".ext", URIUtils::GetExtension("D:\\path\\.movie.ext"));
  EXPECT_EQ(".tar.gz", URIUtils::GetExtension("D:\\path\\movie.tar.gz"));

  EXPECT_EQ(".avi", URIUtils::GetExtension("D:\\path\\movie.name.avi"));
  EXPECT_EQ("", URIUtils::GetExtension("D:\\path\\movie.name."));
  //EXPECT_EQ("", URIUtils::GetExtension("D:\\path\\.movie.name"));
  //EXPECT_EQ("", URIUtils::GetExtension("D:\\path\\movie.name"));
  EXPECT_EQ(".ext", URIUtils::GetExtension("D:\\path\\.movie.name.ext"));
  EXPECT_EQ(".tar.gz", URIUtils::GetExtension("D:\\path\\movie.name.tar.gz"));

  EXPECT_EQ(".avi", URIUtils::GetExtension("\\\\Server\\path\\movie.avi"));
  EXPECT_EQ("", URIUtils::GetExtension("\\\\Server\\path\\movie."));
  EXPECT_EQ("", URIUtils::GetExtension("\\\\Server\\path\\.movie"));
  EXPECT_EQ("", URIUtils::GetExtension("\\\\Server\\path\\movie"));
  EXPECT_EQ(".ext", URIUtils::GetExtension("\\\\Server\\path\\.movie.ext"));
  EXPECT_EQ(".tar.gz", URIUtils::GetExtension("\\\\Server\\path\\movie.tar.gz"));

  EXPECT_EQ(".avi", URIUtils::GetExtension("\\\\Server\\path\\movie.name.avi"));
  EXPECT_EQ("", URIUtils::GetExtension("\\\\Server\\path\\movie.name."));
  //EXPECT_EQ("", URIUtils::GetExtension("\\\\Server\\path\\.movie.name"));
  //EXPECT_EQ("", URIUtils::GetExtension("\\\\Server\\path\\movie.name"));
  EXPECT_EQ(".ext", URIUtils::GetExtension("\\\\Server\\path\\.movie.name.ext"));
  EXPECT_EQ(".tar.gz", URIUtils::GetExtension("\\\\Server\\path\\movie.name.tar.gz"));

  EXPECT_EQ(".avi", URIUtils::GetExtension("smb://path/to/movie.avi"));
  EXPECT_EQ("", URIUtils::GetExtension("smb://path/to/movie."));
  EXPECT_EQ("", URIUtils::GetExtension("smb://path/to/.movie"));
  EXPECT_EQ("", URIUtils::GetExtension("smb://path/to/movie"));
  EXPECT_EQ(".ext", URIUtils::GetExtension("smb://path/to/.movie.ext"));
  EXPECT_EQ(".tar.gz", URIUtils::GetExtension("smb://path/to/movie.tar.gz"));

  EXPECT_EQ(".avi", URIUtils::GetExtension("smb://path/to/movie.name.avi"));
  EXPECT_EQ("", URIUtils::GetExtension("smb://path/to/movie.name."));
  //EXPECT_EQ("", URIUtils::GetExtension("smb://path/to/.movie.name"));
  //EXPECT_EQ("", URIUtils::GetExtension("smb://path/to/movie.name"));
  EXPECT_EQ(".ext", URIUtils::GetExtension("smb://path/to/.movie.name.ext"));
  EXPECT_EQ(".tar.gz", URIUtils::GetExtension("smb://path/to/movie.name.tar.gz"));

  EXPECT_EQ("", URIUtils::GetExtension("."));
  EXPECT_EQ("", URIUtils::GetExtension(".."));
  EXPECT_EQ("", URIUtils::GetExtension("/path/to/."));
  EXPECT_EQ("", URIUtils::GetExtension("/path/to/.."));
  EXPECT_EQ("", URIUtils::GetExtension("D:\\path\\."));
  EXPECT_EQ("", URIUtils::GetExtension("D:\\path\\.."));
  EXPECT_EQ("", URIUtils::GetExtension("\\\\server\\path\\."));
  EXPECT_EQ("", URIUtils::GetExtension("\\\\server\\path\\.."));

  EXPECT_EQ(".avi", URIUtils::GetExtension("movie.avi"));
  EXPECT_EQ("", URIUtils::GetExtension("movie."));
  EXPECT_EQ("", URIUtils::GetExtension(".movie"));
  EXPECT_EQ("", URIUtils::GetExtension("movie"));
  EXPECT_EQ(".ext", URIUtils::GetExtension(".movie.ext"));
  EXPECT_EQ(".tar.gz", URIUtils::GetExtension("movie.tar.gz"));

  EXPECT_EQ(".avi", URIUtils::GetExtension("movie.name.avi"));
  EXPECT_EQ("", URIUtils::GetExtension("movie.name."));
  //EXPECT_EQ("", URIUtils::GetExtension(".movie.name"));
  //EXPECT_EQ("", URIUtils::GetExtension("movie.name"));
  EXPECT_EQ(".ext", URIUtils::GetExtension(".movie.name.ext"));
  EXPECT_EQ(".tar.gz", URIUtils::GetExtension("movie.name.tar.gz"));
}

TEST_F(TestURIUtils, HasExtension)
{
  EXPECT_TRUE(URIUtils::HasExtension("/path/to/movie.AvI"));
  EXPECT_FALSE(URIUtils::HasExtension("/path/to/movie"));
  EXPECT_FALSE(URIUtils::HasExtension("/path/.to/movie"));
  EXPECT_FALSE(URIUtils::HasExtension(""));

  EXPECT_TRUE(URIUtils::HasExtension("/path/to/movie.AvI", ".avi"));
  EXPECT_FALSE(URIUtils::HasExtension("/path/to/movie.AvI", ".mkv"));
  EXPECT_FALSE(URIUtils::HasExtension("/path/.avi/movie", ".avi"));
  EXPECT_FALSE(URIUtils::HasExtension("", ".avi"));

  EXPECT_TRUE(URIUtils::HasExtension("/path/movie.AvI", ".avi|.mkv|.mp4"));
  EXPECT_TRUE(URIUtils::HasExtension("/path/movie.AvI", ".mkv|.avi|.mp4"));
  EXPECT_FALSE(URIUtils::HasExtension("/path/movie.AvI", ".mpg|.mkv|.mp4"));
  EXPECT_FALSE(URIUtils::HasExtension("/path.mkv/movie.AvI", ".mpg|.mkv|.mp4"));
  EXPECT_FALSE(URIUtils::HasExtension("", ".avi|.mkv|.mp4"));

  EXPECT_TRUE(URIUtils::HasExtension("D:\\Path\\movie.AvI"));
  EXPECT_FALSE(URIUtils::HasExtension("D:\\Path\\movie"));
  EXPECT_FALSE(URIUtils::HasExtension("D:\\Path\\.to\\movie"));
  EXPECT_FALSE(URIUtils::HasExtension(""));

  EXPECT_TRUE(URIUtils::HasExtension("D:\\Path\\movie.AvI", ".avi"));
  EXPECT_FALSE(URIUtils::HasExtension("D:\\Path\\movie.AvI", ".mkv"));
  EXPECT_FALSE(URIUtils::HasExtension("D:\\Path\\.avi\\movie", ".avi"));
  EXPECT_FALSE(URIUtils::HasExtension("", ".avi"));

  EXPECT_TRUE(URIUtils::HasExtension("D:\\Path\\movie.AvI", ".avi|.mkv|.mp4"));
  EXPECT_TRUE(URIUtils::HasExtension("D:\\Path\\movie.AvI", ".mkv|.avi|.mp4"));
  EXPECT_FALSE(URIUtils::HasExtension("D:\\Path\\movie.AvI", ".mpg|.mkv|.mp4"));
  EXPECT_FALSE(URIUtils::HasExtension("D:\\Path.mkv\\movie.AvI", ".mpg|.mkv|.mp4"));
  EXPECT_FALSE(URIUtils::HasExtension("", ".avi|.mkv|.mp4"));

  EXPECT_TRUE(URIUtils::HasExtension("smb://path/to/movie.AvI"));
  EXPECT_FALSE(URIUtils::HasExtension("smb://path/to/movie"));
  EXPECT_FALSE(URIUtils::HasExtension("smb://path/.to/movie"));
  EXPECT_FALSE(URIUtils::HasExtension(""));

  EXPECT_TRUE(URIUtils::HasExtension("smb://path/to/movie.AvI", ".avi"));
  EXPECT_FALSE(URIUtils::HasExtension("smb://path/to/movie.AvI", ".mkv"));
  EXPECT_FALSE(URIUtils::HasExtension("smb://path/.avi/movie", ".avi"));
  EXPECT_FALSE(URIUtils::HasExtension("", ".avi"));

  EXPECT_TRUE(URIUtils::HasExtension("smb://path/movie.AvI", ".avi|.mkv|.mp4"));
  EXPECT_TRUE(URIUtils::HasExtension("smb://path/movie.AvI", ".mkv|.avi|.mp4"));
  EXPECT_FALSE(URIUtils::HasExtension("smb://path/movie.AvI", ".mpg|.mkv|.mp4"));
  EXPECT_FALSE(URIUtils::HasExtension("smb://path.mkv/movie.AvI", ".mpg|.mkv|.mp4"));
  EXPECT_FALSE(URIUtils::HasExtension("", ".avi|.mkv|.mp4"));

  EXPECT_TRUE(URIUtils::HasExtension("movie.AvI"));
  EXPECT_FALSE(URIUtils::HasExtension("movie"));
  EXPECT_FALSE(URIUtils::HasExtension("movie."));
  EXPECT_FALSE(URIUtils::HasExtension(".movie"));
  EXPECT_FALSE(URIUtils::HasExtension(""));

  EXPECT_TRUE(URIUtils::HasExtension("movie.AvI", ".avi"));
  EXPECT_FALSE(URIUtils::HasExtension("movie.AvI", ".mkv"));
  EXPECT_FALSE(URIUtils::HasExtension("movie", ".avi"));
  EXPECT_FALSE(URIUtils::HasExtension("movie.", ".avi"));
  EXPECT_FALSE(URIUtils::HasExtension(".movie", ".avi"));
  EXPECT_FALSE(URIUtils::HasExtension("", ".avi"));

  EXPECT_TRUE(URIUtils::HasExtension("movie.AvI", ".avi|.mkv|.mp4"));
  EXPECT_TRUE(URIUtils::HasExtension("movie.AvI", ".mkv|.avi|.mp4"));
  EXPECT_FALSE(URIUtils::HasExtension("movie.AvI", ".mpg|.mkv|.mp4"));
  EXPECT_FALSE(URIUtils::HasExtension("", ".avi|.mkv|.mp4"));
}

TEST_F(TestURIUtils, GetFileName)
{
  EXPECT_EQ("movie.avi", URIUtils::GetFileName("/path/to/movie.avi"));
}

TEST_F(TestURIUtils, RemoveExtension)
{
  std::string ref;
  std::string var;

  /* NOTE: CSettings need to be set to find other extensions. */
  ref = "/path/to/file";
  var = "/path/to/file.xml";
  URIUtils::RemoveExtension(var);
  EXPECT_EQ(ref, var);

  var = "/path/to/file.zip";
  URIUtils::RemoveExtension(var);
  EXPECT_EQ(ref, var);

  var = "/path/to/file.rar";
  URIUtils::RemoveExtension(var);
  EXPECT_EQ(ref, var);

  var = "/path/to/file.gz";
  URIUtils::RemoveExtension(var);
  EXPECT_EQ(ref, var);

  var = "/path/to/file.tar";
  URIUtils::RemoveExtension(var);
  EXPECT_EQ(ref, var);

  var = "/path/to/file.tar.gz";
  URIUtils::RemoveExtension(var);
  EXPECT_EQ(ref, var);

  var = "/path/to/file.";
  URIUtils::RemoveExtension(var);
  EXPECT_EQ(ref, var);

  var = "/path/to/file";
  URIUtils::RemoveExtension(var);
  EXPECT_EQ(ref, var);

  ref = "/path/to/.file";
  var = "/path/to/.file";
  URIUtils::RemoveExtension(var);
  EXPECT_EQ(ref, var);

  ref = "/path/to/file.name";
  var = "/path/to/file.name.mp4";
  URIUtils::RemoveExtension(var);
  EXPECT_EQ(ref, var);

  var = "/path/to/file.name.tar.gz";
  URIUtils::RemoveExtension(var);
  EXPECT_EQ(ref, var);

  ref = ".";
  var = ".";
  URIUtils::RemoveExtension(var);
  EXPECT_EQ(ref, var);

  ref = "..";
  var = "..";
  URIUtils::RemoveExtension(var);
  EXPECT_EQ(ref, var);

  ref = "";
  var = "";
  URIUtils::RemoveExtension(var);
  EXPECT_EQ(ref, var);

  ref = "file";
  var = "file.xml";
  URIUtils::RemoveExtension(var);
  EXPECT_EQ(ref, var);

  var = "file.zip";
  URIUtils::RemoveExtension(var);
  EXPECT_EQ(ref, var);

  var = "file.rar";
  URIUtils::RemoveExtension(var);
  EXPECT_EQ(ref, var);

  var = "file.gz";
  URIUtils::RemoveExtension(var);
  EXPECT_EQ(ref, var);

  var = "file.tar";
  URIUtils::RemoveExtension(var);
  EXPECT_EQ(ref, var);

  var = "file.tar.gz";
  URIUtils::RemoveExtension(var);
  EXPECT_EQ(ref, var);

  var = "file.";
  URIUtils::RemoveExtension(var);
  EXPECT_EQ(ref, var);

  var = "file";
  URIUtils::RemoveExtension(var);
  EXPECT_EQ(ref, var);

  ref = ".file";
  var = ".file";
  URIUtils::RemoveExtension(var);
  EXPECT_EQ(ref, var);

  ref = "file.name";
  var = "file.name.mp4";
  URIUtils::RemoveExtension(var);
  EXPECT_EQ(ref, var);

  var = "file.name.tar.gz";
  URIUtils::RemoveExtension(var);
  EXPECT_EQ(ref, var);
}

TEST_F(TestURIUtils, ReplaceExtension)
{
  std::string ref;
  std::string var;

  ref = "/path/to/file.xsd";
  var = URIUtils::ReplaceExtension("/path/to/file.xml", ".xsd");
  EXPECT_EQ(ref, var);

  var = URIUtils::ReplaceExtension("/path/to/file.tar.gz", ".xsd");
  EXPECT_EQ(ref, var);

  var = URIUtils::ReplaceExtension("/path/to/file.", ".xsd");
  EXPECT_EQ(ref, var);

  var = URIUtils::ReplaceExtension("/path/to/file", ".xsd");
  EXPECT_EQ(ref, var);

  ref = "/path/to/.file.xsd";
  var = URIUtils::ReplaceExtension("/path/to/.file", ".xsd");
  EXPECT_EQ(ref, var);

  var = URIUtils::ReplaceExtension("/path/to/.file.xml", ".xsd");
  EXPECT_EQ(ref, var);
}

TEST_F(TestURIUtils, Split)
{
  std::string refpath;
  std::string reffile;
  std::string varpath;
  std::string varfile;

  refpath = "/path/to/";
  reffile = "movie.avi";
  URIUtils::Split("/path/to/movie.avi", varpath, varfile);
  EXPECT_EQ(refpath, varpath);
  EXPECT_EQ(reffile, varfile);

  std::string varpathOptional, varfileOptional;

  refpath = "/path/to/";
  reffile = "movie?movie.avi";
  URIUtils::Split("/path/to/movie?movie.avi", varpathOptional, varfileOptional);
  EXPECT_EQ(refpath, varpathOptional);
  EXPECT_EQ(reffile, varfileOptional);

  refpath = "file:///path/to/";
  reffile = "movie.avi";
  URIUtils::Split("file:///path/to/movie.avi?showinfo=true", varpathOptional, varfileOptional);
  EXPECT_EQ(refpath, varpathOptional);
  EXPECT_EQ(reffile, varfileOptional);
}

TEST_F(TestURIUtils, SplitPath)
{
  const std::vector<std::string> array{
      URIUtils::SplitPath("http://www.test.com/path/to/movie.avi")};

  EXPECT_EQ("http://www.test.com/", array.at(0));
  EXPECT_EQ("path", array.at(1));
  EXPECT_EQ("to", array.at(2));
  EXPECT_EQ("movie.avi", array.at(3));
}

TEST_F(TestURIUtils, SplitPathLocal)
{
#ifndef TARGET_LINUX
  const char *path = "C:\\path\\to\\movie.avi";
#else
  const char *path = "/path/to/movie.avi";
#endif
  const std::vector<std::string> array{URIUtils::SplitPath(path)};

#ifndef TARGET_LINUX
  EXPECT_EQ("C:", array.at(0));
#else
  EXPECT_EQ("", array.at(0));
#endif
  EXPECT_EQ("path", array.at(1));
  EXPECT_EQ("to", array.at(2));
  EXPECT_EQ("movie.avi", array.at(3));
}

TEST_F(TestURIUtils, GetCommonPath)
{
  std::string ref;
  std::string var;

  ref = "/a/b/";
  var = "/a/b/";
  URIUtils::GetCommonPath(var, "/a/b/");
  EXPECT_EQ(ref, var);

  ref = "/a/b/";
  var = "/a/b/cd1/";
  URIUtils::GetCommonPath(var, "/a/b/cd2");
  EXPECT_EQ(ref, var);

  ref = "/a/b/";
  var = "/a/b/";
  URIUtils::GetCommonPath(var, "/a/b/c/");
  EXPECT_EQ(ref, var);

  ref = "/";
  var = "/a/";
  URIUtils::GetCommonPath(var, "/b/");
  EXPECT_EQ(ref, var);

  ref = "/a/b/";
  var = "/a/b/movie.avi";
  URIUtils::GetCommonPath(var, "/a/b/");
  EXPECT_EQ(ref, var);

  ref = "/a/b/";
  var = "/a/b/cd1/movie.avi";
  URIUtils::GetCommonPath(var, "/a/b/cd2");
  EXPECT_EQ(ref, var);

  ref = "/a/b/";
  var = "/a/b/movie.avi";
  URIUtils::GetCommonPath(var, "/a/b/c/");
  EXPECT_EQ(ref, var);

  ref = "/";
  var = "/a/movie.avi";
  URIUtils::GetCommonPath(var, "/b/");
  EXPECT_EQ(ref, var);
}

struct TestPathData
{
  const char* path;
  const char* parent;
  const char* base;
};

class TestParentPath : public testing::Test, public testing::WithParamInterface<TestPathData>
{
};

class TestBasePath : public testing::Test, public testing::WithParamInterface<TestPathData>
{
};

const TestPathData Paths[] = {
    // Linux path tests
    {.path = "/home/user/movies/movie/file.iso",
     .parent = "/home/user/movies/movie/",
     .base = "/home/user/movies/movie/"},
    {.path = "/home/user/movies/movie/disc 1/file.iso",
     .parent = "/home/user/movies/movie/disc 1/",
     .base = "/home/user/movies/movie/"},
    {.path = "/home/user/movies/movie/video_ts/VIDEO_TS.IFO",
     .parent = "/home/user/movies/movie/",
     .base = "/home/user/movies/movie/"},
    {.path = "/home/user/movies/movie/disc 1/video_ts/VIDEO_TS.IFO",
     .parent = "/home/user/movies/movie/disc 1/",
     .base = "/home/user/movies/movie/"},
    {.path = "/home/user/movies/movie/BDMV/index.bdmv",
     .parent = "/home/user/movies/movie/",
     .base = "/home/user/movies/movie/"},
    {.path = "/home/user/movies/movie/disc 1/BDMV/index.bdmv",
     .parent = "/home/user/movies/movie/disc 1/",
     .base = "/home/user/movies/movie/"},
    {.path = "stack:///home/user/movies/movie.avi , "
             "/home/user/movies/movie.avi",
     .parent = "/home/user/movies/",
     .base = "/home/user/movies/"},
    {.path = "stack:///home/user/movies/movie/cd1/some_file1.avi , "
             "/home/user/movies/movie/cd2/some_file2.avi",
     .parent = "/home/user/movies/movie/",
     .base = "/home/user/movies/movie/"},
    // DOS path tests
    {.path = "D:\\movies\\movie\\file.iso",
     .parent = "D:\\movies\\movie\\",
     .base = "D:\\movies\\movie\\"},
    {.path = "D:\\movies\\movie\\disc 1\\file.iso",
     .parent = "D:\\movies\\movie\\disc 1\\",
     .base = "D:\\movies\\movie\\"},
    {.path = "D:\\movies\\movie\\video_ts\\VIDEO_TS.IFO",
     .parent = "D:\\movies\\movie\\",
     .base = "D:\\movies\\movie\\"},
    {.path = "D:\\movies\\movie\\disc 1\\video_ts\\VIDEO_TS.IFO",
     .parent = "D:\\movies\\movie\\disc 1\\",
     .base = "D:\\movies\\movie\\"},
    {.path = "D:\\movies\\movie\\BDMV\\index.bdmv",
     .parent = "D:\\movies\\movie\\",
     .base = "D:\\movies\\movie\\"},
    {.path = "D:\\movies\\movie\\disc 1\\BDMV\\index.bdmv",
     .parent = "D:\\movies\\movie\\disc 1\\",
     .base = "D:\\movies\\movie\\"},
    {.path = "stack://D:\\movies\\movie_part_1.avi , "
             "D:\\movies\\movie_part_2.avi",
     .parent = "D:\\movies\\",
     .base = "D:\\movies\\"},
    {.path = "stack://D:\\movies\\movie\\movie_part_1\\file.avi , "
             "D:\\movies\\movie\\movie_part_2\\file.avi",
     .parent = "D:\\movies\\movie\\",
     .base = "D:\\movies\\movie\\"},
    // Windows server path tests
    {.path = "\\\\Server\\Movies\\movie\\file.iso",
     .parent = "\\\\Server\\Movies\\movie\\",
     .base = "\\\\Server\\Movies\\movie\\"},
    {.path = "\\\\Server\\Movies\\movie\\disc 1\\file.iso",
     .parent = "\\\\Server\\Movies\\movie\\disc 1\\",
     .base = "\\\\Server\\Movies\\movie\\"},
    {.path = "\\\\Server\\Movies\\movie\\video_ts\\VIDEO_TS.IFO",
     .parent = "\\\\Server\\Movies\\movie\\",
     .base = "\\\\Server\\Movies\\movie\\"},
    {.path = "\\\\Server\\Movies\\movie\\disc 1\\video_ts\\VIDEO_TS.IFO",
     .parent = "\\\\Server\\Movies\\movie\\disc 1\\",
     .base = "\\\\Server\\Movies\\movie\\"},
    {.path = "\\\\Server\\Movies\\movie\\BDMV\\index.bdmv",
     .parent = "\\\\Server\\Movies\\movie\\",
     .base = "\\\\Server\\Movies\\movie\\"},
    {.path = "\\\\Server\\Movies\\movie\\disc 1\\BDMV\\index.bdmv",
     .parent = "\\\\Server\\Movies\\movie\\disc 1\\",
     .base = "\\\\Server\\Movies\\movie\\"},
    {.path = "stack://\\\\Server\\Movies\\movie_part_1.avi , "
             "\\\\Server\\Movies\\movie_part_2.avi",
     .parent = "\\\\Server\\Movies\\",
     .base = "\\\\Server\\Movies\\"},
    {.path = "stack://\\\\Server\\Movies\\movie\\movie_part_1\\file.avi , "
             "\\\\Server\\Movies\\movie\\movie_part_2\\file.avi",
     .parent = "\\\\Server\\Movies\\movie\\",
     .base = "\\\\Server\\Movies\\movie\\"},
    // URL path tests using smb://
    {.path = "smb://server/movies/movie/file.iso",
     .parent = "smb://server/movies/movie/",
     .base = "smb://server/movies/movie/"},
    {.path = "smb://server/movies/movie/disc 1/file.iso",
     .parent = "smb://server/movies/movie/disc 1/",
     .base = "smb://server/movies/movie/"},
    {.path = "smb://server/movies/movie/video_ts/VIDEO_TS.IFO",
     .parent = "smb://server/movies/movie/",
     .base = "smb://server/movies/movie/"},
    {.path = "smb://server/movies/movie/disc 1/video_ts/VIDEO_TS.IFO",
     .parent = "smb://server/movies/movie/disc 1/",
     .base = "smb://server/movies/movie/"},
    {.path = "smb://server/movies/movie/BDMV/index.bdmv",
     .parent = "smb://server/movies/movie/",
     .base = "smb://server/movies/movie/"},
    {.path = "smb://server/movies/movie/disc 1/BDMV/index.bdmv",
     .parent = "smb://server/movies/movie/disc 1/",
     .base = "smb://server/movies/movie/"},
    {.path = "stack://smb://home/user/movies/movie.avi , "
             "smb://home/user/movies/movie.avi",
     .parent = "smb://home/user/movies/",
     .base = "smb://home/user/movies/"},
    {.path = "stack://smb://home/user/movies/movie/cd1/some_file1.avi , "
             "smb://home/user/movies/movie/cd2/some_file2.avi",
     .parent = "smb://home/user/movies/movie/",
     .base = "smb://home/user/movies/movie/"},
    // Embedded URL path tests using smb://
    {.path = "bluray://smb%3a%2f%2fsomepath%2fpath%2f/BDMV/PLAYLIST/00800.mpls",
     .parent = "smb://somepath/path/",
     .base = "smb://somepath/path/"},
    {.path = "bluray://udf%3a%2f%2fsmb%253a%252f%252fsomepath%252fpath%252fmovie.iso%2f/BDMV/"
             "PLAYLIST/00800.mpls",
     .parent = "smb://somepath/path/",
     .base = "smb://somepath/path/"},
    {.path = "zip://smb%3a%2f%2fsomepath%2fpath%2fmovie.zip/movie.mkv",
     .parent = "smb://somepath/path/",
     .base = "smb://somepath/path/"},
    {.path = "zip://smb%3a%2f%2fsomepath%2fpath%2fmovie.zip/BDMV/index.bdmv",
     .parent = "smb://somepath/path/",
     .base = "smb://somepath/path/"},
    {.path = "rar://smb%3a%2f%2fsomepath%2fpath%2fmovie.rar/movie.mkv",
     .parent = "smb://somepath/path/",
     .base = "smb://somepath/path/"},
    {.path = "rar://smb%3a%2f%2fsomepath%2fpath%2fmovie.rar/BDMV/index.bdmv",
     .parent = "smb://somepath/path/",
     .base = "smb://somepath/path/"},
    {.path = "archive://smb%3a%2f%2fsomepath%2fpath%2fmovie.tar.gz/movie.mkv",
     .parent = "smb://somepath/path/",
     .base = "smb://somepath/path/"},
    {.path = "archive://smb%3a%2f%2fsomepath%2fpath%2fmovie.tar.gz/BDMV/index.bdmv",
     .parent = "smb://somepath/path/",
     .base = "smb://somepath/path/"},
    {.path = "bluray://smb%3a%2f%2fsomepath%2fpath%2fdisc%201%2f/BDMV/PLAYLIST/00800.mpls",
     .parent = "smb://somepath/path/disc 1/",
     .base = "smb://somepath/path/"},
    {.path = "bluray://"
             "udf%3a%2f%2fsmb%253a%252f%252fsomepath%252fpath%252fdisc%25201%252fmovie.iso%2f/BDMV/"
             "PLAYLIST/00800.mpls",
     .parent = "smb://somepath/path/disc 1/",
     .base = "smb://somepath/path/"},
    {.path = "zip://smb%3a%2f%2fsomepath%2fpath%2fdisc%201%2fmovie.zip/movie.mkv",
     .parent = "smb://somepath/path/disc 1/",
     .base = "smb://somepath/path/"},
    {.path = "zip://smb%3a%2f%2fsomepath%2fpath%2fdisc%201%2fmovie.zip/BDMV/index.bdmv",
     .parent = "smb://somepath/path/disc 1/",
     .base = "smb://somepath/path/"},
    {.path = "rar://smb%3a%2f%2fsomepath%2fpath%2fdisc%201%2fmovie.rar/movie.mkv",
     .parent = "smb://somepath/path/disc 1/",
     .base = "smb://somepath/path/"},
    {.path = "rar://smb%3a%2f%2fsomepath%2fpath%2fdisc%201%2fmovie.rar/BDMV/index.bdmv",
     .parent = "smb://somepath/path/disc 1/",
     .base = "smb://somepath/path/"},
    {.path = "archive://smb%3a%2f%2fsomepath%2fpath%2fdisc%201%2fmovie.tar.gz/movie.mkv",
     .parent = "smb://somepath/path/disc 1/",
     .base = "smb://somepath/path/"},
    {.path = "archive://smb%3a%2f%2fsomepath%2fpath%2fdisc%201%2fmovie.tar.gz/BDMV/index.bdmv",
     .parent = "smb://somepath/path/disc 1/",
     .base = "smb://somepath/path/"},
    // Embedded linux path tests
    {.path = "bluray://%2fsomepath%2fpath%2f/BDMV/PLAYLIST/00800.mpls",
     .parent = "/somepath/path/",
     .base = "/somepath/path/"},
    {.path = "bluray://udf%3a%2f%2f%252fsomepath%252fpath%252fmovie.iso%2f/BDMV/"
             "PLAYLIST/00800.mpls",
     .parent = "/somepath/path/",
     .base = "/somepath/path/"},
    {.path = "zip://%2fsomepath%2fpath%2fmovie.zip/movie.mkv",
     .parent = "/somepath/path/",
     .base = "/somepath/path/"},
    {.path = "zip://%2fsomepath%2fpath%2fmovie.zip/BDMV/index.bdmv",
     .parent = "/somepath/path/",
     .base = "/somepath/path/"},
    {.path = "rar://%2fsomepath%2fpath%2fmovie.rar/movie.mkv",
     .parent = "/somepath/path/",
     .base = "/somepath/path/"},
    {.path = "rar://%2fsomepath%2fpath%2fmovie.rar/BDMV/index.bdmv",
     .parent = "/somepath/path/",
     .base = "/somepath/path/"},
    {.path = "archive://%2fsomepath%2fpath%2fmovie.tar.gz/movie.mkv",
     .parent = "/somepath/path/",
     .base = "/somepath/path/"},
    {.path = "archive://%2fsomepath%2fpath%2fmovie.tar.gz/BDMV/index.bdmv",
     .parent = "/somepath/path/",
     .base = "/somepath/path/"},
    {.path = "bluray://%2fsomepath%2fpath%2fdisc%201%2f/BDMV/PLAYLIST/00800.mpls",
     .parent = "/somepath/path/disc 1/",
     .base = "/somepath/path/"},
    {.path = "bluray://"
             "udf%3a%2f%2f%252fsomepath%252fpath%252fdisc%25201%252fmovie.iso%2f/BDMV/"
             "PLAYLIST/00800.mpls",
     .parent = "/somepath/path/disc 1/",
     .base = "/somepath/path/"},
    {.path = "zip://%2fsomepath%2fpath%2fdisc%201%2fmovie.zip/movie.mkv",
     .parent = "/somepath/path/disc 1/",
     .base = "/somepath/path/"},
    {.path = "zip://%2fsomepath%2fpath%2fdisc%201%2fmovie.zip/BDMV/index.bdmv",
     .parent = "/somepath/path/disc 1/",
     .base = "/somepath/path/"},
    {.path = "rar://%2fsomepath%2fpath%2fdisc%201%2fmovie.rar/movie.mkv",
     .parent = "/somepath/path/disc 1/",
     .base = "/somepath/path/"},
    {.path = "rar://%2fsomepath%2fpath%2fdisc%201%2fmovie.rar/BDMV/index.bdmv",
     .parent = "/somepath/path/disc 1/",
     .base = "/somepath/path/"},
    {.path = "archive://%2fsomepath%2fpath%2fdisc%201%2fmovie.tar.gz/movie.mkv",
     .parent = "/somepath/path/disc 1/",
     .base = "/somepath/path/"},
    {.path = "archive://%2fsomepath%2fpath%2fdisc%201%2fmovie.tar.gz/BDMV/index.bdmv",
     .parent = "/somepath/path/disc 1/",
     .base = "/somepath/path/"},
    // Embedded DOS path tests
    {.path = "bluray://udf%3a%2f%2fD%253a%255cMovies%255cmovie.iso%2f/BDMV/PLAYLIST/00800.mpls",
     .parent = "D:\\Movies\\",
     .base = "D:\\Movies\\"},
    {.path = "bluray://D%3a%5cMovies%5c/BDMV/PLAYLIST/00800.mpls",
     .parent = "D:\\Movies\\",
     .base = "D:\\Movies\\"},
    {.path = "zip://D%3a%5cMovies%5cmovie.zip/movie.mkv",
     .parent = "D:\\Movies\\",
     .base = "D:\\Movies\\"},
    {.path = "zip://D%3a%5cMovies%5cmovie.zip/BDMV/index.bdmv",
     .parent = "D:\\Movies\\",
     .base = "D:\\Movies\\"},
    {.path = "rar://D%3a%5cMovies%5cmovie.rar/movie.mkv",
     .parent = "D:\\Movies\\",
     .base = "D:\\Movies\\"},
    {.path = "rar://D%3a%5cMovies%5cmovie.rar/BDMV/index.bdmv",
     .parent = "D:\\Movies\\",
     .base = "D:\\Movies\\"},
    {.path = "archive://D%3a%5cMovies%5cmovie.tar.gz/movie.mkv",
     .parent = "D:\\Movies\\",
     .base = "D:\\Movies\\"},
    {.path = "archive://D%3a%5cMovies%5cmovie.tar.gz/BDMV/index.bdmv",
     .parent = "D:\\Movies\\",
     .base = "D:\\Movies\\"},
    {.path = "bluray://D%3a%5cMovies%5cDisc%201%5c/BDMV/PLAYLIST/00800.mpls",
     .parent = "D:\\Movies\\Disc 1\\",
     .base = "D:\\Movies\\"},
    {.path = "bluray://udf%3a%2f%2fD%253a%255cMovies%255cDisc%25201%255cmovie.iso%2f/BDMV/"
             "PLAYLIST/00800.mpls",
     .parent = "D:\\Movies\\Disc 1\\",
     .base = "D:\\Movies\\"},
    {.path = "zip://D%3a%5cMovies%5cDisc%201%5cmovie.zip/movie.mkv",
     .parent = "D:\\Movies\\Disc 1\\",
     .base = "D:\\Movies\\"},
    {.path = "zip://D%3a%5cMovies%5cDisc%201%5cmovie.zip/BDMV/index.bdmv",
     .parent = "D:\\Movies\\Disc 1\\",
     .base = "D:\\Movies\\"},
    {.path = "rar://D%3a%5cMovies%5cDisc%201%5cmovie.rar/movie.mkv",
     .parent = "D:\\Movies\\Disc 1\\",
     .base = "D:\\Movies\\"},
    {.path = "rar://D%3a%5cMovies%5cDisc%201%5cmovie.rar/BDMV/index.bdmv",
     .parent = "D:\\Movies\\Disc 1\\",
     .base = "D:\\Movies\\"},
    {.path = "archive://D%3a%5cMovies%5cDisc%201%5cmovie.tar.gz/movie.mkv",
     .parent = "D:\\Movies\\Disc 1\\",
     .base = "D:\\Movies\\"},
    {.path = "archive://D%3a%5cMovies%5cDisc%201%5cmovie.tar.gz/BDMV/index.bdmv",
     .parent = "D:\\Movies\\Disc 1\\",
     .base = "D:\\Movies\\"},
    // Embedded windows server path tests
    {.path = "bluray://udf%3a%2f%2f%255c%255cServer%255cMovies%255cmovie.iso%2f/BDMV/PLAYLIST/"
             "00800.mpls",
     .parent = "\\\\Server\\Movies\\",
     .base = "\\\\Server\\Movies\\"},
    {.path = "bluray://%5c%5cServer%5cMovies%5c/BDMV/PLAYLIST/00800.mpls",
     .parent = "\\\\Server\\Movies\\",
     .base = "\\\\Server\\Movies\\"},
    {.path = "zip://%5c%5cServer%5cMovies%5cmovie.zip/movie.mkv",
     .parent = "\\\\Server\\Movies\\",
     .base = "\\\\Server\\Movies\\"},
    {.path = "zip://%5c%5cServer%5cMovies%5cmovie.zip/BDMV/index.bdmv",
     .parent = "\\\\Server\\Movies\\",
     .base = "\\\\Server\\Movies\\"},
    {.path = "rar://%5c%5cServer%5cMovies%5cmovie.rar/movie.mkv",
     .parent = "\\\\Server\\Movies\\",
     .base = "\\\\Server\\Movies\\"},
    {.path = "rar://%5c%5cServer%5cMovies%5cmovie.rar/BDMV/index.bdmv",
     .parent = "\\\\Server\\Movies\\",
     .base = "\\\\Server\\Movies\\"},
    {.path = "archive://%5c%5cServer%5cMovies%5cmovie.tar.gz/movie.mkv",
     .parent = "\\\\Server\\Movies\\",
     .base = "\\\\Server\\Movies\\"},
    {.path = "archive://%5c%5cServer%5cMovies%5cmovie.tar.gz/BDMV/index.bdmv",
     .parent = "\\\\Server\\Movies\\",
     .base = "\\\\Server\\Movies\\"},
    {.path = "bluray://%5c%5cServer%5cMovies%5cDisc%201%5c/BDMV/PLAYLIST/00800.mpls",
     .parent = "\\\\Server\\Movies\\Disc 1\\",
     .base = "\\\\Server\\Movies\\"},
    {.path =
         "bluray://udf%3a%2f%2f%255c%255cServer%255cMovies%255cDisc%25201%255cmovie.iso%2f/BDMV/"
         "PLAYLIST/00800.mpls",
     .parent = "\\\\Server\\Movies\\Disc 1\\",
     .base = "\\\\Server\\Movies\\"},
    {.path = "zip://%5c%5cServer%5cMovies%5cDisc%201%5cmovie.zip/movie.mkv",
     .parent = "\\\\Server\\Movies\\Disc 1\\",
     .base = "\\\\Server\\Movies\\"},
    {.path = "zip://%5c%5cServer%5cMovies%5cDisc%201%5cmovie.zip/BDMV/index.bdmv",
     .parent = "\\\\Server\\Movies\\Disc 1\\",
     .base = "\\\\Server\\Movies\\"},
    {.path = "rar://%5c%5cServer%5cMovies%5cDisc%201%5cmovie.rar/movie.mkv",
     .parent = "\\\\Server\\Movies\\Disc 1\\",
     .base = "\\\\Server\\Movies\\"},
    {.path = "rar://%5c%5cServer%5cMovies%5cDisc%201%5cmovie.rar/BDMV/index.bdmv",
     .parent = "\\\\Server\\Movies\\Disc 1\\",
     .base = "\\\\Server\\Movies\\"},
    {.path = "archive://%5c%5cServer%5cMovies%5cDisc%201%5cmovie.tar.gz/movie.mkv",
     .parent = "\\\\Server\\Movies\\Disc 1\\",
     .base = "\\\\Server\\Movies\\"},
    {.path = "archive://%5c%5cServer%5cMovies%5cDisc%201%5cmovie.tar.gz/BDMV/index.bdmv",
     .parent = "\\\\Server\\Movies\\Disc 1\\",
     .base = "\\\\Server\\Movies\\"}};

TEST_P(TestParentPath, GetParentPath)
{
  const std::string& path{GetParam().path};
  const std::string& parent{URIUtils::GetParentPath(path)};
  EXPECT_EQ(parent, GetParam().parent);
}

INSTANTIATE_TEST_SUITE_P(ParentPath, TestParentPath, ValuesIn(Paths));

TEST_P(TestBasePath, GetBasePath)
{
  const std::string& path{GetParam().path};
  const std::string& base{URIUtils::GetBasePath(path)};
  EXPECT_EQ(base, GetParam().parent);
}

INSTANTIATE_TEST_SUITE_P(BasePath, TestBasePath, ValuesIn(Paths));

TEST_F(TestURIUtils, SubstitutePath)
{
  std::string from;
  std::string to;
  std::string ref;
  std::string var;

  from = "C:\\My Videos";
  to = "https://myserver/some%20other%20path";
  CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_pathSubstitutions.emplace_back(
      from, to);

  from = "/this/path1";
  to = "/some/other/path2";
  CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_pathSubstitutions.emplace_back(
      from, to);

  from = "davs://otherserver/my%20music%20path";
  to = "D:\\Local Music\\MP3 Collection";
  CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_pathSubstitutions.emplace_back(
      from, to);

  ref = "https://myserver/some%20other%20path/sub%20dir/movie%20name.avi";
  var = URIUtils::SubstitutePath("C:\\My Videos\\sub dir\\movie name.avi");
  EXPECT_EQ(ref, var);

  ref = "C:\\My Videos\\sub dir\\movie name.avi";
  var = URIUtils::SubstitutePath("https://myserver/some%20other%20path/sub%20dir/movie%20name.avi", true);
  EXPECT_EQ(ref, var);

  ref = "D:\\Local Music\\MP3 Collection\\Phil Collins\\Some CD\\01 - Two Hearts.mp3";
  var = URIUtils::SubstitutePath("davs://otherserver/my%20music%20path/Phil%20Collins/Some%20CD/01%20-%20Two%20Hearts.mp3");
  EXPECT_EQ(ref, var);

  ref = "davs://otherserver/my%20music%20path/Phil%20Collins/Some%20CD/01%20-%20Two%20Hearts.mp3";
  var = URIUtils::SubstitutePath("D:\\Local Music\\MP3 Collection\\Phil Collins\\Some CD\\01 - Two Hearts.mp3", true);
  EXPECT_EQ(ref, var);

  ref = "/some/other/path2/to/movie.avi";
  var = URIUtils::SubstitutePath("/this/path1/to/movie.avi");
  EXPECT_EQ(ref, var);

  ref = "/this/path1/to/movie.avi";
  var = URIUtils::SubstitutePath("/some/other/path2/to/movie.avi", true);
  EXPECT_EQ(ref, var);

  ref = "/no/translation path/";
  var = URIUtils::SubstitutePath(ref);
  EXPECT_EQ(ref, var);

  ref = "/no/translation path/";
  var = URIUtils::SubstitutePath(ref, true);
  EXPECT_EQ(ref, var);

  ref = "c:\\no\\translation path";
  var = URIUtils::SubstitutePath(ref);
  EXPECT_EQ(ref, var);

  ref = "c:\\no\\translation path";
  var = URIUtils::SubstitutePath(ref, true);
  EXPECT_EQ(ref, var);
}

TEST_F(TestURIUtils, IsAddonsPath)
{
  EXPECT_TRUE(URIUtils::IsAddonsPath("addons://path/to/addons"));
}

TEST_F(TestURIUtils, IsSourcesPath)
{
  EXPECT_TRUE(URIUtils::IsSourcesPath("sources://path/to/sources"));
}

TEST_F(TestURIUtils, IsCDDA)
{
  EXPECT_TRUE(URIUtils::IsCDDA("cdda://path/to/cdda"));
}

TEST_F(TestURIUtils, IsDOSPath)
{
  EXPECT_TRUE(URIUtils::IsDOSPath("C://path/to/dosfile"));
}

TEST_F(TestURIUtils, IsDVD)
{
  EXPECT_TRUE(URIUtils::IsDVD("dvd://path/in/video_ts.ifo"));
#if defined(TARGET_WINDOWS)
  EXPECT_TRUE(URIUtils::IsDVD("dvd://path/in/file"));
#else
  EXPECT_TRUE(URIUtils::IsDVD("iso9660://path/in/video_ts.ifo"));
  EXPECT_TRUE(URIUtils::IsDVD("udf://path/in/video_ts.ifo"));
  EXPECT_TRUE(URIUtils::IsDVD("dvd://1"));
#endif
}

TEST_F(TestURIUtils, IsFTP)
{
  EXPECT_TRUE(URIUtils::IsFTP("ftp://path/in/ftp"));
}

TEST_F(TestURIUtils, IsHD)
{
  EXPECT_TRUE(URIUtils::IsHD("/path/to/file"));
  EXPECT_TRUE(URIUtils::IsHD("file:///path/to/file"));
  EXPECT_TRUE(URIUtils::IsHD("special://path/to/file"));
  EXPECT_TRUE(URIUtils::IsHD("stack://path/to/file"));
  EXPECT_TRUE(URIUtils::IsHD("zip://path/to/file"));
}

TEST_F(TestURIUtils, IsInArchive)
{
  EXPECT_TRUE(URIUtils::IsInArchive("zip://path/to/file"));
}

TEST_F(TestURIUtils, IsInRAR)
{
  EXPECT_TRUE(URIUtils::IsInRAR("rar://path/to/file"));
}

TEST_F(TestURIUtils, IsInternetStream)
{
  CURL url1("http://path/to/file");
  CURL url2("https://path/to/file");
  EXPECT_TRUE(URIUtils::IsInternetStream(url1));
  EXPECT_TRUE(URIUtils::IsInternetStream(url2));
}

TEST_F(TestURIUtils, IsInZIP)
{
  EXPECT_TRUE(URIUtils::IsInZIP("zip://path/to/file"));
}

TEST_F(TestURIUtils, IsISO9660)
{
  EXPECT_TRUE(URIUtils::IsISO9660("iso9660://path/to/file"));
}

TEST_F(TestURIUtils, IsLiveTV)
{
  EXPECT_TRUE(URIUtils::IsLiveTV("whatever://path/to/file.pvr"));
}

TEST_F(TestURIUtils, IsPVRRadioChannel)
{
  // pvr://channels/(tv|radio)/<groupname>@<clientid>/<instanceid>@<addonid>_<channeluid>.pvr
  EXPECT_TRUE(
      URIUtils::IsPVRRadioChannel("pvr://channels/radio/groupname@0815/1@pvr.demo_4711.pvr"));
  EXPECT_FALSE(URIUtils::IsPVRRadioChannel(
      "pvr://channels/tv/groupname@0815/1@pvr.demo_4711.pvr")); // a tv channel
  EXPECT_FALSE(
      URIUtils::IsPVRRadioChannel("pvr://channels/radio/")); // root folder for all radio channels
  EXPECT_FALSE(
      URIUtils::IsPVRRadioChannel("pvr://channels/radio/groupname@0815/")); // a radio channel group
}

TEST_F(TestURIUtils, IsMultiPath)
{
  EXPECT_TRUE(URIUtils::IsMultiPath("multipath://path/to/file"));
}

TEST_F(TestURIUtils, IsMusicDb)
{
  EXPECT_TRUE(URIUtils::IsMusicDb("musicdb://path/to/file"));
}

TEST_F(TestURIUtils, IsNfs)
{
  EXPECT_TRUE(URIUtils::IsNfs("nfs://path/to/file"));
  EXPECT_TRUE(URIUtils::IsNfs("stack://nfs://path/to/file"));
}

TEST_F(TestURIUtils, IsOnDVD)
{
  EXPECT_TRUE(URIUtils::IsOnDVD("dvd://path/to/file"));
  EXPECT_TRUE(URIUtils::IsOnDVD("udf://path/to/file"));
  EXPECT_TRUE(URIUtils::IsOnDVD("iso9660://path/to/file"));
  EXPECT_TRUE(URIUtils::IsOnDVD("cdda://path/to/file"));
}

TEST_F(TestURIUtils, IsOnLAN)
{
  std::vector<std::string> multiVec;
  multiVec.emplace_back("smb://path/to/file");
  EXPECT_TRUE(URIUtils::IsOnLAN(CMultiPathDirectory::ConstructMultiPath(multiVec)));
  EXPECT_TRUE(URIUtils::IsOnLAN("stack://smb://path/to/file"));
  EXPECT_TRUE(URIUtils::IsOnLAN("smb://path/to/file"));
  EXPECT_FALSE(URIUtils::IsOnLAN("plugin://path/to/file"));
  EXPECT_TRUE(URIUtils::IsOnLAN("upnp://path/to/file"));
}

TEST_F(TestURIUtils, IsPlugin)
{
  EXPECT_TRUE(URIUtils::IsPlugin("plugin://path/to/file"));
}

TEST_F(TestURIUtils, IsScript)
{
  EXPECT_TRUE(URIUtils::IsScript("script://path/to/file"));
}

TEST_F(TestURIUtils, IsRAR)
{
  EXPECT_TRUE(URIUtils::IsRAR("/path/to/rarfile.rar"));
  EXPECT_TRUE(URIUtils::IsRAR("/path/to/rarfile.cbr"));
  EXPECT_FALSE(URIUtils::IsRAR("/path/to/file"));
  EXPECT_FALSE(URIUtils::IsRAR("rar://path/to/file"));
}

TEST_F(TestURIUtils, IsRemote)
{
  EXPECT_TRUE(URIUtils::IsRemote("http://path/to/file"));
  EXPECT_TRUE(URIUtils::IsRemote("https://path/to/file"));
  EXPECT_FALSE(URIUtils::IsRemote("addons://user/"));
  EXPECT_FALSE(URIUtils::IsRemote("sources://video/"));
  EXPECT_FALSE(URIUtils::IsRemote("videodb://movies/titles"));
  EXPECT_FALSE(URIUtils::IsRemote("musicdb://genres/"));
  EXPECT_FALSE(URIUtils::IsRemote("library://video/"));
  EXPECT_FALSE(URIUtils::IsRemote("androidapp://app"));
  EXPECT_FALSE(URIUtils::IsRemote("plugin://plugin.video.id"));
}

TEST_F(TestURIUtils, IsSmb)
{
  EXPECT_TRUE(URIUtils::IsSmb("smb://path/to/file"));
  EXPECT_TRUE(URIUtils::IsSmb("stack://smb://path/to/file"));
}

TEST_F(TestURIUtils, IsSpecial)
{
  EXPECT_TRUE(URIUtils::IsSpecial("special://path/to/file"));
  EXPECT_TRUE(URIUtils::IsSpecial("stack://special://path/to/file"));
}

TEST_F(TestURIUtils, IsStack)
{
  EXPECT_TRUE(URIUtils::IsStack("stack://path/to/file"));
}

TEST_F(TestURIUtils, IsUPnP)
{
  EXPECT_TRUE(URIUtils::IsUPnP("upnp://path/to/file"));
}

TEST_F(TestURIUtils, IsURL)
{
  EXPECT_TRUE(URIUtils::IsURL("someprotocol://path/to/file"));
  EXPECT_FALSE(URIUtils::IsURL("/path/to/file"));
}

TEST_F(TestURIUtils, IsVideoDb)
{
  EXPECT_TRUE(URIUtils::IsVideoDb("videodb://path/to/file"));
}

TEST_F(TestURIUtils, IsZIP)
{
  EXPECT_TRUE(URIUtils::IsZIP("/path/to/zipfile.zip"));
  EXPECT_TRUE(URIUtils::IsZIP("/path/to/zipfile.cbz"));
  EXPECT_FALSE(URIUtils::IsZIP("/path/to/file"));
  EXPECT_FALSE(URIUtils::IsZIP("zip://path/to/file"));
}

TEST_F(TestURIUtils, IsBlurayPath)
{
  EXPECT_TRUE(URIUtils::IsBlurayPath("bluray://path/to/file"));
  EXPECT_FALSE(URIUtils::IsBlurayPath("zip://path/to/file"));
}

TEST_F(TestURIUtils, AddSlashAtEnd)
{
  std::string ref;
  std::string var;

  ref = "bluray://path/to/file/";
  var = "bluray://path/to/file/";
  URIUtils::AddSlashAtEnd(var);
  EXPECT_EQ(ref, var);
}

TEST_F(TestURIUtils, HasSlashAtEnd)
{
  EXPECT_TRUE(URIUtils::HasSlashAtEnd("bluray://path/to/file/"));
  EXPECT_FALSE(URIUtils::HasSlashAtEnd("bluray://path/to/file"));
}

TEST_F(TestURIUtils, RemoveSlashAtEnd)
{
  std::string ref;
  std::string var;

  ref = "bluray://path/to/file";
  var = "bluray://path/to/file/";
  URIUtils::RemoveSlashAtEnd(var);
  EXPECT_EQ(ref, var);
}

TEST_F(TestURIUtils, CreateArchivePath)
{
  std::string ref;
  std::string var;

  ref = "zip://%2fpath%2fto%2f/file";
  var = URIUtils::CreateArchivePath("zip", CURL("/path/to/"), "file").Get();
  EXPECT_EQ(ref, var);
}

TEST_F(TestURIUtils, AddFileToFolder)
{
  std::string ref = "/path/to/file";
  std::string var = URIUtils::AddFileToFolder("/path/to", "file");
  EXPECT_EQ(ref, var);

  ref = "/path/to/file/and/more";
  var = URIUtils::AddFileToFolder("/path", "to", "file", "and", "more");
  EXPECT_EQ(ref, var);
}

TEST_F(TestURIUtils, HasParentInHostname)
{
  EXPECT_TRUE(URIUtils::HasParentInHostname(CURL("zip://")));
  EXPECT_TRUE(URIUtils::HasParentInHostname(CURL("bluray://")));
}

TEST_F(TestURIUtils, HasEncodedHostname)
{
  EXPECT_TRUE(URIUtils::HasEncodedHostname(CURL("zip://")));
  EXPECT_TRUE(URIUtils::HasEncodedHostname(CURL("bluray://")));
  EXPECT_TRUE(URIUtils::HasEncodedHostname(CURL("musicsearch://")));
}

TEST_F(TestURIUtils, HasEncodedFilename)
{
  EXPECT_TRUE(URIUtils::HasEncodedFilename(CURL("shout://")));
  EXPECT_TRUE(URIUtils::HasEncodedFilename(CURL("dav://")));
  EXPECT_TRUE(URIUtils::HasEncodedFilename(CURL("rss://")));
  EXPECT_TRUE(URIUtils::HasEncodedFilename(CURL("davs://")));
}

TEST_F(TestURIUtils, GetRealPath)
{
  std::string ref;

  ref = "/path/to/file/";
  EXPECT_EQ(ref, URIUtils::GetRealPath(ref));

  ref = "path/to/file";
  EXPECT_EQ(ref, URIUtils::GetRealPath("../path/to/file"));
  EXPECT_EQ(ref, URIUtils::GetRealPath("./path/to/file"));

  ref = "/path/to/file";
  EXPECT_EQ(ref, URIUtils::GetRealPath(ref));
  EXPECT_EQ(ref, URIUtils::GetRealPath("/path/to/./file"));
  EXPECT_EQ(ref, URIUtils::GetRealPath("/./path/to/./file"));
  EXPECT_EQ(ref, URIUtils::GetRealPath("/path/to/some/../file"));
  EXPECT_EQ(ref, URIUtils::GetRealPath("/../path/to/some/../file"));

  ref = "/path/to";
  EXPECT_EQ(ref, URIUtils::GetRealPath("/path/to/some/../file/.."));

#ifdef TARGET_WINDOWS
  ref = "\\\\path\\to\\file\\";
  EXPECT_EQ(ref, URIUtils::GetRealPath(ref));

  ref = "path\\to\\file";
  EXPECT_EQ(ref, URIUtils::GetRealPath("..\\path\\to\\file"));
  EXPECT_EQ(ref, URIUtils::GetRealPath(".\\path\\to\\file"));

  ref = "\\\\path\\to\\file";
  EXPECT_EQ(ref, URIUtils::GetRealPath(ref));
  EXPECT_EQ(ref, URIUtils::GetRealPath("\\\\path\\to\\.\\file"));
  EXPECT_EQ(ref, URIUtils::GetRealPath("\\\\.\\path/to\\.\\file"));
  EXPECT_EQ(ref, URIUtils::GetRealPath("\\\\path\\to\\some\\..\\file"));
  EXPECT_EQ(ref, URIUtils::GetRealPath("\\\\..\\path\\to\\some\\..\\file"));

  ref = "\\\\path\\to";
  EXPECT_EQ(ref, URIUtils::GetRealPath("\\\\path\\to\\some\\..\\file\\.."));
#endif

  // test rar/zip paths
  ref = "zip://%2fpath%2fto%2fzip/subpath/to/file";
  EXPECT_STRCASEEQ(ref.c_str(), URIUtils::GetRealPath(ref).c_str());

  // test rar/zip paths
  ref = "zip://%2fpath%2fto%2fzip/subpath/to/file";
  EXPECT_STRCASEEQ(ref.c_str(), URIUtils::GetRealPath("zip://%2fpath%2fto%2fzip/../subpath/to/file").c_str());
  EXPECT_STRCASEEQ(ref.c_str(), URIUtils::GetRealPath("zip://%2fpath%2fto%2fzip/./subpath/to/file").c_str());
  EXPECT_STRCASEEQ(ref.c_str(), URIUtils::GetRealPath("zip://%2fpath%2fto%2fzip/subpath/to/./file").c_str());
  EXPECT_STRCASEEQ(ref.c_str(), URIUtils::GetRealPath("zip://%2fpath%2fto%2fzip/subpath/to/some/../file").c_str());

  EXPECT_STRCASEEQ(ref.c_str(), URIUtils::GetRealPath("zip://%2fpath%2fto%2f.%2fzip/subpath/to/file").c_str());
  EXPECT_STRCASEEQ(ref.c_str(), URIUtils::GetRealPath("zip://%2fpath%2fto%2fsome%2f..%2fzip/subpath/to/file").c_str());

  // test zip/zip path
  ref ="zip://zip%3a%2f%2f%252Fpath%252Fto%252Fzip%2fpath%2fto%2fzip/subpath/to/file";
  EXPECT_STRCASEEQ(ref.c_str(), URIUtils::GetRealPath("zip://zip%3a%2f%2f%252Fpath%252Fto%252Fsome%252F..%252Fzip%2fpath%2fto%2fsome%2f..%2fzip/subpath/to/some/../file").c_str());
}

TEST_F(TestURIUtils, UpdateUrlEncoding)
{
  std::string oldUrl = "stack://zip://%2fpath%2fto%2farchive%2fsome%2darchive%2dfile%2eCD1%2ezip/video.avi , zip://%2fpath%2fto%2farchive%2fsome%2darchive%2dfile%2eCD2%2ezip/video.avi";
  std::string newUrl = "stack://zip://%2fpath%2fto%2farchive%2fsome-archive-file.CD1.zip/video.avi , zip://%2fpath%2fto%2farchive%2fsome-archive-file.CD2.zip/video.avi";

  EXPECT_TRUE(URIUtils::UpdateUrlEncoding(oldUrl));
  EXPECT_STRCASEEQ(newUrl.c_str(), oldUrl.c_str());

  oldUrl = "zip://%2fpath%2fto%2farchive%2fsome%2darchive%2efile%2ezip/video.avi";
  newUrl = "zip://%2fpath%2fto%2farchive%2fsome-archive.file.zip/video.avi";

  EXPECT_TRUE(URIUtils::UpdateUrlEncoding(oldUrl));
  EXPECT_STRCASEEQ(newUrl.c_str(), oldUrl.c_str());

  oldUrl = "/path/to/some/long%2dnamed%2efile";
  newUrl = "/path/to/some/long%2dnamed%2efile";

  EXPECT_FALSE(URIUtils::UpdateUrlEncoding(oldUrl));
  EXPECT_STRCASEEQ(newUrl.c_str(), oldUrl.c_str());

  oldUrl = "/path/to/some/long-named.file";
  newUrl = "/path/to/some/long-named.file";

  EXPECT_FALSE(URIUtils::UpdateUrlEncoding(oldUrl));
  EXPECT_STRCASEEQ(newUrl.c_str(), oldUrl.c_str());
}

struct URLEncodings
{
  std::string_view input;
  std::string_view encoded;
};

constexpr URLEncodings EncodingTestData[] = {
    // No crash
    {"", ""},

    // No Encoding needed
    {"a", "a"},
    {"z", "z"},
    {"A", "A"},
    {"Z", "Z"},
    {"0", "0"},
    {"9", "9"},

    // Encoding needed
    {" ", "%20"},
    {"\"", "%22"},
    {"#", "%23"},
    {"$", "%24"},
    {"%", "%25"},
    {"&", "%26"},
    {"'", "%27"},
    {"*", "%2a"},
    {"+", "%2b"},
    {",", "%2c"},
    {"/", "%2f"},
    {":", "%3a"},
    {";", "%3b"},
    {"<", "%3c"},
    {"=", "%3d"},
    {">", "%3e"},
    {"?", "%3f"},
    {"@", "%40"},
    {"[", "%5b"},
    {"\\", "%5c"},
    {"]", "%5d"},
    {"^", "%5e"},
    {"`", "%60"},
    {"{", "%7b"},
    {"|", "%7c"},
    {"}", "%7d"},
    {"\u03A9", "%ce%a9"}, // Greek Capital Letter Omega

    // Encoding needed (Non alpha)
    {"\x1", "%01"},
    {"\x2", "%02"},
    {"\x6", "%06"},
    {"\a", "%07"},
    {"\b", "%08"},
    {"\t", "%09"},
    {"\n", "%0a"},
    {"\v", "%0b"},
    {"\f", "%0c"},
    {"\r", "%0d"},

    // Double Encoding
    {"%20", "%2520"},
    {"%22", "%2522"},
    {"%2a", "%252a"},
    {"%2b", "%252b"},
    {"%2c", "%252c"},
    {"%2f", "%252f"},
    {"%0a", "%250a"},
    {"%0b", "%250b"},
    {"%0c", "%250c"},
};

constexpr URLEncodings RFC1738EncodingTestData[] = {
    {"-", "-"}, {"_", "_"}, {".", "."}, {"!", "!"}, {"(", "("}, {")", ")"}, {"~", "%7e"},
};

constexpr URLEncodings RFC3986EncodingTestData[] = {
    {"-", "-"}, {"_", "_"}, {".", "."}, {"!", "%21"}, {"(", "%28"}, {")", "%29"}, {"~", "~"},
};

constexpr URLEncodings InvalidEncodingTestData[] = {
    // Incomplete or Invalid encoding
    {"%", "%"},
    {"%%", "%%"},
    {"%;", "%;"},
    {"%-", "%-"},
    {"%2", "%2"},
    {"%2-", "%2-"},
    {"%2x", "%2x"},

    // Incomplete or Invalid encoding recovers to decode future characters
    {"% ", "%%20"},
    {"%% ", "%%%20"},
    {"%; ", "%;%20"},
    {"%- ", "%-%20"},
    {"%2 ", "%2%20"},
    {"%2- ", "%2-%20"},
    {"%2x ", "%2x%20"},
};

TEST_F(TestURIUtils, URLEncode)
{
  for (const auto& [toEncode, expected] : EncodingTestData)
    EXPECT_EQ(expected, URIUtils::URLEncode(toEncode));

  for (const auto& [toEncode, expected] : RFC1738EncodingTestData)
    EXPECT_EQ(expected, URIUtils::URLEncode(toEncode));

  for (const auto& [toEncode, expected] : RFC3986EncodingTestData)
    EXPECT_EQ(expected, URIUtils::URLEncode(toEncode, URIUtils::RFC3986));
}

TEST_F(TestURIUtils, URLDecode)
{
  for (const auto& [expected, encoded] : EncodingTestData)
    EXPECT_EQ(expected, URIUtils::URLDecode(encoded));

  for (const auto& [expected, encoded] : InvalidEncodingTestData)
    EXPECT_EQ(expected, URIUtils::URLDecode(encoded));

  for (const auto& [expected, encoded] : RFC1738EncodingTestData)
    EXPECT_EQ(expected, URIUtils::URLDecode(encoded));

  for (const auto& [expected, encoded] : RFC3986EncodingTestData)
    EXPECT_EQ(expected, URIUtils::URLDecode(encoded));

  // Test '+' is converted to ' '
  EXPECT_EQ(" ", URIUtils::URLDecode("+"));

  // Test decoding uppercase hex digits
  EXPECT_EQ("{", URIUtils::URLDecode("%7B"));
  EXPECT_EQ("|", URIUtils::URLDecode("%7C"));
  EXPECT_EQ("}", URIUtils::URLDecode("%7D"));
}

TEST_F(TestURIUtils, URLEncodeDecode)
{
  for (unsigned ch = 1; ch < 128; ++ch)
  {
    std::string str{char(ch)};
    EXPECT_EQ(str, CURL::Decode(CURL::Encode(str)));
  }

  for (const auto& [toEncode, encoded] : EncodingTestData)
  {
    EXPECT_EQ(toEncode, URIUtils::URLDecode(CURL::Encode(toEncode)));
    EXPECT_EQ(encoded, URIUtils::URLDecode(CURL::Encode(encoded)));
  }
}

TEST_F(TestURIUtils, ContainersEncodeHostnamePaths)
{
  CURL curl("/path/thing");

  EXPECT_EQ("zip://%2fpath%2fthing/my/archived/path",
            URIUtils::CreateArchivePath("zip", curl, "/my/archived/path").Get());

  EXPECT_EQ("apk://%2fpath%2fthing/my/archived/path",
            URIUtils::CreateArchivePath("apk", curl, "/my/archived/path").Get());

  EXPECT_EQ("udf://%2fpath%2fthing/my/archived/path",
            URIUtils::CreateArchivePath("udf", curl, "/my/archived/path").Get());

  EXPECT_EQ("iso9660://%2fpath%2fthing/my/archived/path",
            URIUtils::CreateArchivePath("iso9660", curl, "/my/archived/path").Get());

  EXPECT_EQ("rar://%2fpath%2fthing/my/archived/path",
            URIUtils::CreateArchivePath("rar", curl, "/my/archived/path").Get());
}

TEST_F(TestURIUtils, GetDiscBase)
{
  std::string refDir{"/somepath/path/"};
  EXPECT_EQ(refDir, URIUtils::GetDiscBase("/somepath/path/BDMV/index.bdmv"));
  EXPECT_EQ(refDir,
            URIUtils::GetDiscBase("bluray://%2fsomepath%2fpath%2f/BDMV/PLAYLIST/00800.mpls"));

  refDir = "/somepath/path/movie.iso";
  EXPECT_EQ(refDir, URIUtils::GetDiscBase("/somepath/path/movie.iso"));
  EXPECT_EQ(refDir,
            URIUtils::GetDiscBase(
                "bluray://udf%3a%2f%2f%252fsomepath%252fpath%252fmovie.iso%2f/BDMV/PLAYLIST/"
                "00800.mpls"));

  refDir = "D:\\Movies\\";
  EXPECT_EQ(refDir, URIUtils::GetDiscBase("D:\\Movies\\BDMV\\index.bdmv"));
  EXPECT_EQ(refDir, URIUtils::GetDiscBase("bluray://D%3a%5cMovies%5c/BDMV/PLAYLIST/00800.mpls"));

  refDir = "D:\\Movies\\movie.iso";
  EXPECT_EQ(refDir, URIUtils::GetDiscBase("D:\\Movies\\movie.iso"));
  EXPECT_EQ(refDir, URIUtils::GetDiscBase(
                        "bluray://udf%3a%2f%2fD%253a%255cMovies%255cmovie.iso%2f/BDMV/PLAYLIST/"
                        "00800.mpls"));

  refDir = "\\\\Server\\Movies\\";
  EXPECT_EQ(refDir, URIUtils::GetDiscBase("\\\\Server\\Movies\\BDMV\\index.bdmv"));
  EXPECT_EQ(refDir,
            URIUtils::GetDiscBase("bluray://%5c%5cServer%5cMovies%5c/BDMV/PLAYLIST/00800.mpls"));

  refDir = "\\\\Server\\Movies\\movie.iso";
  EXPECT_EQ(refDir, URIUtils::GetDiscBase("\\\\Server\\Movies\\movie.iso"));
  EXPECT_EQ(refDir,
            URIUtils::GetDiscBase(
                "bluray://udf%3a%2f%2f%255c%255cServer%255cMovies%255cmovie.iso%2f/BDMV/PLAYLIST/"
                "00800.mpls"));

  refDir = "smb://somepath/path/";
  EXPECT_EQ(refDir, URIUtils::GetDiscBase("smb://somepath/path/BDMV/index.bdmv"));
  EXPECT_EQ(refDir, URIUtils::GetDiscBase(
                        "bluray://smb%3a%2f%2fsomepath%2fpath%2f/BDMV/PLAYLIST/00800.mpls"));

  refDir = "smb://somepath/path/movie.iso";
  EXPECT_EQ(refDir, URIUtils::GetDiscBase("smb://somepath/path/movie.iso"));
  EXPECT_EQ(
      refDir,
      URIUtils::GetDiscBase(
          "bluray://udf%3a%2f%2fsmb%253a%252f%252fsomepath%252fpath%252fmovie.iso%2f/BDMV/PLAYLIST/"
          "00800.mpls"));
}

TEST_F(TestURIUtils, GetDiscBasePath)
{
  std::string refDir{"/somepath/path/"};
  EXPECT_EQ(refDir, URIUtils::GetDiscBasePath("/somepath/path/BDMV/index.bdmv"));
  EXPECT_EQ(refDir,
            URIUtils::GetDiscBasePath("bluray://%2fsomepath%2fpath%2f/BDMV/PLAYLIST/00800.mpls"));
  EXPECT_EQ(refDir, URIUtils::GetDiscBasePath("/somepath/path/movie.iso"));
  EXPECT_EQ(refDir,
            URIUtils::GetDiscBasePath(
                "bluray://udf%3a%2f%2f%252fsomepath%252fpath%252fmovie.iso%2f/BDMV/PLAYLIST/"
                "00800.mpls"));

  refDir = "D:\\Movies\\";
  EXPECT_EQ(refDir, URIUtils::GetDiscBasePath("D:\\Movies\\BDMV\\index.bdmv"));
  EXPECT_EQ(refDir,
            URIUtils::GetDiscBasePath("bluray://D%3a%5cMovies%5c/BDMV/PLAYLIST/00800.mpls"));
  EXPECT_EQ(refDir, URIUtils::GetDiscBasePath("D:\\Movies\\movie.iso"));
  EXPECT_EQ(refDir, URIUtils::GetDiscBasePath(
                        "bluray://udf%3a%2f%2fD%253a%255cMovies%255cmovie.iso%2f/BDMV/PLAYLIST/"
                        "00800.mpls"));

  refDir = "\\\\Server\\Movies\\";
  EXPECT_EQ(refDir, URIUtils::GetDiscBasePath("\\\\Server\\Movies\\BDMV\\index.bdmv"));
  EXPECT_EQ(refDir, URIUtils::GetDiscBasePath(
                        "bluray://%5c%5cServer%5cMovies%5c/BDMV/PLAYLIST/00800.mpls"));
  EXPECT_EQ(refDir, URIUtils::GetDiscBasePath("\\\\Server\\Movies\\movie.iso"));
  EXPECT_EQ(refDir,
            URIUtils::GetDiscBasePath(
                "bluray://udf%3a%2f%2f%255c%255cServer%255cMovies%255cmovie.iso%2f/BDMV/PLAYLIST/"
                "00800.mpls"));

  refDir = "smb://somepath/path/";
  EXPECT_EQ(refDir, URIUtils::GetDiscBasePath("smb://somepath/path/BDMV/index.bdmv"));
  EXPECT_EQ(refDir, URIUtils::GetDiscBasePath(
                        "bluray://smb%3a%2f%2fsomepath%2fpath%2f/BDMV/PLAYLIST/00800.mpls"));
  EXPECT_EQ(refDir, URIUtils::GetDiscBasePath("smb://somepath/path/movie.iso"));
  EXPECT_EQ(
      refDir,
      URIUtils::GetDiscBasePath(
          "bluray://udf%3a%2f%2fsmb%253a%252f%252fsomepath%252fpath%252fmovie.iso%2f/BDMV/PLAYLIST/"
          "00800.mpls"));
}

TEST_F(TestURIUtils, GetDiscFile)
{
  EXPECT_EQ("/somepath/path/BDMV/index.bdmv",
            URIUtils::GetDiscFile("bluray://%2fsomepath%2fpath%2f/BDMV/PLAYLIST/00800.mpls"));
  EXPECT_EQ("/somepath/path/movie.iso",
            URIUtils::GetDiscFile(
                "bluray://udf%3a%2f%2f%252fsomepath%252fpath%252fmovie.iso%2f/BDMV/PLAYLIST/"
                "00800.mpls"));

  EXPECT_EQ("D:\\Movies\\BDMV\\index.bdmv",
            URIUtils::GetDiscFile("bluray://D%3a%5cMovies%5c/BDMV/PLAYLIST/00800.mpls"));
  EXPECT_EQ(
      "D:\\Movies\\movie.iso",
      URIUtils::GetDiscFile("bluray://udf%3a%2f%2fD%253a%255cMovies%255cmovie.iso%2f/BDMV/PLAYLIST/"
                            "00800.mpls"));

  EXPECT_EQ("\\\\Server\\Movies\\BDMV\\index.bdmv",
            URIUtils::GetDiscFile("bluray://%5c%5cServer%5cMovies%5c/BDMV/PLAYLIST/00800.mpls"));
  EXPECT_EQ("\\\\Server\\Movies\\movie.iso",
            URIUtils::GetDiscFile(
                "bluray://udf%3a%2f%2f%255c%255cServer%255cMovies%255cmovie.iso%2f/BDMV/PLAYLIST/"
                "00800.mpls"));

  EXPECT_EQ(
      "smb://somepath/path/BDMV/index.bdmv",
      URIUtils::GetDiscFile("bluray://smb%3a%2f%2fsomepath%2fpath%2f/BDMV/PLAYLIST/00800.mpls"));
  EXPECT_EQ(
      "smb://somepath/path/movie.iso",
      URIUtils::GetDiscFile(
          "bluray://udf%3a%2f%2fsmb%253a%252f%252fsomepath%252fpath%252fmovie.iso%2f/BDMV/PLAYLIST/"
          "00800.mpls"));
}

TEST_F(TestURIUtils, GetDiscUnderlyingFile)
{
  CURL url{"bluray://%2fsomepath%2fpath%2f/file.ext"};
  EXPECT_EQ("/somepath/path/file.ext", URIUtils::GetDiscUnderlyingFile(url));

  url = CURL("bluray://udf%3a%2f%2f%252fsomepath%252fpath%252fmovie.iso%2f/file.ext");
  EXPECT_EQ("udf://%2fsomepath%2fpath%2fmovie.iso/file.ext", URIUtils::GetDiscUnderlyingFile(url));

  url = CURL("bluray://D%3a%5cMovies%5c/file.ext");
  EXPECT_EQ("D:\\Movies\\file.ext", URIUtils::GetDiscUnderlyingFile(url));

  url = CURL("bluray://udf%3a%2f%2fD%253a%255cMovies%255cmovie.iso%2f/file.ext");
  EXPECT_EQ("udf://D%3a%5cMovies%5cmovie.iso/file.ext", URIUtils::GetDiscUnderlyingFile(url));

  url = CURL("bluray://%5c%5cServer%5cMovies%5c/file.ext");
  EXPECT_EQ("\\\\Server\\Movies\\file.ext", URIUtils::GetDiscUnderlyingFile(url));

  url = CURL("bluray://udf%3a%2f%2f%255c%255cServer%255cMovies%255cmovie.iso%2f/file.ext");
  EXPECT_EQ("udf://%5c%5cServer%5cMovies%5cmovie.iso/file.ext",
            URIUtils::GetDiscUnderlyingFile(url));

  url = CURL("bluray://smb%3a%2f%2fsomepath%2fpath%2f/file.ext");
  EXPECT_EQ("smb://somepath/path/file.ext", URIUtils::GetDiscUnderlyingFile(url));

  url = CURL("bluray://udf%3a%2f%2fsmb%253a%252f%252fsomepath%252fpath%252fmovie.iso%2f/file.ext");
  EXPECT_EQ("udf://smb%3a%2f%2fsomepath%2fpath%2fmovie.iso/file.ext",
            URIUtils::GetDiscUnderlyingFile(url));
}

TEST_F(TestURIUtils, GetBlurayRootPath)
{
  std::string refDir{"bluray://%2fsomepath%2fpath%2f/root"};
  EXPECT_EQ(refDir, URIUtils::GetBlurayRootPath("/somepath/path/BDMV/index.bdmv"));
  EXPECT_EQ(refDir,
            URIUtils::GetBlurayRootPath("bluray://%2fsomepath%2fpath%2f/BDMV/PLAYLIST/00800.mpls"));

  refDir = "bluray://udf%3a%2f%2f%252fsomepath%252fpath%252fmovie.iso%2f/root";
  EXPECT_EQ(refDir, URIUtils::GetBlurayRootPath("/somepath/path/movie.iso"));
  EXPECT_EQ(refDir,
            URIUtils::GetBlurayRootPath(
                "bluray://udf%3a%2f%2f%252fsomepath%252fpath%252fmovie.iso%2f/BDMV/PLAYLIST/"
                "00800.mpls"));

  refDir = "bluray://D%3a%5cMovies%5c/root";
  EXPECT_EQ(refDir, URIUtils::GetBlurayRootPath("D:\\Movies\\BDMV\\index.bdmv"));
  EXPECT_EQ(refDir,
            URIUtils::GetBlurayRootPath("bluray://D%3a%5cMovies%5c/BDMV/PLAYLIST/00800.mpls"));

  refDir = "bluray://udf%3a%2f%2fD%253a%255cMovies%255cmovie.iso%2f/root";
  EXPECT_EQ(refDir, URIUtils::GetBlurayRootPath("D:\\Movies\\movie.iso"));
  EXPECT_EQ(refDir, URIUtils::GetBlurayRootPath(
                        "bluray://udf%3a%2f%2fD%253a%255cMovies%255cmovie.iso%2f/BDMV/PLAYLIST/"
                        "00800.mpls"));

  refDir = "bluray://%5c%5cServer%5cMovies%5c/root";
  EXPECT_EQ(refDir, URIUtils::GetBlurayRootPath("\\\\Server\\Movies\\BDMV\\index.bdmv"));
  EXPECT_EQ(refDir, URIUtils::GetBlurayRootPath(
                        "bluray://%5c%5cServer%5cMovies%5c/BDMV/PLAYLIST/00800.mpls"));

  refDir = "bluray://udf%3a%2f%2f%255c%255cServer%255cMovies%255cmovie.iso%2f/root";
  EXPECT_EQ(refDir, URIUtils::GetBlurayRootPath("\\\\Server\\Movies\\movie.iso"));
  EXPECT_EQ(refDir,
            URIUtils::GetBlurayRootPath(
                "bluray://udf%3a%2f%2f%255c%255cServer%255cMovies%255cmovie.iso%2f/BDMV/PLAYLIST/"
                "00800.mpls"));

  refDir = "bluray://smb%3a%2f%2fsomepath%2fpath%2f/root";
  EXPECT_EQ(refDir, URIUtils::GetBlurayRootPath("smb://somepath/path/BDMV/index.bdmv"));
  EXPECT_EQ(refDir, URIUtils::GetBlurayRootPath(
                        "bluray://smb%3a%2f%2fsomepath%2fpath%2f/BDMV/PLAYLIST/00800.mpls"));

  refDir = "bluray://udf%3a%2f%2fsmb%253a%252f%252fsomepath%252fpath%252fmovie.iso%2f/root";
  EXPECT_EQ(refDir, URIUtils::GetBlurayRootPath("smb://somepath/path/movie.iso"));
  EXPECT_EQ(
      refDir,
      URIUtils::GetBlurayRootPath(
          "bluray://udf%3a%2f%2fsmb%253a%252f%252fsomepath%252fpath%252fmovie.iso%2f/BDMV/PLAYLIST/"
          "00800.mpls"));
}

TEST_F(TestURIUtils, GetBlurayEpisodePath)
{
  std::string refDir{"bluray://%2fsomepath%2fpath%2f/root/episode/3/4"};
  EXPECT_EQ(refDir, URIUtils::GetBlurayEpisodePath("/somepath/path/BDMV/index.bdmv", 3, 4));
  EXPECT_EQ(refDir, URIUtils::GetBlurayEpisodePath(
                        "bluray://%2fsomepath%2fpath%2f/BDMV/PLAYLIST/00800.mpls", 3, 4));

  refDir = "bluray://udf%3a%2f%2f%252fsomepath%252fpath%252fmovie.iso%2f/root/episode/3/4";
  EXPECT_EQ(refDir, URIUtils::GetBlurayEpisodePath("/somepath/path/movie.iso", 3, 4));
  EXPECT_EQ(refDir,
            URIUtils::GetBlurayEpisodePath(
                "bluray://udf%3a%2f%2f%252fsomepath%252fpath%252fmovie.iso%2f/BDMV/PLAYLIST/"
                "00800.mpls",
                3, 4));

  refDir = "bluray://D%3a%5cMovies%5c/root/episode/3/4";
  EXPECT_EQ(refDir, URIUtils::GetBlurayEpisodePath("D:\\Movies\\BDMV\\index.bdmv", 3, 4));
  EXPECT_EQ(refDir, URIUtils::GetBlurayEpisodePath(
                        "bluray://D%3a%5cMovies%5c/BDMV/PLAYLIST/00800.mpls", 3, 4));

  refDir = "bluray://udf%3a%2f%2fD%253a%255cMovies%255cmovie.iso%2f/root/episode/3/4";
  EXPECT_EQ(refDir, URIUtils::GetBlurayEpisodePath("D:\\Movies\\movie.iso", 3, 4));
  EXPECT_EQ(refDir, URIUtils::GetBlurayEpisodePath(
                        "bluray://udf%3a%2f%2fD%253a%255cMovies%255cmovie.iso%2f/BDMV/PLAYLIST/"
                        "00800.mpls",
                        3, 4));

  refDir = "bluray://%5c%5cServer%5cMovies%5c/root/episode/3/4";
  EXPECT_EQ(refDir, URIUtils::GetBlurayEpisodePath("\\\\Server\\Movies\\BDMV\\index.bdmv", 3, 4));
  EXPECT_EQ(refDir, URIUtils::GetBlurayEpisodePath(
                        "bluray://%5c%5cServer%5cMovies%5c/BDMV/PLAYLIST/00800.mpls", 3, 4));

  refDir = "bluray://udf%3a%2f%2f%255c%255cServer%255cMovies%255cmovie.iso%2f/root/episode/3/4";
  EXPECT_EQ(refDir, URIUtils::GetBlurayEpisodePath("\\\\Server\\Movies\\movie.iso", 3, 4));
  EXPECT_EQ(refDir,
            URIUtils::GetBlurayEpisodePath(
                "bluray://udf%3a%2f%2f%255c%255cServer%255cMovies%255cmovie.iso%2f/BDMV/PLAYLIST/"
                "00800.mpls",
                3, 4));

  refDir = "bluray://smb%3a%2f%2fsomepath%2fpath%2f/root/episode/3/4";
  EXPECT_EQ(refDir, URIUtils::GetBlurayEpisodePath("smb://somepath/path/BDMV/index.bdmv", 3, 4));
  EXPECT_EQ(refDir, URIUtils::GetBlurayEpisodePath(
                        "bluray://smb%3a%2f%2fsomepath%2fpath%2f/BDMV/PLAYLIST/00800.mpls", 3, 4));

  refDir =
      "bluray://udf%3a%2f%2fsmb%253a%252f%252fsomepath%252fpath%252fmovie.iso%2f/root/episode/3/4";
  EXPECT_EQ(refDir, URIUtils::GetBlurayEpisodePath("smb://somepath/path/movie.iso", 3, 4));
  EXPECT_EQ(
      refDir,
      URIUtils::GetBlurayEpisodePath(
          "bluray://udf%3a%2f%2fsmb%253a%252f%252fsomepath%252fpath%252fmovie.iso%2f/BDMV/PLAYLIST/"
          "00800.mpls",
          3, 4));
}

TEST_F(TestURIUtils, GetBlurayPlaylistPath)
{
  EXPECT_EQ("bluray://%2fsomepath%2fpath%2f/BDMV/PLAYLIST/",
            URIUtils::GetBlurayPlaylistPath("/somepath/path/BDMV/index.bdmv"));
  EXPECT_EQ("bluray://%2fsomepath%2fpath%2f/BDMV/PLAYLIST/00800.mpls",
            URIUtils::GetBlurayPlaylistPath("/somepath/path/BDMV/index.bdmv", 800));
  EXPECT_EQ("bluray://udf%3a%2f%2f%252fsomepath%252fpath%252fmovie.iso%2f/BDMV/PLAYLIST/",
            URIUtils::GetBlurayPlaylistPath("/somepath/path/movie.iso"));
  EXPECT_EQ("bluray://udf%3a%2f%2f%252fsomepath%252fpath%252fmovie.iso%2f/BDMV/PLAYLIST/"
            "00800.mpls",
            URIUtils::GetBlurayPlaylistPath("/somepath/path/movie.iso", 800));

  EXPECT_EQ("bluray://D%3a%5cMovies%5c/BDMV/PLAYLIST/",
            URIUtils::GetBlurayPlaylistPath("D:\\Movies\\BDMV\\index.bdmv"));
  EXPECT_EQ("bluray://D%3a%5cMovies%5c/BDMV/PLAYLIST/00800.mpls",
            URIUtils::GetBlurayPlaylistPath("D:\\Movies\\BDMV\\index.bdmv", 800));
  EXPECT_EQ("bluray://udf%3a%2f%2fD%253a%255cMovies%255cmovie.iso%2f/BDMV/PLAYLIST/",
            URIUtils::GetBlurayPlaylistPath("D:\\Movies\\movie.iso"));
  EXPECT_EQ("bluray://udf%3a%2f%2fD%253a%255cMovies%255cmovie.iso%2f/BDMV/PLAYLIST/"
            "00800.mpls",
            URIUtils::GetBlurayPlaylistPath("D:\\Movies\\movie.iso", 800));

  EXPECT_EQ("bluray://%5c%5cServer%5cMovies%5c/BDMV/PLAYLIST/",
            URIUtils::GetBlurayPlaylistPath("\\\\Server\\Movies\\BDMV\\index.bdmv"));
  EXPECT_EQ("bluray://%5c%5cServer%5cMovies%5c/BDMV/PLAYLIST/00800.mpls",
            URIUtils::GetBlurayPlaylistPath("\\\\Server\\Movies\\BDMV\\index.bdmv", 800));
  EXPECT_EQ("bluray://udf%3a%2f%2f%255c%255cServer%255cMovies%255cmovie.iso%2f/BDMV/PLAYLIST/",
            URIUtils::GetBlurayPlaylistPath("\\\\Server\\Movies\\movie.iso"));
  EXPECT_EQ("bluray://udf%3a%2f%2f%255c%255cServer%255cMovies%255cmovie.iso%2f/BDMV/PLAYLIST/"
            "00800.mpls",
            URIUtils::GetBlurayPlaylistPath("\\\\Server\\Movies\\movie.iso", 800));

  EXPECT_EQ("bluray://smb%3a%2f%2fsomepath%2fpath%2f/BDMV/PLAYLIST/",
            URIUtils::GetBlurayPlaylistPath("smb://somepath/path/BDMV/index.bdmv"));
  EXPECT_EQ("bluray://smb%3a%2f%2fsomepath%2fpath%2f/BDMV/PLAYLIST/00800.mpls",
            URIUtils::GetBlurayPlaylistPath("smb://somepath/path/BDMV/index.bdmv", 800));
  EXPECT_EQ(
      "bluray://udf%3a%2f%2fsmb%253a%252f%252fsomepath%252fpath%252fmovie.iso%2f/BDMV/PLAYLIST/",
      URIUtils::GetBlurayPlaylistPath("smb://somepath/path/movie.iso"));
  EXPECT_EQ(
      "bluray://udf%3a%2f%2fsmb%253a%252f%252fsomepath%252fpath%252fmovie.iso%2f/BDMV/PLAYLIST/"
      "00800.mpls",
      URIUtils::GetBlurayPlaylistPath("smb://somepath/path/movie.iso", 800));
}

TEST_F(TestURIUtils, GetBlurayPlaylistFromPath)
{
  EXPECT_EQ(URIUtils::GetBlurayPlaylistFromPath(
                "bluray://%2fsomepath%2fpath%2f/BDMV/PLAYLIST/00800.mpls"),
            800);
  EXPECT_EQ(URIUtils::GetBlurayPlaylistFromPath(
                "bluray://udf%3a%2f%2f%252fsomepath%252fpath%252fmovie.iso%2f/BDMV/PLAYLIST/"
                "00800.mpls"),
            800);

  EXPECT_EQ(
      URIUtils::GetBlurayPlaylistFromPath("bluray://D%3a%5cMovies%5c/BDMV/PLAYLIST/00800.mpls"),
      800);
  EXPECT_EQ(URIUtils::GetBlurayPlaylistFromPath(
                "bluray://udf%3a%2f%2fD%253a%255cMovies%255cmovie.iso%2f/BDMV/PLAYLIST/"
                "00800.mpls"),
            800);

  EXPECT_EQ(URIUtils::GetBlurayPlaylistFromPath(
                "bluray://%5c%5cServer%5cMovies%5c/BDMV/PLAYLIST/00800.mpls"),
            800);
  EXPECT_EQ(URIUtils::GetBlurayPlaylistFromPath(
                "bluray://udf%3a%2f%2f%255c%255cServer%255cMovies%255cmovie.iso%2f/BDMV/PLAYLIST/"
                "00800.mpls"),
            800);

  EXPECT_EQ(URIUtils::GetBlurayPlaylistFromPath(
                "bluray://smb%3a%2f%2fsomepath%2fpath%2f/BDMV/PLAYLIST/00800.mpls"),
            800);
  EXPECT_EQ(URIUtils::GetBlurayPlaylistFromPath(
                "bluray://udf%3a%2f%2fsmb%253a%252f%252fsomepath%252fpath%252fmovie.iso%2f/BDMV/"
                "PLAYLIST/00800.mpls"),
            800);
}

TEST_F(TestURIUtils, RemovePartNumberFromTitle)
{
  const std::string r{URIUtils::GetTitleTrailingPartNumberRegex()};
  CRegExp regex{true, CRegExp::autoUtf8, r.c_str()};

  std::string title{"Movie (Disc 1)"};
  EXPECT_EQ(regex.RegFind(title), 5);

  title = "Movie-(Disk 100)";
  EXPECT_EQ(regex.RegFind(title), 5);

  title = "Movie";
  EXPECT_EQ(regex.RegFind(title), -1);
}

struct TestIsHostOnLANData
{
  const static bool DONT_TEST_LOCAL_NETWORK = false;

  bool expectedHostOnAnyLAN;
  bool expectedOnAnyLAN;
  bool expectedHostOnLocalLAN;
  bool expectedOnLocalLAN;
  std::string input;
  bool testLocalNetwork = true;
};

std::ostream& operator<<(std::ostream& os, const TestIsHostOnLANData& rhs)
{
  return os << rhs.input;
}

class TestLANParamTest : public testing::Test,
                         public testing::WithParamInterface<TestIsHostOnLANData>
{
};

const TestIsHostOnLANData values[] = {
    // clang-format off
    /*
    +-------------+--------------+
    |   Private   |    Local     |
    | Host | URL  | Host | URL   |
    +------+------+------+-------+   */
    // Assumption that hostnames without a period are local (smb, netbios)
    { true,  false, true,  false, "localhost" },
    { true,  true,  true,  true,  "some_unresolveable_hostname" },
    { true,  true,  true,  true,  "google" },
    { true,  true,  true,  true,  "aol" },

    { false, false, false, false, "www.some_unresolveable_hostname.com" },
    { false, false, false, false, "www.google.com" },
    { false, false, false, false, "google.com" },
    { false, false, false, false, "www.aol.com" },
    { false, false, false, false, "aol.com" },
    { false, false, false, false, "168.219.34.129" },

    { false, false, false, false, "0.0.0.0" },
    { false, false, false, false, "127.0.0.1" },
    /*
     * The following checks cannot be reliable tested on build machines
     * These tests pass for a class B network (172.16.0.0/16) but cannot be guaranteed to work under different network configurations
     */
    { true,  true,  false, false, "192.168.0.3", TestIsHostOnLANData::DONT_TEST_LOCAL_NETWORK },
    { true,  true,  true,  true,  "172.16.0.2",  TestIsHostOnLANData::DONT_TEST_LOCAL_NETWORK },
    { true,  true,  false, false, "10.0.0.9",    TestIsHostOnLANData::DONT_TEST_LOCAL_NETWORK },
    // clang-format on
};

TEST_P(TestLANParamTest, TestIsHostOnLAN)
{
  using enum LanCheckMode;

  auto& param = GetParam();
  auto hostnameURL = "http://" + param.input;

  EXPECT_EQ(param.expectedHostOnAnyLAN, URIUtils::IsHostOnLAN(param.input, ANY_PRIVATE_SUBNET));
  EXPECT_EQ(param.expectedOnAnyLAN, URIUtils::IsOnLAN(hostnameURL, ANY_PRIVATE_SUBNET));

  if (param.testLocalNetwork)
  {
    EXPECT_EQ(param.expectedHostOnLocalLAN, URIUtils::IsHostOnLAN(param.input, ONLY_LOCAL_SUBNET));
    EXPECT_EQ(param.expectedOnLocalLAN, URIUtils::IsOnLAN(hostnameURL, ONLY_LOCAL_SUBNET));
  }
}

INSTANTIATE_TEST_SUITE_P(TestURIUtils, TestLANParamTest, testing::ValuesIn(values));

TEST_F(TestURIUtils, CheckConsistencyBetweenFileNameUtilities)
{
  auto URIUtils_Split = [=](const std::string& in)
  {
    std::string _, splitCurlFileName;
    URIUtils::Split(in, _, splitCurlFileName);
    return splitCurlFileName;
  };
  auto CURL_FileName_URIUtils_Split = [=](const std::string& in)
  { return URIUtils_Split(CURL(in).GetFileName()); };

  {
    EXPECT_EQ("?", CURL("?").GetFileName());

    EXPECT_EQ("?", URIUtils::GetFileName("?"));
    EXPECT_EQ("?", CURL_FileName_URIUtils_Split("?"));
    EXPECT_EQ("?", URIUtils_Split("?"));
  }
  {
    EXPECT_EQ("", URIUtils_Split("C:"));

    EXPECT_EQ("", URIUtils::GetFileName("C:"));
    EXPECT_EQ("", CURL_FileName_URIUtils_Split("C:"));
    EXPECT_EQ("", URIUtils_Split("C:"));
  }
  {
    EXPECT_EQ("", URIUtils::GetFileName("/"));
    EXPECT_EQ("", CURL_FileName_URIUtils_Split("/"));
    EXPECT_EQ("", URIUtils_Split("/"));

    EXPECT_EQ("", URIUtils::GetFileName("\\"));
    EXPECT_EQ("", CURL_FileName_URIUtils_Split("\\"));
    EXPECT_EQ("", URIUtils_Split("\\"));

    EXPECT_EQ(".thing", URIUtils::GetFileName("/.thing"));
    EXPECT_EQ(".thing", CURL_FileName_URIUtils_Split("/.thing"));
    EXPECT_EQ(".thing", URIUtils_Split("/.thing"));

    EXPECT_EQ(".thing", URIUtils::GetFileName("/hello/there/.thing"));
    EXPECT_EQ(".thing", CURL_FileName_URIUtils_Split("/hello/there/.thing"));
    EXPECT_EQ(".thing", URIUtils_Split("/hello/there/.thing"));
  }
  {
    EXPECT_EQ("srv/share/movie.avi?option=true",
              CURL("nfs://127.0.0.1/srv/share/movie.avi?option=true").GetFileName());

    EXPECT_EQ("movie.avi?option=true",
              URIUtils::GetFileName("nfs://127.0.0.1/srv/share/movie.avi?option=true"));
    EXPECT_EQ("movie.avi?option=true",
              CURL_FileName_URIUtils_Split("nfs://127.0.0.1/srv/share/movie.avi?option=true"));
    EXPECT_EQ("movie.avi", URIUtils_Split("nfs://127.0.0.1/srv/share/movie.avi?option=true"));
  }
  {
    EXPECT_EQ("srv/share/movie.avi|option=true",
              CURL("nfs://127.0.0.1/srv/share/movie.avi|option=true").GetFileName());

    EXPECT_EQ("movie.avi|option=true",
              URIUtils::GetFileName("nfs://127.0.0.1/srv/share/movie.avi|option=true"));
    EXPECT_EQ("movie.avi|option=true",
              CURL_FileName_URIUtils_Split("nfs://127.0.0.1/srv/share/movie.avi|option=true"));
    EXPECT_EQ("movie.avi", URIUtils_Split("nfs://127.0.0.1/srv/share/movie.avi|option=true"));
  }
  {
    EXPECT_EQ("a:b", URIUtils::GetFileName("/hello/there/a:b"));
    EXPECT_EQ("a:b", CURL_FileName_URIUtils_Split("/hello/there/a:b"));
    EXPECT_EQ("a:b", URIUtils_Split("/hello/there/a:b"));
  }
  {
    const std::string path{
        "bluray://udf%3a%2f%2fsmb%253a%252f%252fsomepath%252fpath%252fmovie.iso%2f/BDMV/PLAYLIST/"
        "00800.mpls"};

    EXPECT_EQ("BDMV/PLAYLIST/00800.mpls", CURL(path).GetFileName());

    EXPECT_EQ("00800.mpls", URIUtils::GetFileName(path));
    EXPECT_EQ("00800.mpls", CURL_FileName_URIUtils_Split(path));
    EXPECT_EQ("00800.mpls", URIUtils_Split(path));
  }
  {
    const std::string path{"zip://smb%3a%2f%2fsomepath%2fpath%2fmovie.zip/movie.mkv"};

    EXPECT_EQ("movie.mkv", CURL(path).GetFileName());

    EXPECT_EQ("movie.mkv", URIUtils::GetFileName(path));
    EXPECT_EQ("movie.mkv", CURL_FileName_URIUtils_Split(path));
    EXPECT_EQ("movie.mkv", URIUtils_Split(path));
  }
  {
    const std::string path{"rar://smb%3a%2f%2fsomepath%2fpath%2fmovie.zip/BDMV/index.bdmv"};

    EXPECT_EQ("BDMV/index.bdmv", CURL(path).GetFileName());

    EXPECT_EQ("index.bdmv", URIUtils::GetFileName(path));
    EXPECT_EQ("index.bdmv", CURL_FileName_URIUtils_Split(path));
    EXPECT_EQ("index.bdmv", URIUtils_Split(path));
  }
  {
    const std::string path{"archive://smb%3a%2f%2fsomepath%2fpath%2fmovie.tar.gz/BDMV/index.bdmv"};

    EXPECT_EQ("BDMV/index.bdmv", CURL(path).GetFileName());

    EXPECT_EQ("index.bdmv", URIUtils::GetFileName(path));
    EXPECT_EQ("index.bdmv", CURL_FileName_URIUtils_Split(path));
    EXPECT_EQ("index.bdmv", URIUtils_Split(path));
  }
}

struct CURLArchiveConstructionTestData
{
  std::string input;
  std::string protocol;
  std::string filename;
  std::string hostname;
  bool translateFilename{false};
};

std::ostream& operator<<(std::ostream& os, const CURLArchiveConstructionTestData& data)
{
  return os << "CURLArchiveConstructionTestData { " << data.input << " }" << std::endl;
}

const auto ArchiveFileParsingTests = std::array{
    // Zip file tests
    CURLArchiveConstructionTestData{
        "xbmc/utils/test/resources/does_not_exist.zip/kodi-dev.png",
        "",
        "xbmc/utils/test/resources/does_not_exist.zip/kodi-dev.png",
        "",
        true,
    },
    CURLArchiveConstructionTestData{
        "xbmc/utils/test/resources/zipfile.zip/kodi-dev.png",
        "zip",
        "kodi-dev.png",
        "xbmc/utils/test/resources/zipfile.zip",
    },
    CURLArchiveConstructionTestData{
        "xbmc/utils/test/resources/archives_in_zip.zip/does_not_exist.png",
        "zip",
        "does_not_exist.png",
        "xbmc/utils/test/resources/archives_in_zip.zip",
    },
    CURLArchiveConstructionTestData{
        "xbmc/utils/test/resources/archives_in_zip.zip/zipfile.zip",
        "zip",
        "zipfile.zip",
        "xbmc/utils/test/resources/archives_in_zip.zip",
    },
    CURLArchiveConstructionTestData{
        "xbmc/utils/test/resources/archives_in_zip.zip/rarfile.rar",
        "zip",
        "rarfile.rar",
        "xbmc/utils/test/resources/archives_in_zip.zip",
    },
    CURLArchiveConstructionTestData{
        "xbmc/utils/test/resources/archives_in_zip.zip/zipfile.zip/does_not_exist.png",
        "zip",
        "zipfile.zip/does_not_exist.png",
        "xbmc/utils/test/resources/archives_in_zip.zip",
    },
    CURLArchiveConstructionTestData{
        "xbmc/utils/test/resources/archives_in_zip.zip/zipfile.zip/kodi-dev.png",
        "zip",
        "zipfile.zip/kodi-dev.png",
        "xbmc/utils/test/resources/archives_in_zip.zip",
    },
    CURLArchiveConstructionTestData{
        "xbmc/utils/test/resources/archives_in_zip.zip/directory/does_not_exist.png",
        "zip",
        "directory/does_not_exist.png",
        "xbmc/utils/test/resources/archives_in_zip.zip",
    },
    CURLArchiveConstructionTestData{
        "xbmc/utils/test/resources/archives_in_zip.zip/directory/kodi-dev.png",
        "zip",
        "directory/kodi-dev.png",
        "xbmc/utils/test/resources/archives_in_zip.zip",
    },
};

class ArchiveFileParsingTester : public testing::Test,
                                 public testing::WithParamInterface<CURLArchiveConstructionTestData>
{
};

TEST_P(ArchiveFileParsingTester, TestArchiveFileParsingNativeSlashes)
{
  const auto& param = GetParam();
  const std::string path = XBMC_REF_FILE_PATH(param.input);
  const CURL url(path);

  EXPECT_EQ(param.protocol, url.GetProtocol());
  if (param.translateFilename)
  {
    EXPECT_EQ(XBMC_REF_FILE_PATH(param.filename), url.GetFileName());
    EXPECT_EQ(param.hostname, CURL::Decode(url.GetHostName()));
  }
  else
  {
    EXPECT_EQ(param.filename, url.GetFileName());
    EXPECT_EQ(XBMC_REF_FILE_PATH(param.hostname), CURL::Decode(url.GetHostName()));
  }
}

INSTANTIATE_TEST_SUITE_P(TestURIUtils,
                         ArchiveFileParsingTester,
                         testing::ValuesIn(ArchiveFileParsingTests));

struct GetFileOrFolderNameTest
{
  std::string_view input;
  std::string expected;
};

// The function was written for limited cases and has UB for inputs like D:\\ D:foo
// clang-format off
const auto GetFileOrFolderNameTests = std::array{
    GetFileOrFolderNameTest{"foo", "foo"},
    GetFileOrFolderNameTest{"foo/bar", "bar"},
    GetFileOrFolderNameTest{"foo/bar/", "bar"},
    GetFileOrFolderNameTest{"foo/", "foo"},
    GetFileOrFolderNameTest{"/foo", "foo"},    
    GetFileOrFolderNameTest{"/foo/", "foo"},
    GetFileOrFolderNameTest{"/", ""},
    GetFileOrFolderNameTest{"", ""},
    GetFileOrFolderNameTest{"foo\\bar", "bar"},
    GetFileOrFolderNameTest{"foo\\bar\\", "bar"},
    GetFileOrFolderNameTest{"foo\\", "foo"},
    GetFileOrFolderNameTest{"\\foo\\", "foo"},
    GetFileOrFolderNameTest{"\\foo", "foo"},
};
//clang-format on

class GetFileOrFolderNameTester : public testing::WithParamInterface<GetFileOrFolderNameTest>,
                                  public testing::Test
{
};

TEST_P(GetFileOrFolderNameTester, TestValue)
{
  EXPECT_EQ(GetParam().expected, URIUtils::GetFileOrFolderName(GetParam().input));
}

INSTANTIATE_TEST_SUITE_P(TestURIUtils,
                         GetFileOrFolderNameTester,
                         testing::ValuesIn(GetFileOrFolderNameTests));
