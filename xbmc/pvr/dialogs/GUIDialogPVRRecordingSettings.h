#pragma once
/*
 *      Copyright (C) 2017 Team Kodi
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

#include "settings/dialogs/GUIDialogSettingsManualBase.h"

#include "settings/SettingConditions.h"
#include "settings/lib/SettingDependency.h"

#include "pvr/PVRTypes.h"

#include <string>

class CSetting;

namespace PVR
{
  class CGUIDialogPVRRecordingSettings : public CGUIDialogSettingsManualBase
  {
  public:
    CGUIDialogPVRRecordingSettings();

    void SetRecording(const CPVRRecordingPtr &recording);

  protected:
    // implementation of ISettingCallback
    void OnSettingChanged(std::shared_ptr<const CSetting> setting) override;

    // specialization of CGUIDialogSettingsBase
    bool AllowResettingSettings() const override { return false; }
    void Save() override;
    void SetupView() override;

    // specialization of CGUIDialogSettingsManualBase
    void InitializeSettings() override;

  private:
    void AddCondition(std::shared_ptr<CSetting> setting, const std::string &identifier,
                      SettingConditionCheck condition,
                      SettingDependencyType depType, const std::string &settingId);

    CPVRRecordingPtr m_recording;
    std::string m_strTitle;
    int m_iPlayCount;
  };
} // namespace PVR
