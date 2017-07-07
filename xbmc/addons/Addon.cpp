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

#include "Addon.h"

#include <algorithm>
#include <string.h>
#include <ostream>
#include <utility>
#include <vector>

#include "AddonManager.h"
#include "addons/Service.h"
#include "addons/settings/AddonSettings.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "guilib/LocalizeStrings.h"
#include "RepositoryUpdater.h"
#include "settings/Settings.h"
#include "ServiceBroker.h"
#include "system.h"
#include "URL.h"
#include "Util.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "utils/XMLUtils.h"

#ifdef HAS_PYTHON
#include "interfaces/python/XBPython.h"
#endif
#if defined(TARGET_DARWIN)
#include "../platform/darwin/OSXGNUReplacements.h"
#endif
#ifdef TARGET_FREEBSD
#include "freebsd/FreeBSDGNUReplacements.h"
#endif

using XFILE::CDirectory;
using XFILE::CFile;

namespace ADDON
{

CAddon::CAddon(CAddonInfo addonInfo)
  : m_addonInfo(std::move(addonInfo))
  , m_userSettingsPath()
  , m_loadSettingsFailed(false)
  , m_hasUserSettings(false)
  , m_profilePath(StringUtils::Format("special://profile/addon_data/%s/", m_addonInfo.ID().c_str()))
  , m_settings(nullptr)
{
  m_userSettingsPath = URIUtils::AddFileToFolder(m_profilePath, "settings.xml");
}

/**
 * Settings Handling
 */
bool CAddon::HasSettings()
{
  return LoadSettings(false);
}

bool CAddon::SettingsInitialized() const
{
  return m_settings != nullptr && m_settings->IsInitialized();
}

bool CAddon::SettingsLoaded() const
{
  return m_settings != nullptr && m_settings->IsLoaded();
}

bool CAddon::LoadSettings(bool bForce, bool loadUserSettings /* = true */)
{
  if (SettingsInitialized() && !bForce)
    return true;

  if (m_loadSettingsFailed)
    return false;

  // assume loading settings fails
  m_loadSettingsFailed = true;

  // reset the settings if we are forced to
  if (SettingsInitialized() && bForce)
    GetSettings()->Uninitialize();

  // load the settings definition XML file
  auto addonSettingsDefinitionFile = URIUtils::AddFileToFolder(m_addonInfo.Path(), "resources", "settings.xml");
  CXBMCTinyXML addonSettingsDefinitionDoc;
  if (!addonSettingsDefinitionDoc.LoadFile(addonSettingsDefinitionFile))
  {
    if (CFile::Exists(addonSettingsDefinitionFile))
    {
      CLog::Log(LOGERROR, "CAddon[%s]: unable to load: %s, Line %d\n%s",
        ID().c_str(), addonSettingsDefinitionFile.c_str(), addonSettingsDefinitionDoc.ErrorRow(), addonSettingsDefinitionDoc.ErrorDesc());
    }

    return false;
  }

  // initialize the settings definition
  if (!GetSettings()->Initialize(addonSettingsDefinitionDoc))
  {
    CLog::Log(LOGERROR, "CAddon[%s]: failed to initialize addon settings", ID().c_str());
    return false;
  }

  // loading settings didn't fail
  m_loadSettingsFailed = false;

  // load user settings / values
  if (loadUserSettings)
    LoadUserSettings();

  return true;
}

bool CAddon::HasUserSettings()
{
  if (!LoadSettings(false))
    return false;

  return SettingsLoaded() && m_hasUserSettings;
}

bool CAddon::ReloadSettings()
{
  return LoadSettings(true);
}

bool CAddon::LoadUserSettings()
{
  if (!SettingsInitialized())
    return false;

  m_hasUserSettings = false;

  // there are no user settings
  if (!CFile::Exists(m_userSettingsPath))
  {
    // mark the settings as loaded
    GetSettings()->SetLoaded();
    return true;
  }

  CXBMCTinyXML doc;
  if (!doc.LoadFile(m_userSettingsPath))
  {
    CLog::Log(LOGERROR, "CAddon[%s]: failed to load addon settings from %s", ID().c_str(), m_userSettingsPath.c_str());
    return false;
  }

  return SettingsFromXML(doc);
}

bool CAddon::HasSettingsToSave() const
{
  return SettingsLoaded();
}

void CAddon::SaveSettings(void)
{
  if (!HasSettingsToSave())
    return; // no settings to save

  // break down the path into directories
  std::string strAddon = URIUtils::GetDirectory(m_userSettingsPath);
  URIUtils::RemoveSlashAtEnd(strAddon);
  std::string strRoot = URIUtils::GetDirectory(strAddon);
  URIUtils::RemoveSlashAtEnd(strRoot);

  // create the individual folders
  if (!CDirectory::Exists(strRoot))
    CDirectory::Create(strRoot);
  if (!CDirectory::Exists(strAddon))
    CDirectory::Create(strAddon);

  // create the XML file
  CXBMCTinyXML doc;
  if (SettingsToXML(doc))
    doc.SaveFile(m_userSettingsPath);

  m_hasUserSettings = true;
  
  //push the settings changes to the running addon instance
  CAddonMgr::GetInstance().ReloadSettings(ID());
#ifdef HAS_PYTHON
  g_pythonParser.OnSettingsChanged(ID());
#endif
}

std::string CAddon::GetSetting(const std::string& key)
{
  if (key.empty() || !LoadSettings(false))
    return ""; // no settings available

  auto setting = m_settings->GetSetting(key);
  if (setting != nullptr)
    return setting->ToString();

  return "";
}

template<class TSetting>
bool GetSettingValue(CAddon& addon, const std::string& key, typename TSetting::Value& value)
{
  if (key.empty() || !addon.HasSettings())
    return false;

  auto setting = addon.GetSettings()->GetSetting(key);
  if (setting == nullptr || setting->GetType() != TSetting::Type())
    return false;

  value = std::static_pointer_cast<TSetting>(setting)->GetValue();
  return true;
}

bool CAddon::GetSettingBool(const std::string& key, bool& value)
{
  return GetSettingValue<CSettingBool>(*this, key, value);
}

bool CAddon::GetSettingInt(const std::string& key, int& value)
{
  return GetSettingValue<CSettingInt>(*this, key, value);
}

bool CAddon::GetSettingNumber(const std::string& key, double& value)
{
  return GetSettingValue<CSettingNumber>(*this, key, value);
}

bool CAddon::GetSettingString(const std::string& key, std::string& value)
{
  return GetSettingValue<CSettingString>(*this, key, value);
}

void CAddon::UpdateSetting(const std::string& key, const std::string& value)
{
  if (key.empty() || !LoadSettings(false))
    return;

  // try to get the setting
  auto setting = m_settings->GetSetting(key);

  // if the setting doesn't exist, try to add it
  if (setting == nullptr)
  {
    setting = m_settings->AddSetting(key, value);
    if (setting == nullptr)
    {
      CLog::Log(LOGERROR, "CAddon[%s]: failed to add undefined setting \"%s\"", ID().c_str(), key.c_str());
      return;
    }
  }

  setting->FromString(value);
}

template<class TSetting>
bool UpdateSettingValue(CAddon& addon, const std::string& key, typename TSetting::Value value)
{
  if (key.empty() || !addon.HasSettings())
    return false;

  // try to get the setting
  auto setting = addon.GetSettings()->GetSetting(key);

  // if the setting doesn't exist, try to add it
  if (setting == nullptr)
  {
    setting = addon.GetSettings()->AddSetting(key, value);
    if (setting == nullptr)
    {
      CLog::Log(LOGERROR, "CAddon[%s]: failed to add undefined setting \"%s\"", addon.ID().c_str(), key.c_str());
      return false;
    }
  }

  if (setting->GetType() != TSetting::Type())
    return false;

  return std::static_pointer_cast<TSetting>(setting)->SetValue(value);
}

bool CAddon::UpdateSettingBool(const std::string& key, bool value)
{
  return UpdateSettingValue<CSettingBool>(*this, key, value);
}

bool CAddon::UpdateSettingInt(const std::string& key, int value)
{
  return UpdateSettingValue<CSettingInt>(*this, key, value);
}

bool CAddon::UpdateSettingNumber(const std::string& key, double value)
{
  return UpdateSettingValue<CSettingNumber>(*this, key, value);
}

bool CAddon::UpdateSettingString(const std::string& key, const std::string& value)
{
  return UpdateSettingValue<CSettingString>(*this, key, value);
}

bool CAddon::SettingsFromXML(const CXBMCTinyXML &doc, bool loadDefaults /* = false */)
{
  if (doc.RootElement() == nullptr)
    return false;

  // if the settings haven't been initialized yet, try it from the given XML
  if (!SettingsInitialized())
  {
    if (!GetSettings()->Initialize(doc))
    {
      CLog::Log(LOGERROR, "CAddon[%s]: failed to initialize addon settings", ID().c_str());
      return false;
    }
  }

  // reset all setting values to their default value
  if (loadDefaults)
    GetSettings()->SetDefaults();

  // try to load the setting's values from the given XML
  if (!GetSettings()->Load(doc))
  {
    CLog::Log(LOGERROR, "CAddon[%s]: failed to load user settings", ID().c_str());
    return false;
  }

  m_hasUserSettings = true;

  return true;
}

bool CAddon::SettingsToXML(CXBMCTinyXML &doc) const
{
  if (!SettingsInitialized())
    return false;

  if (!m_settings->Save(doc))
  {
    CLog::Log(LOGERROR, "CAddon[%s]: failed to save addon settings", ID().c_str());
    return false;
  }

  return true;
}

CAddonSettings* CAddon::GetSettings() const
{
  // initialize addon settings if necessary
  if (m_settings == nullptr)
    m_settings = std::make_shared<CAddonSettings>(enable_shared_from_this::shared_from_this());

  return m_settings.get();
}

std::string CAddon::LibPath() const
{
  if (m_addonInfo.LibName().empty())
    return "";
  return URIUtils::AddFileToFolder(m_addonInfo.Path(), m_addonInfo.LibName());
}

AddonVersion CAddon::GetDependencyVersion(const std::string &dependencyID) const
{
  const ADDON::ADDONDEPS &deps = GetDeps();
  ADDONDEPS::const_iterator it = deps.find(dependencyID);
  if (it != deps.end())
    return it->second.first;
  return AddonVersion("0.0.0");
}

void OnEnabled(const std::string& id)
{
  // If the addon is a special, call enabled handler
  AddonPtr addon;
  if (CAddonMgr::GetInstance().GetAddon(id, addon, ADDON_PVRDLL))
    return addon->OnEnabled();
}

void OnDisabled(const std::string& id)
{

  AddonPtr addon;
  if (CAddonMgr::GetInstance().GetAddon(id, addon, ADDON_PVRDLL, false))
    return addon->OnDisabled();
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

} /* namespace ADDON */

