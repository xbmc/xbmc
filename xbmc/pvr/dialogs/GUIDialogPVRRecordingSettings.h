#pragma once
/*
 *      Copyright (C) 2017-present Team Kodi
 *      This file is part of Kodi - https://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include <string>

#include "settings/SettingConditions.h"
#include "settings/dialogs/GUIDialogSettingsManualBase.h"
#include "settings/lib/SettingDependency.h"

#include "pvr/PVRTypes.h"

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
    bool OnSettingChanging(std::shared_ptr<const CSetting> setting) override;
    void OnSettingChanged(std::shared_ptr<const CSetting> setting) override;

    // specialization of CGUIDialogSettingsBase
    bool AllowResettingSettings() const override { return false; }
    void Save() override;
    void SetupView() override;

    // specialization of CGUIDialogSettingsManualBase
    void InitializeSettings() override;

  private:
    static void LifetimesFiller(std::shared_ptr<const CSetting> setting,
                                std::vector<std::pair<std::string, int>> &list,
                                int &current, void *data);

    CPVRRecordingPtr m_recording;
    std::string m_strTitle;
    int m_iPlayCount;
    int m_iLifetime;
  };
} // namespace PVR
