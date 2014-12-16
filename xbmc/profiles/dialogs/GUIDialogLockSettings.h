#pragma once

/*
 *      Copyright (C) 2005-2014 Team XBMC
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

#include "GUIPassword.h"
#include "profiles/Profile.h"
#include "settings/dialogs/GUIDialogSettingsManualBase.h"

class CGUIDialogLockSettings : public CGUIDialogSettingsManualBase
{
public:
  CGUIDialogLockSettings();
  virtual ~CGUIDialogLockSettings();

  static bool ShowAndGetLock(LockType &lockMode, std::string &password, int header = 20091);
  static bool ShowAndGetLock(CProfile::CLock &locks, int buttonLabel = 20091, bool conditional = false, bool details = true);
  static bool ShowAndGetUserAndPassword(std::string &user, std::string &password, const std::string &url, bool *saveUserDetails);

protected:
  // implementations of ISettingCallback
  virtual void OnSettingChanged(const CSetting *setting);
  virtual void OnSettingAction(const CSetting *setting);

  // specialization of CGUIDialogSettingsBase
  virtual bool AllowResettingSettings() const { return false; }
  virtual void Save() { }
  virtual void OnCancel();
  virtual void SetupView();

  // specialization of CGUIDialogSettingsManualBase
  virtual void InitializeSettings();

private:
  void setDetailSettingsEnabled(bool enabled);
  void setLockCodeLabel();

  bool m_changed;

  CProfile::CLock m_locks;
  std::string m_user;
  std::string m_url;
  bool m_details;
  bool m_conditionalDetails;
  bool m_getUser;
  bool* m_saveUserDetails;
  int m_buttonLabel;
};