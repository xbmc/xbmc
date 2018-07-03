/*
 *      Copyright (C) 2017 Team KODI
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
 *  along with KODI; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include <map>

#include "settings/dialogs/GUIDialogSettingsManualBase.h"
#include "settings/LibExportSettings.h"

class CGUIDialogLibExportSettings : public CGUIDialogSettingsManualBase
{
public:
  CGUIDialogLibExportSettings();

  // specialization of CGUIWindow
  bool HasListItems() const override { return true; };
  static bool Show(CLibExportSettings& settings);

protected:
  // specializations of CGUIWindow
  void OnInitWindow() override;

  // implementations of ISettingCallback
  void OnSettingChanged(std::shared_ptr<const CSetting> setting) override;
  void OnSettingAction(std::shared_ptr<const CSetting> setting) override;

  // specialization of CGUIDialogSettingsBase
  bool OnMessage(CGUIMessage& message) override;
  bool AllowResettingSettings() const override { return false; }
  void Save() override;
  void SetupView() override;

  // specialization of CGUIDialogSettingsManualBase
  void InitializeSettings() override;

  void OnOK();
  void UpdateButtons();

private:
  void SetLabel2(const std::string &settingid, const std::string &label);
  void ToggleState(const std::string &settingid, bool enabled);

  using CGUIDialogSettingsManualBase::SetFocus;
  void SetFocus(const std::string &settingid);
  static int GetExportItemsFromSetting(SettingConstPtr setting);

  CLibExportSettings m_settings;
  bool m_destinationChecked = false;
};
