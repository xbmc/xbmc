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

#include "GUIDialogLockSettings.h"
#include "URL.h"
#include "dialogs/GUIDialogContextMenu.h"
#include "dialogs/GUIDialogGamepad.h"
#include "guilib/GUIKeyboardFactory.h"
#include "dialogs/GUIDialogNumeric.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "settings/lib/Setting.h"
#include "settings/lib/SettingSection.h"
#include "settings/windows/GUIControlSettings.h"
#include "utils/log.h"
#include "utils/StringUtils.h"

#define SETTING_USERNAME            "user.name"
#define SETTING_PASSWORD            "user.password"
#define SETTING_PASSWORD_REMEMBER   "user.rememberpassword"

#define SETTING_LOCKCODE            "lock.code"
#define SETTING_LOCK_MUSIC          "lock.music"
#define SETTING_LOCK_VIDEOS         "lock.videos"
#define SETTING_LOCK_PICTURES       "lock.pictures"
#define SETTING_LOCK_PROGRAMS       "lock.programs"
#define SETTING_LOCK_FILEMANAGER    "lock.filemanager"
#define SETTING_LOCK_SETTINGS       "lock.settings"
#define SETTING_LOCK_ADDONMANAGER   "lock.addonmanager"

CGUIDialogLockSettings::CGUIDialogLockSettings()
    : CGUIDialogSettingsManualBase(WINDOW_DIALOG_LOCK_SETTINGS, "LockSettings.xml"),
      m_changed(false),
      m_details(true),
      m_conditionalDetails(false),
      m_getUser(false),
      m_saveUserDetails(NULL),
      m_buttonLabel(20091)
{ }

CGUIDialogLockSettings::~CGUIDialogLockSettings()
{ }

bool CGUIDialogLockSettings::ShowAndGetLock(LockType &lockMode, std::string &password, int header /* = 20091 */)
{
  CProfile::CLock locks(lockMode, password);
  if (!ShowAndGetLock(locks, header, false, false))
    return false;

  locks.Validate();
  lockMode = locks.mode;
  password = locks.code;

  return true;
}

bool CGUIDialogLockSettings::ShowAndGetLock(CProfile::CLock &locks, int buttonLabel /* = 20091 */, bool conditional /* = false */, bool details /* = true */)
{
  CGUIDialogLockSettings *dialog = static_cast<CGUIDialogLockSettings*>(g_windowManager.GetWindow(WINDOW_DIALOG_LOCK_SETTINGS));
  if (dialog == NULL)
    return false;

  dialog->m_locks = locks;
  dialog->m_buttonLabel = buttonLabel;
  dialog->m_getUser = false;
  dialog->m_conditionalDetails = conditional;
  dialog->m_details = details;
  dialog->DoModal();

  if (!dialog->m_changed)
    return false;

  locks = dialog->m_locks;
  return true;
}

bool CGUIDialogLockSettings::ShowAndGetUserAndPassword(std::string &user, std::string &password, const std::string &url, bool *saveUserDetails)
{
  CGUIDialogLockSettings *dialog = static_cast<CGUIDialogLockSettings*>(g_windowManager.GetWindow(WINDOW_DIALOG_LOCK_SETTINGS));
  if (dialog == NULL)
    return false;

  dialog->m_getUser = true;
  dialog->m_locks.code = password;
  dialog->m_user = user;
  dialog->m_url = url;
  dialog->m_saveUserDetails = saveUserDetails;
  dialog->DoModal();

  if (!dialog->m_changed)
    return false;

  user = dialog->m_user;
  password = dialog->m_locks.code;
  return true;
}

