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

#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace ADDON
{

enum class AddonType;

class CAddonBuilder;
class CAddonInfo;
class CAddonType;
typedef std::shared_ptr<CAddonInfo> AddonInfoPtr;
typedef std::vector<AddonInfoPtr> AddonInfos;

using AddonInstanceId = uint32_t;

/*!
 * Defines the default language code used as fallback in case the requested language is not
 * available. Used, for instance, to handle content from addon.xml.
 */
constexpr const char* KODI_ADDON_DEFAULT_LANGUAGE_CODE = "en_GB";

enum class AddonDisabledReason
{
  /// @brief Special reason for returning all disabled addons.
  ///
  /// Only used as an actual value when an addon is enabled.
  NONE = 0,
  USER = 1,
  INCOMPATIBLE = 2,
  PERMANENT_FAILURE = 3
};

enum class AddonOriginType
{
  /// @brief The type of the origin of an addon.
  ///
  /// Represents where an addon was installed from.
  SYSTEM = 0, /// The addon is a system addon
  REPOSITORY = 1, /// The addon origin is a repository
  MANUAL = 2 /// The addon origin is a zip file, package or development build
};

//! @brief Reasons why an addon is not updateable
enum class AddonUpdateRule
{
  ANY = 0, //!< used internally, not to be explicitly set
  USER_DISABLED_AUTO_UPDATE = 1, //!< automatic updates disabled via AddonInfo dialog
  PIN_OLD_VERSION = 2, //!< user downgraded to an older version
  PIN_ZIP_INSTALL = 3, //!< user installed manually from zip
};

/*!
 * @brief Independent add-on instance support.
 *
 * Used to be able to find out its instance path for the respective add-on types.
 */
enum class AddonInstanceSupport
{
  //! If add-on type does not support instances.
  SUPPORT_NONE = 0,

  //! If add-on type needs support for several instances individually.
  SUPPORT_MANDATORY = 1,

  //! If add-on type can support several instances individually.
  SUPPORT_OPTIONAL = 2,

  //! If add-on type supports multiple instances using independent settings.
  SUPPORT_SETTINGS = 3,
};

/*!
 * @brief Add-on state defined within addon.xml to report about the current addon
 * lifecycle state.
 *
 * E.g. the add-on is broken and can no longer be used.
 *
 * XML examples:
 * ~~~~~~~~~~~~~{.xml}
 * <lifecyclestate type="broken" lang="en_GB">SOME TEXT</lifecyclestate>
 * ~~~~~~~~~~~~~
 */
enum class AddonLifecycleState
{
  NORMAL = 0, //!< Used if an add-on has no special lifecycle state which is the default state
  DEPRECATED = 1, //!< the add-on should be marked as deprecated but is still usable
  BROKEN = 2, //!< the add-on should marked as broken in the repository
};

struct DependencyInfo
{
  std::string id;
  CAddonVersion versionMin, version;
  bool optional;
  DependencyInfo(std::string id,
                 const CAddonVersion& versionMin,
                 const CAddonVersion& version,
                 bool optional)
    : id(std::move(id)),
      versionMin(versionMin.empty() ? version : versionMin),
      version(version),
      optional(optional)
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
  CAddonInfo(std::string id, AddonType type);

  void SetMainType(AddonType type) { m_mainType = type; }
  void SetBinary(bool isBinary) { m_isBinary = isBinary; }
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
  AddonType MainType() const { return m_mainType; }

  /**
   * @brief To check addon contains a type
   *
   * @param[in] type The to checked type identifier
   * @param[in] mainOnly to check only in first defined main addon inside addon.xml
   * @return true in case the wanted type is supported, false if not
   */
  bool HasType(AddonType type, bool mainOnly = false) const;

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
  const CAddonType* Type(AddonType type) const;

  bool ProvidesSubContent(AddonType content, AddonType mainType) const;
  bool ProvidesSeveralSubContents() const;

  const CAddonVersion& Version() const { return m_version; }
  const CAddonVersion& MinVersion() const { return m_minversion; }
  bool IsBinary() const { return m_isBinary; }
  const CAddonVersion& DependencyMinVersion(const std::string& dependencyID) const;
  const CAddonVersion& DependencyVersion(const std::string& dependencyID) const;
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
  const std::string& ProfilePath() const { return m_profilePath; }
  const std::string& ChangeLog() const { return GetTranslatedText(m_changelog); }
  const std::string& Icon() const { return m_icon; }
  const ArtMap& Art() const { return m_art; }
  const std::vector<std::string>& Screenshots() const { return m_screenshots; }
  const std::string& Disclaimer() const { return GetTranslatedText(m_disclaimer); }
  const std::vector<DependencyInfo>& GetDependencies() const { return m_dependencies; }
  AddonLifecycleState LifecycleState() const { return m_lifecycleState; }
  const std::string& LifecycleStateDescription() const
  {
    return GetTranslatedText(m_lifecycleStateDescription);
  }
  const std::string& Origin() const { return m_origin; }
  const std::string& OriginName() const;

  const InfoMap& ExtraInfo() const { return m_extrainfo; }

  bool MeetsVersion(const CAddonVersion& versionMin, const CAddonVersion& version) const;
  uint64_t PackageSize() const { return m_packageSize; }
  CDateTime InstallDate() const { return m_installDate; }
  CDateTime LastUpdated() const { return m_lastUpdated; }
  CDateTime LastUsed() const { return m_lastUsed; }

  bool SupportsMultipleInstances() const;
  AddonInstanceSupport InstanceUseType() const { return m_addonInstanceSupportType; }

  bool SupportsAddonSettings() const { return m_supportsAddonSettings; }
  bool SupportsInstanceSettings() const { return m_supportsInstanceSettings; }
  std::vector<AddonInstanceId> GetKnownInstanceIds() const;

  /*!
    * @brief Utilities to translate add-on parts to his requested part.
    */
  //@{
  static std::string TranslateType(AddonType type, bool pretty = false);
  static std::string TranslateIconType(AddonType type);
  static AddonType TranslateType(const std::string& string);
  static AddonType TranslateSubContent(const std::string& content);
  static AddonInstanceSupport InstanceSupportType(AddonType type);
  //@}

private:
  friend class CAddonInfoBuilder;
  friend class CAddonInfoBuilderFromDB;

  std::string m_id;
  AddonType m_mainType{};
  std::vector<CAddonType> m_types;

  CAddonVersion m_version;
  CAddonVersion m_minversion;
  bool m_isBinary = false;
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
  std::string m_profilePath;
  std::unordered_map<std::string, std::string> m_changelog;
  std::string m_icon;
  ArtMap m_art;
  std::vector<std::string> m_screenshots;
  std::unordered_map<std::string, std::string> m_disclaimer;
  std::vector<DependencyInfo> m_dependencies;
  AddonLifecycleState m_lifecycleState = AddonLifecycleState::NORMAL;
  std::unordered_map<std::string, std::string> m_lifecycleStateDescription;
  CDateTime m_installDate;
  CDateTime m_lastUpdated;
  CDateTime m_lastUsed;
  std::string m_origin;
  mutable std::unique_ptr<std::string> m_originName; // @todo use std::optional once we use c++17
  uint64_t m_packageSize = 0;
  std::string m_libname;
  InfoMap m_extrainfo;
  std::vector<std::string> m_platforms;
  AddonInstanceSupport m_addonInstanceSupportType{AddonInstanceSupport::SUPPORT_NONE};
  bool m_supportsAddonSettings{false};
  bool m_supportsInstanceSettings{false};

  const std::string& GetTranslatedText(const std::unordered_map<std::string, std::string>& locales) const;
};

} /* namespace ADDON */
