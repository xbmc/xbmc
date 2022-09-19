/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "addons/IAddon.h"
#include "settings/dialogs/GUIDialogSettingsManagerBase.h"

class CGUIDialogAddonSettings : public CGUIDialogSettingsManagerBase
{
public:
  CGUIDialogAddonSettings();
  ~CGUIDialogAddonSettings() override = default;

  // specializations of CGUIControl
  bool OnMessage(CGUIMessage& message) override;
  bool OnAction(const CAction& action) override;

  static bool ShowForAddon(const ADDON::AddonPtr& addon, bool saveToDisk = true);
  static void SaveAndClose();

  std::string GetCurrentAddonID() const;

protected:
  // implementation of CGUIDialogSettingsBase
  void SetupView() override;
  std::string GetLocalizedString(uint32_t labelId) const override;
  std::string GetSettingsLabel(const std::shared_ptr<ISetting>& setting) override;
  int GetSettingLevel() const override;
  std::shared_ptr<CSettingSection> GetSection() override;

  // implementation of CGUIDialogSettingsManagerBase
  bool AllowResettingSettings() const override { return false; }
  bool Save() override { return true; }
  CSettingsManager* GetSettingsManager() const override;

  // implementation of ISettingCallback
  void OnSettingAction(const std::shared_ptr<const CSetting>& setting) override;

private:
  static bool ShowForSingleInstance(const ADDON::AddonPtr& addon,
                                    bool saveToDisk,
                                    ADDON::AddonInstanceId instanceId = ADDON::ADDON_SETTINGS_ID);
  static bool ShowForMultipleInstances(const ADDON::AddonPtr& addon, bool saveToDisk);

  ADDON::AddonPtr m_addon;
  ADDON::AddonInstanceId m_instanceId{ADDON::ADDON_SETTINGS_ID};
  bool m_saveToDisk = false;
};
