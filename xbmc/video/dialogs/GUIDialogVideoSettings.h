/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/VideoPlayer/Interface/StreamInfo.h"
#include "settings/dialogs/GUIDialogSettingsManualBase.h"

#include <string>
#include <utility>
#include <vector>

struct IntegerSettingOption;

class CGUIDialogVideoSettings : public CGUIDialogSettingsManualBase
{
public:
  CGUIDialogVideoSettings();
  ~CGUIDialogVideoSettings() override;

protected:
  // implementations of ISettingCallback
  void OnSettingChanged(const std::shared_ptr<const CSetting>& setting) override;
  void OnSettingAction(const std::shared_ptr<const CSetting>& setting) override;

  void AddVideoStreams(const std::shared_ptr<CSettingGroup>& group, const std::string& settingId);
  static void VideoStreamsOptionFiller(const std::shared_ptr<const CSetting>& setting,
                                       std::vector<IntegerSettingOption>& list,
                                       int& current,
                                       void* data);

  static void VideoOrientationFiller(const std::shared_ptr<const CSetting>& setting,
                                     std::vector<IntegerSettingOption>& list,
                                     int& current,
                                     void* data);

  static std::string FormatFlags(StreamFlags flags);

  // specialization of CGUIDialogSettingsBase
  bool AllowResettingSettings() const override { return false; }
  bool Save() override;
  void SetupView() override;

  // specialization of CGUIDialogSettingsManualBase
  void InitializeSettings() override;

private:
  int m_videoStream;
  bool m_viewModeChanged = false;
};
