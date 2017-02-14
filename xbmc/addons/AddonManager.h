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
#include "Addon.h"
#include "AddonDatabase.h"
#include "AddonEvents.h"
#include "threads/CriticalSection.h"
#include "utils/EventStream.h"
#include <string>
#include <vector>
#include <map>
#include <deque>

namespace ADDON
{
  class CAddonDll;
  typedef std::shared_ptr<CAddonDll> AddonDllPtr;

  typedef std::map<std::string, AddonInfoPtr> AddonInfoList;

  const std::string ADDON_PYTHON_EXT = "*.py";

  /**
  * Class - IAddonMgrCallback
  * This callback should be inherited by any class which manages
  * specific addon types. Could be mostly used for Dll addon types to handle
  * cleanup before restart/removal
  */
  class IAddonMgrCallback
  {
    public:
      virtual ~IAddonMgrCallback() {};
      virtual bool RequestRestart(AddonPtr addon, bool datachanged)=0;
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
    static CAddonMgr &GetInstance();
    bool ReInit() { DeInit(); return Init(); }
    bool Init();
    void DeInit();

    CAddonMgr();
    CAddonMgr(const CAddonMgr&);
    CAddonMgr const& operator=(CAddonMgr const&);
    virtual ~CAddonMgr();

    /*! \brief Retrieve a specific addon (of a specific type)
     \param id the id of the addon to retrieve.
     \param addon [out] the retrieved addon pointer - only use if the function returns true.
     \param type type of addon to retrieve - defaults to any type.
     \param enabledOnly whether we only want enabled addons - set to false to allow both enabled and disabled addons - defaults to true.
     \return true if an addon matching the id of the given type is available and is enabled (if enabledOnly is true).
     */
    bool GetAddon(const std::string &addonId, AddonPtr &addon, const TYPE &type = ADDON_UNKNOWN);

    /// @todo New parts to handle multi instance addon, still in todo
    //@{
    AddonDllPtr GetAddon(const std::string& addonId, const IAddonInstanceHandler* handler);
    AddonDllPtr GetAddon(const AddonInfoPtr& addonInfo, const IAddonInstanceHandler* handler);
    void ReleaseAddon(AddonDllPtr& addon, const IAddonInstanceHandler* handler);
    //@}

    AddonDllPtr GetAddon(const TYPE &type, const std::string &id);    

    /*!
     * @brief Checks system about given type to know related add-on's are
     * installed.
     *
     * @param[in] type Add-on type to check installed
     * @return true if given type is installed
     */
    bool HasInstalledAddons(const TYPE &type);

    /*!
     * @brief Checks system about given type to know related add-on's are
     * installed and also minimum one enabled.
     *
     * @param[in] type Add-on type to check enabled
     * @return true if given type is enabled
     */
    bool HasEnabledAddons(const TYPE &type);

    /*!
     * @brief Checks whether an addon is installed.
     *
     * @param[in] addonId id of the addon
     * @param[in] type Add-on type to check installed
     * @return true if installed
     */
    bool IsAddonInstalled(const std::string& addonId, const TYPE &type = ADDON_UNKNOWN);

    /*!
     * @brief Check whether an addon has been enabled.
     *
     * @param[in] addonId id of the addon
     * @param[in] type Add-on type to check installed and enabled
     * @return true if enabled
     */
    bool IsAddonEnabled(const std::string& addonId, const TYPE &type = ADDON_UNKNOWN);

    /*!
     * @brief Get a list of add-on's with info's for the on system available
     * ones.
     *
     * @param[in] enabledOnly If true are only enabled ones given back,
     *                        if false all on system available. Default is true.
     * @param[in] type        The requested type, with "ADDON_UNKNOWN"
     *                        are all add-on types given back who match the case
     *                        with value before.
     *                        If a type id becomes added are only add-ons
     *                        returned who match them. Default is for all types.
     * @param[in] useTimeData [opt] if set to true also the dates from updates,
     *                        installation and usage becomes set on info.
     * @return The list with of available add-on's with info tables.
     */
    AddonInfos GetAddonInfos(bool enabledOnly, const TYPE &type, bool useTimeData = false);

    /*!
     * @brief Get a list of add-on's with info's for the on system available
     * ones.
     *
     * @param[out] addonInfos list where finded addon information becomes stored
     * @param[in] enabledOnly If true are only enabled ones given back,
     *                        if false all on system available. Default is true.
     * @param[in] type        The requested type, with "ADDON_UNKNOWN"
     *                        are all add-on types given back who match the case
     *                        with value before.
     *                        If a type id becomes added are only add-ons
     *                        returned who match them. Default is for all types.
     * @param[in] useTimeData [opt] if set to true also the dates from updates,
     *                        installation and usage becomes set on info.
     */
    void GetAddonInfos(AddonInfos& addonInfos, bool enabledOnly, const TYPE &type, bool useTimeData = false);

