/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "pvr/windows/GUIWindowPVRBase.h"

#include <memory>

class CFileItem;

namespace PVR
{
class CGUIWindowPVRTimersBase : public CGUIWindowPVRBase
{
public:
  CGUIWindowPVRTimersBase(bool bRadio, int id, const std::string& xmlFile);
  ~CGUIWindowPVRTimersBase() override;

  bool OnMessage(CGUIMessage& message) override;
  bool OnAction(const CAction& action) override;
  void OnPrepareFileItems(CFileItemList& items) override;
  bool Update(const std::string& strDirectory, bool updateFilterPath = true) override;
  void UpdateButtons() override;

private:
  bool ActionShowTimer(const CFileItem& item);

  std::shared_ptr<CFileItem> m_currentFileItem;
};
} // namespace PVR
