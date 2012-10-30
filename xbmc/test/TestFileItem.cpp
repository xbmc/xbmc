/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
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

TEST(TestFileItem, GetLocalArt)
{
  typedef struct
  {
    const char *file;
    bool use_folder;
    const char *base;
  } testfiles;

  const testfiles test_files[] = {{ "c:\\dir\\filename.avi", false, "c:\\dir\\filename-art.jpg" },
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

  const testfiles test_file2[] = {{ "c:\\dir\\filename.avi", false, "c:\\dir\\filename.tbn" },
                                  { "/dir/filename.avi", false, "/dir/filename.tbn" },
                                  { "smb://somepath/file.avi", false, "smb://somepath/file.tbn" },
                                  { "/home/user/TV Shows/Dexter/S1/1x01.avi", false, "/home/user/TV Shows/Dexter/S1/1x01.tbn" },
                                  { "rar://g%3a%5cmultimedia%5cmovies%5cSphere%2erar/Sphere.avi", false, "g:\\multimedia\\movies\\Sphere.tbn" }};

  g_advancedSettings.Initialize();

  for (unsigned int i = 0; i < sizeof(test_files) / sizeof(testfiles); i++)
  {
    CFileItem item;
    item.SetPath(test_files[i].file);
    std::string path = CURL(item.GetLocalArt("art.jpg", test_files[i].use_folder)).Get();
    std::string compare = CURL(test_files[i].base).Get();
    EXPECT_EQ(path, compare);
  }

  for (unsigned int i = 0; i < sizeof(test_file2) / sizeof(testfiles); i++)
  {
    CFileItem item;
    item.SetPath(test_file2[i].file);
    std::string path = CURL(item.GetLocalArt("", test_file2[i].use_folder)).Get();
    std::string compare = CURL(test_file2[i].base).Get();
    EXPECT_EQ(path, compare);
  }
}

TEST(TestFileItem, GetBaseMoviePath)
{
  typedef struct
  {
    const char *file;
    bool use_folder;
    const char *base;
  } testfiles;

  const testfiles test_files[] = {{ "c:\\dir\\filename.avi", false, "c:\\dir\\filename.avi" },
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

  for (unsigned int i = 0; i < sizeof(test_files) / sizeof(testfiles); i++)
  {
    CFileItem item;
    item.SetPath(test_files[i].file);
    std::string path = CURL(item.GetBaseMoviePath(test_files[i].use_folder)).Get();
    std::string compare = CURL(test_files[i].base).Get();
    EXPECT_EQ(path, compare);
  }
}
