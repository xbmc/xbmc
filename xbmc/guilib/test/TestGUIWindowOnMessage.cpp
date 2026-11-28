/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ServiceBroker.h"
#include "guilib/GUIControl.h"
#include "guilib/GUIMessage.h"
#include "guilib/GUIWindow.h"
#include "guilib/test/TestGUIStubs.h"
#include "utils/TransformMatrix.h"
#include "windowing/GraphicContext.h"

#include <gtest/gtest.h>

namespace
{

constexpr int WINDOW_ID = 5000;
constexpr int CONTROL_ID = 5001;

using KODI::GUILIB::TEST::CTestWinSystem;

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

//! \brief A control that records the GUI scale it is handed a message with
class CScaleRecordingControl : public CGUIControl
{
public:
  CScaleRecordingControl(int parentId, int id, float& scaleX)
    : CGUIControl(parentId, id, 0.0f, 0.0f, 10.0f, 10.0f),
      m_scaleX(scaleX)
  {
  }

  CScaleRecordingControl* Clone() const override { return new CScaleRecordingControl(*this); }

  bool OnMessage(CGUIMessage& message) override
  {
    m_scaleX = CServiceBroker::GetWinSystem()->GetGfxContext().GetGUIScaleX();
    return CGUIControl::OnMessage(message);
  }

private:
  float& m_scaleX;
};

} // namespace

TEST(TestGUIWindowOnMessage, RefreshTimerReplacesTheScaleLeftByAnotherWindow)
{
  CTestWinSystem winSystem;
  CScopedWinSystemRegistration registration(winSystem);

  float scaleX = 0.0f;
  CGUIWindow window(WINDOW_ID, "");
  window.AddControl(new CScaleRecordingControl(WINDOW_ID, CONTROL_ID, scaleX));

  // the scale of whatever was drawn last
  winSystem.GetGfxContext().SetTransform(TransformMatrix(), 2.0f, 2.0f);

  CGUIMessage message(GUI_MSG_REFRESH_TIMER, 0, 0);
  window.OnMessage(message);

  EXPECT_FLOAT_EQ(1.0f, scaleX);
}
