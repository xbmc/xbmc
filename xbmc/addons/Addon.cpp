/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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

#include "Addon.h"
#include "AddonManager.h"
#include "addons/Service.h"
#include "ContextMenuManager.h"
#include "settings/Settings.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "system.h"
#ifdef HAS_PYTHON
#include "interfaces/python/XBPython.h"
#endif
#if defined(TARGET_DARWIN)
#include "../osx/OSXGNUReplacements.h"
#endif
#ifdef TARGET_FREEBSD
#include "freebsd/FreeBSDGNUReplacements.h"
#endif
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "URL.h"
#include "Util.h"
#include <vector>
#include <string.h>
#include <ostream>

using XFILE::CDirectory;
using XFILE::CFile;
using namespace std;

namespace ADDON
{

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
   {"visualization-library",             ADDON_VIZ_LIBRARY,         24084, "" },
   {"xbmc.python.pluginsource",          ADDON_PLUGIN,              24005, "" },
   {"xbmc.python.script",                ADDON_SCRIPT,              24009, "" },
   {"xbmc.python.weather",               ADDON_SCRIPT_WEATHER,      24027, "DefaultAddonWeather.png" },
   {"xbmc.python.lyrics",                ADDON_SCRIPT_LYRICS,       24013, "DefaultAddonLyrics.png" },
   {"xbmc.python.library",               ADDON_SCRIPT_LIBRARY,      24081, "DefaultAddonHelper.png" },
   {"xbmc.python.module",                ADDON_SCRIPT_MODULE,       24082, "DefaultAddonLibrary.png" },
   {"xbmc.subtitle.module",              ADDON_SUBTITLE_MODULE,     24012, "DefaultAddonSubtitles.png" },
   {"kodi.context.item",                 ADDON_CONTEXT_ITEM,        24025, "DefaultAddonContextItem.png" },
   {"xbmc.gui.skin",                     ADDON_SKIN,                  166, "DefaultAddonSkin.png" },
   {"xbmc.webinterface",                 ADDON_WEB_INTERFACE,         199, "DefaultAddonWebSkin.png" },
   {"xbmc.addon.repository",             ADDON_REPOSITORY,          24011, "DefaultAddonRepository.png" },
   {"xbmc.pvrclient",                    ADDON_PVRDLL,              24019, "DefaultAddonPVRClient.png" },
   {"xbmc.addon.video",                  ADDON_VIDEO,                1037, "DefaultAddonVideo.png" },
   {"xbmc.addon.audio",                  ADDON_AUDIO,                1038, "DefaultAddonMusic.png" },
   {"xbmc.addon.image",                  ADDON_IMAGE,                1039, "DefaultAddonPicture.png" },
   {"xbmc.addon.executable",             ADDON_EXECUTABLE,           1043, "DefaultAddonProgram.png" },
   {"xbmc.audioencoder",                 ADDON_AUDIOENCODER,         200,  "DefaultAddonAudioEncoder.png" },
   {"kodi.audiodecoder",                 ADDON_AUDIODECODER,         201,  "DefaultAddonAudioDecoder.png" },
   {"xbmc.service",                      ADDON_SERVICE,             24018, "DefaultAddonService.png" },
   {"kodi.resource.language",            ADDON_RESOURCE_LANGUAGE,   24026, "DefaultAddonLanguage.png" },
   {"kodi.resource.uisounds",            ADDON_RESOURCE_UISOUNDS,   24006, "DefaultAddonUISounds.png" },
  };

const std::string TranslateType(const ADDON::TYPE &type, bool pretty/*=false*/)
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

TYPE TranslateType(const std::string &string)
{
  for (unsigned int index=0; index < ARRAY_SIZE(types); ++index)
  {
    const TypeMapping &map = types[index];
    if (string == map.name)
      return map.type;
  }

  return ADDON_UNKNOWN;
}

const std::string GetIcon(const ADDON::TYPE& type)
{
  for (unsigned int index=0; index < ARRAY_SIZE(types); ++index)
  {
    const TypeMapping &map = types[index];
    if (type == map.type)
      return map.icon;
  }
  return "";
}

