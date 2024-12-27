/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "profiles/Profile.h"
#include "settings/dialogs/GUIDialogSettingsManualBase.h"

class CGUIDialogLockSettings : public CGUIDialogSettingsManualBase
{
public:
  CGUIDialogLockSettings();
  ~CGUIDialogLockSettings() override;

  static bool ShowAndGetLock(LockMode& lockMode, std::string& password, int header = 20091);
  static bool ShowAndGetLock(CProfile::CLock &locks, int buttonLabel = 20091, bool conditional = false, bool details = true);
  static bool ShowAndGetUserAndPassword(std::string &user, std::string &password, const std::string &url, bool *saveUserDetails);

protected:
  // implementations of ISettingCallback
  void OnSettingChanged(const std::shared_ptr<const CSetting>& setting) override;
  void OnSettingAction(const std::shared_ptr<const CSetting>& setting) override;

  // specialization of CGUIDialogSettingsBase
  bool AllowResettingSettings() const override { return false; }
  bool Save() override { return true; }
  void OnCancel() override;
  void SetupView() override;

  // specialization of CGUIDialogSettingsManualBase
  void InitializeSettings() override;

private:
  std::string GetLockModeLabel();
  void SetDetailSettingsEnabled(bool enabled);
  void SetSettingLockCodeLabel();

  bool m_changed = false;

  CProfile::CLock m_locks;
  std::string m_user;
  std::string m_url;
  bool m_details = true;
  bool m_conditionalDetails = false;
  bool m_getUser = false;
  bool* m_saveUserDetails;
  int m_buttonLabel = 20091;
};
