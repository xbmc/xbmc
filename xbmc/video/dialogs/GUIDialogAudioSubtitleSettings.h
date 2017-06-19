#pragma once

/*
 *      Copyright (C) 2005-2014 Team XBMC
 *      http://xbmc.org
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

#include <string>
#include <utility>
#include <vector>

#include "settings/dialogs/GUIDialogSettingsManualBase.h"

class CVariant;

class CGUIDialogAudioSubtitleSettings : public CGUIDialogSettingsManualBase
{
public:
  CGUIDialogAudioSubtitleSettings();
  virtual ~CGUIDialogAudioSubtitleSettings();

  // specialization of CGUIWindow
  virtual void FrameMove();

  static std::string FormatDelay(float value, float interval);
  static std::string FormatDecibel(float value);
  static std::string FormatPercentAsDecibel(float value);

  static std::string BrowseForSubtitle();

protected:
  // implementations of ISettingCallback
  virtual void OnSettingChanged(std::shared_ptr<const CSetting> setting);
  virtual void OnSettingAction(std::shared_ptr<const CSetting> setting);

  // specialization of CGUIDialogSettingsBase
  virtual bool AllowResettingSettings() const { return false; }
  virtual void Save();
  virtual void SetupView();

  // specialization of CGUIDialogSettingsManualBase
  virtual void InitializeSettings();

  bool SupportsAudioFeature(int feature);
  bool SupportsSubtitleFeature(int feature);

  void AddAudioStreams(std::shared_ptr<CSettingGroup> group, const std::string &settingId);
  void AddSubtitleStreams(std::shared_ptr<CSettingGroup> group, const std::string &settingId);

  static bool IsPlayingPassthrough(const std::string &condition, const std::string &value, std::shared_ptr<const CSetting> setting, void *data);

  static void AudioStreamsOptionFiller(std::shared_ptr<const CSetting> setting, std::vector< std::pair<std::string, int> > &list, int &current, void *data);
  static void SubtitleStreamsOptionFiller(std::shared_ptr<const CSetting> setting, std::vector< std::pair<std::string, int> > &list, int &current, void *data);
  
  static std::string SettingFormatterDelay(std::shared_ptr<const CSettingControlSlider> control, const CVariant &value, const CVariant &minimum, const CVariant &step, const CVariant &maximum);
  static std::string SettingFormatterPercentAsDecibel(std::shared_ptr<const CSettingControlSlider> control, const CVariant &value, const CVariant &minimum, const CVariant &step, const CVariant &maximum);

  float m_volume;
  int m_audioStream;
  bool m_passthrough;
  int m_subtitleStream;
  bool m_subtitleVisible;
  bool m_dspEnabled;

  typedef std::vector<int> Features;
  Features m_audioCaps;
  Features m_subCaps;
};
