/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AddonRepos.h"

#include "Addon.h"
#include "AddonDatabase.h"
#include "AddonManager.h"
#include "AddonRepoInfo.h"
#include "AddonSystemSettings.h"
#include "CompileInfo.h"
#include "Repository.h"
#include "RepositoryUpdater.h"
#include "ServiceBroker.h"
#include "messaging/helpers/DialogOKHelper.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#include <vector>

using namespace ADDON;

static std::vector<RepoInfo> officialRepoInfos = CCompileInfo::LoadOfficialRepoInfos();

/**********************************************************
 * CAddonRepos
 *
 */

bool CAddonRepos::IsFromOfficialRepo(const std::shared_ptr<IAddon>& addon)
{
  return IsFromOfficialRepo(addon, false);
}

bool CAddonRepos::IsFromOfficialRepo(const std::shared_ptr<IAddon>& addon, bool bCheckAddonPath)
{
  auto comparator = [&](const RepoInfo& officialRepo) {
    if (bCheckAddonPath)
    {
      return (addon->Origin() == officialRepo.m_repoId &&
              StringUtils::StartsWithNoCase(addon->Path(), officialRepo.m_origin));
    }

    return addon->Origin() == officialRepo.m_repoId;
  };

  return addon->Origin() == ORIGIN_SYSTEM ||
         std::any_of(officialRepoInfos.begin(), officialRepoInfos.end(), comparator);
}

bool CAddonRepos::LoadAddonsFromDatabase(const CAddonDatabase& database)
{
  return LoadAddonsFromDatabase(database, "", nullptr);
}

bool CAddonRepos::LoadAddonsFromDatabase(const CAddonDatabase& database, const std::string& addonId)
{
  return LoadAddonsFromDatabase(database, addonId, nullptr);
}

bool CAddonRepos::LoadAddonsFromDatabase(const CAddonDatabase& database,
                                         const std::shared_ptr<IAddon>& repoAddon)
{
  return LoadAddonsFromDatabase(database, "", repoAddon);
}

bool CAddonRepos::LoadAddonsFromDatabase(const CAddonDatabase& database,
                                         const std::string& addonId,
                                         const std::shared_ptr<IAddon>& repoAddon)
{
  m_allAddons.clear();

  if (repoAddon)
  {
    if (!database.GetRepositoryContent(repoAddon->ID(), m_allAddons))
    {
      // Repo content is invalid. Ask for update and wait.
      CServiceBroker::GetRepositoryUpdater().CheckForUpdates(
          std::static_pointer_cast<CRepository>(repoAddon));
      CServiceBroker::GetRepositoryUpdater().Await();

      if (!database.GetRepositoryContent(repoAddon->ID(), m_allAddons))
      {
        KODI::MESSAGING::HELPERS::ShowOKDialogText(CVariant{repoAddon->Name()}, CVariant{24991});
        return false;
      }
    }
  }
  else if (addonId.empty())
  {
    // load full repository content
    database.GetRepositoryContent(m_allAddons);
  }
  else
  {
    // load specific addonId only
    database.FindByAddonId(addonId, m_allAddons);
  }

  m_addonsByRepoMap.clear();
  for (const auto& addon : m_allAddons)
  {
    if (m_addonMgr.IsCompatible(*addon))
    {
      m_addonsByRepoMap[addon->Origin()].insert({addon->ID(), addon});
    }
  }

  for (const auto& map : m_addonsByRepoMap)
    CLog::Log(LOGDEBUG, "ADDONS: repo: {} - {} addon(s) loaded", map.first, map.second.size());

  SetupLatestVersionMaps();

  return true;
}

void CAddonRepos::SetupLatestVersionMaps()
{
  m_latestOfficialVersions.clear();
  m_latestPrivateVersions.clear();
  m_latestVersionsByRepo.clear();

  for (const auto& repo : m_addonsByRepoMap)
  {
    const auto& addonsPerRepo = repo.second;

    for (const auto& addonMapEntry : addonsPerRepo)
    {
      const auto& addonToAdd = addonMapEntry.second;

      if (IsFromOfficialRepo(addonToAdd, true))
      {
        AddAddonIfLatest(addonToAdd, m_latestOfficialVersions);
      }
      else
      {
        AddAddonIfLatest(addonToAdd, m_latestPrivateVersions);
      }

      // add to latestVersionsByRepo
      AddAddonIfLatest(repo.first, addonToAdd, m_latestVersionsByRepo);
    }
  }
}

