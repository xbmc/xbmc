/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "Addon.h"
#include "AddonDatabase.h"
#include "Repository.h"
#include "threads/CriticalSection.h"
#include "utils/EventStream.h"

#include <map>
#include <mutex>

namespace ADDON
{
  typedef std::map<TYPE, VECADDONS> MAPADDONS;
  typedef std::map<TYPE, VECADDONS>::iterator IMAPADDONS;
  typedef std::map<std::string, AddonInfoPtr> ADDON_INFO_LIST;

  const std::string ADDON_PYTHON_EXT           = "*.py";

  /**
  * Class - IAddonMgrCallback
  * This callback should be inherited by any class which manages
  * specific addon types. Could be mostly used for Dll addon types to handle
  * cleanup before restart/removal
  */
  class IAddonMgrCallback
  {
    public:
      virtual ~IAddonMgrCallback() = default;
      virtual bool RequestRestart(const std::string& id, bool datachanged)=0;
  };

  /**
  * Class - CAddonMgr
  * Holds references to all addons, enabled or
  * otherwise. Services the generic callbacks available
  * to all addon variants.
  */
  class CAddonMgr
  {
  public:
    bool ReInit() { DeInit(); return Init(); }
    bool Init();
    void DeInit();

    CAddonMgr() = default;
    CAddonMgr(const CAddonMgr&) = delete;
    virtual ~CAddonMgr();

    CEventStream<AddonEvent>& Events() { return m_events; }
    CEventStream<AddonEvent>& UnloadEvents() { return m_unloadEvents; }

    IAddonMgrCallback* GetCallbackForType(TYPE type);
    bool RegisterAddonMgrCallback(TYPE type, IAddonMgrCallback* cb);
    void UnregisterAddonMgrCallback(TYPE type);

    /*! \brief Retrieve a specific addon (of a specific type)
     \param id the id of the addon to retrieve.
     \param addon[out] the retrieved addon pointer - only use if the function returns true.
     \param type type of addon to retrieve - defaults to any type.
     \param enabledOnly whether we only want enabled addons - set to false to allow both enabled and disabled addons - defaults to true.
     \return true if an addon matching the id of the given type is available and is enabled (if enabledOnly is true).
     */
    bool GetAddon(const std::string& id,
                  AddonPtr& addon,
                  const TYPE& type = ADDON_UNKNOWN,
                  bool enabledOnly = true) const;

    bool HasType(const std::string &id, const TYPE &type);

    bool HasAddons(const TYPE &type);

    bool HasInstalledAddons(const TYPE &type);

    /*! Returns all installed, enabled and incompatible (and disabled) add-ons. */
    bool GetAddonsForUpdate(VECADDONS& addons) const;

    /*! Returns all installed, enabled add-ons. */
    bool GetAddons(VECADDONS& addons) const;

    /*! Returns enabled add-ons with given type. */
    bool GetAddons(VECADDONS& addons, const TYPE& type);

    /*! Returns all installed, including disabled. */
    bool GetInstalledAddons(VECADDONS& addons);

    /*! Returns installed add-ons, including disabled, with given type. */
    bool GetInstalledAddons(VECADDONS& addons, const TYPE& type);

    bool GetDisabledAddons(VECADDONS& addons);

    bool GetDisabledAddons(VECADDONS& addons, const TYPE& type);

    /*! Get all installable addons */
    bool GetInstallableAddons(VECADDONS& addons);

    bool GetInstallableAddons(VECADDONS& addons, const TYPE &type);

    /*! \brief Get the installable addon depending on install rules
     *         or fall back to highest version.
     * \note This function gets called in different contexts. If it's
     *       called for checking possible updates for already installed addons
     *       our update restriction rules apply.
     *       If it's called to (for example) populate an addon-select-dialog
     *       the addon is not installed yet, and we have to fall back to the
     *       highest version.
     * \param addonId addon to check for update or installation
     * \param addon[out] the retrieved addon pointer - only use if the function returns true.
     * \return true if an addon matching the id is available.
     */
    bool FindInstallableById(const std::string& addonId, AddonPtr& addon);

