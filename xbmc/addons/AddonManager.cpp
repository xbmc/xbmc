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

#include "AddonManager.h"

#include <algorithm>
#include <iterator>
#include <memory>
#include <utility>

#include "LangInfo.h"
#include "system.h"
#include "Util.h"
#include "ContextMenuManager.h"
#include "ServiceBroker.h"
#include "events/AddonManagementEvent.h"
#include "events/EventLog.h"
#include "filesystem/Directory.h"
#include "filesystem/SpecialProtocol.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "threads/SingleLock.h"
#include "utils/JobManager.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/XBMCTinyXML.h"
#include "utils/XMLUtils.h"

/* CAddon class included add-on's */
#include "addons/AudioDecoder.h"
#include "addons/AudioEncoder.h"
#include "addons/ContextMenuAddon.h"
#include "addons/GameResource.h"
#include "addons/ImageDecoder.h"
#include "addons/ImageResource.h"
#include "addons/LanguageResource.h"
#include "addons/Repository.h"
#include "addons/Scraper.h"
#include "addons/Service.h"
#include "addons/Skin.h"
#include "addons/UISoundsResource.h"
#include "addons/VFSEntry.h"
#include "addons/Webinterface.h"
#include "cores/AudioEngine/Engines/ActiveAE/AudioDSPAddons/ActiveAEDSP.h"
#include "games/addons/GameClient.h"
#include "games/controllers/Controller.h"
#include "peripherals/addons/PeripheralAddon.h"
#include "addons/PVRClient.h"

using namespace XFILE;

