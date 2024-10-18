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

enum class IPlayerAudioCaps;
class CVariant;
struct IntegerSettingOption;

class CGUIDialogAudioSettings : public CGUIDialogSettingsManualBase
{
public:
  CGUIDialogAudioSettings();
  ~CGUIDialogAudioSettings() override;

  // specialization of CGUIWindow
  void FrameMove() override;

  static std::string FormatDelay(float value, float interval);
  static std::string FormatDecibel(float value);
  static std::string FormatPercentAsDecibel(float value);

protected:
  // implementations of ISettingCallback
  void OnSettingChanged(const std::shared_ptr<const CSetting>& setting) override;
  void OnSettingAction(const std::shared_ptr<const CSetting>& setting) override;

  // specialization of CGUIDialogSettingsBase
  bool AllowResettingSettings() const override { return false; }
  bool Save() override;
  void SetupView() override;

  // specialization of CGUIDialogSettingsManualBase
  void InitializeSettings() override;

  bool SupportsAudioFeature(IPlayerAudioCaps feature);

  void AddAudioStreams(const std::shared_ptr<CSettingGroup>& group, const std::string& settingId);

  static bool IsPlayingPassthrough(const std::string& condition,
                                   const std::string& value,
                                   const std::shared_ptr<const CSetting>& setting,
                                   void* data);

  static void AudioStreamsOptionFiller(const std::shared_ptr<const CSetting>& setting,
                                       std::vector<IntegerSettingOption>& list,
                                       int& current,
                                       void* data);

  static std::string SettingFormatterDelay(
      const std::shared_ptr<const CSettingControlSlider>& control,
      const CVariant& value,
      const CVariant& minimum,
      const CVariant& step,
      const CVariant& maximum);
  static std::string SettingFormatterPercentAsDecibel(
      const std::shared_ptr<const CSettingControlSlider>& control,
      const CVariant& value,
      const CVariant& minimum,
      const CVariant& step,
      const CVariant& maximum);

  float m_volume;
  int m_audioStream;
  bool m_passthrough = false;

  std::vector<IPlayerAudioCaps> m_audioCaps;

private:
  static std::string FormatFlags(StreamFlags flags);
};
