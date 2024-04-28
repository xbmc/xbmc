/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FileItem.h"
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

using OptDef = std::pair<std::string, bool>;

class OpticalMediaPathTest : public testing::WithParamInterface<OptDef>, public testing::Test
{
};

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
