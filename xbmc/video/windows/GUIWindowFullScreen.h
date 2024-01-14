/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/GUIWindow.h"

#include <chrono>

class CGUIDialog;

class CGUIWindowFullScreen : public CGUIWindow
{
public:
  CGUIWindowFullScreen();
  ~CGUIWindowFullScreen(void) override;
  bool OnMessage(CGUIMessage& message) override;
  bool OnAction(const CAction &action) override;
  void ClearBackground() override;
  void FrameMove() override;
  void Process(unsigned int currentTime, CDirtyRegionList &dirtyregion) override;
  void Render() override;
  void RenderEx() override;
  void OnWindowLoaded() override;
  bool HasVisibleControls() override;

protected:
  EVENT_RESULT OnMouseEvent(const CPoint& point, const KODI::MOUSE::CMouseEvent& event) override;

private:
  void SeekChapter(int iChapter);
  void ToggleOSD();
  void TriggerOSD();
  CGUIDialog *GetOSD();

  bool m_viewModeChanged;
  std::chrono::time_point<std::chrono::steady_clock> m_dwShowViewModeTimeout;

  bool m_bShowCurrentTime;
};
