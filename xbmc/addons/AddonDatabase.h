/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "AddonBuilder.h"
#include "FileItem.h"
#include "addons/Addon.h"
#include "dbwrappers/Database.h"

#include <string>
#include <vector>

namespace ADDON
{

class CAddonDatabase : public CDatabase
{
public:
  CAddonDatabase();
  ~CAddonDatabase() override;
  bool Open() override;

  /*! @deprecated: use CAddonMgr::FindInstallableById */
  bool GetAddon(const std::string& addonID, ADDON::AddonPtr& addon);

  /*! \brief Get an addon with a specific version and repository. */
  bool GetAddon(const std::string& addonID, const ADDON::AddonVersion& version, const std::string& repoId, ADDON::AddonPtr& addon);

  /*! Get the addon IDs that have been set to disabled */
  bool GetDisabled(std::map<std::string, ADDON::AddonDisabledReason>& addons);

  /*! @deprecated: use FindByAddonId */
  bool GetAvailableVersions(const std::string& addonId,
      std::vector<std::pair<ADDON::AddonVersion, std::string>>& versionsInfo);

  /*! @deprecated use CAddonMgr::FindInstallableById */
  std::pair<ADDON::AddonVersion, std::string> GetAddonVersion(const std::string &id);

  /*! Returns all addons in the repositories with id `addonId`. */
  bool FindByAddonId(const std::string& addonId, ADDON::VECADDONS& addons) const;

  bool UpdateRepositoryContent(const std::string& repositoryId, const ADDON::AddonVersion& version,
      const std::string& checksum, const std::vector<ADDON::AddonPtr>& addons);

  int GetRepoChecksum(const std::string& id, std::string& checksum);

  /*!
   \brief Get addons in repository `id`
   \param id id of the repository
   \returns true on success, false on error or if repository have never been synced.
   */
  bool GetRepositoryContent(const std::string& id, ADDON::VECADDONS& addons) const;

  /*! Get addons across all repositories */
  bool GetRepositoryContent(ADDON::VECADDONS& addons) const;

  struct RepoUpdateData
  {
    /*! \brief last time the repo was checked, or invalid CDateTime if never checked */
    CDateTime lastCheckedAt;
    /*! \brief last version of the repo add-on that was checked, or empty if never checked */
    ADDON::AddonVersion lastCheckedVersion{""};
    /*! \brief next time the repo should be checked, or invalid CDateTime if unknown */
    CDateTime nextCheckAt;

    RepoUpdateData() = default;

    RepoUpdateData(CDateTime lastCheckedAt,
                   ADDON::AddonVersion lastCheckedVersion,
                   CDateTime nextCheckAt)
      : lastCheckedAt{lastCheckedAt},
        lastCheckedVersion{lastCheckedVersion},
        nextCheckAt{nextCheckAt}
    {
    }
  };

  /*!
   \brief Set data concerning repository update (last/next date etc.), and create the repo if needed
   \param id add-on id of the repository
   \param updateData update data to set
   \returns id of the repository, or -1 on error.
   */
  int SetRepoUpdateData(const std::string& id, const RepoUpdateData& updateData);

  /*!
   \brief Retrieve repository update data (last/next date etc.)
   \param id add-on id of the repo
   \return update data of the repository
   */
  RepoUpdateData GetRepoUpdateData(const std::string& id);

  bool Search(const std::string& search, ADDON::VECADDONS& items);

  /*!
   * \brief Disable an addon.
   * Sets a flag that this addon has been disabled.  If disabled, it is usually still available on
   * disk.
   * \param addonID id of the addon to disable
   * \param disabledReason the reason why the addon is being disabled
   * \return true on success, false on failure
   * \sa IsAddonDisabled, HasDisabledAddons, EnableAddon
   */
  bool DisableAddon(const std::string& addonID, ADDON::AddonDisabledReason disabledReason);

  /*! \brief Enable an addon.
   * Enables an addon that has previously been disabled
   * \param addonID id of the addon to enable
   * \return true on success, false on failure
   * \sa DisableAddon, IsAddonDisabled, HasDisabledAddons
   */
  bool EnableAddon(const std::string& addonID);

  bool BlacklistAddon(const std::string& addonID);
  bool RemoveAddonFromBlacklist(const std::string& addonID);
  bool GetBlacklisted(std::set<std::string>& addons);

  /*! \brief Store an addon's package filename and that file's hash for future verification
      \param  addonID         id of the addon we're adding a package for
      \param  packageFileName filename of the package
      \param  hash            MD5 checksum of the package
      \return Whether or not the info successfully made it into the DB.
      \sa GetPackageHash, RemovePackage
  */
  bool AddPackage(const std::string& addonID,
                  const std::string& packageFileName,
                  const std::string& hash);
  /*! \brief Query the MD5 checksum of the given given addon's given package
      \param  addonID         id of the addon we're who's package we're querying
      \param  packageFileName filename of the package
      \param  hash            return the MD5 checksum of the package
      \return Whether or not we found a hash for the given addon's given package
      \sa AddPackage, RemovePackage
  */
  bool GetPackageHash(const std::string& addonID,
                      const std::string& packageFileName,
                      std::string&       hash);
  /*! \brief Remove a package's info from the database
      \param  packageFileName filename of the package
      \return Whether or not we succeeded in removing the package
      \sa AddPackage, GetPackageHash
  */
  bool RemovePackage(const std::string& packageFileName);

  /*! Clear internal fields that shouldn't be kept around indefinitely */
  void OnPostUnInstall(const std::string& addonId);

  void SyncInstalled(const std::set<std::string>& ids,
                     const std::set<std::string>& system,
                     const std::set<std::string>& optional);

  bool SetLastUpdated(const std::string& addonId, const CDateTime& dateTime);
  bool SetOrigin(const std::string& addonId, const std::string& origin);
  bool SetLastUsed(const std::string& addonId, const CDateTime& dateTime);

  void GetInstallData(const ADDON::AddonInfoPtr& addon);

protected:
  void CreateTables() override;
  void CreateAnalytics() override;
  void UpdateTables(int version) override;
  int GetMinSchemaVersion() const override;
  int GetSchemaVersion() const override;
  const char *GetBaseDBName() const override { return "Addons"; }

  bool GetAddon(int id, ADDON::AddonPtr& addon);
  void DeleteRepository(const std::string& id);
  void DeleteRepositoryContents(const std::string& id);
  int GetRepositoryId(const std::string& addonId);
};

}; // namespace ADDON
