/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AddonManager.h"

#include "LangInfo.h"
#include "ServiceBroker.h"
#include "addons/AddonInstaller.h"
#include "addons/AddonSystemSettings.h"
#include "addons/addoninfo/AddonInfoBuilder.h"
#include "events/AddonManagementEvent.h"
#include "events/EventLog.h"
#include "events/NotificationEvent.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "filesystem/SpecialProtocol.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/XMLUtils.h"
#include "utils/log.h"

#include <algorithm>
#include <array>
#include <set>
#include <utility>

using namespace XFILE;

namespace ADDON
{

/**********************************************************
 * CAddonMgr
 *
 */

std::map<TYPE, IAddonMgrCallback*> CAddonMgr::m_managers;

static bool LoadManifest(std::set<std::string>& system, std::set<std::string>& optional)
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

CAddonMgr::~CAddonMgr()
{
  DeInit();
}

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

bool CAddonMgr::Init()
{
  CSingleLock lock(m_critSection);

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
    AddonPtr addon;
    if (!GetAddon(id, addon, ADDON_UNKNOWN))
    {
      CLog::Log(LOGFATAL, "addon '%s' not installed or not enabled.", id.c_str());
      return false;
    }
  }

  return true;
}

void CAddonMgr::DeInit()
{
  m_database.Close();

  /* If temporary directory was used from add-on, delete it */
  if (XFILE::CDirectory::Exists(m_tempAddonBasePath))
    XFILE::CDirectory::RemoveRecursive(CSpecialProtocol::TranslatePath(m_tempAddonBasePath));
}

bool CAddonMgr::HasAddons(const TYPE &type)
{
  VECADDONS addons;
  return GetAddonsInternal(type, addons, true);
}

