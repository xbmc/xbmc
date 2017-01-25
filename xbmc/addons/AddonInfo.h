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

#include <map>
#include <memory>
#include <set>
#include <vector>

class TiXmlElement;

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
    ADDON_VIDEO, // virtual addon types
    ADDON_AUDIO,
    ADDON_IMAGE,
    ADDON_EXECUTABLE,
    ADDON_GAME,
    ADDON_SCRAPER_LIBRARY,
    ADDON_SCRIPT_LIBRARY,
    ADDON_SCRIPT_MODULE,
    ADDON_GAME_CONTROLLER,
    ADDON_MAX
  } TYPE;

  class CAddonMgr;
  class AddonVersion;
  typedef std::map<std::string, std::pair<const AddonVersion, bool> > ADDONDEPS;
  typedef std::map<std::string, std::string> InfoMap;

  class CAddonInfo;
  typedef std::shared_ptr<CAddonInfo> AddonInfoPtr;
  typedef std::vector<AddonInfoPtr> AddonInfos;
  typedef std::vector<AddonInfoPtr>::iterator AddonInfosIter;

  class CAddon;
  class CAddonBuilder;

  class CAddonExtensions;
  typedef std::vector<std::pair<std::string, CAddonExtensions> > EXT_ELEMENTS;
  typedef std::vector<std::pair<std::string, std::string> > EXT_VALUE;
  typedef std::vector<std::pair<std::string, EXT_VALUE> > EXT_VALUES;
  class CAddonExtensions
  {
  public:
    CAddonExtensions();
    ~CAddonExtensions();
    bool ParseExtension(const TiXmlElement* element);

    std::string GetExtValue(std::string id) const;
    const EXT_VALUES& GetExtValues() const;
    CAddonExtensions* GetExtElement(std::string id);
    bool GetExtElements(std::string id, EXT_ELEMENTS &result);
    bool GetExtList(std::string id, std::vector<std::string> &result) const;

    void Insert(std::string id, std::string value);
//   private:
    bool m_ready;
    std::string m_point;
    EXT_VALUES m_values;
    EXT_ELEMENTS m_childs;
  };
  
  class CAddonInfo : public CAddonExtensions
  {
  public:
    enum SubContent
    {
      UNKNOWN,
      AUDIO,
      IMAGE,
      EXECUTABLE,
      VIDEO,
      GAME
    };

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

    CAddonInfo();
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

    void SetInstallDate(CDateTime installDate) { m_installDate = installDate; }
    void SetLastUpdated(CDateTime lastUpdated) { m_lastUpdated = lastUpdated; }
    void SetLastUsed(CDateTime lastUsed) { m_lastUsed = lastUsed; }
    void SetOrigin(std::string origin) { m_origin = std::move(origin); }

    const std::string& ID() const { return m_id; }
    TYPE Type() const { return m_type; }
    bool IsType(TYPE type) const;
    const AddonVersion& Version() const { return m_version; }
    const AddonVersion& MinVersion() const { return m_minversion; }
    const std::string& Name() const { return m_name; }
    const std::string& License() const { return m_license; }
    const std::string& Summary() const { return m_summary; }
    const std::string& Description() const { return m_description; }
    const std::string& Libname() const { return m_libname; }
    const std::string& Author() const { return m_author; }
    const std::string& Source() const { return m_source; }
    const std::string& Path() const { return m_path; }
    const std::string& Icon() const { return m_icon; }
    const std::string& ChangeLog() const { return m_changelog; }
    const std::string& FanArt() const { return m_fanart; }
    const std::vector<std::string>& Screenshots() const { return m_screenshots; }
    const std::string& Disclaimer() const { return m_disclaimer; }
    const ADDONDEPS& GetDeps() const { return m_dependencies; }
    const std::string& Broken() const { return m_broken; }
    const CDateTime& InstallDate() const { return m_installDate; }
    const CDateTime& LastUpdated() const { return m_lastUpdated; }
    const CDateTime& LastUsed() const { return m_lastUsed; }
    const std::string& Origin() const { return m_origin; }
    uint64_t PackageSize() const { return m_packageSize; }

    const InfoMap& ExtraInfo() const { return m_extrainfo; }

    std::string LibPath() const;

    bool ProvidesSubContent(const SubContent& content) const
    {
      return content == UNKNOWN ? false : m_providedSubContent.count(content) > 0;
    }

    bool ProvidesSeveralSubContents() const
    {
      return m_providedSubContent.size() > 1;
    }
  
    std::string SerializeMetadata();
    void DeserializeMetadata(const std::string& document);
    bool MeetsVersion(const AddonVersion &version) const;

    /*!
     * @brief Utilities to translate add-on parts to his requested part.
     */
    //@{
    static std::string TranslateType(TYPE type, bool pretty=false);
    static std::string TranslateIconType(TYPE type);
    static TYPE TranslateType(const std::string &string);
    //@}

  /*private: So long public until all add-on types reworked to new way! */
    bool m_usable;

    std::string m_id;
    TYPE m_type;
    AddonVersion m_version{"0.0.0"};
    AddonVersion m_minversion{"0.0.0"};
    std::string m_name;
    std::string m_license;
    std::string m_summary;
    std::string m_description;
    std::string m_libname;
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
    InfoMap m_extrainfo;
    CDateTime m_installDate;
    CDateTime m_lastUpdated;
    CDateTime m_lastUsed;
    std::string m_origin;
    uint64_t m_packageSize;

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
     * @brief Parse the given xml element about the used platform library name.
     *
     * @note The returned library names depends on the OS where Kodi is compiled.
     * @param[in] element The TinyXML element to read
     * @return The string of used library e.g. 'library_linux' for Linux
     */
    static const char* GetPlatformLibraryName(const TiXmlElement* element);

    static SubContent TranslateSubContent(const std::string &content);
  
    /*!
     * @brief Set the provided content for this plugin
     * 
     * If no valid content types are passed in, we set the EXECUTABLE type
     * 
     * @param content a space-separated list of content types
     */
    void SetProvides(const std::string &content);

    std::set<SubContent> m_providedSubContent;
  };

} /* namespace ADDON */