void CGUIDialogLockSettings::OnSettingChanged(const CSetting *setting)
{
  if (setting == NULL)
    return;

  CGUIDialogSettingsManualBase::OnSettingChanged(setting);

  const std::string &settingId = setting->GetId();
  if (settingId == SETTING_USERNAME)
    m_user = static_cast<const CSettingString*>(setting)->GetValue();
  else if (settingId == SETTING_PASSWORD)
    m_locks.code = static_cast<const CSettingString*>(setting)->GetValue();
  else if (settingId == SETTING_PASSWORD_REMEMBER)
    *m_saveUserDetails = static_cast<const CSettingBool*>(setting)->GetValue();
  else if (settingId == SETTING_LOCK_MUSIC)
    m_locks.music = static_cast<const CSettingBool*>(setting)->GetValue();
  else if (settingId == SETTING_LOCK_VIDEOS)
    m_locks.video = static_cast<const CSettingBool*>(setting)->GetValue();
  else if (settingId == SETTING_LOCK_PICTURES)
    m_locks.pictures = static_cast<const CSettingBool*>(setting)->GetValue();
  else if (settingId == SETTING_LOCK_PROGRAMS)
    m_locks.programs = static_cast<const CSettingBool*>(setting)->GetValue();
  else if (settingId == SETTING_LOCK_FILEMANAGER)
    m_locks.files = static_cast<const CSettingBool*>(setting)->GetValue();
  else if (settingId == SETTING_LOCK_SETTINGS)
    m_locks.settings = static_cast<LOCK_LEVEL::SETTINGS_LOCK>(static_cast<const CSettingInt*>(setting)->GetValue());
  else if (settingId == SETTING_LOCK_ADDONMANAGER)
    m_locks.addonManager = static_cast<const CSettingBool*>(setting)->GetValue();

  m_changed = true;
}

void CGUIDialogLockSettings::OnSettingAction(const CSetting *setting)
{
  if (setting == NULL)
    return;

  CGUIDialogSettingsManualBase::OnSettingAction(setting);

  const std::string &settingId = setting->GetId();
  if (settingId == SETTING_LOCKCODE)
  {
    CContextButtons choices;
    choices.Add(1, 1223);
    choices.Add(2, 12337);
    choices.Add(3, 12338);
    choices.Add(4, 12339);
    int choice = CGUIDialogContextMenu::ShowAndGetChoice(choices);

    std::string newPassword;
    LockType iLockMode = LOCK_MODE_UNKNOWN;
    bool bResult = false;
    switch(choice)
    {
      case 1:
        iLockMode = LOCK_MODE_EVERYONE; //Disabled! Need check routine!!!
        bResult = true;
        break;

      case 2:
        iLockMode = LOCK_MODE_NUMERIC;
        bResult = CGUIDialogNumeric::ShowAndVerifyNewPassword(newPassword);
        break;

      case 3:
        iLockMode = LOCK_MODE_GAMEPAD;
        bResult = CGUIDialogGamepad::ShowAndVerifyNewPassword(newPassword);
        break;

      case 4:
        iLockMode = LOCK_MODE_QWERTY;
        bResult = CGUIKeyboardFactory::ShowAndVerifyNewPassword(newPassword);
        break;

      default:
        break;
    }

    if (bResult)
    {
      if (iLockMode == LOCK_MODE_EVERYONE)
        newPassword = "-";
      m_locks.code = newPassword;
      if (m_locks.code == "-")
        iLockMode = LOCK_MODE_EVERYONE;
      m_locks.mode = iLockMode;

      setLockCodeLabel();
      setDetailSettingsEnabled(m_locks.mode != LOCK_MODE_EVERYONE);
      m_changed = true;
    }
  }
}

void CGUIDialogLockSettings::OnCancel()
{
  m_changed = false;

  CGUIDialogSettingsManualBase::OnCancel();
}

void CGUIDialogLockSettings::SetupView()
{
  CGUIDialogSettingsManualBase::SetupView();
  
  // set the title
  if (m_getUser)
    SetHeading(StringUtils::Format(g_localizeStrings.Get(20152).c_str(), CURL::Decode(m_url).c_str()));
  else
  {
    SetHeading(20066);
    setLockCodeLabel();
    setDetailSettingsEnabled(m_locks.mode != LOCK_MODE_EVERYONE);
  }
}

