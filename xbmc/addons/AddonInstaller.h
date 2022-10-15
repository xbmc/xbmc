/*
 *  Copyright (C) 2011-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "threads/Event.h"
#include "utils/Job.h"

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

class CFileItemList;

namespace ADDON
{

class CAddonVersion;

class CAddonDatabase;

class CRepository;
using RepositoryPtr = std::shared_ptr<CRepository>;

class IAddon;
using AddonPtr = std::shared_ptr<IAddon>;
using VECADDONS = std::vector<AddonPtr>;

enum class BackgroundJob : bool
{
  CHOICE_YES = true,
  CHOICE_NO = false,
};

enum class ModalJob : bool
{
  CHOICE_YES = true,
  CHOICE_NO = false,
};

enum class AutoUpdateJob : bool
{
  CHOICE_YES = true,
  CHOICE_NO = false,
};

enum class DependencyJob : bool
{
  CHOICE_YES = true,
  CHOICE_NO = false,
};

enum class InstallModalPrompt : bool
{
  CHOICE_YES = true,
  CHOICE_NO = false,
};

enum class AllowCheckForUpdates : bool
{
  CHOICE_YES = true,
  CHOICE_NO = false,
};

enum class RecurseOrphaned : bool
{
  CHOICE_YES = true,
  CHOICE_NO = false,
};

class CAddonInstaller : public IJobCallback
{
public:
  static CAddonInstaller &GetInstance();

  bool IsDownloading() const;
  void GetInstallList(ADDON::VECADDONS &addons) const;
  bool GetProgress(const std::string& addonID, unsigned int& percent, bool& downloadFinshed) const;
  bool Cancel(const std::string &addonID);

  /*! \brief Installs the addon while showing a modal progress dialog
   \param addonID the addon ID of the item to install.
   \param addon [out] the installed addon for later use.
   \param promptForInstall Whether or not to prompt the user before installing the addon.
   \return true on successful install, false otherwise.
   \sa Install
   */
  bool InstallModal(const std::string& addonID,
                    ADDON::AddonPtr& addon,
                    InstallModalPrompt promptForInstall);

  /*! \brief Install an addon if it is available in a repository
   \param addonID the addon ID of the item to install
   \param background whether to install in the background or not.
   \param modal whether to show a modal dialog when not installing in background
   \return true on successful install, false on failure.
   \sa DoInstall
   */
  bool InstallOrUpdate(const std::string& addonID, BackgroundJob background, ModalJob modal);

  /*! \brief Install a dependency from a specific repository
   \param dependsId the dependency to install
   \param repo the repository to install the addon from
   \return true on successful install, false on failure.
   \sa DoInstall
   */
  bool InstallOrUpdateDependency(const ADDON::AddonPtr& dependsId,
                                 const ADDON::RepositoryPtr& repo);

  /*! \brief Remove a single dependency from the system
   \param dependsId the dependency to remove
   \return true on successful uninstall, false on failure.
   */
  bool RemoveDependency(const std::shared_ptr<IAddon>& dependsId) const;

  /*!
   * \brief Removes all orphaned add-ons recursively. Removal may orphan further
   *        add-ons/dependencies, so loop until no orphaned is left on the system
   * \return Names of add-ons that have effectively been removed
   */
  std::vector<std::string> RemoveOrphanedDepsRecursively() const;

  /*! \brief Installs a vector of addons
   *  \param addons the list of addons to install
   *  \param wait if the method should wait for all the DoInstall jobs to finish or if it should return right away
   *  \param allowCheckForUpdates indicates if content update checks are allowed
   *         after installation of a repository addon from the vector
   *  \sa DoInstall
   */
  void InstallAddons(const ADDON::VECADDONS& addons,
                     bool wait,
                     AllowCheckForUpdates allowCheckForUpdates);

  /*! \brief Install an addon from the given zip path
   \param path the zip file to install from
   \return true if successful, false otherwise
   \sa DoInstall
   */
  bool InstallFromZip(const std::string &path);

   /*! Install an addon with a specific version and repository */
  bool Install(const std::string& addonId,
               const ADDON::CAddonVersion& version,
               const std::string& repoId);

  /*! Uninstall an addon, remove addon data if requested */
  bool UnInstall(const ADDON::AddonPtr& addon, bool removeData);

  /*! \brief Check whether dependencies of an addon exist or are installable.
  Iterates through the addon's dependencies, checking they're installed or installable.
  Each dependency must also satisfies CheckDependencies in turn.
  \param addon the addon to check
  \param database the database instance to update. Defaults to NULL.
  \return true if dependencies are available, false otherwise.
  */
  bool CheckDependencies(const ADDON::AddonPtr& addon, CAddonDatabase* database = nullptr);

  /*! \brief Check whether dependencies of an addon exist or are installable.
   Iterates through the addon's dependencies, checking they're installed or installable.
   Each dependency must also satisfies CheckDependencies in turn.
   \param addon the addon to check
   \param failedDep Dependency addon that isn't available
   \param database the database instance to update. Defaults to NULL.
   \return true if dependencies are available, false otherwise.
   */
  bool CheckDependencies(const ADDON::AddonPtr& addon,
                         std::pair<std::string, std::string>& failedDep,
                         CAddonDatabase* database = nullptr);

  /*! \brief Check if an installation job for a given add-on is already queued up
   *  \param ID The ID of the add-on
   *  \return true if a job exists, false otherwise
   */
  bool HasJob(const std::string& ID) const;

  void OnJobComplete(unsigned int jobID, bool success, CJob* job) override;
  void OnJobProgress(unsigned int jobID, unsigned int progress, unsigned int total, const CJob *job) override;

  class CDownloadJob
  {
  public:
    explicit CDownloadJob(unsigned int id) : jobID(id) { }

    unsigned int jobID;
    unsigned int progress = 0;
    bool downloadFinshed = false;
  };

  typedef std::map<std::string, CDownloadJob> JobMap;