    /*!
     * @brief Get a list of disabled add-on's with info's for the on system
     * available ones.
     *
     * @param[out] addonInfos list where finded addon information becomes stored
     * @param[in] type        The requested type, with "ADDON_UNKNOWN"
     *                        are all add-on types given back who match the case
     *                        with value before.
     *                        If a type id becomes added are only add-ons
     *                        returned who match them. Default is for all types.
     */
    void GetDisabledAddonInfos(AddonInfos& addonInfos, const TYPE& type);

    /*!
     * @brief Get a list of from repositories installable add-on's with info's
     * for the on system available ones.
     *
     * @param[out] addonInfos list where finded addon information becomes stored
     * @param[in] type        The requested type, with "ADDON_UNKNOWN"
     *                        are all add-on types given back who match the case
     *                        with value before.
     *                        If a type id becomes added are only add-ons
     *                        returned who match them. Default is for all types.
     */
    void GetInstallableAddonInfos(AddonInfos& addonInfos, const TYPE &type);

    /*!
     * @brief Returns a list of add-ons with given type, where related process
     * class becomes created.
     *
     * @note in case only standard parts who defined on addon.xml becomes used,
     * use 'GetAddonInfos' only!
     *
     * @param[out] addons list of add-on classes by requested type
     * @param[in] type requested add-on type for them add-on classes becomes
     *                 created
     * @param[in] enabledOnly [opt] is default to true, where only enabled
     *                        add-on's becomes added to list. If set to false
     *                        becomes all installed add-on's returned.
     */
    bool GetAddons(VECADDONS &addons, const TYPE &type, bool enabledOnly = true);

    /*!
     * @brief To get information from a installed add-on
     *
     * @param[in] addonId the add-on id to get the info for
     * @return add-on information pointer of installed add-on
     *
     * @note is recommended to use the 'type' value, it increase the
     * performance a bit.
     */
    const AddonInfoPtr GetInstalledAddonInfo(const std::string& addonId, const TYPE &type = ADDON_UNKNOWN);
    /*!
     * @brief Checks for available addon updates
     *
     * @return true if there is any addon with available updates, otherwise
     * false
     */
    bool HasAvailableUpdates();

    /*!
     * @brief Get addons with available updates
     *
     * @return the list of addon infos
     */
    AddonInfos GetAvailableUpdates();

    /*!
     * @brief Get the installable addon with the highest version.
     *
     * @param[in] addonId id of the addon
     * @param[out] addonInfo The add-on info if found by id
     * @return true if add-on is known from system, otherwise false
     */
    bool FindInstallableById(const std::string& addonId, AddonInfoPtr& addonInfo);

    /*!
     * @brief Check the given add-on id is a system one
     *
     * @param[in] addonId id of the addon to check
     * @return true if system add-on
     */
    bool IsSystemAddon(const std::string& addonId);

    /*!
     * @brief Compare the given add-on info to his related dependency versions.
     *
     * The dependency versions are set on addon.xml with:
     * ```
     * ...
     * <requires>
     *   <import addon="kodi.???" version="???"/>
     * </requires>
     * ...
     * ```
     *
     * @param[in] addonInfo The add-on properties to compare to dependency
     *                       versions.
     * @return true if compatible, if not returns it false.
     */
    bool IsCompatible(const AddonInfoPtr& addonInfo);

    /*!
     * @brief Check a add-on id is blacklisted
     *
     * @param[in] addonId The add-on id to check
     * @return true in case it is blacklisted, otherwise false
     */
    bool IsBlacklisted(const std::string& addonId) const;

    /*!
     * @brief Add add-on id to blacklist where it becomes noted that updates are
     * available.
     *
     * The add-on database becomes also updated with call from this function.
     *
     * @param[in] addonId The add-on id to blacklist
     * @return true if successfully done, otherwise false
     *
     * @note In case add-on is already in blacklist becomes the add skipped and
     * returns a true.
     */
    bool AddToUpdateBlacklist(const std::string& addonId);

    /*!
     * @brief Remove a add-on from blacklist in case a update is no more
     * possible or done.
     *
     * The add-on database becomes also updated with call from this function.
     *
     * @param[in] addonId The add-on id to blacklist
     * @return true if successfully done, otherwise false
     *
     * @note In case add-on is not in blacklist becomes the remove skipped and
     * returns a true.
     */
    bool RemoveFromUpdateBlacklist(const std::string& addonId);

    /*!
     * @brief Add a new addon to system and enable it by default after a
     * installation.
     *
     * @param[in] addonInfo informatio from new installed add-on
     * @return Returns true if the addon was successfully loaded and enabled,
     *         otherwise false.
     */
    bool AddNewInstalledAddon(AddonInfoPtr& addonInfo);

    /*!
     * @brief To update the last used time on database for the given add-on id
     *
     * @param[in] addonId The add-on id to set
     */
    void UpdateLastUsed(const std::string& addonId);

    /*!
     * @brief Checks for new / updated add-ons
     *
     * @return True if everything went ok, false otherwise
     */
    bool FindAddons();

