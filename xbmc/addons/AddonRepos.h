/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "AddonDatabase.h"

#include <map>
#include <vector>

namespace ADDON
{
/**
 * Struct - RepoInfo
 */
struct RepoInfo
{
  std::string m_repoId;
  std::string m_origin;
};

class CAddonMgr;
class CRepository;
class IAddon;

/**
 * Class - CAddonRepos
 * Reads information about installed official/third party repos and their contained add-ons from the database.
 * Used to check for updates for installed add-ons and dependencies while obeying permission rules.
 * Note that this class is not responsible for refreshing the repo data stored in the database.
 */
class CAddonRepos
{
public:
  CAddonRepos(const CAddonMgr& addonMgr) : m_addonMgr(addonMgr){}; // ctor

  /*!
   * \brief Load the map of all available addon versions in any installed repository
   * \param database reference to the database to load addons from
   * \return true on success, false otherwise
   */
  bool LoadAddonsFromDatabase(const CAddonDatabase& database);

  /*!
   * \brief Load the map of all available versions of an addonId in any installed repository
   * \param database reference to the database to load addons from
   * \param addonId the addon id we want to retrieve versions for
   * \return true on success, false otherwise
   */
  bool LoadAddonsFromDatabase(const CAddonDatabase& database, const std::string& addonId);

  /*!
   * \brief Load the map of all available versions in one installed repository
   * \param database reference to the database to load addons from
   * \param repoAddon pointer to the repo we want to retrieve versions from
   *        note this is of type AddonPtr, not RepositoryPtr
   * \return true on success, false otherwise
   */
  bool LoadAddonsFromDatabase(const CAddonDatabase& database,
                              const std::shared_ptr<IAddon>& repoAddon);

  /*!
   * \brief Build the list of addons to be updated depending on defined rules
   * \param installed vector of all addons installed on the system that are
   *        checked for an update
   * \param[out] updates list of addon versions that are going to be installed
   */
  void BuildUpdateList(const std::vector<std::shared_ptr<IAddon>>& installed,
                       std::vector<std::shared_ptr<IAddon>>& updates) const;

  /*!
   * \brief Checks if the origin-repository of a given addon is defined as official repo
   *        but does not check the origin path (e.g. https://mirrors.kodi.tv ...)
   * \param addon pointer to addon to be checked
   * \return true if the repository id of a given addon is defined as official
   */
  static bool IsFromOfficialRepo(const std::shared_ptr<IAddon>& addon);

  /*!
   * \brief Checks if the origin-repository of a given addon is defined as official repo
   *        and verify if the origin-path is also defined and matching
   * \param addon pointer to addon to be checked
   * \param bCheckAddonPath also check origin path
   * \return true if the repository id of a given addon is defined as official
   *         and the addons origin matches the defined official origin of the repo id
   */
  static bool IsFromOfficialRepo(const std::shared_ptr<IAddon>& addon, bool bCheckAddonPath);

  /*!
   * \brief Check if an update is available for a single addon
   * \param addon that is checked for an update
   * \param[out] update pointer to the found update
   * \return true if an installable update was found, false otherwise
   */
  bool DoAddonUpdateCheck(const std::shared_ptr<IAddon>& addon,
                          std::shared_ptr<IAddon>& update) const;

  /*!
   * \brief Retrieves the latest version of an addon from all installed repositories
   *        follows addon origin restriction rules
   * \param addonId addon id we're looking the latest version for
   * \param[out] addon pointer to the found addon
   * \return true if a version was found, false otherwise
   */
  bool GetLatestAddonVersionFromAllRepos(const std::string& addonId,
                                         std::shared_ptr<IAddon>& addon) const;

  /*!
   * \brief Retrieves the latest official versions of addons to vector.
   *        Private versions are added obeying the set updateMode.
   *        (either OFFICIAL_ONLY or ANY_REPOSITORY)
   * \param[out] addonList retrieved addon list in a vector
   */
  void GetLatestAddonVersions(std::vector<std::shared_ptr<IAddon>>& addonList) const;

