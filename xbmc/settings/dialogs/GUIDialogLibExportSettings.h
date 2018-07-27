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
