/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Addon.h"

#include "AddonManager.h"
#include "RepositoryUpdater.h"
#include "ServiceBroker.h"
#include "addons/settings/AddonSettings.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "settings/Settings.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/XMLUtils.h"
#include "utils/log.h"

#include <algorithm>
#include <ostream>
#include <string.h>
#include <utility>
#include <vector>

#ifdef HAS_PYTHON
#include "interfaces/python/XBPython.h"
#endif

using XFILE::CDirectory;
using XFILE::CFile;

namespace ADDON
{

CAddon::CAddon(const AddonInfoPtr& addonInfo, TYPE addonType)
  : m_addonInfo(addonInfo), m_type(addonType == ADDON_UNKNOWN ? addonInfo->MainType() : addonType)
{
}

/**
 * Settings Handling
 */

std::vector<AddonInstanceId> CAddon::GetKnownInstanceIds() const
{
  return m_addonInfo->GetKnownInstanceIds();
}

bool CAddon::SupportsMultipleInstances() const
{
  return m_addonInfo->SupportsMultipleInstances();
}

AddonInstanceSupport CAddon::InstanceUseType() const
{
  return m_addonInfo->InstanceUseType();
}

bool CAddon::SupportsInstanceSettings() const
{
  return m_addonInfo->SupportsInstanceSettings();
}

bool CAddon::DeleteInstanceSettings(AddonInstanceId instance)
{
  if (instance == ADDON_SETTINGS_ID)
    return false;

  const auto itr = m_settings.find(instance);
  if (itr == m_settings.end())
    return false;

  if (CFile::Exists(itr->second.m_userSettingsPath))
    CFile::Delete(itr->second.m_userSettingsPath);

  ResetSettings(instance);

  return true;
}

bool CAddon::CanHaveAddonOrInstanceSettings()
{
  return HasSettings(ADDON_SETTINGS_ID) || SupportsInstanceSettings();
}

bool CAddon::HasSettings(AddonInstanceId id /* = ADDON_SETTINGS_ID */)
{
  return LoadSettings(false, true, id) && m_settings[id].m_addonSettings->HasSettings();
}

bool CAddon::SettingsInitialized(AddonInstanceId id /* = ADDON_SETTINGS_ID */) const
{
  const auto addonSettings = FindInstanceSettings(id);
  return addonSettings && addonSettings->IsInitialized();
}

bool CAddon::SettingsLoaded(AddonInstanceId id /* = ADDON_SETTINGS_ID */) const
{
  const auto addonSettings = FindInstanceSettings(id);
  return addonSettings && addonSettings->IsLoaded();
}

bool CAddon::LoadSettings(bool bForce,
                          bool loadUserSettings,
                          AddonInstanceId id /* = ADDON_SETTINGS_ID */)
{
  if (SettingsInitialized(id) && !bForce)
    return true;

  const auto itr = m_settings.find(id);
  if (itr != m_settings.end())
  {
    if (itr->second.m_loadSettingsFailed)
      return false;
  }
  else
  {
    InitSettings(id);
  }

  // assume loading settings fails
  m_settings[id].m_loadSettingsFailed = true;

  // reset the settings if we are forced to
  if (SettingsInitialized(id) && bForce)
    GetSettings(id)->Uninitialize();

  // load the settings definition XML file
  const auto addonSettingsDefinitionFile = m_settings[id].m_addonSettingsPath;
  CXBMCTinyXML addonSettingsDefinitionDoc;
  if (!addonSettingsDefinitionDoc.LoadFile(addonSettingsDefinitionFile))
  {
    if (CFile::Exists(addonSettingsDefinitionFile))
    {
      CLog::Log(LOGERROR, "CAddon[{}]: unable to load: {}, Line {}\n{}", ID(),
                addonSettingsDefinitionFile, addonSettingsDefinitionDoc.ErrorRow(),
                addonSettingsDefinitionDoc.ErrorDesc());
    }

    return false;
  }

  // initialize the settings definition
  if (!GetSettings(id)->Initialize(addonSettingsDefinitionDoc))
  {
    CLog::Log(LOGERROR, "CAddon[{}]: failed to initialize addon settings", ID());
    return false;
  }

  // loading settings didn't fail
  m_settings[id].m_loadSettingsFailed = false;

  // load user settings / values
  if (loadUserSettings)
    LoadUserSettings(id);

  return true;
}

bool CAddon::HasUserSettings(AddonInstanceId id /* = ADDON_SETTINGS_ID */)
{
  if (!LoadSettings(false, true, id))
    return false;

  return SettingsLoaded(id) && m_settings[id].m_hasUserSettings;
}

bool CAddon::ReloadSettings(AddonInstanceId id /* = ADDON_SETTINGS_ID */)
{
  return LoadSettings(true, true, id);
}

void CAddon::ResetSettings(AddonInstanceId id /* = ADDON_SETTINGS_ID */)
{
  m_settings.erase(id);
}

bool CAddon::LoadUserSettings(AddonInstanceId id /* = ADDON_SETTINGS_ID */)
{
  if (!SettingsInitialized(id) && !InitSettings(id))
    return false;

  CSettingsData& data = m_settings[id];

  data.m_hasUserSettings = false;

  // there are no user settings
  if (!CFile::Exists(data.m_userSettingsPath))
  {
    // mark the settings as loaded
    GetSettings(id)->SetLoaded();
    return true;
  }

  CXBMCTinyXML doc;
  if (!doc.LoadFile(data.m_userSettingsPath))
  {
    CLog::Log(LOGERROR, "CAddon[{}]: failed to load addon settings from {}", ID(),
              data.m_userSettingsPath);
    return false;
  }

  return SettingsFromXML(doc, false, id);
}

bool CAddon::HasSettingsToSave(AddonInstanceId id /* = ADDON_SETTINGS_ID */) const
{
  return SettingsLoaded(id);
}

void CAddon::SaveSettings(AddonInstanceId id /* = ADDON_SETTINGS_ID */)
{
  if (!HasSettingsToSave(id))
    return; // no settings to save

  CSettingsData& data = m_settings[id];

  // break down the path into directories
  const std::string strAddon = URIUtils::GetDirectory(data.m_userSettingsPath);
  const std::string strRoot = URIUtils::GetDirectory(strAddon);

  // create the individual folders
  if (!CDirectory::Exists(strRoot))
    CDirectory::Create(strRoot);
  if (!CDirectory::Exists(strAddon))
    CDirectory::Create(strAddon);

  // create the XML file
  CXBMCTinyXML doc;
  if (SettingsToXML(doc, id))
    doc.SaveFile(data.m_userSettingsPath);

  data.m_hasUserSettings = true;

  //push the settings changes to the running addon instance
  CServiceBroker::GetAddonMgr().ReloadSettings(ID(), id);
#ifdef HAS_PYTHON
  CServiceBroker::GetXBPython().OnSettingsChanged(ID());
#endif
}

std::string CAddon::GetSetting(const std::string& key, AddonInstanceId id)
{
  if (key.empty() || !LoadSettings(false, true, id))
    return ""; // no settings available

  auto setting = m_settings[id].m_addonSettings->GetSetting(key);
  if (setting != nullptr)
    return setting->ToString();

  return "";
}

template<class TSetting>
bool GetSettingValue(CAddon& addon,
                     AddonInstanceId instanceId,
                     const std::string& key,
                     typename TSetting::Value& value)
{
  if (key.empty() || !addon.HasSettings(instanceId))
    return false;

  auto setting = addon.GetSettings(instanceId)->GetSetting(key);
  if (setting == nullptr || setting->GetType() != TSetting::Type())
    return false;

  value = std::static_pointer_cast<TSetting>(setting)->GetValue();
  return true;
}

bool CAddon::GetSettingBool(const std::string& key,
                            bool& value,
                            AddonInstanceId id /* = ADDON_SETTINGS_ID */)
{
  return GetSettingValue<CSettingBool>(*this, id, key, value);
}

bool CAddon::GetSettingInt(const std::string& key,
                           int& value,
                           AddonInstanceId id /* = ADDON_SETTINGS_ID */)
{
  return GetSettingValue<CSettingInt>(*this, id, key, value);
}

bool CAddon::GetSettingNumber(const std::string& key,
                              double& value,
                              AddonInstanceId id /* = ADDON_SETTINGS_ID */)
{
  return GetSettingValue<CSettingNumber>(*this, id, key, value);
}

bool CAddon::GetSettingString(const std::string& key,
                              std::string& value,
                              AddonInstanceId id /* = ADDON_SETTINGS_ID */)
{
  return GetSettingValue<CSettingString>(*this, id, key, value);
}

void CAddon::UpdateSetting(const std::string& key,
                           const std::string& value,
                           AddonInstanceId id /* = ADDON_SETTINGS_ID */)
{
  if (key.empty() || !LoadSettings(false, true, id))
    return;

  // try to get the setting
  auto setting = m_settings[id].m_addonSettings->GetSetting(key);

  // if the setting doesn't exist, try to add it
  if (setting == nullptr)
  {
    setting = m_settings[id].m_addonSettings->AddSetting(key, value);
    if (setting == nullptr)
    {
      CLog::Log(LOGERROR, "CAddon[{}]: failed to add undefined setting \"{}\"", ID(), key);
      return;
    }
  }

  setting->FromString(value);
}

template<class TSetting>
bool UpdateSettingValue(CAddon& addon,
                        AddonInstanceId instanceId,
                        const std::string& key,
                        typename TSetting::Value value)
{
  if (key.empty() || !addon.HasSettings(instanceId))
    return false;

  // try to get the setting
  auto setting = addon.GetSettings(instanceId)->GetSetting(key);

  // if the setting doesn't exist, try to add it
  if (setting == nullptr)
  {
    setting = addon.GetSettings(instanceId)->AddSetting(key, value);
    if (setting == nullptr)
    {
      CLog::Log(LOGERROR, "CAddon[{}]: failed to add undefined setting \"{}\"", addon.ID(), key);
      return false;
    }
  }

  if (setting->GetType() != TSetting::Type())
    return false;

  return std::static_pointer_cast<TSetting>(setting)->SetValue(value);
}

bool CAddon::UpdateSettingBool(const std::string& key,
                               bool value,
                               AddonInstanceId id /* = ADDON_SETTINGS_ID */)
{
  return UpdateSettingValue<CSettingBool>(*this, id, key, value);
}

bool CAddon::UpdateSettingInt(const std::string& key,
                              int value,
                              AddonInstanceId id /* = ADDON_SETTINGS_ID */)
{
  return UpdateSettingValue<CSettingInt>(*this, id, key, value);
}

bool CAddon::UpdateSettingNumber(const std::string& key,
                                 double value,
                                 AddonInstanceId id /* = ADDON_SETTINGS_ID */)
{
  return UpdateSettingValue<CSettingNumber>(*this, id, key, value);
}

bool CAddon::UpdateSettingString(const std::string& key,
                                 const std::string& value,
                                 AddonInstanceId id /* = ADDON_SETTINGS_ID */)
{
  return UpdateSettingValue<CSettingString>(*this, id, key, value);
}

bool CAddon::SettingsFromXML(const CXBMCTinyXML& doc,
                             bool loadDefaults,
                             AddonInstanceId id /* = ADDON_SETTINGS_ID */)
{
  if (doc.RootElement() == nullptr)
    return false;

  // if the settings haven't been initialized yet, try it from the given XML
  if (!SettingsInitialized(id))
  {
    if (!GetSettings(id)->Initialize(doc))
    {
      CLog::Log(LOGERROR, "CAddon[{}]: failed to initialize addon settings", ID());
      return false;
    }
  }

  // reset all setting values to their default value
  if (loadDefaults)
    GetSettings(id)->SetDefaults();

  // try to load the setting's values from the given XML
  if (!GetSettings(id)->Load(doc))
  {
    CLog::Log(LOGERROR, "CAddon[{}]: failed to load user settings", ID());
    return false;
  }

  m_settings[id].m_hasUserSettings = true;

  return true;
}

bool CAddon::SettingsToXML(CXBMCTinyXML& doc, AddonInstanceId id /* = ADDON_SETTINGS_ID */) const
{
  if (!SettingsInitialized(id))
    return false;

  if (!m_settings[id].m_addonSettings->Save(doc))
  {
    CLog::Log(LOGERROR, "CAddon[{}]: failed to save addon settings", ID());
    return false;
  }

  return true;
}

bool CAddon::InitSettings(AddonInstanceId id)
{
  // initialize addon settings if necessary
  if (!FindInstanceSettings(id))
  {
    CSettingsData data;

    data.m_addonSettings =
        std::make_shared<CAddonSettings>(enable_shared_from_this::shared_from_this(), id);
    if (id == ADDON_SETTINGS_ID)
    {
      data.m_addonSettingsPath =
          URIUtils::AddFileToFolder(m_addonInfo->Path(), "resources", "settings.xml");
      data.m_userSettingsPath = URIUtils::AddFileToFolder(Profile(), "settings.xml");
    }
    else
    {
      data.m_addonSettingsPath =
          URIUtils::AddFileToFolder(m_addonInfo->Path(), "resources", "instance-settings.xml");
      data.m_userSettingsPath =
          URIUtils::AddFileToFolder(Profile(), StringUtils::Format("instance-settings-{}.xml", id));
    }

    m_settings[id] = std::move(data);
    return true;
  }

  return false;
}

std::shared_ptr<CAddonSettings> CAddon::FindInstanceSettings(AddonInstanceId id) const
{
  const auto itr = m_settings.find(id);
  if (itr == m_settings.end())
    return nullptr;

  return itr->second.m_addonSettings;
}

std::shared_ptr<CAddonSettings> CAddon::GetSettings(AddonInstanceId id /* = ADDON_SETTINGS_ID */)
{
  if (InitSettings(id))
    LoadSettings(false, true, id);

  return m_settings[id].m_addonSettings;
}

std::string CAddon::LibPath() const
{
  // Get library related to given type on construction
  std::string libName = m_addonInfo->Type(m_type)->LibName();
  if (libName.empty())
  {
    // If not present fallback to master library
    libName = m_addonInfo->LibName();
    if (libName.empty())
      return "";
  }
  return URIUtils::AddFileToFolder(m_addonInfo->Path(), libName);
}

AddonVersion CAddon::GetDependencyVersion(const std::string& dependencyID) const
{
  return m_addonInfo->DependencyVersion(dependencyID);
}

void OnPreInstall(const AddonPtr& addon)
{
  //Fallback to the pre-install callback in the addon.
  //! @bug If primary extension point have changed we're calling the wrong method.
  addon->OnPreInstall();
}

void OnPostInstall(const AddonPtr& addon, bool update, bool modal)
{
  addon->OnPostInstall(update, modal);
}

void OnPreUnInstall(const AddonPtr& addon)
{
  addon->OnPreUnInstall();
}

void OnPostUnInstall(const AddonPtr& addon)
{
  addon->OnPostUnInstall();
}

} // namespace ADDON
