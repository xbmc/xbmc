/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AddonInfoBuilder.h"

#include "CompileInfo.h"
#include "LangInfo.h"
#include "addons/Repository.h"
#include "addons/addoninfo/AddonInfo.h"
#include "addons/addoninfo/AddonType.h"
#include "filesystem/File.h"
#include "filesystem/SpecialProtocol.h"
#include "utils/JSONVariantParser.h"
#include "utils/JSONVariantWriter.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "utils/XBMCTinyXML.h"
#include "utils/log.h"

#include <algorithm>
#include <memory>
#include <regex>

namespace
{
// Note that all of these characters are url-safe
const std::string VALID_ADDON_IDENTIFIER_CHARACTERS = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.-_@!$";
}

namespace ADDON
{

CAddonInfoBuilderFromDB::CAddonInfoBuilderFromDB() : m_addonInfo(std::make_shared<CAddonInfo>())
{
}

void CAddonInfoBuilderFromDB::SetId(std::string id)
{
  m_addonInfo->m_id = std::move(id);
}

void CAddonInfoBuilderFromDB::SetName(std::string name)
{
  m_addonInfo->m_name = std::move(name);
}

void CAddonInfoBuilderFromDB::SetLicense(std::string license)
{
  m_addonInfo->m_license = std::move(license);
}

void CAddonInfoBuilderFromDB::SetSummary(std::string summary)
{
  m_addonInfo->m_summary.insert(std::pair<std::string, std::string>("unk", std::move(summary)));
}

void CAddonInfoBuilderFromDB::SetDescription(std::string description)
{
  m_addonInfo->m_description.insert(
      std::pair<std::string, std::string>("unk", std::move(description)));
}

void CAddonInfoBuilderFromDB::SetDisclaimer(std::string disclaimer)
{
  m_addonInfo->m_disclaimer.insert(
      std::pair<std::string, std::string>("unk", std::move(disclaimer)));
}

void CAddonInfoBuilderFromDB::SetAuthor(std::string author)
{
  m_addonInfo->m_author = std::move(author);
}

void CAddonInfoBuilderFromDB::SetSource(std::string source)
{
  m_addonInfo->m_source = std::move(source);
}

void CAddonInfoBuilderFromDB::SetWebsite(std::string website)
{
  m_addonInfo->m_website = std::move(website);
}

void CAddonInfoBuilderFromDB::SetForum(std::string forum)
{
  m_addonInfo->m_forum = std::move(forum);
}

void CAddonInfoBuilderFromDB::SetEMail(std::string email)
{
  m_addonInfo->m_email = std::move(email);
}

void CAddonInfoBuilderFromDB::SetIcon(std::string icon)
{
  m_addonInfo->m_icon = std::move(icon);
}

void CAddonInfoBuilderFromDB::SetArt(const std::string& type, std::string value)
{
  m_addonInfo->m_art[type] = std::move(value);
}

void CAddonInfoBuilderFromDB::SetArt(std::map<std::string, std::string> art)
{
  m_addonInfo->m_art = std::move(art);
}

void CAddonInfoBuilderFromDB::SetScreenshots(std::vector<std::string> screenshots)
{
  m_addonInfo->m_screenshots = std::move(screenshots);
}

void CAddonInfoBuilderFromDB::SetChangelog(std::string changelog)
{
  m_addonInfo->m_changelog.insert(std::pair<std::string, std::string>("unk", std::move(changelog)));
}

void CAddonInfoBuilderFromDB::SetLifecycleState(AddonLifecycleState state, std::string description)
{
  m_addonInfo->m_lifecycleState = state;
  m_addonInfo->m_lifecycleStateDescription.emplace("unk", std::move(description));
}

void CAddonInfoBuilderFromDB::SetPath(std::string path)
{
  m_addonInfo->m_path = std::move(path);
}

void CAddonInfoBuilderFromDB::SetLibName(std::string libname)
{
  m_addonInfo->m_libname = std::move(libname);
}

void CAddonInfoBuilderFromDB::SetVersion(CAddonVersion version)
{
  m_addonInfo->m_version = std::move(version);
}

void CAddonInfoBuilderFromDB::SetDependencies(std::vector<DependencyInfo> dependencies)
{
  m_addonInfo->m_dependencies = std::move(dependencies);
}

void CAddonInfoBuilderFromDB::SetExtrainfo(InfoMap extrainfo)
{
  m_addonInfo->m_extrainfo = std::move(extrainfo);
}

void CAddonInfoBuilderFromDB::SetInstallDate(const CDateTime& installDate)
{
  m_addonInfo->m_installDate = installDate;
}

void CAddonInfoBuilderFromDB::SetLastUpdated(const CDateTime& lastUpdated)
{
  m_addonInfo->m_lastUpdated = lastUpdated;
}

void CAddonInfoBuilderFromDB::SetLastUsed(const CDateTime& lastUsed)
{
  m_addonInfo->m_lastUsed = lastUsed;
}

void CAddonInfoBuilderFromDB::SetOrigin(std::string origin)
{
  m_addonInfo->m_origin = std::move(origin);
}

void CAddonInfoBuilderFromDB::SetPackageSize(uint64_t size)
{
  m_addonInfo->m_packageSize = size;
}

void CAddonInfoBuilderFromDB::SetExtensions(CAddonType addonType)
{
  if (!addonType.GetValue("provides").empty())
    addonType.SetProvides(addonType.GetValue("provides").asString());

  m_addonInfo->m_types.push_back(std::move(addonType));
  m_addonInfo->m_mainType = addonType.m_type;
}

AddonInfoPtr CAddonInfoBuilder::Generate(const std::string& id, AddonType type)
{
  // Check addon identifier for forbidden characters
  // The identifier is used e.g. in URLs so we shouldn't allow just
  // any character to go through.
  if (id.empty() || id.find_first_not_of(VALID_ADDON_IDENTIFIER_CHARACTERS) != std::string::npos)
  {
    CLog::Log(LOGERROR, "CAddonInfoBuilder::{}: identifier '{}' is invalid", __FUNCTION__, id);
    return nullptr;
  }

  AddonInfoPtr addon = std::make_shared<CAddonInfo>();
  addon->m_id = id;
  addon->m_mainType = type;
  return addon;
}

AddonInfoPtr CAddonInfoBuilder::Generate(const std::string& addonPath, bool platformCheck /*= true*/)
{
  auto addonRealPath = CSpecialProtocol::TranslatePath(addonPath);

  CXBMCTinyXML xmlDoc;
  if (!xmlDoc.LoadFile(URIUtils::AddFileToFolder(addonRealPath, "addon.xml")))
  {
    CLog::Log(LOGERROR, "CAddonInfoBuilder::{}: Unable to load '{}', Line {}\n{}",
                                               __FUNCTION__,
                                               URIUtils::AddFileToFolder(addonRealPath, "addon.xml"),
                                               xmlDoc.ErrorRow(),
                                               xmlDoc.ErrorDesc());
    return nullptr;
  }

  AddonInfoPtr addon = std::make_shared<CAddonInfo>();
  if (!ParseXML(addon, xmlDoc.RootElement(), addonRealPath))
    return nullptr;

  if (!platformCheck || PlatformSupportsAddon(addon))
    return addon;

  return nullptr;
}

AddonInfoPtr CAddonInfoBuilder::Generate(const TiXmlElement* baseElement,
                                         const RepositoryDirInfo& repo,
                                         bool platformCheck /*= true*/)
{
  AddonInfoPtr addon = std::make_shared<CAddonInfo>();
  if (!ParseXML(addon, baseElement, repo.datadir, repo))
    return nullptr;

  if (!platformCheck || PlatformSupportsAddon(addon))
    return addon;

  return nullptr;
}

void CAddonInfoBuilder::SetInstallData(const AddonInfoPtr& addon, const CDateTime& installDate, const CDateTime& lastUpdated,
                                       const CDateTime& lastUsed, const std::string& origin)
{
  if (!addon)
    return;

  addon->m_installDate = installDate;
  addon->m_lastUpdated = lastUpdated;
  addon->m_lastUsed = lastUsed;
  addon->m_origin = origin;
}

bool CAddonInfoBuilder::ParseXML(const AddonInfoPtr& addon,
                                 const TiXmlElement* element,
                                 const std::string& addonPath)
{
  return ParseXML(addon, element, addonPath, {});
}

bool CAddonInfoBuilder::ParseXML(const AddonInfoPtr& addon,
                                 const TiXmlElement* element,
                                 const std::string& addonPath,
                                 const RepositoryDirInfo& repo)
{
  /*
   * Following values currently not set from creator:
   * - CDateTime installDate;
   * - CDateTime lastUpdated;
   * - CDateTime lastUsed;
   * - std::string origin;
   */

  if (!StringUtils::EqualsNoCase(element->Value(), "addon"))
  {
    CLog::Log(LOGERROR, "CAddonInfoBuilder::{}: file from '{}' doesn't contain <addon>", __FUNCTION__, addonPath);
    return false;
  }

  /*
   * The function variable "repo" is only used when reading data stored on the internet.
   * A boolean value is then set here for easier identification.
   */
  const bool isRepoXMLContent = !repo.datadir.empty();

  /*
   * Parse addon.xml:
   * <addon id="???"
   *        name="???"
   *        version="???"
   *        provider-name="???">
   */
  addon->m_id = StringUtils::CreateFromCString(element->Attribute("id"));
  addon->m_name = StringUtils::CreateFromCString(element->Attribute("name"));
  addon->m_author = StringUtils::CreateFromCString(element->Attribute("provider-name"));

  const std::string version = StringUtils::CreateFromCString(element->Attribute("version"));
  addon->m_version = CAddonVersion(version);

  if (addon->m_id.empty() || addon->m_version.empty())
  {
    CLog::Log(LOGERROR, "CAddonInfoBuilder::{}: file '{}' doesn't contain required values on <addon ... > id='{}', version='{}'",
              __FUNCTION__,
              addonPath,
              addon->m_id.empty() ? "missing" : addon->m_id,
              addon->m_version.empty() ? "missing" : addon->m_version.asString());
    return false;
  }

  // Check addon identifier for forbidden characters
  // The identifier is used e.g. in URLs so we shouldn't allow just
  // any character to go through.
  if (addon->m_id.find_first_not_of(VALID_ADDON_IDENTIFIER_CHARACTERS) != std::string::npos)
  {
    CLog::Log(LOGERROR, "CAddonInfoBuilder::{}: identifier {} is invalid", __FUNCTION__, addon->m_id);
    return false;
  }

  /*
   * Parse addon.xml:
   * <backwards-compatibility abi="???"/>
   */
  const TiXmlElement* backwards = element->FirstChildElement("backwards-compatibility");
  if (backwards)
  {
    const std::string minVersion = StringUtils::CreateFromCString(backwards->Attribute("abi"));
    addon->m_minversion = CAddonVersion(minVersion);
  }

  /*
   * Parse addon.xml:
   * <requires>
   *   <import addon="???" minversion="???" version="???" optional="???"/>
   * </requires>
   */
  const TiXmlElement* _requires = element->FirstChildElement("requires");
  if (_requires)
  {
    for (const TiXmlElement* child = _requires->FirstChildElement("import"); child != nullptr;
         child = child->NextSiblingElement("import"))
    {
      if (child->Attribute("addon"))
      {
        const std::string minVersion =
            StringUtils::CreateFromCString(child->Attribute("minversion"));
        const std::string version = StringUtils::CreateFromCString(child->Attribute("version"));

        bool optional = false;
        child->QueryBoolAttribute("optional", &optional);

        addon->m_dependencies.emplace_back(child->Attribute("addon"), CAddonVersion(minVersion),
                                           CAddonVersion(version), optional);
      }
    }
  }

  std::string assetBasePath;
  if (!isRepoXMLContent && !addonPath.empty())
  {
    // Default for add-on information not loaded from repository
    assetBasePath = addonPath;
    addon->m_path = addonPath;
  }
  else
  {
    assetBasePath = URIUtils::AddFileToFolder(repo.artdir, addon->m_id);
    addon->m_path = URIUtils::AddFileToFolder(repo.datadir, addon->m_id, StringUtils::Format("{}-{}.zip", addon->m_id, addon->m_version.asString()));
  }

  addon->m_profilePath = StringUtils::Format("special://profile/addon_data/{}/", addon->m_id);

  /*
   * Parse addon.xml:
   * <extension>
   *   ...
   * </extension>
   */
  for (const TiXmlElement* child = element->FirstChildElement("extension"); child != nullptr; child = child->NextSiblingElement("extension"))
  {
    const std::string point = StringUtils::CreateFromCString(child->Attribute("point"));

    if (point == "kodi.addon.metadata" || point == "xbmc.addon.metadata")
    {
      /*
       * Parse addon.xml "<path">...</path>" (special related to repository path),
       * do first and if present override the default. Also set assetBasePath to
       * find screenshots and icons.
       */
      element = child->FirstChildElement("path");
      if (element && element->GetText() != nullptr && !repo.datadir.empty())
      {
        addon->m_path = URIUtils::AddFileToFolder(repo.datadir, element->GetText());
        assetBasePath = URIUtils::GetDirectory(URIUtils::AddFileToFolder(repo.artdir, element->GetText()));
      }

      /*
       * Parse addon.xml "<summary lang="..">...</summary>"
       */
      GetTextList(child, "summary", addon->m_summary);

      /*
       * Parse addon.xml "<description lang="..">...</description>"
       */
      GetTextList(child, "description", addon->m_description);

      /*
       * Parse addon.xml "<disclaimer lang="..">...</disclaimer>"
       */
      GetTextList(child, "disclaimer", addon->m_disclaimer);

      /*
       * Parse addon.xml "<assets>...</assets>"
       */
      const TiXmlElement* element = child->FirstChildElement("assets");
      if (element)
      {
        for (const TiXmlElement* elementsAssets = element->FirstChildElement(); elementsAssets != nullptr; elementsAssets = elementsAssets->NextSiblingElement())
        {
          std::string value = elementsAssets->Value();
          if (value == "icon")
          {
            if (elementsAssets->GetText() != nullptr)
              addon->m_icon = URIUtils::AddFileToFolder(assetBasePath, elementsAssets->GetText());
          }
          else if (value == "screenshot")
          {
            if (elementsAssets->GetText() != nullptr)
              addon->m_screenshots.emplace_back(URIUtils::AddFileToFolder(assetBasePath, elementsAssets->GetText()));
          }
          else if (value == "fanart")
          {
            if (elementsAssets->GetText() != nullptr)
              addon->m_art[value] = URIUtils::AddFileToFolder(assetBasePath, elementsAssets->GetText());
          }
          else if (value == "banner")
          {
            if (elementsAssets->GetText() != nullptr)
              addon->m_art[value] = URIUtils::AddFileToFolder(assetBasePath, elementsAssets->GetText());
          }
          else if (value == "clearlogo")
          {
            if (elementsAssets->GetText() != nullptr)
              addon->m_art[value] = URIUtils::AddFileToFolder(assetBasePath, elementsAssets->GetText());
          }
          else if (value == "thumb")
          {
            if (elementsAssets->GetText() != nullptr)
              addon->m_art[value] =
                  URIUtils::AddFileToFolder(assetBasePath, elementsAssets->GetText());
          }
        }
      }

      /* Parse addon.xml "<platform">...</platform>" */
      element = child->FirstChildElement("platform");
      if (element && element->GetText() != nullptr)
      {
        auto platforms = StringUtils::Split(element->GetText(),
                                            {" ", "\t", "\n", "\r"});
        platforms.erase(std::remove_if(platforms.begin(), platforms.end(),
                        [](const std::string& platform) { return platform.empty(); }),
                        platforms.cend());
        addon->m_platforms = platforms;
      }

      /* Parse addon.xml "<license">...</license>" */
      element = child->FirstChildElement("license");
      if (element && element->GetText() != nullptr)
        addon->m_license = element->GetText();

      /* Parse addon.xml "<source">...</source>" */
      element = child->FirstChildElement("source");
      if (element && element->GetText() != nullptr)
        addon->m_source = element->GetText();

      /* Parse addon.xml "<email">...</email>" */
      element = child->FirstChildElement("email");
      if (element && element->GetText() != nullptr)
        addon->m_email = element->GetText();

      /* Parse addon.xml "<website">...</website>" */
      element = child->FirstChildElement("website");
      if (element && element->GetText() != nullptr)
        addon->m_website = element->GetText();

      /* Parse addon.xml "<forum">...</forum>" */
      element = child->FirstChildElement("forum");
      if (element && element->GetText() != nullptr)
        addon->m_forum = element->GetText();

      /* Parse addon.xml "<broken">...</broken>"
       * NOTE: Replaced with <lifecyclestate>, available for backward compatibility */
      element = child->FirstChildElement("broken");
      if (element && element->GetText() != nullptr)
      {
        addon->m_lifecycleState = AddonLifecycleState::BROKEN;
        addon->m_lifecycleStateDescription.emplace(KODI_ADDON_DEFAULT_LANGUAGE_CODE,
                                                   element->GetText());
      }

      /* Parse addon.xml "<lifecyclestate">...</lifecyclestate>" */
      element = child->FirstChildElement("lifecyclestate");
      if (element && element->GetText() != nullptr)
      {
        const char* lang = element->Attribute("type");
        if (lang)
        {
          if (strcmp(lang, "broken") == 0)
            addon->m_lifecycleState = AddonLifecycleState::BROKEN;
          else if (strcmp(lang, "deprecated") == 0)
            addon->m_lifecycleState = AddonLifecycleState::DEPRECATED;
          else
            addon->m_lifecycleState = AddonLifecycleState::NORMAL;

          GetTextList(child, "lifecyclestate", addon->m_lifecycleStateDescription);
        }
      }

      /* Parse addon.xml "<language">...</language>" */
      element = child->FirstChildElement("language");
      if (element && element->GetText() != nullptr)
        addon->AddExtraInfo("language", element->GetText());

      /* Parse addon.xml "<reuselanguageinvoker">...</reuselanguageinvoker>" */
      element = child->FirstChildElement("reuselanguageinvoker");
      if (element && element->GetText() != nullptr)
        addon->AddExtraInfo("reuselanguageinvoker", element->GetText());

      /* Parse addon.xml "<size">...</size>" */
      element = child->FirstChildElement("size");
      if (element && element->GetText() != nullptr)
        addon->m_packageSize = StringUtils::ToUint64(element->GetText(), 0);

      /* Parse addon.xml "<news lang="..">...</news>"
       *
       * In the event that the changelog (news) in addon.xml is empty, check
       * whether it is an installed addon and read a changelog.txt as a
       * replacement, if available. */
      GetTextList(child, "news", addon->m_changelog);
      if (addon->m_changelog.empty() && !isRepoXMLContent && !addonPath.empty())
      {
        using XFILE::CFile;

        const std::string changelog = URIUtils::AddFileToFolder(addonPath, "changelog.txt");
        if (CFile::Exists(changelog))
        {
          CFile file;
          std::vector<uint8_t> buf;
          if (file.LoadFile(changelog, buf) > 0)
            addon->m_changelog[KODI_ADDON_DEFAULT_LANGUAGE_CODE].assign(
                reinterpret_cast<char*>(buf.data()), buf.size());
        }
      }
    }
    else
    {
      AddonType type = CAddonInfo::TranslateType(point);
      if (type == AddonType::UNKNOWN || type >= AddonType::MAX_TYPES)
      {
        CLog::Log(LOGERROR, "CAddonInfoBuilder::{}: file '{}' doesn't contain a valid add-on type name ({})", __FUNCTION__, addon->m_path, point);
        return false;
      }

      CAddonType addonType(type);
      if (ParseXMLTypes(addonType, addon, child))
        addon->m_types.emplace_back(std::move(addonType));
    }
  }

  /*
   * If nothing is defined in addon.xml set addon as unknown to have minimum one
   * instance type present.
   */
  if (addon->m_types.empty())
  {
    CAddonType addonType(AddonType::UNKNOWN);
    addon->m_types.emplace_back(std::move(addonType));
  }

  addon->m_mainType = addon->m_types[0].Type();
  addon->m_libname = addon->m_types[0].m_libname;
  if (!addon->m_types[0].GetValue("provides").empty())
    addon->AddExtraInfo("provides", addon->m_types[0].GetValue("provides").asString());

  // Ensure binary types have a valid library for the platform
  if (addon->m_mainType == AddonType::VISUALIZATION ||
      addon->m_mainType == AddonType::SCREENSAVER || addon->m_mainType == AddonType::PVRDLL ||
      addon->m_mainType == AddonType::AUDIOENCODER ||
      addon->m_mainType == AddonType::AUDIODECODER || addon->m_mainType == AddonType::VFS ||
      addon->m_mainType == AddonType::IMAGEDECODER || addon->m_mainType == AddonType::INPUTSTREAM ||
      addon->m_mainType == AddonType::PERIPHERALDLL || addon->m_mainType == AddonType::GAMEDLL)
  {
    if (addon->m_libname.empty())
    {
      // Prevent log file entry if data is from repository, there normal on
      // addons for other OS's
      if (!isRepoXMLContent)
        CLog::Log(LOGERROR, "CAddonInfoBuilder::{}: addon.xml from '{}' for binary type '{}' doesn't contain library and addon becomes ignored",
                      __FUNCTION__, addon->ID(), CAddonInfo::TranslateType(addon->m_mainType));
      return false;
    }
  }

  if (!isRepoXMLContent)
  {
    using XFILE::CFile;
    if (CFile::Exists(URIUtils::AddFileToFolder(addonPath, "resources", "settings.xml")))
      addon->m_supportsAddonSettings = true;
    if (CFile::Exists(URIUtils::AddFileToFolder(addonPath, "resources", "instance-settings.xml")))
      addon->m_supportsInstanceSettings = true;
  }

  addon->m_addonInstanceSupportType = CAddonInfo::InstanceSupportType(addon->m_mainType);

  return true;
}

bool CAddonInfoBuilder::ParseXMLTypes(CAddonType& addonType,
                                      const AddonInfoPtr& info,
                                      const TiXmlElement* child)
{
  if (child)
  {
    addonType.m_path = info->Path();

    // Get add-on library file name (if present)
    const char* library = child->Attribute("library");
    if (library == nullptr)
      library = GetPlatformLibraryName(child);
    if (library != nullptr)
    {
      addonType.m_libname = library;

      try
      {
        // linux is different and has the version number after the suffix
        static const std::regex libRegex("^.*" +
                                        CCompileInfo::CCompileInfo::GetSharedLibrarySuffix() +
                                        "\\.?[0-9]*\\.?[0-9]*\\.?[0-9]*$");
        if (std::regex_match(library, libRegex))
        {
          info->SetBinary(true);
          CLog::Log(LOGDEBUG, "CAddonInfoBuilder::{}: Binary addon found: {}", __func__,
                    info->ID());
        }
      }
      catch (const std::regex_error& e)
      {
        CLog::Log(LOGERROR, "CAddonInfoBuilder::{}: Regex error caught: {}", __func__,
                  e.what());
      }
    }

    if (!ParseXMLExtension(addonType, child))
    {
      CLog::Log(LOGERROR, "CAddonInfoBuilder::{}: addon.xml file doesn't contain a valid add-on extensions ({})", __FUNCTION__, info->ID());
      return false;
    }
    if (!addonType.GetValue("provides").empty())
      addonType.SetProvides(addonType.GetValue("provides").asString());
    return true;
  }
  return false;
}

bool CAddonInfoBuilder::ParseXMLExtension(CAddonExtensions& addonExt, const TiXmlElement* element)
{
  addonExt.m_point = StringUtils::CreateFromCString(element->Attribute("point"));

  EXT_VALUE extension;
  const TiXmlAttribute* attribute = element->FirstAttribute();
  while (attribute)
  {
    std::string name = attribute->Name();
    if (name != "point")
    {
      const std::string value = StringUtils::CreateFromCString(attribute->Value());
      if (!value.empty())
      {
        name = "@" + name;
        extension.emplace_back(std::make_pair(name, SExtValue(value)));
      }
    }
    attribute = attribute->Next();
  }
  if (!extension.empty())
    addonExt.m_values.emplace_back(std::pair<std::string, EXT_VALUE>("", std::move(extension)));

  const TiXmlElement* childElement = element->FirstChildElement();
  while (childElement)
  {
    const std::string id = StringUtils::CreateFromCString(childElement->Value());
    if (!id.empty())
    {
      EXT_VALUE extension;
      const TiXmlAttribute* attribute = childElement->FirstAttribute();
      while (attribute)
      {
        std::string name = attribute->Name();
        if (name != "point")
        {
          const std::string value = StringUtils::CreateFromCString(attribute->Value());
          if (!value.empty())
          {
            name = id + "@" + name;
            extension.emplace_back(std::make_pair(name, SExtValue(value)));
          }
        }
        attribute = attribute->Next();
      }

      const std::string childElementText = StringUtils::CreateFromCString(childElement->GetText());

      if (!childElementText.empty())
      {
        extension.emplace_back(std::make_pair(id, SExtValue(childElementText)));
      }

      if (!extension.empty())
        addonExt.m_values.emplace_back(std::make_pair(id, std::move(extension)));

      if (childElementText.empty())
      {
        const TiXmlElement* childSubElement = childElement->FirstChildElement();
        if (childSubElement)
        {
          CAddonExtensions subElement;
          if (ParseXMLExtension(subElement, childElement))
            addonExt.m_children.emplace_back(std::make_pair(id, std::move(subElement)));
        }
      }
    }
    childElement = childElement->NextSiblingElement();
  }

  return true;
}

bool CAddonInfoBuilder::GetTextList(const TiXmlElement* element, const std::string& tag, std::unordered_map<std::string, std::string>& translatedValues)
{
  if (!element)
    return false;

  translatedValues.clear();

  for (const TiXmlElement* child = element->FirstChildElement(tag); child != nullptr; child = child->NextSiblingElement(tag))
  {
    const char* lang = child->Attribute("lang");
    const char* text = child->GetText();
    if (lang != nullptr)
    {
      if (strcmp(lang, "no") == 0)
        translatedValues.insert(std::make_pair("nb_NO", text != nullptr ? text : ""));
      else
        translatedValues.insert(std::make_pair(lang, text != nullptr ? text : ""));
    }
    else
      translatedValues.insert(
          std::make_pair(KODI_ADDON_DEFAULT_LANGUAGE_CODE, text != nullptr ? text : ""));
  }

  return !translatedValues.empty();
}

const char* CAddonInfoBuilder::GetPlatformLibraryName(const TiXmlElement* element)
{
  const char* libraryName;
#if defined(TARGET_ANDROID)
  libraryName = element->Attribute("library_android");
#elif defined(TARGET_LINUX) || defined(TARGET_FREEBSD)
#if defined(TARGET_FREEBSD)
  libraryName = element->Attribute("library_freebsd");
  if (libraryName == nullptr)
#endif
  libraryName = element->Attribute("library_linux");
#elif defined(TARGET_WINDOWS_DESKTOP)
  libraryName = element->Attribute("library_windx");
  if (libraryName == nullptr)
    libraryName = element->Attribute("library_windows");
#elif defined(TARGET_WINDOWS_STORE)
  libraryName = element->Attribute("library_windowsstore");
#elif defined(TARGET_DARWIN)
#if defined(TARGET_DARWIN_EMBEDDED)
  libraryName = element->Attribute("library_darwin_embedded");
#else
  libraryName = element->Attribute("library_osx");
#endif
#endif

  return libraryName;
}

bool CAddonInfoBuilder::PlatformSupportsAddon(const AddonInfoPtr& addon)
{
  if (addon->m_platforms.empty())
    return true;

  std::vector<std::string> supportedPlatforms = {
    "all",
#if defined(TARGET_ANDROID)
    "android",
#if defined(__ARM_ARCH_7A__)
    "android-armv7",
#elif defined(__aarch64__)
    "android-aarch64",
#elif defined(__i686__)
    "android-i686",
#elif defined(__x86_64__)
    "android-x86_64",
#else
    #warning no architecture dependant platform tag
#endif
#elif defined(TARGET_FREEBSD)
    "freebsd",
#elif defined(TARGET_LINUX)
    "linux",
#if defined(__ARM_ARCH_7A__)
    "linux-armv7",
#elif defined(__aarch64__)
    "linux-aarch64",
#elif defined(__i686__)
    "linux-i686",
#elif defined(__x86_64__)
    "linux-x86_64",
#else
    #warning no architecture dependant platform tag
#endif
#elif defined(TARGET_WINDOWS_DESKTOP)
    "windx",
    "windows",
#if defined(_M_IX86)
    "windows-i686",
#elif defined(_M_AMD64)
    "windows-x86_64",
#else
#error no architecture dependant platform tag
#endif
#elif defined(TARGET_WINDOWS_STORE)
    "windowsstore",
#elif defined(TARGET_DARWIN_EMBEDDED)
    "darwin_embedded",
#if defined(TARGET_DARWIN_IOS)
    "ios",
#if defined(__aarch64__)
    "ios-aarch64",
#else
#warning no architecture dependant platform tag
#endif
#elif defined(TARGET_DARWIN_TVOS)
    "tvos",
    "tvos-aarch64",
#endif
#elif defined(TARGET_DARWIN_OSX)
    "osx",
#if defined(__x86_64__)
    "osx64",
    "osx-x86_64",
#elif defined(__aarch64__)
    "osxarm64",
    "osx-arm64",
#else
#warning no architecture dependant platform tag
#endif
#endif
  };

  return std::find_first_of(addon->m_platforms.begin(), addon->m_platforms.end(),
      supportedPlatforms.begin(), supportedPlatforms.end()) != addon->m_platforms.end();
}

}
