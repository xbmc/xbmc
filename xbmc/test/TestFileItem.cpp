/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "FileItem.h"
#include "URL.h"
#include "settings/AdvancedSettings.h"

#include "gtest/gtest.h"

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
  // Force all settings to be reset to defaults
  g_advancedSettings.OnSettingsUnloaded();
  g_advancedSettings.Initialize();
}

class TestFileItemSpecifiedArtJpg : public AdvancedSettingsResetBase,
                                    public WithParamInterface<TestFileData>
{
};


TEST_P(TestFileItemSpecifiedArtJpg, GetLocalArt)
{
  CFileItem item;
  item.SetPath(GetParam().file);
  std::string path = CURL(item.GetLocalArt("art.jpg", GetParam().use_folder)).Get();
  std::string compare = CURL(GetParam().base).Get();
  EXPECT_EQ(compare, path);
}

const TestFileData MovieFiles[] = {{ "c:\\dir\\filename.avi", false, "c:\\dir\\filename-art.jpg" },
                                   { "c:\\dir\\filename.avi", true,  "c:\\dir\\art.jpg" },
                                   { "/dir/filename.avi", false, "/dir/filename-art.jpg" },
                                   { "/dir/filename.avi", true,  "/dir/art.jpg" },
                                   { "smb://somepath/file.avi", false, "smb://somepath/file-art.jpg" },
                                   { "smb://somepath/file.avi", true, "smb://somepath/art.jpg" },
                                   { "stack:///path/to/movie-cd1.avi , /path/to/movie-cd2.avi", false,  "/path/to/movie-art.jpg" },
                                   { "stack:///path/to/movie-cd1.avi , /path/to/movie-cd2.avi", true,  "/path/to/art.jpg" },
                                   { "stack:///path/to/movie_name/cd1/some_file1.avi , /path/to/movie_name/cd2/some_file2.avi", true,  "/path/to/movie_name/art.jpg" },
                                   { "/home/user/TV Shows/Dexter/S1/1x01.avi", false, "/home/user/TV Shows/Dexter/S1/1x01-art.jpg" },
                                   { "/home/user/TV Shows/Dexter/S1/1x01.avi", true, "/home/user/TV Shows/Dexter/S1/art.jpg" },
                                   { "rar://g%3a%5cmultimedia%5cmovies%5cSphere%2erar/Sphere.avi", false, "g:\\multimedia\\movies\\Sphere-art.jpg" },
                                   { "rar://g%3a%5cmultimedia%5cmovies%5cSphere%2erar/Sphere.avi", true, "g:\\multimedia\\movies\\art.jpg" },
                                   { "/home/user/movies/movie_name/video_ts/VIDEO_TS.IFO", false, "/home/user/movies/movie_name/art.jpg" },
                                   { "/home/user/movies/movie_name/video_ts/VIDEO_TS.IFO", true, "/home/user/movies/movie_name/art.jpg" },
                                   { "/home/user/movies/movie_name/BDMV/index.bdmv", false, "/home/user/movies/movie_name/art.jpg" },
                                   { "/home/user/movies/movie_name/BDMV/index.bdmv", true, "/home/user/movies/movie_name/art.jpg" }};

INSTANTIATE_TEST_CASE_P(MovieFiles, TestFileItemSpecifiedArtJpg, ValuesIn(MovieFiles));

class TestFileItemFallbackArt : public AdvancedSettingsResetBase,
                                public WithParamInterface<TestFileData>
{
};

TEST_P(TestFileItemFallbackArt, GetLocalArt)
{
  CFileItem item;
  item.SetPath(GetParam().file);
  std::string path = CURL(item.GetLocalArt("", GetParam().use_folder)).Get();
  std::string compare = CURL(GetParam().base).Get();
  EXPECT_EQ(compare, path);
}

const TestFileData NoArtFiles[] = {{ "c:\\dir\\filename.avi", false, "c:\\dir\\filename.tbn" },
                                   { "/dir/filename.avi", false, "/dir/filename.tbn" },
                                   { "smb://somepath/file.avi", false, "smb://somepath/file.tbn" },
                                   { "/home/user/TV Shows/Dexter/S1/1x01.avi", false, "/home/user/TV Shows/Dexter/S1/1x01.tbn" },
                                   { "rar://g%3a%5cmultimedia%5cmovies%5cSphere%2erar/Sphere.avi", false, "g:\\multimedia\\movies\\Sphere.tbn" }};

INSTANTIATE_TEST_CASE_P(NoArt, TestFileItemFallbackArt, ValuesIn(NoArtFiles));

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
                                   { "rar://g%3a%5cmultimedia%5cmovies%5cSphere%2erar/Sphere.avi", true, "g:\\multimedia\\movies\\" },
                                   { "/home/user/movies/movie_name/video_ts/VIDEO_TS.IFO", false, "/home/user/movies/movie_name/" },
                                   { "/home/user/movies/movie_name/video_ts/VIDEO_TS.IFO", true, "/home/user/movies/movie_name/" },
                                   { "/home/user/movies/movie_name/BDMV/index.bdmv", false, "/home/user/movies/movie_name/" },
                                   { "/home/user/movies/movie_name/BDMV/index.bdmv", true, "/home/user/movies/movie_name/" }};

INSTANTIATE_TEST_CASE_P(BaseNameMovies, TestFileItemBasePath, ValuesIn(BaseMovies));
