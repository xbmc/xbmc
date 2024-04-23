/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "addons/AddonDatabase.h"

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace ADDON
{

class CAddonVersion;
class CAddonMgr;
class CRepository;
class IAddon;
enum class AddonCheckType : bool;

enum class CheckAddonPath
{
  CHOICE_YES = true,
  CHOICE_NO = false,
};

using AddonWithUpdate = std::pair<std::shared_ptr<IAddon>, std::shared_ptr<IAddon>>;

/**
 * Class - CAddonRepos
 * Reads information about installed official/third party repos and their contained add-ons from the database.
 * Used to check for updates for installed add-ons and dependencies while obeying permission rules.
 * Note that this class is not responsible for refreshing the repo data stored in the database.
 */
class CAddonRepos
{
public:
  CAddonRepos(); // load all add-ons from all installed repositories
  explicit CAddonRepos(const std::string& addonId); // load a specific add-on id only
  explicit CAddonRepos(const std::shared_ptr<IAddon>& repoAddon); // load add-ons of a specific repo

  /*!
   * \brief Build the list of addons to be updated depending on defined rules
   *        or the list of outdated addons
   * \param installed vector of all addons installed on the system that are
   *        checked for an update
   * \param[in] addonCheckType build list of OUTDATED or UPDATES
   * \param[out] result list of addon versions that are going to be installed
   *             or are outdated
   */
  void BuildUpdateOrOutdatedList(const std::vector<std::shared_ptr<IAddon>>& installed,
                                 std::vector<std::shared_ptr<IAddon>>& result,
                                 AddonCheckType addonCheckType) const;

  /*!
   * \brief Build the list of outdated addons and their available updates.
   * \param installed vector of all addons installed on the system that are
   *        checked for an update
   * \param[out] addonsWithUpdate target map
   */
  void BuildAddonsWithUpdateList(const std::vector<std::shared_ptr<IAddon>>& installed,
                                 std::map<std::string, AddonWithUpdate>& addonsWithUpdate) const;

  /*!
   * \brief Checks if the origin-repository of a given addon is defined as official repo
   *        and can also verify if the origin-path (e.g. https://mirrors.kodi.tv ...)
   *        is matching
   * \note if this function is called on locally installed add-ons, for instance when populating
   *       'My add-ons', the local installation path is returned as origin.
   *       thus parameter CheckAddonPath::CHOICE_NO needs to be passed in such cases
   * \param addon pointer to addon to be checked
   * \param checkAddonPath also check origin path
   * \return true if the repository id of a given addon is defined as official
   *         and the addons origin matches the defined official origin of the repo id
   */
  static bool IsFromOfficialRepo(const std::shared_ptr<IAddon>& addon,
                                 CheckAddonPath checkAddonPath);

  /*!
   * \brief Checks if the passed in repository is defined as official repo
   *        which includes ORIGIN_SYSTEM
   * \param repoId repository id to check
   * \return true if the repository id is defined as official, false otherwise
   */
  static bool IsOfficialRepo(const std::string& repoId);

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
   *        Private versions are added obeying updateMode.
   *        (either OFFICIAL_ONLY or ANY_REPOSITORY)
   * \param[out] addonList retrieved addon list in a vector
   */
  void GetLatestAddonVersions(std::vector<std::shared_ptr<IAddon>>& addonList) const;

  /*!
   * \brief Retrieves the latest official versions of addons to vector.
   *        Private versions (latest per repository) are added obeying updateMode.
   *        (either OFFICIAL_ONLY or ANY_REPOSITORY)
   * \param[out] addonList retrieved addon list in a vector
   */
  void GetLatestAddonVersionsFromAllRepos(std::vector<std::shared_ptr<IAddon>>& addonList) const;

  /*!
   * \brief Find a dependency to install during an addon install or update
   *        If the dependency cannot be found in official versions we look in the
   *        installing/updating addon's (the parent's) origin repository
   * \param dependsId addon id of the dependency we're looking for
   * \param parentRepoId origin repository of the dependee
   * \param [out] dependencyToInstall pointer to the found dependency, only use
   *              if function returns true
   * \param [out] repoForDep the repository that dependency will install from finally
   * \return true if the dependency was found, false otherwise
   */
  bool FindDependency(const std::string& dependsId,
                      const std::string& parentRepoId,
                      std::shared_ptr<IAddon>& dependencyToInstall,
                      std::shared_ptr<CRepository>& repoForDep) const;

  /*!
   * \brief Find a dependency addon in the repository of its parent
   * \param dependsId addon id of the dependency we're looking for
   * \param parentRepoId origin repository of the dependee
   * \param [out] dependencyToInstall pointer to the found dependency, only use
   *              if function returns true
   * \return true if the dependency was found, false otherwise
   */
  bool FindDependencyByParentRepo(const std::string& dependsId,
                                  const std::string& parentRepoId,
                                  std::shared_ptr<IAddon>& dependencyToInstall) const;

  /*!
   * \brief Build compatible versions list based on the contents of m_allAddons
   * \note content of m_allAddons depends on the preceding call to @ref LoadAddonsFromDatabase()
   * \param[out] compatibleVersions target vector to be filled
   */
  void BuildCompatibleVersionsList(std::vector<std::shared_ptr<IAddon>>& compatibleVersions) const;

  /*!
   * \brief Return whether add-ons repo/version information was properly loaded after construction
   * \return true on success, false otherwise
   */
  bool IsValid() const { return m_valid; }

private:
  /*!
   * \brief Load and configure add-on maps
   * \return true on success, false otherwise
   */
  bool LoadAddonsFromDatabase(const std::string& addonId, const std::shared_ptr<IAddon>& repoAddon);

  /*!
   * \brief Looks up an addon in a given repository map and
   *        checks if an update is available
   * \param addonToCheck the addon we want to find and version check
   * \param map the repository map we want to check against
   * \param[out] pointer to the found update. if the addon is
   *              up-to-date on our system, this param will return 'nullptr'
   * \return true if the addon was found in the desired map and
   *         its version is newer than our local version.
   *         false if the addon does NOT exist in the map or it is up to date.
   */
  bool FindAddonAndCheckForUpdate(const std::shared_ptr<IAddon>& addonToCheck,
                                  const std::map<std::string, std::shared_ptr<IAddon>>& map,
                                  std::shared_ptr<IAddon>& update) const;

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
  CAddonDatabase m_addonDb;
  bool m_valid{false};

  std::vector<std::shared_ptr<IAddon>> m_allAddons;

  std::map<std::string, std::shared_ptr<IAddon>> m_latestOfficialVersions;
  std::map<std::string, std::shared_ptr<IAddon>> m_latestPrivateVersions;
  std::map<std::string, std::map<std::string, std::shared_ptr<IAddon>>> m_latestVersionsByRepo;
  std::map<std::string, std::multimap<std::string, std::shared_ptr<IAddon>>> m_addonsByRepoMap;
};

}; /* namespace ADDON */
