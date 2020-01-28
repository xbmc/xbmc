/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "XBDateTime.h"
#include "addons/AddonVersion.h"
#include "addons/addoninfo/AddonType.h"

#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace ADDON
{

class CAddonBuilder;
class CAddonInfo;
typedef std::shared_ptr<CAddonInfo> AddonInfoPtr;
typedef std::vector<AddonInfoPtr> AddonInfos;

struct DependencyInfo
{
  std::string id;
  AddonVersion versionMin, version;
  bool optional;
  DependencyInfo(std::string id, AddonVersion versionMin, AddonVersion version, bool optional)
    : id(id)
    , versionMin(versionMin.empty() ? version : versionMin)
    , version(version)
    , optional(optional)
  {
  }

  bool operator==(const DependencyInfo& rhs) const
  {
    return id == rhs.id && versionMin == rhs.versionMin && version == rhs.version &&
           optional == rhs.optional;
  }

  bool operator!=(const DependencyInfo& rhs) const
  {
    return !(rhs == *this);
  }
};

typedef std::map<std::string, std::string> InfoMap;
typedef std::map<std::string, std::string> ArtMap;

class CAddonInfoBuilder;

class CAddonInfo
{
public:
  CAddonInfo() = default;
  CAddonInfo(std::string id, TYPE type);

  void SetMainType(TYPE type) { m_mainType = type; }
  void SetLibName(const std::string& libname) { m_libname = libname; }
  void SetPath(const std::string& path) { m_path = path; }
  void AddExtraInfo(const std::string& idName, const std::string& value) { m_extrainfo[idName] = value; }
  void SetLastUsed(const CDateTime& dateTime) { m_lastUsed = dateTime; }

  const std::string& ID() const { return m_id; }

  /**
   * @brief To get the main type of this addon
   *
   * This is the first type defined in addon.xml.
   *
   * @return The used main type of addon
   */
  TYPE MainType() const { return m_mainType; }

  /**
   * @brief To check addon contains a type
   *
   * @param[in] type The to checked type identifier
   * @param[in] mainOnly to check only in first defined main addon inside addon.xml
   * @return true in case the wanted type is supported, false if not
   */
  bool HasType(TYPE type, bool mainOnly = false) const;

  /**
   * @brief To get all available types inside the addon
   *
   * To have all `<extension point="..." />` defined in addon.xml inside a list.
   *
   * @return List of all supported types
   */
  const std::vector<CAddonType>& Types() const { return m_types; }

  /**
   * @brief The get for given addon type information and extension data
   *
   * @param[in] type The wanted type data
   * @return addon type class with @ref CAddonExtensions as information
   *
   * @note This function return never a "nullptr", in case the wanted type is
   * not supported, becomes a dummy of @ref CAddonType given.
   *
   * ------------------------------------------------------------------------
   *
   * **Example:**
   * ~~~~~~~~~~~~~{.cpp}
   * // To get <extension ... name="blablabla" /> from addon.xml
   * std::string name = Type(ADDON_...)->GetValue("@name").asString();
   * ~~~~~~~~~~~~~
   *
   */
  const CAddonType* Type(TYPE type) const;

  bool ProvidesSubContent(const TYPE& content, const TYPE& mainType = ADDON_UNKNOWN) const;
  bool ProvidesSeveralSubContents() const;

  const AddonVersion& Version() const { return m_version; }
  const AddonVersion& MinVersion() const { return m_minversion; }
  const AddonVersion& DependencyMinVersion(const std::string& dependencyID) const;
  const AddonVersion& DependencyVersion(const std::string& dependencyID) const;
  const std::string& Name() const { return m_name; }
  const std::string& License() const { return m_license; }
  const std::string& Summary() const { return GetTranslatedText(m_summary); }
  const std::string& Description() const { return GetTranslatedText(m_description); }
  const std::string& LibName() const { return m_libname; }
  const std::string& Author() const { return m_author; }
  const std::string& Source() const { return m_source; }
  const std::string& Website() const { return m_website; }
  const std::string& Forum() const { return m_forum; }
  const std::string& EMail() const { return m_email; }
  const std::string& Path() const { return m_path; }
  const std::string& ChangeLog() const { return GetTranslatedText(m_changelog); }
  const std::string& Icon() const { return m_icon; }
  const ArtMap& Art() const { return m_art; }
  const std::vector<std::string>& Screenshots() const { return m_screenshots; }
  const std::string& Disclaimer() const { return GetTranslatedText(m_disclaimer); }
  const std::vector<DependencyInfo>& GetDependencies() const { return m_dependencies; }
  const std::string& Broken() const { return m_broken; }
  const std::string& Origin() const { return m_origin; }
  const InfoMap& ExtraInfo() const { return m_extrainfo; }

  bool MeetsVersion(const AddonVersion& versionMin, const AddonVersion& version) const;
  uint64_t PackageSize() const { return m_packageSize; }
  CDateTime InstallDate() const { return m_installDate; }
  CDateTime LastUpdated() const { return m_lastUpdated; }
  CDateTime LastUsed() const { return m_lastUsed; }

  /*!
    * @brief Utilities to translate add-on parts to his requested part.
    */
  //@{
  static std::string TranslateType(TYPE type, bool pretty = false);
  static std::string TranslateIconType(TYPE type);
  static TYPE TranslateType(const std::string& string);
  static TYPE TranslateSubContent(const std::string& content);
  //@}

private:
  friend class CAddonInfoBuilder;

  std::string m_id;
  TYPE m_mainType = ADDON_UNKNOWN;
  std::vector<CAddonType> m_types;

  AddonVersion m_version;
  AddonVersion m_minversion;
  std::string m_name;
  std::string m_license;
  std::unordered_map<std::string, std::string> m_summary;
  std::unordered_map<std::string, std::string> m_description;
  std::string m_author;
  std::string m_source;
  std::string m_website;
  std::string m_forum;
  std::string m_email;
  std::string m_path;
  std::unordered_map<std::string, std::string> m_changelog;
  std::string m_icon;
  ArtMap m_art;
  std::vector<std::string> m_screenshots;
  std::unordered_map<std::string, std::string> m_disclaimer;
  std::vector<DependencyInfo> m_dependencies;
  std::string m_broken;
  CDateTime m_installDate;
  CDateTime m_lastUpdated;
  CDateTime m_lastUsed;
  std::string m_origin;
  uint64_t m_packageSize = 0;
  std::string m_libname;
  InfoMap m_extrainfo;
  std::vector<std::string> m_platforms;

  const std::string& GetTranslatedText(const std::unordered_map<std::string, std::string>& locales) const;
};

} /* namespace ADDON */
