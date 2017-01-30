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

#include <string.h>
#include <ostream>
#include <utility>
#include <vector>

#include "AddonManager.h"
#include "addons/Service.h"
#include "ContextMenuManager.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
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

CAddon::CAddon(AddonInfoPtr props)
  : m_addonInfo(props)
{
  m_profilePath = StringUtils::Format("special://profile/addon_data/%s/", ID().c_str());
  m_userSettingsPath = URIUtils::AddFileToFolder(m_profilePath, "settings.xml");
  m_hasSettings = true;
  m_settingsLoaded = false;
  m_userSettingsLoaded = false;
}

bool CAddon::MeetsVersion(const AddonVersion &version) const
{
  return m_addonInfo->m_minversion <= version && version <= m_addonInfo->m_version;
}

/**
 * Settings Handling
 */
bool CAddon::HasSettings()
{
  return LoadSettings();
}

bool CAddon::LoadSettings(bool bForce /* = false*/)
{
  if (m_settingsLoaded && !bForce)
    return true;
  if (!m_hasSettings)
    return false;
  std::string addonFileName = URIUtils::AddFileToFolder(m_addonInfo->m_path, "resources", "settings.xml");

  if (!m_addonXmlDoc.LoadFile(addonFileName))
  {
    if (CFile::Exists(addonFileName))
      CLog::Log(LOGERROR, "Unable to load: %s, Line %d\n%s", addonFileName.c_str(), m_addonXmlDoc.ErrorRow(), m_addonXmlDoc.ErrorDesc());
    m_hasSettings = false;
    return false;
  }

  // Make sure that the addon XML has the settings element
  TiXmlElement *setting = m_addonXmlDoc.RootElement();
  if (!setting || strcmpi(setting->Value(), "settings") != 0)
  {
    CLog::Log(LOGERROR, "Error loading Settings %s: cannot find root element 'settings'", addonFileName.c_str());
    return false;
  }
  SettingsFromXML(m_addonXmlDoc, true);
  LoadUserSettings();
  m_settingsLoaded = true;
  return true;
}

bool CAddon::HasUserSettings()
{
  if (!LoadSettings())
    return false;

  return m_userSettingsLoaded;
}

bool CAddon::ReloadSettings()
{
  return LoadSettings(true);
}

bool CAddon::LoadUserSettings()
{
  m_userSettingsLoaded = false;
  CXBMCTinyXML doc;
  if (doc.LoadFile(m_userSettingsPath))
    m_userSettingsLoaded = SettingsFromXML(doc);
  return m_userSettingsLoaded;
}

bool CAddon::HasSettingsToSave() const
{
  return !m_settings.empty();
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
  SettingsToXML(doc);
  doc.SaveFile(m_userSettingsPath);
  m_userSettingsLoaded = true;
  
  CAddonMgr::GetInstance().ReloadSettings(ID());//push the settings changes to the running addon instance
#ifdef HAS_PYTHON
  g_pythonParser.OnSettingsChanged(ID());
#endif
}

std::string CAddon::GetSetting(const std::string& key)
{
  if (!LoadSettings())
    return ""; // no settings available

  std::map<std::string, std::string>::const_iterator i = m_settings.find(key);
  if (i != m_settings.end())
    return i->second;
  return "";
}

void CAddon::UpdateSetting(const std::string& key, const std::string& value)
{
  LoadSettings();
  if (key.empty()) return;
  m_settings[key] = value;
}

void CAddon::UpdateSettings(std::map<std::string, std::string>& settings)
{
  LoadSettings();
  m_settings = settings;
}

bool CAddon::SettingsFromXML(const CXBMCTinyXML &doc, bool loadDefaults /*=false */)
{
  if (!doc.RootElement())
    return false;

  if (loadDefaults)
    m_settings.clear();

  const TiXmlElement* category = doc.RootElement()->FirstChildElement("category");
  if (!category)
    category = doc.RootElement();

  bool foundSetting = false;
  while (category)
  {
    const TiXmlElement *setting = category->FirstChildElement("setting");
    while (setting)
    {
      const char *id = setting->Attribute("id");
      const char *value = setting->Attribute(loadDefaults ? "default" : "value");
      if (id && value)
      {
        m_settings[id] = value;
        foundSetting = true;
      }
      setting = setting->NextSiblingElement("setting");
    }
    category = category->NextSiblingElement("category");
  }
  return foundSetting;
}