  /*!
   * \brief Find a dependency to install during an addon install or update
   *        If the dependency cannot be found in official versions we look in the
   *        installing/updating addon's (the parent's) origin repository
   * \param dependsId addon id of the dependency we're looking for
   * \param parent addon that is the dependee / parent
   * \param [out] dependencyToInstall pointer to the found dependency, only use
   *              if function returns true
   * \param [out] repoForDep the repository that dependency will install from finally
   * \return true if the dependency was found, false otherwise
   */
  bool FindDependency(const std::string& dependsId,
                      const std::shared_ptr<IAddon>& parent,
                      std::shared_ptr<IAddon>& dependencyToInstall,
                      std::shared_ptr<CRepository>& repoForDep) const;

  /*!
   * \brief Find a dependency addon in the repository of its parent
   * \param dependsId addon id of the dependency we're looking for
   * \param parent addon that is the dependee / parent
   * \param [out] dependencyToInstall pointer to the found dependency, only use
   *              if function returns true
   * \return true if the dependency was found, false otherwise
   */
  bool FindDependencyByParentRepo(const std::string& dependsId,
                                  const std::shared_ptr<IAddon>& parent,
                                  std::shared_ptr<IAddon>& dependencyToInstall) const;

private:
  /*!
   * \brief Load the map of addons
   * \note this function should only by called from publicly exposed wrappers
   * \return true on success, false otherwise
   */
  bool LoadAddonsFromDatabase(const CAddonDatabase& database,
                              const std::string& addonId,
                              const std::shared_ptr<IAddon>& repoAddon);

  /*!
   * \brief Looks up an addon in a given repository map and
   *        checks if an update is available
   * \param addonToCheck the addon we want to find and version check
   * \param map the repository map we want to check against
   * \param[out] pointer to the found update. if the addon is
   *              up-to-date on our system, this param will return 'nullptr'
   * \return true if the addon was found in the desired map,
   *         either up-to-date or newer version.
   *         false if the addon does NOT exist in the map
   */
  bool FindAddonAndCheckForUpdate(const std::shared_ptr<IAddon>& addonToCheck,
                                  const std::map<std::string, std::shared_ptr<IAddon>>& map,
                                  std::shared_ptr<IAddon>& update) const;

  /*!
   * \brief Sets up latest version maps from scratch
   */
  void SetupLatestVersionMaps();

  /*!
   * \brief Adds the latest version of an addon to the desired map
   * \param addonToAdd the addon whose latest version should be added
   * \param map target map, e.g. latestOfficialVersions or latestPrivateVersions
   */
  void AddAddonIfLatest(const std::shared_ptr<IAddon>& addonToAdd,
                        std::map<std::string, std::shared_ptr<IAddon>>& map) const;

  /*!
   * \brief Adds the latest version of an addon to the desired map per repository
   *        used to populate 'latestVersionsByRepo'
   * \param repoId the repository that addon comes from
   * \param addonToAdd the addon whose latest version should be added
   * \param map target map, latestVersionsByRepo
   */
  void AddAddonIfLatest(
      const std::string& repoId,
      const std::shared_ptr<IAddon>& addonToAdd,
      std::map<std::string, std::map<std::string, std::shared_ptr<IAddon>>>& map) const;

  /*!
   * \brief Looks up an addon entry in a specific map
   * \param addonId addon we want to retrieve
   * \param map the map we're looking into for the wanted addon
   * \param[out] addon pointer to the found addon, only use when function returns true
   * \return true if the addon was found in the map, false otherwise
   */
  bool GetLatestVersionByMap(const std::string& addonId,
                             const std::map<std::string, std::shared_ptr<IAddon>>& map,
                             std::shared_ptr<IAddon>& addon) const;

  const CAddonMgr& m_addonMgr;

  std::vector<std::shared_ptr<IAddon>> m_allAddons;

  std::map<std::string, std::shared_ptr<IAddon>> m_latestOfficialVersions;
  std::map<std::string, std::shared_ptr<IAddon>> m_latestPrivateVersions;
  std::map<std::string, std::map<std::string, std::shared_ptr<IAddon>>> m_latestVersionsByRepo;
  std::map<std::string, std::multimap<std::string, std::shared_ptr<IAddon>>> m_addonsByRepoMap;
};

}; /* namespace ADDON */
