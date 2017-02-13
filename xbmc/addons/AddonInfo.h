#pragma once
/*
 *      Copyright (C) 2005-2017 Team Kodi
 *      http://kodi.tv
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
 *  along with Kodi; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "XBDateTime.h"
#include "addons/AddonVersion.h"
#include "utils/StringUtils.h"

#include <map>
#include <memory>
#include <set>
#include <stdlib.h>
#include <string>
#include <unordered_set>
#include <vector>

class TiXmlElement;
namespace XBMCAddon { namespace xbmcgui { class WindowXML; } } 
namespace KodiAPI { namespace GUI { class CAddonCallbacksGUI; } } 

namespace ADDON
{

  typedef enum
  {
    ADDON_UNKNOWN,
    ADDON_VIZ,
    ADDON_SKIN,
    ADDON_PVRDLL,
    ADDON_ADSPDLL,
    ADDON_INPUTSTREAM,
    ADDON_GAMEDLL,
    ADDON_PERIPHERALDLL,
    ADDON_SCRIPT,
    ADDON_SCRIPT_WEATHER,
    ADDON_SUBTITLE_MODULE,
    ADDON_SCRIPT_LYRICS,
    ADDON_SCRAPER_ALBUMS,
    ADDON_SCRAPER_ARTISTS,
    ADDON_SCRAPER_MOVIES,
    ADDON_SCRAPER_MUSICVIDEOS,
    ADDON_SCRAPER_TVSHOWS,
    ADDON_SCREENSAVER,
    ADDON_PLUGIN,
    ADDON_REPOSITORY,
    ADDON_WEB_INTERFACE,
    ADDON_SERVICE,
    ADDON_AUDIOENCODER,
    ADDON_CONTEXT_ITEM,
    ADDON_AUDIODECODER,
    ADDON_RESOURCE_IMAGES,
    ADDON_RESOURCE_LANGUAGE,
    ADDON_RESOURCE_UISOUNDS,
    ADDON_RESOURCE_GAMES,
    ADDON_VFS,
    ADDON_IMAGEDECODER,
    ADDON_SCRAPER_LIBRARY,
    ADDON_SCRIPT_LIBRARY,
    ADDON_SCRIPT_MODULE,
    ADDON_GAME_CONTROLLER,
    ADDON_VIDEOCODEC,

    /**
     * @brief virtual addon types
     */
    //@{
    ADDON_VIDEO,
    ADDON_AUDIO,
    ADDON_IMAGE,
    ADDON_EXECUTABLE,
    ADDON_GAME,
    //@}

    ADDON_MAX
  } TYPE;

  struct extValue
  {
    extValue(const std::string& strValue) : str(strValue) { }
    const std::string asString()  const { return str; }
    const bool asBoolean() const { return StringUtils::EqualsNoCase(str, "true"); }
    const int asInteger() const { return atoi(str.c_str()); }
    const float asFloat() const { return static_cast<float>(atof(str.c_str())); }
    const bool empty() const { return str.empty(); }
    const std::string str;
  };

  class CAddonExtensions;
  typedef std::vector<std::pair<std::string, CAddonExtensions> > EXT_ELEMENTS;
  typedef std::vector<std::pair<std::string, extValue> > EXT_VALUE;
  typedef std::vector<std::pair<std::string, EXT_VALUE> > EXT_VALUES;

  class CAddonExtensions
  {
  public:
    CAddonExtensions() { }
    ~CAddonExtensions() { }
    bool ParseExtension(const TiXmlElement* element);

    const extValue GetValue(std::string id) const;
    const EXT_VALUES& GetValues() const;
    const CAddonExtensions* GetElement(std::string id) const;
    const EXT_ELEMENTS GetElements(std::string id = "") const;

    void Insert(std::string id, std::string value);

  private:
    std::string m_point;
    EXT_VALUES m_values;
    EXT_ELEMENTS m_childs;
  };

  class CAddonInfo;
  typedef std::shared_ptr<CAddonInfo> AddonInfoPtr;
  typedef std::vector<AddonInfoPtr> AddonInfos;

  /*!
   */
  class CAddonType : public CAddonExtensions
  {
  public:
    CAddonType(TYPE type, CAddonInfo* info, const TiXmlElement* child);

    const TYPE Type() const { return m_type; }
    std::string LibPath() const;
    std::string LibName() const { return m_libname; }
    bool ProvidesSubContent(const TYPE& content) const
    {
      return content == ADDON_UNKNOWN ? false : m_type == content || m_providedSubContent.count(content) > 0;
    }

    bool ProvidesSeveralSubContents() const
    {
      return m_providedSubContent.size() > 1;
    }

    int ProvidedSubContents() const
    {
      return m_providedSubContent.size();
    }

    const char* GetPlatformLibraryName(const TiXmlElement* element);
    void SetProvides(const std::string &content);

  private:
    friend class CAddonInfo;

    TYPE m_type;
    std::string m_path;
    std::string m_libname;
    std::set<TYPE> m_providedSubContent;
  };

  class IAddonInstanceHandler
  {
  public:
    IAddonInstanceHandler(TYPE type) : m_type(type) { }

    TYPE UsedType() { return m_type; }

  private:
    TYPE m_type;
  };

  class CAddonMgr;
  class CAddonDll;
  typedef std::shared_ptr<CAddonDll> AddonDllPtr;
  class AddonVersion;
  typedef std::map<std::string, std::pair<const AddonVersion, bool> > ADDONDEPS;

  /*!
   * @brief Add-on Information class
   *
   * This class stores all available standard informations of add-ons. This can
   * be as source the content of a repository or with local installed ones.
   *
   * To generate the information becomes the addon.xml read.
   */
  class CAddonInfo
  {
  public:
    /*!
     * @brief Class constructor for local available addons where his addon.xml
     * is present.
     *
     * @param[in] addonPath The folder name where addon.xml is included
     */
    CAddonInfo(std::string addonPath);

    /*!
     * @brief Class constructor used for repository list of addons where not
     * available on local system.
     *
     * Source is normally a "addons.xml"
     *
     * @param[in] baseElement The TinyXML base element to parse
     * @param[in] addonRepoXmlPath Path to the element related xml file, used
     *                             to know on log messages to source of fault
     *                             For this class constructor it contains the
     *                             big addons.xml from repository!
     */
    CAddonInfo(const TiXmlElement* baseElement, std::string addonRepoXmlPath);

    /*!
     * @brief Class constructor used for already known parts, like from
     * database.
     *
     * Source is normally a "addons.xml"
     *
     * @param[in] id Identification string of add-on
     * @param[in] version Version of them
     * @param[in] name Add-on name
     * @param[in] summary Summary description
     * @param[in] description Bigger description
     * @param[in] metadata Other data from add-on (includes also his type)
     * @param[in] changelog The changelog
     * @param[in] origin
     */
    CAddonInfo(const std::string& id,
               const AddonVersion& version,
               const std::string& name,
               const std::string& summary,
               const std::string& description,
               const std::string& metadata,
               const std::string& changelog,
               const std::string& origin);

    /*!
     * @brief Constructor used on XBMCAddon::xbmcgui::WindowXML
     *
     * Used there to create a Pseudo Skin Add-on used upon the Python add-on
     * interface for a code generated window.
     *
     * @param[in] id used id for the info
     * @param[in] type add-on type used for the info
     */
    CAddonInfo(std::string id, TYPE type);

    /*!
     * @brief To ask generated class is usable and all needed parts are set.
     *
     * @return True is OK and usable, otherwise false.
     *
     * @note This function is only be related to constructor's who read the
     * addon xml part.
     */
    bool IsUsable() const { return m_usable; }

    /*!
     * @brief Parts used from ADDON::CAddonDatabase
     */
    //@{
    void SetInstallDate(CDateTime installDate) { m_installDate = installDate; }
    void SetLastUpdated(CDateTime lastUpdated) { m_lastUpdated = lastUpdated; }
    void SetLastUsed(CDateTime lastUsed) { m_lastUsed = lastUsed; }
    void SetOrigin(std::string origin) { m_origin = std::move(origin); }
    //@}

    /*!
     * @brief Get the ID from add-on
     *
     * The ID is defined in addon.xml but should also equal with add-on path!
     *
     * @return ID string of add-on
     */
    const std::string& ID() const { return m_id; }

    /*!
     * @brief The master type id of add-on
     *
     * @note to get a human readable name for the type can be TranslateType(...)
     * from here used.
     *
     * @return Master Type ID of add-on
     */ 
    TYPE MainType() const { return m_mainType; }

    /*!
     * @brief To check add-on to support type
     *
     * This is a bit different from 'Type()' call, there becomes also the
     * sub content checked.
     *
     * @return true if main type of sub typo is present on add-on
     */
    bool IsType(TYPE type) const;

    /*!
     * @brief Get all available types from add-on
     *
     * @return a list of types supported on add-on
     */
    const std::vector<CAddonType>& Types() const { return m_types; }

    /*!
     * @brief Get the type class from given type identifier
     *
     * @param[in] type The type to select, to become the master use
     *                 ADDON_UNKNOWN
     * @return Type class or `nullptr` if not present
     */
    const CAddonType* Type(TYPE type) const;

    /*!
     * @brief Get the library path where the add-on is present with his
     * library/exe part.
     *
     * The library path can be a local and also for them on repository with
     * URL to them.
     *
     * @return Path with lib where add-on is present, returns empty if no
     * libname is defined.
     */
    std::string MainLibPath() const;

    /*!
     * @brief To get version of add-on
     *
     * @return The AddonVersion class from add-on
     */
    const AddonVersion& Version() const { return m_version; }

    /*!
     * @brief To get min version of add-on
     *
     * Used to check compatibility with others who request them
     *
     * @return The AddonVersion class from add-on with minimum version
     */
    const AddonVersion& MinVersion() const { return m_minversion; }

    /*!
     * @brief Human readable name of add-on
     *
     * @return Add-on name
     */
    const std::string& Name() const { return m_name; }

    /*!
     * @brief Licence text of add-on
     *
     * @return the complete licence text defined on addon.xml
     */
    const std::string& License() const { return m_license; }

    /*!
     * @brief To get a small add-on description text
     *
     * @return description text
     */
    const std::string& Summary() const { return m_summary; }

    /*!
     * @brief To get a bigger add-on description
     *
     * @return description text
     */
    const std::string& Description() const { return m_description; }

    /*!
     * @brief To get the master library name of an add-on
     *
     * Currently is this used only for binary add-ons where only one library is
     * present where everything becomes included.
     *
     * @return the library name of add-on
     */
    const std::string& Libname() const { return m_types[0].m_libname; }

    /*!
     * @brief to get the author of add-on
     * @return Author name
     */
    const std::string& Author() const { return m_author; }

    /*!
     * @brief The source URL of the add-on
     * @return Source URL where add-on is from
     */
    const std::string& Source() const { return m_source; }

    /*!
     * @brief Path where add-on is present
     *
     * Can be local or also repository URL's
     *
     * @return Path where add-on is
     */
    const std::string& Path() const { return m_path; }

    /*!
     * @brief To get Path with icon file
     *
     * @return If a icon present a path to them, otherwise empty
     */
    const std::string& Icon() const { return m_icon; }

    /*!
     * @brief To get a changelog from add-on
     *
     * Used to inform the user about changes
     *
     * @return Changelog text
     */
    const std::string& ChangeLog() const { return m_changelog; }

    /*!
     * @brief To get a add-on fanart picture
     *
     * Is used as background on kodi's add-on window.
     *
     * @return Path with image who used as fanart
     */
    const std::string& FanArt() const { return m_fanart; }

    /*!
     * @brief To get a list of available screenshots from add-on
     *
     * Used on kodi's add-on window to descripe a bit from add-on
     *
     * @return a list of screenshots if present
     */
    const std::vector<std::string>& Screenshots() const { return m_screenshots; }

    /*!
     * @brief To get a disclaimer of warranties
     *
     * @return a disclaimer of addon
     */
    const std::string& Disclaimer() const { return m_disclaimer; }

    /*!
     * @brief To get a list with dependencies related to this add-on
     */
    const ADDONDEPS& GetDeps() const { return m_dependencies; }

    /*!
     * @brief To get broken parts
     *
     * @todo make description more detailed
     */
    const std::string& Broken() const { return m_broken; }

    /*!
     * @brief To get install date of add-on
     *
     * @note this is only be set from add-on browser and not always present!
     */
    const CDateTime& InstallDate() const { return m_installDate; }

    /*!
     * @brief To get last updated date of add-on
     *
     * @note this is only be set from add-on browser and not always present!
     */
    const CDateTime& LastUpdated() const { return m_lastUpdated; }

    /*!
     * @brief To get the last used date of add-on
     *
     * @note this is only be set from add-on browser and not always present!
     */
    const CDateTime& LastUsed() const { return m_lastUsed; }

    /*!
     * @brief The origin of add-on
     *
     * @todo make description more detailed
     * @note this is only be set from add-on browser and not always present!
     */
    const std::string& Origin() const { return m_origin; }

    /*!
     * @brief Size of the add-on package in Bytes
     */
    uint64_t PackageSize() const { return m_packageSize; }

    /*!
     * @brief User language of add-on
     *
     * @return the on addon.xml defined language string
     */
    std::string Language() { return m_language; }

    /*!
     * @brief Check the information about supported sub content
     *
     * @param[in] content Sub content type to check present
     * @param[in] mainType [opt] Type to check sub type from, default to
     *                     ADDON_UNKNOWN who check all included parts.
     * @return true if present, otherwise false
     */
    bool ProvidesSubContent(const TYPE& content, const TYPE& mainType = ADDON_UNKNOWN) const;

    /*!
     * @brief To check addon contains several sub contents
     *
     * e.g. Music together with Video
     *
     * @return true if more as one is supported
     */
    bool ProvidesSeveralSubContents() const;
  
    std::string SerializeMetadata();
    bool DeserializeMetadata(const std::string& document);

    /*!
     * @brief Checks the information to match asked version
     *
     * @param[in] version Version to check
     * @return true if add-on's version match asked version, otherwise false
     */
    bool MeetsVersion(const AddonVersion &version) const;

    /*!
     * @brief Utilities to translate add-on parts to his requested part.
     */
    //@{
    static std::string TranslateType(TYPE type, bool pretty=false);
    static std::string TranslateIconType(TYPE type);
    static TYPE TranslateType(const std::string &string);
    static TYPE TranslateSubContent(const std::string &content);
    //@}

  private:
    friend class XBMCAddon::xbmcgui::WindowXML;
    friend class KodiAPI::GUI::CAddonCallbacksGUI;

    bool m_usable;

    std::string m_id;
    TYPE m_mainType;
    std::vector<CAddonType> m_types;

    AddonVersion m_version{"0.0.0"};
    AddonVersion m_minversion{"0.0.0"};
    std::string m_name;
    std::string m_license;
    std::string m_summary;
    std::string m_description;
    std::string m_author;
    std::string m_source;
    std::string m_path;
    std::string m_icon;
    std::string m_changelog;
    std::string m_fanart;
    std::vector<std::string> m_screenshots;
    std::string m_disclaimer;
    ADDONDEPS m_dependencies;
    std::string m_broken;
    CDateTime m_installDate;
    CDateTime m_lastUpdated;
    CDateTime m_lastUsed;
    std::string m_origin;
    uint64_t m_packageSize;
    std::string m_language;

    /*!
     * @brief Function to load data xml file to set all property values
     *
     * @param[in] element The tiny xml element to read
     * @param[in] addonXmlPath Path to the element related xml file, used
     *                         to know on log messages to source of fault
     * @return true if successfully done, otherwise false
     */
    bool LoadAddonXML(const TiXmlElement* element, std::string addonXmlPath);

  /*!
   * @brief active addon handle parts
   *
   * This area contains the created addon class and all active instances of the
   * add-on. If active ones are empty becomes the pointer reseted. For this
   * reasons is CAddonMgr added as friend here and the private separated from
   * the rest.
   *
   * @todo the AddonDllPtr need to removed in future and everything done only
   * with AddonPtr.
   */
  //@{
  private:
    friend class CAddonMgr;
    AddonDllPtr m_activeAddon;
    std::unordered_set<const IAddonInstanceHandler*> m_activeAddonHandlers;
  //@}
  };

} /* namespace ADDON */