    /*!
     * @brief Unload addon from the system.
     *
     * @param[in] addonInfo information class of the addon
     * @return Returns true if it was unloaded, otherwise false.
     */
    bool UnloadAddon(const AddonInfoPtr& addonInfo);

    /*!
     * @brief Disable an addon.
     *
     * @param[in] addonId id of the addon
     * @return Returns true on success, false on failure.
     */
    bool DisableAddon(const std::string& addonId);

    /*!
     * @brief Enable an addon.
     *
     * @param[in] addonId id of the addon
     * @return Returns true on success, false on failure.
     */
    bool EnableAddon(const std::string& addonId);

    /*!
     * @brief Check add-on id can installed
     *
     * @param[in] addonId id of the addon
     * @return Returns true on success, false on failure.
     */
    bool CanAddonBeEnabled(const std::string& addonId);

    /*!
     * @brief Check add-on can uninstalled
     *
     * @param[in] addonInfo information class of the addon
     * @return Returns true on if can, false if not.
     */
    bool CanUninstall(const AddonInfoPtr& addonInfo);

    /*!
     * @brief To find add-on's on given path
     *
     * @param[out] addonmap Map where finded add-on's becomes stored
     * @param[in] path The path string to check (the addon.xml must be present
     *                 on them)
     */
    static void FindAddons(AddonInfoList& addonmap, const std::string &path);

    /*!
     * @brief Load the addon in the given path
     *
     * This loads the addon using CAddonInfo creator with path as value which parses the addon descriptor file.
     *
     * @param[out] addon returned addon.
     * @param[in] path folder that contains the addon.
     * @return true if addons xml is valid, false otherwise.
     */
    static bool LoadAddonDescription(AddonInfoPtr &addon, const std::string &path);

    //! @todo need to investigate, change and remove
    //@{
    CEventStream<AddonEvent>& Events() { return m_events; }

    IAddonMgrCallback* GetCallbackForType(TYPE type);
    bool RegisterAddonMgrCallback(TYPE type, IAddonMgrCallback* cb);
    void UnregisterAddonMgrCallback(TYPE type);

    void AddToUpdateableAddons(AddonPtr &pAddon);
    void RemoveFromUpdateableAddons(AddonPtr &pAddon);    
    bool ReloadSettings(const std::string &id);

    /*!
     * @brief Start all services addons.
     *
     * @param[in] beforelogin To inform from which place the function is called
     * @return True is all addons are started, false otherwise
     */
    bool StartServices(const bool beforelogin);

    /*!
     * @brief Stop all services addons.
     *
     * @param[in] onlylogin if performed on login part of system
     */
    void StopServices(const bool onlylogin);

    /*!
     * @brief To see service add-on's are started
     *
     * @return true if services are started, otherwise false
     */
    bool ServicesHasStarted() const;

    /*!
     * @brief Checks whether an addon can be disabled via DisableAddon.
     *
     * @param[in] addonID id of the addon
     *
     * @sa DisableAddon
     */
    bool CanAddonBeDisabled(const std::string& addonID);
    //@}

  private:
    VECADDONS    m_updateableAddons;

    bool EnableSingle(const std::string& addonId);

    static std::map<TYPE, IAddonMgrCallback*> m_managers;
    CCriticalSection m_critSection;
    CAddonDatabase m_database;
    CEventSource<AddonEvent> m_events;
    bool m_serviceSystemStarted;

    /*!
     * @brief To resolv dependencies for given add-on id
     *
     * @param[in] addonId The add-on identification to check
     * @param[out] needed List of add-on id's of needed dependencies
     * @param[out] missing List of add-on id's of needed dependencies who
     *                     missing on system
     */
    void ResolveDependencies(const std::string& addonId, std::vector<std::string>& needed, std::vector<std::string>& missing);

    /*!
     * @brief To load the add-on manifest where is defined which are at least
     * required to start and run Kodi!
     *
     * @param[out] system List of required add-on's who must be present
     * @param[out] optional List of optional add-on's who can be present but not required
     * @return true if load is successfully done.
     */
    static bool LoadManifest(std::set<std::string>& system, std::set<std::string>& optional);

    /*!
     * @brief Add-on creation function
     *
     * With them becomes from given addon information the needed add-on class
     * generated. This class is defined with "CAddon" as parent and some
     * add-ons bring the own childs to them.
     *
     * @param[in] addonInfo Pointer to the add-on information class from where
     *                      it becomes created.
     * @return The pointer of created add-on class
     */
    static std::shared_ptr<CAddon> CreateAddon(AddonInfoPtr addonInfo, TYPE addonType);

    AddonInfoList m_installedAddons;
    AddonInfoList m_enabledAddons;
    std::set<std::string> m_systemAddons;
    std::set<std::string> m_optionalAddons;
    std::set<std::string> m_updateBlacklist;
  };

}; /* namespace ADDON */
