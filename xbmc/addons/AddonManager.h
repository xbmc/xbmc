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
#include "threads/CriticalSection.h"
#include "utils/Observer.h"
#include <string>
#include <vector>
#include <map>
#include <deque>
#include "AddonDatabase.h"

class DllLibCPluff;
extern "C"
{
#include "lib/cpluff/libcpluff/cpluff.h"
}

namespace ADDON
{
  typedef std::map<TYPE, VECADDONS> MAPADDONS;
  typedef std::map<TYPE, VECADDONS>::iterator IMAPADDONS;
  typedef std::vector<cp_cfg_element_t*> ELEMENTS;

  const std::string ADDON_METAFILE             = "description.xml";
  const std::string ADDON_VIS_EXT              = "*.vis";
  const std::string ADDON_PYTHON_EXT           = "*.py";
  const std::string ADDON_SCRAPER_EXT          = "*.xml";
  const std::string ADDON_SCREENSAVER_EXT      = "*.xbs";
  const std::string ADDON_PVRDLL_EXT           = "*.pvr";
  const std::string ADDON_DSP_AUDIO_EXT        = "*.adsp";
  const std::string ADDON_VERSION_RE = "(?<Major>\\d*)\\.?(?<Minor>\\d*)?\\.?(?<Build>\\d*)?\\.?(?<Revision>\\d*)?";

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
  class CAddonMgr : public Observable
  {
  public:
    static CAddonMgr &Get();
    bool ReInit() { DeInit(); return Init(); }
    bool Init();
    void DeInit();

    IAddonMgrCallback* GetCallbackForType(TYPE type);
    bool RegisterAddonMgrCallback(TYPE type, IAddonMgrCallback* cb);
    void UnregisterAddonMgrCallback(TYPE type);

    /* Addon access */
    bool GetDefault(const TYPE &type, AddonPtr &addon);
    bool SetDefault(const TYPE &type, const std::string &addonID);
    /*! \brief Retrieve a specific addon (of a specific type)
     \param id the id of the addon to retrieve.
     \param addon [out] the retrieved addon pointer - only use if the function returns true.
     \param type type of addon to retrieve - defaults to any type.
     \param enabledOnly whether we only want enabled addons - set to false to allow both enabled and disabled addons - defaults to true.
     \return true if an addon matching the id of the given type is available and is enabled (if enabledOnly is true).
     */
    bool GetAddon(const std::string &id, AddonPtr &addon, const TYPE &type = ADDON_UNKNOWN, bool enabledOnly = true);
    bool HasAddons(const TYPE &type, bool enabled = true);
    bool GetAddons(const TYPE &type, VECADDONS &addons, bool enabled = true);
    bool GetAllAddons(VECADDONS &addons, bool enabled = true);
    void AddToUpdateableAddons(AddonPtr &pAddon);
    void RemoveFromUpdateableAddons(AddonPtr &pAddon);    
    bool ReloadSettings(const std::string &id);
    /*! \brief Get all addons with available updates
     \param addons List to fill with all outdated addons
     \param getLocalVersion Whether to get the local addon version or the addon verion from the repository
     \return True if there are outdated addons otherwise false
     */
    bool GetAllOutdatedAddons(VECADDONS &addons, bool getLocalVersion = false);
    /*! \brief Checks if there is any addon with available updates
     \return True if there are outdated addons otherwise false
     */
    bool HasOutdatedAddons();
    std::string GetString(const std::string &id, const int number);

    std::string GetTranslatedString(const cp_cfg_element_t *root, const char *tag);
    static AddonPtr AddonFromProps(AddonProps& props);
    void FindAddons();
    void UnregisterAddon(const std::string& ID);

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

    /* \brief Checks whether an addon is installed.
     \param ID id of the addon
    */
    bool IsAddonInstalled(const std::string& ID);

    /* \brief Checks whether an addon can be installed. Broken addons can't be installed.
     \param ID id of the addon
     */
    bool CanAddonBeInstalled(const std::string& ID);

    /* \brief Checks whether an addon can be installed. Broken addons can't be installed.
    \param addon addon to be checked
    */
    bool CanAddonBeInstalled(const AddonPtr& addon);

    /* libcpluff */
    std::string GetExtValue(cp_cfg_element_t *base, const char *path);

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

    /*! \brief Load the addon in the given path
     This loads the addon using c-pluff which parses the addon descriptor file.
     \param path folder that contains the addon.
     \param addon [out] returned addon.
     \return true if addon is set, false otherwise.
     */
    bool LoadAddonDescription(const std::string &path, AddonPtr &addon);

    /*! \brief Load the addon in the given in-memory xml
     This loads the addon using c-pluff which parses the addon descriptor file.
     \param root Root element of an XML document.
     \param addon [out] returned addon.
     \return true if addon is set, false otherwise.
     */
    bool LoadAddonDescriptionFromMemory(const TiXmlElement *root, AddonPtr &addon);

    /*! \brief Parse a repository XML file for addons and load their descriptors
     A repository XML is essentially a concatenated list of addon descriptors.
     \param root Root element of an XML document.
     \param addons [out] returned list of addons.
     \return true if the repository XML file is parsed, false otherwise.
     */
    bool AddonsFromRepoXML(const TiXmlElement *root, VECADDONS &addons);

    /*! \brief Start all services addons.
        \return True is all addons are started, false otherwise
    */
    bool StartServices(const bool beforelogin);
    /*! \brief Stop all services addons.
    */
    void StopServices(const bool onlylogin);

  private:
    void LoadAddons(const std::string &path,
                    std::map<std::string, AddonPtr>& unresolved);

    /* libcpluff */
    cp_context_t *m_cp_context;
    DllLibCPluff *m_cpluff;
    VECADDONS    m_updateableAddons;

    /*! \brief Fetch a (single) addon from a plugin descriptor.
     Assumes that there is a single (non-trivial) extension point per addon.
     \param info the plugin descriptor
     \param type the extension point we want
     \return an AddonPtr based on the descriptor.  May be NULL if no suitable extension point is found.
     */
    AddonPtr GetAddonFromDescriptor(const cp_plugin_info_t *info,
                                    const std::string& type="");

    /*! \brief Check whether this addon is supported on the current platform
     \param info the plugin descriptor
     \return true if the addon is supported, false otherwise.
     */
    bool PlatformSupportsAddon(const cp_plugin_info_t *info) const;

    AddonPtr Factory(const cp_extension_t *props);
    bool CheckUserDirs(const cp_cfg_element_t *element);

    // private construction, and no assignements; use the provided singleton methods
    CAddonMgr();
    CAddonMgr(const CAddonMgr&);
    CAddonMgr const& operator=(CAddonMgr const&);
    virtual ~CAddonMgr();

    std::set<std::string> m_disabled;
    static std::map<TYPE, IAddonMgrCallback*> m_managers;
    CCriticalSection m_critSection;
    CAddonDatabase m_database;
  };

}; /* namespace ADDON */
