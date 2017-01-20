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
#include "Repository.h"
#include "threads/CriticalSection.h"
#include "utils/EventStream.h"
#include <string>
#include <vector>
#include <map>
#include <deque>


class DllLibCPluff;
extern "C"
{
#include "lib/cpluff/libcpluff/cpluff.h"
}

namespace ADDON
{
  class CAddonDll;
  typedef std::shared_ptr<CAddonDll> AddonDllPtr;

  typedef std::map<TYPE, VECADDONS> MAPADDONS;
  typedef std::map<TYPE, VECADDONS>::iterator IMAPADDONS;
  typedef std::vector<cp_cfg_element_t*> ELEMENTS;

  typedef std::map<std::string, AddonPropsPtr> AddonInfoList;
  typedef std::map<TYPE, AddonInfoList> AddonInfoMap;

  typedef std::vector<AddonPropsPtr> AddonInfos;

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
      virtual ~IAddonMgrCallback() {};
      virtual bool RequestRestart(AddonPtr addon, bool datachanged)=0;
      virtual bool RequestRemoval(AddonPtr addon)=0;
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

    CEventStream<AddonEvent>& Events() { return m_events; }

    IAddonMgrCallback* GetCallbackForType(TYPE type);
    bool RegisterAddonMgrCallback(TYPE type, IAddonMgrCallback* cb);
    void UnregisterAddonMgrCallback(TYPE type);

    /*! \brief Retrieve a specific addon (of a specific type)
     \param id the id of the addon to retrieve.
     \param addon [out] the retrieved addon pointer - only use if the function returns true.
     \param type type of addon to retrieve - defaults to any type.
     \param enabledOnly whether we only want enabled addons - set to false to allow both enabled and disabled addons - defaults to true.
     \return true if an addon matching the id of the given type is available and is enabled (if enabledOnly is true).
     */
    bool GetAddon(const std::string &id, AddonPtr &addon, const TYPE &type = ADDON_UNKNOWN, bool enabledOnly = true);

    AddonDllPtr GetAddon(const TYPE &type, const std::string &id);

    bool HasAddons(const TYPE &type);

    bool HasInstalledAddons(const TYPE &type);

    /*! Returns all installed, enabled add-ons. */
    bool GetAddons(VECADDONS& addons);

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

    /*! Get the installable addon with the highest version. */
    bool FindInstallableById(const std::string& addonId, AddonPtr& addon);

    void AddToUpdateableAddons(AddonPtr &pAddon);
    void RemoveFromUpdateableAddons(AddonPtr &pAddon);    
    bool ReloadSettings(const std::string &id);

    /*! Get addons with available updates */
    VECADDONS GetAvailableUpdates();

    /*! Returns true if there is any addon with available updates, otherwise false */
    bool HasAvailableUpdates();

    std::string GetTranslatedString(const cp_cfg_element_t *root, const char *tag);
    static AddonPtr AddonFromProps(AddonProps& props);

    /*! \brief Checks for new / updated add-ons
     \return True if everything went ok, false otherwise
     */
    bool FindAddons();

    /*! Unload addon from the system. Returns true if it was unloaded, otherwise false. */
    bool UnloadAddon(const AddonPtr& addon);

    /*! Returns true if the addon was successfully loaded and enabled; otherwise false. */
    bool ReloadAddon(AddonPtr& addon);

    /*! Hook for clearing internal state after uninstall. */
    void OnPostUnInstall(const std::string& id);

    /*! \brief Disable an addon. Returns true on success, false on failure. */
    bool DisableAddon(const std::string& ID);

    /*! \brief Enable an addon. Returns true on success, false on failure. */
    bool EnableAddon(const std::string& ID);

    /* \brief Check whether an addon has been disabled via DisableAddon.
     In case the disabled cache does not know about the current state the database routine will be used.
     \param ID id of the addon
     \sa DisableAddon
     */
    bool IsAddonDisabled(const std::string& ID);

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

    void UpdateLastUsed(const std::string& id);

    /* libcpluff */
    std::string GetExtValue(cp_cfg_element_t *base, const char *path) const;

    /*! \brief Retrieve an element from a given configuration element
     \param base the base configuration element.
     \param path the path to the configuration element from the base element.
     \param element [out] returned element.
     \return true if the configuration element is present
     */
    cp_cfg_element_t *GetExtElement(cp_cfg_element_t *base, const char *path);

    /*! \brief Retrieve a vector of repeated elements from a given configuration element
     \param base the base configuration element.
     \param path the path to the configuration element from the base element.
     \param result [out] returned list of elements.
     \return true if the configuration element is present and the list of elements is non-empty
     */
    bool GetExtElements(cp_cfg_element_t *base, const char *path, ELEMENTS &result);

