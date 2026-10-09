/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ServiceBroker.h"
#include "guilib/GUIButtonControl.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUILabel.h"
#include "guilib/GUITexture.h"
#include "guilib/GUIToggleButtonControl.h"
#include "guilib/GUIWindow.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/test/TestGUIStubs.h"
#include "interfaces/legacy/Control.h"
#include "interfaces/legacy/Window.h"

#include <memory>

#include <gtest/gtest.h>

namespace
{

constexpr int WINDOW_ID = 5000;
constexpr int BUTTON_ID = 5001;
constexpr int TOGGLE_ID = 5002;

using KODI::GUILIB::TEST::CTestGUIComponent;
using KODI::GUILIB::TEST::CTestWinSystem;

class CTestGUITexture : public CGUITexture
{
public:
  CTestGUITexture(float posX, float posY, float width, float height, const CTextureInfo& texture)
    : CGUITexture(posX, posY, width, height, texture)
  {
  }
  CGUITexture* Clone() const override { return new CTestGUITexture(*this); }

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

class TestWindowGetControl : public ::testing::Test
{
protected:
  void SetUp() override
  {
    // CGUIButtonControl's constructor calls CGUITexture::CreateTexture, which throws unregistered
    CGUITexture::Register(
        [](float posX, float posY, float width, float height,
           const CTextureInfo& texture) -> CGUITexture*
        { return new CTestGUITexture(posX, posY, width, height, texture); },
        [](const CRect&, KODI::UTILS::COLOR::Color, CTexture*, const CRect*, float, bool) {});

    CServiceBroker::RegisterWinSystem(&m_winSystem);
    // The window code under test dereferences the info manager through CServiceBroker::GetGUI()
    m_gui = std::make_unique<CTestGUIComponent>(CTestGUIComponent::InfoManager::WITH);

    const CTextureInfo texture;
    const CLabelInfo label;
    auto* window = new CGUIWindow(WINDOW_ID, "");
    window->AddControl(
        new CGUIButtonControl(WINDOW_ID, BUTTON_ID, 0, 0, 10, 10, texture, texture, label));
    window->AddControl(new CGUIToggleButtonControl(WINDOW_ID, TOGGLE_ID, 0, 0, 10, 10, texture,
                                                   texture, texture, texture, label));
    CServiceBroker::GetGUI()->GetWindowManager().Add(window);
  }

  void TearDown() override
  {
    // CGUIWindowManager::DeInitialize locks the gfx context, so the window system outlives the GUI
    m_gui.reset();
    CServiceBroker::UnregisterWinSystem();
  }

private:
  CTestWinSystem m_winSystem;
  std::unique_ptr<CTestGUIComponent> m_gui;
};

} // namespace

TEST_F(TestWindowGetControl, ButtonWrapsAsControlButton)
{
  XBMCAddon::xbmcgui::Window window(WINDOW_ID);

  XBMCAddon::xbmcgui::Control* control = nullptr;
  ASSERT_NO_THROW(control = window.getControl(BUTTON_ID));
  ASSERT_NE(control, nullptr);
  EXPECT_NE(dynamic_cast<XBMCAddon::xbmcgui::ControlButton*>(control), nullptr);
}

TEST_F(TestWindowGetControl, ToggleButtonWrapsAsControlButton)
{
  XBMCAddon::xbmcgui::Window window(WINDOW_ID);

  XBMCAddon::xbmcgui::Control* control = nullptr;
  ASSERT_NO_THROW(control = window.getControl(TOGGLE_ID));
  ASSERT_NE(control, nullptr);
  EXPECT_NE(dynamic_cast<XBMCAddon::xbmcgui::ControlButton*>(control), nullptr);
}
