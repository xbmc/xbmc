/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "TextureDatabase.h"
#include "URL.h"

#include <gtest/gtest.h>

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

INSTANTIATE_TEST_SUITE_P(SampleFiles, TestTextureUtils, ValuesIn(test_files));
}