#define EMPTY_IF(x,y) \
  { \
    std::string fan=CAddonMgr::Get().GetExtValue(metadata->configuration, x); \
    if (fan == "true") \
      y.clear(); \
  }

#define SS(x) (x) ? x : ""

AddonProps::AddonProps(const cp_extension_t *ext)
  : id(SS(ext->plugin->identifier))
  , version(SS(ext->plugin->version))
  , minversion(SS(ext->plugin->abi_bw_compatibility))
  , name(SS(ext->plugin->name))
  , path(SS(ext->plugin->plugin_path))
  , author(SS(ext->plugin->provider_name))
  , stars(0)
{
  if (ext->ext_point_id)
    type = TranslateType(ext->ext_point_id);

  icon = "icon.png";
  fanart = URIUtils::AddFileToFolder(path, "fanart.jpg");
  changelog = URIUtils::AddFileToFolder(path, "changelog.txt");
  // Grab more detail from the props...
  const cp_extension_t *metadata = CAddonMgr::Get().GetExtension(ext->plugin, "xbmc.addon.metadata"); //<! backword compatibilty
  if (!metadata)
    metadata = CAddonMgr::Get().GetExtension(ext->plugin, "kodi.addon.metadata");
  if (metadata)
  {
    summary = CAddonMgr::Get().GetTranslatedString(metadata->configuration, "summary");
    description = CAddonMgr::Get().GetTranslatedString(metadata->configuration, "description");
    disclaimer = CAddonMgr::Get().GetTranslatedString(metadata->configuration, "disclaimer");
    license = CAddonMgr::Get().GetExtValue(metadata->configuration, "license");
    std::string language;
    language = CAddonMgr::Get().GetExtValue(metadata->configuration, "language");
    if (!language.empty())
      extrainfo.insert(make_pair("language",language));
    broken = CAddonMgr::Get().GetExtValue(metadata->configuration, "broken");
    EMPTY_IF("nofanart",fanart)
    EMPTY_IF("noicon",icon)
    EMPTY_IF("nochangelog",changelog)
  }
  BuildDependencies(ext->plugin);
}

AddonProps::AddonProps(const cp_plugin_info_t *plugin)
  : id(SS(plugin->identifier))
  , type(ADDON_UNKNOWN)
  , version(SS(plugin->version))
  , minversion(SS(plugin->abi_bw_compatibility))
  , name(SS(plugin->name))
  , path(SS(plugin->plugin_path))
  , author(SS(plugin->provider_name))
  , stars(0)
{
  BuildDependencies(plugin);
}

void AddonProps::Serialize(CVariant &variant) const
{
  variant["addonid"] = id;
  variant["type"] = TranslateType(type);
  variant["version"] = version.asString();
  variant["minversion"] = minversion.asString();
  variant["name"] = name;
  variant["license"] = license;
  variant["summary"] = summary;
  variant["description"] = description;
  variant["path"] = path;
  variant["libname"] = libname;
  variant["author"] = author;
  variant["source"] = source;

  if (CURL::IsFullPath(icon))
    variant["icon"] = icon;
  else
    variant["icon"] = URIUtils::AddFileToFolder(path, icon);

  variant["thumbnail"] = variant["icon"];
  variant["disclaimer"] = disclaimer;
  variant["changelog"] = changelog;

  if (CURL::IsFullPath(fanart))
    variant["fanart"] = fanart;
  else
    variant["fanart"] = URIUtils::AddFileToFolder(path, fanart);

  variant["dependencies"] = CVariant(CVariant::VariantTypeArray);
  for (ADDONDEPS::const_iterator it = dependencies.begin(); it != dependencies.end(); ++it)
  {
    CVariant dep(CVariant::VariantTypeObject);
    dep["addonid"] = it->first;
    dep["version"] = it->second.first.asString();
    dep["optional"] = it->second.second;
    variant["dependencies"].push_back(dep);
  }
  if (broken.empty())
    variant["broken"] = false;
  else
    variant["broken"] = broken;
  variant["extrainfo"] = CVariant(CVariant::VariantTypeArray);
  for (InfoMap::const_iterator it = extrainfo.begin(); it != extrainfo.end(); ++it)
  {
    CVariant info(CVariant::VariantTypeObject);
    info["key"] = it->first;
    info["value"] = it->second;
    variant["extrainfo"].push_back(info);
  }
  variant["rating"] = stars;
}

