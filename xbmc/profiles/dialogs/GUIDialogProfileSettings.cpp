/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include "GUIDialogProfileSettings.h"
#include "FileItem.h"
#include "GUIPassword.h"
#include "Util.h"
#include "dialogs/GUIDialogFileBrowser.h"
#include "dialogs/GUIDialogYesNo.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "guilib/GUIKeyboardFactory.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "profiles/ProfilesManager.h"
#include "profiles/dialogs/GUIDialogLockSettings.h"
#include "settings/lib/Setting.h"
#include "storage/MediaManager.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"

#define CONTROL_PROFILE_IMAGE         CONTROL_SETTINGS_CUSTOM + 1
#define CONTROL_PROFILE_NAME          CONTROL_SETTINGS_CUSTOM + 2
#define CONTROL_PROFILE_DIRECTORY     CONTROL_SETTINGS_CUSTOM + 3

#define SETTING_PROFILE_NAME          "profile.name"
#define SETTING_PROFILE_IMAGE         "profile.image"
#define SETTING_PROFILE_DIRECTORY     "profile.directory"
#define SETTING_PROFILE_LOCKS         "profile.locks"
#define SETTING_PROFILE_MEDIA         "profile.media"
#define SETTING_PROFILE_MEDIA_SOURCES "profile.mediasources"

CGUIDialogProfileSettings::CGUIDialogProfileSettings()
    : CGUIDialogSettingsManualBase(WINDOW_DIALOG_PROFILE_SETTINGS, "ProfileSettings.xml"),
      m_needsSaving(false)
{ }

CGUIDialogProfileSettings::~CGUIDialogProfileSettings()
{ }

bool CGUIDialogProfileSettings::ShowForProfile(unsigned int iProfile, bool firstLogin)
{
  if (firstLogin && iProfile > CProfilesManager::Get().GetNumberOfProfiles())
    return false;

  CGUIDialogProfileSettings *dialog = (CGUIDialogProfileSettings *)g_windowManager.GetWindow(WINDOW_DIALOG_PROFILE_SETTINGS);
  if (dialog == NULL)
    return false;

  dialog->m_needsSaving = false;
  dialog->m_isDefault = iProfile == 0;
  dialog->m_showDetails = !firstLogin;

  const CProfile *profile = CProfilesManager::Get().GetProfile(iProfile);
  if (profile == NULL)
  {
    dialog->m_name.clear();
    dialog->m_dbMode = 2;
    dialog->m_sourcesMode = 2;
    dialog->m_locks = CProfile::CLock();

    bool bLock = CProfilesManager::Get().GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser;
    dialog->m_locks.addonManager = bLock;
    dialog->m_locks.settings = (bLock) ? LOCK_LEVEL::ALL : LOCK_LEVEL::NONE;
    dialog->m_locks.files = bLock;

    dialog->m_directory.clear();
    dialog->m_thumb.clear();

    // prompt for a name
    std::string profileName;
    if (!CGUIKeyboardFactory::ShowAndGetInput(profileName, g_localizeStrings.Get(20093),false) || profileName.empty())
      return false;
    dialog->m_name = profileName;

    // create a default path
    std::string defaultDir = URIUtils::AddFileToFolder("profiles", CUtil::MakeLegalFileName(dialog->m_name));
    URIUtils::AddSlashAtEnd(defaultDir);
    XFILE::CDirectory::Create(URIUtils::AddFileToFolder("special://masterprofile/", defaultDir));

    // prompt for the user to change it if they want
    std::string userDir = defaultDir;
    if (GetProfilePath(userDir, false)) // can't be the master user
    {
      if (!StringUtils::StartsWith(userDir, defaultDir)) // user chose a different folder
        XFILE::CDirectory::Remove(URIUtils::AddFileToFolder("special://masterprofile/", defaultDir));
    }
    dialog->m_directory = userDir;
    dialog->m_needsSaving = true;
  }
  else
  {
    dialog->m_name = profile->getName();
    dialog->m_thumb = profile->getThumb();
    dialog->m_directory = profile->getDirectory();
    dialog->m_dbMode = profile->canWriteDatabases() ? 0 : 1;
    if (profile->hasDatabases())
      dialog->m_dbMode += 2;
    dialog->m_sourcesMode = profile->canWriteSources() ? 0 : 1;
    if (profile->hasSources())
      dialog->m_sourcesMode += 2;

    dialog->m_locks = profile->GetLocks();
  }

  dialog->DoModal();
  if (dialog->m_needsSaving)
  {
    if (iProfile >= CProfilesManager::Get().GetNumberOfProfiles())
    {
      if (dialog->m_name.empty() || dialog->m_directory.empty())
        return false;

      /*std::string strLabel;
      strLabel.Format(g_localizeStrings.Get(20047),dialog->m_strName);
      if (!CGUIDialogYesNo::ShowAndGetInput(20058, strLabel, dialog->m_strDirectory, ""))
      {
        CDirectory::Remove(URIUtils::AddFileToFolder(CProfilesManager::Get().GetUserDataFolder(), dialog->m_strDirectory));
        return false;
      }*/

      // check for old profile settings
      CProfile profile(dialog->m_directory, dialog->m_name, CProfilesManager::Get().GetNextProfileId());
      CProfilesManager::Get().AddProfile(profile);
      bool exists = XFILE::CFile::Exists(URIUtils::AddFileToFolder("special://masterprofile/", dialog->m_directory + "/guisettings.xml"));

      if (exists && !CGUIDialogYesNo::ShowAndGetInput(20058, 20104))
        exists = false;

      if (!exists)
      {
        // copy masterprofile guisettings to new profile guisettings
        // If the user selects 'start fresh', do nothing as a fresh
        // guisettings.xml will be created on first profile use.
        if (CGUIDialogYesNo::ShowAndGetInput(20058, 20048, "", "", 20044, 20064))
        {
          XFILE::CFile::Copy(URIUtils::AddFileToFolder("special://masterprofile/", "guisettings.xml"),
                              URIUtils::AddFileToFolder("special://masterprofile/", dialog->m_directory + "/guisettings.xml"));
        }
      }

      exists = XFILE::CFile::Exists(URIUtils::AddFileToFolder("special://masterprofile/", dialog->m_directory + "/sources.xml"));
      if (exists && !CGUIDialogYesNo::ShowAndGetInput(20058, 20106))
        exists = false;

      if (!exists)
      {
        if ((dialog->m_sourcesMode & 2) == 2)
          // prompt user to copy masterprofile's sources.xml file
          // If 'start fresh' (no) is selected, do nothing.
          if (CGUIDialogYesNo::ShowAndGetInput(20058, 20071, "", "", 20044, 20064))
          {
            XFILE::CFile::Copy(URIUtils::AddFileToFolder("special://masterprofile/", "sources.xml"),
                                URIUtils::AddFileToFolder("special://masterprofile/", dialog->m_directory + "/sources.xml"));
          }
      }
    }

    /*if (!dialog->m_bIsNewUser)
      if (!CGUIDialogYesNo::ShowAndGetInput(20067, 20103))
        return false;*/

    CProfile *profile = CProfilesManager::Get().GetProfile(iProfile);
    assert(profile);
    profile->setName(dialog->m_name);
    profile->setDirectory(dialog->m_directory);
    profile->setThumb(dialog->m_thumb);
    profile->setWriteDatabases(!((dialog->m_dbMode & 1) == 1));
    profile->setWriteSources(!((dialog->m_sourcesMode & 1) == 1));
    profile->setDatabases((dialog->m_dbMode & 2) == 2);
    profile->setSources((dialog->m_sourcesMode & 2) == 2);
    profile->SetLocks(dialog->m_locks);
    CProfilesManager::Get().Save();

    return true;
  }

  return dialog->m_needsSaving;
}

