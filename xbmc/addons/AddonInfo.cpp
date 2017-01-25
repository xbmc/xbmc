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

#include "AddonInfo.h"
#include "addons/kodi-addon-dev-kit/include/kodi/versions.h"
#include "Util.h"
#include "filesystem/File.h"
#include "filesystem/SpecialProtocol.h"
#include "guilib/LocalizeStrings.h"
#include "utils/JSONVariantParser.h"
#include "utils/JSONVariantWriter.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/XBMCTinyXML.h"

namespace ADDON
{

CAddonExtensions::CAddonExtensions()
{
  //fprintf(stderr, "------------------------------------------CAddonExtensions() No. %i\n", m_cnt);
}

bool CAddonExtensions::ParseExtension(const TiXmlElement* element)
{
  const char* cstring;
  const char* cstring2;

  cstring = element->Attribute("point");
  if (cstring && (strcmp(cstring, "kodi.addon.metadata") == 0 || strcmp(cstring, "xbmc.addon.metadata") == 0))
    return true;

  m_point = cstring ? cstring : "";

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
        extension.push_back(std::pair<std::string, std::string>(name, value));
      }
    }
    attribute = attribute->Next();
  }
  if (!extension.empty())
    m_values.push_back(std::pair<std::string, EXT_VALUE>("", extension));

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
          cstring2 = attribute->Value();
          if (cstring2)
          {
            std::string value = cstring2;
            name = id + "@" + name;
            extension.push_back(std::pair<std::string, std::string>(name, value));
          }
        }
        attribute = attribute->Next();
      }
    
      cstring2 = childElement->GetText();
      if (cstring2)
        extension.push_back(std::pair<std::string, std::string>(id, cstring2));

      if (!extension.empty())
        m_values.push_back(std::pair<std::string, EXT_VALUE>(id, extension));

      if (!cstring2)
      {
        const TiXmlElement* childSubElement = childElement->FirstChildElement();
        if (childSubElement)
        {
          CAddonExtensions subElement;
          if (subElement.ParseExtension(childElement))
            m_childs.push_back(std::pair<std::string, CAddonExtensions>(id, subElement));
        }
      }
    }
    childElement = childElement->NextSiblingElement();
  }

        //fprintf (stderr, "attribute->Name(); %s - %s\n", name.c_str(), value.c_str());
  
//   if (!m_point.empty())
//   {
//     for (auto ptr : m_extension)
//      fprintf (stderr, "        No. %i - m_extension %s - %s\n", m_cnt, ptr.first.c_str(), ptr.second.c_str());
// 
//     fprintf (stderr, "\n\n END---       No. %i - m_point %s\n", m_cnt, m_point.c_str());
//     for (int i = 0; i < m_values.size(); i++)
//     {
//       for (int j = 0; j < m_values[i].second.size(); j++)
//       {
//         fprintf (stderr, " ---> %03i-%03i: %s%s - %s\n", i, j, m_values[i].first.c_str(), m_values[i].second[j].first.c_str(), m_values[i].second[j].second.c_str());
//       }
//     }
//     if (!m_childs.empty())
//     {
//       for (auto child : m_childs)
//       {
//         fprintf (stderr, "        No. %i - child %s\n", m_cnt, child.first.c_str());
//         for (int i = 0; i < child.second.m_values.size(); i++)
//         {
//           for (int j = 0; j < child.second.m_values[i].second.size(); j++)
//           {
//             fprintf (stderr, " ---> %03i-%03i: %s%s - %s\n", i, j, child.second.m_values[i].first.c_str(), child.second.m_values[i].second[j].first.c_str(), child.second.m_values[i].second[j].second.c_str());
//           }
//         }
// //         for (auto ptr : child.second.m_extension)
// //           fprintf (stderr, "        No. %i -   child->m_extension %s - %s\n", m_cnt, ptr.first.c_str(), ptr.second.c_str());
//       }
//     }
// 
//     fprintf (stderr, "\n\n\n");
//   }
  return true;
}

