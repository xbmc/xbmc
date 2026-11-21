/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ServiceBroker.h"
#include "guilib/GUIMessage.h"
#include "guilib/GUIWindow.h"
#include "guilib/VisibleEffect.h"
#include "guilib/test/TestGUIStubs.h"
#include "messaging/ApplicationMessenger.h"

#include <memory>
#include <thread>

#include <gtest/gtest.h>

namespace
{
constexpr unsigned int CLOSE_DURATION = 100;

using KODI::GUILIB::TEST::CTestGUIComponent;
using KODI::GUILIB::TEST::CTestWinSystem;
using KODI::MESSAGING::CApplicationMessenger;

class CAnimationWindow : public CGUIWindow
{
public:
  CAnimationWindow() : CGUIWindow(5000, "")
  {
    m_needsScaling = false;
    SetAnimations(
        {CAnimation::CreateFader(100.0f, 0.0f, 0, CLOSE_DURATION, ANIM_TYPE_WINDOW_CLOSE)});
  }

  bool IsSoundEnabled() const override { return false; }
  bool IsWindowActive() const { return m_active; }
  bool AreAnimationsEnabled() const { return m_animationsEnabled; }
};

class TestGUIWindowAnimations : public testing::Test
{
protected:
  void SetUp() override
  {
    m_previousWinSystem = CServiceBroker::GetWinSystem();
    m_previousGUI = CServiceBroker::GetGUI();
    m_previousMessenger = CServiceBroker::GetAppMessenger();

    CServiceBroker::RegisterWinSystem(&m_winSystem);
    auto messenger = std::make_shared<CApplicationMessenger>();
    messenger->SetProcessThread(std::this_thread::get_id());
    CServiceBroker::RegisterAppMessenger(messenger);
    m_gui = std::make_unique<CTestGUIComponent>(CTestGUIComponent::InfoManager::WITH);
  }

  void TearDown() override
  {
    // GUI teardown unregisters itself and still needs the test graphics context.
    m_gui.reset();
    CServiceBroker::RegisterGUI(m_previousGUI);
    CServiceBroker::RegisterAppMessenger(m_previousMessenger);
    CServiceBroker::RegisterWinSystem(m_previousWinSystem);
  }

  static void ProcessFrame(CGUIWindow& window, unsigned int time)
  {
    CDirtyRegionList dirtyRegions;
    window.DoProcess(time, dirtyRegions);
    window.AfterRender();
  }

private:
  CTestWinSystem m_winSystem;
  CWinSystemBase* m_previousWinSystem{nullptr};
  CGUIComponent* m_previousGUI{nullptr};
  std::shared_ptr<CApplicationMessenger> m_previousMessenger;
  std::unique_ptr<CTestGUIComponent> m_gui;
};
} // namespace

TEST_F(TestGUIWindowAnimations, ReinitializationRestoresAnimatedCloseAfterForcedUnload)
{
  CAnimationWindow window;
  CGUIMessage init(GUI_MSG_WINDOW_INIT, 0, 0);
  ASSERT_TRUE(window.OnMessage(init));
  ProcessFrame(window, 1000);

  // Window manager teardown disables animations before closing and unloading windows.
  window.DisableAnimations();
  window.Close(true);
  window.FreeResources(true);
  ASSERT_FALSE(window.IsWindowActive());
  ASSERT_FALSE(window.AreAnimationsEnabled());
  ASSERT_TRUE(window.HasAnimation(ANIM_TYPE_WINDOW_CLOSE));

  // The empty XML filename bypasses SetDefaults() when this instance is reused.
  ASSERT_TRUE(window.OnMessage(init));
  ProcessFrame(window, 1900);
  ASSERT_TRUE(window.IsWindowActive());
  ASSERT_TRUE(window.HasProcessed());
  EXPECT_TRUE(window.AreAnimationsEnabled());

  window.Close(false);
  ProcessFrame(window, 2000);
  EXPECT_TRUE(window.IsWindowActive());
  ProcessFrame(window, 2000 + CLOSE_DURATION);
  EXPECT_FALSE(window.IsWindowActive());
}
