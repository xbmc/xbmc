/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FileItem.h"
#include "music/MusicFileItemClassify.h"

#include <array>

#include <gtest/gtest.h>

using namespace KODI;

struct SimpleDefinition
{
  std::string path;
  bool folder;
  bool result;
};

class CuesheetTest : public testing::WithParamInterface<SimpleDefinition>, public testing::Test
{
};

TEST_P(CuesheetTest, IsCUESheet)
{
  EXPECT_EQ(MUSIC::IsCUESheet(CFileItem(GetParam().path, GetParam().folder)), GetParam().result);
}

const auto cuesheet_tests = std::array{
    SimpleDefinition{"/home/user/test.cue", false, true},
    SimpleDefinition{"/home/user/test.cue/", true, false},
    SimpleDefinition{"/home/user/test.foo", false, false},
};

INSTANTIATE_TEST_SUITE_P(TestMusicFileItemClassify,
                         CuesheetTest,
                         testing::ValuesIn(cuesheet_tests));
