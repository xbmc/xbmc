/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
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
#include "dialogs/GUIDialogNumeric.h"
#include "guilib/GUIKeyboardFactory.h"
#include "dialogs/GUIDialogGamepad.h"
#include "dialogs/GUIDialogContextMenu.h"
#include "guilib/GUIWindowManager.h"
#include "URL.h"
#include "guilib/LocalizeStrings.h"

CGUIDialogLockSettings::CGUIDialogLockSettings(void)
    : CGUIDialogSettings(WINDOW_DIALOG_LOCK_SETTINGS, "LockSettings.xml")
{
}

CGUIDialogLockSettings::~CGUIDialogLockSettings(void)

{
}

void CGUIDialogLockSettings::OnCancel()
{
  m_bChanged = false;
}

void CGUIDialogLockSettings::SetupPage()
{
  CGUIDialogSettings::SetupPage();
  // update our settings label
  if (m_bGetUser)
  {
    CStdString strLabel;
    CStdString strLabel2=m_strURL;
    CURL::Decode(strLabel2);
    strLabel.Format(g_localizeStrings.Get(20152),strLabel2.c_str());
    SET_CONTROL_LABEL(2,strLabel);
  }
  else
    SET_CONTROL_LABEL(2,g_localizeStrings.Get(20066));
  SET_CONTROL_HIDDEN(3);
}

void CGUIDialogLockSettings::EnableDetails(bool bEnable)
{
  for (int i=2;i<9;++i)
  {
    m_settings[i].enabled = bEnable || !m_bConditionalDetails;
    UpdateSetting(i+1);
  }
}

void CGUIDialogLockSettings::CreateSettings()
{
  // clear out any old settings
  m_settings.clear();
  // create our settings
  if (m_bGetUser)
  {
    AddButton(1,20142);
    if (!m_strUser.IsEmpty())
      m_settings[0].name.Format("%s (%s)",g_localizeStrings.Get(20142).c_str(),m_strUser.c_str());
    AddButton(2,12326);
    if (!m_locks.code.IsEmpty())
      m_settings[1].name.Format("%s (%s)",g_localizeStrings.Get(12326).c_str(),g_localizeStrings.Get(20141).c_str());
    if (m_saveUserDetails)
      AddBool(3, 13423, m_saveUserDetails);
    return;
  }
  AddButton(1,m_iButtonLabel);
  if (m_locks.mode > LOCK_MODE_QWERTY)
    m_locks.mode = LOCK_MODE_EVERYONE;
  if (m_locks.mode != LOCK_MODE_EVERYONE)
    m_settings[0].name.Format("%s (%s)",g_localizeStrings.Get(m_iButtonLabel).c_str(),g_localizeStrings.Get(12336+m_locks.mode).c_str());
  else
    m_settings[0].name.Format("%s (%s)",g_localizeStrings.Get(m_iButtonLabel).c_str(),g_localizeStrings.Get(1223).c_str());

  if (m_bDetails)
  {
    AddSeparator(2);
    AddBool(3,20038,&m_locks.music);
    AddBool(4,20039,&m_locks.video);
    AddBool(5,20040,&m_locks.pictures);
    AddBool(6,20041,&m_locks.programs);
    AddBool(7,20042,&m_locks.files);
    AddBool(8,20043,&m_locks.settings);
    AddBool(9,24090,&m_locks.addonManager);
    EnableDetails(m_locks.mode != LOCK_MODE_EVERYONE);
  }
}

