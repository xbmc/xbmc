/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "interfaces/generic/HeldNotifications.h"

#include <thread>

#include <gtest/gtest.h>

namespace
{
// Ids no script of the test run has
constexpr int SCRIPT{900001};

//! Raises a notification of \p script from a thread of its own, as a script's interpreter does
void Raise(long script, int& shown)
{
  std::thread raising(
      [script, &shown]
      {
        auto show = [&shown] { ++shown; };
        if (!CHeldNotifications::Hold(script, show))
          show();
      });
  raising.join();
}
} // namespace

TEST(TestHeldNotifications, AScriptStartedByAThreadWithoutAHolderIsShownAtOnce)
{
  int shown{0};
  CHeldNotifications::ScriptStarting(SCRIPT);
  Raise(SCRIPT, shown);
  EXPECT_EQ(1, shown);
}

TEST(TestHeldNotifications, AScriptsNotificationsAreShownWhenTheHolderGoes)
{
  int shown{0};
  {
    CHeldNotifications held;
    CHeldNotifications::ScriptStarting(SCRIPT);
    Raise(SCRIPT, shown);
    Raise(SCRIPT, shown);
    EXPECT_EQ(0, shown);
  }
  EXPECT_EQ(2, shown);
}

TEST(TestHeldNotifications, DroppedNotificationsAreNotShown)
{
  int shown{0};
  {
    CHeldNotifications held;
    CHeldNotifications::ScriptStarting(SCRIPT);
    Raise(SCRIPT, shown);
    held.Drop();
    Raise(SCRIPT, shown);
  }
  EXPECT_EQ(1, shown);
}

TEST(TestHeldNotifications, AScriptStartedAgainByAThreadWithoutAHolderIsNoLongerHeld)
{
  int shown{0};
  CHeldNotifications held;
  CHeldNotifications::ScriptStarting(SCRIPT);
  std::thread again([] { CHeldNotifications::ScriptStarting(SCRIPT); });
  again.join();
  Raise(SCRIPT, shown);
  EXPECT_EQ(1, shown);
}

TEST(TestHeldNotifications, AHolderInsideAnotherHandsItsNotificationsToIt)
{
  int shown{0};
  {
    CHeldNotifications outer;
    {
      CHeldNotifications inner;
      CHeldNotifications::ScriptStarting(SCRIPT);
      Raise(SCRIPT, shown);
    }
    EXPECT_EQ(0, shown);
    outer.Drop();
  }
  EXPECT_EQ(0, shown);
}

TEST(TestHeldNotifications, AScriptsNotificationsAreNotHeldOnceItsHolderHasGone)
{
  int shown{0};
  {
    CHeldNotifications held;
    CHeldNotifications::ScriptStarting(SCRIPT);
  }
  Raise(SCRIPT, shown);
  EXPECT_EQ(1, shown);
}