namespace ADDON
{

/**********************************************************
 * CAddonMgr
 *
 */

std::map<TYPE, IAddonMgrCallback*> CAddonMgr::m_managers;

CAddonMgr::CAddonMgr()
  : m_serviceSystemStarted(false)
{ }

CAddonMgr::~CAddonMgr()
{
  DeInit();
}

CAddonMgr &CAddonMgr::GetInstance()
{
  return CServiceBroker::GetAddonMgr();
}

bool CAddonMgr::Init()
{
  if (!LoadManifest(m_systemAddons, m_optionalAddons))
  {
    CLog::Log(LOGERROR, "ADDONS: Failed to read manifest");
    return false;
  }

  if (!m_database.Open())
    CLog::Log(LOGFATAL, "ADDONS: Failed to open database");

  FindAddons();

  //Ensure required add-ons are installed and enabled
  for (const auto& id : m_systemAddons)
  {
    if (!IsAddonInstalled(id))
    {
      CLog::Log(LOGFATAL, "ADDONS: required addon '%s' not installed or not enabled.", id.c_str());
      return false;
    }
  }

//   for (auto& repos : m_installedAddons[ADDON_REPOSITORY])
//     CLog::Log(LOGNOTICE, "ADDONS: Using repository %s", repos.first.c_str());

  return true;
}

void CAddonMgr::DeInit()
{
  m_database.Close();
  m_installedAddons.clear();
  m_enabledAddons.clear();
  m_systemAddons.clear();
  m_optionalAddons.clear();
  m_updateBlacklist.clear();
}

bool CAddonMgr::FindAddons()
{
  CSingleLock lock(m_critSection);

  AddonInfoList installedAddons;

  FindAddons(installedAddons, "special://xbmcbin/addons");
  FindAddons(installedAddons, "special://xbmc/addons");
  FindAddons(installedAddons, "special://home/addons");

  m_installedAddons = std::move(installedAddons);

  std::set<std::string> installed;
  for (auto& addonInfo : m_installedAddons)
  {
    installed.insert(addonInfo.second->ID());
  }
  m_database.SyncInstalled(installed, m_systemAddons, m_optionalAddons);

  // Reload caches
  std::set<std::string> tmp;
  m_database.GetEnabled(tmp);
  AddonInfoList enabledAddons;
  for (auto addonId : tmp)
  {
    const AddonInfoPtr info = GetInstalledAddonInfo(addonId);
    if (info == nullptr)
    {
      CLog::Log(LOGERROR, "ADDONS: Addon Id '%s' does not mach installed map", addonId.c_str());
      continue;
    }
    enabledAddons[addonId] = info;
  }
  m_enabledAddons = std::move(enabledAddons);

  tmp.clear();
  m_database.GetBlacklisted(tmp);
  m_updateBlacklist = std::move(tmp);

  m_events.Publish(AddonEvents::InstalledChanged());

  return true;
}

bool CAddonMgr::HasInstalledAddons(const TYPE &type)
{
  CSingleLock lock(m_critSection);
  for (auto info : m_installedAddons)
  {
    if (info.second->IsType(type))
      return true;
  }
  return false;
}

bool CAddonMgr::HasEnabledAddons(const TYPE &type)
{
  CSingleLock lock(m_critSection);
  for (auto info : m_enabledAddons)
  {
    if (info.second->IsType(type))
      return true;
  }
  return false;
}

bool CAddonMgr::IsAddonInstalled(const std::string& addonId, const TYPE &type/* = ADDON_UNKNOWN*/)
{
  CSingleLock lock(m_critSection);

  if (m_installedAddons.find(addonId) != m_installedAddons.end())
    return true;
  return false;
}

bool CAddonMgr::IsAddonEnabled(const std::string& addonId, const TYPE &type/* = ADDON_UNKNOWN*/)
{
  CSingleLock lock(m_critSection);

  if (m_enabledAddons.find(addonId) != m_enabledAddons.end())
    return true;

  return false;
}

AddonInfos CAddonMgr::GetAddonInfos(bool enabledOnly, const TYPE &type, bool useTimeData/* = false*/)
{
  AddonInfos infos;
  GetAddonInfos(infos, enabledOnly, type, useTimeData);
  return infos;
}

void CAddonMgr::GetAddonInfos(AddonInfos& addonInfos, bool enabledOnly, const TYPE &type, bool useTimeData/* = false*/)
{
  CSingleLock lock(m_critSection);

  AddonInfoList* addons;
  if (enabledOnly)
    addons = &m_enabledAddons;
  else
    addons = &m_installedAddons;

  for (auto info : *addons)
  {
    if (type == ADDON_UNKNOWN || info.second->IsType(type))
    {
      if (useTimeData)
        m_database.GetInstallData(info.second);
      addonInfos.push_back(info.second);
    }
  }
}

void CAddonMgr::GetDisabledAddonInfos(AddonInfos& addonInfos, const TYPE& type)
{
  CSingleLock lock(m_critSection);

  for (auto info : m_installedAddons)
  {
    if (type == ADDON_UNKNOWN || info.second->IsType(type))
    {
      if (!IsAddonEnabled(info.second->ID(), type))
        addonInfos.push_back(info.second);
    }
  }
}

void CAddonMgr::GetInstallableAddonInfos(AddonInfos& addonInfos, const TYPE &type)
{
  CSingleLock lock(m_critSection);

  // get all addons
  AddonInfos installable;
  if (!m_database.GetRepositoryContent(installable))
    return;

  // go through all addons and remove all that are already installed

  installable.erase(std::remove_if(installable.begin(), installable.end(),
    [this, type](const AddonInfoPtr& addon)
    {
      bool bErase = false;

      // check if the addon matches the provided addon type
      if (type != ADDON::ADDON_UNKNOWN && !addon->IsType(type))
        bErase = true;

      // can't install already installed addon
      if (IsAddonInstalled(addon->ID()))
        bErase = true;

      // can't install broken addons
      if (!addon->Broken().empty())
        bErase = true;

      return bErase;
    }), installable.end());

  addonInfos.insert(addonInfos.end(), installable.begin(), installable.end());
}

bool CAddonMgr::GetAddons(VECADDONS &addons, const TYPE &type, bool enabledOnly/* = true*/)
{
  CSingleLock lock(m_critSection);

  for (auto info : GetAddonInfos(enabledOnly, type))
  {
    AddonPtr addon = CreateAddon(info, type);
    if (addon)
    {
      // if the addon has a running instance, grab that
      AddonPtr runningAddon = addon->GetRunningInstance();
      if (runningAddon)
        addon = runningAddon;
      addons.emplace_back(std::move(addon));
    }
  }
  return addons.size() > 0;
}

bool CAddonMgr::GetAddon(const std::string &addonId, AddonPtr &addon, const TYPE &type/*=ADDON_UNKNOWN*/)
{
  CSingleLock lock(m_critSection);

  const AddonInfoPtr info = GetInstalledAddonInfo(addonId, type);
  if (info)
    addon = CreateAddon(info, type);

  if (addon)
  {
    // if the addon has a running instance, grab that
    AddonPtr runningAddon = addon->GetRunningInstance();
    if (runningAddon)
      addon = runningAddon;
  }

  return nullptr != addon.get();
}

/// @todo New parts to handle multi instance addon, still in todo
//@{
AddonDllPtr CAddonMgr::GetAddon(const std::string& addonId, const IAddonInstanceHandler* handler)
{
  if (handler == nullptr)
  {
    CLog::Log(LOGERROR, "ADDONS: GetAddon for Id '%s' called with empty instance handler", addonId.c_str());
    return nullptr;
  }

  CSingleLock lock(m_critSection);

  const auto& itr = m_installedAddons.find(addonId);
  if (itr == m_installedAddons.end())
  {
    CLog::Log(LOGERROR, "ADDONS: GetAddon for Id '%s' is not present", addonId.c_str());
    return nullptr;
  }

  AddonInfoPtr addonInfo = itr->second;

  // If no 'm_activeAddon' is defined create it new.
  if (addonInfo->m_activeAddon == nullptr)
    addonInfo->m_activeAddon = std::make_shared<CAddonDll>(addonInfo);

  // add the instance handler to the info to know used amount on addon
  addonInfo->m_activeAddonHandlers.insert(handler);

  return addonInfo->m_activeAddon;
}

AddonDllPtr CAddonMgr::GetAddon(const AddonInfoPtr& addonInfo, const IAddonInstanceHandler* handler)
{
  if (addonInfo == nullptr)
  {
    CLog::Log(LOGERROR, "ADDONS: GetAddon called with empty addon info handler");
    return nullptr;
  }

  if (handler == nullptr)
  {
    CLog::Log(LOGERROR, "ADDONS: GetAddon for Id '%s' called with empty instance handler", addonInfo->ID().c_str());
    return nullptr;
  }

  CSingleLock lock(m_critSection);

  // If no 'm_activeAddon' is defined create it new.
  if (addonInfo->m_activeAddon == nullptr)
    addonInfo->m_activeAddon = std::make_shared<CAddonDll>(addonInfo);

  // add the instance handler to the info to know used amount on addon
  addonInfo->m_activeAddonHandlers.insert(handler);

  return addonInfo->m_activeAddon;
}

void CAddonMgr::ReleaseAddon(AddonDllPtr& addon, const IAddonInstanceHandler* handler)
{
  if (addon == nullptr)
  {
    CLog::Log(LOGERROR, "ADDONS: ReleaseAddon called with empty addon handler");
    return;
  }

  if (handler == nullptr)
  {
    CLog::Log(LOGERROR, "ADDONS: ReleaseAddon for Id '%s' called with empty instance handler", addon->ID().c_str());
    return;
  }

  CSingleLock lock(m_critSection);

  AddonInfoPtr addonInfo = addon->AddonInfo();

  const auto& presentHandler = addonInfo->m_activeAddonHandlers.find(handler);
  if (presentHandler == addonInfo->m_activeAddonHandlers.end())
    return;

  addonInfo->m_activeAddonHandlers.erase(presentHandler);

  // if no handler is present anymore reset and delete the add-on class on informations
  if (addonInfo->m_activeAddonHandlers.empty())
  {
    addonInfo->m_activeAddon.reset();
  }

  addon.reset();
}
//@}

AddonDllPtr CAddonMgr::GetAddon(const TYPE &type, const std::string &id)
{
  AddonPtr addon;
  GetAddon(id, addon, type);
  return std::dynamic_pointer_cast<CAddonDll>(addon);
}

const AddonInfoPtr CAddonMgr::GetInstalledAddonInfo(const std::string& addonId, const TYPE &type/* = ADDON_UNKNOWN*/)
{
  const auto& addon = m_installedAddons.find(addonId);
  if (addon != m_installedAddons.end() && (type == ADDON_UNKNOWN || addon->second->IsType(type)))
    return addon->second;

  return nullptr;
}

bool CAddonMgr::HasAvailableUpdates()
{
  CSingleLock lock(m_critSection);

  AddonInfos updates;
  for (auto addonInfo : m_enabledAddons)
  {
    AddonInfoPtr remote;
    if (m_database.GetAddonInfo(addonInfo.first, remote) && remote->Version() > addonInfo.second->Version())
      return true;
  }
  return false;
}

AddonInfos CAddonMgr::GetAvailableUpdates()
{
  CSingleLock lock(m_critSection);

  AddonInfos updates;
  for (auto addonInfo : m_enabledAddons)
  {
    AddonInfoPtr remote;
    if (m_database.GetAddonInfo(addonInfo.first, remote) && remote->Version() > addonInfo.second->Version())
      updates.emplace_back(std::move(remote));
  }

  return updates;
}

bool CAddonMgr::FindInstallableById(const std::string& addonId, AddonInfoPtr& addonInfo)
{
  AddonInfos versions;
  {
    CSingleLock lock(m_critSection);
    if (!m_database.FindByAddonId(addonId, versions) || versions.empty())
      return false;
  }

  addonInfo = *std::max_element(versions.begin(), versions.end(),
      [](const AddonInfoPtr& a, const AddonInfoPtr& b) { return a->Version() < b->Version(); });
  return true;
}

bool CAddonMgr::IsSystemAddon(const std::string& addonId)
{
  CSingleLock lock(m_critSection);
  return std::find(m_systemAddons.begin(), m_systemAddons.end(), addonId) != m_systemAddons.end();
}

bool CAddonMgr::IsCompatible(const AddonInfoPtr& addonProps)
{
  for (const auto& dependencyInfo : addonProps->GetDeps())
  {
    const auto& optional = dependencyInfo.second.second;
    if (!optional)
    {
      const auto& dependencyId = dependencyInfo.first;
      const auto& version = dependencyInfo.second.first;

      // Intentionally only check the xbmc.* and kodi.* magic dependencies. Everything else will
      // not be missing anyway, unless addon was installed in an unsupported way.
      if (StringUtils::StartsWith(dependencyId, "xbmc.") ||
          StringUtils::StartsWith(dependencyId, "kodi."))
      {
        AddonInfoPtr dependency = GetInstalledAddonInfo(dependencyId);
        if (!dependency || !dependency->MeetsVersion(version))
          return false;
      }
    }
  }

  return true;
}

bool CAddonMgr::IsBlacklisted(const std::string& addonId) const
{
  CSingleLock lock(m_critSection);
  return m_updateBlacklist.find(addonId) != m_updateBlacklist.end();
}

bool CAddonMgr::AddToUpdateBlacklist(const std::string& addonId)
{
  CSingleLock lock(m_critSection);
  if (IsBlacklisted(addonId))
    return true;
  return m_database.BlacklistAddon(addonId) && m_updateBlacklist.insert(addonId).second;
}

bool CAddonMgr::RemoveFromUpdateBlacklist(const std::string& addonId)
{
  CSingleLock lock(m_critSection);
  if (!IsBlacklisted(addonId))
    return true;
  return m_database.RemoveAddonFromBlacklist(addonId) && m_updateBlacklist.erase(addonId) > 0;
}

bool CAddonMgr::AddNewInstalledAddon(AddonInfoPtr& addonInfo)
{
  if (!addonInfo)
    return false;

  CSingleLock lock(m_critSection);

  AddonInfoPtr installedAddonInfo = std::make_shared<CAddonInfo>("special://home/addons/" + addonInfo->ID());
  if (installedAddonInfo->IsUsable())
  {
    m_installedAddons[addonInfo->ID()] = installedAddonInfo;
    if (EnableAddon(installedAddonInfo->ID()))
      return true;
  }

  return false;
}

void CAddonMgr::UpdateLastUsed(const std::string& id)
{
  auto time = CDateTime::GetCurrentDateTime();
  CJobManager::GetInstance().Submit([this, id, time](){
    {
      CSingleLock lock(m_critSection);
      m_database.SetLastUsed(id, time);
    }
    m_events.Publish(AddonEvents::MetadataChanged(id));
  });
}

bool CAddonMgr::UnloadAddon(const AddonInfoPtr& addonInfo)
{
  CSingleLock lock(m_critSection);
  
  std::string id = addonInfo->ID();
  m_enabledAddons.erase(id);
  m_installedAddons.erase(id);
  m_updateBlacklist.erase(id);

  m_events.Publish(AddonEvents::InstalledChanged());
  return true;
}

bool CAddonMgr::EnableAddon(const std::string& addonId)
{
  if (addonId.empty() || !IsAddonInstalled(addonId))
    return false;
  std::vector<std::string> needed;
  std::vector<std::string> missing;
  ResolveDependencies(addonId, needed, missing);
  for (const auto& dep : missing)
    CLog::Log(LOGWARNING, "CAddonMgr: '%s' required by '%s' is missing. Add-on may not function correctly", dep.c_str(), addonId.c_str());
  for (auto it = needed.rbegin(); it != needed.rend(); ++it)
    EnableSingle(*it);

  return true;
}

bool CAddonMgr::DisableAddon(const std::string& id)
{
  CSingleLock lock(m_critSection);

  if (!CanAddonBeDisabled(id))
    return false;
  if (!IsAddonEnabled(id))
    return true; //already disabled
  if (!m_database.DisableAddon(id, true))
    return false;

  const AddonInfoPtr addonInfo = GetInstalledAddonInfo(id);
  if (addonInfo == nullptr)
  {
    CLog::Log(LOGERROR, "ADDONS: Addon Id '%s' does not mach installed map", id.c_str());
    return false;
  }
  m_enabledAddons.erase(id);

  //success
  ADDON::OnDisabled(id);

  CEventLog::GetInstance().Add(EventPtr(new CAddonManagementEvent(addonInfo, 24141)));

  m_events.Publish(AddonEvents::Disabled(addonInfo));
  return true;
}

bool CAddonMgr::EnableSingle(const std::string& addonId)
{
  CSingleLock lock(m_critSection);

  if (IsAddonEnabled(addonId))
    return true; //already enabled
  if (!m_database.DisableAddon(addonId, false))
    return false;

  const AddonInfoPtr addonInfo = GetInstalledAddonInfo(addonId);
  if (addonInfo == nullptr)
  {
    CLog::Log(LOGERROR, "ADDONS: Addon Id '%s' does not mach installed map", addonId.c_str());
    return false;
  }
  m_enabledAddons[addonId] = addonInfo;

  ADDON::OnEnabled(addonId);

  CEventLog::GetInstance().Add(EventPtr(new CAddonManagementEvent(addonInfo, 24064)));

  CLog::Log(LOGDEBUG, "CAddonMgr: enabled %s", addonInfo->ID().c_str());
  m_events.Publish(AddonEvents::Enabled(addonInfo));
  return true;
}

bool CAddonMgr::CanAddonBeEnabled(const std::string& addonId)
{
  return !addonId.empty() && IsAddonInstalled(addonId);
}

bool CAddonMgr::CanUninstall(const AddonInfoPtr& addonProps)
{
  return addonProps && CanAddonBeDisabled(addonProps->ID()) &&
      !StringUtils::StartsWith(addonProps->Path(), CSpecialProtocol::TranslatePath("special://xbmc/addons"));
}

void CAddonMgr::FindAddons(AddonInfoList& addonmap, const std::string &path)
{
  CFileItemList items;
  if (XFILE::CDirectory::GetDirectory(path, items, "", XFILE::DIR_FLAG_NO_FILE_DIRS))
  {
    for (int i = 0; i < items.Size(); ++i)
    {
      std::string path = items[i]->GetPath();
      if (XFILE::CFile::Exists(path + "addon.xml"))
      {
        AddonInfoPtr addonInfo = std::make_shared<CAddonInfo>(path);
        if (addonInfo->IsUsable())
        {
          addonmap[addonInfo->ID()] = addonInfo;
        }
      }
    }
  }
}

bool CAddonMgr::LoadAddonDescription(AddonInfoPtr &addon, const std::string &path)
{
  addon = std::make_shared<CAddonInfo>(path);
  return addon != nullptr && addon->IsUsable();
}

void CAddonMgr::ResolveDependencies(const std::string& addonId, std::vector<std::string>& needed, std::vector<std::string>& missing)
{
  if (std::find(needed.begin(), needed.end(), addonId) != needed.end())
    return;

  AddonInfoPtr addon = GetInstalledAddonInfo(addonId);
  if (addon)
  {
    needed.push_back(addonId);
    for (const auto& dep : addon->GetDeps())
    {
      if (!dep.second.second) // ignore 'optional'
        ResolveDependencies(dep.first, needed, missing);
    }
  }
  else
    missing.push_back(addonId);
}

bool CAddonMgr::LoadManifest(std::set<std::string>& system, std::set<std::string>& optional)
{
  CXBMCTinyXML doc;
  if (!doc.LoadFile("special://xbmc/system/addon-manifest.xml"))
  {
    CLog::Log(LOGERROR, "ADDONS: manifest missing");
    return false;
  }

  auto root = doc.RootElement();
  if (!root || root->ValueStr() != "addons")
  {
    CLog::Log(LOGERROR, "ADDONS: malformed manifest");
    return false;
  }

  auto elem = root->FirstChildElement("addon");
  while (elem)
  {
    if (elem->FirstChild())
    {
      if (XMLUtils::GetAttribute(elem, "optional") == "true")
        optional.insert(elem->FirstChild()->ValueStr());
      else
        system.insert(elem->FirstChild()->ValueStr());
    }
    elem = elem->NextSiblingElement("addon");
  }
  return true;
}

std::shared_ptr<CAddon> CAddonMgr::CreateAddon(AddonInfoPtr addonInfo, TYPE addonType)
{
  if (addonType == ADDON_UNKNOWN)
    addonType = addonInfo->MainType();

  switch (addonType)
  {
    case ADDON_PLUGIN:
    case ADDON_SCRIPT:
    case ADDON_SCRIPT_LIBRARY:
    case ADDON_SCRIPT_LYRICS:
    case ADDON_SCRIPT_MODULE:
    case ADDON_SUBTITLE_MODULE:
    case ADDON_SCRIPT_WEATHER:
      return std::make_shared<CAddon>(addonInfo);
    case ADDON_WEB_INTERFACE:
      return std::make_shared<CWebinterface>(addonInfo);
    case ADDON_SERVICE:
      return std::make_shared<CService>(addonInfo);
    case ADDON_SCRAPER_ALBUMS:
    case ADDON_SCRAPER_ARTISTS:
    case ADDON_SCRAPER_MOVIES:
    case ADDON_SCRAPER_MUSICVIDEOS:
    case ADDON_SCRAPER_TVSHOWS:
    case ADDON_SCRAPER_LIBRARY:
      return std::make_shared<CScraper>(addonType, addonInfo);
    case ADDON_VIZ:
    case ADDON_SCREENSAVER:
      return std::make_shared<CAddonDll>(addonInfo);
    case ADDON_PVRDLL:
      return std::make_shared<PVR::CPVRClient>(addonInfo);
    case ADDON_ADSPDLL:
      return std::make_shared<ActiveAE::CActiveAEDSPAddon>(addonInfo);
    case ADDON_AUDIODECODER:
      return std::make_shared<CAudioDecoder>(addonInfo);
    case ADDON_IMAGEDECODER:
      return std::make_shared<CImageDecoder>(addonInfo);
    case ADDON_PERIPHERALDLL:
      return std::make_shared<PERIPHERALS::CPeripheralAddon>(addonInfo);
    case ADDON_GAMEDLL:
      return std::make_shared<GAME::CGameClient>(addonInfo);
    case ADDON_VFS:
      return std::make_shared<CVFSEntry>(addonInfo);
    case ADDON_SKIN:
      return std::make_shared<CSkinInfo>(addonInfo);
    case ADDON_RESOURCE_IMAGES:
      return std::make_shared<CImageResource>(addonInfo);
    case ADDON_RESOURCE_GAMES:
      return std::make_shared<CGameResource>(addonInfo);
    case ADDON_RESOURCE_LANGUAGE:
      return std::make_shared<CLanguageResource>(addonInfo);
    case ADDON_RESOURCE_UISOUNDS:
      return std::make_shared<CUISoundsResource>(addonInfo);
    case ADDON_REPOSITORY:
      return std::make_shared<CRepository>(addonInfo);
    case ADDON_CONTEXT_ITEM:
      return std::make_shared<CContextMenuAddon>(addonInfo);
    case ADDON_GAME_CONTROLLER:
      return std::make_shared<GAME::CController>(addonInfo);
    default:
      break;
  }

  return std::make_shared<CAddon>(addonInfo);
}

//! @todo need to investigate, change and remove
//@{
IAddonMgrCallback* CAddonMgr::GetCallbackForType(TYPE type)
{
  if (m_managers.find(type) == m_managers.end())
    return NULL;
  else
    return m_managers[type];
}

bool CAddonMgr::RegisterAddonMgrCallback(const TYPE type, IAddonMgrCallback* cb)
{
  if (cb == NULL)
    return false;

  m_managers.erase(type);
  m_managers[type] = cb;

  return true;
}

void CAddonMgr::UnregisterAddonMgrCallback(TYPE type)
{
  m_managers.erase(type);
}

void CAddonMgr::AddToUpdateableAddons(AddonPtr &pAddon)
{
  CSingleLock lock(m_critSection);
  m_updateableAddons.push_back(pAddon);
}

void CAddonMgr::RemoveFromUpdateableAddons(AddonPtr &pAddon)
{
  CSingleLock lock(m_critSection);
  VECADDONS::iterator it = std::find(m_updateableAddons.begin(), m_updateableAddons.end(), pAddon);

  if(it != m_updateableAddons.end())
  {
    m_updateableAddons.erase(it);
  }
}

struct AddonIdFinder
{
  AddonIdFinder(const std::string& id)
    : m_id(id)
  {}

