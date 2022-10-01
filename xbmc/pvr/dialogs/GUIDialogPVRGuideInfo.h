/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/GUIDialog.h"

#include <memory>

class CGUIMessage;

namespace PVR
{
class CGUIDialogPVRGuideInfo : public CGUIDialog
{
public:
  CGUIDialogPVRGuideInfo();
  ~CGUIDialogPVRGuideInfo() override;
  bool OnMessage(CGUIMessage& message) override;
  bool OnInfo(int actionID) override;
  bool HasListItems() const override { return true; }
  CFileItemPtr GetCurrentListItem(int offset = 0) override;

  void SetProgInfo(const std::shared_ptr<CFileItem>& item);

protected:
  void OnInitWindow() override;

private:
  bool OnClickButtonOK(const CGUIMessage& message);
  bool OnClickButtonRecord(const CGUIMessage& message);
  bool OnClickButtonPlay(const CGUIMessage& message);
  bool OnClickButtonFind(const CGUIMessage& message);
  bool OnClickButtonAddTimer(const CGUIMessage& message);
  bool OnClickButtonSetReminder(const CGUIMessage& message);

  std::shared_ptr<CFileItem> m_progItem;
};
} // namespace PVR
