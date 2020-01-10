/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AddonInfoBuilder.h"

#include "LangInfo.h"
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

namespace
{
// Note that all of these characters are url-safe
const std::string VALID_ADDON_IDENTIFIER_CHARACTERS = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.-_@!$";
}

namespace ADDON
{

AddonInfoPtr CAddonInfoBuilder::Generate(const std::string& id, TYPE type)
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
  addon->m_id = std::move(id);
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

AddonInfoPtr CAddonInfoBuilder::Generate(const TiXmlElement* baseElement, const CRepository::DirInfo& repo, bool platformCheck /*= true*/)
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

bool CAddonInfoBuilder::ParseXML(const AddonInfoPtr& addon, const TiXmlElement* element, const std::string& addonPath, const CRepository::DirInfo& repo)
{
  /*
   * Following values currently not set from creator:
   * - CDateTime installDate;
   * - CDateTime lastUpdated;
   * - CDateTime lastUsed;
   * - std::string origin;
   */
  const char* cstring; /* "C" string point where parts from TinyXML becomes
                          stored, is used as this to prevent double use of
                          calls and to prevent not wanted "C++" throws if
                          std::string want to become set with nullptr! */

  if (!StringUtils::EqualsNoCase(element->Value(), "addon"))
  {
    CLog::Log(LOGERROR, "CAddonInfoBuilder::{}: file from '{}' doesnt contain <addon>", __FUNCTION__, addonPath);
    return false;
  }

  /*
   * Parse addon.xml:
   * <addon id="???"
   *        name="???"
   *        version="???"
   *        provider-name="???">
   */
  cstring = element->Attribute("id");
  addon->m_id = cstring ? cstring : "";
  cstring = element->Attribute("name");
  addon->m_name = cstring ? cstring : "";
  cstring = element->Attribute("version");
  addon->m_version = AddonVersion(cstring ? cstring : "");
  cstring = element->Attribute("provider-name");
  addon->m_author = cstring ? cstring : "";
  if (addon->m_id.empty() || addon->m_version.empty())
  {
    CLog::Log(LOGERROR, "CAddonInfoBuilder::{}: file '{}' doesnt contain required values on <addon ... > id='{}', version='{}'",
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
    cstring = backwards->Attribute("abi");
    addon->m_minversion = AddonVersion(cstring ? cstring : "");
  }

  /*
   * Parse addon.xml:
   * <requires>
   *   <import addon="???" minversion="???" version="???" optional="???"/>
   * </requires>
   */
  const TiXmlElement* requires = element->FirstChildElement("requires");
  if (requires)
  {
    for (const TiXmlElement* child = requires->FirstChildElement("import"); child != nullptr; child = child->NextSiblingElement("import"))
    {
      cstring = child->Attribute("addon");
      if (cstring)
      {
        const char* versionMin = child->Attribute("minversion");
        const char* version = child->Attribute("version");
        bool optional = false;
        child->QueryBoolAttribute("optional", &optional);

        addon->m_dependencies.emplace_back(cstring, AddonVersion(versionMin), AddonVersion(version),
                                           optional);
      }
    }
  }

  std::string assetBasePath;
  if (repo.artdir.empty() && !addonPath.empty())
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

  /*
   * Parse addon.xml:
   * <extension>
   *   ...
   * </extension>
   */
  for (const TiXmlElement* child = element->FirstChildElement("extension"); child != nullptr; child = child->NextSiblingElement("extension"))
  {
    cstring = child->Attribute("point");
    std::string point = cstring ? cstring : "";

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
        }
      }

      /* Parse addon.xml "<platform">...</platform>" */
      element = child->FirstChildElement("platform");
      if (element && element->GetText() != nullptr)
        addon->m_platforms = StringUtils::Split(element->GetText(), " ");

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

      /* Parse addon.xml "<broken">...</broken>" */
      element = child->FirstChildElement("broken");
      if (element && element->GetText() != nullptr)
        addon->m_broken = element->GetText();

      /* Parse addon.xml "<language">...</language>" */
      element = child->FirstChildElement("language");
      if (element && element->GetText() != nullptr)
        addon->AddExtraInfo("language", element->GetText());

      /* Parse addon.xml "<reuselanguageinvoker">...</reuselanguageinvoker>" */
      element = child->FirstChildElement("reuselanguageinvoker");
      if (element && element->GetText() != nullptr)
        addon->AddExtraInfo("reuselanguageinvoker", element->GetText());

      /* Parse addon.xml "<noicon">...</noicon>" */
      if (addon->m_icon.empty())
      {
        element = child->FirstChildElement("noicon");
        addon->m_icon = (element && strcmp(element->GetText() , "true") == 0) ? "" : URIUtils::AddFileToFolder(assetBasePath, "icon.png");
      }

      /* Parse addon.xml "<nofanart">...</nofanart>" */
      if (addon->m_art.empty())
      {
        element = child->FirstChildElement("nofanart");
        if (!element || strcmp(element->GetText(), "true") != 0)
          addon->m_art["fanart"] = URIUtils::AddFileToFolder(assetBasePath, "fanart.jpg");
      }

      /* Parse addon.xml "<size">...</size>" */
      element = child->FirstChildElement("size");
      if (element && element->GetText() != nullptr)
        addon->m_packageSize = StringUtils::ToUint64(element->GetText(), 0);

      /* Parse addon.xml "<news lang="..">...</news>" */
      GetTextList(child, "news", addon->m_changelog);
    }
    else
    {
      TYPE type = CAddonInfo::TranslateType(point);
      if (type == ADDON_UNKNOWN || type >= ADDON_MAX)
      {
        CLog::Log(LOGERROR, "CAddonInfoBuilder::{}: file '{}' doesn't contain a valid add-on type name ({})", __FUNCTION__, addon->m_path, point);
        return false;
      }

      CAddonType addonType(type);
      if (ParseXMLTypes(addonType, addon, child))
        addon->m_types.push_back(std::move(addonType));
    }
  }

  /*
   * If nothing is defined in addon.xml set addon as unknown to have minimum one
   * instance type present.
   */
  if (addon->m_types.empty())
  {
    CAddonType addonType(ADDON_UNKNOWN);
    addon->m_types.push_back(std::move(addonType));
  }

  addon->m_mainType = addon->m_types[0].Type();
  addon->m_libname = addon->m_types[0].m_libname;
  if (!addon->m_types[0].GetValue("provides").empty())
    addon->AddExtraInfo("provides", addon->m_types[0].GetValue("provides").asString());

  // Ensure binary types have a valid library for the platform
  if (addon->m_mainType == ADDON_VIZ ||
      addon->m_mainType == ADDON_SCREENSAVER ||
      addon->m_mainType == ADDON_PVRDLL ||
      addon->m_mainType == ADDON_AUDIOENCODER ||
      addon->m_mainType == ADDON_AUDIODECODER ||
      addon->m_mainType == ADDON_VFS ||
      addon->m_mainType == ADDON_IMAGEDECODER ||
      addon->m_mainType == ADDON_INPUTSTREAM ||
      addon->m_mainType == ADDON_PERIPHERALDLL ||
      addon->m_mainType == ADDON_GAMEDLL)
  {
    if (addon->m_libname.empty())
    {
      // Prevent log file entry if data is from repository, there normal on
      // addons for other OS's
      if (repo.datadir.empty())
        CLog::Log(LOGERROR, "CAddonInfoBuilder::{}: addon.xml from '{}' for binary type '{}' doesn't contain library and addon becomes ignored",
                      __FUNCTION__, addon->ID(), CAddonInfo::TranslateType(addon->m_mainType));
      return false;
    }
  }

  return true;
}