void AddonProps::BuildDependencies(const cp_plugin_info_t *plugin)
{
  if (!plugin)
    return;
  for (unsigned int i = 0; i < plugin->num_imports; ++i)
    dependencies.insert(make_pair(std::string(plugin->imports[i].plugin_id),
                                  make_pair(AddonVersion(SS(plugin->imports[i].version)), plugin->imports[i].optional != 0)));
}

/**
 * CAddon
 *
 */

CAddon::CAddon(const cp_extension_t *ext)
  : m_props(ext)
{
  BuildLibName(ext);
  Props().libname = m_strLibName;
  BuildProfilePath();
  m_userSettingsPath = URIUtils::AddFileToFolder(Profile(), "settings.xml");
  m_hasSettings = true;
  m_hasStrings = false;
  m_checkedStrings = false;
  m_settingsLoaded = false;
  m_userSettingsLoaded = false;
}

CAddon::CAddon(const cp_plugin_info_t *plugin)
  : m_props(plugin)
{
  m_hasSettings = false;
  m_hasStrings = false;
  m_checkedStrings = true;
  m_settingsLoaded = false;
  m_userSettingsLoaded = false;
}

CAddon::CAddon(const AddonProps &props)
  : m_props(props)
{
  if (props.libname.empty()) BuildLibName();
  else m_strLibName = props.libname;
  BuildProfilePath();
  m_userSettingsPath = URIUtils::AddFileToFolder(Profile(), "settings.xml");
  m_hasSettings = true;
  m_hasStrings = false;
  m_checkedStrings = false;
  m_settingsLoaded = false;
  m_userSettingsLoaded = false;
}

CAddon::CAddon(const CAddon &rhs)
  : m_props(rhs.Props()),
    m_settings(rhs.m_settings)
{
  m_addonXmlDoc = rhs.m_addonXmlDoc;
  m_settingsLoaded = rhs.m_settingsLoaded;
  m_userSettingsLoaded = rhs.m_userSettingsLoaded;
  m_hasSettings = rhs.m_hasSettings;
  BuildProfilePath();
  m_userSettingsPath = URIUtils::AddFileToFolder(Profile(), "settings.xml");
  m_strLibName  = rhs.m_strLibName;
  m_hasStrings  = false;
  m_checkedStrings  = false;
}

AddonPtr CAddon::Clone() const
{
  return AddonPtr(new CAddon(*this));
}

bool CAddon::MeetsVersion(const AddonVersion &version) const
{
  return m_props.minversion <= version && version <= m_props.version;
}

