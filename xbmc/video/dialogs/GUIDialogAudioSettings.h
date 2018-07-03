/*
 *      Copyright (C) 2005-2014 Team XBMC
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include <string>
#include <utility>
#include <vector>

#include "settings/dialogs/GUIDialogSettingsManualBase.h"
#include "cores/VideoPlayer/Interface/StreamInfo.h"

class CVariant;

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
  void OnSettingChanged(std::shared_ptr<const CSetting> setting) override;
  void OnSettingAction(std::shared_ptr<const CSetting> setting) override;

  // specialization of CGUIDialogSettingsBase
  bool AllowResettingSettings() const override { return false; }
  void Save() override;
  void SetupView() override;

  // specialization of CGUIDialogSettingsManualBase
  void InitializeSettings() override;

  bool SupportsAudioFeature(int feature);

  void AddAudioStreams(std::shared_ptr<CSettingGroup> group, const std::string &settingId);

  static bool IsPlayingPassthrough(const std::string &condition, const std::string &value, std::shared_ptr<const CSetting> setting, void *data);

  static void AudioStreamsOptionFiller(std::shared_ptr<const CSetting> setting, std::vector< std::pair<std::string, int> > &list, int &current, void *data);

  static std::string SettingFormatterDelay(std::shared_ptr<const CSettingControlSlider> control, const CVariant &value, const CVariant &minimum, const CVariant &step, const CVariant &maximum);
  static std::string SettingFormatterPercentAsDecibel(std::shared_ptr<const CSettingControlSlider> control, const CVariant &value, const CVariant &minimum, const CVariant &step, const CVariant &maximum);

  float m_volume;
  int m_audioStream;
  bool m_passthrough = false;

  typedef std::vector<int> Features;
  Features m_audioCaps;
private:
  static std::string FormatFlags(StreamFlags flags);
};
