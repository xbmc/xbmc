/*
 *      Copyright (C) 2012-2013 Team XBMC
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

#include "URL.h"
#include "TextureDatabase.h"

#include "gtest/gtest.h"

using ::testing::ValuesIn;

namespace
{
typedef struct
{
  const char *in;
  const char *type;
  const char *options;
  const char *out;
} TestFiles;

const TestFiles test_files[] = {{ "/path/to/image/file.jpg", "", "", "image://%2fpath%2fto%2fimage%2ffile.jpg/" },
                                { "/path/to/image/file.jpg", "", "size=thumb", "image://%2fpath%2fto%2fimage%2ffile.jpg/transform?size=thumb" },
                                { "/path/to/video/file.mkv", "video", "", "image://video@%2fpath%2fto%2fvideo%2ffile.mkv/" },
                                { "/path/to/music/file.mp3", "music", "", "image://music@%2fpath%2fto%2fmusic%2ffile.mp3/" },
                                { "image://%2fpath%2fto%2fimage%2ffile.jpg/", "", "", "image://%2fpath%2fto%2fimage%2ffile.jpg/" },
                                { "image://%2fpath%2fto%2fimage%2ffile.jpg/transform?size=thumb", "", "size=thumb", "image://%2fpath%2fto%2fimage%2ffile.jpg/transform?size=thumb" }};


class TestTextureUtils :
  public ::testing::TestWithParam<TestFiles>
{
};

TEST_P(TestTextureUtils, GetWrappedImageURL)
{
  const TestFiles &testFiles(GetParam());

  std::string expected = testFiles.out;
  std::string out = CTextureUtils::GetWrappedImageURL(testFiles.in,
                                                      testFiles.type,
                                                      testFiles.options);
  EXPECT_EQ(expected, out);
}

INSTANTIATE_TEST_CASE_P(SampleFiles, TestTextureUtils,
                        ValuesIn(test_files));
}
