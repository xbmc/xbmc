/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FileItem.h"
#include "platform/Filesystem.h"
#include "utils/ArtUtils.h"
#include "utils/FileUtils.h"
#include "utils/URIUtils.h"

#include <array>
#include <fstream>

#include <fmt/format.h>
#include <gtest/gtest.h>

using namespace KODI;

struct TbnTest
{
  std::string path;
  std::string result;
  bool isFolder = false;
};

class GetTbnTest : public testing::WithParamInterface<TbnTest>, public testing::Test
{
};

TEST_P(GetTbnTest, TbnTest)
{
  EXPECT_EQ(ART::GetTBNFile(CFileItem(GetParam().path, GetParam().isFolder)), GetParam().result);
}

const auto tbn_tests = std::array{
    TbnTest{"/home/user/video.avi", "/home/user/video.tbn"},
    TbnTest{"/home/user/video/", "/home/user/video.tbn", true},
    TbnTest{"/home/user/bar.xbt", "/home/user/bar.tbn", true},
    TbnTest{"zip://%2fhome%2fuser%2fbar.zip/foo.avi", "/home/user/foo.tbn"},
    TbnTest{"stack:///home/user/foo-cd1.avi , /home/user/foo-cd2.avi", "/home/user/foo.tbn"}};

INSTANTIATE_TEST_SUITE_P(TestArtUtils, GetTbnTest, testing::ValuesIn(tbn_tests));

TEST(TestArtUtils, GetTbnStack)
{
  std::error_code ec;
  auto path = KODI::PLATFORM::FILESYSTEM::temp_directory_path(ec);
  ASSERT_TRUE(!ec);
  const auto file_path = URIUtils::AddFileToFolder(path, "foo-cd1.tbn");
  {
    std::ofstream of(file_path, std::ios::out);
  }
  const std::string stackPath =
      fmt::format("stack://{} , {}", URIUtils::AddFileToFolder(path, "foo-cd1.avi"),
                  URIUtils::AddFileToFolder(path, "foo-cd2.avi"));
  CFileItem item(stackPath, false);
  EXPECT_EQ(ART::GetTBNFile(item), file_path);
  CFileUtils::DeleteItem(file_path);
}
