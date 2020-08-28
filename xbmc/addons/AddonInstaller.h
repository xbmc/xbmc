/*
 *  Copyright (C) 2011-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "addons/Addon.h"
#include "addons/Repository.h"
#include "threads/Event.h"
#include "utils/FileOperationJob.h"
#include "utils/Stopwatch.h"

#include <string>
#include <utility>
#include <vector>

namespace ADDON
{

class CAddonDatabase;

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
  bool InstallModal(const std::string &addonID, ADDON::AddonPtr &addon, bool promptForInstall = true);

  /*! \brief Install an addon if it is available in a repository
   \param addonID the addon ID of the item to install
   \param background whether to install in the background or not. Defaults to true.
   \param modal whether to show a modal dialog when not installing in background
   \return true on successful install, false on failure.
   \sa DoInstall
   */
  bool InstallOrUpdate(const std::string &addonID, bool background = true, bool modal = false);

  /*! \brief Install an addon from a specific repository
   \param addon the addon to install
   \param repo the repository to install the addon from
   \return true on successful install, false on failure.
   \sa DoInstall
   */
  bool InstallOrUpdate(const ADDON::AddonPtr& addon, const ADDON::RepositoryPtr& repo);

  /*! \brief Installs a vector of addons
   \param addons the list of addons to install
   \param wait if the method should wait for all the DoInstall jobs to finish or if it should return right away
   \sa DoInstall
   */
  void InstallAddons(const ADDON::VECADDONS& addons, bool wait);

  /*! \brief Install an addon from the given zip path
   \param path the zip file to install from
   \return true if successful, false otherwise
   \sa DoInstall
   */
  bool InstallFromZip(const std::string &path);

   /*! Install an addon with a specific version and repository */
  void Install(const std::string& addonId, const ADDON::AddonVersion& version, const std::string& repoId);

  /*! \brief Check whether dependencies of an addon exist or are installable.
  Iterates through the addon's dependencies, checking they're installed or installable.
  Each dependency must also satisfies CheckDependencies in turn.
  \param addon the addon to check
  \param database the database instance to update. Defaults to NULL.
  \return true if dependencies are available, false otherwise.
  */
  bool CheckDependencies(const ADDON::AddonPtr &addon, CAddonDatabase *database = NULL);

  /*! \brief Check whether dependencies of an addon exist or are installable.
   Iterates through the addon's dependencies, checking they're installed or installable.
   Each dependency must also satisfies CheckDependencies in turn.
   \param addon the addon to check
   \param failedDep Dependency addon that isn't available
   \param database the database instance to update. Defaults to NULL.
   \return true if dependencies are available, false otherwise.
   */
  bool CheckDependencies(const ADDON::AddonPtr &addon, std::pair<std::string, std::string> &failedDep, CAddonDatabase *database = NULL);

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
   \param addon the AddonPtr describing the addon
   \param repo the repository to install addon from
   \param background whether to install in the background or not. Defaults to true.
   \return true on successful install, false on failure.
   */
  bool DoInstall(const ADDON::AddonPtr &addon, const ADDON::RepositoryPtr &repo,
      bool background = true, bool modal = false, bool autoUpdate = false);

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
  int64_t EnumeratePackageFolder(std::map<std::string,CFileItemList*>& result);

  mutable CCriticalSection m_critSection;
  JobMap m_downloadJobs;
  CEvent m_idle;
};

class CAddonInstallJob : public CFileOperationJob
{
public:
  CAddonInstallJob(const ADDON::AddonPtr& addon, const ADDON::RepositoryPtr& repo, bool isAutoUpdate);

  bool DoWork() override;

  static constexpr const char* TYPE_DOWNLOAD = "DOWNLOAD";
  static constexpr const char* TYPE_INSTALL = "INSTALL";
  /*!
   * \brief Returns the current processing type in the installation job
   *
   * \return The current processing type as string, can be \ref TYPE_DOWNLOAD or
   *         \ref TYPE_INSTALL
   */
  const char* GetType() const override { return m_currentType; }

  /*! \brief Find the add-on and its repository for the given add-on ID
   *  \param addonID ID of the add-on to find
   *  \param[out] repo the repository to use
   *  \param[out] addon Add-on with the given add-on ID
   *  \return True if the add-on and its repository were found, false otherwise.
   */
  static bool GetAddon(const std::string& addonID, ADDON::RepositoryPtr& repo, ADDON::AddonPtr& addon);

private:
  void OnPreInstall();
  void OnPostInstall();
  bool Install(const std::string &installFrom, const ADDON::RepositoryPtr& repo = ADDON::RepositoryPtr());
  bool DownloadPackage(const std::string &path, const std::string &dest);

  bool DoFileOperation(FileAction action, CFileItemList &items, const std::string &file, bool useSameJob = true);

  /*! \brief Queue a notification for addon installation/update failure
   \param addonID - addon id
   \param fileName - filename which is shown in case the addon id is unknown
   \param message - error message to be displayed
   */
  void ReportInstallError(const std::string& addonID, const std::string& fileName, const std::string& message = "");

  ADDON::AddonPtr m_addon;
  ADDON::RepositoryPtr m_repo;
  bool m_isUpdate;
  bool m_isAutoUpdate;
  const char* m_currentType = TYPE_DOWNLOAD;
};

class CAddonUnInstallJob : public CFileOperationJob
{
public:
  CAddonUnInstallJob(const ADDON::AddonPtr &addon, bool removeData);

  bool DoWork() override;

private:
  void ClearFavourites();

  ADDON::AddonPtr m_addon;
  bool m_removeData;
};

}; // namespace ADDON
