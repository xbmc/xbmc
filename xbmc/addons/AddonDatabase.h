#pragma once
/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "dbwrappers/Database.h"
#include "addons/Addon.h"
#include "FileItem.h"
#include <string>

class CAddonDatabase : public CDatabase
{
public:
  CAddonDatabase();
  virtual ~CAddonDatabase();
  virtual bool Open();

  int GetAddonId(const ADDON::AddonPtr& item);
  int AddAddon(const ADDON::AddonPtr& item, int idRepo);
  bool GetAddon(const std::string& addonID, ADDON::AddonPtr& addon);
  bool GetAddons(ADDON::VECADDONS& addons, const ADDON::TYPE &type = ADDON::ADDON_UNKNOWN);

  /*! Get the addon IDs that has been set to disabled */
  bool GetDisabled(std::vector<std::string>& addons);

  /*! \brief grab the (largest) add-on version for an add-on */
  ADDON::AddonVersion GetAddonVersion(const std::string &id);

  /*! \brief Grab the repository from which a given addon came
   \param addonID - the id of the addon in question
   \param repo [out] - the id of the repository
   \return true if a repo was found, false otherwise.
   */
  bool GetRepoForAddon(const std::string& addonID, std::string& repo);
  int AddRepository(const std::string& id, const ADDON::VECADDONS& addons, const std::string& checksum, const ADDON::AddonVersion& version);
  void DeleteRepository(const std::string& id);
  void DeleteRepository(int id);
  int GetRepoChecksum(const std::string& id, std::string& checksum);
  bool GetRepository(const std::string& id, ADDON::VECADDONS& addons);
  bool GetRepository(int id, ADDON::VECADDONS& addons);
  bool SetRepoTimestamp(const std::string& id, const std::string& timestamp, const ADDON::AddonVersion& version);

  /*! \brief Retrieve the time a repository was last checked
   \param id id of the repo
   \return last time the repo was checked, current time if not available
   \sa SetRepoTimestamp */
  CDateTime GetRepoTimestamp(const std::string& id);

  ADDON::AddonVersion GetRepoVersion(const std::string& id);

  bool Search(const std::string& search, ADDON::VECADDONS& items);
  static void SetPropertiesFromAddon(const ADDON::AddonPtr& addon, CFileItemPtr& item); 

  /*! \brief Disable an addon.
   Sets a flag that this addon has been disabled.  If disabled, it is usually still available on disk.
   \param addonID id of the addon to disable
   \param disable whether to enable or disable.  Defaults to true (disable)
   \return true on success, false on failure
   \sa IsAddonDisabled, HasDisabledAddons */
  bool DisableAddon(const std::string &addonID, bool disable = true);

  /*! \brief Checks if an addon is in the database.
   \param addonID id of the addon to be checked
   \return true if addon is in database, false if addon is not in database yet */
  bool HasAddon(const std::string &addonID);
  
  /*! \brief Check whether an addon has been disabled via DisableAddon.
   \param addonID id of the addon to check
   \return true if the addon is disabled, false otherwise
   \sa DisableAddon, HasDisabledAddons */
  bool IsAddonDisabled(const std::string &addonID);

  /*! \brief Check whether we have disabled addons.
   \return true if we have disabled addons, false otherwise
   \sa DisableAddon, IsAddonDisabled */
  bool HasDisabledAddons();

  /*! @deprecated only here to allow clean upgrades from earlier pvr versions
   */
  bool IsSystemPVRAddonEnabled(const std::string &addonID);

  /*! \brief Mark an addon as broken
   Sets a flag that this addon has been marked as broken in the repository.
   \param addonID id of the addon to mark as broken
   \param reason why it is broken - if non empty we take it as broken.  Defaults to empty
   \return true on success, false on failure
   \sa IsAddonBroken */
  bool BreakAddon(const std::string &addonID, const std::string& reason="");

  /*! \brief Check whether an addon has been marked as broken via BreakAddon.
   \param addonID id of the addon to check
   \return reason if the addon is broken, blank otherwise
   \sa BreakAddon */
  std::string IsAddonBroken(const std::string &addonID);

  bool BlacklistAddon(const std::string& addonID, const std::string& version);
  bool IsAddonBlacklisted(const std::string& addonID, const std::string& version);
  bool RemoveAddonFromBlacklist(const std::string& addonID,
                                const std::string& version);

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

  /*! \brief allow adding a system addon like PVR or AUDIODECODER
      \param addonID id of the addon
  */
  bool AddSystemAddon(const std::string &addonID);

  /*! \brief check if system addon is registered
      \param addonID id of the addon
  */
  bool IsSystemAddonRegistered(const std::string &addonID);

protected:
  virtual void CreateTables();
  virtual void CreateAnalytics();
  virtual void UpdateTables(int version);
  virtual int GetMinSchemaVersion() const { return 15; }
  virtual int GetSchemaVersion() const { return 19; }
  const char *GetBaseDBName() const { return "Addons"; }

  bool GetAddon(int id, ADDON::AddonPtr& addon);

  /* keep in sync with the select in GetAddon */
  enum AddonFields
  {
    addon_id=0,
    addon_type,
    addon_name,
    addon_summary,
    addon_description,
    addon_stars,
    addon_path,
    addon_addonID,
    addon_icon,
    addon_version,
    addon_changelog,
    addon_fanart,
    addon_author,
    addon_disclaimer,
    addon_minversion,
    broken_reason,
    addonextra_key,
    addonextra_value,
    dependencies_addon,
    dependencies_version,
    dependencies_optional
  };
};

