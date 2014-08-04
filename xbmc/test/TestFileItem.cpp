/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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

TEST(TestFileItemList, PositionAfterAdd)
{
  CFileItemList items;

  items.Add(CFileItemPtr(new CFileItem()));
  EXPECT_EQ(0, items.Get(0)->m_iPosition);

  items.Add(CFileItemPtr(new CFileItem()));
  EXPECT_EQ(0, items.Get(0)->m_iPosition);
  EXPECT_EQ(1, items.Get(1)->m_iPosition);
}

TEST(TestFileItemList, PositionAfterAddFront)
{
  CFileItemList items;

  // simple AddFront() to index 0
  items.AddFront(CFileItemPtr(new CFileItem()), 0);
  EXPECT_EQ(0, items.Get(0)->m_iPosition);
  items.Clear();

  // AddFront() to index 5 (even though there are not 5 items in the list)
  items.AddFront(CFileItemPtr(new CFileItem()), 5);
  EXPECT_EQ(0, items.Get(0)->m_iPosition);
  items.Clear();

  // AddFront() to index -5 (even though there are not 5 items in the list)
  items.AddFront(CFileItemPtr(new CFileItem()), -5);
  EXPECT_EQ(0, items.Get(0)->m_iPosition);
  items.Clear();

  // AddFront() to index 0 moving the existing item to index 1
  items.Add(CFileItemPtr(new CFileItem("first")));
  EXPECT_EQ(0, items.Get(0)->m_iPosition);
  items.AddFront(CFileItemPtr(new CFileItem("second")), 0);
  EXPECT_EQ(0, items.Get(0)->m_iPosition);
  EXPECT_STREQ("second", items.Get(0)->GetLabel().c_str());
  EXPECT_EQ(1, items.Get(1)->m_iPosition);
  EXPECT_STREQ("first", items.Get(1)->GetLabel().c_str());

  // AddFront() to index 1 leaving the existing item at index 0 and moving the item from index 1 to index 2
  items.AddFront(CFileItemPtr(new CFileItem("third")), 1);
  EXPECT_EQ(0, items.Get(0)->m_iPosition);
  EXPECT_STREQ("second", items.Get(0)->GetLabel().c_str());
  EXPECT_EQ(1, items.Get(1)->m_iPosition);
  EXPECT_STREQ("third", items.Get(1)->GetLabel().c_str());
  EXPECT_EQ(2, items.Get(2)->m_iPosition);
  EXPECT_STREQ("first", items.Get(2)->GetLabel().c_str());

  // AddFront() to index -1 leaving the existing items at index 0 and 1 and moving the item from index 2 to index 3
  items.AddFront(CFileItemPtr(new CFileItem("fourth")), -1);
  EXPECT_EQ(0, items.Get(0)->m_iPosition);
  EXPECT_STREQ("second", items.Get(0)->GetLabel().c_str());
  EXPECT_EQ(1, items.Get(1)->m_iPosition);
  EXPECT_STREQ("third", items.Get(1)->GetLabel().c_str());
  EXPECT_EQ(2, items.Get(2)->m_iPosition);
  EXPECT_STREQ("fourth", items.Get(2)->GetLabel().c_str());
  EXPECT_EQ(3, items.Get(3)->m_iPosition);
  EXPECT_STREQ("first", items.Get(3)->GetLabel().c_str());

  // AddFront() to index -4 moving the item from indexes 1-3 to indexes 2-4
  items.AddFront(CFileItemPtr(new CFileItem("fifth")), -4);
  EXPECT_EQ(0, items.Get(0)->m_iPosition);
  EXPECT_STREQ("fifth", items.Get(0)->GetLabel().c_str());
  EXPECT_EQ(1, items.Get(1)->m_iPosition);
  EXPECT_STREQ("second", items.Get(1)->GetLabel().c_str());
  EXPECT_EQ(2, items.Get(2)->m_iPosition);
  EXPECT_STREQ("third", items.Get(2)->GetLabel().c_str());
  EXPECT_EQ(3, items.Get(3)->m_iPosition);
  EXPECT_STREQ("fourth", items.Get(3)->GetLabel().c_str());
  EXPECT_EQ(4, items.Get(4)->m_iPosition);
  EXPECT_STREQ("first", items.Get(4)->GetLabel().c_str());
}