void CGUIDialogLockSettings::OnSettingChanged(SettingInfo &setting)
{
  // check and update anything that needs it
  if (setting.id == 1)
  {
    if (m_bGetUser)
    {
      CStdString strHeading;
      CStdString strDecodeUrl = m_strURL;
      CURL::Decode(strDecodeUrl);
      strHeading.Format("%s %s",g_localizeStrings.Get(14062).c_str(),strDecodeUrl.c_str());
      if (CGUIKeyboardFactory::ShowAndGetInput(m_strUser,strHeading,true))
      {
        m_bChanged = true;
        m_settings[0].name.Format("%s (%s)",g_localizeStrings.Get(20142).c_str(),m_strUser.c_str());
        UpdateSetting(1);
      }
      return;
    }
    CContextButtons choices;
    choices.Add(1, 1223);
    choices.Add(2, 12337);
    choices.Add(3, 12338);
    choices.Add(4, 12339);
    
    int choice = CGUIDialogContextMenu::ShowAndGetChoice(choices);

    CStdString newPassword;
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
      if (m_bDetails)
        EnableDetails(m_locks.mode != LOCK_MODE_EVERYONE);
      m_bChanged = true;
      if (m_locks.mode != LOCK_MODE_EVERYONE)
        setting.name.Format("%s (%s)",g_localizeStrings.Get(m_iButtonLabel).c_str(),g_localizeStrings.Get(12336+m_locks.mode).c_str());
      else
        setting.name.Format("%s (%s)",g_localizeStrings.Get(m_iButtonLabel).c_str(),g_localizeStrings.Get(1223).c_str());

      UpdateSetting(1);
    }
  }
  if (setting.id == 2 && m_bGetUser)
  {
    CStdString strHeading;
    CStdString strDecodeUrl = m_strURL;
    CURL::Decode(strDecodeUrl);
    strHeading.Format("%s %s",g_localizeStrings.Get(20143).c_str(),strDecodeUrl.c_str());
    if (CGUIKeyboardFactory::ShowAndGetInput(m_locks.code,strHeading,true,true))
    {
      m_settings[1].name.Format("%s (%s)",g_localizeStrings.Get(12326).c_str(),g_localizeStrings.Get(20141).c_str());
      m_bChanged = true;
      UpdateSetting(2);
    }
    return;
  }
  if (setting.id > 1)
    m_bChanged = true;
}

bool CGUIDialogLockSettings::ShowAndGetUserAndPassword(CStdString& strUser, CStdString& strPassword, const CStdString& strURL, bool *saveUserDetails)
{
  CGUIDialogLockSettings *dialog = (CGUIDialogLockSettings *)g_windowManager.GetWindow(WINDOW_DIALOG_LOCK_SETTINGS);
  if (!dialog) return false;
  dialog->m_bGetUser = true;
  dialog->m_locks.code = strPassword;
  dialog->m_strUser = strUser;
  dialog->m_strURL = strURL;
  dialog->m_bChanged = false;
  dialog->m_saveUserDetails = saveUserDetails;
  dialog->DoModal();
  if (dialog->m_bChanged)
  {
    strUser = dialog->m_strUser;
    strPassword = dialog->m_locks.code;
    return true;
  }

  return false;
}

bool CGUIDialogLockSettings::ShowAndGetLock(LockType& iLockMode, CStdString& strPassword, int iHeader)
{
  CProfile::CLock locks(iLockMode, strPassword);
  if (ShowAndGetLock(locks, iHeader, false, false))
  {
    locks.Validate();
    iLockMode = locks.mode;
    strPassword = locks.code;
    return true;
  }
  return false;
}

bool CGUIDialogLockSettings::ShowAndGetLock(CProfile::CLock &locks, int iButtonLabel, bool bConditional, bool bDetails)
{
  CGUIDialogLockSettings *dialog = (CGUIDialogLockSettings *)g_windowManager.GetWindow(WINDOW_DIALOG_LOCK_SETTINGS);
  if (!dialog) return false;
  dialog->m_locks = locks;
  dialog->m_iButtonLabel = iButtonLabel;
  dialog->m_bChanged = false;
  dialog->m_bGetUser = false;
  dialog->m_bConditionalDetails = bConditional;
  dialog->m_bDetails = bDetails;
  dialog->DoModal();
  if (dialog->m_bChanged)
  {
    locks = dialog->m_locks;
    return true;
  }

  return false;
}
