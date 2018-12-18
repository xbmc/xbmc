/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "settings/dialogs/GUIDialogSettingsManualBase.h"

struct StringSettingOption;

class CGUIDialogCMSSettings : public CGUIDialogSettingsManualBase
{
public:
  CGUIDialogCMSSettings();
  ~CGUIDialogCMSSettings() override;

protected:
  // implementations of ISettingCallback
  void OnSettingChanged(std::shared_ptr<const CSetting> setting) override;

  // specialization of CGUIDialogSettingsBase
  bool AllowResettingSettings() const override { return false; }
  bool OnBack(int actionID) override;
  void Save() override;
  void SetupView() override;

  // specialization of CGUIDialogSettingsManualBase
  void InitializeSettings() override;

private:
  static void Cms3dLutsFiller(
    std::shared_ptr<const CSetting> setting,
    std::vector<StringSettingOption> &list,
    std::string &current,
    void *data);
};