bool CAddonInfoBuilder::ParseXMLTypes(CAddonType& addonType, AddonInfoPtr info, const TiXmlElement* child)
{
  if (child)
  {
    addonType.m_path = info->Path();

    // Get add-on library file name (if present)
    const char* library = child->Attribute("library");
    if (library == nullptr)
      library = GetPlatformLibraryName(child);
    if (library != nullptr)
      addonType.m_libname = library;

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
  const char* cstring; /* "C" string point where parts from TinyXML becomes
                          stored, is used as this to prevent double use of
                          calls and to prevent not wanted "C++" throws if
                          std::string want to become set with nullptr! */

  cstring = element->Attribute("point");
  addonExt.m_point = cstring ? cstring : "";

  EXT_VALUE extension;
  const TiXmlAttribute* attribute = element->FirstAttribute();
  while (attribute)
  {
    std::string name = attribute->Name();
    if (name != "point")
    {
      cstring = attribute->Value();
      if (cstring)
      {
        std::string value = cstring;
        name = "@" + name;
        extension.push_back(std::make_pair(name, SExtValue(value)));
      }
    }
    attribute = attribute->Next();
  }
  if (!extension.empty())
    addonExt.m_values.push_back(std::pair<std::string, EXT_VALUE>("", std::move(extension)));

  const TiXmlElement* childElement = element->FirstChildElement();
  while (childElement)
  {
    cstring = childElement->Value();
    if (cstring)
    {
      std::string id = cstring;

      EXT_VALUE extension;
      const TiXmlAttribute* attribute = childElement->FirstAttribute();
      while (attribute)
      {
        std::string name = attribute->Name();
        if (name != "point")
        {
          cstring = attribute->Value();
          if (cstring)
          {
            std::string value = cstring;
            name = id + "@" + name;
            extension.push_back(std::make_pair(name, SExtValue(value)));
          }
        }
        attribute = attribute->Next();
      }

      cstring = childElement->GetText();
      if (cstring)
      {
        extension.push_back(std::make_pair(id, SExtValue(cstring)));
      }

      if (!extension.empty())
        addonExt.m_values.push_back(std::make_pair(id, std::move(extension)));

      if (!cstring)
      {
        const TiXmlElement* childSubElement = childElement->FirstChildElement();
        if (childSubElement)
        {
          CAddonExtensions subElement;
          if (ParseXMLExtension(subElement, childElement))
            addonExt.m_children.push_back(std::make_pair(id, std::move(subElement)));
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
      translatedValues.insert(std::make_pair("en_GB", text != nullptr ? text : ""));
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
#elif defined(TARGET_RASPBERRY_PI)
  libraryName = element->Attribute("library_rbpi");
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
#else
    #warning no architecture dependant platform tag
#endif
#elif defined(TARGET_FREEBSD)
    "freebsd",
    "linux",
#elif defined(TARGET_LINUX)
    "linux",
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
#if defined(__ARM_ARCH_7A__)
    "ios-armv7",
#elif defined(__aarch64__)
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
#elif defined(__i686__)
    "osx-i686",
    "osx32",
#else
#warning no architecture dependant platform tag
#endif
#endif
  };

  return std::find_first_of(addon->m_platforms.begin(), addon->m_platforms.end(),
      supportedPlatforms.begin(), supportedPlatforms.end()) != addon->m_platforms.end();
}

}
