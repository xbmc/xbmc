/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "URL.h"
#include "imagefiles/ImageFileURL.h"

#include <gtest/gtest.h>

using ::testing::ValuesIn;

namespace
{
typedef struct
{
  const char* in;
  const char* type;
  const char* out;
} TestFiles;

const TestFiles test_files[] = {
    {"/path/to/image/file.jpg", "", "image://%2fpath%2fto%2fimage%2ffile.jpg/"},
    {"/path/to/video/file.mkv", "video", "image://video@%2fpath%2fto%2fvideo%2ffile.mkv/"},
    {"/path/to/music/file.mp3", "music", "image://music@%2fpath%2fto%2fmusic%2ffile.mp3/"},
    {"image://%2fpath%2fto%2fimage%2ffile.jpg/", "", "image://%2fpath%2fto%2fimage%2ffile.jpg/"}};

class TestImageFileURL : public ::testing::TestWithParam<TestFiles>
{
};

TEST_P(TestImageFileURL, URLFromFile)
{
  const TestFiles& testFiles(GetParam());

  std::string expected = testFiles.out;
  std::string out = IMAGE_FILES::URLFromFile(testFiles.in, testFiles.type);
  EXPECT_EQ(expected, out);
}

INSTANTIATE_TEST_SUITE_P(SampleFiles, TestImageFileURL, ValuesIn(test_files));
} // namespace
