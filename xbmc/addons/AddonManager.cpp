/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AddonManager.h"

#include "CompileInfo.h"
#include "FileItem.h"
#include "FileItemList.h"
#include "LangInfo.h"
#include "ServiceBroker.h"
#include "addons/AddonBuilder.h"
#include "addons/AddonDatabase.h"
#include "addons/AddonEvents.h"
#include "addons/AddonInstaller.h"
#include "addons/AddonRepos.h"
#include "addons/AddonSystemSettings.h"
#include "addons/AddonUpdateRules.h"
#include "addons/IAddon.h"
#include "addons/addoninfo/AddonInfo.h"
#include "addons/addoninfo/AddonInfoBuilder.h"
#include "addons/addoninfo/AddonType.h"
#include "events/AddonManagementEvent.h"
#include "events/EventLog.h"
#include "events/NotificationEvent.h"
#include "filesystem/Directory.h"
#include "filesystem/SpecialProtocol.h"
#include "utils/FileUtils.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/XMLUtils.h"
#include "utils/log.h"

#include <algorithm>
#include <array>
#include <mutex>
#include <set>
#include <utility>

using namespace XFILE;

namespace ADDON
{

/**********************************************************
 * CAddonMgr
 *
 */

std::map<AddonType, IAddonMgrCallback*> CAddonMgr::m_managers;

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

CAddonMgr::CAddonMgr()
  : m_database(std::make_unique<CAddonDatabase>()),
    m_updateRules(std::make_unique<CAddonUpdateRules>())
{
}

CAddonMgr::~CAddonMgr()
{
  DeInit();
}

IAddonMgrCallback* CAddonMgr::GetCallbackForType(AddonType type)
{
  if (m_managers.find(type) == m_managers.end())
    return nullptr;
  else
    return m_managers[type];
}

bool CAddonMgr::RegisterAddonMgrCallback(AddonType type, IAddonMgrCallback* cb)
{
  if (cb == nullptr)
    return false;

  m_managers.erase(type);
  m_managers[type] = cb;

  return true;
}

void CAddonMgr::UnregisterAddonMgrCallback(AddonType type)
{
  m_managers.erase(type);
}

bool CAddonMgr::Init()
{
  std::unique_lock<CCriticalSection> lock(m_critSection);

  if (!LoadManifest(m_systemAddons, m_optionalSystemAddons))
  {
    CLog::Log(LOGERROR, "ADDONS: Failed to read manifest");
    return false;
  }

  if (!m_database->Open())
    CLog::Log(LOGFATAL, "ADDONS: Failed to open database");

  FindAddons();

  //Ensure required add-ons are installed and enabled
  for (const auto& id : m_systemAddons)
  {
    AddonPtr addon;
    if (!GetAddon(id, addon, AddonType::UNKNOWN, OnlyEnabled::CHOICE_YES))
    {
      CLog::Log(LOGFATAL, "addon '{}' not installed or not enabled.", id);
      return false;
    }
  }

  return true;
}

void CAddonMgr::DeInit()
{
  m_database->Close();

  /* If temporary directory was used from add-on, delete it */
  if (XFILE::CDirectory::Exists(m_tempAddonBasePath))
    XFILE::CDirectory::RemoveRecursive(CSpecialProtocol::TranslatePath(m_tempAddonBasePath));
}

bool CAddonMgr::HasAddons(AddonType type)
{
  std::unique_lock<CCriticalSection> lock(m_critSection);

  for (const auto& addonInfo : m_installedAddons)
  {
    if (addonInfo.second->HasType(type) && !IsAddonDisabled(addonInfo.second->ID()))
      return true;
  }
  return false;
}

bool CAddonMgr::HasInstalledAddons(AddonType type)
{
  std::unique_lock<CCriticalSection> lock(m_critSection);

  for (const auto& addonInfo : m_installedAddons)
  {
    if (addonInfo.second->HasType(type))
      return true;
  }
  return false;
}

void CAddonMgr::AddToUpdateableAddons(AddonPtr &pAddon)
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  m_updateableAddons.push_back(pAddon);
}

void CAddonMgr::RemoveFromUpdateableAddons(AddonPtr &pAddon)
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
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

bool CAddonMgr::ReloadSettings(const std::string& addonId, AddonInstanceId instanceId)
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  VECADDONS::iterator it =
      std::find_if(m_updateableAddons.begin(), m_updateableAddons.end(), AddonIdFinder(addonId));

  if( it != m_updateableAddons.end())
  {
    return (*it)->ReloadSettings(instanceId);
  }
  return false;
}

std::vector<std::shared_ptr<IAddon>> CAddonMgr::GetAvailableUpdates() const
{
  std::vector<std::shared_ptr<IAddon>> availableUpdates =
      GetAvailableUpdatesOrOutdatedAddons(AddonCheckType::AVAILABLE_UPDATES);

  std::lock_guard<std::mutex> lock(m_lastAvailableUpdatesCountMutex);
  m_lastAvailableUpdatesCountAsString = std::to_string(availableUpdates.size());

  return availableUpdates;
}