CAddonExtensions::~CAddonExtensions()
{
  
}

std::string CAddonExtensions::GetExtValue(std::string id) const
{
  for (int i = 0; i < m_values.size(); i++)
  {
    for (int j = 0; j < m_values[i].second.size(); j++)
    {
      if (m_values[i].second[j].first == id)
        return m_values[i].second[j].second;
    }
  }
  return "";
}

const EXT_VALUES& CAddonExtensions::GetExtValues() const
{
  return m_values;
}

CAddonExtensions* CAddonExtensions::GetExtElement(std::string id)
{
  for (EXT_ELEMENTS::iterator it = m_childs.begin(); it != m_childs.end(); ++it)
  {
    if (it->first == id)
      return &it->second;
  }
 
  return nullptr;
}

bool CAddonExtensions::GetExtElements(std::string id, EXT_ELEMENTS &result)
{
 
  return true;
}

bool CAddonExtensions::GetExtList(std::string id, std::vector<std::string> &result) const
{
 
  return true;
}

void CAddonExtensions::Insert(std::string id, std::string value)
{
  EXT_VALUE extension;
  extension.push_back(std::pair<std::string, std::string>(id, value));
  m_values.push_back(std::pair<std::string, EXT_VALUE>(id, extension));
}

/**
 * helper functions 
 *
 */

typedef struct
{
  const char* name;
  TYPE        type;
  int         pretty;
  const char* icon;
} TypeMapping;

