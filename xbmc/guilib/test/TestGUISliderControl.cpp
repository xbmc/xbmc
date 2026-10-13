/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ServiceBroker.h"
#include "guilib/GUISliderControl.h"
#include "guilib/IMsgTargetCallback.h"
#include "guilib/guiinfo/GUIInfoLabels.h"
#include "guilib/guiinfo/GUIInfoProvider.h"
#include "guilib/guiinfo/GUIInfoProviders.h"
#include "guilib/test/TestGUIStubs.h"
#include "input/actions/ActionIDs.h"
#include "input/mouse/MouseEvent.h"
#include "input/mouse/MouseStat.h"

#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>

namespace
{
class CTestInfoProvider : public KODI::GUILIB::GUIINFO::CGUIInfoProvider
{
public:
  bool InitCurrentItem(CFileItem* item) override { return false; }
  bool GetLabel(std::string& value,
                const CFileItem* item,
                int contextWindow,
                const KODI::GUILIB::GUIINFO::CGUIInfo& info,
                std::string* fallback) const override
  {
    return false;
  }
  bool GetBool(bool& value,
               const CGUIListItem* item,
               int contextWindow,
               const KODI::GUILIB::GUIINFO::CGUIInfo& info) const override
  {
    return false;
  }
  bool GetInt(int& value,
              const CGUIListItem* item,
              int contextWindow,
              const KODI::GUILIB::GUIINFO::CGUIInfo& info) const override
  {
    value = 73;
    return true;
  }
};

class CTestTexture : public CGUITexture
{
public:
  CTestTexture(float x, float y, float width, float height, const CTextureInfo& texture)
    : CGUITexture(x, y, width, height, texture)
  {
  }
  CGUITexture* Clone() const override { return new CTestTexture(*this); }

protected:
  void Begin(KODI::UTILS::COLOR::Color color) override {}
  void Draw(float* x,
            float* y,
            float* z,
            const CRect& texture,
            const CRect& diffuse,
            int orientation) override
  {
  }
  void End() override {}
};

class CTestSlider : public CGUISliderControl
{
public:
  explicit CTestSlider(int controlID = 5001)
    : CGUISliderControl(5000,
                        controlID,
                        0,
                        0,
                        100,
                        10,
                        {},
                        {},
                        {},
                        {},
                        {},
                        SLIDER_CONTROL_TYPE_PERCENTAGE,
                        HORIZONTAL)
  {
  }
  using CGUISliderControl::OnMouseEvent;
  bool IsDragging() const { return m_dragging; }
};

class TestGUISliderControl : public ::testing::Test, public IMsgTargetCallback
{
protected:
  void SetUp() override
  {
    m_previousWinSystem = CServiceBroker::GetWinSystem();
    CServiceBroker::RegisterWinSystem(&m_winSystem);
    m_previousGUI = CServiceBroker::GetGUI();
    m_gui = std::make_unique<KODI::GUILIB::TEST::CTestGUIComponent>(
        KODI::GUILIB::TEST::CTestGUIComponent::InfoManager::WITH);
    m_gui->GetInfoManager().GetInfoProviders().RegisterProvider(&m_infoProvider, false);
    m_gui->GetWindowManager().AddMsgTarget(this);
    CGUITexture::Register(
        [](float x, float y, float width, float height, const CTextureInfo& texture) -> CGUITexture*
        { return new CTestTexture(x, y, width, height, texture); },
        [](const CRect&, KODI::UTILS::COLOR::Color, CTexture*, const CRect*, float, bool) {});
  }

  void TearDown() override
  {
    m_gui->GetInfoManager().GetInfoProviders().UnregisterProvider(&m_infoProvider);
    m_gui->GetWindowManager().RemoveMsgTarget(this);
    m_gui.reset();
    if (m_previousGUI)
      CServiceBroker::RegisterGUI(m_previousGUI);
    else
      CServiceBroker::UnregisterGUI();
    if (m_previousWinSystem)
      CServiceBroker::RegisterWinSystem(m_previousWinSystem);
    else
      CServiceBroker::UnregisterWinSystem();
  }