const std::string& CAddonMgr::GetLastAvailableUpdatesCountAsString() const
{
  std::lock_guard<std::mutex> lock(m_lastAvailableUpdatesCountMutex);
  return m_lastAvailableUpdatesCountAsString;
};

std::vector<std::shared_ptr<IAddon>> CAddonMgr::GetOutdatedAddons() const
{
  return GetAvailableUpdatesOrOutdatedAddons(AddonCheckType::OUTDATED_ADDONS);
}

std::vector<std::shared_ptr<IAddon>> CAddonMgr::GetAvailableUpdatesOrOutdatedAddons(
    AddonCheckType addonCheckType) const
{
  auto start = std::chrono::steady_clock::now();

  std::vector<std::shared_ptr<IAddon>> result;
  std::vector<std::shared_ptr<IAddon>> installed;
  CAddonRepos addonRepos;

  if (addonRepos.IsValid())
  {
    GetAddonsForUpdate(installed);
    addonRepos.BuildUpdateOrOutdatedList(installed, result, addonCheckType);
  }

  auto end = std::chrono::steady_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
  CLog::Log(LOGDEBUG, "CAddonMgr::GetAvailableUpdatesOrOutdatedAddons took {} ms",
            duration.count());

  return result;
}

std::map<std::string, AddonWithUpdate> CAddonMgr::GetAddonsWithAvailableUpdate() const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  auto start = std::chrono::steady_clock::now();

  std::vector<std::shared_ptr<IAddon>> installed;
  std::map<std::string, AddonWithUpdate> result;
  CAddonRepos addonRepos;

  if (addonRepos.IsValid())
  {
    GetAddonsForUpdate(installed);
    addonRepos.BuildAddonsWithUpdateList(installed, result);
  }

  auto end = std::chrono::steady_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
  CLog::Log(LOGDEBUG, "CAddonMgr::{} took {} ms", __func__, duration.count());

  return result;
}

std::vector<std::shared_ptr<IAddon>> CAddonMgr::GetCompatibleVersions(
    const std::string& addonId) const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  auto start = std::chrono::steady_clock::now();

  CAddonRepos addonRepos(addonId);
  std::vector<std::shared_ptr<IAddon>> result;

  if (addonRepos.IsValid())
    addonRepos.BuildCompatibleVersionsList(result);

  auto end = std::chrono::steady_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
  CLog::Log(LOGDEBUG, "CAddonMgr::{} took {} ms", __func__, duration.count());

  return result;
}

bool CAddonMgr::HasAvailableUpdates()
{
  return !GetAvailableUpdates().empty();
}

std::vector<std::shared_ptr<IAddon>> CAddonMgr::GetOrphanedDependencies() const
{
  std::vector<std::shared_ptr<IAddon>> allAddons;
  GetAddonsInternal(AddonType::UNKNOWN, allAddons, OnlyEnabled::CHOICE_NO,
                    CheckIncompatible::CHOICE_YES);

  std::vector<std::shared_ptr<IAddon>> orphanedDependencies;
  for (const auto& addon : allAddons)
  {
    if (IsOrphaned(addon, allAddons))
    {
      orphanedDependencies.emplace_back(addon);
    }
  }

  return orphanedDependencies;
}

bool CAddonMgr::IsOrphaned(const std::shared_ptr<IAddon>& addon,
                           const std::vector<std::shared_ptr<IAddon>>& allAddons) const
{
  if (CServiceBroker::GetAddonMgr().IsSystemAddon(addon->ID()) ||
      !CAddonType::IsDependencyType(addon->MainType()))
    return false;

  auto dependsOnCapturedAddon = [&addon](const std::shared_ptr<IAddon>& _) {
    const auto& deps = _->GetDependencies();
    return std::any_of(deps.begin(), deps.end(),
                       [&addon](const DependencyInfo& dep) { return dep.id == addon->ID(); });
  };

  return std::none_of(allAddons.begin(), allAddons.end(), dependsOnCapturedAddon);
}

bool CAddonMgr::GetAddonsForUpdate(VECADDONS& addons) const
{
  return GetAddonsInternal(AddonType::UNKNOWN, addons, OnlyEnabled::CHOICE_YES,
                           CheckIncompatible::CHOICE_YES);
}

bool CAddonMgr::GetAddons(VECADDONS& addons) const
{
  return GetAddonsInternal(AddonType::UNKNOWN, addons, OnlyEnabled::CHOICE_YES,
                           CheckIncompatible::CHOICE_NO);
}

bool CAddonMgr::GetAddons(VECADDONS& addons, AddonType type)
{
  return GetAddonsInternal(type, addons, OnlyEnabled::CHOICE_YES, CheckIncompatible::CHOICE_NO);
}

bool CAddonMgr::GetInstalledAddons(VECADDONS& addons)
{
  return GetAddonsInternal(AddonType::UNKNOWN, addons, OnlyEnabled::CHOICE_NO,
                           CheckIncompatible::CHOICE_NO);
}

bool CAddonMgr::GetInstalledAddons(VECADDONS& addons, AddonType type)
{
  return GetAddonsInternal(type, addons, OnlyEnabled::CHOICE_NO, CheckIncompatible::CHOICE_NO);
}