//TODO platform/path crap should be negotiated between the addon and
// the handler for it's type
void CAddon::BuildLibName(const cp_extension_t *extension)
{
  if (!extension)
  {
    m_strLibName = "default";
    std::string ext;
    switch (m_props.type)
    {
    case ADDON_SCRAPER_ALBUMS:
    case ADDON_SCRAPER_ARTISTS:
    case ADDON_SCRAPER_MOVIES:
    case ADDON_SCRAPER_MUSICVIDEOS:
    case ADDON_SCRAPER_TVSHOWS:
    case ADDON_SCRAPER_LIBRARY:
      ext = ADDON_SCRAPER_EXT;
      break;
    case ADDON_SCREENSAVER:
      ext = ADDON_SCREENSAVER_EXT;
      break;
    case ADDON_SKIN:
      m_strLibName = "skin.xml";
      return;
    case ADDON_VIZ:
      ext = ADDON_VIS_EXT;
      break;
    case ADDON_PVRDLL:
      ext = ADDON_PVRDLL_EXT;
      break;
    case ADDON_SCRIPT:
    case ADDON_SCRIPT_LIBRARY:
    case ADDON_SCRIPT_LYRICS:
    case ADDON_SCRIPT_WEATHER:
    case ADDON_SUBTITLE_MODULE:        
    case ADDON_PLUGIN:
    case ADDON_SERVICE:
    case ADDON_CONTEXT_ITEM:
      ext = ADDON_PYTHON_EXT;
      break;
    default:
      m_strLibName.clear();
      return;
    }
    // extensions are returned as *.ext
    // so remove the asterisk
    ext.erase(0,1);
    m_strLibName.append(ext);
  }
  else
  {
    switch (m_props.type)
    {
      case ADDON_SCREENSAVER:
      case ADDON_SCRIPT:
      case ADDON_SCRIPT_LIBRARY:
      case ADDON_SCRIPT_LYRICS:
      case ADDON_SCRIPT_WEATHER:
      case ADDON_SCRIPT_MODULE:
      case ADDON_SUBTITLE_MODULE:
      case ADDON_SCRAPER_ALBUMS:
      case ADDON_SCRAPER_ARTISTS:
      case ADDON_SCRAPER_MOVIES:
      case ADDON_SCRAPER_MUSICVIDEOS:
      case ADDON_SCRAPER_TVSHOWS:
      case ADDON_SCRAPER_LIBRARY:
      case ADDON_PVRDLL:
      case ADDON_PLUGIN:
      case ADDON_WEB_INTERFACE:
      case ADDON_SERVICE:
      case ADDON_REPOSITORY:
      case ADDON_AUDIOENCODER:
      case ADDON_CONTEXT_ITEM:
      case ADDON_AUDIODECODER:
        {
          std::string temp = CAddonMgr::Get().GetExtValue(extension->configuration, "@library");
          m_strLibName = temp;
        }
        break;
      default:
        m_strLibName.clear();
        break;
    }
  }
}

/**
 * Language File Handling
 */
bool CAddon::LoadStrings()
{
  // Path where the language strings reside
  std::string chosenPath = URIUtils::AddFileToFolder(m_props.path, "resources/language/");

  m_hasStrings = m_strings.Load(chosenPath, CSettings::Get().GetString("locale.language"));
  return m_checkedStrings = true;
}

void CAddon::ClearStrings()
{
  // Unload temporary language strings
  m_strings.Clear();
  m_hasStrings = false;
}

std::string CAddon::GetString(uint32_t id)
{
  if (!m_hasStrings && ! m_checkedStrings && !LoadStrings())
     return "";

  return m_strings.Get(id);
}

/**
 * Settings Handling
 */
bool CAddon::HasSettings()
{
  return LoadSettings();
}

bool CAddon::LoadSettings(bool bForce /* = false*/)
{
  if (m_settingsLoaded && !bForce)
    return true;
  if (!m_hasSettings)
    return false;
  std::string addonFileName = URIUtils::AddFileToFolder(m_props.path, "resources/settings.xml");

  if (!m_addonXmlDoc.LoadFile(addonFileName))
  {
    if (CFile::Exists(addonFileName))
      CLog::Log(LOGERROR, "Unable to load: %s, Line %d\n%s", addonFileName.c_str(), m_addonXmlDoc.ErrorRow(), m_addonXmlDoc.ErrorDesc());
    m_hasSettings = false;
    return false;
  }

  // Make sure that the addon XML has the settings element
  TiXmlElement *setting = m_addonXmlDoc.RootElement();
  if (!setting || strcmpi(setting->Value(), "settings") != 0)
  {
    CLog::Log(LOGERROR, "Error loading Settings %s: cannot find root element 'settings'", addonFileName.c_str());
    return false;
  }
  SettingsFromXML(m_addonXmlDoc, true);
  LoadUserSettings();
  m_settingsLoaded = true;
  return true;
}

bool CAddon::HasUserSettings()
{
  if (!LoadSettings())
    return false;

  return m_userSettingsLoaded;
}