  bool operator()(const AddonPtr& addon)
  {
    return m_id == addon->ID();
  }
  private:
  std::string m_id;
};

bool CAddonMgr::ReloadSettings(const std::string &id)
{
  CSingleLock lock(m_critSection);
  VECADDONS::iterator it = std::find_if(m_updateableAddons.begin(), m_updateableAddons.end(), AddonIdFinder(id));

  if( it != m_updateableAddons.end())
  {
    return (*it)->ReloadSettings();
  }
  return false;
}

bool CAddonMgr::ServicesHasStarted() const
{
  CSingleLock lock(m_critSection);
  return m_serviceSystemStarted;
}

bool CAddonMgr::StartServices(const bool beforelogin)
{
  CLog::Log(LOGDEBUG, "ADDON: Starting service addons.");

  VECADDONS services;
  if (!GetAddons(services, ADDON_SERVICE))
    return false;

  bool ret = true;
  for (auto addon : services)
  {
    std::shared_ptr<CService> service = std::dynamic_pointer_cast<CService>(addon);
    if (service)
    {
      if ( (beforelogin && service->GetStartOption() == CService::STARTUP)
        || (!beforelogin && service->GetStartOption() == CService::LOGIN) )
        ret &= service->Start();
    }
  }

  CSingleLock lock(m_critSection);
  m_serviceSystemStarted = true;

  return ret;
}

void CAddonMgr::StopServices(const bool onlylogin)
{
  CLog::Log(LOGDEBUG, "ADDON: Stopping service addons.");

  VECADDONS services;
  if (!GetAddons(services, ADDON_SERVICE))
    return;

  for (auto addon : services)
  {
    std::shared_ptr<CService> service = std::dynamic_pointer_cast<CService>(addon);
    if (service)
    {
      if ( (onlylogin && service->GetStartOption() == CService::LOGIN)
        || (!onlylogin) )
        service->Stop();
    }
  }

  CSingleLock lock(m_critSection);
  m_serviceSystemStarted = false;
}

bool CAddonMgr::CanAddonBeDisabled(const std::string& addonID)
{
  if (addonID.empty())
    return false;

  CSingleLock lock(m_critSection);
  if (IsSystemAddon(addonID))
    return false;

  AddonPtr localAddon;
  // can't disable an addon that isn't installed
  if (!GetAddon(addonID, localAddon, ADDON_UNKNOWN))
    return false;

  // can't disable an addon that is in use
  if (localAddon->IsInUse())
    return false;

  return true;
}

//@}

} /* namespace ADDON */