bool CAddonMgr::GetDisabledAddons(VECADDONS& addons)
{
  return CAddonMgr::GetDisabledAddons(addons, AddonType::UNKNOWN);
}

bool CAddonMgr::GetDisabledAddons(VECADDONS& addons, AddonType type)
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
  return GetInstallableAddons(addons, AddonType::UNKNOWN);
}

bool CAddonMgr::GetInstallableAddons(VECADDONS& addons, AddonType type)
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  CAddonRepos addonRepos;

  if (!addonRepos.IsValid())
    return false;

  // get all addons
  addonRepos.GetLatestAddonVersions(addons);

  // go through all addons and remove all that are already installed

  addons.erase(std::remove_if(addons.begin(), addons.end(),
    [this, type](const AddonPtr& addon)
    {
      bool bErase = false;

      // check if the addon matches the provided addon type
      if (type != AddonType::UNKNOWN && addon->Type() != type && !addon->HasType(type))
        bErase = true;

      if (!this->CanAddonBeInstalled(addon))
        bErase = true;

      return bErase;
    }), addons.end());

  return true;
}

bool CAddonMgr::FindInstallableById(const std::string& addonId, AddonPtr& result)
{
  std::unique_lock<CCriticalSection> lock(m_critSection);

  CAddonRepos addonRepos(addonId);
  if (!addonRepos.IsValid())
    return false;

  AddonPtr addonToUpdate;

  // check for an update if addon is installed already

  if (GetAddon(addonId, addonToUpdate, AddonType::UNKNOWN, OnlyEnabled::CHOICE_NO))
  {
    if (addonRepos.DoAddonUpdateCheck(addonToUpdate, result))
      return true;
  }

  // get the latest version from all repos if the
  // addon is up-to-date or not installed yet

  CLog::LogFC(
      LOGDEBUG, LOGADDONS,
      "addon {} is up-to-date or not installed. falling back to get latest version from all repos",
      addonId);

  return addonRepos.GetLatestAddonVersionFromAllRepos(addonId, result);
}

