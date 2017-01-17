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
  // utils
  std::string TranslateType(TYPE type, bool pretty=false);
  std::string GetIcon(TYPE type);
  TYPE TranslateType(const std::string &string);

  class AddonProps;
  typedef std::shared_ptr<AddonProps> AddonPropsPtr;
  typedef std::vector<AddonPropsPtr> VECAddonProps;
  typedef std::vector<AddonPropsPtr>::iterator VECAddonPropsIter;

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
     * @param[in] addonXmlPath Path to the element related xml file, used
     *                         to know on log messages to source of fault
     */
    AddonProps(const TiXmlElement* baseElement, std::string addonXmlPath);

    AddonProps();
    AddonProps(std::string id, TYPE type);

    const std::string& ID() const { return id; }
    TYPE Type() const { return type; }
    const AddonVersion& Version() const { return version; }
    const AddonVersion& MinVersion() const { return minversion; }
    const std::string& Name() const { return name; }
    const std::string& License() const { return license; }
    const std::string& Summary() const { return summary; }
    const std::string& Description() const { return description; }
    const std::string& Libname() const { return libname; }
    const std::string& Author() const { return author; }
    const std::string& Source() const { return source; }
    const std::string& Path() const { return path; }
    const std::string& Icon() const { return icon; }
    const std::string& ChangeLog() const { return changelog; }
    const std::string& FanArt() const { return fanart; }
    const std::vector<std::string>& Screenshots() const { return screenshots; }
    const std::string& Disclaimer() const { return disclaimer; }
    const ADDONDEPS& GetDeps() const { return dependencies; }
    const std::string& Broken() const { return broken; }
    const InfoMap& ExtraInfo() const { return extrainfo; }
    const CDateTime& InstallDate() const { return installDate; }
    const CDateTime& LastUpdated() const { return lastUpdated; }
    const CDateTime& LastUsed() const { return lastUsed; }
    const std::string& Origin() const { return origin; }
    uint64_t PackageSize() const { return packageSize; }

    /*!
     * @brief Used to set path for not local available addons who are related
     * to a repository.
     *
     * @param[in] strPath Path where addon is available (normally with a *.zip
     *                    ending.
     */
    void SetPath(std::string strPath) { path = std::move(strPath); }
    bool IsUsable() const { return m_usable; }

  /*private: So long public until all add-on types reworked to new way! */
    bool m_usable;

    std::string id;
    TYPE type;
    AddonVersion version{"0.0.0"};
    AddonVersion minversion{"0.0.0"};
    std::string name;
    std::string license;
    std::string summary;
    std::string description;
    std::string libname;
    std::string author;
    std::string source;
    std::string path;
    std::string icon;
    std::string changelog;
    std::string fanart;
    std::vector<std::string> screenshots;
    std::string disclaimer;
    ADDONDEPS dependencies;
    std::string broken;
    InfoMap extrainfo;
    CDateTime installDate;
    CDateTime lastUpdated;
    CDateTime lastUsed;
    std::string origin;
    uint64_t packageSize;

    /*!
     * @brief Function to load data xml file to set all property values
     *
     * @param[in] element The tiny xml element to read
     * @param[in] addonXmlPath Path to the element related xml file, used
     *                         to know on log messages to source of fault
     * @return true if successfully done, otherwise false
     */
    bool LoadAddonXML(const TiXmlElement* element, std::string addonXmlPath);
  };

} /* namespace ADDON */