void CAddonRepos::AddAddonIfLatest(const std::shared_ptr<IAddon>& addonToAdd,
                                   std::map<std::string, std::shared_ptr<IAddon>>& map) const
{
  const auto& latestKnown = map.find(addonToAdd->ID());
  if (latestKnown == map.end() || addonToAdd->Version() > latestKnown->second->Version())
    map[addonToAdd->ID()] = addonToAdd;
}

void CAddonRepos::AddAddonIfLatest(
    const std::string& repoId,
    const std::shared_ptr<IAddon>& addonToAdd,
    std::map<std::string, std::map<std::string, std::shared_ptr<IAddon>>>& map) const
{
  const auto& latestVersionByRepo = map.find(repoId);

  if (latestVersionByRepo == map.end()) // repo not found
  {
    map[repoId].insert({addonToAdd->ID(), addonToAdd});
  }
  else
  {
    const auto& latestVersionEntryByRepo = latestVersionByRepo->second;
    const auto& latestKnown = latestVersionEntryByRepo.find(addonToAdd->ID());

    if (latestKnown == latestVersionEntryByRepo.end() ||
        addonToAdd->Version() > latestKnown->second->Version())
      map[repoId][addonToAdd->ID()] = addonToAdd;
  }
}

void CAddonRepos::BuildUpdateList(const std::vector<std::shared_ptr<IAddon>>& installed,
                                  std::vector<std::shared_ptr<IAddon>>& updates) const
{
  std::shared_ptr<IAddon> update;

  CLog::Log(LOGDEBUG, "ADDONS: *** building update list (installed add-ons) ***");

  for (const auto& addon : installed)
  {
    if (DoAddonUpdateCheck(addon, update))
    {
      updates.emplace_back(update);
    }
  }
}

bool CAddonRepos::DoAddonUpdateCheck(const std::shared_ptr<IAddon>& addon,
                                     std::shared_ptr<IAddon>& update) const
{
  CLog::Log(LOGDEBUG, "ADDONS: update check: addonID = {} / Origin = {}", addon->ID(),
            addon->Origin());

  update.reset();

  const AddonRepoUpdateMode updateMode =
      CAddonSystemSettings::GetInstance().GetAddonRepoUpdateMode();

  bool hasOfficialUpdate = FindAddonAndCheckForUpdate(addon, m_latestOfficialVersions, update);

  // addons with an empty origin have at least been checked against official repositories
  if (!addon->Origin().empty())
  {
    if (ORIGIN_SYSTEM != addon->Origin() && !hasOfficialUpdate) // not a system addon
    {
      // If we didn't find an official update
      if (IsFromOfficialRepo(addon, true)) // is an official addon
      {
        if (updateMode == AddonRepoUpdateMode::ANY_REPOSITORY)
        {
          if (!FindAddonAndCheckForUpdate(addon, m_latestPrivateVersions, update))
          {
            return false;
          }
        }
      }
      else
      {
        // ...we check for updates in the origin repo only
        const auto& repoEntry = m_latestVersionsByRepo.find(addon->Origin());
        if (repoEntry != m_latestVersionsByRepo.end())
        {
          if (!FindAddonAndCheckForUpdate(addon, repoEntry->second, update))
          {
            return false;
          }
        }
      }
    }
  }

  if (update != nullptr)
  {
    CLog::Log(LOGDEBUG, "ADDONS: -- found -->: addonID = {} / Origin = {} / Version = {}",
              update->ID(), update->Origin(), update->Version().asString());
    return true;
  }

  return false;
}

bool CAddonRepos::FindAddonAndCheckForUpdate(
    const std::shared_ptr<IAddon>& addonToCheck,
    const std::map<std::string, std::shared_ptr<IAddon>>& map,
    std::shared_ptr<IAddon>& update) const
{
  const auto& remote = map.find(addonToCheck->ID());
  if (remote != map.end()) // is addon in the desired map?
  {
    if ((remote->second->Version() > addonToCheck->Version()) ||
        m_addonMgr.IsAddonDisabledWithReason(addonToCheck->ID(), AddonDisabledReason::INCOMPATIBLE))
    {
      // return addon update
      update = remote->second;
    }
    else
    {
      // addon found, but it's up to date
      update = nullptr;
    }
    return true;
  }

  return false;
}

bool CAddonRepos::GetLatestVersionByMap(const std::string& addonId,
                                        const std::map<std::string, std::shared_ptr<IAddon>>& map,
                                        std::shared_ptr<IAddon>& addon) const
{
  const auto& remote = map.find(addonId);
  if (remote != map.end()) // is addon in the desired map?
  {
    addon = remote->second;
    return true;
  }

  return false;
}

