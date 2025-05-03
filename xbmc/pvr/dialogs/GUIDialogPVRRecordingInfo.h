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

class CFileItem;
class CGUIMessage;

namespace PVR
{
class CGUIDialogPVRRecordingInfo : public CGUIDialog
{
public:
  CGUIDialogPVRRecordingInfo();
  ~CGUIDialogPVRRecordingInfo() override = default;
  bool OnMessage(CGUIMessage& message) override;
  bool OnInfo(int actionID) override;
  bool HasListItems() const override { return true; }
  CFileItemPtr GetCurrentListItem(int offset = 0) override;

  void SetRecording(const CFileItem& item);

private:
  bool OnClickButtonFind(const CGUIMessage& message);
  bool OnClickButtonOK(const CGUIMessage& message);
  bool OnClickButtonPlay(const CGUIMessage& message);

  std::shared_ptr<CFileItem> m_recordItem;
};
} // namespace PVR
