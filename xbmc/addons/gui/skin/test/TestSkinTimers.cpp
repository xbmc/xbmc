/*
 *  Copyright (C) 2023- Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */
#include "GUIInfoManager.h"
#include "addons/gui/skin/SkinTimerManager.h"
#include "test/TestUtils.h"

#include <gtest/gtest.h>

class TestSkinTimers : public ::testing::Test
{
protected:
  TestSkinTimers() = default;
};

TEST_F(TestSkinTimers, TestSkinTimerParsing)
{
  auto timersFile = XBMC_REF_FILE_PATH("xbmc/addons/gui/skin/test/testdata/Timers.xml");
  CGUIInfoManager infoMgr;
  CSkinTimerManager skinTimerManager{infoMgr};
  skinTimerManager.LoadTimers(timersFile);
  // ensure only 6 timers are loaded (there are two invalid timers in the test file)
  EXPECT_EQ(skinTimerManager.GetTimerCount(), 6);
  // timer1
  EXPECT_EQ(skinTimerManager.TimerExists("timer1"), true);
  auto timer1 = skinTimerManager.GrabTimer("timer1");
  EXPECT_NE(timer1, nullptr);
  EXPECT_EQ(skinTimerManager.GetTimerCount(), 5);
  EXPECT_EQ(timer1->GetName(), "timer1");
  EXPECT_EQ(timer1->ResetsOnStart(), true);
  EXPECT_EQ(timer1->GetStartCondition()->GetExpression(), "true");
  EXPECT_EQ(timer1->GetStopCondition()->GetExpression(), "true");
  EXPECT_EQ(timer1->GetResetCondition()->GetExpression(), "true");
  EXPECT_EQ(timer1->GetStartActions().GetActionCount(), 2);
  EXPECT_EQ(timer1->GetStopActions().GetActionCount(), 2);
  EXPECT_EQ(timer1->GetStartActions().HasConditionalActions(), false);
  EXPECT_EQ(timer1->GetStopActions().HasConditionalActions(), false);
  EXPECT_EQ(timer1->GetStartActions().HasAnyActions(), true);
  EXPECT_EQ(timer1->GetStopActions().HasAnyActions(), true);
  // timer2
  auto timer2 = skinTimerManager.GrabTimer("timer2");
  EXPECT_NE(timer2, nullptr);
  EXPECT_EQ(skinTimerManager.GetTimerCount(), 4);
  EXPECT_EQ(timer2->ResetsOnStart(), false);
  EXPECT_EQ(timer2->GetStartActions().GetActionCount(), 1);
  EXPECT_EQ(timer2->GetStopActions().GetActionCount(), 1);
  EXPECT_EQ(timer2->GetStartActions().HasConditionalActions(), false);
  EXPECT_EQ(timer2->GetStopActions().HasConditionalActions(), false);
  EXPECT_EQ(timer2->GetStartActions().HasAnyActions(), true);
  EXPECT_EQ(timer2->GetStopActions().HasAnyActions(), true);
  // timer3
  auto timer3 = skinTimerManager.GrabTimer("timer3");
  EXPECT_NE(timer3, nullptr);
  EXPECT_EQ(skinTimerManager.GetTimerCount(), 3);
  EXPECT_EQ(timer3->GetName(), "timer3");
  EXPECT_EQ(timer3->GetStartActions().HasConditionalActions(), false);
  EXPECT_EQ(timer3->GetStopActions().HasConditionalActions(), false);
  EXPECT_EQ(timer3->GetStartActions().HasAnyActions(), false);
  EXPECT_EQ(timer3->GetStopActions().HasAnyActions(), false);
  EXPECT_EQ(timer3->GetStartCondition(), nullptr);
  EXPECT_EQ(timer3->GetStopCondition(), nullptr);
  EXPECT_EQ(timer3->GetResetCondition(), nullptr);
  // timer4
  auto timer4 = skinTimerManager.GrabTimer("timer4");
  EXPECT_NE(timer4, nullptr);
  EXPECT_EQ(skinTimerManager.GetTimerCount(), 2);
  EXPECT_EQ(timer4->GetName(), "timer4");
  EXPECT_EQ(timer4->GetStartCondition(), nullptr);
  EXPECT_EQ(timer4->GetStopCondition(), nullptr);
  EXPECT_EQ(timer4->GetResetCondition(), nullptr);
  EXPECT_EQ(timer4->GetStartActions().HasAnyActions(), true);
  EXPECT_EQ(timer4->GetStopActions().HasAnyActions(), true);
  EXPECT_EQ(timer4->GetStartActions().HasConditionalActions(), false);
  EXPECT_EQ(timer4->GetStopActions().HasConditionalActions(), false);
  // timer5
  auto timer5 = skinTimerManager.GrabTimer("timer5");
  EXPECT_NE(timer5, nullptr);
  EXPECT_EQ(skinTimerManager.GetTimerCount(), 1);
  EXPECT_EQ(timer5->GetName(), "timer5");
  EXPECT_EQ(timer5->GetStartActions().HasAnyActions(), true);
  EXPECT_EQ(timer5->GetStopActions().HasAnyActions(), true);
  EXPECT_EQ(timer5->GetStartActions().HasConditionalActions(), true);
  EXPECT_EQ(timer5->GetStopActions().HasConditionalActions(), true);
  // timer6
  auto timer6 = skinTimerManager.GrabTimer("timer6");
  EXPECT_NE(timer6, nullptr);
  EXPECT_EQ(skinTimerManager.GetTimerCount(), 0);
  EXPECT_EQ(timer6->GetName(), "timer6");
  EXPECT_EQ(timer6->GetStartCondition(), nullptr);
  EXPECT_EQ(timer6->GetStopCondition(), nullptr);
  EXPECT_EQ(timer6->GetResetCondition(), nullptr);
  EXPECT_EQ(timer6->GetStartActions().HasAnyActions(), false);
  EXPECT_EQ(timer6->GetStopActions().HasAnyActions(), false);
  EXPECT_EQ(timer6->GetStartActions().HasConditionalActions(), false);
  EXPECT_EQ(timer6->GetStopActions().HasConditionalActions(), false);
}