void CGUIDialogProfileSettings::OnWindowLoaded()
{
  CGUIDialogSettingsManualBase::OnWindowLoaded();

  CGUIMessage msg(GUI_MSG_GET_FILENAME, GetID(), CONTROL_PROFILE_IMAGE);
  OnMessage(msg);
  m_defaultImage = msg.GetLabel();
}

void CGUIDialogProfileSettings::OnSettingChanged(const CSetting *setting)
{
  if (setting == NULL)
    return;

  CGUIDialogSettingsManualBase::OnSettingChanged(setting);

  const std::string &settingId = setting->GetId();
  if (settingId == SETTING_PROFILE_NAME)
  {
    m_name = static_cast<const CSettingString*>(setting)->GetValue();
    updateProfileName();
  }
  else if (settingId == SETTING_PROFILE_MEDIA)
    m_dbMode = static_cast<const CSettingInt*>(setting)->GetValue();
  else if (settingId == SETTING_PROFILE_MEDIA_SOURCES)
    m_sourcesMode = static_cast<const CSettingInt*>(setting)->GetValue();

  m_needsSaving = true;
}

void CGUIDialogProfileSettings::OnSettingAction(const CSetting *setting)
{
  if (setting == NULL)
    return;

  CGUIDialogSettingsManualBase::OnSettingAction(setting);

  const std::string &settingId = setting->GetId();
  if (settingId == SETTING_PROFILE_IMAGE)
  {
    VECSOURCES shares;
    g_mediaManager.GetLocalDrives(shares);

    CFileItemList items;
    if (!m_thumb.empty())
    {
      CFileItemPtr item(new CFileItem("thumb://Current", false));
      item->SetArt("thumb", m_thumb);
      item->SetLabel(g_localizeStrings.Get(20016));
      items.Add(item);
    }

    CFileItemPtr item(new CFileItem("thumb://None", false));
    item->SetArt("thumb", m_defaultImage);
    item->SetLabel(g_localizeStrings.Get(20018));
    items.Add(item);

    std::string thumb;
    if (CGUIDialogFileBrowser::ShowAndGetImage(items, shares, g_localizeStrings.Get(1030), thumb) &&
        !StringUtils::EqualsNoCase(thumb, "thumb://Current"))
    {
      m_needsSaving = true;
      m_thumb = StringUtils::EqualsNoCase(thumb, "thumb://None") ? "" : thumb;

      SET_CONTROL_FILENAME(CONTROL_PROFILE_IMAGE, !m_thumb.empty() ? m_thumb : m_defaultImage);
    }
  }
  else if (settingId == SETTING_PROFILE_DIRECTORY)
  {
    if (!GetProfilePath(m_directory, m_isDefault))
      return;

    m_needsSaving = true;
    updateProfileDirectory();
  }
  else if (settingId == SETTING_PROFILE_LOCKS)
  {
    if (m_showDetails)
    {
      if (CProfilesManager::Get().GetMasterProfile().getLockMode() == LOCK_MODE_EVERYONE && !m_isDefault)
      {
        if (CGUIDialogYesNo::ShowAndGetInput(20066, 20118))
          g_passwordManager.SetMasterLockMode(false);
        if (CProfilesManager::Get().GetMasterProfile().getLockMode() == LOCK_MODE_EVERYONE)
          return;
      }
      if (CGUIDialogLockSettings::ShowAndGetLock(m_locks, m_isDefault ? 12360 : 20068,
              CProfilesManager::Get().GetMasterProfile().getLockMode() == LOCK_MODE_EVERYONE || m_isDefault))
        m_needsSaving = true;
    }
    else
    {
      if (CGUIDialogLockSettings::ShowAndGetLock(m_locks, m_isDefault ? 12360 : 20068, false, false))
        m_needsSaving = true;
    }
  }
}