bool CAddonMgr::HasInstalledAddons(const TYPE &type)
{
  VECADDONS addons;
  return GetAddonsInternal(type, addons, false);
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
    explicit AddonIdFinder(const std::string& id)
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

VECADDONS CAddonMgr::GetAvailableUpdates() const
{
  CSingleLock lock(m_critSection);
  auto start = XbmcThreads::SystemClockMillis();

  VECADDONS updates;
  VECADDONS installed;
  VECADDONS all_addons;
  m_database.GetRepositoryContent(all_addons);
  std::map<std::string, AddonPtr> last_versions;
  for (const auto& addon : all_addons)
  {
    const auto& latest_known = last_versions.find(addon->ID());
    if (latest_known == last_versions.end() || addon->Version() > latest_known->second->Version())
      last_versions[addon->ID()] = std::move(addon);
  }

  GetAddons(installed);
  for (const auto& addon : installed)
  {
    const auto& remote = last_versions.find(addon->ID());
    if (remote != last_versions.end() && remote->second->Version() > addon->Version())
      updates.emplace_back(std::move(remote->second));
  }
  CLog::Log(LOGDEBUG, "CAddonMgr::GetAvailableUpdates took %i ms", XbmcThreads::SystemClockMillis() - start);
  return updates;
}

bool CAddonMgr::HasAvailableUpdates()
{
  return !GetAvailableUpdates().empty();
}

bool CAddonMgr::GetAddons(VECADDONS& addons) const
{
  return GetAddonsInternal(ADDON_UNKNOWN, addons, true);
}

bool CAddonMgr::GetAddons(VECADDONS& addons, const TYPE& type)
{
  return GetAddonsInternal(type, addons, true);
}

bool CAddonMgr::GetInstalledAddons(VECADDONS& addons)
{
  return GetAddonsInternal(ADDON_UNKNOWN, addons, false);
}

bool CAddonMgr::GetInstalledAddons(VECADDONS& addons, const TYPE& type)
{
  return GetAddonsInternal(type, addons, false);
}

bool CAddonMgr::GetDisabledAddons(VECADDONS& addons)
{
  return CAddonMgr::GetDisabledAddons(addons, ADDON_UNKNOWN);
}

bool CAddonMgr::GetDisabledAddons(VECADDONS& addons, const TYPE& type)
{
  VECADDONS all;
  if (GetInstalledAddons(all, type))
  {
    std::copy_if(all.begin(), all.end(), std::back_inserter(addons),
        [this](const AddonPtr& addon){ return IsAddonDisabled(addon->ID()); });
    return true;
  }
  return false;
}

bool CAddonMgr::GetInstallableAddons(VECADDONS& addons)
{
  return GetInstallableAddons(addons, ADDON_UNKNOWN);
}

bool CAddonMgr::GetInstallableAddons(VECADDONS& addons, const TYPE &type)
{
  CSingleLock lock(m_critSection);

  // get all addons
  if (!m_database.GetRepositoryContent(addons))
    return false;

  // go through all addons and remove all that are already installed

  addons.erase(std::remove_if(addons.begin(), addons.end(),
    [this, type](const AddonPtr& addon)
    {
      bool bErase = false;

      // check if the addon matches the provided addon type
      if (type != ADDON::ADDON_UNKNOWN && addon->Type() != type && !addon->HasType(type))
        bErase = true;

      if (!this->CanAddonBeInstalled(addon))
        bErase = true;

      return bErase;
    }), addons.end());

  return true;
}

bool CAddonMgr::FindInstallableById(const std::string& addonId, AddonPtr& result)
{
  VECADDONS versions;
  {
    CSingleLock lock(m_critSection);
    if (!m_database.FindByAddonId(addonId, versions) || versions.empty())
      return false;
  }

  result = *std::max_element(versions.begin(), versions.end(),
      [](const AddonPtr& a, const AddonPtr& b) { return a->Version() < b->Version(); });
  return true;
}

bool CAddonMgr::GetInstalledBinaryAddons(BINARY_ADDON_LIST& binaryAddonList)
{
  CSingleLock lock(m_critSection);

  for (auto addon : m_installedAddons)
  {
    BINARY_ADDON_LIST_ENTRY binaryAddon;
    if (GetInstalledBinaryAddon(addon.first, binaryAddon))
      binaryAddonList.push_back(std::move(binaryAddon));
  }

  return !binaryAddonList.empty();
}

bool CAddonMgr::GetInstalledBinaryAddon(const std::string& addonId, BINARY_ADDON_LIST_ENTRY& binaryAddon)
{
  bool ret = false;

  CSingleLock lock(m_critSection);

  AddonInfoPtr addon = GetAddonInfo(addonId);
  if (addon)
  {
    binaryAddon = BINARY_ADDON_LIST_ENTRY(!IsAddonDisabled(addonId), addon);
    ret = true;
  }

  return ret;
}

bool CAddonMgr::GetAddonsInternal(const TYPE& type, VECADDONS& addons, bool enabledOnly) const
{
  CSingleLock lock(m_critSection);

  for (const auto& addonInfo : m_installedAddons)
  {
    if (type != ADDON_UNKNOWN && !addonInfo.second->HasType(type))
      continue;

    if (enabledOnly && IsAddonDisabled(addonInfo.second->ID()))
      continue;

    //FIXME: hack for skipping special dependency addons (xbmc.python etc.).
    //Will break if any extension point is added to them
    if (addonInfo.second->MainType() == ADDON_UNKNOWN)
      continue;

    AddonPtr addon = CAddonBuilder::Generate(addonInfo.second, type);
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

bool CAddonMgr::GetIncompatibleAddons(VECADDONS& incompatible) const
{
  GetAddons(incompatible);
  incompatible.erase(std::remove_if(incompatible.begin(), incompatible.end(),
                                    [this](const AddonPtr a) { return IsCompatible(*a); }),
                     incompatible.end());
  return !incompatible.empty();
}

std::vector<std::string> CAddonMgr::MigrateAddons()
{
  CLog::Log(LOGINFO, "ADDON: waiting for add-ons to update...");
  CheckAndInstallAddonUpdates(true);

  VECADDONS incompatible;
  GetIncompatibleAddons(incompatible);
  std::vector<std::string> changed;

  for (const auto& addon : incompatible)
  {
    CLog::Log(LOGINFO, "ADDON: {} version {} is incompatible", addon->ID(),
              addon->Version().asString());

    if (!CAddonSystemSettings::GetInstance().UnsetActive(addon))
    {
      CLog::Log(LOGWARNING, "ADDON: failed to unset {}", addon->ID());
      continue;
    }
    if (!DisableAddon(addon->ID()))
    {
      CLog::Log(LOGWARNING, "ADDON: failed to disable {}", addon->ID());
    }
    changed.push_back(addon->Name());
  }

  return changed;
}

void CAddonMgr::CheckAndInstallAddonUpdates(bool wait) const
{
  // Get Addons in need of an update and remove all the blacklisted ones
  auto updates = GetAvailableUpdates();
  updates.erase(std::remove_if(updates.begin(), updates.end(), [this](const AddonPtr& addon) {
    return IsBlacklisted(addon->ID());
  }), updates.end());

  // install all
  CAddonInstaller::GetInstance().InstallAddons(updates, wait);
}

bool CAddonMgr::GetAddon(const std::string& str,
                         AddonPtr& addon,
                         const TYPE& type /*=ADDON_UNKNOWN*/,
                         bool enabledOnly /*= true*/) const
{
  CSingleLock lock(m_critSection);

  AddonInfoPtr addonInfo = GetAddonInfo(str, type);
  if (addonInfo)
  {
    addon = CAddonBuilder::Generate(addonInfo, type);
    if (addon)
    {
      if (enabledOnly && IsAddonDisabled(addonInfo->ID()))
        return false;

      // if the addon has a running instance, grab that
      AddonPtr runningAddon = addon->GetRunningInstance();
      if (runningAddon)
        addon = runningAddon;
    }
    return NULL != addon.get();
  }

  return false;
}

bool CAddonMgr::HasType(const std::string &id, const TYPE &type)
{
  AddonPtr addon;
  return GetAddon(id, addon, type, false);
}

bool CAddonMgr::FindAddons()
{
  ADDON_INFO_LIST installedAddons;

  FindAddons(installedAddons, "special://xbmcbin/addons");
  FindAddons(installedAddons, "special://xbmc/addons");
  FindAddons(installedAddons, "special://home/addons");

  std::set<std::string> installed;
  for (const auto& addon : installedAddons)
    installed.insert(addon.second->ID());

  CSingleLock lock(m_critSection);

  // Sync with db
  m_database.SyncInstalled(installed, m_systemAddons, m_optionalAddons);
  for (const auto& addon : installedAddons)
  {
    m_database.GetInstallData(addon.second);
    CLog::Log(LOGINFO, "CAddonMgr::{}: {} v{} installed", __FUNCTION__, addon.second->ID(),
              addon.second->Version().asString());
  }

  m_installedAddons = std::move(installedAddons);

  // Reload caches
  std::set<std::string> tmp;
  m_database.GetDisabled(tmp);
  m_disabled = std::move(tmp);

  tmp.clear();
  m_database.GetBlacklisted(tmp);
  m_updateBlacklist = std::move(tmp);

  return true;
}

bool CAddonMgr::UnloadAddon(const std::string& addonId)
{
  CSingleLock lock(m_critSection);

  if (!IsAddonInstalled(addonId))
    return true;

  m_installedAddons.erase(addonId);
  CLog::Log(LOGDEBUG, "CAddonMgr: %s unloaded", addonId.c_str());

  lock.Leave();
  AddonEvents::Unload event(addonId);

  return true;
}

bool CAddonMgr::LoadAddon(const std::string& addonId)
{
  CSingleLock lock(m_critSection);

  AddonPtr addon;
  if (GetAddon(addonId, addon, ADDON_UNKNOWN, false))
  {
    return true;
  }

  if (!FindAddons())
  {
    CLog::Log(LOGERROR, "CAddonMgr: could not reload add-on %s. FindAddons failed.", addonId.c_str());
    return false;
  }

  if (!GetAddon(addonId, addon, ADDON_UNKNOWN, false))
  {
    CLog::Log(LOGERROR, "CAddonMgr: could not load add-on %s. No add-on with that ID is installed.", addonId.c_str());
    return false;
  }

  lock.Leave();

  AddonEvents::Load event(addon->ID());
  m_unloadEvents.HandleEvent(event);

  if (IsAddonDisabled(addon->ID()))
  {
    EnableAddon(addon->ID());
    return true;
  }

  m_events.Publish(AddonEvents::ReInstalled(addon->ID()));
  CLog::Log(LOGDEBUG, "CAddonMgr: %s successfully loaded", addon->ID().c_str());
  return true;
}

void CAddonMgr::OnPostUnInstall(const std::string& id)
{
  CSingleLock lock(m_critSection);
  m_disabled.erase(id);
  m_updateBlacklist.erase(id);
  m_events.Publish(AddonEvents::UnInstalled(id));
}

bool CAddonMgr::RemoveFromUpdateBlacklist(const std::string& id)
{
  CSingleLock lock(m_critSection);
  if (!IsBlacklisted(id))
    return true;
  return m_database.RemoveAddonFromBlacklist(id) && m_updateBlacklist.erase(id) > 0;
}

bool CAddonMgr::AddToUpdateBlacklist(const std::string& id)
{
  CSingleLock lock(m_critSection);
  if (IsBlacklisted(id))
    return true;
  return m_database.BlacklistAddon(id) && m_updateBlacklist.insert(id).second;
}

bool CAddonMgr::IsBlacklisted(const std::string& id) const
{
  CSingleLock lock(m_critSection);
  return m_updateBlacklist.find(id) != m_updateBlacklist.end();
}

void CAddonMgr::UpdateLastUsed(const std::string& id)
{
  auto time = CDateTime::GetCurrentDateTime();
  CJobManager::GetInstance().Submit([this, id, time](){
    {
      CSingleLock lock(m_critSection);
      m_database.SetLastUsed(id, time);
      auto addonInfo = GetAddonInfo(id);
      if (addonInfo)
        addonInfo->SetLastUsed(time);
    }
    m_events.Publish(AddonEvents::MetadataChanged(id));
  });
}

static void ResolveDependencies(const std::string& addonId, std::vector<std::string>& needed, std::vector<std::string>& missing)
{
  if (std::find(needed.begin(), needed.end(), addonId) != needed.end())
    return;

  AddonPtr addon;
  if (!CServiceBroker::GetAddonMgr().GetAddon(addonId, addon, ADDON_UNKNOWN, false))
    missing.push_back(addonId);
  else
  {
    needed.push_back(addonId);
    for (const auto& dep : addon->GetDependencies())
      if (!dep.optional)
        ResolveDependencies(dep.id, needed, missing);
  }
}

bool CAddonMgr::DisableAddon(const std::string& id)
{
  CSingleLock lock(m_critSection);
  if (!CanAddonBeDisabled(id))
    return false;
  if (m_disabled.find(id) != m_disabled.end())
    return true; //already disabled
  if (!m_database.DisableAddon(id))
    return false;
  if (!m_disabled.insert(id).second)
    return false;

  //success
  CLog::Log(LOGDEBUG, "CAddonMgr: %s disabled", id.c_str());
  AddonPtr addon;
  if (GetAddon(id, addon, ADDON_UNKNOWN, false) && addon != NULL)
  {
    CServiceBroker::GetEventLog().Add(EventPtr(new CAddonManagementEvent(addon, 24141)));
  }

  m_events.Publish(AddonEvents::Disabled(id));
  return true;
}

bool CAddonMgr::EnableSingle(const std::string& id)
{
  CSingleLock lock(m_critSection);

  if (m_disabled.find(id) == m_disabled.end())
    return true; //already enabled

  AddonPtr addon;
  if (!GetAddon(id, addon, ADDON_UNKNOWN, false) || addon == nullptr)
    return false;

  if (!IsCompatible(*addon))
  {
    CLog::Log(LOGERROR, "Add-on '%s' is not compatible with Kodi", addon->ID().c_str());
    CServiceBroker::GetEventLog().AddWithNotification(EventPtr(new CNotificationEvent(addon->Name(), 24152, EventLevel::Error)));
    return false;
  }

  if (!m_database.DisableAddon(id, false))
    return false;
  m_disabled.erase(id);

  CServiceBroker::GetEventLog().Add(EventPtr(new CAddonManagementEvent(addon, 24064)));

  CLog::Log(LOGDEBUG, "CAddonMgr: enabled %s", addon->ID().c_str());
  m_events.Publish(AddonEvents::Enabled(id));
  return true;
}

bool CAddonMgr::EnableAddon(const std::string& id)
{
  if (id.empty() || !IsAddonInstalled(id))
    return false;
  std::vector<std::string> needed;
  std::vector<std::string> missing;
  ResolveDependencies(id, needed, missing);
  for (const auto& dep : missing)
    CLog::Log(LOGWARNING, "CAddonMgr: '%s' required by '%s' is missing. Add-on may not function "
        "correctly", dep.c_str(), id.c_str());
  for (auto it = needed.rbegin(); it != needed.rend(); ++it)
    EnableSingle(*it);

  return true;
}

bool CAddonMgr::IsAddonDisabled(const std::string& ID) const
{
  CSingleLock lock(m_critSection);
  return m_disabled.find(ID) != m_disabled.end();
}

bool CAddonMgr::CanAddonBeDisabled(const std::string& ID)
{
  if (ID.empty())
    return false;

  CSingleLock lock(m_critSection);
  if (IsSystemAddon(ID))
    return false;

  AddonPtr localAddon;
  // can't disable an addon that isn't installed
  if (!GetAddon(ID, localAddon, ADDON_UNKNOWN, false))
    return false;

  // can't disable an addon that is in use
  if (localAddon->IsInUse())
    return false;

  return true;
}

bool CAddonMgr::CanAddonBeEnabled(const std::string& id)
{
  return !id.empty() && IsAddonInstalled(id);
}

bool CAddonMgr::IsAddonInstalled(const std::string& ID)
{
  AddonPtr tmp;
  return GetAddon(ID, tmp, ADDON_UNKNOWN, false);
}

bool CAddonMgr::CanAddonBeInstalled(const AddonPtr& addon)
{
  return addon != nullptr && !addon->IsBroken() && !IsAddonInstalled(addon->ID());
}

bool CAddonMgr::CanUninstall(const AddonPtr& addon)
{
  return addon && CanAddonBeDisabled(addon->ID()) &&
      !StringUtils::StartsWith(addon->Path(), CSpecialProtocol::TranslatePath("special://xbmc/addons"));
}

bool CAddonMgr::IsSystemAddon(const std::string& id)
{
  CSingleLock lock(m_critSection);
  return std::find(m_systemAddons.begin(), m_systemAddons.end(), id) != m_systemAddons.end();
}

bool CAddonMgr::LoadAddonDescription(const std::string &directory, AddonPtr &addon)
{
  auto addonInfo = CAddonInfoBuilder::Generate(directory);
  if (addonInfo)
    addon = CAddonBuilder::Generate(addonInfo, ADDON_UNKNOWN);

  return addon != nullptr;
}

bool CAddonMgr::AddonsFromRepoXML(const CRepository::DirInfo& repo, const std::string& xml, VECADDONS& addons)
{
  CXBMCTinyXML doc;
  if (!doc.Parse(xml))
  {
    CLog::Log(LOGERROR, "CAddonMgr::{}: Failed to parse addons.xml", __FUNCTION__);
    return false;
  }

  if (doc.RootElement() == nullptr || doc.RootElement()->ValueStr() != "addons")
  {
    CLog::Log(LOGERROR, "CAddonMgr::{}: Failed to parse addons.xml. Malformed", __FUNCTION__);
    return false;
  }

  // each addon XML should have a UTF-8 declaration
  auto element = doc.RootElement()->FirstChildElement("addon");
  while (element)
  {
    auto addonInfo = CAddonInfoBuilder::Generate(element, repo);
    auto addon = CAddonBuilder::Generate(addonInfo, ADDON_UNKNOWN);
    if (addon)
      addons.push_back(std::move(addon));

    element = element->NextSiblingElement("addon");
  }

  return true;
}

bool CAddonMgr::IsCompatible(const IAddon& addon) const
{
  for (const auto& dependency : addon.GetDependencies())
  {
    if (!dependency.optional)
    {
      // Intentionally only check the xbmc.* and kodi.* magic dependencies. Everything else will
      // not be missing anyway, unless addon was installed in an unsupported way.
      if (StringUtils::StartsWith(dependency.id, "xbmc.") ||
          StringUtils::StartsWith(dependency.id, "kodi."))
      {
        AddonPtr addon;
        bool haveAddon = GetAddon(dependency.id, addon);
        if (!haveAddon || !addon->MeetsVersion(dependency.versionMin, dependency.version))
          return false;
      }
    }
  }
  return true;
}

std::vector<DependencyInfo> CAddonMgr::GetDepsRecursive(const std::string& id)
{
  std::vector<DependencyInfo> added;
  AddonPtr root_addon;
  if (!FindInstallableById(id, root_addon) && !GetAddon(id, root_addon))
    return added;

  std::vector<DependencyInfo> toProcess;
  for (const auto& dep : root_addon->GetDependencies())
    toProcess.push_back(dep);

  while (!toProcess.empty())
  {
    auto current_dep = *toProcess.begin();
    toProcess.erase(toProcess.begin());
    if (StringUtils::StartsWith(current_dep.id, "xbmc.") ||
        StringUtils::StartsWith(current_dep.id, "kodi."))
      continue;

    auto added_it = std::find_if(added.begin(), added.end(), [&](const DependencyInfo& d){ return d.id == current_dep.id;});
    if (added_it != added.end())
    {
      if (current_dep.version < added_it->version)
        continue;

      bool aopt = added_it->optional;
      added.erase(added_it);
      added.push_back(current_dep);
      if (!current_dep.optional && aopt)
        continue;
    }
    else
      added.push_back(current_dep);

    AddonPtr current_addon;
    if (FindInstallableById(current_dep.id, current_addon))
    {
      for (const auto& item : current_addon->GetDependencies())
        toProcess.push_back(item);
    }
  }

  return added;
}

bool CAddonMgr::GetAddonInfos(AddonInfos& addonInfos, TYPE type) const
{
  CSingleLock lock(m_critSection);

  bool forUnknown = type == ADDON_UNKNOWN;
  for (auto& info : m_installedAddons)
  {
    if (info.second->MainType() != ADDON_UNKNOWN && (forUnknown || info.second->HasType(type)))
      addonInfos.push_back(info.second);
  }

  return !addonInfos.empty();
}

const AddonInfoPtr CAddonMgr::GetAddonInfo(const std::string& id,
                                           TYPE type /*= ADDON_UNKNOWN*/) const
{
  CSingleLock lock(m_critSection);

  auto addon = m_installedAddons.find(id);
  if (addon != m_installedAddons.end())
    if ((type == ADDON_UNKNOWN || addon->second->HasType(type)))
      return addon->second;

  return nullptr;
}

void CAddonMgr::FindAddons(ADDON_INFO_LIST& addonmap, const std::string& path)
{
  CFileItemList items;
  if (XFILE::CDirectory::GetDirectory(path, items, "", XFILE::DIR_FLAG_NO_FILE_DIRS))
  {
    for (int i = 0; i < items.Size(); ++i)
    {
      std::string path = items[i]->GetPath();
      if (XFILE::CFile::Exists(path + "addon.xml"))
      {
        AddonInfoPtr addonInfo = CAddonInfoBuilder::Generate(path);
        if (addonInfo)
        {
          const auto& it = addonmap.find(addonInfo->ID());
          if (it != addonmap.end())
          {
            if (it->second->Version() > addonInfo->Version())
            {
              CLog::Log(LOGWARNING, "CAddonMgr::{}: Addon '{}' already present with higher version {} at '{}' - other version {} at '{}' will be ignored",
                           __FUNCTION__, addonInfo->ID(), it->second->Version().asString(), it->second->Path(), addonInfo->Version().asString(), addonInfo->Path());
              continue;
            }
            CLog::Log(LOGDEBUG, "CAddonMgr::{}: Addon '{}' already present with version {} at '{}' replaced with version {} at '{}'",
                         __FUNCTION__, addonInfo->ID(), it->second->Version().asString(), it->second->Path(), addonInfo->Version().asString(), addonInfo->Path());
          }

          addonmap[addonInfo->ID()] = addonInfo;
        }
      }
    }
  }
}

} /* namespace ADDON */