bool CAddonMgr::GetAddonsInternal(AddonType type,
                                  VECADDONS& addons,
                                  OnlyEnabled onlyEnabled,
                                  CheckIncompatible checkIncompatible) const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);

  for (const auto& addonInfo : m_installedAddons)
  {
    if (type != AddonType::UNKNOWN && !addonInfo.second->HasType(type))
      continue;

    if (onlyEnabled == OnlyEnabled::CHOICE_YES &&
        ((checkIncompatible == CheckIncompatible::CHOICE_NO &&
          IsAddonDisabled(addonInfo.second->ID())) ||
         (checkIncompatible == CheckIncompatible::CHOICE_YES &&
          IsAddonDisabledExcept(addonInfo.second->ID(), AddonDisabledReason::INCOMPATIBLE))))
      continue;

    //FIXME: hack for skipping special dependency addons (xbmc.python etc.).
    //Will break if any extension point is added to them
    if (addonInfo.second->MainType() == AddonType::UNKNOWN)
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

bool CAddonMgr::GetIncompatibleEnabledAddonInfos(std::vector<AddonInfoPtr>& incompatible) const
{
  return GetIncompatibleAddonInfos(incompatible, false);
}

bool CAddonMgr::GetIncompatibleAddonInfos(std::vector<AddonInfoPtr>& incompatible,
                                          bool includeDisabled) const
{
  GetAddonInfos(incompatible, true, AddonType::UNKNOWN);
  if (includeDisabled)
    GetDisabledAddonInfos(incompatible, AddonType::UNKNOWN, AddonDisabledReason::INCOMPATIBLE);
  incompatible.erase(std::remove_if(incompatible.begin(), incompatible.end(),
                                    [this](const AddonInfoPtr& a) { return IsCompatible(a); }),
                     incompatible.end());
  return !incompatible.empty();
}

std::vector<AddonInfoPtr> CAddonMgr::MigrateAddons()
{
  // install all addon updates
  std::lock_guard<std::mutex> lock(m_installAddonsMutex);
  CLog::Log(LOGINFO, "ADDON: waiting for add-ons to update...");
  VECADDONS updates;
  GetAddonUpdateCandidates(updates);
  InstallAddonUpdates(updates, true, AllowCheckForUpdates::CHOICE_NO);

  // get addons that became incompatible and disable them
  std::vector<AddonInfoPtr> incompatible;
  GetIncompatibleAddonInfos(incompatible, true);

  return DisableIncompatibleAddons(incompatible);
}

std::vector<AddonInfoPtr> CAddonMgr::DisableIncompatibleAddons(
    const std::vector<AddonInfoPtr>& incompatible)
{
  std::vector<AddonInfoPtr> changed;
  for (const auto& addon : incompatible)
  {
    CLog::Log(LOGINFO, "ADDON: {} version {} is incompatible", addon->ID(),
              addon->Version().asString());

    if (!CAddonSystemSettings::GetInstance().UnsetActive(addon))
    {
      CLog::Log(LOGWARNING, "ADDON: failed to unset {}", addon->ID());
      continue;
    }
    if (!DisableAddon(addon->ID(), AddonDisabledReason::INCOMPATIBLE))
    {
      CLog::Log(LOGWARNING, "ADDON: failed to disable {}", addon->ID());
    }

    changed.emplace_back(addon);
  }

  return changed;
}

void CAddonMgr::CheckAndInstallAddonUpdates(bool wait) const
{
  std::lock_guard<std::mutex> lock(m_installAddonsMutex);
  VECADDONS updates;
  GetAddonUpdateCandidates(updates);
  InstallAddonUpdates(updates, wait, AllowCheckForUpdates::CHOICE_YES);
}

bool CAddonMgr::GetAddonUpdateCandidates(VECADDONS& updates) const
{
  // Get Addons in need of an update and remove all the blacklisted ones
  updates = GetAvailableUpdates();
  updates.erase(
      std::remove_if(updates.begin(), updates.end(),
                     [this](const AddonPtr& addon) { return !IsAutoUpdateable(addon->ID()); }),
      updates.end());
  return updates.empty();
}

void CAddonMgr::SortByDependencies(VECADDONS& updates) const
{
  std::vector<std::shared_ptr<ADDON::IAddon>> sorted;
  while (!updates.empty())
  {
    for (auto it = updates.begin(); it != updates.end();)
    {
      const auto& addon = *it;

      const auto& dependencies = addon->GetDependencies();
      bool addToSortedList = true;
      // if the addon has dependencies we need to check for each dependency if it also has
      // an update to be installed (and in that case, if it is already in the sorted vector).
      // if all dependency match the said conditions, the addon doesn't depend on other addons
      // waiting to be updated. Hence, the addon being processed can be installed (i.e. added to
      // the end of the sorted vector of addon updates)
      for (const auto& dep : dependencies)
      {
        auto comparator = [&dep](const std::shared_ptr<ADDON::IAddon>& addon) {
          return addon->ID() == dep.id;
        };

        if ((std::any_of(updates.begin(), updates.end(), comparator)) &&
            (!std::any_of(sorted.begin(), sorted.end(), comparator)))
        {
          addToSortedList = false;
          break;
        }
      }

      // add to the end of sorted list of addons
      if (addToSortedList)
      {
        sorted.emplace_back(addon);
        it = updates.erase(it);
      }
      else
      {
        ++it;
      }
    }
  }
  updates = sorted;
}

void CAddonMgr::InstallAddonUpdates(VECADDONS& updates,
                                    bool wait,
                                    AllowCheckForUpdates allowCheckForUpdates) const
{
  // sort addons by dependencies (ensure install order) and install all
  SortByDependencies(updates);
  CAddonInstaller::GetInstance().InstallAddons(updates, wait, allowCheckForUpdates);
}

bool CAddonMgr::GetAddon(const std::string& str,
                         AddonPtr& addon,
                         AddonType type,
                         OnlyEnabled onlyEnabled) const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);

  AddonInfoPtr addonInfo = GetAddonInfo(str, type);
  if (addonInfo)
  {
    addon = CAddonBuilder::Generate(addonInfo, type);
    if (addon)
    {
      if (onlyEnabled == OnlyEnabled::CHOICE_YES && IsAddonDisabled(addonInfo->ID()))
        return false;

      // if the addon has a running instance, grab that
      AddonPtr runningAddon = addon->GetRunningInstance();
      if (runningAddon)
        addon = runningAddon;
    }
    return nullptr != addon.get();
  }

  return false;
}

bool CAddonMgr::GetAddon(const std::string& str, AddonPtr& addon, OnlyEnabled onlyEnabled) const
{
  return GetAddon(str, addon, AddonType::UNKNOWN, onlyEnabled);
}

bool CAddonMgr::HasType(const std::string& id, AddonType type)
{
  AddonPtr addon;
  return GetAddon(id, addon, type, OnlyEnabled::CHOICE_NO);
}

bool CAddonMgr::FindAddon(const std::string& addonId,
                          const std::string& origin,
                          const CAddonVersion& addonVersion)
{
  std::map<std::string, std::shared_ptr<CAddonInfo>> installedAddons;

  FindAddons(installedAddons, "special://xbmcbin/addons");
  // Confirm special://xbmcbin/addons and special://xbmc/addons are not the same
  if (!CSpecialProtocol::ComparePath("special://xbmcbin/addons", "special://xbmc/addons"))
    FindAddons(installedAddons, "special://xbmc/addons");
  FindAddons(installedAddons, "special://home/addons");

  const auto it = installedAddons.find(addonId);
  if (it == installedAddons.cend() || it->second->Version() != addonVersion)
    return false;

  std::unique_lock<CCriticalSection> lock(m_critSection);

  m_database->GetInstallData(it->second);
  CLog::Log(LOGINFO, "CAddonMgr::{}: {} v{} installed", __FUNCTION__, addonId,
            addonVersion.asString());

  m_installedAddons[addonId] = it->second; // insert/replace entry
  m_database->AddInstalledAddon(it->second, origin);

  // Reload caches
  std::map<std::string, AddonDisabledReason> tmpDisabled;
  m_database->GetDisabled(tmpDisabled);
  m_disabled = std::move(tmpDisabled);

  m_updateRules->RefreshRulesMap(*m_database);
  return true;
}

