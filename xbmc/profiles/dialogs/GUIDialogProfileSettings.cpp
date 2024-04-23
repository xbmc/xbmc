/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIDialogProfileSettings.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "GUIPassword.h"
#include "ServiceBroker.h"
#include "Util.h"
#include "dialogs/GUIDialogFileBrowser.h"
#include "dialogs/GUIDialogYesNo.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIKeyboardFactory.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "profiles/ProfileManager.h"
#include "profiles/dialogs/GUIDialogLockSettings.h"
#include "settings/SettingsComponent.h"
#include "settings/lib/Setting.h"
#include "settings/windows/GUIControlSettings.h"
#include "storage/MediaManager.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "utils/log.h"

#include <cassert>
#include <utility>

#define SETTING_PROFILE_NAME          "profile.name"
#define SETTING_PROFILE_IMAGE         "profile.image"
#define SETTING_PROFILE_DIRECTORY     "profile.directory"
#define SETTING_PROFILE_LOCKS         "profile.locks"
#define SETTING_PROFILE_MEDIA         "profile.media"
#define SETTING_PROFILE_MEDIA_SOURCES "profile.mediasources"

CGUIDialogProfileSettings::CGUIDialogProfileSettings()
    : CGUIDialogSettingsManualBase(WINDOW_DIALOG_PROFILE_SETTINGS, "DialogSettings.xml")
{ }

CGUIDialogProfileSettings::~CGUIDialogProfileSettings() = default;

