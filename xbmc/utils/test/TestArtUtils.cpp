/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FileItem.h"
#include "URL.h"
#include "filesystem/Directory.h"
#include "platform/Filesystem.h"
#include "utils/ArtUtils.h"
#include "utils/FileUtils.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "video/VideoInfoTag.h"

#include <array>
#include <fstream>
#include <random>

#include <fmt/format.h>
#include <gtest/gtest.h>

using namespace KODI;

namespace
{

std::string unique_path(const std::string& input)
{
  std::random_device rd;
  std::mt19937 gen(rd());
  auto randchar = [&gen]()
  {
    const std::string set = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    std::uniform_int_distribution<> select(0, set.size() - 1);
    return set[select(gen)];
  };

  std::string ret;
  ret.reserve(input.size());
  std::transform(input.begin(), input.end(), std::back_inserter(ret),
                 [&randchar](const char c) { return (c == '%') ? randchar() : c; });

  return ret;
}

struct FanartTest
{
  std::string path;
  std::string result;
};

class GetLocalFanartTest : public testing::WithParamInterface<FanartTest>, public testing::Test
{
};

const auto local_fanart_tests = std::array{
    FanartTest{"stack://#DIRECTORY#foo-cd1.avi , #DIRECTORY#foo-cd2.avi", "foo-fanart.jpg"},
    FanartTest{"stack://#DIRECTORY#foo-cd1.avi , #DIRECTORY#foo-cd2.avi", "foo-cd1-fanart.jpg"},
    FanartTest{"zip://#URLENCODED_DIRECTORY#bar.zip/foo.avi", "foo-fanart.jpg"},
    FanartTest{"ftp://some.where/foo.avi", ""},
    FanartTest{"https://some.where/foo.avi", ""},
    FanartTest{"upnp://some.where/123", ""},
    FanartTest{"bluray://1", ""},
    FanartTest{"/home/user/1.pvr", ""},
    FanartTest{"plugin://random.video/1", ""},
    FanartTest{"addons://plugins/video/1", ""},
    FanartTest{"dvd://1", ""},
    FanartTest{"", ""},
    FanartTest{"foo.avi", ""},
    FanartTest{"foo.avi", "foo-fanart.jpg"},
    FanartTest{"videodb://movies/1", ""},
    FanartTest{"videodb://movies/1", "foo-fanart.jpg"},
};

struct TbnTest
{
  std::string path;
  std::string result;
  bool isFolder = false;
};

class GetTbnTest : public testing::WithParamInterface<TbnTest>, public testing::Test
{
};

} // namespace

TEST_P(GetLocalFanartTest, GetLocalFanart)
{
  std::string path, file_path, uniq;
  if (GetParam().result.empty())
    file_path = GetParam().path;
  else
  {
    std::error_code ec;
    auto tmpdir = KODI::PLATFORM::FILESYSTEM::temp_directory_path(ec);
    ASSERT_TRUE(!ec);
    uniq = unique_path("FanartTest%%%%%");
    path = URIUtils::AddFileToFolder(tmpdir, uniq);
    URIUtils::AddSlashAtEnd(path);
    XFILE::CDirectory::Create(path);
    std::ofstream of(URIUtils::AddFileToFolder(path, GetParam().result), std::ios::out);
    if (GetParam().path.find("#DIRECTORY#") != std::string::npos)
    {
      file_path = GetParam().path;
      StringUtils::Replace(file_path, "#DIRECTORY#", path);
    }
    else if (GetParam().path.find("#URLENCODED_DIRECTORY#") != std::string::npos)
    {
      file_path = GetParam().path;
      StringUtils::Replace(file_path, "#URLENCODED_DIRECTORY#", CURL::Encode(path));
    }
    else if (GetParam().path.starts_with("videodb://"))
    {
      file_path = GetParam().path;
    }
    else
      file_path =
          URIUtils::AddFileToFolder(URIUtils::AddFileToFolder(tmpdir, uniq), GetParam().path);
  }

  CFileItem item(file_path, false);
  if (GetParam().path.starts_with("videodb://") && !GetParam().result.empty())
  {
    item.GetVideoInfoTag()->m_strFileNameAndPath = URIUtils::AddFileToFolder(path, "foo.avi");
  }
  const std::string res = ART::GetLocalFanart(item);

  EXPECT_EQ(URIUtils::GetFileName(res), GetParam().result);

  if (!GetParam().result.empty())
    XFILE::CDirectory::RemoveRecursive(path);
}

INSTANTIATE_TEST_SUITE_P(TestArtUtils, GetLocalFanartTest, testing::ValuesIn(local_fanart_tests));

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