bool CAddonMgr::FindAddons()
{
  ADDON_INFO_LIST installedAddons;

  FindAddons(installedAddons, "special://xbmcbin/addons");
  // Confirm special://xbmcbin/addons and special://xbmc/addons are not the same
  if (!CSpecialProtocol::ComparePath("special://xbmcbin/addons", "special://xbmc/addons"))
    FindAddons(installedAddons, "special://xbmc/addons");
  FindAddons(installedAddons, "special://home/addons");

  std::set<std::string> installed;
  for (const auto& addon : installedAddons)
    installed.insert(addon.second->ID());

  std::unique_lock<CCriticalSection> lock(m_critSection);

  // Sync with db
  m_database->SyncInstalled(installed, m_systemAddons, m_optionalSystemAddons);
  for (const auto& addon : installedAddons)
  {
    m_database->GetInstallData(addon.second);
    CLog::Log(LOGINFO, "CAddonMgr::{}: {} v{} installed", __FUNCTION__, addon.second->ID(),
              addon.second->Version().asString());
  }

  m_installedAddons = std::move(installedAddons);

  // Reload caches
  std::map<std::string, AddonDisabledReason> tmpDisabled;
  m_database->GetDisabled(tmpDisabled);
  m_disabled = std::move(tmpDisabled);

  m_updateRules->RefreshRulesMap(*m_database);

  return true;
}

bool CAddonMgr::UnloadAddon(const std::string& addonId)
{
  std::unique_lock<CCriticalSection> lock(m_critSection);

  if (!IsAddonInstalled(addonId))
    return true;

  AddonPtr localAddon;
  // can't unload an binary addon that is in use
  if (GetAddon(addonId, localAddon, AddonType::UNKNOWN, OnlyEnabled::CHOICE_NO) &&
      localAddon->IsBinary() && localAddon->IsInUse())
  {
    CLog::Log(LOGERROR, "CAddonMgr::{}: could not unload binary add-on {}, as is in use", __func__,
              addonId);
    return false;
  }

  m_installedAddons.erase(addonId);
  CLog::Log(LOGDEBUG, "CAddonMgr::{}: {} unloaded", __func__, addonId);

  lock.unlock();
  AddonEvents::Unload event(addonId);
  m_unloadEvents.HandleEvent(event);

  return true;
}

bool CAddonMgr::LoadAddon(const std::string& addonId,
                          const std::string& origin,
                          const CAddonVersion& addonVersion)
{
  std::unique_lock<CCriticalSection> lock(m_critSection);

  AddonPtr addon;
  if (GetAddon(addonId, addon, AddonType::UNKNOWN, OnlyEnabled::CHOICE_NO))
  {
    return true;
  }

  if (!FindAddon(addonId, origin, addonVersion))
  {
    CLog::Log(LOGERROR, "CAddonMgr: could not reload add-on {}. FindAddon failed.", addonId);
    return false;
  }

  if (!GetAddon(addonId, addon, AddonType::UNKNOWN, OnlyEnabled::CHOICE_NO))
  {
    CLog::Log(LOGERROR, "CAddonMgr: could not load add-on {}. No add-on with that ID is installed.",
              addonId);
    return false;
  }

  lock.unlock();

  AddonEvents::Load event(addon->ID());
  m_unloadEvents.HandleEvent(event);

  if (IsAddonDisabled(addon->ID()))
  {
    EnableAddon(addon->ID());
    return true;
  }

  m_events.Publish(AddonEvents::ReInstalled(addon->ID()));
  CLog::Log(LOGDEBUG, "CAddonMgr: {} successfully loaded", addon->ID());
  return true;
}

void CAddonMgr::OnPostUnInstall(const std::string& id)
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  m_disabled.erase(id);
  RemoveAllUpdateRulesFromList(id);
  m_events.Publish(AddonEvents::UnInstalled(id));
}