void CGUIDialogLockSettings::InitializeSettings()
{
  CGUIDialogSettingsManualBase::InitializeSettings();

  CSettingCategory *category = AddCategory("locksettings", -1);
  if (category == NULL)
  {
    CLog::Log(LOGERROR, "CGUIDialogLockSettings: unable to setup settings");
    return;
  }

  CSettingGroup *group = AddGroup(category);
  if (group == NULL)
  {
    CLog::Log(LOGERROR, "CGUIDialogLockSettings: unable to setup settings");
    return;
  }

  if (m_getUser)
  {
    AddEdit(group, SETTING_USERNAME, 20142, 0, m_user);
    AddEdit(group, SETTING_PASSWORD, 12326, 0, m_locks.code, false, true);
    if (m_saveUserDetails != NULL)
      AddToggle(group, SETTING_PASSWORD_REMEMBER, 13423, 0, *m_saveUserDetails);

    return;
  }

  AddButton(group, SETTING_LOCKCODE, m_buttonLabel, 0);

  if (m_details)
  {
    CSettingGroup *groupDetails = AddGroup(category);
    if (groupDetails == NULL)
    {
      CLog::Log(LOGERROR, "CGUIDialogLockSettings: unable to setup settings");
      return;
    }

    AddToggle(groupDetails, SETTING_LOCK_MUSIC, 20038, 0, m_locks.music);
    AddToggle(groupDetails, SETTING_LOCK_VIDEOS, 20039, 0, m_locks.video);
    AddToggle(groupDetails, SETTING_LOCK_PICTURES, 20040, 0, m_locks.pictures);
    AddToggle(groupDetails, SETTING_LOCK_PROGRAMS, 20041, 0, m_locks.programs);
    AddToggle(groupDetails, SETTING_LOCK_FILEMANAGER, 20042, 0, m_locks.files);

    StaticIntegerSettingOptions settingsLevelOptions;
    settingsLevelOptions.push_back(std::make_pair(106,    LOCK_LEVEL::NONE));
    settingsLevelOptions.push_back(std::make_pair(593,    LOCK_LEVEL::ALL));
    settingsLevelOptions.push_back(std::make_pair(10037,  LOCK_LEVEL::STANDARD));
    settingsLevelOptions.push_back(std::make_pair(10038,  LOCK_LEVEL::ADVANCED));
    settingsLevelOptions.push_back(std::make_pair(10039,  LOCK_LEVEL::EXPERT));
    AddSpinner(groupDetails, SETTING_LOCK_SETTINGS, 20043, 0, static_cast<int>(m_locks.settings), settingsLevelOptions);
    
    AddToggle(groupDetails, SETTING_LOCK_ADDONMANAGER, 24090, 0, m_locks.addonManager);
  }

  m_changed = false;
}

void CGUIDialogLockSettings::setDetailSettingsEnabled(bool enabled)
{
  if (!m_details)
    return;

  enabled |= !m_conditionalDetails;
  GetSettingControl(SETTING_LOCK_MUSIC)->GetSetting()->SetEnabled(enabled);
  GetSettingControl(SETTING_LOCK_VIDEOS)->GetSetting()->SetEnabled(enabled);
  GetSettingControl(SETTING_LOCK_PICTURES)->GetSetting()->SetEnabled(enabled);
  GetSettingControl(SETTING_LOCK_PROGRAMS)->GetSetting()->SetEnabled(enabled);
  GetSettingControl(SETTING_LOCK_FILEMANAGER)->GetSetting()->SetEnabled(enabled);
  GetSettingControl(SETTING_LOCK_SETTINGS)->GetSetting()->SetEnabled(enabled);
  GetSettingControl(SETTING_LOCK_ADDONMANAGER)->GetSetting()->SetEnabled(enabled);
}

void CGUIDialogLockSettings::setLockCodeLabel()
{
  // adjust label2 of the lock code button
  if (m_locks.mode > LOCK_MODE_QWERTY)
    m_locks.mode = LOCK_MODE_EVERYONE;
  BaseSettingControlPtr settingControl = GetSettingControl(SETTING_LOCKCODE);
  if (settingControl != NULL && settingControl->GetControl() != NULL)
    SET_CONTROL_LABEL2(settingControl->GetID(), g_localizeStrings.Get(m_locks.mode == LOCK_MODE_EVERYONE ? 1223 : 12336 + m_locks.mode));
}
