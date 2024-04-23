/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AddonRepos.h"

#include "CompileInfo.h"
#include "ServiceBroker.h"
#include "addons/Addon.h"
#include "addons/AddonManager.h"
#include "addons/AddonRepoInfo.h"
#include "addons/AddonSystemSettings.h"
#include "addons/Repository.h"
#include "addons/RepositoryUpdater.h"
#include "addons/addoninfo/AddonInfo.h"
#include "addons/addoninfo/AddonType.h"
#include "messaging/helpers/DialogOKHelper.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#include <algorithm>
#include <vector>

namespace
{
constexpr auto ALL_ADDON_IDS = "";
constexpr auto ALL_REPOSITORIES = nullptr;
} // anonymous namespace

using namespace ADDON;

static std::vector<RepoInfo> officialRepoInfos = CCompileInfo::LoadOfficialRepoInfos();

/**********************************************************
 * CAddonRepos
 *
 */

CAddonRepos::CAddonRepos() : m_addonMgr(CServiceBroker::GetAddonMgr())
{
  m_valid = m_addonDb.Open() && LoadAddonsFromDatabase(ALL_ADDON_IDS, ALL_REPOSITORIES);
}

CAddonRepos::CAddonRepos(const std::string& addonId) : m_addonMgr(CServiceBroker::GetAddonMgr())
{
  m_valid = m_addonDb.Open() && LoadAddonsFromDatabase(addonId, ALL_REPOSITORIES);
}

CAddonRepos::CAddonRepos(const std::shared_ptr<IAddon>& repoAddon)
  : m_addonMgr(CServiceBroker::GetAddonMgr())
{
  m_valid = m_addonDb.Open() && LoadAddonsFromDatabase(ALL_ADDON_IDS, repoAddon);
}

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

bool CAddonRepos::LoadAddonsFromDatabase(const std::string& addonId,
                                         const std::shared_ptr<IAddon>& repoAddon)
{
  if (repoAddon != ALL_REPOSITORIES)
  {
    if (!m_addonDb.GetRepositoryContent(repoAddon->ID(), m_allAddons))
    {
      // Repo content is invalid. Ask for update and wait.
      CServiceBroker::GetRepositoryUpdater().CheckForUpdates(
          std::static_pointer_cast<CRepository>(repoAddon));
      CServiceBroker::GetRepositoryUpdater().Await();

      if (!m_addonDb.GetRepositoryContent(repoAddon->ID(), m_allAddons))
      {

        // could not connect to repository
        KODI::MESSAGING::HELPERS::ShowOKDialogText(CVariant{repoAddon->Name()}, CVariant{24991});
        return false;
      }
    }
  }
  else if (addonId == ALL_ADDON_IDS)
  {
    // load full repository content
    m_addonDb.GetRepositoryContent(m_allAddons);
    if (m_allAddons.empty())
      return true;
  }
  else
  {
    // load specific addonId only
    m_addonDb.FindByAddonId(addonId, m_allAddons);
  }

  if (m_allAddons.empty())
    return false;

  for (const auto& addon : m_allAddons)
  {
    if (m_addonMgr.IsCompatible(addon))
    {
      m_addonsByRepoMap[addon->Origin()].emplace(addon->ID(), addon);
    }
  }

  for (const auto& [repoId, addonsPerRepo] : m_addonsByRepoMap)
  {
    CLog::LogFC(LOGDEBUG, LOGADDONS, "{} - {} addon(s) loaded", repoId, addonsPerRepo.size());

    for (const auto& [addonId, addonToAdd] : addonsPerRepo)
    {
      if (IsFromOfficialRepo(addonToAdd, CheckAddonPath::CHOICE_YES))
      {
        AddAddonIfLatest(addonToAdd, m_latestOfficialVersions);
      }
      else
      {
        AddAddonIfLatest(addonToAdd, m_latestPrivateVersions);
      }

      // add to latestVersionsByRepo
      AddAddonIfLatest(repoId, addonToAdd, m_latestVersionsByRepo);
    }
  }

  return true;
}

void CAddonRepos::AddAddonIfLatest(const std::shared_ptr<IAddon>& addonToAdd,
                                   std::map<std::string, std::shared_ptr<IAddon>>& map) const
{
  const auto latestKnownIt = map.find(addonToAdd->ID());
  if (latestKnownIt == map.end() || addonToAdd->Version() > latestKnownIt->second->Version())
    map[addonToAdd->ID()] = addonToAdd;
}

void CAddonRepos::AddAddonIfLatest(
    const std::string& repoId,
    const std::shared_ptr<IAddon>& addonToAdd,
    std::map<std::string, std::map<std::string, std::shared_ptr<IAddon>>>& map) const
{
  bool doInsert{true};

  const auto latestVersionByRepoIt = map.find(repoId);
  if (latestVersionByRepoIt != map.end()) // we already have this repository in the outer map
  {
    const auto& latestVersionEntryByRepo = latestVersionByRepoIt->second;
    const auto latestKnownIt = latestVersionEntryByRepo.find(addonToAdd->ID());

    if (latestKnownIt != latestVersionEntryByRepo.end() &&
        addonToAdd->Version() <= latestKnownIt->second->Version())
    {
      doInsert = false;
    }
  }

  if (doInsert)
    map[repoId][addonToAdd->ID()] = addonToAdd;
}

