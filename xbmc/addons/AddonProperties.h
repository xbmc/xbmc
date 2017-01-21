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

#include "addons/IAddon.h"
#include "addons/AddonVersion.h"

class TiXmlElement;

namespace ADDON
{

  class AddonProps;
  typedef std::shared_ptr<AddonProps> AddonPropsPtr;
  typedef std::vector<AddonPropsPtr> AddonInfos;
  typedef std::vector<AddonPropsPtr>::iterator AddonInfosIter;

  class CAddon;
  class CAddonBuilder;

  class AddonProps
  {
  public:
    /*!
     * @brief Class constructor for local available addons where his addon.xml
     * is present.
     *
     * @param[in] addonPath The folder name where addon.xml is included
     */
    AddonProps(std::string addonPath);

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
    AddonProps(const TiXmlElement* baseElement, std::string addonRepoXmlPath);

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
    AddonProps(const std::string& id,
               const AddonVersion& version,
               const std::string& name,
               const std::string& summary,
               const std::string& description,
               const std::string& metadata,
               const std::string& changelog,
               const std::string& origin);

    AddonProps();
    AddonProps(std::string id, TYPE type);

    /*!
     * @brief To ask generated class is usable and all needed parts are set.
     *
     * @return True is OK and usable, otherwise false.
     *
     * @note This function is only be related to constructor's who read the
     * addon xml part.
     */
    bool IsUsable() const { return m_usable; }

    const std::string& ID() const { return m_id; }
    TYPE Type() const { return m_type; }
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
    std::string ExtraInfoValueString(std::string id) const;
    bool ExtraInfoValueBool(std::string id) const;
    int ExtraInfoValueInt(std::string id) const;
    float ExtraInfoValueFloat(std::string id) const;

    std::string LibPath() const;

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
  };

} /* namespace ADDON */