TEST(TestFileItemList, PositionAfterRemove)
{
  CFileItemList items;
  CFileItemPtr third(new CFileItem("third"));

  items.Add(CFileItemPtr(new CFileItem("first")));
  items.Add(CFileItemPtr(new CFileItem("second")));
  items.Add(third);
  items.Add(CFileItemPtr(new CFileItem("fourth")));
  items.Add(CFileItemPtr(new CFileItem("fifth")));
  EXPECT_EQ(0, items.Get(0)->m_iPosition);
  EXPECT_STREQ("first", items.Get(0)->GetLabel().c_str());
  EXPECT_EQ(1, items.Get(1)->m_iPosition);
  EXPECT_STREQ("second", items.Get(1)->GetLabel().c_str());
  EXPECT_EQ(2, items.Get(2)->m_iPosition);
  EXPECT_STREQ("third", items.Get(2)->GetLabel().c_str());
  EXPECT_EQ(3, items.Get(3)->m_iPosition);
  EXPECT_STREQ("fourth", items.Get(3)->GetLabel().c_str());
  EXPECT_EQ(4, items.Get(4)->m_iPosition);
  EXPECT_STREQ("fifth", items.Get(4)->GetLabel().c_str());

  // remove invalid indexes
  items.Remove(-1);
  items.Remove(items.Size());
  EXPECT_EQ(0, items.Get(0)->m_iPosition);
  EXPECT_STREQ("first", items.Get(0)->GetLabel().c_str());
  EXPECT_EQ(1, items.Get(1)->m_iPosition);
  EXPECT_STREQ("second", items.Get(1)->GetLabel().c_str());
  EXPECT_EQ(2, items.Get(2)->m_iPosition);
  EXPECT_STREQ("third", items.Get(2)->GetLabel().c_str());
  EXPECT_EQ(3, items.Get(3)->m_iPosition);
  EXPECT_STREQ("fourth", items.Get(3)->GetLabel().c_str());
  EXPECT_EQ(4, items.Get(4)->m_iPosition);
  EXPECT_STREQ("fifth", items.Get(4)->GetLabel().c_str());

  // remove the last item
  items.Remove(items.Size() - 1);
  EXPECT_EQ(0, items.Get(0)->m_iPosition);
  EXPECT_STREQ("first", items.Get(0)->GetLabel().c_str());
  EXPECT_EQ(1, items.Get(1)->m_iPosition);
  EXPECT_STREQ("second", items.Get(1)->GetLabel().c_str());
  EXPECT_EQ(2, items.Get(2)->m_iPosition);
  EXPECT_STREQ("third", items.Get(2)->GetLabel().c_str());
  EXPECT_EQ(3, items.Get(3)->m_iPosition);
  EXPECT_STREQ("fourth", items.Get(3)->GetLabel().c_str());

  // remove the third item
  items.Remove(third.get());
  EXPECT_EQ(0, items.Get(0)->m_iPosition);
  EXPECT_STREQ("first", items.Get(0)->GetLabel().c_str());
  EXPECT_EQ(1, items.Get(1)->m_iPosition);
  EXPECT_STREQ("second", items.Get(1)->GetLabel().c_str());
  EXPECT_EQ(2, items.Get(2)->m_iPosition);
  EXPECT_STREQ("fourth", items.Get(2)->GetLabel().c_str());

  // remove the first item
  items.Remove(0);
  EXPECT_EQ(0, items.Get(0)->m_iPosition);
  EXPECT_STREQ("second", items.Get(0)->GetLabel().c_str());
  EXPECT_EQ(1, items.Get(1)->m_iPosition);
  EXPECT_STREQ("fourth", items.Get(1)->GetLabel().c_str());

  // remove the first item
  items.Remove(0);
  EXPECT_EQ(0, items.Get(0)->m_iPosition);
  EXPECT_STREQ("fourth", items.Get(0)->GetLabel().c_str());

  // remove the first item
  items.Remove(0);
  EXPECT_TRUE(items.IsEmpty());
}
