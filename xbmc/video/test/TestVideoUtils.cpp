/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FileItem.h"
#include "URL.h"
#include "Util.h"
#include "filesystem/Directory.h"
#include "platform/Filesystem.h"
#include "utils/FileUtils.h"
#include "utils/URIUtils.h"
#include "video/VideoUtils.h"

#include <array>
#include <fstream>

#include <gtest/gtest.h>

using namespace KODI;
namespace fs = KODI::PLATFORM::FILESYSTEM;

namespace
{

using OptDef = std::pair<std::string, bool>;

class OpticalMediaPathTest : public testing::WithParamInterface<OptDef>, public testing::Test
{
};

using TrailerDef = std::pair<std::string, std::string>;

class TrailerTest : public testing::WithParamInterface<TrailerDef>, public testing::Test
{
};

} // namespace

TEST_P(OpticalMediaPathTest, GetOpticalMediaPath)
{
  std::error_code ec;
  const std::string temp_path = fs::create_temp_directory(ec);
  EXPECT_FALSE(ec);
  const std::string file_path = URIUtils::AddFileToFolder(temp_path, GetParam().first);
  EXPECT_TRUE(CUtil::CreateDirectoryEx(URIUtils::GetDirectory(file_path)));
  {
    std::ofstream of(file_path);
  }
  CFileItem item(temp_path, true);
  if (GetParam().second)
    EXPECT_EQ(VIDEO::UTILS::GetOpticalMediaPath(item), file_path);
  else
    EXPECT_EQ(VIDEO::UTILS::GetOpticalMediaPath(item), "");

  XFILE::CDirectory::RemoveRecursive(temp_path);
}

const auto mediapath_tests = std::array{
    OptDef{"VIDEO_TS.IFO", true},    OptDef{"VIDEO_TS/VIDEO_TS.IFO", true},
    OptDef{"some.file", false},
#ifdef HAVE_LIBBLURAY
    OptDef{"index.bdmv", true},      OptDef{"INDEX.BDM", true},
    OptDef{"BDMV/index.bdmv", true}, OptDef{"BDMV/INDEX.BDM", true},
#endif
};

INSTANTIATE_TEST_SUITE_P(TestVideoUtils, OpticalMediaPathTest, testing::ValuesIn(mediapath_tests));

TEST_P(TrailerTest, FindTrailer)
{
  std::string temp_path;
  if (!GetParam().second.empty())
  {
    std::error_code ec;
    temp_path = fs::create_temp_directory(ec);
    EXPECT_FALSE(ec);
    XFILE::CDirectory::Create(temp_path);
    const std::string file_path = URIUtils::AddFileToFolder(temp_path, GetParam().second);
    {
      std::ofstream of(file_path);
    }
    URIUtils::AddSlashAtEnd(temp_path);
  }

  std::string input_path = GetParam().first;
  if (!temp_path.empty())
  {
    StringUtils::Replace(input_path, "#DIRECTORY#", temp_path);
    StringUtils::Replace(input_path, "#URLENCODED_DIRECTORY#", CURL::Encode(temp_path));
  }

  CFileItem item(input_path, false);
  EXPECT_EQ(VIDEO::UTILS::FindTrailer(item),
            GetParam().second.empty() ? ""
                                      : URIUtils::AddFileToFolder(temp_path, GetParam().second));

  if (!temp_path.empty())
    XFILE::CDirectory::RemoveRecursive(temp_path);
}

const auto trailer_tests = std::array{
    TrailerDef{"https://some.where/foo", ""},
    TrailerDef{"upnp://1/2/3", ""},
    TrailerDef{"bluray://1", ""},
    TrailerDef{"pvr://foobar.pvr", ""},
    TrailerDef{"plugin://plugin.video.foo/foo?param=1", ""},
    TrailerDef{"dvd://1", ""},
    TrailerDef{"stack://#DIRECTORY#foo-cd1.avi , #DIRECTORY#foo-cd2.avi", "foo-trailer.mkv"},
    TrailerDef{"stack://#DIRECTORY#foo-cd1.avi , #DIRECTORY#foo-cd2.avi", "foo-cd1-trailer.avi"},
    TrailerDef{"stack://#DIRECTORY#foo-cd1.avi , #DIRECTORY#foo-cd2.avi", "movie-trailer.mp4"},
    TrailerDef{"zip://#URLENCODED_DIRECTORY#bar.zip/bar.avi", "bar-trailer.mov"},
    TrailerDef{"zip://#URLENCODED_DIRECTORY#bar.zip/bar.mkv", "movie-trailer.ogm"},
    TrailerDef{"#DIRECTORY#bar.mkv", "bar-trailer.mkv"},
    TrailerDef{"#DIRECTORY#bar.mkv", "movie-trailer.avi"},
};

INSTANTIATE_TEST_SUITE_P(TestVideoUtils, TrailerTest, testing::ValuesIn(trailer_tests));
