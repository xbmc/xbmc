/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ServiceBroker.h"
#include "dialogs/GUIDialogBusy.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "rendering/RenderSystem.h"
#include "threads/Event.h"
#include "windowing/WinSystem.h"

#include <chrono>
#include <memory>
#include <thread>

#include <gtest/gtest.h>

using namespace std::chrono_literals;

namespace
{
// Every test here keeps the timeout at or under displaytime, so the wait is answered before the
// busy dialog is looked up. That lookup needs a GUI component and a windowing system, and the test
// environment registers neither. Raising the timeout past this would crash rather than fail.
constexpr unsigned int DISPLAY_TIME{5000};

std::chrono::milliseconds Elapsed(const std::chrono::steady_clock::time_point start)
{
  return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() -
                                                               start);
}

// Enough of a windowing system for CGUIWindowManager::GetWindow, which takes the graphics context
// lock. Duplicated from xbmc/guilib/test/TestGUIWindowOnAction.cpp rather than shared, to keep this
// change to the files it already touches.
class CTestRenderSystem : public CRenderSystemBase
{
public:
  bool InitRenderSystem() override { return true; }
  bool DestroyRenderSystem() override { return true; }
  bool ResetRenderSystem(int width, int height) override { return true; }
  bool BeginRender() override { return true; }
  bool EndRender() override { return true; }
  void PresentRender(bool rendered, bool videoLayer) override {}
  bool ClearBuffers(KODI::UTILS::COLOR::Color color) override { return true; }
  bool IsExtSupported(const char* extension) const override { return false; }
  void SetViewPort(const CRect& viewPort) override {}
  void GetViewPort(CRect& viewPort) override {}
  void SetScissors(const CRect& rect) override {}
  void ResetScissors() override {}
  void CaptureStateBlock() override {}
  void ApplyStateBlock() override {}
  void SetCameraPosition(const CPoint& camera,
                         int screenWidth,
                         int screenHeight,
                         float stereoFactor) override
  {
  }
};

class CTestWinSystem : public CWinSystemBase
{
public:
  CRenderSystemBase* GetRenderSystem() override { return &m_renderSystem; }
  bool CreateNewWindow(const std::string& name, bool fullScreen, RESOLUTION_INFO& res) override
  {
    return true;
  }
  bool ResizeWindow(int newWidth, int newHeight, int newLeft, int newTop) override { return true; }
  bool SetFullScreen(bool fullScreen, RESOLUTION_INFO& res, bool blankOtherDisplays) override
  {
    return true;
  }
  void Register(IDispResource* resource) override {}
  void Unregister(IDispResource* resource) override {}

private:
  CTestRenderSystem m_renderSystem;
};

// A GUI carrying nothing but an empty window manager, so the busy dialog is looked up and not
// found - the branch a caller reaches when the dialog is unavailable.
class CTestGUIComponent : public CGUIComponent
{
public:
  CTestGUIComponent() : CGUIComponent(false)
  {
    m_pWindowManager = std::make_unique<CGUIWindowManager>();
    CServiceBroker::RegisterGUI(this);
  }
};
} // unnamed namespace

TEST(TestGUIDialogBusy, ATimeoutEndsTheWait)
{
  CEvent event;
  const auto start{std::chrono::steady_clock::now()};

  EXPECT_FALSE(CGUIDialogBusy::WaitOnEvent(event, DISPLAY_TIME, true, 200ms));

  // the wait ends on its own deadline rather than running on to displaytime
  const auto elapsed{Elapsed(start)};
  EXPECT_GE(elapsed, 200ms);
  EXPECT_LT(elapsed, 1000ms);
}

TEST(TestGUIDialogBusy, AZeroTimeoutEndsTheWaitAtOnce)
{
  CEvent event;
  const auto start{std::chrono::steady_clock::now()};

  EXPECT_FALSE(CGUIDialogBusy::WaitOnEvent(event, DISPLAY_TIME, true, 0ms));

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
  EXPECT_TRUE(CGUIDialogBusy::WaitOnEvent(event, DISPLAY_TIME, true, 500ms));
  EXPECT_LT(Elapsed(start), 500ms);

  setter.join();
}

TEST(TestGUIDialogBusy, AnEventAlreadySetNeverWaits)
{
  CEvent event;
  event.Set();

  const auto start{std::chrono::steady_clock::now()};
  EXPECT_TRUE(CGUIDialogBusy::WaitOnEvent(event, DISPLAY_TIME, true, 200ms));
  EXPECT_LT(Elapsed(start), 200ms);
}

TEST(TestGUIDialogBusy, AZeroTimeoutStillSeesAnEventAlreadySet)
{
  CEvent event;
  event.Set();

  EXPECT_TRUE(CGUIDialogBusy::WaitOnEvent(event, DISPLAY_TIME, true, 0ms));
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

    // longer than displaytime, so the dialog is looked up: without one, master returned at once
    // and reported the event had arrived
    EXPECT_FALSE(CGUIDialogBusy::WaitOnEvent(event, 100, true, 400ms));
    EXPECT_GE(Elapsed(start), 400ms);
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

    EXPECT_TRUE(CGUIDialogBusy::WaitOnEvent(event, 100, true, 2000ms));

    setter.join();
  }
  CServiceBroker::UnregisterWinSystem();
}