bool CAddon::ReloadSettings()
{
  return LoadSettings(true);
}

bool CAddon::LoadUserSettings()
{
  m_userSettingsLoaded = false;
  CXBMCTinyXML doc;
  if (doc.LoadFile(m_userSettingsPath))
    m_userSettingsLoaded = SettingsFromXML(doc);
  return m_userSettingsLoaded;
}

void CAddon::SaveSettings(void)
{
  if (m_settings.empty())
    return; // no settings to save

  // break down the path into directories
  std::string strAddon = URIUtils::GetDirectory(m_userSettingsPath);
  URIUtils::RemoveSlashAtEnd(strAddon);
  std::string strRoot = URIUtils::GetDirectory(strAddon);
  URIUtils::RemoveSlashAtEnd(strRoot);

  // create the individual folders
  if (!CDirectory::Exists(strRoot))
    CDirectory::Create(strRoot);
  if (!CDirectory::Exists(strAddon))
    CDirectory::Create(strAddon);

  // create the XML file
  CXBMCTinyXML doc;
  SettingsToXML(doc);
  doc.SaveFile(m_userSettingsPath);
  m_userSettingsLoaded = true;
  
  CAddonMgr::Get().ReloadSettings(ID());//push the settings changes to the running addon instance
#ifdef HAS_PYTHON
  g_pythonParser.OnSettingsChanged(ID());
#endif
}

std::string CAddon::GetSetting(const std::string& key)
{
  if (!LoadSettings())
    return ""; // no settings available

  map<std::string, std::string>::const_iterator i = m_settings.find(key);
  if (i != m_settings.end())
    return i->second;
  return "";
}

void CAddon::UpdateSetting(const std::string& key, const std::string& value)
{
  LoadSettings();
  if (key.empty()) return;
  m_settings[key] = value;
}

bool CAddon::SettingsFromXML(const CXBMCTinyXML &doc, bool loadDefaults /*=false */)
{
  if (!doc.RootElement())
    return false;

  if (loadDefaults)
    m_settings.clear();

  const TiXmlElement* category = doc.RootElement()->FirstChildElement("category");
  if (!category)
    category = doc.RootElement();

  bool foundSetting = false;
  while (category)
  {
    const TiXmlElement *setting = category->FirstChildElement("setting");
    while (setting)
    {
      const char *id = setting->Attribute("id");
      const char *value = setting->Attribute(loadDefaults ? "default" : "value");
      if (id && value)
      {
        m_settings[id] = value;
        foundSetting = true;
      }
      setting = setting->NextSiblingElement("setting");
    }
    category = category->NextSiblingElement("category");
  }
  return foundSetting;
}

void CAddon::SettingsToXML(CXBMCTinyXML &doc) const
{
  TiXmlElement node("settings");
  doc.InsertEndChild(node);
  for (map<std::string, std::string>::const_iterator i = m_settings.begin(); i != m_settings.end(); ++i)
  {
    TiXmlElement nodeSetting("setting");
    nodeSetting.SetAttribute("id", i->first.c_str());
    nodeSetting.SetAttribute("value", i->second.c_str());
    doc.RootElement()->InsertEndChild(nodeSetting);
  }
  doc.SaveFile(m_userSettingsPath);
}

TiXmlElement* CAddon::GetSettingsXML()
{
  return m_addonXmlDoc.RootElement();
}

void CAddon::BuildProfilePath()
{
  m_profile = StringUtils::Format("special://profile/addon_data/%s/", ID().c_str());
}

const std::string CAddon::Icon() const
{
  if (CURL::IsFullPath(m_props.icon))
    return m_props.icon;
  return URIUtils::AddFileToFolder(m_props.path, m_props.icon);
}

const std::string CAddon::LibPath() const
{
  return URIUtils::AddFileToFolder(m_props.path, m_strLibName);
}

AddonVersion CAddon::GetDependencyVersion(const std::string &dependencyID) const
{
  const ADDON::ADDONDEPS &deps = GetDeps();
  ADDONDEPS::const_iterator it = deps.find(dependencyID);
  if (it != deps.end())
    return it->second.first;
  return AddonVersion("0.0.0");
}

