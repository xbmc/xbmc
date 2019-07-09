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

class CVariant;
struct IntegerSettingOption;

class CGUIDialogSubtitleSettings : public CGUIDialogSettingsManualBase
{
public:
  CGUIDialogSubtitleSettings();
  ~CGUIDialogSubtitleSettings() override;
  bool OnMessage(CGUIMessage& message) override;

  // specialization of CGUIWindow
  void FrameMove() override;

  static std::string BrowseForSubtitle();

protected:
  // implementations of ISettingCallback
  void OnSettingChanged(std::shared_ptr<const CSetting> setting) override;
  void OnSettingAction(std::shared_ptr<const CSetting> setting) override;

  // specialization of CGUIDialogSettingsBase
  bool AllowResettingSettings() const override { return false; }
  void Save() override;
  void SetupView() override;

  // specialization of CGUIDialogSettingsManualBase
  void InitializeSettings() override;

private:
  bool SupportsSubtitleFeature(int feature);

  void AddSubtitleStreams(std::shared_ptr<CSettingGroup> group, const std::string &settingId);

  int m_subtitleStream;
  bool m_subtitleVisible;
  std::shared_ptr<CSettingInt> m_subtitleStreamSetting;

  std::vector<int> m_subtitleCapabilities;
  static std::string FormatFlags(StreamFlags flags);

  static void SubtitleStreamsOptionFiller(std::shared_ptr<const CSetting> setting, std::vector<IntegerSettingOption> &list, int &current, void *data);

  static std::string SettingFormatterDelay(std::shared_ptr<const CSettingControlSlider> control, const CVariant &value, const CVariant &minimum, const CVariant &step, const CVariant &maximum);
};
