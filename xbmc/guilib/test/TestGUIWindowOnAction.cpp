/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ServiceBroker.h"
#include "guilib/GUIControl.h"
#include "guilib/GUIControlGroup.h"
#include "guilib/GUIWindow.h"
#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"
#include "rendering/RenderSystem.h"
#include "windowing/WinSystem.h"

#include <functional>
#include <memory>
#include <utility>

#include <gtest/gtest.h>

namespace
{

constexpr int WINDOW_ID = 5000;
constexpr int GROUP_ID = 5001;
constexpr int CHILD_ID = 5002;
constexpr int DEFAULT_CONTROL_ID = 5003;

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

class CScopedWinSystemRegistration
{
public:
  explicit CScopedWinSystemRegistration(CWinSystemBase& winSystem)
    : m_previousWinSystem(CServiceBroker::GetWinSystem())
  {
    CServiceBroker::RegisterWinSystem(&winSystem);
  }

  ~CScopedWinSystemRegistration()
  {
    if (m_previousWinSystem)
      CServiceBroker::RegisterWinSystem(m_previousWinSystem);
    else
      CServiceBroker::UnregisterWinSystem();
  }

private:
  CWinSystemBase* m_previousWinSystem;
};

/*! \brief A focusable control that runs a callback when it is handed an action.
 *
 * Never handles the action, so CGUIWindow::OnAction goes on to walk up the chain.
 */
class CCallbackControl : public CGUIControl
{
public:
  CCallbackControl(int parentId, int id, std::function<void()> onAction)
    : CGUIControl(parentId, id, 0.0f, 0.0f, 10.0f, 10.0f),
      m_onAction(std::move(onAction))
  {
  }

  CCallbackControl* Clone() const override { return new CCallbackControl(*this); }

  bool CanFocus() const override { return true; }

  bool OnAction(const CAction& action) override
  {
    if (m_onAction)
      m_onAction();
    return false;
  }

private:
  std::function<void()> m_onAction;
};

/*! \brief A group that records having been offered an action.
 *
 * The flag is held through a shared_ptr so the test can still read it after the group
 * has been destroyed.
 */
class CRecordingGroup : public CGUIControlGroup
{
public:
  CRecordingGroup(int parentId, int id, std::shared_ptr<bool> offered)
    : CGUIControlGroup(parentId, id, 0.0f, 0.0f, 10.0f, 10.0f),
      m_offered(std::move(offered))
  {
  }

  CRecordingGroup* Clone() const override { return new CRecordingGroup(*this); }

  bool OnAction(const CAction& action) override
  {
    *m_offered = true;
    return false;
  }

private:
  std::shared_ptr<bool> m_offered;
};

/*! \brief A window holding a group holding a focused control.
 *
 * The nesting matters: a control parented directly by the window ends the walk
 * immediately, so there has to be a group in between for there to be a parent to
 * walk up to.
 */
class CTestWindow : public CGUIWindow
{
public:
  CTestWindow(std::shared_ptr<bool> parentOffered, std::function<void()> onChildAction)
    : CGUIWindow(WINDOW_ID, "")
  {
    auto* group = new CRecordingGroup(WINDOW_ID, GROUP_ID, std::move(parentOffered));
    auto* child = new CCallbackControl(GROUP_ID, CHILD_ID, std::move(onChildAction));
    child->SetFocus(true);
    group->AddControl(child);
    AddControl(group);
  }
};

class CUnfocusableControl : public CGUIControl
{
public:
  CUnfocusableControl(int parentId, int id) : CGUIControl(parentId, id, 0.0f, 0.0f, 10.0f, 10.0f) {}

  CUnfocusableControl* Clone() const override { return new CUnfocusableControl(*this); }

  bool CanFocus() const override { return false; }
};

class CFocusRecoveryWindow : public CGUIWindow
{
public:
  CFocusRecoveryWindow() : CGUIWindow(WINDOW_ID, "")
  {
    SetDefaultControl(DEFAULT_CONTROL_ID, true);

    auto* control = new CUnfocusableControl(WINDOW_ID, CHILD_ID);
    control->SetFocus(true);
    AddControl(control);
  }

  bool OnMessage(CGUIMessage&) override
  {
    ++m_focusRequests;
    return true;
  }

  int GetFocusRequests() const { return m_focusRequests; }
  void MarkInitializedForTest() { m_active = true; }

private:
  int m_focusRequests{0};
};

} // namespace

TEST(TestGUIWindowOnAction, PropagatesToTheParentWhenTheControlsSurvive)
{
  auto parentOffered = std::make_shared<bool>(false);
  CTestWindow window(parentOffered, nullptr);

  const CAction action(ACTION_MOVE_LEFT);
  EXPECT_FALSE(window.OnAction(action));

  // nothing destroyed the controls, so the unhandled action reaches the parent group
  EXPECT_TRUE(*parentOffered);
}

TEST(TestGUIWindowOnAction, StopsPropagatingWhenTheControlsAreDestroyed)
{
  auto parentOffered = std::make_shared<bool>(false);

  // A skin reload destroys every control in the window while the action is being
  // handled. ClearAll() reproduces that: the focused control and its parent are freed
  // before CGUIWindow::OnAction regains control.
  CTestWindow* window = nullptr;
  auto tearDown = [&window]() { window->ClearAll(); };

  CTestWindow instance(parentOffered, tearDown);
  window = &instance;

  const CAction action(ACTION_MOVE_LEFT);
  EXPECT_TRUE(instance.OnAction(action));

  // the parent was freed by the teardown, so it must not have been offered the action
  EXPECT_FALSE(*parentOffered);
}

TEST(TestGUIWindowOnAction, DoesNotRecoverFocusBeforeInitialization)
{
  CTestWinSystem winSystem;
  CScopedWinSystemRegistration registration(winSystem);
  CFocusRecoveryWindow window;
  CDirtyRegionList dirtyRegions;

  window.DoProcess(0, dirtyRegions);

  EXPECT_EQ(0, window.GetFocusRequests());
}

TEST(TestGUIWindowOnAction, RecoversFocusAfterInitialization)
{
  CTestWinSystem winSystem;
  CScopedWinSystemRegistration registration(winSystem);
  CFocusRecoveryWindow window;
  CDirtyRegionList dirtyRegions;

  window.MarkInitializedForTest();
  window.DoProcess(0, dirtyRegions);

  EXPECT_EQ(1, window.GetFocusRequests());
}