void CAddonRepos::BuildUpdateOrOutdatedList(const std::vector<std::shared_ptr<IAddon>>& installed,
                                            std::vector<std::shared_ptr<IAddon>>& result,
                                            AddonCheckType addonCheckType) const
{
  std::shared_ptr<IAddon> update;

  CLog::LogFC(LOGDEBUG, LOGADDONS, "Building {} list from installed add-ons",
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
    std::map<std::string, AddonWithUpdate>& addonsWithUpdate) const
{
  std::shared_ptr<IAddon> update;

  CLog::LogFC(LOGDEBUG, LOGADDONS,
              "Building combined addons-with-update map from installed add-ons");
  for (const auto& addon : installed)
  {
    if (DoAddonUpdateCheck(addon, update))
    {
      addonsWithUpdate.try_emplace(addon->ID(), addon, update);
    }
  }
}

bool CAddonRepos::DoAddonUpdateCheck(const std::shared_ptr<IAddon>& addon,
                                     std::shared_ptr<IAddon>& update) const
{
  CLog::LogFC(LOGDEBUG, LOGADDONS, "update check: addonID = {} / Origin = {} / Version = {}",
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

      // we didn't find an official update.
      // either version is current or that add-on isn't contained in official repos
      if (IsFromOfficialRepo(addon, CheckAddonPath::CHOICE_NO))
      {

        // check further if it IS contained in official repos
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
        const auto repoEntryIt = m_latestVersionsByRepo.find(addon->Origin());
        if (repoEntryIt != m_latestVersionsByRepo.end())
        {
          if (!FindAddonAndCheckForUpdate(addon, repoEntryIt->second, update))
          {
            return false;
          }
        }
      }
    }
  }

  if (update != nullptr)
  {
    CLog::LogFC(LOGDEBUG, LOGADDONS, "-- found -->: addonID = {} / Origin = {} / Version = {}",
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
  const auto remoteIt = map.find(addonToCheck->ID());
  if (remoteIt != map.end()) // is addon in the desired map?
  {
    if ((remoteIt->second->Version() > addonToCheck->Version()) ||
        m_addonMgr.IsAddonDisabledWithReason(addonToCheck->ID(), AddonDisabledReason::INCOMPATIBLE))
    {
      // return addon update
      update = remoteIt->second;
      return true; // update found
    }
  }

  // either addon wasn't found or it's up to date
  return false;
}

bool CAddonRepos::GetLatestVersionByMap(const std::string& addonId,
                                        const std::map<std::string, std::shared_ptr<IAddon>>& map,
                                        std::shared_ptr<IAddon>& addon) const
{
  const auto remoteIt = map.find(addonId);
  if (remoteIt != map.end()) // is addon in the desired map?
  {
    addon = remoteIt->second;
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

  for (const auto& officialVersion : m_latestOfficialVersions)
    addonList.emplace_back(officialVersion.second);

  // then we insert private addon versions if they don't exist in the official map
  // or installation from ANY_REPOSITORY is allowed and the private version is higher

  for (const auto& [privateVersionId, privateVersion] : m_latestPrivateVersions)
  {
    const auto officialVersionIt = m_latestOfficialVersions.find(privateVersionId);

    if (officialVersionIt == m_latestOfficialVersions.end() ||
        (updateMode == AddonRepoUpdateMode::ANY_REPOSITORY &&
         privateVersion->Version() > officialVersionIt->second->Version()))
    {
      addonList.emplace_back(privateVersion);
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

  for (const auto& officialVersion : m_latestOfficialVersions)
    addonList.emplace_back(officialVersion.second);

  // then we insert latest version per addon and repository if they don't exist in the official map
  // or installation from ANY_REPOSITORY is allowed and the private version is higher

  for (const auto& repo : m_latestVersionsByRepo)
  {
    // content of official repos is stored in m_latestVersionsByRepo too
    // so we need to filter them out

    if (std::none_of(officialRepoInfos.begin(), officialRepoInfos.end(),
                     [&repo](const ADDON::RepoInfo& officialRepo)
                     { return repo.first == officialRepo.m_repoId; }))
    {
      for (const auto& [latestAddonId, latestAddon] : repo.second)
      {
        const auto officialVersionIt = m_latestOfficialVersions.find(latestAddonId);

        if (officialVersionIt == m_latestOfficialVersions.end() ||
            (updateMode == AddonRepoUpdateMode::ANY_REPOSITORY &&
             latestAddon->Version() > officialVersionIt->second->Version()))
        {
          addonList.emplace_back(latestAddon);
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
  if (!m_addonMgr.GetAddon(dependencyToInstall->Origin(), tmp, AddonType::REPOSITORY,
                           OnlyEnabled::CHOICE_YES))
    return false;

  repoForDep = std::static_pointer_cast<CRepository>(tmp);

  CLog::LogFC(LOGDEBUG, LOGADDONS, "found dependency [{}] for install/update from repo [{}]",
              dependencyToInstall->ID(), repoForDep->ID());

  if (dependencyToInstall->HasType(AddonType::REPOSITORY))
  {
    CLog::LogFC(LOGDEBUG, LOGADDONS,
                "dependency with id [{}] has type ADDON_REPOSITORY and will not install!",
                dependencyToInstall->ID());

    return false;
  }

  return true;
}

bool CAddonRepos::FindDependencyByParentRepo(const std::string& dependsId,
                                             const std::string& parentRepoId,
                                             std::shared_ptr<IAddon>& dependencyToInstall) const
{
  const auto repoEntryIt = m_latestVersionsByRepo.find(parentRepoId);
  if (repoEntryIt != m_latestVersionsByRepo.end())
  {
    if (GetLatestVersionByMap(dependsId, repoEntryIt->second, dependencyToInstall))
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
    if (m_addonMgr.IsCompatible(addon))
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