static const TypeMapping types[] =
  {{"unknown",                           ADDON_UNKNOWN,                 0, "" },
   {"xbmc.metadata.scraper.albums",      ADDON_SCRAPER_ALBUMS,      24016, "DefaultAddonAlbumInfo.png" },
   {"xbmc.metadata.scraper.artists",     ADDON_SCRAPER_ARTISTS,     24017, "DefaultAddonArtistInfo.png" },
   {"xbmc.metadata.scraper.movies",      ADDON_SCRAPER_MOVIES,      24007, "DefaultAddonMovieInfo.png" },
   {"xbmc.metadata.scraper.musicvideos", ADDON_SCRAPER_MUSICVIDEOS, 24015, "DefaultAddonMusicVideoInfo.png" },
   {"xbmc.metadata.scraper.tvshows",     ADDON_SCRAPER_TVSHOWS,     24014, "DefaultAddonTvInfo.png" },
   {"xbmc.metadata.scraper.library",     ADDON_SCRAPER_LIBRARY,     24083, "DefaultAddonInfoLibrary.png" },
   {"xbmc.ui.screensaver",               ADDON_SCREENSAVER,         24008, "DefaultAddonScreensaver.png" },
   {"xbmc.player.musicviz",              ADDON_VIZ,                 24010, "DefaultAddonVisualization.png" },
   {"xbmc.python.pluginsource",          ADDON_PLUGIN,              24005, "" },
   {"xbmc.python.script",                ADDON_SCRIPT,              24009, "" },
   {"xbmc.python.weather",               ADDON_SCRIPT_WEATHER,      24027, "DefaultAddonWeather.png" },
   {"xbmc.python.lyrics",                ADDON_SCRIPT_LYRICS,       24013, "DefaultAddonLyrics.png" },
   {"xbmc.python.library",               ADDON_SCRIPT_LIBRARY,      24081, "DefaultAddonHelper.png" },
   {"xbmc.python.module",                ADDON_SCRIPT_MODULE,       24082, "DefaultAddonLibrary.png" },
   {"xbmc.subtitle.module",              ADDON_SUBTITLE_MODULE,     24012, "DefaultAddonSubtitles.png" },
   {"kodi.context.item",                 ADDON_CONTEXT_ITEM,        24025, "DefaultAddonContextItem.png" },
   {"kodi.game.controller",              ADDON_GAME_CONTROLLER,     35050, "DefaultAddonGame.png" },
   {"xbmc.gui.skin",                     ADDON_SKIN,                  166, "DefaultAddonSkin.png" },
   {"xbmc.webinterface",                 ADDON_WEB_INTERFACE,         199, "DefaultAddonWebSkin.png" },
   {"xbmc.addon.repository",             ADDON_REPOSITORY,          24011, "DefaultAddonRepository.png" },
   {"kodi.pvrclient",                    ADDON_PVRDLL,              24019, "DefaultAddonPVRClient.png" },
   {"kodi.gameclient",                   ADDON_GAMEDLL,             35049, "DefaultAddonGame.png" },
   {"kodi.peripheral",                   ADDON_PERIPHERALDLL,       35010, "DefaultAddonPeripheral.png" },
   {"xbmc.addon.video",                  ADDON_VIDEO,                1037, "DefaultAddonVideo.png" },
   {"xbmc.addon.audio",                  ADDON_AUDIO,                1038, "DefaultAddonMusic.png" },
   {"xbmc.addon.image",                  ADDON_IMAGE,                1039, "DefaultAddonPicture.png" },
   {"xbmc.addon.executable",             ADDON_EXECUTABLE,           1043, "DefaultAddonProgram.png" },
   {"kodi.addon.game",                   ADDON_GAME,                35049, "DefaultAddonGame.png" },
   {"kodi.audioencoder",                 ADDON_AUDIOENCODER,         200,  "DefaultAddonAudioEncoder.png" },
   {"kodi.audiodecoder",                 ADDON_AUDIODECODER,         201,  "DefaultAddonAudioDecoder.png" },
   {"xbmc.service",                      ADDON_SERVICE,             24018, "DefaultAddonService.png" },
   {"kodi.resource.images",              ADDON_RESOURCE_IMAGES,     24035, "DefaultAddonImages.png" },
   {"kodi.resource.language",            ADDON_RESOURCE_LANGUAGE,   24026, "DefaultAddonLanguage.png" },
   {"kodi.resource.uisounds",            ADDON_RESOURCE_UISOUNDS,   24006, "DefaultAddonUISounds.png" },
   {"kodi.resource.games",               ADDON_RESOURCE_GAMES,      35209, "DefaultAddonGame.png" },
   {"kodi.adsp",                         ADDON_ADSPDLL,             24135, "DefaultAddonAudioDSP.png" },
   {"kodi.inputstream",                  ADDON_INPUTSTREAM,         24048, "DefaultAddonInputstream.png" },
   {"kodi.vfs",                          ADDON_VFS,                 39013, "DefaultAddonVfs.png" },
  };

std::string CAddonInfo::TranslateType(ADDON::TYPE type, bool pretty/*=false*/)
{
  for (unsigned int index=0; index < ARRAY_SIZE(types); ++index)
  {
    const TypeMapping &map = types[index];
    if (type == map.type)
    {
      if (pretty && map.pretty)
        return g_localizeStrings.Get(map.pretty);
      else
        return map.name;
    }
  }
  return "";
}

TYPE CAddonInfo::TranslateType(const std::string &string)
{
  for (unsigned int index=0; index < ARRAY_SIZE(types); ++index)
  {
    const TypeMapping &map = types[index];
    if (string == map.name)
      return map.type;
  }

  return ADDON_UNKNOWN;
}

std::string CAddonInfo::TranslateIconType(ADDON::TYPE type)
{
  for (unsigned int index=0; index < ARRAY_SIZE(types); ++index)
  {
    const TypeMapping &map = types[index];
    if (type == map.type)
      return map.icon;
  }
  return "";
}

const char* CAddonInfo::GetPlatformLibraryName(const TiXmlElement* element)
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
#elif defined(TARGET_WINDOWS) && defined(HAS_DX)
  libraryName = element->Attribute("library_windx");
  if (libraryName == nullptr)
    libraryName = element->Attribute("library_windows");
