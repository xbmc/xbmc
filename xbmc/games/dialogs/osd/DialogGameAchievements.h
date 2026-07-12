/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "FileItemList.h"
#include "guilib/GUIDialog.h"
#include "view/GUIViewControl.h"

namespace KODI
{
namespace GAME
{

class CDialogGameAchievements : public CGUIDialog
{
public:
  CDialogGameAchievements();
  ~CDialogGameAchievements() override = default;

  // Implementation of CGUIControl via CGUIDialog
  bool OnMessage(CGUIMessage& message) override;

protected:
  // Implementation of CGUIWindow via CGUIDialog
  void OnWindowLoaded() override;
  void OnWindowUnload() override;
  void OnInitWindow() override;

private:
  // Utility function
  void PopulateList();

  // Dialog parameters
  CFileItemList m_items;
  CGUIViewControl m_viewControl;
};

} // namespace GAME
} // namespace KODI
