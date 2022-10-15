/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "XBDateTime.h"
#include "addons/AddonVersion.h"
#include "dbwrappers/Database.h"

#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

class CVariant;

namespace ADDON
{

enum class AddonDisabledReason;
enum class AddonUpdateRule;

class CAddonExtensions;
class CAddonInfoBuilderFromDB;

class CAddonInfo;
using AddonInfoPtr = std::shared_ptr<CAddonInfo>;

class IAddon;
using AddonPtr = std::shared_ptr<IAddon>;
using VECADDONS = std::vector<AddonPtr>;

/*!
 * @brief Addon content serializer/deserializer.
 *
 * Used to save data from the add-on in the database using json format.
 * The corresponding field in SQL is "addons" for "metadata".
 *
 * @warning Changes in the json format need a way to update the addon database
 * for users, otherwise problems may occur when reading the old content.
 */
class CAddonDatabaseSerializer
{
  CAddonDatabaseSerializer() = delete;

public:
  static std::string SerializeMetadata(const CAddonInfo& addon);
  static void DeserializeMetadata(const std::string& document, CAddonInfoBuilderFromDB& builder);

private:
  static CVariant SerializeExtensions(const CAddonExtensions& addonType);
  static void DeserializeExtensions(const CVariant& document, CAddonExtensions& addonType);
};

class CAddonDatabase : public CDatabase
{
public:
  CAddonDatabase();
  ~CAddonDatabase() override;
  bool Open() override;

  /*! \brief Get an addon with a specific version and repository. */
  bool GetAddon(const std::string& addonID,
                const ADDON::CAddonVersion& version,
                const std::string& repoId,
                ADDON::AddonPtr& addon);

  /*! Get the addon IDs that have been set to disabled */
  bool GetDisabled(std::map<std::string, ADDON::AddonDisabledReason>& addons);

  /*! Returns all addons in the repositories with id `addonId`. */
  bool FindByAddonId(const std::string& addonId, ADDON::VECADDONS& addons) const;

  bool UpdateRepositoryContent(const std::string& repositoryId,
                               const ADDON::CAddonVersion& version,
                               const std::string& checksum,
                               const std::vector<AddonInfoPtr>& addons);

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
    ADDON::CAddonVersion lastCheckedVersion{""};
    /*! \brief next time the repo should be checked, or invalid CDateTime if unknown */
    CDateTime nextCheckAt;

    RepoUpdateData() = default;

    RepoUpdateData(const CDateTime& lastCheckedAt,
                   const ADDON::CAddonVersion& lastCheckedVersion,
                   const CDateTime& nextCheckAt)
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

  /*!
   * \brief Write dataset with addon-id and rule to the db
   * \param addonID the addonID
   * \param updateRule the rule value to be written
   * \return true on success, false otherwise
   */
  bool AddUpdateRuleForAddon(const std::string& addonID, ADDON::AddonUpdateRule updateRule);

  /*!
   * \brief Remove all rule datasets for an addon-id from the db
   * \param addonID the addonID
   * \return true on success, false otherwise
   */
  bool RemoveAllUpdateRulesForAddon(const std::string& addonID);

  /*!
   * \brief Remove a single rule dataset for an addon-id from the db
   * \note specifying AddonUpdateRule::ANY will remove all rules.
   *       use @ref RemoveAllUpdateRulesForAddon() instead
   * \param addonID the addonID
   * \param updateRule the rule to remove
   * \return true on success, false otherwise
   */
  bool RemoveUpdateRuleForAddon(const std::string& addonID, AddonUpdateRule updateRule);

  /*!
   * \brief Retrieve all rule datasets from db and store them into map
   * \param rulesMap target map
   * \return true on success, false otherwise
   */
  bool GetAddonUpdateRules(std::map<std::string, std::vector<AddonUpdateRule>>& rulesMap) const;

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

  /*! \brief Add dataset for a new installed addon to the database
   *  \param addon the addon to insert
   *  \param origin the origin it was installed from
   *  \return true on success, false otherwise
   */
  bool AddInstalledAddon(const std::shared_ptr<CAddonInfo>& addon, const std::string& origin);

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
