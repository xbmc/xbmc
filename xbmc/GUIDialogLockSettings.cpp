/*
 *      Copyright (C) 2005-2008 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "GUIDialogLockSettings.h"
#include "GUIDialogNumeric.h"
#include "GUIDialogKeyboard.h"
#include "GUIDialogGamepad.h"
#include "GUIDialogContextMenu.h"
#include "GUIWindowManager.h"
#include "Util.h"
#include "LocalizeStrings.h"

CGUIDialogLockSettings::CGUIDialogLockSettings(void)
    : CGUIDialogSettings(WINDOW_DIALOG_LOCK_SETTINGS, "LockSettings.xml")
{
}

CGUIDialogLockSettings::~CGUIDialogLockSettings(void)

{
}

bool CGUIDialogLockSettings::OnMessage(CGUIMessage &message)
{
  switch (message.GetMessage())
  {
  case GUI_MSG_WINDOW_DEINIT:
    {
      CGUIDialogSettings::OnMessage(message);
    }
    break;
  }
  return CGUIDialogSettings::OnMessage(message);
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
    CUtil::UrlDecode(strLabel2);
    strLabel.Format(g_localizeStrings.Get(20152),strLabel2.c_str());
    SET_CONTROL_LABEL(2,strLabel);
  }
  else
    SET_CONTROL_LABEL(2,g_localizeStrings.Get(20066));
  SET_CONTROL_HIDDEN(3);
}

void CGUIDialogLockSettings::EnableDetails(bool bEnable)
{
  for (int i=2;i<8;++i)
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
    if (!m_strLock.IsEmpty())
      m_settings[1].name.Format("%s (%s)",g_localizeStrings.Get(12326).c_str(),g_localizeStrings.Get(20141).c_str());

    return;
  }
  AddButton(1,m_iButtonLabel);
  if (m_iLock > LOCK_MODE_QWERTY)
    m_iLock = LOCK_MODE_EVERYONE;
  if (m_iLock != LOCK_MODE_EVERYONE)
    m_settings[0].name.Format("%s (%s)",g_localizeStrings.Get(m_iButtonLabel).c_str(),g_localizeStrings.Get(12336+m_iLock).c_str());
  else
    m_settings[0].name.Format("%s (%s)",g_localizeStrings.Get(m_iButtonLabel).c_str(),g_localizeStrings.Get(1223).c_str());

  if (m_bDetails)
  {
    AddSeparator(2);
    AddBool(3,20038,&m_bLockMusic);
    AddBool(4,20039,&m_bLockVideo);
    AddBool(5,20040,&m_bLockPictures);
    AddBool(6,20041,&m_bLockPrograms);
    AddBool(7,20042,&m_bLockFiles);
    AddBool(8,20043,&m_bLockSettings);
    EnableDetails(m_iLock != LOCK_MODE_EVERYONE);
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
      CUtil::UrlDecode(strDecodeUrl);
      strHeading.Format("%s %s",g_localizeStrings.Get(14062).c_str(),strDecodeUrl.c_str());
      if (CGUIDialogKeyboard::ShowAndGetInput(m_strUser,strHeading,true))
      {
        m_bChanged = true;
        m_settings[0].name.Format("%s (%s)",g_localizeStrings.Get(20142).c_str(),m_strUser.c_str());
        UpdateSetting(1);
      }
      return;
    }
    CGUIDialogContextMenu *menu = (CGUIDialogContextMenu *)g_windowManager.GetWindow(WINDOW_DIALOG_CONTEXT_MENU);
    if (menu)
    {
      menu->Initialize();
      menu->AddButton(1223);
      menu->AddButton(12337);
      menu->AddButton(12338);
      menu->AddButton(12339);
      menu->CenterWindow();
      menu->DoModal();

      CStdString newPassword;
      LockType iLockMode = LOCK_MODE_UNKNOWN;
      bool bResult = false;
      switch(menu->GetButton())
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
        bResult = CGUIDialogKeyboard::ShowAndVerifyNewPassword(newPassword);
        break;
      default:
        break;
      }
      if (bResult)
      {
        if (iLockMode == LOCK_MODE_EVERYONE)
          newPassword = "-";
        m_strLock = newPassword;
        if (m_strLock == "-")
          iLockMode = LOCK_MODE_EVERYONE;
        m_iLock = iLockMode;
        if (m_bDetails)
          EnableDetails(m_iLock != LOCK_MODE_EVERYONE);
        m_bChanged = true;
        if (m_iLock != LOCK_MODE_EVERYONE)
          setting.name.Format("%s (%s)",g_localizeStrings.Get(m_iButtonLabel).c_str(),g_localizeStrings.Get(12336+m_iLock).c_str());
        else
          setting.name.Format("%s (%s)",g_localizeStrings.Get(m_iButtonLabel).c_str(),g_localizeStrings.Get(1223).c_str());

        UpdateSetting(1);
      }
    }
  }
  if (setting.id == 2 && m_bGetUser)
  {
    CStdString strHeading;
    CStdString strDecodeUrl = m_strURL;
    CUtil::UrlDecode(strDecodeUrl);
    strHeading.Format("%s %s",g_localizeStrings.Get(20143).c_str(),strDecodeUrl.c_str());
    if (CGUIDialogKeyboard::ShowAndGetInput(m_strLock,strHeading,true,true))
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

bool CGUIDialogLockSettings::ShowAndGetUserAndPassword(CStdString& strUser, CStdString& strPassword, const CStdString& strURL)
{
  CGUIDialogLockSettings *dialog = (CGUIDialogLockSettings *)g_windowManager.GetWindow(WINDOW_DIALOG_LOCK_SETTINGS);
  if (!dialog) return false;
  dialog->m_bGetUser = true;
  dialog->m_strLock = strPassword;
  dialog->m_strUser = strUser;
  dialog->m_strURL = strURL;
  dialog->m_bChanged = false;
  dialog->DoModal();
  if (dialog->m_bChanged)
  {
    strUser = dialog->m_strUser;
    strPassword = dialog->m_strLock;
    return true;
  }

  return false;
}

bool CGUIDialogLockSettings::ShowAndGetLock(LockType& iLockMode, CStdString& strPassword, int iHeader)
{
  bool f;
  return ShowAndGetLock(iLockMode,strPassword,f,f,f,f,f,f,iHeader,false,false);
}

bool CGUIDialogLockSettings::ShowAndGetLock(LockType& iLockMode, CStdString& strPassword, bool& bLockMusic, bool& bLockVideo, bool& bLockPictures, bool& bLockPrograms, bool& bLockFiles, bool& bLockSettings, int iButtonLabel, bool bConditional, bool bDetails)
{
  CGUIDialogLockSettings *dialog = (CGUIDialogLockSettings *)g_windowManager.GetWindow(WINDOW_DIALOG_LOCK_SETTINGS);
  if (!dialog) return false;
  dialog->m_iLock = iLockMode;
  dialog->m_iButtonLabel = iButtonLabel;
  dialog->m_strLock = strPassword;
  dialog->m_bChanged = false;
  dialog->m_bGetUser = false;
  dialog->m_bConditionalDetails = bConditional;
  if (bDetails)
  {
    dialog->m_bLockMusic = bLockMusic;
    dialog->m_bLockVideo = bLockVideo;
    dialog->m_bLockPrograms = bLockPrograms;
    dialog->m_bLockPictures = bLockPictures;
    dialog->m_bLockFiles = bLockFiles;
    dialog->m_bLockSettings = bLockSettings;
  }
  dialog->m_bDetails = bDetails;
  dialog->DoModal();
  if (dialog->m_bChanged)
  {
    if (dialog->m_iLock != LOCK_MODE_EVERYONE && (dialog->m_strLock == "-" || dialog->m_strLock.IsEmpty()))
      iLockMode = LOCK_MODE_EVERYONE;
    else
      iLockMode = dialog->m_iLock;

    if (dialog->m_strLock.IsEmpty() || iLockMode == LOCK_MODE_EVERYONE)
      strPassword = "-";
    else
      strPassword = dialog->m_strLock;
    if (bDetails)
    {
      bLockMusic = dialog->m_bLockMusic;
      bLockVideo = dialog->m_bLockVideo;
      bLockPrograms = dialog->m_bLockPrograms;
      bLockPictures = dialog->m_bLockPictures;
      bLockFiles = dialog->m_bLockFiles;
      bLockSettings = dialog->m_bLockSettings;
    }
    return true;
  }

  return false;
}

void CGUIDialogLockSettings::OnInitWindow()
{
  CGUIDialogSettings::OnInitWindow();
}