#elif defined(TARGET_DARWIN)
#if defined(TARGET_DARWIN_IOS)
  libraryName = element->Attribute("library_ios");
  if (libraryName == nullptr)
#endif
  libraryName = element->Attribute("library_osx");
#endif

  return libraryName;
}

CAddonInfo::CAddonInfo(std::string addonPath)
  : m_usable(false),
    m_type(ADDON_UNKNOWN),
    m_path(addonPath)
{
  auto addonXmlPath = CSpecialProtocol::TranslatePath(URIUtils::AddFileToFolder(m_path, "addon.xml"));

  CXBMCTinyXML xmlDoc;
  if (!xmlDoc.LoadFile(addonXmlPath))
  {
    CLog::Log(LOGERROR, "CAddonInfo: Unable to load '%s', Line %d\n%s",
                                               m_path.c_str(),
                                               xmlDoc.ErrorRow(),
                                               xmlDoc.ErrorDesc());
    return;
  }

  m_usable = LoadAddonXML(xmlDoc.RootElement(), addonXmlPath);
  if (m_usable)
  {
    if (m_icon.empty())
    {
      std::string tmpIcon = URIUtils::AddFileToFolder(m_path, "icon.png");
      if (XFILE::CFile::Exists(tmpIcon))
        m_icon = tmpIcon;
    }

    if (m_fanart.empty())
    {
      std::string tmpFanart = URIUtils::AddFileToFolder(m_path, "fanart.jpg");
      if (XFILE::CFile::Exists(tmpFanart))
        m_fanart = tmpFanart;
    }
  }
}

CAddonInfo::CAddonInfo(const TiXmlElement* baseElement, std::string addonRepoXmlPath)
  : m_usable(false),
    m_type(ADDON_UNKNOWN)
{
  m_usable = LoadAddonXML(baseElement, addonRepoXmlPath);
  if (m_usable)
  {
    /*
     * For repo based addon data becomes the folders set from here, to know the
     * place is the add-on id needed who comes from xml.
     *
     * Also need the add-on path set to the zip file on repository place.
     */
    for (unsigned int i = 0; i < m_screenshots.size(); ++i)
    {
      m_screenshots[i] = URIUtils::AddFileToFolder(addonRepoXmlPath, StringUtils::Format("%s/%s", m_id.c_str(), m_screenshots[i].c_str()));
    }
    if (!m_fanart.empty())
      m_fanart = URIUtils::AddFileToFolder(addonRepoXmlPath, StringUtils::Format("%s/%s", m_id.c_str(), m_fanart.c_str()));
    if (!m_icon.empty())
      m_icon = URIUtils::AddFileToFolder(addonRepoXmlPath, StringUtils::Format("%s/%s", m_id.c_str(), m_icon.c_str()));
    m_path = URIUtils::AddFileToFolder(addonRepoXmlPath, StringUtils::Format("%s/%s-%s.zip", m_id.c_str(), m_id.c_str(), m_version.asString().c_str()));
  }
}

CAddonInfo::CAddonInfo(const std::string& id,
                       const AddonVersion& version,
                       const std::string& name,
                       const std::string& summary,
                       const std::string& description,
                       const std::string& metadata,
                       const std::string& changelog,
                       const std::string& origin)
  : m_usable(false),
    m_id(id),
    m_type(ADDON_UNKNOWN),
    m_version(version),
    m_name(name),
    m_summary(summary),
    m_description(description),
    m_changelog(changelog),
    m_origin(origin)
{
  DeserializeMetadata(metadata);
  if (!m_id.empty() && m_type > ADDON_UNKNOWN && m_type < ADDON_MAX)
  {
    m_usable = true;

    SetProvides(GetExtValue("provides"));
  }
  else
  {
    m_usable = false;
    CLog::Log(LOGERROR, "CAddonInfo: tried to create add-on info with invalid data (id='%s', type='%i')", m_id.c_str(), m_type);
  }
}

