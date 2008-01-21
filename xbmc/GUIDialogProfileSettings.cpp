/*
 *      Copyright (C) 2005-2007 Team XboxMediaCenter
 *      http://www.xboxmediacenter.com
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
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include "GUIDialogProfileSettings.h"
#include "GUIDialogFileBrowser.h"
#include "GUIDialogContextMenu.h"
#include "GUIDialogNumeric.h"
#include "GUIDialogKeyboard.h"
#include "GUIDialogGamepad.h"
#include "GUIDialogLockSettings.h"
#include "MediaManager.h"
#include "Util.h"
#include "GUIPassword.h"
#include "Picture.h"

using namespace XFILE;
using namespace DIRECTORY;

#define CONTROL_PROFILE_IMAGE      2
#define CONTROL_START              30

CGUIDialogProfileSettings::CGUIDialogProfileSettings(void)
    : CGUIDialogSettings(WINDOW_DIALOG_PROFILE_SETTINGS, "ProfileSettings.xml")
{
  m_bNeedSave = false;
}

CGUIDialogProfileSettings::~CGUIDialogProfileSettings(void)
{
}

bool CGUIDialogProfileSettings::OnMessage(CGUIMessage &message)
{
  switch (message.GetMessage())
  {
  case GUI_MSG_WINDOW_DEINIT:
    {
      CGUIDialogSettings::OnMessage(message);
    }
    break;

  case GUI_MSG_CLICKED:
    int iControl = message.GetSenderId();
    if (iControl == 500)
      Close();
    if (iControl == 501)
    {
      m_bNeedSave = false;
      Close();
    }
    break;
  }
  return CGUIDialogSettings::OnMessage(message);
}

void CGUIDialogProfileSettings::OnWindowLoaded()
{
  CGUIDialogSettings::OnWindowLoaded();
  CGUIImage *pImage = (CGUIImage*)GetControl(2);
  m_strDefaultImage = pImage->GetFileName();
}

void CGUIDialogProfileSettings::SetupPage()
{
  CGUIDialogSettings::SetupPage();
  SET_CONTROL_LABEL(1000,m_strName);
  SET_CONTROL_LABEL(1001,m_strDirectory);
  CGUIImage *pImage = (CGUIImage*)GetControl(2);
  if (!m_strThumb.IsEmpty())
    pImage->SetFileName(m_strThumb);
  else
    pImage->SetFileName(m_strDefaultImage);
}

void CGUIDialogProfileSettings::CreateSettings()
{
  // clear out any old settings
  m_settings.clear();

  AddButton(1,20093);
  AddButton(2,20065);
  if (!m_bIsDefault && m_bShowDetails)
    AddButton(3,20070);

  if (m_bShowDetails)
    AddButton(4,20066);
  if (!m_bShowDetails && m_iLockMode == LOCK_MODE_EVERYONE && g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE)
    AddButton(4,20066);

  if (!m_bIsDefault && m_bShowDetails)
  {
    SettingInfo setting;
    setting.id = 5;
    setting.name = g_localizeStrings.Get(20060);
    setting.data = &m_iDbMode;
    setting.type = SettingInfo::SPIN;
    setting.entry.push_back(g_localizeStrings.Get(20062));
    setting.entry.push_back(g_localizeStrings.Get(20063));
    setting.entry.push_back(g_localizeStrings.Get(20061));
    if (g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE)
      setting.entry.push_back(g_localizeStrings.Get(20107));

    m_settings.push_back(setting);

    SettingInfo setting2;
    setting2.id = 6;
    setting2.name = g_localizeStrings.Get(20094);
    setting2.data = &m_iSourcesMode;
    setting2.type = SettingInfo::SPIN;
    setting2.entry.push_back(g_localizeStrings.Get(20062));
    setting2.entry.push_back(g_localizeStrings.Get(20063));
    setting2.entry.push_back(g_localizeStrings.Get(20061));
    if (g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE)
      setting2.entry.push_back(g_localizeStrings.Get(20107));

    m_settings.push_back(setting2);
  }
  if (m_bIsNewUser)
  {
    SetupPage();
    OnSettingChanged(0); // id=1
    if (!m_bNeedSave)
    {
      OnCancel();
      Close();
      return;
    }
    if (!m_strName.IsEmpty())
    {
      m_strDirectory.Format("profiles\\%s",m_strName.c_str());
      CUtil::GetFatXQualifiedPath(m_strDirectory);
      CDirectory::Create(g_settings.m_vecProfiles[0].getDirectory()+"\\"+m_strDirectory);
    }
    CStdString strPath = m_strDirectory;
    OnSettingChanged(2); // id=3
    if (strPath != m_strDirectory)
      CDirectory::Remove(g_settings.m_vecProfiles[0].getDirectory()+"\\"+strPath);
  }
}

void CGUIDialogProfileSettings::OnSettingChanged(unsigned int num)
{
  // setting has changed - update anything that needs it
  if (num >= m_settings.size()) return;
  SettingInfo &setting = m_settings.at(num);
  // check and update anything that needs it
  if (setting.id == 1)
  {
    if (CGUIDialogKeyboard::ShowAndGetInput(m_strName,g_localizeStrings.Get(20093),false))
    {
      m_bNeedSave = true;
      SET_CONTROL_LABEL(1000,m_strName);
    }
  }
  if (setting.id == 2)
  {
    CStdString strThumb;
    VECSHARES shares;
    g_mediaManager.GetLocalDrives(shares);
    if (CGUIDialogFileBrowser::ShowAndGetImage(shares,g_localizeStrings.Get(1030),strThumb))
    {
      m_bNeedSave = true;
      CGUIImage *pImage = (CGUIImage*)GetControl(2);
      CFileItem item(strThumb);
      item.m_strPath = strThumb;
      m_strThumb = item.GetCachedProfileThumb();
      if (CFile::Exists(m_strThumb))
        CFile::Delete(m_strThumb);

      CPicture pic;
      pic.DoCreateThumbnail(strThumb, m_strThumb);
      pImage->SetFileName("foo.bmp");
      pImage->Update();
      pImage->SetFileName(m_strThumb);
    }
  }
  if (setting.id == 3)
  {
    VECSHARES shares;
    CShare share;
    share.strName = "Profiles";
    share.strPath = g_settings.m_vecProfiles[0].getDirectory()+"\\profiles";
    shares.push_back(share);
    CStdString strDirectory;
    if (m_strDirectory == "")
      strDirectory = share.strPath;
    else
      strDirectory.Format("%s\\%s",g_settings.m_vecProfiles[0].getDirectory().c_str(),m_strDirectory.c_str());
    if (CGUIDialogFileBrowser::ShowAndGetDirectory(shares,g_localizeStrings.Get(657),strDirectory,true))
    {
      m_strDirectory = strDirectory;
      if (!m_bIsDefault)
        m_strDirectory.erase(0,g_settings.m_vecProfiles[0].getDirectory().size()+1);
      m_bNeedSave = true;
      SET_CONTROL_LABEL(1001,m_strDirectory);
    }
  }

  if (setting.id == 4)
  {
    if (m_bShowDetails)
    {
      if (g_settings.m_vecProfiles[0].getLockMode() == LOCK_MODE_EVERYONE && !m_bIsDefault)
      {
        if (CGUIDialogYesNo::ShowAndGetInput(20066,20118,20119,20022))
          g_passwordManager.SetMasterLockMode(false);
        if (g_settings.m_vecProfiles[0].getLockMode() == LOCK_MODE_EVERYONE)
          return;
      }
      if (CGUIDialogLockSettings::ShowAndGetLock(m_iLockMode,m_strLockCode,m_bLockMusic,m_bLockVideo,m_bLockPictures,m_bLockPrograms,m_bLockFiles,m_bLockSettings,m_bIsDefault?12360:20068,g_settings.m_vecProfiles[0].getLockMode() == LOCK_MODE_EVERYONE || m_bIsDefault))
        m_bNeedSave = true;
    }
    else
    {
      if (CGUIDialogLockSettings::ShowAndGetLock(m_iLockMode,m_strLockCode,m_bIsDefault?12360:20068))
        m_bNeedSave = true;
    }
  }
  if (setting.id > 4)
    m_bNeedSave = true;
}

void CGUIDialogProfileSettings::OnCancel()
{
  m_bNeedSave = false;
}

bool CGUIDialogProfileSettings::ShowForProfile(unsigned int iProfile, bool bDetails)
{
  CGUIDialogProfileSettings *dialog = (CGUIDialogProfileSettings *)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROFILE_SETTINGS);
  if (!dialog) return false;
  if (iProfile == 0)
    dialog->m_bIsDefault = true;
  else
    dialog->m_bIsDefault = false;
  if (!bDetails && iProfile > g_settings.m_vecProfiles.size())
    return false;

  dialog->m_bShowDetails = bDetails;

  if (iProfile >= g_settings.m_vecProfiles.size())
  {
    dialog->m_strName.Empty();
    dialog->m_iDbMode = 2;
    dialog->m_iLockMode = LOCK_MODE_EVERYONE;
    dialog->m_iSourcesMode = 2;
    dialog->m_bLockSettings = true;
    dialog->m_bLockMusic = false;
    dialog->m_bLockVideo = false;
    dialog->m_bLockFiles = true;
    dialog->m_bLockPictures = false;
    dialog->m_bLockPrograms = false;

    dialog->m_strDirectory.Empty();
    dialog->m_strThumb.Empty();
    dialog->m_strName = "";
    dialog->m_bIsNewUser = true;
  }
  else
  {
    dialog->m_strName = g_settings.m_vecProfiles[iProfile].getName();
    dialog->m_strThumb = g_settings.m_vecProfiles[iProfile].getThumb();
    dialog->m_strDirectory = g_settings.m_vecProfiles[iProfile].getDirectory();
    dialog->m_iDbMode = g_settings.m_vecProfiles[iProfile].canWriteDatabases()?0:1;
    dialog->m_iSourcesMode = g_settings.m_vecProfiles[iProfile].canWriteSources()?0:1;
    if (g_settings.m_vecProfiles[iProfile].hasDatabases())
      dialog->m_iDbMode += 2;
    if (g_settings.m_vecProfiles[iProfile].hasSources())
      dialog->m_iSourcesMode += 2;

    dialog->m_iLockMode = g_settings.m_vecProfiles[iProfile].getLockMode();
    dialog->m_strLockCode = g_settings.m_vecProfiles[iProfile].getLockCode();
    dialog->m_bLockFiles = g_settings.m_vecProfiles[iProfile].filesLocked();
    dialog->m_bLockMusic = g_settings.m_vecProfiles[iProfile].musicLocked();
    dialog->m_bLockVideo = g_settings.m_vecProfiles[iProfile].videoLocked();
    dialog->m_bLockPrograms = g_settings.m_vecProfiles[iProfile].programsLocked();
    dialog->m_bLockPictures = g_settings.m_vecProfiles[iProfile].picturesLocked();
    dialog->m_bLockSettings = g_settings.m_vecProfiles[iProfile].settingsLocked();
    dialog->m_bIsNewUser = false;
  }
  dialog->DoModal();
  if (dialog->m_bNeedSave)
  {
    if (iProfile >= g_settings.m_vecProfiles.size())
    {
      if (dialog->m_strName.IsEmpty() || dialog->m_strDirectory.IsEmpty())
        return false;
      /*CStdString strLabel;
      strLabel.Format(g_localizeStrings.Get(20047),dialog->m_strName);
      if (!CGUIDialogYesNo::ShowAndGetInput(g_localizeStrings.Get(20058),strLabel,dialog->m_strDirectory,""))
      {
        CDirectory::Remove(g_settings.GetUserDataFolder()+"\\"+dialog->m_strDirectory);
        return false;
      }*/

      // check for old profile settings
      CProfile profile;
      g_settings.m_vecProfiles.push_back(profile);
      bool bExists = CFile::Exists(g_settings.GetUserDataFolder()+"\\"+dialog->m_strDirectory+"\\guisettings.xml");

      if (bExists)
        if (!CGUIDialogYesNo::ShowAndGetInput(20058,20104,20105,20022))
          bExists = false;

      if (!bExists)
      {
        // save new profile guisettings
        if (CGUIDialogYesNo::ShowAndGetInput(20058,20048,20102,20022,20044,20064))
          CFile::Cache(g_settings.GetUserDataFolder()+"\\guisettings.xml",g_settings.GetUserDataFolder()+"\\"+dialog->m_strDirectory+"\\guisettings.xml");
        else
        {
          CGUISettings settings = g_guiSettings;
          CGUISettings settings2;
          g_guiSettings = settings2;
          g_settings.SaveSettings(g_settings.GetUserDataFolder()+"\\"+dialog->m_strDirectory+"\\guisettings.xml");
          g_guiSettings = settings;
        }
      }

      bExists = CFile::Exists(g_settings.GetUserDataFolder()+"\\"+dialog->m_strDirectory+"\\sources.xml");
      if (bExists)
        if (!CGUIDialogYesNo::ShowAndGetInput(20058,20106,20105,20022))
          bExists = false;

      if (!bExists)
      {
        if ((dialog->m_iSourcesMode & 2) == 2)
          if (CGUIDialogYesNo::ShowAndGetInput(20058,20071,20102,20022,20044,20064))
            CFile::Cache(g_settings.GetUserDataFolder()+"\\sources.xml",g_settings.GetUserDataFolder()+"\\"+dialog->m_strDirectory+"\\sources.xml");
      }
    }

    /*if (!dialog->m_bIsNewUser)
      if (!CGUIDialogYesNo::ShowAndGetInput(20067,20103,20022,20022))
        return false;*/

    g_settings.m_vecProfiles[iProfile].setName(dialog->m_strName);
    g_settings.m_vecProfiles[iProfile].setDirectory(dialog->m_strDirectory);
    g_settings.m_vecProfiles[iProfile].setThumb(dialog->m_strThumb);
    g_settings.m_vecProfiles[iProfile].setWriteDatabases(!((dialog->m_iDbMode & 1) == 1));
    g_settings.m_vecProfiles[iProfile].setWriteSources(!((dialog->m_iSourcesMode & 1) == 1));
    g_settings.m_vecProfiles[iProfile].setDatabases((dialog->m_iDbMode & 2) == 2);
    g_settings.m_vecProfiles[iProfile].setSources((dialog->m_iSourcesMode & 2) == 2);
    if (dialog->m_strLockCode == "-")
      g_settings.m_vecProfiles[iProfile].setLockMode(LOCK_MODE_EVERYONE);
    else
      g_settings.m_vecProfiles[iProfile].setLockMode(dialog->m_iLockMode);
    if (dialog->m_iLockMode == LOCK_MODE_EVERYONE)
      g_settings.m_vecProfiles[iProfile].setLockCode("-");
    else
      g_settings.m_vecProfiles[iProfile].setLockCode(dialog->m_strLockCode);
    g_settings.m_vecProfiles[iProfile].setMusicLocked(dialog->m_bLockMusic);
    g_settings.m_vecProfiles[iProfile].setVideoLocked(dialog->m_bLockVideo);
    g_settings.m_vecProfiles[iProfile].setSettingsLocked(dialog->m_bLockSettings);
    g_settings.m_vecProfiles[iProfile].setFilesLocked(dialog->m_bLockFiles);
    g_settings.m_vecProfiles[iProfile].setPicturesLocked(dialog->m_bLockPictures);
    g_settings.m_vecProfiles[iProfile].setProgramsLocked(dialog->m_bLockPrograms);

    g_settings.SaveProfiles("q:\\system\\profiles.xml");
    return true;
  }

  return !dialog->m_bNeedSave;
}

void CGUIDialogProfileSettings::OnInitWindow()
{
  m_bNeedSave = false;

  CGUIDialogSettings::OnInitWindow();
}

