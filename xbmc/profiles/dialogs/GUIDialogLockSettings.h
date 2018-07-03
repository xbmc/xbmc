/*
 *      Copyright (C) 2005-2014 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include "GUIPassword.h"
#include "profiles/Profile.h"
#include "settings/dialogs/GUIDialogSettingsManualBase.h"

class CGUIDialogLockSettings : public CGUIDialogSettingsManualBase
{
public:
  CGUIDialogLockSettings();
  ~CGUIDialogLockSettings() override;

  static bool ShowAndGetLock(LockType &lockMode, std::string &password, int header = 20091);
  static bool ShowAndGetLock(CProfile::CLock &locks, int buttonLabel = 20091, bool conditional = false, bool details = true);
  static bool ShowAndGetUserAndPassword(std::string &user, std::string &password, const std::string &url, bool *saveUserDetails);

protected:
  // implementations of ISettingCallback
  void OnSettingChanged(std::shared_ptr<const CSetting> setting) override;
  void OnSettingAction(std::shared_ptr<const CSetting> setting) override;

  // specialization of CGUIDialogSettingsBase
  bool AllowResettingSettings() const override { return false; }
  void Save() override { }
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