CAddonInfo::CAddonInfo()
  : m_usable(true),
    m_type(ADDON_UNKNOWN),
    m_packageSize(0)
{
}

CAddonInfo::CAddonInfo(std::string id, TYPE type)
  : m_usable(true),
    m_id(std::move(id)),
    m_type(type),
    m_packageSize(0)
{

}

bool CAddonInfo::LoadAddonXML(const TiXmlElement* baseElement, std::string addonXmlPath)
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

  if (!StringUtils::EqualsNoCase(baseElement->Value(), "addon"))
  {
    CLog::Log(LOGERROR, "CAddonInfo: file from '%s' doesnt contain <addon>", addonXmlPath.c_str());
    return false;
  }

  /*
   * Parse addon.xml:
   * <addon id="???"
   *        name="???"
   *        version="???"
   *        provider-name="???">
   */
  cstring = baseElement->Attribute("id");
  m_id = cstring ? cstring : "";
  cstring = baseElement->Attribute("name");
  m_name = cstring ? cstring : "";
  cstring = baseElement->Attribute("version");
  m_version = AddonVersion(ADDON::AddonVersion(cstring ? cstring : ""));
  cstring = baseElement->Attribute("provider-name");
  m_author = cstring ? cstring : "";
  if (m_id.empty() || m_version.empty())
  {
    CLog::Log(LOGERROR, "CAddonInfo: file '%s' doesnt contain required values on <addon ... > id='%s', version='%s'", 
              addonXmlPath.c_str(),
              m_id.empty() ? "missing" : m_id.c_str(),
              m_version.empty() ? "missing" : m_version.asString().c_str());
    return false;
  }

  /*
   * Parse addon.xml:
   * <requires>
   *   <import addon="???" version="???" optional="???"/>
   * </requires>
   */
  const TiXmlElement* requires = baseElement->FirstChildElement("requires");
  if (requires)
  {
    for (const TiXmlElement* child = requires->FirstChildElement("import"); child != nullptr; child = child->NextSiblingElement("import"))
    {
      cstring = child->Attribute("addon");
      if (cstring)
      {
        const char* version = child->Attribute("version");
        bool optional = false;
        child->QueryBoolAttribute("optional", &optional);
        m_dependencies.emplace(cstring, std::make_pair(ADDON::AddonVersion(version ? version : "0.0.0"), optional));
      }
    }
  }

  /*
   * Parse addon.xml:
   * <extension>
   *   ...
   * </extension>
   */
  for (const TiXmlElement* child = baseElement->FirstChildElement("extension"); child != nullptr; child = child->NextSiblingElement("extension"))
  {
    cstring = child->Attribute("point");
    std::string point = cstring ? cstring : "";

    if (!ParseExtension(child))
    {
      CLog::Log(LOGERROR, "CAddonInfo: file '%s' doesn't contain a valid add-on extensions (%s)", addonXmlPath.c_str(), point.c_str());
      return false;
    }

    if (point == "kodi.addon.metadata" || point == "xbmc.addon.metadata")
    {
      /*
       * Parse addon.xml "<summary lang="..">...</summary>"
       */
      const char* strSummary = nullptr;
      for (const TiXmlElement* element = child->FirstChildElement("summary"); element != nullptr; element = element->NextSiblingElement("summary"))
      {
        cstring = element->Attribute("lang");
        if (cstring != nullptr)
        {
          std::string lang = cstring;
          if (g_langInfo.GetLocale().Matches(lang))
          {
            strSummary = element->GetText();
            break;
          }
          else if (lang == "en" || lang == "en_GB" || strSummary == nullptr)
          {
            strSummary = element->GetText();
          }
        }
        else
          strSummary = element->GetText();
      }
      m_summary = strSummary ? strSummary : "";

      /*
       * Parse addon.xml "<description lang="..">...</description>"
       */
      const char* strDescription = nullptr;
      for (const TiXmlElement* element = child->FirstChildElement("description"); element != nullptr; element = element->NextSiblingElement("description"))
      {
        cstring = element->Attribute("lang");
        if (cstring != nullptr)
        {
          std::string lang = cstring;
          if (g_langInfo.GetLocale().Matches(lang))
          {
            strDescription = element->GetText();
            break;
          }
          else if (lang == "en" || lang == "en_GB" || strDescription == nullptr)
          {
            strDescription = element->GetText();
          }
        }
        else
          strDescription = element->GetText();
      }
      m_description = strDescription ? strDescription : "";
  
      /*
       * Parse addon.xml "<disclaimer lang="..">...</disclaimer>"
       */
      const char* strDisclaimer = nullptr;
      for (const TiXmlElement* element = child->FirstChildElement("disclaimer"); element != nullptr; element = element->NextSiblingElement("disclaimer"))
      {
        cstring = element->Attribute("lang");
        if (cstring != nullptr)
        {
          std::string lang = cstring;
          if (g_langInfo.GetLocale().Matches(lang))
          {
            strDisclaimer = element->GetText();
            break;
          }
          else if (lang == "en" || lang == "en_GB" || strDisclaimer == nullptr)
          {
            strDisclaimer = element->GetText();
          }
        }
        else
          strDisclaimer = element->GetText();
      }
      m_disclaimer = strDisclaimer ? strDisclaimer : "";

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
              m_icon = URIUtils::AddFileToFolder(m_path, elementsAssets->GetText());
          }
          else if (value == "fanart")
          {
            if (elementsAssets->GetText() != nullptr)
              m_fanart = URIUtils::AddFileToFolder(m_path, elementsAssets->GetText());
          }
          else if (value == "screenshot")
          {
            if (elementsAssets->GetText() != nullptr)
              m_screenshots.emplace_back(URIUtils::AddFileToFolder(m_path, elementsAssets->GetText()));
          }
        }
      }

      /* Parse addon.xml "<license">...</license>" */
      element = child->FirstChildElement("license");
      if (element && element->GetText() != nullptr)
        m_license = element->GetText();

      /* Parse addon.xml "<source">...</source>" */
      element = child->FirstChildElement("source");
      if (element && element->GetText() != nullptr)
        m_source = element->GetText();

      /* Parse addon.xml "<broken">...</broken>" */
      element = child->FirstChildElement("broken");
      if (element && element->GetText() != nullptr)
        m_broken = element->GetText();

      /* Parse addon.xml "<language">...</language>" */
      element = child->FirstChildElement("language");
      if (element && element->GetText() != nullptr)
        m_extrainfo.insert(std::make_pair("language", element->GetText()));

      /* Parse addon.xml "<noicon">...</noicon>" */
      if (m_icon.empty())
      {
        element = child->FirstChildElement("noicon");
        m_icon = (element && strcmp(element->GetText() , "true") == 0) ? "" : "icon.png";
      }

      /* Parse addon.xml "<nofanart">...</nofanart>" */
      if (m_fanart.empty())
      {
        element = child->FirstChildElement("nofanart");
        m_fanart = (element && strcmp(element->GetText() , "true") == 0) ? "" : "fanart.jpg";
      }

      /* Parse addon.xml "<size">...</size>" */
      element = child->FirstChildElement("size");
      if (element && element->GetText() != nullptr)
        m_packageSize = StringUtils::ToUint64(element->GetText(), 0);

      /* Parse addon.xml "<news lang="..">...</news>" */
      const char* strChangelog = nullptr;
      element = child->FirstChildElement("news");
      while (element)
      {
        const char *lang = element->Attribute("lang");
        if (lang != nullptr && g_langInfo.GetLocale().Matches(lang))
        {
          strChangelog = element->GetText();
          break;
        }
        else if (lang == nullptr || strcmp(lang, "en") == 0 || strcmp(lang, "en_GB") == 0)
        {
          strChangelog = element->GetText();
        }

        element = element->NextSiblingElement("news");
      }
      m_changelog = strChangelog ? strChangelog : "";
    }
    else if (point == "xbmc.python.script" && m_type == ADDON_UNKNOWN)
    {
      m_type = ADDON_SCRIPT;
      /* Parse addon.xml "<provides">...</provides>" */
      const TiXmlElement* element = child->FirstChildElement("provides");
      if (element && element->GetText() != nullptr)
        m_extrainfo.insert(std::make_pair("provides", element->GetText()));
    }
    else if (point == "xbmc.python.pluginsource" && m_type == ADDON_UNKNOWN)
    {
      m_type = ADDON_PLUGIN;
      /* Parse addon.xml "<provides">...</provides>" */
      const TiXmlElement* element = child->FirstChildElement("provides");
      if (element && element->GetText() != nullptr)
        m_extrainfo.insert(std::make_pair("provides", element->GetText()));
    }
    else if (point == "kodi.resource.language" && m_type == ADDON_UNKNOWN)
    {
      m_type = ADDON_RESOURCE_LANGUAGE;
      /* Parse addon.xml "<locale">...</locale>" */
      cstring = child->Attribute("locale");
      if (cstring != nullptr)
        m_extrainfo.insert(std::make_pair("locale", cstring));
    }
    else
    {
//       if (!ParseExtension(child))
//       {
//         CLog::Log(LOGERROR, "CAddonInfo: file '%s' doesn't contain a valid add-on extensions (%s)", addonXmlPath.c_str(), point.c_str());
//         return false;
//       }

      // Get add-on type
      if (m_type == ADDON_UNKNOWN)
      {
        m_type = TranslateType(point);
        if (m_type == ADDON_UNKNOWN || m_type >= ADDON_MAX)
        {
          CLog::Log(LOGERROR, "CAddonInfo: file '%s' doesn't contain a valid add-on type name (%s)", addonXmlPath.c_str(), point.c_str());
          return false;
        }
      }

      // Get add-on library file name (if present)
      const char* library = child->Attribute("library");
      if (library == nullptr)
        library = GetPlatformLibraryName(child);
      if (library != nullptr)
        m_libname = library;

      const TiXmlAttribute* attribute = child->FirstAttribute();
      while (attribute)
      {
        std::string name = attribute->Name();
        if (name != "point" && !StringUtils::StartsWithNoCase(name, "library") )
        {
          const char* value = attribute->Value();
          if (value)
            m_extrainfo.insert(std::make_pair(name, value));
        }
        attribute = attribute->Next();
      }
    }
  }

  SetProvides(GetExtValue("provides"));

  return true;
}