  bool OnMessage(CGUIMessage& message) override
  {
    if (message.GetMessage() == GUI_MSG_EXECUTE)
      m_actions.push_back(message.GetStringParam());
    else if (message.GetMessage() == GUI_MSG_EXCLUSIVE_MOUSE)
      m_capture = message.GetSenderId();
    return true;
  }

  std::vector<std::string> m_actions;
  int m_capture{0};

private:
  KODI::GUILIB::TEST::CTestWinSystem m_winSystem;
  CWinSystemBase* m_previousWinSystem{nullptr};
  CTestInfoProvider m_infoProvider;
  CGUIComponent* m_previousGUI{nullptr};
  std::unique_ptr<KODI::GUILIB::TEST::CTestGUIComponent> m_gui;
};
} // namespace

TEST_F(TestGUISliderControl, InfoFeedbackDoesNotReplaceTouchPreview)
{
  CTestSlider slider;
  slider.SetAction("seek");
  slider.SetInfo(PLAYER_PROGRESS);
  slider.OnMouseEvent({10, 5}, {ACTION_GESTURE_BEGIN});
  slider.OnMouseEvent({25, 5}, {ACTION_GESTURE_PAN});
  CDirtyRegionList dirtyRegions;
  slider.Process(0, dirtyRegions);
  EXPECT_FLOAT_EQ(slider.GetPercentage(), 25);
  slider.OnMouseEvent({1200, -600}, {ACTION_GESTURE_END});
  ASSERT_EQ(m_actions.size(), 1);
  EXPECT_NE(m_actions.front().find("25.000000"), std::string::npos);
  slider.Process(1, dirtyRegions);
  EXPECT_FLOAT_EQ(slider.GetPercentage(), 73);
}

TEST_F(TestGUISliderControl, SeekCommitsOnceOnGestureEnd)
{
  CTestSlider slider;
  slider.SetAction("seek");
  slider.OnMouseEvent({10, 5}, {ACTION_GESTURE_BEGIN});
  EXPECT_EQ(m_capture, slider.GetID());
  for (int x = 10; x <= 75; ++x)
  {
    slider.OnMouseEvent({static_cast<float>(x), 5}, {ACTION_GESTURE_NOTIFY});
    slider.OnMouseEvent({static_cast<float>(x), 5}, {ACTION_GESTURE_PAN});
  }
  EXPECT_TRUE(slider.IsDragging());
  EXPECT_FLOAT_EQ(slider.GetPercentage(), 75);
  EXPECT_TRUE(m_actions.empty());

  // Gesture-end coordinates carry velocity, not the final pointer position.
  slider.OnMouseEvent({1200, -600}, {ACTION_GESTURE_END});
  ASSERT_EQ(m_actions.size(), 1);
  EXPECT_NE(m_actions.front().find("75.000000"), std::string::npos);
  EXPECT_FALSE(slider.IsDragging());
  EXPECT_EQ(m_capture, 0);
  slider.OnMouseEvent({1200, -600}, {ACTION_GESTURE_END});
  EXPECT_EQ(m_actions.size(), 1);
}

TEST_F(TestGUISliderControl, PvrSeekUsesTheSameReleasePolicy)
{
  CTestSlider slider;
  slider.SetAction("pvr.seek");
  slider.OnMouseEvent({10, 5}, {ACTION_GESTURE_BEGIN});
  slider.OnMouseEvent({25, 5}, {ACTION_GESTURE_PAN});
  EXPECT_TRUE(m_actions.empty());
  slider.OnMouseEvent({0, 0}, {ACTION_GESTURE_END});
  ASSERT_EQ(m_actions.size(), 1);
  EXPECT_EQ(m_actions.front(), "PVR.SeekPercentage(25.000000)");
}