bool CGUIDialogProfileSettings::ShowForProfile(unsigned int iProfile, bool firstLogin)
{
  const std::shared_ptr<CProfileManager> profileManager = CServiceBroker::GetSettingsComponent()->GetProfileManager();

  if (firstLogin && iProfile > profileManager->GetNumberOfProfiles())
    return false;

  CGUIDialogProfileSettings *dialog = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogProfileSettings>(WINDOW_DIALOG_PROFILE_SETTINGS);
  if (dialog == NULL)
    return false;

  dialog->m_needsSaving = false;
  dialog->m_isDefault = iProfile == 0;
  dialog->m_showDetails = !firstLogin;

  const CProfile *profile = profileManager->GetProfile(iProfile);
  if (profile == NULL)
  {
    dialog->m_name.clear();
    dialog->m_dbMode = 2;
    dialog->m_sourcesMode = 2;
    dialog->m_locks = CProfile::CLock();

    bool bLock = profileManager->GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser;
    dialog->m_locks.addonManager = bLock;
    dialog->m_locks.settings = (bLock) ? LOCK_LEVEL::ALL : LOCK_LEVEL::NONE;
    dialog->m_locks.files = bLock;

    dialog->m_directory.clear();
    dialog->m_thumb.clear();

    // prompt for a name
    std::string profileName;
    if (!CGUIKeyboardFactory::ShowAndGetInput(profileName, CVariant{g_localizeStrings.Get(20093)}, false) || profileName.empty())
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
      if (!URIUtils::PathHasParent(userDir, defaultDir)) // user chose a different folder
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

  dialog->Open();
  if (dialog->m_needsSaving)
  {
    if (iProfile >= profileManager->GetNumberOfProfiles())
    {
      if (dialog->m_name.empty() || dialog->m_directory.empty())
        return false;

      /*std::string strLabel;
      strLabel.Format(g_localizeStrings.Get(20047),dialog->m_strName);
      if (!CGUIDialogYesNo::ShowAndGetInput(20058, strLabel, dialog->m_strDirectory, ""))
      {
        CDirectory::Remove(URIUtils::AddFileToFolder(profileManager.GetUserDataFolder(), dialog->m_strDirectory));
        return false;
      }*/

      // check for old profile settings
      CProfile profile(dialog->m_directory, dialog->m_name, profileManager->GetNextProfileId());
      profileManager->AddProfile(profile);
      bool exists = XFILE::CFile::Exists(URIUtils::AddFileToFolder("special://masterprofile/", dialog->m_directory, "guisettings.xml"));

      if (exists && !CGUIDialogYesNo::ShowAndGetInput(CVariant{20058}, CVariant{20104}))
        exists = false;

      if (!exists)
      {
        // copy masterprofile guisettings to new profile guisettings
        // If the user selects 'start fresh', do nothing as a fresh
        // guisettings.xml will be created on first profile use.
        if (CGUIDialogYesNo::ShowAndGetInput(CVariant{20058}, CVariant{20048}, CVariant{""}, CVariant{""}, CVariant{20044}, CVariant{20064}))
        {
          XFILE::CFile::Copy(URIUtils::AddFileToFolder("special://masterprofile/", "guisettings.xml"),
                              URIUtils::AddFileToFolder("special://masterprofile/", dialog->m_directory, "guisettings.xml"));
        }
      }

      exists = XFILE::CFile::Exists(URIUtils::AddFileToFolder("special://masterprofile/", dialog->m_directory, "sources.xml"));
      if (exists && !CGUIDialogYesNo::ShowAndGetInput(CVariant{20058}, CVariant{20106}))
        exists = false;

      if (!exists)
      {
        if ((dialog->m_sourcesMode & 2) == 2)
          // prompt user to copy masterprofile's sources.xml file
          // If 'start fresh' (no) is selected, do nothing.
          if (CGUIDialogYesNo::ShowAndGetInput(CVariant{20058}, CVariant{20071}, CVariant{""}, CVariant{""}, CVariant{20044}, CVariant{20064}))
          {
            XFILE::CFile::Copy(URIUtils::AddFileToFolder("special://masterprofile/", "sources.xml"),
                                URIUtils::AddFileToFolder("special://masterprofile/", dialog->m_directory, "sources.xml"));
          }
      }
    }

    /*if (!dialog->m_bIsNewUser)
      if (!CGUIDialogYesNo::ShowAndGetInput(20067, 20103))
        return false;*/

    CProfile *profile = profileManager->GetProfile(iProfile);
    assert(profile);
    profile->setName(dialog->m_name);
    profile->setDirectory(dialog->m_directory);
    profile->setThumb(dialog->m_thumb);
    profile->setWriteDatabases(!((dialog->m_dbMode & 1) == 1));
    profile->setWriteSources(!((dialog->m_sourcesMode & 1) == 1));
    profile->setDatabases((dialog->m_dbMode & 2) == 2);
    profile->setSources((dialog->m_sourcesMode & 2) == 2);
    profile->SetLocks(dialog->m_locks);
    profileManager->Save();

    return true;
  }

  return dialog->m_needsSaving;
}

void CGUIDialogProfileSettings::OnWindowLoaded()
{
  CGUIDialogSettingsManualBase::OnWindowLoaded();
}

void CGUIDialogProfileSettings::OnSettingChanged(const std::shared_ptr<const CSetting>& setting)
{
  if (setting == NULL)
    return;

  CGUIDialogSettingsManualBase::OnSettingChanged(setting);

  const std::string &settingId = setting->GetId();
  if (settingId == SETTING_PROFILE_NAME)
  {
    m_name = std::static_pointer_cast<const CSettingString>(setting)->GetValue();
  }
  else if (settingId == SETTING_PROFILE_MEDIA)
    m_dbMode = std::static_pointer_cast<const CSettingInt>(setting)->GetValue();
  else if (settingId == SETTING_PROFILE_MEDIA_SOURCES)
    m_sourcesMode = std::static_pointer_cast<const CSettingInt>(setting)->GetValue();

  m_needsSaving = true;
}

void CGUIDialogProfileSettings::OnSettingAction(const std::shared_ptr<const CSetting>& setting)
{
  if (setting == NULL)
    return;

  CGUIDialogSettingsManualBase::OnSettingAction(setting);

  const std::string &settingId = setting->GetId();
  if (settingId == SETTING_PROFILE_IMAGE)
  {
    VECSOURCES shares;
    CServiceBroker::GetMediaManager().GetLocalDrives(shares);

    CFileItemList items;
    if (!m_thumb.empty())
    {
      CFileItemPtr item(new CFileItem("thumb://Current", false));
      item->SetArt("thumb", m_thumb);
      item->SetLabel(g_localizeStrings.Get(20016));
      items.Add(item);
    }

    CFileItemPtr item(new CFileItem("thumb://None", false));
    item->SetArt("thumb", "DefaultUser.png");
    item->SetLabel(g_localizeStrings.Get(20018));
    items.Add(item);

    std::string thumb;
    if (CGUIDialogFileBrowser::ShowAndGetImage(items, shares, g_localizeStrings.Get(1030), thumb) &&
        !StringUtils::EqualsNoCase(thumb, "thumb://Current"))
    {
      m_needsSaving = true;
      m_thumb = StringUtils::EqualsNoCase(thumb, "thumb://None") ? "" : thumb;

      UpdateProfileImage();
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
      const std::shared_ptr<CProfileManager> profileManager = CServiceBroker::GetSettingsComponent()->GetProfileManager();

      if (profileManager->GetMasterProfile().getLockMode() == LOCK_MODE_EVERYONE && !m_isDefault)
      {
        if (CGUIDialogYesNo::ShowAndGetInput(CVariant{20066}, CVariant{20118}))
          g_passwordManager.SetMasterLockMode(false);
        if (profileManager->GetMasterProfile().getLockMode() == LOCK_MODE_EVERYONE)
          return;
      }
      if (CGUIDialogLockSettings::ShowAndGetLock(m_locks, m_isDefault ? 12360 : 20068,
              profileManager->GetMasterProfile().getLockMode() == LOCK_MODE_EVERYONE || m_isDefault))
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

  SET_CONTROL_HIDDEN(CONTROL_SETTINGS_CUSTOM_BUTTON);
  SET_CONTROL_LABEL(CONTROL_SETTINGS_OKAY_BUTTON, 186);
  SET_CONTROL_LABEL(CONTROL_SETTINGS_CANCEL_BUTTON, 222);

  // set the profile image and directory
  UpdateProfileImage();
  updateProfileDirectory();
}

void CGUIDialogProfileSettings::InitializeSettings()
{
  CGUIDialogSettingsManualBase::InitializeSettings();

  const std::shared_ptr<CSettingCategory> category = AddCategory("profilesettings", -1);
  if (category == NULL)
  {
    CLog::Log(LOGERROR, "CGUIDialogProfileSettings: unable to setup settings");
    return;
  }

  const std::shared_ptr<CSettingGroup> group = AddGroup(category);
  if (group == NULL)
  {
    CLog::Log(LOGERROR, "CGUIDialogProfileSettings: unable to setup settings");
    return;
  }

  const std::shared_ptr<CProfileManager> profileManager = CServiceBroker::GetSettingsComponent()->GetProfileManager();

  AddEdit(group, SETTING_PROFILE_NAME, 20093, SettingLevel::Basic, m_name);
  AddButton(group, SETTING_PROFILE_IMAGE, 20065, SettingLevel::Basic);

  if (!m_isDefault && m_showDetails)
    AddButton(group, SETTING_PROFILE_DIRECTORY, 20070, SettingLevel::Basic);

  if (m_showDetails ||
     (m_locks.mode == LOCK_MODE_EVERYONE && profileManager->GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE))
    AddButton(group, SETTING_PROFILE_LOCKS, 20066, SettingLevel::Basic);

  if (!m_isDefault && m_showDetails)
  {
    const std::shared_ptr<CSettingGroup> groupMedia = AddGroup(category);
    if (groupMedia == NULL)
    {
      CLog::Log(LOGERROR, "CGUIDialogProfileSettings: unable to setup settings");
      return;
    }

    TranslatableIntegerSettingOptions entries;
    entries.emplace_back(20062, 0);
    entries.emplace_back(20063, 1);
    entries.emplace_back(20061, 2);
    if (profileManager->GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE)
      entries.emplace_back(20107, 3);

    AddSpinner(groupMedia, SETTING_PROFILE_MEDIA, 20060, SettingLevel::Basic, m_dbMode, entries);
    AddSpinner(groupMedia, SETTING_PROFILE_MEDIA_SOURCES, 20094, SettingLevel::Basic, m_sourcesMode, entries);
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

void CGUIDialogProfileSettings::UpdateProfileImage()
{
  BaseSettingControlPtr settingControl = GetSettingControl(SETTING_PROFILE_IMAGE);
  if (settingControl != NULL && settingControl->GetControl() != NULL)
    SET_CONTROL_LABEL2(settingControl->GetID(), URIUtils::GetFileName(m_thumb));
}

void CGUIDialogProfileSettings::updateProfileDirectory()
{
  BaseSettingControlPtr settingControl = GetSettingControl(SETTING_PROFILE_DIRECTORY);
  if (settingControl != NULL && settingControl->GetControl() != NULL)
    SET_CONTROL_LABEL2(settingControl->GetID(), m_directory);
}