std::string CAddonInfo::SerializeMetadata()
{
  CVariant variant;
  variant["author"] = m_author;
  variant["disclaimer"] = m_disclaimer;
  variant["broken"] = m_broken;
  variant["size"] = m_packageSize;

  variant["path"] = m_path;
  variant["fanart"] = m_fanart;
  variant["icon"] = m_icon;

  variant["screenshots"] = CVariant(CVariant::VariantTypeArray);
  for (const auto& item : m_screenshots)
    variant["screenshots"].push_back(item);

  variant["extensions"] = CVariant(CVariant::VariantTypeArray);
  variant["extensions"].push_back(TranslateType(m_type, false));

  variant["dependencies"] = CVariant(CVariant::VariantTypeArray);
  for (const auto& kv : m_dependencies)
  {
    CVariant dep(CVariant::VariantTypeObject);
    dep["addonId"] = kv.first;
    dep["version"] = kv.second.first.asString();
    dep["optional"] = kv.second.second;
    variant["dependencies"].push_back(std::move(dep));
  }

  variant["extrainfo"] = CVariant(CVariant::VariantTypeArray);
  for (const auto& kv : m_extrainfo)
  {
    CVariant info(CVariant::VariantTypeObject);
    info["key"] = kv.first;
    info["value"] = kv.second;
    variant["extrainfo"].push_back(std::move(info));
  }

  return CJSONVariantWriter::Write(variant, true);
}

