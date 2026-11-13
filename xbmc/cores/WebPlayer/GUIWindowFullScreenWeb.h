/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/GUIWindow.h"

namespace KODI
{
namespace WEB
{

/*!
 * \brief The window shown while the web player is active: the page fills
 * the screen through the render manager, the web OSD and Kodi's dialogs sit
 * on top, and the keys and pointer events the keymap does not claim go to
 * the web client.
 */
class CGUIWindowFullScreenWeb : public CGUIWindow
{
public:
  CGUIWindowFullScreenWeb();
  ~CGUIWindowFullScreenWeb() override;

  // implementation of CGUIControl via CGUIWindow
  void Process(unsigned int currentTime, CDirtyRegionList& dirtyregion) override;
  void Render() override;
  void RenderEx() override;
  bool OnAction(const CAction& action) override;
  bool OnMessage(CGUIMessage& message) override;

  // implementation of CGUIWindow
  void ClearBackground() override;
  bool HasVisibleControls() override;
  void OnWindowLoaded() override;
  void OnDeinitWindow(int nextWindowID) override;

protected:
  // implementation of CGUIWindow
  void OnInitWindow() override;

private:
  void ToggleOSD();
};

} // namespace WEB
} // namespace KODI