private:
  // private construction, and no assignments; use the provided singleton methods
  CAddonInstaller();
  CAddonInstaller(const CAddonInstaller&) = delete;
  CAddonInstaller const& operator=(CAddonInstaller const&) = delete;
  ~CAddonInstaller() override;

  /*! \brief Install an addon from a repository or zip
   *  \param addon the AddonPtr describing the addon
   *  \param repo the repository to install addon from
   *  \param background whether to install in the background or not.
   *  \param modal whether to install in modal mode or not.
   *  \param autoUpdate whether the addon is installed in auto update mode.
   *         (i.e. no notification)
   *  \param dependsInstall whether this is the installation of a dependency addon
   *  \param allowCheckForUpdates whether content update check after installation of
   *         a repository addon is allowed
   *  \return true on successful install, false on failure.
   */
  bool DoInstall(const ADDON::AddonPtr& addon,
                 const ADDON::RepositoryPtr& repo,
                 BackgroundJob background,
                 ModalJob modal,
                 AutoUpdateJob autoUpdate,
                 DependencyJob dependsInstall,
                 AllowCheckForUpdates allowCheckForUpdates);

  /*! \brief Check whether dependencies of an addon exist or are installable.
   Iterates through the addon's dependencies, checking they're installed or installable.
   Each dependency must also satisfies CheckDependencies in turn.
   \param addon the addon to check
   \param preDeps previous dependencies encountered during recursion. aids in avoiding infinite recursion
   \param database database instance to update
   \param failedDep Dependency addon that isn't available
   \return true if dependencies are available, false otherwise.
   */
  bool CheckDependencies(const ADDON::AddonPtr &addon, std::vector<std::string>& preDeps, CAddonDatabase &database, std::pair<std::string, std::string> &failedDep);

  void PrunePackageCache();
  int64_t EnumeratePackageFolder(std::map<std::string, std::unique_ptr<CFileItemList>>& result);

  mutable CCriticalSection m_critSection;
  JobMap m_downloadJobs;
  CEvent m_idle;
};

}; // namespace ADDON
