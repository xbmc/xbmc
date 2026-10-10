/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "settings/DisplaySettings.h"

#include <thread>

#include <gtest/gtest.h>

TEST(TestDisplaySettings, AChangeIsConfirmedOnlyWhileTheScopeLives)
{
  EXPECT_FALSE(CDisplaySettings::IsChangeConfirmed());
  {
    CDisplaySettings::CConfirmedChange confirmed;
    EXPECT_TRUE(CDisplaySettings::IsChangeConfirmed());
    {
      CDisplaySettings::CConfirmedChange nested;
      EXPECT_TRUE(CDisplaySettings::IsChangeConfirmed());
    }
    EXPECT_TRUE(CDisplaySettings::IsChangeConfirmed());
  }
  EXPECT_FALSE(CDisplaySettings::IsChangeConfirmed());
}

TEST(TestDisplaySettings, AConfirmedChangeDoesNotReachAnotherThread)
{
  CDisplaySettings::CConfirmedChange confirmed;

  bool confirmedElsewhere = true;
  std::thread([&confirmedElsewhere] { confirmedElsewhere = CDisplaySettings::IsChangeConfirmed(); })
      .join();

  EXPECT_FALSE(confirmedElsewhere);
}