void OnEnabled(const std::string& id)
{
  // If the addon is a special, call enabled handler
  AddonPtr addon;
  if (CAddonMgr::Get().GetAddon(id, addon, ADDON_PVRDLL))
    return addon->OnEnabled();

  if (CAddonMgr::Get().GetAddon(id, addon, ADDON_SERVICE))
    std::static_pointer_cast<CService>(addon)->Start();

  if (CAddonMgr::Get().GetAddon(id, addon, ADDON_CONTEXT_ITEM))
    CContextMenuManager::Get().Register(std::static_pointer_cast<CContextItemAddon>(addon));
}

void OnDisabled(const std::string& id)
{
  AddonPtr addon;
  if (CAddonMgr::Get().GetAddon(id, addon, ADDON_PVRDLL, false))
    return addon->OnDisabled();

  if (CAddonMgr::Get().GetAddon(id, addon, ADDON_SERVICE, false))
    std::static_pointer_cast<CService>(addon)->Stop();

  if (CAddonMgr::Get().GetAddon(id, addon, ADDON_CONTEXT_ITEM, false))
    CContextMenuManager::Get().Unregister(std::static_pointer_cast<CContextItemAddon>(addon));
}

void OnPreInstall(const AddonPtr& addon)
{
  //Before installing we need to stop/unregister any local addon
  //that have this id, regardless of what the 'new' addon is.
  AddonPtr localAddon;
  if (CAddonMgr::Get().GetAddon(addon->ID(), localAddon, ADDON_SERVICE))
    std::static_pointer_cast<CService>(localAddon)->Stop();

  if (CAddonMgr::Get().GetAddon(addon->ID(), localAddon, ADDON_CONTEXT_ITEM))
    CContextMenuManager::Get().Unregister(std::static_pointer_cast<CContextItemAddon>(localAddon));

  //Fallback to the pre-install callback in the addon.
  //BUG: If primary extension point have changed we're calling the wrong method.
  addon->OnPreInstall();
}

void OnPostInstall(const AddonPtr& addon, bool update, bool modal)
{
  AddonPtr localAddon;
  if (CAddonMgr::Get().GetAddon(addon->ID(), localAddon, ADDON_SERVICE))
    std::static_pointer_cast<CService>(localAddon)->Start();

  if (CAddonMgr::Get().GetAddon(addon->ID(), localAddon, ADDON_CONTEXT_ITEM))
    CContextMenuManager::Get().Register(std::static_pointer_cast<CContextItemAddon>(localAddon));

  addon->OnPostInstall(update, modal);
}

void OnPreUnInstall(const AddonPtr& addon)
{
  AddonPtr localAddon;
  if (CAddonMgr::Get().GetAddon(addon->ID(), localAddon, ADDON_SERVICE))
    std::static_pointer_cast<CService>(localAddon)->Stop();

  if (CAddonMgr::Get().GetAddon(addon->ID(), localAddon, ADDON_CONTEXT_ITEM))
    CContextMenuManager::Get().Unregister(std::static_pointer_cast<CContextItemAddon>(localAddon));

  addon->OnPreUnInstall();
}

void OnPostUnInstall(const AddonPtr& addon)
{
  addon->OnPostUnInstall();
}


/**
 * CAddonLibrary
 *
 */

CAddonLibrary::CAddonLibrary(const cp_extension_t *ext)
  : CAddon(ext)
  , m_addonType(SetAddonType())
{
}

CAddonLibrary::CAddonLibrary(const AddonProps& props)
  : CAddon(props)
  , m_addonType(SetAddonType())
{
}

AddonPtr CAddonLibrary::Clone() const
{
  return AddonPtr(new CAddonLibrary(*this));
}

TYPE CAddonLibrary::SetAddonType()
{
  if (Type() == ADDON_VIZ_LIBRARY)
    return ADDON_VIZ;
  else
    return ADDON_UNKNOWN;
}

} /* namespace ADDON */