void CAddonInfo::DeserializeMetadata(const std::string& document)
{
  CVariant variant = CJSONVariantParser::Parse(document);

  m_author = variant["author"].asString();
  m_disclaimer = variant["disclaimer"].asString();
  m_broken = variant["broken"].asString();
  m_packageSize = variant["size"].asUnsignedInteger();

  m_path = variant["path"].asString();
  m_fanart = variant["fanart"].asString();
  m_icon = variant["icon"].asString();

  std::vector<std::string> screenshots;
  for (auto it = variant["screenshots"].begin_array(); it != variant["screenshots"].end_array(); ++it)
    screenshots.push_back(it->asString());
  screenshots = std::move(screenshots);

  m_type = TranslateType(variant["extensions"][0].asString());

  ADDONDEPS deps;
  for (auto it = variant["dependencies"].begin_array(); it != variant["dependencies"].end_array(); ++it)
  {
    AddonVersion version((*it)["version"].asString());
    deps.emplace((*it)["addonId"].asString(), std::make_pair(std::move(version), (*it)["optional"].asBoolean()));
  }
  m_dependencies = std::move(deps);

  //InfoMap extraInfo;
  for (auto it = variant["extrainfo"].begin_array(); it != variant["extrainfo"].end_array(); ++it)
    Insert((*it)["key"].asString(), (*it)["value"].asString());
//     extraInfo.emplace((*it)["key"].asString(), (*it)["value"].asString());
//   m_extrainfo = std::move(extraInfo);
  
  m_usable = true;
}

