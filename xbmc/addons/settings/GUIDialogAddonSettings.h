/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "settings/dialogs/GUIDialogSettingsManagerBase.h"
#include "addons/IAddon.h"

class CGUIDialogAddonSettings : public CGUIDialogSettingsManagerBase
{
public:
  CGUIDialogAddonSettings();
  ~CGUIDialogAddonSettings() override = default;

  // specializations of CGUIControl
  bool OnMessage(CGUIMessage &message) override;

  static bool ShowForAddon(const ADDON::AddonPtr &addon, bool saveToDisk = true);
  static void SaveAndClose();

  std::string GetCurrentAddonID() const;

protected:
  // implementation of CGUIDialogSettingsBase
  void SetupView() override;
  std::string GetLocalizedString(uint32_t labelId) const override;
  std::string GetSettingsLabel(std::shared_ptr<ISetting> setting) override;
  int GetSettingLevel() const override;
  std::shared_ptr<CSettingSection> GetSection() override;

  // implementation of CGUIDialogSettingsManagerBase
  bool AllowResettingSettings() const override { return false; }
  void Save() override { }
  CSettingsManager* GetSettingsManager() const override;

private:
  ADDON::AddonPtr m_addon;
  bool m_saveToDisk = false;
};