void CAddonMgr::UpdateLastUsed(const std::string& id)
{
  auto time = CDateTime::GetCurrentDateTime();
  CServiceBroker::GetJobManager()->Submit([this, id, time]() {
    {
      std::unique_lock<CCriticalSection> lock(m_critSection);
      m_database->SetLastUsed(id, time);
      auto addonInfo = GetAddonInfo(id, AddonType::UNKNOWN);
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
  if (!CServiceBroker::GetAddonMgr().GetAddon(addonId, addon, AddonType::UNKNOWN,
                                              OnlyEnabled::CHOICE_NO))
    missing.push_back(addonId);
  else
  {
    needed.push_back(addonId);
    for (const auto& dep : addon->GetDependencies())
      if (!dep.optional)
        ResolveDependencies(dep.id, needed, missing);
  }
}

bool CAddonMgr::DisableAddon(const std::string& id, AddonDisabledReason disabledReason)
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  if (!CanAddonBeDisabled(id))
    return false;
  if (m_disabled.find(id) != m_disabled.end())
    return true; //already disabled
  if (!m_database->DisableAddon(id, disabledReason))
    return false;
  if (!m_disabled.emplace(id, disabledReason).second)
    return false;

  //success
  CLog::Log(LOGDEBUG, "CAddonMgr: {} disabled", id);
  AddonPtr addon;
  if (GetAddon(id, addon, AddonType::UNKNOWN, OnlyEnabled::CHOICE_NO) && addon != nullptr)
  {
    auto eventLog = CServiceBroker::GetEventLog();
    if (eventLog)
      eventLog->Add(EventPtr(new CAddonManagementEvent(addon, 24141)));
  }

  m_events.Publish(AddonEvents::Disabled(id));
  return true;
}

bool CAddonMgr::UpdateDisabledReason(const std::string& id, AddonDisabledReason newDisabledReason)
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  if (!IsAddonDisabled(id))
    return false;
  if (!m_database->DisableAddon(id, newDisabledReason))
    return false;

  m_disabled[id] = newDisabledReason;

  // success
  CLog::Log(LOGDEBUG, "CAddonMgr: DisabledReason for {} updated to {}", id,
            static_cast<int>(newDisabledReason));
  return true;
}

bool CAddonMgr::EnableSingle(const std::string& id)
{
  std::unique_lock<CCriticalSection> lock(m_critSection);

  if (m_disabled.find(id) == m_disabled.end())
    return true; //already enabled

  AddonPtr addon;
  if (!GetAddon(id, addon, AddonType::UNKNOWN, OnlyEnabled::CHOICE_NO) || addon == nullptr)
    return false;

  auto eventLog = CServiceBroker::GetEventLog();

  if (!IsCompatible(addon))
  {
    CLog::Log(LOGERROR, "Add-on '{}' is not compatible with Kodi", addon->ID());
    if (eventLog)
      eventLog->AddWithNotification(
          EventPtr(new CNotificationEvent(addon->Name(), 24152, EventLevel::Error)));
    UpdateDisabledReason(addon->ID(), AddonDisabledReason::INCOMPATIBLE);
    return false;
  }

  if (!m_database->EnableAddon(id))
    return false;
  m_disabled.erase(id);

  // If enabling a repo add-on without an origin, set its origin to its own id
  if (addon->HasType(AddonType::REPOSITORY) && addon->Origin().empty())
    SetAddonOrigin(id, id, false);

  if (eventLog)
    eventLog->Add(EventPtr(new CAddonManagementEvent(addon, 24064)));

  CLog::Log(LOGDEBUG, "CAddonMgr: enabled {}", addon->ID());
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
    CLog::Log(LOGWARNING,
              "CAddonMgr: '{}' required by '{}' is missing. Add-on may not function "
              "correctly",
              dep, id);
  for (auto it = needed.rbegin(); it != needed.rend(); ++it)
    EnableSingle(*it);

  return true;
}

bool CAddonMgr::IsAddonDisabled(const std::string& ID) const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  return m_disabled.find(ID) != m_disabled.end();
}

bool CAddonMgr::IsAddonDisabledExcept(const std::string& ID,
                                      AddonDisabledReason disabledReason) const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  const auto disabledAddon = m_disabled.find(ID);
  return disabledAddon != m_disabled.end() && disabledAddon->second != disabledReason;
}

bool CAddonMgr::CanAddonBeDisabled(const std::string& ID)
{
  if (ID.empty())
    return false;

  std::unique_lock<CCriticalSection> lock(m_critSection);
  // Required system add-ons can not be disabled
  if (IsRequiredSystemAddon(ID))
    return false;

  AddonPtr localAddon;
  // can't disable an addon that isn't installed
  if (!GetAddon(ID, localAddon, AddonType::UNKNOWN, OnlyEnabled::CHOICE_NO))
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
  return GetAddon(ID, tmp, AddonType::UNKNOWN, OnlyEnabled::CHOICE_NO);
}

bool CAddonMgr::IsAddonInstalled(const std::string& ID, const std::string& origin) const
{
  AddonPtr tmp;

  if (GetAddon(ID, tmp, AddonType::UNKNOWN, OnlyEnabled::CHOICE_NO) && tmp)
  {
    if (tmp->Origin() == ORIGIN_SYSTEM)
    {
      return CAddonRepos::IsOfficialRepo(origin);
    }
    else
    {
      return tmp->Origin() == origin;
    }
  }
  return false;
}

bool CAddonMgr::IsAddonInstalled(const std::string& ID,
                                 const std::string& origin,
                                 const CAddonVersion& version)
{
  AddonPtr tmp;

  if (GetAddon(ID, tmp, AddonType::UNKNOWN, OnlyEnabled::CHOICE_NO) && tmp)
  {
    if (tmp->Origin() == ORIGIN_SYSTEM)
    {
      return CAddonRepos::IsOfficialRepo(origin) && tmp->Version() == version;
    }
    else
    {
      return tmp->Origin() == origin && tmp->Version() == version;
    }
  }
  return false;
}