void CAddon::SettingsToXML(CXBMCTinyXML &doc) const
{
  TiXmlElement node("settings");
  doc.InsertEndChild(node);
  for (std::map<std::string, std::string>::const_iterator i = m_settings.begin(); i != m_settings.end(); ++i)
  {
    TiXmlElement nodeSetting("setting");
    nodeSetting.SetAttribute("id", i->first.c_str());
    nodeSetting.SetAttribute("value", i->second.c_str());
    doc.RootElement()->InsertEndChild(nodeSetting);
  }
  doc.SaveFile(m_userSettingsPath);
}

TiXmlElement* CAddon::GetSettingsXML()
{
  return m_addonXmlDoc.RootElement();
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
  if (CAddonMgr::GetInstance().GetAddon(id, addon, ADDON_PVRDLL) ||
      CAddonMgr::GetInstance().GetAddon(id, addon, ADDON_ADSPDLL))
    return addon->OnEnabled();

  if (CAddonMgr::GetInstance().ServicesHasStarted())
  {
    if (CAddonMgr::GetInstance().GetAddon(id, addon, ADDON_SERVICE))
      std::static_pointer_cast<CService>(addon)->Start();
  }

  if (CAddonMgr::GetInstance().GetAddon(id, addon, ADDON_REPOSITORY))
    CRepositoryUpdater::GetInstance().ScheduleUpdate(); //notify updater there is a new addon
}

void OnDisabled(const std::string& id)
{

  AddonPtr addon;
  if (CAddonMgr::GetInstance().GetAddon(id, addon, ADDON_PVRDLL) ||
      CAddonMgr::GetInstance().GetAddon(id, addon, ADDON_ADSPDLL))
    return addon->OnDisabled();

  if (CAddonMgr::GetInstance().ServicesHasStarted())
  {
    if (CAddonMgr::GetInstance().GetAddon(id, addon, ADDON_SERVICE))
      std::static_pointer_cast<CService>(addon)->Stop();
  }

  if (CAddonMgr::GetInstance().GetAddon(id, addon, ADDON_CONTEXT_ITEM))
    CContextMenuManager::GetInstance().Unload(*std::static_pointer_cast<CContextMenuAddon>(addon));
}

void OnPreInstall(const AddonPtr& addon)
{
  //Before installing we need to stop/unregister any local addon
  //that have this id, regardless of what the 'new' addon is.
  AddonPtr localAddon;

  if (CAddonMgr::GetInstance().ServicesHasStarted())
  {
    if (CAddonMgr::GetInstance().GetAddon(addon->ID(), localAddon, ADDON_SERVICE))
      std::static_pointer_cast<CService>(localAddon)->Stop();
  }

  if (CAddonMgr::GetInstance().GetAddon(addon->ID(), localAddon, ADDON_CONTEXT_ITEM))
    CContextMenuManager::GetInstance().Unload(*std::static_pointer_cast<CContextMenuAddon>(localAddon));

  //Fallback to the pre-install callback in the addon.
  //! @bug If primary extension point have changed we're calling the wrong method.
  addon->OnPreInstall();
}

void OnPostInstall(const AddonPtr& addon, bool update, bool modal)
{
  AddonPtr localAddon;
  if (CAddonMgr::GetInstance().ServicesHasStarted())
  {
    if (CAddonMgr::GetInstance().GetAddon(addon->ID(), localAddon, ADDON_SERVICE))
      std::static_pointer_cast<CService>(localAddon)->Start();
  }

  if (CAddonMgr::GetInstance().GetAddon(addon->ID(), localAddon, ADDON_REPOSITORY))
    CRepositoryUpdater::GetInstance().ScheduleUpdate(); //notify updater there is a new addon or version

  addon->OnPostInstall(update, modal);
}

void OnPreUnInstall(const AddonPtr& addon)
{
  AddonPtr localAddon;

  if (CAddonMgr::GetInstance().ServicesHasStarted())
  {
    if (CAddonMgr::GetInstance().GetAddon(addon->ID(), localAddon, ADDON_SERVICE))
      std::static_pointer_cast<CService>(localAddon)->Stop();
  }

  if (CAddonMgr::GetInstance().GetAddon(addon->ID(), localAddon, ADDON_CONTEXT_ITEM))
    CContextMenuManager::GetInstance().Unload(*std::static_pointer_cast<CContextMenuAddon>(localAddon));

  addon->OnPreUnInstall();
}

void OnPostUnInstall(const AddonPtr& addon)
{
  addon->OnPostUnInstall();
}

} /* namespace ADDON */

