/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ServiceBroker.h"
#include "dialogs/GUIDialogBusy.h"
#include "guilib/test/TestGUIStubs.h"
#include "threads/Event.h"

#include <chrono>
#include <memory>
#include <thread>

#include <gtest/gtest.h>

using namespace std::chrono_literals;
using WaitResult = CGUIDialogBusy::WaitResult;

namespace
{
// Tests without the stub GUI keep the timeout at or under displaytime, so the wait is answered
// before the busy dialog is looked up. That lookup needs a GUI component and a windowing system,
// which the test environment does not register.
constexpr unsigned int DISPLAY_TIME{5000};

// The deadline is held in whole milliseconds, so a wait can end up to one short of it.
constexpr std::chrono::milliseconds DEADLINE_RESOLUTION{1};

std::chrono::milliseconds Elapsed(const std::chrono::steady_clock::time_point start)
{
  return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() -
                                                               start);
}

// The stub GUI holds no busy dialog, so it is looked up and not found - the branch a caller
// reaches when the dialog is unavailable.
using KODI::GUILIB::TEST::CTestGUIComponent;
using KODI::GUILIB::TEST::CTestWinSystem;
} // unnamed namespace

TEST(TestGUIDialogBusy, ATimeoutEndsTheWait)
{
  CEvent event;
  const auto start{std::chrono::steady_clock::now()};

  EXPECT_EQ(WaitResult::TIMED_OUT,
            CGUIDialogBusy::WaitOnEventFor(event, 200ms, DISPLAY_TIME, true));

  // the wait ends on its own deadline rather than running on to displaytime
  const auto elapsed{Elapsed(start)};
  EXPECT_GE(elapsed, 200ms - DEADLINE_RESOLUTION);
  EXPECT_LT(elapsed, 1000ms);
}

TEST(TestGUIDialogBusy, AZeroTimeoutEndsTheWaitAtOnce)
{
  CEvent event;
  const auto start{std::chrono::steady_clock::now()};

  EXPECT_EQ(WaitResult::TIMED_OUT, CGUIDialogBusy::WaitOnEventFor(event, 0ms, DISPLAY_TIME, true));

  EXPECT_LT(Elapsed(start), 200ms);
}

TEST(TestGUIDialogBusy, AnEventArrivingBeforeTheTimeoutIsNotATimeout)
{
  CEvent event;
  std::thread setter(
      [&event]()
      {
        std::this_thread::sleep_for(30ms);
        event.Set();
      });

  const auto start{std::chrono::steady_clock::now()};

  // the deadline is armed and must not be what ends this wait
  EXPECT_EQ(WaitResult::COMPLETED,
            CGUIDialogBusy::WaitOnEventFor(event, 500ms, DISPLAY_TIME, true));
  EXPECT_LT(Elapsed(start), 500ms);

  setter.join();
}

TEST(TestGUIDialogBusy, AnEventAlreadySetNeverWaits)
{
  CEvent event;
  event.Set();

  const auto start{std::chrono::steady_clock::now()};
  EXPECT_EQ(WaitResult::COMPLETED,
            CGUIDialogBusy::WaitOnEventFor(event, 200ms, DISPLAY_TIME, true));
  EXPECT_LT(Elapsed(start), 200ms);
}

TEST(TestGUIDialogBusy, AZeroTimeoutStillSeesAnEventAlreadySet)
{
  CEvent event;
  event.Set();

  EXPECT_EQ(WaitResult::COMPLETED, CGUIDialogBusy::WaitOnEventFor(event, 0ms, DISPLAY_TIME, true));
}

TEST(TestGUIDialogBusy, AnUnsetTimeoutIsNotEndedByADeadline)
{
  CEvent event;
  std::thread setter(
      [&event]()
      {
        std::this_thread::sleep_for(50ms);
        event.Set();
      });

  // the three argument form every other caller uses
  EXPECT_TRUE(CGUIDialogBusy::WaitOnEvent(event, DISPLAY_TIME, true));

  setter.join();
}

TEST(TestGUIDialogBusy, ATimeoutEndsTheWaitWithNoBusyDialogToShow)
{
  CTestWinSystem winSystem;
  CServiceBroker::RegisterWinSystem(&winSystem);
  {
    CTestGUIComponent gui;

    CEvent event;
    const auto start{std::chrono::steady_clock::now()};

    // longer than displaytime, so the dialog is looked up and found missing
    EXPECT_EQ(WaitResult::TIMED_OUT, CGUIDialogBusy::WaitOnEventFor(event, 400ms, 100, true));
    EXPECT_GE(Elapsed(start), 400ms - DEADLINE_RESOLUTION);
  }
  CServiceBroker::UnregisterWinSystem();
}

TEST(TestGUIDialogBusy, AnEventArrivesWithNoBusyDialogToShow)
{
  CTestWinSystem winSystem;
  CServiceBroker::RegisterWinSystem(&winSystem);
  {
    CTestGUIComponent gui;

    CEvent event;
    std::thread setter(
        [&event]()
        {
          std::this_thread::sleep_for(150ms);
          event.Set();
        });

    EXPECT_EQ(WaitResult::COMPLETED, CGUIDialogBusy::WaitOnEventFor(event, 2000ms, 100, true));

    setter.join();
  }
  CServiceBroker::UnregisterWinSystem();
}