    void AddToUpdateableAddons(AddonPtr &pAddon);
    void RemoveFromUpdateableAddons(AddonPtr &pAddon);
    bool ReloadSettings(const std::string &id);

    /*! Get addons with available updates */
    VECADDONS GetAvailableUpdates() const;

    /*! Returns true if there is any addon with available updates, otherwise false */
    bool HasAvailableUpdates();

    /*! \brief Checks for new / updated add-ons
     \return True if everything went ok, false otherwise
     */
    bool FindAddons();

    /*!
     * @brief Fills the the provided vector with the list of incompatible
     * enabled addons and returns if there's any.
     *
     * @param[out] incompatible List of incompatible addons
     * @return true if there are incompatible addons
     */
    bool GetIncompatibleEnabledAddonInfos(std::vector<AddonInfoPtr>& incompatible) const;

    /*!
     * Migrate all the addons (updates all addons that have an update pending and disables those
     * that got incompatible)
     *
     * @return list of all addons (infos) that were modified.
     */
    std::vector<AddonInfoPtr> MigrateAddons();

    /*!
     * @brief Try to disable addons in the given list.
     *
     * @param[in] incompatible List of incompatible addon infos
     * @return list of all addon Infos that were disabled
     */
    std::vector<AddonInfoPtr> DisableIncompatibleAddons(
        const std::vector<AddonInfoPtr>& incompatible);

    /*!
     * Install available addon updates, if any.
     * @param wait If kodi should wait for all updates to download and install before returning
     */
    void CheckAndInstallAddonUpdates(bool wait) const;

    /*!
     * @note: should only be called by AddonInstaller
     *
     * Unload addon from the system. Returns true if it was unloaded, otherwise false.
     */
    bool UnloadAddon(const std::string& addonId);

    /*!
     * @note: should only be called by AddonInstaller
     *
     * Returns true if the addon was successfully loaded and enabled; otherwise false.
     */
    bool LoadAddon(const std::string& addonId);

    /*! @note: should only be called by AddonInstaller
     *
     * Hook for clearing internal state after uninstall.
     */
    void OnPostUnInstall(const std::string& id);

    /*! \brief Disable an addon. Returns true on success, false on failure. */
    bool DisableAddon(const std::string& ID, AddonDisabledReason disabledReason);

    /*! \brief Enable an addon. Returns true on success, false on failure. */
    bool EnableAddon(const std::string& ID);

    /* \brief Check whether an addon has been disabled via DisableAddon.
     In case the disabled cache does not know about the current state the database routine will be used.
     \param ID id of the addon
     \sa DisableAddon
     */
    bool IsAddonDisabled(const std::string& ID) const;

    /*!
     * @brief Check whether an addon has been disabled via DisableAddon except for a particular
     * reason In case the disabled cache does not know about the current state the database routine
     * will be used.
     * @param[in] ID id of the addon
     * @param[in] disabledReason the reason that will be an exception to being disabled
     * @return true if the addon was disabled except for the specified reason
     * @sa DisableAddon
     */
    bool IsAddonDisabledExcept(const std::string& ID, AddonDisabledReason disabledReason) const;

    /* \brief Checks whether an addon can be disabled via DisableAddon.
     \param ID id of the addon
     \sa DisableAddon
     */
    bool CanAddonBeDisabled(const std::string& ID);

    bool CanAddonBeEnabled(const std::string& id);

    /* \brief Checks whether an addon is installed.
     \param ID id of the addon
    */
    bool IsAddonInstalled(const std::string& ID);

    /* \brief Checks whether an addon can be installed. Broken addons can't be installed.
    \param addon addon to be checked
    */
    bool CanAddonBeInstalled(const AddonPtr& addon);

    bool CanUninstall(const AddonPtr& addon);

    bool IsSystemAddon(const std::string& id);

