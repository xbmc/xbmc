/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

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
    static bool CanEditRecording(const CFileItem& item);

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
    int m_iPlayCount = 0;
    int m_iLifetime = 0;
  };
} // namespace PVR
