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
#include "interfaces/legacy/Control.h"
#include "interfaces/legacy/Window.h"
#include "rendering/RenderSystem.h"
#include "windowing/WinSystem.h"

#include <memory>

#include <gtest/gtest.h>

namespace
{

constexpr int WINDOW_ID = 5000;
constexpr int BUTTON_ID = 5001;
constexpr int TOGGLE_ID = 5002;

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

class CTestGUIComponent : public CGUIComponent
{
public:
  CTestGUIComponent() : CGUIComponent(false)
  {
    m_pWindowManager = std::make_unique<CGUIWindowManager>();
    CServiceBroker::RegisterGUI(this);
  }
};

class TestWindowGetControl : public ::testing::Test
{
protected:
  void SetUp() override
  {
    // CGUIButtonControl's constructor calls CGUITexture::CreateTexture, which throws unregistered
    CGUITexture::Register(
        [](float posX, float posY, float width, float height, const CTextureInfo& texture)
            -> CGUITexture* { return new CTestGUITexture(posX, posY, width, height, texture); },
        [](const CRect&, KODI::UTILS::COLOR::Color, CTexture*, const CRect*, float, bool) {});

    CServiceBroker::RegisterWinSystem(&m_winSystem);
    m_gui = std::make_unique<CTestGUIComponent>();

    const CTextureInfo texture;
    const CLabelInfo label;
    auto* window = new CGUIWindow(WINDOW_ID, "");
    window->AddControl(new CGUIButtonControl(WINDOW_ID, BUTTON_ID, 0, 0, 10, 10, texture,
                                             texture, label));
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
