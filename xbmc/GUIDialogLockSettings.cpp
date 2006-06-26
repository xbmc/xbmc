#include "stdafx.h"
#include "GUIDialogLockSettings.h"
#include "GUIDialogNumeric.h"
#include "GUIDialogKeyboard.h"
#include "GUIDialogGamepad.h"
#include "GUIDialogContextMenu.h"

CGUIDialogLockSettings::CGUIDialogLockSettings(void)
    : CGUIDialogSettings(WINDOW_DIALOG_LOCK_SETTINGS, "LockSettings.xml")
{
  m_loadOnDemand = true;
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

void CGUIDialogLockSettings::SetupPage()
{
  CGUIDialogSettings::SetupPage();
  // update our settings label
  SET_CONTROL_LABEL(2,g_localizeStrings.Get(20066));
}

void CGUIDialogLockSettings::CreateSettings()
{
  // clear out any old settings
  m_settings.clear();
  // create our settings
  AddButton(1,20091);
  if (m_iLock != LOCK_MODE_EVERYONE)
    m_settings[0].name.Format("%s (%s)",g_localizeStrings.Get(20091).c_str(),g_localizeStrings.Get(12336+m_iLock));
  else
    m_settings[0].name.Format("%s (%s)",g_localizeStrings.Get(20091).c_str(),g_localizeStrings.Get(1223).c_str());

  if (m_bDetails)
  {
    AddSeparator(2);
    AddBool(3,20038,&m_bLockMusic);
    AddBool(4,20039,&m_bLockVideo);
    AddBool(5,20040,&m_bLockPictures);
    AddBool(6,20041,&m_bLockPrograms);
    AddBool(7,20042,&m_bLockFiles);
    AddBool(8,20043,&m_bLockSettings);
  }
}

void CGUIDialogLockSettings::OnSettingChanged(unsigned int num)
{
  // setting has changed - update anything that needs it
  if (num >= m_settings.size()) return;
  SettingInfo &setting = m_settings.at(num);
  // check and update anything that needs it
  if (setting.id == 1)
  {
    CGUIDialogContextMenu *menu = (CGUIDialogContextMenu *)m_gWindowManager.GetWindow(WINDOW_DIALOG_CONTEXT_MENU);
    if (menu)
    {
      menu->Initialize();
      menu->AddButton(1223);
      menu->AddButton(12337);
      menu->AddButton(12338);
      menu->AddButton(12339);
      menu->SetPosition((g_graphicsContext.GetWidth() - menu->GetWidth()) / 2, (g_graphicsContext.GetHeight() - menu->GetHeight()) / 2);
      menu->DoModal();

      CStdString newPassword;
      int iLockMode = -1;
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
      }
      if (bResult)
      {
        if (iLockMode == LOCK_MODE_EVERYONE)
          newPassword = "-";
        m_strLock = newPassword;
        if (m_strLock == "-")
          iLockMode = LOCK_MODE_EVERYONE;
        m_iLock = iLockMode;
        m_bChanged = true;
        if (m_iLock != LOCK_MODE_EVERYONE)
          setting.name.Format("%s (%s)",g_localizeStrings.Get(20091),g_localizeStrings.Get(12336+m_iLock));
        else
          setting.name.Format("%s (%s)",g_localizeStrings.Get(20091),g_localizeStrings.Get(1223));

        UpdateSetting(1);
      }
    }
  }
  if (setting.id > 1)
    m_bChanged = true;
}

bool CGUIDialogLockSettings::ShowAndGetLock(int& iLockMode, CStdString& strPassword)
{
  bool f;
  return ShowAndGetLock(iLockMode,strPassword,f,f,f,f,f,f,false);
}

bool CGUIDialogLockSettings::ShowAndGetLock(int& iLockMode, CStdString& strPassword, bool& bLockMusic, bool& bLockVideo, bool& bLockPictures, bool& bLockPrograms, bool& bLockFiles, bool& bLockSettings, bool bDetails)
{
  CGUIDialogLockSettings *dialog = (CGUIDialogLockSettings *)m_gWindowManager.GetWindow(WINDOW_DIALOG_LOCK_SETTINGS);
  if (!dialog) return false;
  dialog->m_iLock = iLockMode;
  dialog->m_strLock = strPassword;
  dialog->m_bChanged = false;
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
    if (dialog->m_iLock != LOCK_MODE_EVERYONE && dialog->m_strLock == "-")
      iLockMode = LOCK_MODE_EVERYONE;
    else
      iLockMode = dialog->m_iLock;

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