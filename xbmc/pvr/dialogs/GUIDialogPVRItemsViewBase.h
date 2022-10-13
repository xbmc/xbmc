/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/GUIDialog.h"
#include "view/GUIViewControl.h"

#include <memory>
#include <string>

class CFileItemList;

namespace PVR
{
class CGUIDialogPVRItemsViewBase : public CGUIDialog
{
public:
  CGUIDialogPVRItemsViewBase() = delete;
  CGUIDialogPVRItemsViewBase(int id, const std::string& xmlFile);
  ~CGUIDialogPVRItemsViewBase() override = default;

  void OnWindowLoaded() override;
  void OnWindowUnload() override;
  bool OnAction(const CAction& action) override;

protected:
  void Init();

  void OnInitWindow() override;
  void OnDeinitWindow(int nextWindowID) override;
  CGUIControl* GetFirstFocusableControl(int id) override;

  std::unique_ptr<CFileItemList> m_vecItems;
  CGUIViewControl m_viewControl;

private:
  void Clear();
  void ShowInfo(int itemIdx);
  bool ContextMenu(int iItemIdx);
};
} // namespace PVR
