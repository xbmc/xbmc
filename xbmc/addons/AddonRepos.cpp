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

#include <algorithm>
#include <vector>

using namespace ADDON;

static std::vector<RepoInfo> officialRepoInfos = CCompileInfo::LoadOfficialRepoInfos();

/**********************************************************
 * CAddonRepos
 *
 */

bool CAddonRepos::IsFromOfficialRepo(const std::shared_ptr<IAddon>& addon,
                                     CheckAddonPath checkAddonPath)
{
  auto comparator = [&](const RepoInfo& officialRepo) {
    if (checkAddonPath == CheckAddonPath::CHOICE_YES)
    {
      return (addon->Origin() == officialRepo.m_repoId &&
              StringUtils::StartsWithNoCase(addon->Path(), officialRepo.m_origin));
    }

    return addon->Origin() == officialRepo.m_repoId;
  };

  return addon->Origin() == ORIGIN_SYSTEM ||
         std::any_of(officialRepoInfos.begin(), officialRepoInfos.end(), comparator);
}

bool CAddonRepos::IsOfficialRepo(const std::string& repoId)
{
  return repoId == ORIGIN_SYSTEM || std::any_of(officialRepoInfos.begin(), officialRepoInfos.end(),
                                                [&repoId](const RepoInfo& officialRepo) {
                                                  return repoId == officialRepo.m_repoId;
                                                });
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

      if (IsFromOfficialRepo(addonToAdd, CheckAddonPath::CHOICE_YES))
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

void CAddonRepos::BuildUpdateOrOutdatedList(const std::vector<std::shared_ptr<IAddon>>& installed,
                                            std::vector<std::shared_ptr<IAddon>>& result,
                                            AddonCheckType addonCheckType) const
{
  std::shared_ptr<IAddon> update;

  CLog::Log(LOGDEBUG, "CAddonRepos::{}: Building {} list from installed add-ons", __func__,
            addonCheckType == AddonCheckType::OUTDATED_ADDONS ? "outdated" : "update");

  for (const auto& addon : installed)
  {
    if (DoAddonUpdateCheck(addon, update))
    {
      result.emplace_back(addonCheckType == AddonCheckType::OUTDATED_ADDONS ? addon : update);
    }
  }
}

void CAddonRepos::BuildAddonsWithUpdateList(
    const std::vector<std::shared_ptr<IAddon>>& installed,
    std::map<std::string, CAddonWithUpdate>& addonsWithUpdate) const
{
  std::shared_ptr<IAddon> update;

  CLog::Log(LOGDEBUG,
            "CAddonRepos::{}: Building combined addons-with-update map from installed add-ons",
            __func__);

  for (const auto& addon : installed)
  {
    if (DoAddonUpdateCheck(addon, update))
    {
      addonsWithUpdate.insert({addon->ID(), {addon, update}});
    }
  }
}

bool CAddonRepos::DoAddonUpdateCheck(const std::shared_ptr<IAddon>& addon,
                                     std::shared_ptr<IAddon>& update) const
{
  CLog::Log(LOGDEBUG, "ADDONS: update check: addonID = {} / Origin = {} / Version = {}",
            addon->ID(), addon->Origin(), addon->Version().asString());

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
      if (IsFromOfficialRepo(addon, CheckAddonPath::CHOICE_YES)) // is an official addon
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

void CAddonRepos::GetLatestAddonVersionsFromAllRepos(
    std::vector<std::shared_ptr<IAddon>>& addonList) const
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

  // then we insert latest version per addon and repository if they don't exist in the official map
  // or installation from ANY_REPOSITORY is allowed and the private version is higher

  for (const auto& repo : m_latestVersionsByRepo)
  {
    // content of official repos is stored in m_latestVersionsByRepo too
    // so we need to filter them out

    if (std::none_of(officialRepoInfos.begin(), officialRepoInfos.end(),
                     [&](const ADDON::RepoInfo& officialRepo) {
                       return repo.first == officialRepo.m_repoId;
                     }))
    {
      for (const auto& latestAddon : repo.second)
      {
        const auto& officialVersion = m_latestOfficialVersions.find(latestAddon.first);
        if (officialVersion == m_latestOfficialVersions.end() ||
            (updateMode == AddonRepoUpdateMode::ANY_REPOSITORY &&
             latestAddon.second->Version() > officialVersion->second->Version()))
        {
          addonList.emplace_back(latestAddon.second);
        }
      }
    }
  }
}

bool CAddonRepos::FindDependency(const std::string& dependsId,
                                 const std::string& parentRepoId,
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
    if (!FindDependencyByParentRepo(dependsId, parentRepoId, dependencyToInstall))
      return false;
  }

  // we got the dependency, so now get a repository-pointer to return

  std::shared_ptr<IAddon> tmp;
  if (!m_addonMgr.GetAddon(dependencyToInstall->Origin(), tmp, ADDON_REPOSITORY,
                           OnlyEnabled::CHOICE_YES))
    return false;

  repoForDep = std::static_pointer_cast<CRepository>(tmp);

  CLog::Log(LOGDEBUG, "ADDONS: found dependency [{}] for install/update from repo [{}]",
            dependencyToInstall->ID(), repoForDep->ID());

  if (dependencyToInstall->HasType(ADDON_REPOSITORY))
  {
    CLog::Log(LOGDEBUG,
              "ADDONS: dependency with id [{}] has type ADDON_REPOSITORY and will not install!",
              dependencyToInstall->ID());

    return false;
  }

  return true;
}

bool CAddonRepos::FindDependencyByParentRepo(const std::string& dependsId,
                                             const std::string& parentRepoId,
                                             std::shared_ptr<IAddon>& dependencyToInstall) const
{
  const auto& repoEntry = m_latestVersionsByRepo.find(parentRepoId);
  if (repoEntry != m_latestVersionsByRepo.end())
  {
    if (GetLatestVersionByMap(dependsId, repoEntry->second, dependencyToInstall))
      return true;
  }

  return false;
}

void CAddonRepos::BuildCompatibleVersionsList(
    std::vector<std::shared_ptr<IAddon>>& compatibleVersions) const
{
  std::vector<std::shared_ptr<IAddon>> officialVersions;
  std::vector<std::shared_ptr<IAddon>> privateVersions;

  for (const auto& addon : m_allAddons)
  {
    if (m_addonMgr.IsCompatible(*addon))
    {
      if (IsFromOfficialRepo(addon, CheckAddonPath::CHOICE_YES))
      {
        officialVersions.emplace_back(addon);
      }
      else
      {
        privateVersions.emplace_back(addon);
      }
    }
  }

  auto comparator = [](const std::shared_ptr<IAddon>& a, const std::shared_ptr<IAddon>& b) {
    return (a->Version() > b->Version());
  };

  std::sort(officialVersions.begin(), officialVersions.end(), comparator);
  std::sort(privateVersions.begin(), privateVersions.end(), comparator);

  compatibleVersions = std::move(officialVersions);
  std::move(privateVersions.begin(), privateVersions.end(), std::back_inserter(compatibleVersions));
}
