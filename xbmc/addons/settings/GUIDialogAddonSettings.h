#pragma once
/*
 *      Copyright (C) 2005-present Team Kodi
 *      http://kodi.tv
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
