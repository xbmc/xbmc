/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <map>

#include "settings/dialogs/GUIDialogSettingsManualBase.h"

#include "pvr/addons/PVRClients.h"

class CSetting;

namespace PVR
{
  class CGUIDialogPVRClientPriorities : public CGUIDialogSettingsManualBase
  {
  public:
    CGUIDialogPVRClientPriorities();

  protected:
    // implementation of ISettingCallback
    void OnSettingChanged(std::shared_ptr<const CSetting> setting) override;

    // specialization of CGUIDialogSettingsBase
    std::string GetSettingsLabel(std::shared_ptr<ISetting> pSetting) override;
    bool AllowResettingSettings() const override { return false; }
    void Save() override;
    void SetupView() override;

    // specialization of CGUIDialogSettingsManualBase
    void InitializeSettings() override;

  private:
    CPVRClientMap m_clients;
    std::map<std::string, int> m_changedValues;
  };
} // namespace PVR