void CGUIDialogProfileSettings::OnCancel()
{
  m_needsSaving = false;

  CGUIDialogSettingsManualBase::OnCancel();
}

void CGUIDialogProfileSettings::SetupView()
{
  CGUIDialogSettingsManualBase::SetupView();

  // set the heading
  SetHeading(!m_showDetails ? 20255 : 20067);

  // set the profile name and directory
  updateProfileName();
  updateProfileDirectory();

  // set the image
  SET_CONTROL_FILENAME(CONTROL_PROFILE_IMAGE, !m_thumb.empty() ? m_thumb : m_defaultImage);
}

void CGUIDialogProfileSettings::InitializeSettings()
{
  CGUIDialogSettingsManualBase::InitializeSettings();

  CSettingCategory *category = AddCategory("profilesettings", -1);
  if (category == NULL)
  {
    CLog::Log(LOGERROR, "CGUIDialogProfileSettings: unable to setup settings");
    return;
  }

  CSettingGroup *group = AddGroup(category);
  if (group == NULL)
  {
    CLog::Log(LOGERROR, "CGUIDialogProfileSettings: unable to setup settings");
    return;
  }

  AddEdit(group, SETTING_PROFILE_NAME, 20093, 0, m_name);
  AddButton(group, SETTING_PROFILE_IMAGE, 20065, 0);

  if (!m_isDefault && m_showDetails)
    AddButton(group, SETTING_PROFILE_DIRECTORY, 20070, 0);

  if (m_showDetails ||
     (m_locks.mode == LOCK_MODE_EVERYONE && CProfilesManager::Get().GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE))
    AddButton(group, SETTING_PROFILE_LOCKS, 20066, 0);

  if (!m_isDefault && m_showDetails)
  {
    CSettingGroup *groupMedia = AddGroup(category);
    if (groupMedia == NULL)
    {
      CLog::Log(LOGERROR, "CGUIDialogProfileSettings: unable to setup settings");
      return;
    }

    StaticIntegerSettingOptions entries;
    entries.push_back(std::make_pair(20062, 0));
    entries.push_back(std::make_pair(20063, 1));
    entries.push_back(std::make_pair(20061, 2));
    if (CProfilesManager::Get().GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE)
      entries.push_back(std::make_pair(20107, 3));

    AddSpinner(groupMedia, SETTING_PROFILE_MEDIA, 20060, 0, m_dbMode, entries);
    AddSpinner(groupMedia, SETTING_PROFILE_MEDIA_SOURCES, 20094, 0, m_sourcesMode, entries);
  }
}

bool CGUIDialogProfileSettings::GetProfilePath(std::string &directory, bool isDefault)
{
  VECSOURCES shares;
  CMediaSource share;
  share.strName = g_localizeStrings.Get(13200);
  share.strPath = "special://masterprofile/profiles/";
  shares.push_back(share);

  std::string strDirectory;
  if (directory.empty())
    strDirectory = share.strPath;
  else
    strDirectory = URIUtils::AddFileToFolder("special://masterprofile/", directory);

  if (!CGUIDialogFileBrowser::ShowAndGetDirectory(shares, g_localizeStrings.Get(657), strDirectory, true))
    return false;

  directory = strDirectory;
  if (!isDefault)
    directory.erase(0, 24);

  return true;
}

void CGUIDialogProfileSettings::updateProfileName()
{
  SET_CONTROL_LABEL(CONTROL_PROFILE_NAME, m_name);
}

void CGUIDialogProfileSettings::updateProfileDirectory()
{
  SET_CONTROL_LABEL(CONTROL_PROFILE_DIRECTORY, m_directory);
}