    bool AddToUpdateBlacklist(const std::string& id);
    bool RemoveFromUpdateBlacklist(const std::string& id);
    bool IsBlacklisted(const std::string& id) const;

    void UpdateLastUsed(const std::string& id);

    /*! \brief Load the addon in the given path
     This loads the addon using c-pluff which parses the addon descriptor file.
     \param path folder that contains the addon.
     \param addon [out] returned addon.
     \return true if addon is set, false otherwise.
     */
    bool LoadAddonDescription(const std::string &path, AddonPtr &addon);

    /*! \brief Parse a repository XML file for addons and load their descriptors
     A repository XML is essentially a concatenated list of addon descriptors.
     \param repo The repository info.
     \param xml The XML document from repository.
     \param addons [out] returned list of addons.
     \return true if the repository XML file is parsed, false otherwise.
     */
    bool AddonsFromRepoXML(const CRepository::DirInfo& repo, const std::string& xml, VECADDONS& addons);

    bool ServicesHasStarted() const;

    /*!
     * @deprecated This addon function should no more used and becomes replaced
     * in future with the other below by his callers.
     */
    bool IsCompatible(const IAddon& addon) const;

    /*!
     * @brief Check given addon information is compatible with Kodi.
     *
     * @param[in] addonInfo Addon information to check
     * @return true if compatible, false if not
     */
    bool IsCompatible(const AddonInfoPtr& addonInfo) const;

    /*! \brief Recursively get dependencies for an add-on
     */
    std::vector<DependencyInfo> GetDepsRecursive(const std::string& id);

    /*!
     * @brief Get a list of add-on's with info's for the on system available
     * ones.
     *
     * @param[out] addonInfos list where finded addon information becomes stored
     * @param[in] enabledOnly If true are only enabled ones given back,
     *                        if false all on system available. Default is true.
     * @param[in] type The requested type, with "ADDON_UNKNOWN" are all add-on
     *                 types given back who match the case with value before.
     *                 If a type id becomes added are only add-ons returned who
     *                 match them. Default is for all types.
     * @return true if the list contains entries
     */
    bool GetAddonInfos(AddonInfos& addonInfos, bool enabledOnly, TYPE type) const;

    /*!
     * @brief Get a list of disabled add-on's with info's
     *
     * @param[out] addonInfos list where finded addon information becomes stored
     * @param[in] type        The requested type, with "ADDON_UNKNOWN"
     *                        are all add-on types given back who match the case
     *                        with value before.
     *                        If a type id becomes added are only add-ons
     *                        returned who match them. Default is for all types.
     * @return true if the list contains entries
     */
    bool GetDisabledAddonInfos(std::vector<AddonInfoPtr>& addonInfos, TYPE type) const;

    /*!
     * @brief Get a list of disabled add-on's with info's for the on system
     * available ones with a specific disabled reason.
     *
     * @param[out] addonInfos list where finded addon information becomes stored
     * @param[in] type        The requested type, with "ADDON_UNKNOWN"
     *                        are all add-on types given back who match the case
     *                        with value before.
     *                        If a type id becomes added are only add-ons
     *                        returned who match them. Default is for all types.
     * @param[in] disabledReason To get all disabled addons use the value
     *                           "AddonDiasbledReason::NONE". If any other value
     *                           is supplied only addons with that reason will be
     *                           returned.
     * @return true if the list contains entries
     */
    bool GetDisabledAddonInfos(std::vector<AddonInfoPtr>& addonInfos,
                               TYPE type,
                               AddonDisabledReason disabledReason) const;

    const AddonInfoPtr GetAddonInfo(const std::string& id, TYPE type = ADDON_UNKNOWN) const;

    /*!
     * @brief Get the path where temporary add-on files are stored
     *
     * @return the base path used for temporary addon paths
     *
     * @warning the folder and its contents are deleted when Kodi is closed
     */
    const std::string& GetTempAddonBasePath() { return m_tempAddonBasePath; }