TEST_F(TestGUISliderControl, UnnamedSeekSlidersDoNotWaitForGestureEnd)
{
  for (const auto* action : {"seek", "pvr.seek"})
  {
    SCOPED_TRACE(action);
    m_actions.clear();
    CTestSlider slider(0);
    slider.SetAction(action);
    slider.SetInfo(PLAYER_PROGRESS);
    slider.OnMouseEvent({10, 5}, {ACTION_GESTURE_BEGIN});
    EXPECT_EQ(m_capture, 0);
    EXPECT_FALSE(slider.IsDragging());
    slider.OnMouseEvent({25, 5}, {ACTION_GESTURE_PAN});
    EXPECT_EQ(m_actions.size(), 1);
    slider.OnMouseEvent({75, 5}, {ACTION_GESTURE_PAN});
    ASSERT_EQ(m_actions.size(), 2);
    EXPECT_NE(m_actions.back().find("75.000000"), std::string::npos);

    // Without capture, gesture end can miss the slider entirely.
    CDirtyRegionList dirtyRegions;
    slider.Process(0, dirtyRegions);
    EXPECT_FALSE(slider.IsDragging());
    EXPECT_FLOAT_EQ(slider.GetPercentage(), 73);
  }
}

TEST_F(TestGUISliderControl, AbortReleasesCaptureWithoutSeeking)
{
  CTestSlider slider;
  slider.SetAction("pvr.seek");
  slider.OnMouseEvent({10, 5}, {ACTION_GESTURE_BEGIN});
  slider.OnMouseEvent({75, 5}, {ACTION_GESTURE_PAN});
  slider.OnMouseEvent({0, 0}, {ACTION_GESTURE_ABORT});
  slider.OnMouseEvent({0, 0}, {ACTION_GESTURE_END});
  EXPECT_FALSE(slider.IsDragging());
  EXPECT_EQ(m_capture, 0);
  EXPECT_TRUE(m_actions.empty());
}

TEST_F(TestGUISliderControl, SimultaneousSwipePreservesDrag)
{
  CTestSlider slider;
  slider.SetAction("seek");
  slider.OnMouseEvent({10, 5}, {ACTION_GESTURE_BEGIN});
  slider.OnMouseEvent({75, 5}, {ACTION_GESTURE_PAN});
  slider.OnMouseEvent({75, 5}, {ACTION_GESTURE_SWIPE_RIGHT});
  EXPECT_TRUE(slider.IsDragging());
  slider.OnMouseEvent({1200, -600}, {ACTION_GESTURE_END});
  ASSERT_EQ(m_actions.size(), 1);
  EXPECT_FLOAT_EQ(slider.GetPercentage(), 75);
}

TEST_F(TestGUISliderControl, VolumeRemainsContinuousDuringPan)
{
  CTestSlider slider;
  slider.SetAction("volume");
  slider.OnMouseEvent({10, 5}, {ACTION_GESTURE_BEGIN});
  slider.OnMouseEvent({25, 5}, {ACTION_GESTURE_PAN});
  slider.OnMouseEvent({75, 5}, {ACTION_GESTURE_PAN});
  EXPECT_EQ(m_actions.size(), 2);
  slider.OnMouseEvent({0, 0}, {ACTION_GESTURE_END});
  EXPECT_EQ(m_actions.size(), 2);
}

TEST_F(TestGUISliderControl, MouseDragStillSeeksOnRelease)
{
  CTestSlider slider;
  slider.SetAction("seek");
  slider.OnMouseEvent({25, 5}, {ACTION_MOUSE_DRAG, static_cast<int>(HoldAction::DRAG)});
  EXPECT_TRUE(m_actions.empty());
  slider.OnMouseEvent({75, 5}, {ACTION_MOUSE_DRAG_END, static_cast<int>(HoldAction::DRAG_END)});
  EXPECT_EQ(m_actions.size(), 1);
  EXPECT_FLOAT_EQ(slider.GetPercentage(), 75);
}

TEST_F(TestGUISliderControl, TapStillSeeksImmediately)
{
  CTestSlider slider;
  slider.SetAction("seek");
  slider.OnMouseEvent({25, 5}, {ACTION_MOUSE_LEFT_CLICK});
  EXPECT_EQ(m_actions.size(), 1);
  EXPECT_FLOAT_EQ(slider.GetPercentage(), 25);
}
