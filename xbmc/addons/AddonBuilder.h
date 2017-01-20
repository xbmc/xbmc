#pragma once
/*
 *      Copyright (C) 2016 Team Kodi
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
#include "IAddon.h"
#include "Service.h"

namespace ADDON
{

class CAddonBuilder
{
public:
  CAddonBuilder() : m_built(false), m_extPoint(nullptr) {}

  std::shared_ptr<IAddon> Build();
  void SetId(std::string id) { m_props.m_id = std::move(id); }
  void SetName(std::string name) { m_props.m_name = std::move(name); }
  void SetLicense(std::string license) { m_props.m_license = std::move(license); }
  void SetSummary(std::string summary) { m_props.m_summary = std::move(summary); }
  void SetDescription(std::string description) { m_props.m_description = std::move(description); }
  void SetDisclaimer(std::string disclaimer) { m_props.m_disclaimer = std::move(disclaimer); }
  void SetAuthor(std::string author) { m_props.m_author = std::move(author); }
  void SetSource(std::string source) { m_props.m_source = std::move(source); }
  void SetIcon(std::string icon) { m_props.m_icon = std::move(icon); }
  void SetFanart(std::string fanart) { m_props.m_fanart = std::move(fanart); }
  void SetScreenshots(std::vector<std::string> screenshots) { m_props.m_screenshots = std::move(screenshots); }
  void SetChangelog(std::string changelog) { m_props.m_changelog = std::move(changelog); }
  void SetBroken(std::string broken) { m_props.m_broken = std::move(broken); }
  void SetPath(std::string path) { m_props.m_path = std::move(path); }
  void SetLibName(std::string libname) { m_props.m_libname = std::move(libname); }
  void SetVersion(AddonVersion version) { m_props.m_version = std::move(version); }
  void SetMinVersion(AddonVersion minversion) { m_props.m_minversion = std::move(minversion); }
  void SetDependencies(ADDONDEPS dependencies) { m_props.m_dependencies = std::move(dependencies); }
  void SetExtrainfo(InfoMap extrainfo) { m_props.m_extrainfo = std::move(extrainfo); }
  void SetType(TYPE type) { m_props.m_type = type; }
  void SetExtPoint(cp_extension_t* ext) { m_extPoint = ext; }
  void SetInstallDate(CDateTime installDate) { m_props.m_installDate = installDate; }
  void SetLastUpdated(CDateTime lastUpdated) { m_props.m_lastUpdated = lastUpdated; }
  void SetLastUsed(CDateTime lastUsed) { m_props.m_lastUsed = lastUsed; }
  void SetOrigin(std::string origin) { m_props.m_origin = std::move(origin); }
  void SetPackageSize(uint64_t size) { m_props.m_packageSize = size; }

  const std::string& GetId() const { return m_props.m_id; }
  const AddonVersion& GetVersion() const { return m_props.m_version; }

private:
  static std::shared_ptr<IAddon> FromProps(AddonProps addonProps);

  bool m_built;
  AddonProps m_props;
  cp_extension_t* m_extPoint;
};

};