    /*! \brief Retrieve a list of strings from a given configuration element
     Assumes the configuration element or attribute contains a whitespace separated list of values (eg xs:list schema).
     \param base the base configuration element.
     \param path the path to the configuration element or attribute from the base element.
     \param result [out] returned list of strings.
     \return true if the configuration element is present and the list of strings is non-empty
     */
    bool GetExtList(cp_cfg_element_t *base, const char *path, std::vector<std::string> &result) const;

    const cp_extension_t *GetExtension(const cp_plugin_info_t *props, const char *extension) const;

    /*! \brief Retrieves the platform-specific library name from the given configuration element
     */
    std::string GetPlatformLibraryName(cp_cfg_element_t *base) const;

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
     \param AddonProps [out] returned list of addon properties.
     \return true if the repository XML file is parsed, false otherwise.
     */
    bool AddonsFromRepoXML(const CRepository::DirInfo& repo, const std::string& xml, VECAddonProps& AddonProps);

    /*! \brief Start all services addons.
        \return True is all addons are started, false otherwise
    */
    bool StartServices(const bool beforelogin);
    /*! \brief Stop all services addons.
    */
    void StopServices(const bool onlylogin);

    bool ServicesHasStarted() const;

    static AddonPtr Factory(const cp_plugin_info_t* plugin, TYPE type);
    static bool Factory(const cp_plugin_info_t* plugin, TYPE type, CAddonBuilder& builder);
    static void FillCpluffMetadata(const cp_plugin_info_t* plugin, CAddonBuilder& builder);

    const AddonPropsPtr GetInstalledAddonInfo(TYPE addonType, std::string addonId);

    /*!
     * @brief Add add-on id to blacklist where it becomes noted that updates are
     * available.
     *
     * The add-on database becomes also updated with call from this function.
     *
     * @param[in] id The add-on id to blacklist
     * @return true if successfully done, otherwise false
     *
     * @note In case add-on is already in blacklist becomes the add skipped and
     * returns a true.
     */
    bool AddToUpdateBlacklist(const std::string& id);

    /*!
     * @brief Remove a add-on from blacklist in case a update is no more
     * possible or done.
     *
     * The add-on database becomes also updated with call from this function.
     *
     * @param[in] id The add-on id to blacklist
     * @return true if successfully done, otherwise false
     *
     * @note In case add-on is not in blacklist becomes the remove skipped and
     * returns a true.
     */
    bool RemoveFromUpdateBlacklist(const std::string& id);

    /*!
     * @brief Check a add-on id is blacklisted
     *
     * @param[in] id The add-on id to check
     * @return true in case it is blacklisted, otherwise false
     */
    bool IsBlacklisted(const std::string& id) const;

    /*!
     * @brief Get a list of add-on's with info's for the on system available
     * ones.
     *
     * @param[in] enabledOnly [opt] If true are only enabled ones given back,
     *                        if false all on system available. Default is true.
     * @param[in] type        [opt] The requested type, with "ADDON_UNKNOWN"
     *                        are all add-on types given back who match the case
     *                        with value before.
     *                        If a type id becomes added are only add-ons
     *                        returned who match them. Default is for all types.
     * @return The list with of available add-on's with info tables.
     */
    AddonInfos GetAddonInfos(bool enabledOnly = true, const TYPE &type = ADDON_UNKNOWN);

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
     * @param[in] addonProps The add-on properties to compare to dependency
     *                       versions.
     * @return true if compatible, if not returns it false.
     */
    bool IsCompatible(const AddonProps& addonProps);

  private:

    /* libcpluff */
    cp_context_t *m_cp_context;
    std::unique_ptr<DllLibCPluff> m_cpluff;
    VECADDONS    m_updateableAddons;

    /*! \brief Check whether this addon is supported on the current platform
     \param info the plugin descriptor
     \return true if the addon is supported, false otherwise.
     */
    static bool PlatformSupportsAddon(const cp_plugin_info_t *info);

    bool GetAddonsInternal(const TYPE &type, VECADDONS &addons, bool enabledOnly);
    bool EnableSingle(const std::string& id);

    std::set<std::string> m_disabled;
    static std::map<TYPE, IAddonMgrCallback*> m_managers;
    CCriticalSection m_critSection;
    CAddonDatabase m_database;
    CEventSource<AddonEvent> m_events;
    std::set<std::string> m_systemAddons;
    std::set<std::string> m_optionalAddons;
    bool m_serviceSystemStarted;

    void FindAddons(AddonInfoMap& addonmap, std::string path);
    const AddonPropsPtr GetInstalledAddonInfo(const std::string& addonId);

    AddonInfoMap m_installedAddons;
    AddonInfoMap m_enabledAddons;
    std::set<std::string> m_updateBlacklist;
  };

}; /* namespace ADDON */