bool CAddonMgr::CanAddonBeInstalled(const AddonPtr& addon)
{
  return addon != nullptr && addon->LifecycleState() != AddonLifecycleState::BROKEN &&
         !IsAddonInstalled(addon->ID());
}

bool CAddonMgr::CanUninstall(const AddonPtr& addon)
{
  return addon && CanAddonBeDisabled(addon->ID()) && !IsBundledAddon(addon->ID());
}

bool CAddonMgr::IsBundledAddon(const std::string& id)
{
  return XFILE::CDirectory::Exists(
             CSpecialProtocol::TranslatePath("special://xbmc/addons/" + id + "/")) ||
         XFILE::CDirectory::Exists(
             CSpecialProtocol::TranslatePath("special://xbmcbin/addons/" + id + "/"));
}

bool CAddonMgr::IsSystemAddon(const std::string& id)
{
  return IsOptionalSystemAddon(id) || IsRequiredSystemAddon(id);
}

bool CAddonMgr::IsRequiredSystemAddon(const std::string& id)
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  return std::find(m_systemAddons.begin(), m_systemAddons.end(), id) != m_systemAddons.end();
}

bool CAddonMgr::IsOptionalSystemAddon(const std::string& id)
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  return std::find(m_optionalSystemAddons.begin(), m_optionalSystemAddons.end(), id) !=
         m_optionalSystemAddons.end();
}

bool CAddonMgr::LoadAddonDescription(const std::string &directory, AddonPtr &addon)
{
  auto addonInfo = CAddonInfoBuilder::Generate(directory);
  if (addonInfo)
    addon = CAddonBuilder::Generate(addonInfo, AddonType::UNKNOWN);

  return addon != nullptr;
}

bool CAddonMgr::AddUpdateRuleToList(const std::string& id, AddonUpdateRule updateRule)
{
  return m_updateRules->AddUpdateRuleToList(*m_database, id, updateRule);
}

bool CAddonMgr::RemoveAllUpdateRulesFromList(const std::string& id)
{
  return m_updateRules->RemoveAllUpdateRulesFromList(*m_database, id);
}

bool CAddonMgr::RemoveUpdateRuleFromList(const std::string& id, AddonUpdateRule updateRule)
{
  return m_updateRules->RemoveUpdateRuleFromList(*m_database, id, updateRule);
}

bool CAddonMgr::IsAutoUpdateable(const std::string& id) const
{
  return m_updateRules->IsAutoUpdateable(id);
}

void CAddonMgr::PublishEventAutoUpdateStateChanged(const std::string& id)
{
  m_events.Publish(AddonEvents::AutoUpdateStateChanged(id));
}

void CAddonMgr::PublishInstanceAdded(const std::string& addonId, AddonInstanceId instanceId)
{
  m_events.Publish(AddonEvents::InstanceAdded(addonId, instanceId));
}

void CAddonMgr::PublishInstanceRemoved(const std::string& addonId, AddonInstanceId instanceId)
{
  m_events.Publish(AddonEvents::InstanceRemoved(addonId, instanceId));
}

bool CAddonMgr::IsCompatible(const std::shared_ptr<const IAddon>& addon) const
{
  for (const auto& dependency : addon->GetDependencies())
  {
    if (!dependency.optional)
    {
      // Intentionally only check the xbmc.* and kodi.* magic dependencies. Everything else will
      // not be missing anyway, unless addon was installed in an unsupported way.
      if (StringUtils::StartsWith(dependency.id, "xbmc.") ||
          StringUtils::StartsWith(dependency.id, "kodi."))
      {
        std::shared_ptr<IAddon> dep;
        const bool haveDependency =
            GetAddon(dependency.id, dep, AddonType::UNKNOWN, OnlyEnabled::CHOICE_YES);
        if (!haveDependency || !dep->MeetsVersion(dependency.versionMin, dependency.version))
          return false;
      }
    }
  }
  return true;
}

bool CAddonMgr::IsCompatible(const AddonInfoPtr& addonInfo) const
{
  for (const auto& dependency : addonInfo->GetDependencies())
  {
    if (!dependency.optional)
    {
      // Intentionally only check the xbmc.* and kodi.* magic dependencies. Everything else will
      // not be missing anyway, unless addon was installed in an unsupported way.
      if (StringUtils::StartsWith(dependency.id, "xbmc.") ||
          StringUtils::StartsWith(dependency.id, "kodi."))
      {
        AddonInfoPtr addonInfo = GetAddonInfo(dependency.id, AddonType::UNKNOWN);
        if (!addonInfo || !addonInfo->MeetsVersion(dependency.versionMin, dependency.version))
          return false;
      }
    }
  }
  return true;
}