bool CAddonInfo::MeetsVersion(const AddonVersion &version) const
{
  return m_minversion <= version && version <= m_version;
}

CAddonInfo::SubContent CAddonInfo::TranslateSubContent(const std::string &content)
{
  if (content == "audio")
    return CAddonInfo::AUDIO;
  else if (content == "image")
    return CAddonInfo::IMAGE;
  else if (content == "executable")
    return CAddonInfo::EXECUTABLE;
  else if (content == "video")
    return CAddonInfo::VIDEO;
  else if (content == "game")
    return CAddonInfo::GAME;
  else
    return CAddonInfo::UNKNOWN;
}

std::string CAddonInfo::LibPath() const
{
  if (m_libname.empty())
    return "";
  return URIUtils::AddFileToFolder(m_path, m_libname);
}

void CAddonInfo::SetProvides(const std::string &content)
{
  if (!content.empty())
  {
    fprintf(stderr, "---->ID: %s\n", ID().c_str());
    for (auto provide : StringUtils::Split(content, ' '))
    {
      fprintf(stderr, "   ->Provide: %s\n", provide.c_str());
      SubContent content = TranslateSubContent(provide);
      if (content != UNKNOWN)
        m_providedSubContent.insert(content);
    }
  }
  if (m_type == ADDON_SCRIPT && m_providedSubContent.empty())
    m_providedSubContent.insert(EXECUTABLE);
}

bool CAddonInfo::IsType(TYPE type) const
{
  return ((type == ADDON_VIDEO && ProvidesSubContent(VIDEO))
       || (type == ADDON_AUDIO && ProvidesSubContent(AUDIO))
       || (type == ADDON_IMAGE && ProvidesSubContent(IMAGE))
       || (type == ADDON_GAME && ProvidesSubContent(GAME))
       || (type == ADDON_EXECUTABLE && ProvidesSubContent(EXECUTABLE))
       || (type == m_type));
}
    
} /* namespace ADDON */