    AddonOriginType GetAddonOriginType(const AddonPtr& addon) const;

    /*!
     * \brief Check whether an addon has been disabled with a special reason.
     * \param ID id of the addon
     * \param disabledReason reason we want to check for (NONE, USER, INCOMPATIBLE, PERMANENT_FAILURE)
     * \return true or false 
     */
    bool IsAddonDisabledWithReason(const std::string& ID, AddonDisabledReason disabledReason) const;

    /*!
     * @brief Addon update and install management.
     *
     * Parts inside here are used for changes about addon system.
     *
     * @warning This should be never used from other places outside of addon
     * system directory.
     *
     * Currently listed call sources:
     * - @ref CAddonInstallJob::DoWork
     */
    /*@{{{*/

    /*!
     * @brief Update addon origin data.
     *
     * This becomes called from @ref CAddonInstallJob to set the source repo and
     * if update, to set also the date.
     *
     * @note This must be called after the addon manager has inserted a new addon
     * with @ref FindAddons() into database.
     *
     * @param[in] addonId Identifier of addon
     * @param[in] repoAddonId Identifier of related repository addon
     * @param[in] isUpdate If call becomes done on already installed addon and
     *                     update only.
     * @return True if successfully done, otherwise false
     */
    bool SetAddonOrigin(const std::string& addonId, const std::string& repoAddonId, bool isUpdate);

    /*@}}}*/

  private:
    CAddonMgr& operator=(CAddonMgr const&) = delete;

    VECADDONS m_updateableAddons;

    bool GetAddonsInternal(const TYPE& type,
                           VECADDONS& addons,
                           bool enabledOnly,
                           bool checkIncompatible = false) const;
    bool EnableSingle(const std::string& id);

    void FindAddons(ADDON_INFO_LIST& addonmap, const std::string& path);

    /*!
     * @brief Fills the the provided vector with the list of incompatible
     * addons and returns if there's any.
     *
     * @param[out] incompatible List of incompatible addons
     * @param[in] whether or not to include incompatible addons that are disabled
     * @return true if there are incompatible addons
     */
    bool GetIncompatibleAddonInfos(std::vector<AddonInfoPtr>& incompatible,
                                   bool includeDisabled) const;

    /*!
     * Get the list of of available updates
     * \param[in,out] updates the vector of addons to be filled with addons that need to be updated (not blacklisted)
     * \return if there are any addons needing updates
     */
    bool GetAddonUpdateCandidates(VECADDONS& updates) const;

    /*!\brief Sort a list of addons for installation, i.e., defines the order of installation depending
     * of each addon dependencies.
     * \param[in,out] updates the vector of addons to sort
     */
    void SortByDependencies(VECADDONS& updates) const;

    /*!
     * Install the list of addon updates via AddonInstaller
     * \param[in,out] updates the vector of addons to install (will be sorted)
     * \param wait if the process should wait for all addons to install
     */
    void InstallAddonUpdates(VECADDONS& updates, bool wait) const;

    // This guards the addon installation process to make sure
    // addon updates are not installed concurrently
    // while the migration is running. Addon updates can be triggered
    // as a result of a repository update event.
    // (migration will install any available update anyway)
    mutable std::mutex m_installAddonsMutex;

    std::map<std::string, AddonDisabledReason> m_disabled;
    std::set<std::string> m_updateBlacklist;
    static std::map<TYPE, IAddonMgrCallback*> m_managers;
    mutable CCriticalSection m_critSection;
    CAddonDatabase m_database;
    CEventSource<AddonEvent> m_events;
    CBlockingEventSource<AddonEvent> m_unloadEvents;
    std::set<std::string> m_systemAddons;
    std::set<std::string> m_optionalSystemAddons;
    ADDON_INFO_LIST m_installedAddons;

    // Temporary path given to add-ons, whose content is deleted when Kodi is stopped
    const std::string m_tempAddonBasePath = "special://temp/addons";
  };

}; /* namespace ADDON */
