/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "addons/Repository.h"
#include "addons/addoninfo/AddonInfo.h"

class TiXmlElement;

namespace ADDON
{

class CAddonInfoBuilder
{
public:
  static AddonInfoPtr Generate(const std::string& id, TYPE type);
  static AddonInfoPtr Generate(const std::string& addonPath, bool platformCheck = true);
  static AddonInfoPtr Generate(const TiXmlElement* baseElement, const CRepository::DirInfo& repo, bool platformCheck = true);

  class CFromDB
  {
  public:
    CFromDB() : m_addonInfo(std::make_shared<CAddonInfo>()) { }

    void SetId(std::string id) { m_addonInfo->m_id = std::move(id); }
    void SetName(std::string name) { m_addonInfo->m_name = std::move(name); }
    void SetLicense(std::string license) { m_addonInfo->m_license = std::move(license); }
    void SetSummary(std::string summary) { m_addonInfo->m_summary.insert(std::pair<std::string, std::string>("unk", std::move(summary))); }
    void SetDescription(std::string description) { m_addonInfo->m_description.insert(std::pair<std::string, std::string>("unk", std::move(description))); }
    void SetDisclaimer(std::string disclaimer) { m_addonInfo->m_disclaimer.insert(std::pair<std::string, std::string>("unk", std::move(disclaimer))); }
    void SetAuthor(std::string author) { m_addonInfo->m_author = std::move(author); }
    void SetSource(std::string source) { m_addonInfo->m_source = std::move(source); }
    void SetWebsite(std::string website) { m_addonInfo->m_website = std::move(website); }
    void SetForum(std::string forum) { m_addonInfo->m_forum = std::move(forum); }
    void SetEMail(std::string email) { m_addonInfo->m_email = std::move(email); }
    void SetIcon(std::string icon) { m_addonInfo->m_icon = std::move(icon); }
    void SetArt(std::string type, std::string value) { m_addonInfo->m_art[type] = value; }
    void SetArt(std::map<std::string, std::string> art) { m_addonInfo->m_art = std::move(art); }
    void SetScreenshots(std::vector<std::string> screenshots) { m_addonInfo->m_screenshots = std::move(screenshots); }
    void SetChangelog(std::string changelog) { m_addonInfo->m_changelog.insert(std::pair<std::string, std::string>("unk", std::move(changelog))); }
    void SetBroken(std::string broken) { m_addonInfo->m_broken = std::move(broken); }
    void SetPath(std::string path) { m_addonInfo->m_path = std::move(path); }
    void SetLibName(std::string libname) { m_addonInfo->m_libname = std::move(libname); }
    void SetVersion(AddonVersion version) { m_addonInfo->m_version = std::move(version); }
    void SetMinVersion(AddonVersion minversion) { m_addonInfo->m_minversion = std::move(minversion); }
    void SetDependencies(std::vector<DependencyInfo> dependencies) { m_addonInfo->m_dependencies = std::move(dependencies); }
    void SetExtrainfo(InfoMap extrainfo)
    {
      m_addonInfo->m_extrainfo = std::move(extrainfo);

      const auto& it = m_addonInfo->m_extrainfo.find("provides");
      if (it != m_addonInfo->m_extrainfo.end())
      {
        CAddonType addonType(m_addonInfo->m_mainType);
        addonType.SetProvides(it->second);
        m_addonInfo->m_types.push_back(addonType);
      }
    }
    void SetType(TYPE type) { m_addonInfo->m_mainType = type; }
    void SetInstallDate(const CDateTime& installDate) { m_addonInfo->m_installDate = installDate; }
    void SetLastUpdated(const CDateTime& lastUpdated) { m_addonInfo->m_lastUpdated = lastUpdated; }
    void SetLastUsed(const CDateTime& lastUsed) { m_addonInfo->m_lastUsed = lastUsed; }
    void SetOrigin(std::string origin) { m_addonInfo->m_origin = std::move(origin); }
    void SetPackageSize(uint64_t size) { m_addonInfo->m_packageSize = size; }

    const AddonInfoPtr& get() { return m_addonInfo; }

  private:
    AddonInfoPtr m_addonInfo;
  };

  /*!
    * @brief Parts used from CAddonDatabase
    */
  //@{
  static void SetInstallData(const AddonInfoPtr& addon, const CDateTime& installDate,
                             const CDateTime& lastUpdated, const CDateTime& lastUsed, const std::string& origin);
  //@}

private:
  static bool ParseXML(const AddonInfoPtr& addon, const TiXmlElement* element, const std::string& addonPath, const CRepository::DirInfo& repo = {});
  static bool ParseXMLTypes(CAddonType& addonType, AddonInfoPtr info, const TiXmlElement* child);
  static bool ParseXMLExtension(CAddonExtensions& addonExt, const TiXmlElement* element);
  static bool GetTextList(const TiXmlElement* element, const std::string& tag, std::unordered_map<std::string, std::string>& translatedValues);
  static const char* GetPlatformLibraryName(const TiXmlElement* element);
  static bool PlatformSupportsAddon(const AddonInfoPtr& addon);
};

}
