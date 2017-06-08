#pragma once
/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include "settings/dialogs/GUIDialogSettingsManagerBase.h"
#include "addons/IAddon.h"

class CGUIDialogAddonSettings : public CGUIDialogSettingsManagerBase
{
public:
  CGUIDialogAddonSettings();
  ~CGUIDialogAddonSettings() = default;

  // specializations of CGUIControl
  virtual bool OnMessage(CGUIMessage &message);

  static bool ShowForAddon(const ADDON::AddonPtr &addon, bool saveToDisk = true);

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
};