bool CAddonRepos::GetLatestAddonVersionFromAllRepos(const std::string& addonId,
                                                    std::shared_ptr<IAddon>& addon) const
{
  const AddonRepoUpdateMode updateMode =
      CAddonSystemSettings::GetInstance().GetAddonRepoUpdateMode();

  bool hasOfficialVersion = GetLatestVersionByMap(addonId, m_latestOfficialVersions, addon);

  if (hasOfficialVersion)
  {
    if (updateMode == AddonRepoUpdateMode::ANY_REPOSITORY)
    {
      std::shared_ptr<IAddon> thirdPartyAddon;

      // only use this version if it's higher than the official one
      if (GetLatestVersionByMap(addonId, m_latestPrivateVersions, thirdPartyAddon))
      {
        if (thirdPartyAddon->Version() > addon->Version())
          addon = thirdPartyAddon;
      }
    }
  }
  else
  {
    if (!GetLatestVersionByMap(addonId, m_latestPrivateVersions, addon))
      return false;
  }

  return true;
}

void CAddonRepos::GetLatestAddonVersions(std::vector<std::shared_ptr<IAddon>>& addonList) const
{
  const AddonRepoUpdateMode updateMode =
      CAddonSystemSettings::GetInstance().GetAddonRepoUpdateMode();

  addonList.clear();

  // first we insert all official addon versions into the resulting vector

  std::transform(m_latestOfficialVersions.begin(), m_latestOfficialVersions.end(),
                 back_inserter(addonList),
                 [](const std::pair<std::string, std::shared_ptr<IAddon>>& officialVersion) {
                   return officialVersion.second;
                 });

  // then we insert private addon versions if they don't exist in the official map
  // or installation from ANY_REPOSITORY is allowed and the private version is higher

  for (const auto& privateVersion : m_latestPrivateVersions)
  {
    const auto& officialVersion = m_latestOfficialVersions.find(privateVersion.first);
    if (officialVersion == m_latestOfficialVersions.end() ||
        (updateMode == AddonRepoUpdateMode::ANY_REPOSITORY &&
         privateVersion.second->Version() > officialVersion->second->Version()))
    {
      addonList.emplace_back(privateVersion.second);
    }
  }
}

bool CAddonRepos::FindDependency(const std::string& dependsId,
                                 const std::shared_ptr<IAddon>& parent,
                                 std::shared_ptr<IAddon>& dependencyToInstall,
                                 std::shared_ptr<CRepository>& repoForDep) const
{
  const AddonRepoUpdateMode updateMode =
      CAddonSystemSettings::GetInstance().GetAddonRepoUpdateMode();

  bool dependencyHasOfficialVersion =
      GetLatestVersionByMap(dependsId, m_latestOfficialVersions, dependencyToInstall);

  if (dependencyHasOfficialVersion)
  {
    if (updateMode == AddonRepoUpdateMode::ANY_REPOSITORY)
    {
      std::shared_ptr<IAddon> thirdPartyDependency;

      // only use this version if it's higher than the official one
      if (GetLatestVersionByMap(dependsId, m_latestPrivateVersions, thirdPartyDependency))
      {
        if (thirdPartyDependency->Version() > dependencyToInstall->Version())
          dependencyToInstall = thirdPartyDependency;
      }
    }
  }
  else
  {
    // If we didn't find an official version of this dependency
    // ...we check in the origin repo of the parent
    if (!FindDependencyByParentRepo(dependsId, parent, dependencyToInstall))
      return false;
  }

  // we got the dependency, so now get a repository-pointer to return

  std::shared_ptr<IAddon> tmp;
  if (!m_addonMgr.GetAddon(dependencyToInstall->Origin(), tmp, ADDON_REPOSITORY))
    return false;

  repoForDep = std::static_pointer_cast<CRepository>(tmp);

  CLog::Log(LOGDEBUG,
            "ADDONS: found dependency [{}] for install/update from repo [{}]. dependee is [{}]",
            dependencyToInstall->ID(), repoForDep->ID(), parent->ID());

  return true;
}

bool CAddonRepos::FindDependencyByParentRepo(const std::string& dependsId,
                                             const std::shared_ptr<IAddon>& parent,
                                             std::shared_ptr<IAddon>& dependencyToInstall) const
{
  const auto& repoEntry = m_latestVersionsByRepo.find(parent->Origin());
  if (repoEntry != m_latestVersionsByRepo.end())
  {
    if (GetLatestVersionByMap(dependsId, repoEntry->second, dependencyToInstall))
      return true;
  }

  return false;
}
