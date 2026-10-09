/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "settings/lib/SettingLevel.h"

#include <array>

#include <gtest/gtest.h>

namespace
{

constexpr std::array<SettingLevel, 4> VIEWER_LEVELS{SettingLevel::Basic, SettingLevel::Standard,
                                                    SettingLevel::Advanced, SettingLevel::Expert};

} // unnamed namespace

TEST(TestSettingLevelName, EveryLevelAViewerCanBeAtHasAName)
{
  for (const auto level : VIEWER_LEVELS)
  {
    EXPECT_NE(nullptr, SettingLevelToString(level)) << "value " << static_cast<int>(level);
  }
}

//! \brief Internal is never a level the viewer is at, and having no name keeps it out of an answer
TEST(TestSettingLevelName, InternalHasNoName)
{
  EXPECT_EQ(nullptr, SettingLevelToString(SettingLevel::Internal));
}
