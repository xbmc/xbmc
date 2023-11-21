/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIDialogLockSettings.h"

#include "ServiceBroker.h"
#include "URL.h"
#include "dialogs/GUIDialogGamepad.h"
#include "dialogs/GUIDialogNumeric.h"
#include "dialogs/GUIDialogSelect.h"
#include "favourites/FavouritesService.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIKeyboardFactory.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "settings/lib/Setting.h"
#include "settings/lib/SettingSection.h"
#include "settings/windows/GUIControlSettings.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#include <utility>

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
    : CGUIDialogSettingsManualBase(WINDOW_DIALOG_LOCK_SETTINGS, "DialogSettings.xml"),
      m_saveUserDetails(NULL)
{ }

CGUIDialogLockSettings::~CGUIDialogLockSettings() = default;

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
  CGUIDialogLockSettings *dialog = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogLockSettings>(WINDOW_DIALOG_LOCK_SETTINGS);
  if (dialog == NULL)
    return false;

  dialog->m_locks = locks;
  dialog->m_buttonLabel = buttonLabel;
  dialog->m_getUser = false;
  dialog->m_conditionalDetails = conditional;
  dialog->m_details = details;
  dialog->Open();

  if (!dialog->m_changed)
    return false;

  locks = dialog->m_locks;

  // changed lock settings for certain sections (e.g. video, audio, or pictures)
  // => refresh favourites due to possible visibility changes
  CServiceBroker::GetFavouritesService().RefreshFavourites();

  return true;
}

bool CGUIDialogLockSettings::ShowAndGetUserAndPassword(std::string &user, std::string &password, const std::string &url, bool *saveUserDetails)
{
  CGUIDialogLockSettings *dialog = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogLockSettings>(WINDOW_DIALOG_LOCK_SETTINGS);
  if (dialog == NULL)
    return false;

  dialog->m_getUser = true;
  dialog->m_locks.code = password;
  dialog->m_user = user;
  dialog->m_url = url;
  dialog->m_saveUserDetails = saveUserDetails;
  dialog->Open();

  if (!dialog->m_changed)
    return false;

  user = dialog->m_user;
  password = dialog->m_locks.code;
  return true;
}

void CGUIDialogLockSettings::OnSettingChanged(const std::shared_ptr<const CSetting>& setting)
{
  if (setting == NULL)
    return;

  CGUIDialogSettingsManualBase::OnSettingChanged(setting);

  const std::string &settingId = setting->GetId();
  if (settingId == SETTING_USERNAME)
    m_user = std::static_pointer_cast<const CSettingString>(setting)->GetValue();
  else if (settingId == SETTING_PASSWORD)
    m_locks.code = std::static_pointer_cast<const CSettingString>(setting)->GetValue();
  else if (settingId == SETTING_PASSWORD_REMEMBER)
    *m_saveUserDetails = std::static_pointer_cast<const CSettingBool>(setting)->GetValue();
  else if (settingId == SETTING_LOCK_MUSIC)
    m_locks.music = std::static_pointer_cast<const CSettingBool>(setting)->GetValue();
  else if (settingId == SETTING_LOCK_VIDEOS)
    m_locks.video = std::static_pointer_cast<const CSettingBool>(setting)->GetValue();
  else if (settingId == SETTING_LOCK_PICTURES)
    m_locks.pictures = std::static_pointer_cast<const CSettingBool>(setting)->GetValue();
  else if (settingId == SETTING_LOCK_PROGRAMS)
    m_locks.programs = std::static_pointer_cast<const CSettingBool>(setting)->GetValue();
  else if (settingId == SETTING_LOCK_FILEMANAGER)
    m_locks.files = std::static_pointer_cast<const CSettingBool>(setting)->GetValue();
  else if (settingId == SETTING_LOCK_SETTINGS)
    m_locks.settings = static_cast<LOCK_LEVEL::SETTINGS_LOCK>(std::static_pointer_cast<const CSettingInt>(setting)->GetValue());
  else if (settingId == SETTING_LOCK_ADDONMANAGER)
    m_locks.addonManager = std::static_pointer_cast<const CSettingBool>(setting)->GetValue();

  m_changed = true;
}

