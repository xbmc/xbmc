/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/GUIWindow.h"

class CGUIDialog;

namespace KODI
{
namespace RETRO
{
class CGameWindowFullScreenText;
class CGUIRenderHandle;

class CGameWindowFullScreen : public CGUIWindow
{
public:
  CGameWindowFullScreen();
  ~CGameWindowFullScreen() override;

  // implementation of CGUIControl via CGUIWindow
  void Process(unsigned int currentTime, CDirtyRegionList& dirtyregion) override;
  void Render() override;
  void RenderEx() override;
  bool OnAction(const CAction& action) override;
  bool OnMessage(CGUIMessage& message) override;

  // implementation of CGUIWindow
  void FrameMove() override;
  void ClearBackground() override;
  bool HasVisibleControls() override;
  void OnWindowLoaded() override;
  void OnDeinitWindow(int nextWindowID) override;

protected:
  // implementation of CGUIWindow
  void OnInitWindow() override;

private:
  void TriggerOSD();
  CGUIDialog* GetOSD();

  void RegisterWindow();
  void UnregisterWindow();

  // GUI parameters
  std::unique_ptr<CGameWindowFullScreenText> m_fullscreenText;

  // Rendering parameters
  std::shared_ptr<CGUIRenderHandle> m_renderHandle;
};
} // namespace RETRO
} // namespace KODI
