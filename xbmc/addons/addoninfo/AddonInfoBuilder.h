/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "addons/IAddon.h"

#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class CDateTime;
class TiXmlElement;

namespace ADDON
{
enum class AddonLifecycleState;
enum class AddonType;

class CAddonExtensions;
class CAddonType;

struct DependencyInfo;
struct RepositoryDirInfo;

class CAddonInfo;
using AddonInfoPtr = std::shared_ptr<CAddonInfo>;

class CAddonInfoBuilder
{
public:
  static AddonInfoPtr Generate(const std::string& id, AddonType type);
  static AddonInfoPtr Generate(const std::string& addonPath, bool platformCheck = true);
  static AddonInfoPtr Generate(const TiXmlElement* baseElement,
                               const RepositoryDirInfo& repo,
                               bool platformCheck = true);

  /*!
    * @brief Parts used from CAddonDatabase
    */
  //@{
  static void SetInstallData(const AddonInfoPtr& addon, const CDateTime& installDate,
                             const CDateTime& lastUpdated, const CDateTime& lastUsed, const std::string& origin);
  //@}

private:
  static bool ParseXML(const AddonInfoPtr& addon,
                       const TiXmlElement* element,
                       const std::string& addonPath);
  static bool ParseXML(const AddonInfoPtr& addon,
                       const TiXmlElement* element,
                       const std::string& addonPath,
                       const RepositoryDirInfo& repo);
  static bool ParseXMLTypes(CAddonType& addonType,
                            const AddonInfoPtr& info,
                            const TiXmlElement* child);
  static bool ParseXMLExtension(CAddonExtensions& addonExt, const TiXmlElement* element);
  static bool GetTextList(const TiXmlElement* element, const std::string& tag, std::unordered_map<std::string, std::string>& translatedValues);
  static const char* GetPlatformLibraryName(const TiXmlElement* element);
  static bool PlatformSupportsAddon(const AddonInfoPtr& addon);
};

class CAddonInfoBuilderFromDB
{
public:
  CAddonInfoBuilderFromDB();

  void SetId(std::string id) const;
  void SetName(std::string name) const;
  void SetLicense(std::string license) const;
  void SetSummary(std::string summary) const;
  void SetDescription(std::string description) const;
  void SetDisclaimer(std::string disclaimer) const;
  void SetAuthor(std::string author) const;
  void SetSource(std::string source) const;
  void SetWebsite(std::string website) const;
  void SetForum(std::string forum) const;
  void SetEMail(std::string email) const;
  void SetIcon(std::string icon) const;
  void SetArt(const std::string& type, std::string value) const;
  void SetArt(std::map<std::string, std::string> art) const;
  void SetScreenshots(std::vector<std::string> screenshots) const;
  void SetChangelog(std::string changelog) const;
  void SetLifecycleState(AddonLifecycleState state, std::string description) const;
  void SetPath(std::string path) const;
  void SetLibName(std::string libname) const;
  void SetVersion(CAddonVersion version) const;
  void SetDependencies(std::vector<DependencyInfo> dependencies) const;
  void SetExtrainfo(InfoMap extrainfo) const;
  void SetInstallDate(const CDateTime& installDate) const;
  void SetLastUpdated(const CDateTime& lastUpdated) const;
  void SetLastUsed(const CDateTime& lastUsed) const;
  void SetOrigin(std::string origin) const;
  void SetPackageSize(uint64_t size) const;
  void SetExtensions(CAddonType addonType) const;

  const AddonInfoPtr& get() { return m_addonInfo; }

private:
  AddonInfoPtr m_addonInfo;
};
}