void CGUIDialogLockSettings::OnSettingAction(const std::shared_ptr<const CSetting>& setting)
{
  if (setting == NULL)
    return;

  CGUIDialogSettingsManualBase::OnSettingAction(setting);

  const std::string &settingId = setting->GetId();
  if (settingId == SETTING_LOCKCODE)
  {
    CGUIDialogSelect* dialog = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogSelect>(WINDOW_DIALOG_SELECT);
    if (!dialog)
      return;

    dialog->Reset();
    dialog->SetHeading(CVariant{12360});
    dialog->Add(g_localizeStrings.Get(1223));
    dialog->Add(g_localizeStrings.Get(12337));
    dialog->Add(g_localizeStrings.Get(12338));
    dialog->Add(g_localizeStrings.Get(12339));
    dialog->SetSelected(GetLockModeLabel());
    dialog->Open();

    std::string newPassword;
    LockType iLockMode = LOCK_MODE_UNKNOWN;
    bool bResult = false;
    switch (dialog->GetSelectedItem())
    {
      case 0:
        iLockMode = LOCK_MODE_EVERYONE; //Disabled! Need check routine!!!
        bResult = true;
        break;

      case 1:
        iLockMode = LOCK_MODE_NUMERIC;
        bResult = CGUIDialogNumeric::ShowAndVerifyNewPassword(newPassword);
        break;

      case 2:
        iLockMode = LOCK_MODE_GAMEPAD;
        bResult = CGUIDialogGamepad::ShowAndVerifyNewPassword(newPassword);
        break;

      case 3:
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

      SetSettingLockCodeLabel();
      SetDetailSettingsEnabled(m_locks.mode != LOCK_MODE_EVERYONE);
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
    SetHeading(StringUtils::Format(g_localizeStrings.Get(20152), CURL::Decode(m_url)));
  else
  {
    SetHeading(20066);
    SetSettingLockCodeLabel();
    SetDetailSettingsEnabled(m_locks.mode != LOCK_MODE_EVERYONE);
  }
  SET_CONTROL_HIDDEN(CONTROL_SETTINGS_CUSTOM_BUTTON);
  SET_CONTROL_LABEL(CONTROL_SETTINGS_OKAY_BUTTON, 186);
  SET_CONTROL_LABEL(CONTROL_SETTINGS_CANCEL_BUTTON, 222);
}

void CGUIDialogLockSettings::InitializeSettings()
{
  CGUIDialogSettingsManualBase::InitializeSettings();

  const std::shared_ptr<CSettingCategory> category = AddCategory("locksettings", -1);
  if (category == NULL)
  {
    CLog::Log(LOGERROR, "CGUIDialogLockSettings: unable to setup settings");
    return;
  }

  const std::shared_ptr<CSettingGroup> group = AddGroup(category);
  if (group == NULL)
  {
    CLog::Log(LOGERROR, "CGUIDialogLockSettings: unable to setup settings");
    return;
  }

  if (m_getUser)
  {
    AddEdit(group, SETTING_USERNAME, 20142, SettingLevel::Basic, m_user);
    AddEdit(group, SETTING_PASSWORD, 12326, SettingLevel::Basic, m_locks.code, false, true);
    if (m_saveUserDetails != NULL)
      AddToggle(group, SETTING_PASSWORD_REMEMBER, 13423, SettingLevel::Basic, *m_saveUserDetails);

    return;
  }

  AddButton(group, SETTING_LOCKCODE, m_buttonLabel, SettingLevel::Basic);

  if (m_details)
  {
    const std::shared_ptr<CSettingGroup> groupDetails = AddGroup(category);
    if (groupDetails == NULL)
    {
      CLog::Log(LOGERROR, "CGUIDialogLockSettings: unable to setup settings");
      return;
    }

    AddToggle(groupDetails, SETTING_LOCK_MUSIC, 20038, SettingLevel::Basic, m_locks.music);
    AddToggle(groupDetails, SETTING_LOCK_VIDEOS, 20039, SettingLevel::Basic, m_locks.video);
    AddToggle(groupDetails, SETTING_LOCK_PICTURES, 20040, SettingLevel::Basic, m_locks.pictures);
    AddToggle(groupDetails, SETTING_LOCK_PROGRAMS, 20041, SettingLevel::Basic, m_locks.programs);
    AddToggle(groupDetails, SETTING_LOCK_FILEMANAGER, 20042, SettingLevel::Basic, m_locks.files);

    TranslatableIntegerSettingOptions settingsLevelOptions;
    settingsLevelOptions.emplace_back(106, LOCK_LEVEL::NONE);
    settingsLevelOptions.emplace_back(593, LOCK_LEVEL::ALL);
    settingsLevelOptions.emplace_back(10037, LOCK_LEVEL::STANDARD);
    settingsLevelOptions.emplace_back(10038, LOCK_LEVEL::ADVANCED);
    settingsLevelOptions.emplace_back(10039, LOCK_LEVEL::EXPERT);
    AddList(groupDetails, SETTING_LOCK_SETTINGS, 20043, SettingLevel::Basic, static_cast<int>(m_locks.settings), settingsLevelOptions, 20043);

    AddToggle(groupDetails, SETTING_LOCK_ADDONMANAGER, 24090, SettingLevel::Basic, m_locks.addonManager);
  }

  m_changed = false;
}

std::string CGUIDialogLockSettings::GetLockModeLabel()
{
  return g_localizeStrings.Get(m_locks.mode == LOCK_MODE_EVERYONE ? 1223 : 12336 + m_locks.mode);
}

void CGUIDialogLockSettings::SetDetailSettingsEnabled(bool enabled)
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

void CGUIDialogLockSettings::SetSettingLockCodeLabel()
{
  // adjust label2 of the lock code setting button
  if (m_locks.mode > LOCK_MODE_QWERTY)
    m_locks.mode = LOCK_MODE_EVERYONE;
  BaseSettingControlPtr settingControl = GetSettingControl(SETTING_LOCKCODE);
  if (settingControl != NULL && settingControl->GetControl() != NULL)
    SET_CONTROL_LABEL2(settingControl->GetID(), GetLockModeLabel());
}