std::vector<DependencyInfo> CAddonMgr::GetDepsRecursive(const std::string& id,
                                                        OnlyEnabledRootAddon onlyEnabledRootAddon)
{
  std::vector<DependencyInfo> added;
  AddonPtr root_addon;
  if (!FindInstallableById(id, root_addon) &&
      !GetAddon(id, root_addon, AddonType::UNKNOWN, static_cast<OnlyEnabled>(onlyEnabledRootAddon)))
  {
    return added;
  }

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

bool CAddonMgr::GetAddonInfos(AddonInfos& addonInfos, bool onlyEnabled, AddonType type) const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);

  bool forUnknown = type == AddonType::UNKNOWN;
  for (auto& info : m_installedAddons)
  {
    if (onlyEnabled && m_disabled.find(info.first) != m_disabled.end())
      continue;

    if (info.second->MainType() != AddonType::UNKNOWN && (forUnknown || info.second->HasType(type)))
      addonInfos.push_back(info.second);
  }

  return !addonInfos.empty();
}

std::vector<AddonInfoPtr> CAddonMgr::GetAddonInfos(bool onlyEnabled,
                                                   const std::vector<AddonType>& types) const
{
  std::vector<AddonInfoPtr> infos;
  if (types.empty())
    return infos;

  std::unique_lock<CCriticalSection> lock(m_critSection);

  for (auto& info : m_installedAddons)
  {
    if (onlyEnabled && m_disabled.find(info.first) != m_disabled.end())
      continue;

    if (info.second->MainType() == AddonType::UNKNOWN)
      continue;

    const auto it = std::find_if(types.begin(), types.end(),
                                 [info](AddonType t) { return info.second->HasType(t); });
    if (it != types.end())
      infos.emplace_back(info.second);
  }

  return infos;
}

bool CAddonMgr::GetDisabledAddonInfos(std::vector<AddonInfoPtr>& addonInfos, AddonType type) const
{
  return GetDisabledAddonInfos(addonInfos, type, AddonDisabledReason::NONE);
}

bool CAddonMgr::GetDisabledAddonInfos(std::vector<AddonInfoPtr>& addonInfos,
                                      AddonType type,
                                      AddonDisabledReason disabledReason) const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);

  bool forUnknown = type == AddonType::UNKNOWN;
  for (const auto& info : m_installedAddons)
  {
    const auto disabledAddon = m_disabled.find(info.first);
    if (disabledAddon == m_disabled.end())
      continue;

    if (info.second->MainType() != AddonType::UNKNOWN &&
        (forUnknown || info.second->HasType(type)) &&
        (disabledReason == AddonDisabledReason::NONE || disabledReason == disabledAddon->second))
      addonInfos.emplace_back(info.second);
  }

  return !addonInfos.empty();
}

const AddonInfoPtr CAddonMgr::GetAddonInfo(const std::string& id, AddonType type) const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);

  auto addon = m_installedAddons.find(id);
  if (addon != m_installedAddons.end())
    if ((type == AddonType::UNKNOWN || addon->second->HasType(type)))
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
      if (CFileUtils::Exists(path + "addon.xml"))
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

AddonOriginType CAddonMgr::GetAddonOriginType(const AddonPtr& addon) const
{
  if (addon->Origin() == ORIGIN_SYSTEM)
    return AddonOriginType::SYSTEM;
  else if (!addon->Origin().empty())
    return AddonOriginType::REPOSITORY;
  else
    return AddonOriginType::MANUAL;
}

bool CAddonMgr::IsAddonDisabledWithReason(const std::string& ID,
                                          AddonDisabledReason disabledReason) const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  const auto& disabledAddon = m_disabled.find(ID);
  return disabledAddon != m_disabled.end() && disabledAddon->second == disabledReason;
}

/*!
 * @brief Addon update and install management.
 */
/*@{{{*/

bool CAddonMgr::SetAddonOrigin(const std::string& addonId, const std::string& repoAddonId, bool isUpdate)
{
  std::unique_lock<CCriticalSection> lock(m_critSection);

  m_database->SetOrigin(addonId, repoAddonId);
  if (isUpdate)
    m_database->SetLastUpdated(addonId, CDateTime::GetCurrentDateTime());

  // If available in manager update
  const AddonInfoPtr info = GetAddonInfo(addonId, AddonType::UNKNOWN);
  if (info)
    m_database->GetInstallData(info);
  return true;
}

bool CAddonMgr::AddonsFromRepoXML(const RepositoryDirInfo& repo,
                                  const std::string& xml,
                                  std::vector<AddonInfoPtr>& addons)
{
  CXBMCTinyXML doc;
  if (!doc.Parse(xml))
  {
    CLog::Log(LOGERROR, "CAddonMgr::{}: Failed to parse addons.xml", __func__);
    return false;
  }

  if (doc.RootElement() == nullptr || doc.RootElement()->ValueStr() != "addons")
  {
    CLog::Log(LOGERROR, "CAddonMgr::{}: Failed to parse addons.xml. Malformed", __func__);
    return false;
  }

  // each addon XML should have a UTF-8 declaration
  auto element = doc.RootElement()->FirstChildElement("addon");
  while (element)
  {
    auto addonInfo = CAddonInfoBuilder::Generate(element, repo);
    if (addonInfo)
      addons.emplace_back(addonInfo);

    element = element->NextSiblingElement("addon");
  }

  return true;
}

/*@}}}*/

} /* namespace ADDON */
