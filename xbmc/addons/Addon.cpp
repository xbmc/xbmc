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

#include <string.h>
#include <ostream>
#include <utility>
#include <vector>

#include "AddonManager.h"
#include "addons/Service.h"
#include "ContextMenuManager.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "filesystem/SpecialProtocol.h"
#include "RepositoryUpdater.h"
#include "settings/Settings.h"
#include "ServiceBroker.h"
#include "system.h"
#include "URL.h"
#include "Util.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"

#ifdef HAS_PYTHON
#include "interfaces/python/XBPython.h"
#endif
#if defined(TARGET_DARWIN)
#include "../platform/darwin/OSXGNUReplacements.h"
#endif
#ifdef TARGET_FREEBSD
#include "freebsd/FreeBSDGNUReplacements.h"
#endif

using XFILE::CDirectory;
using XFILE::CFile;

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
   {"xbmc.pvrclient",                    ADDON_PVRDLL,              24019, "DefaultAddonPVRClient.png" },
   {"kodi.gameclient",                   ADDON_GAMEDLL,             35049, "DefaultAddonGame.png" },
   {"kodi.peripheral",                   ADDON_PERIPHERALDLL,       35010, "DefaultAddonPeripheral.png" },
   {"xbmc.addon.video",                  ADDON_VIDEO,                1037, "DefaultAddonVideo.png" },
   {"xbmc.addon.audio",                  ADDON_AUDIO,                1038, "DefaultAddonMusic.png" },
   {"xbmc.addon.image",                  ADDON_IMAGE,                1039, "DefaultAddonPicture.png" },
   {"xbmc.addon.executable",             ADDON_EXECUTABLE,           1043, "DefaultAddonProgram.png" },
   {"kodi.addon.game",                   ADDON_GAME,                35049, "DefaultAddonGame.png" },
   {"xbmc.audioencoder",                 ADDON_AUDIOENCODER,         200,  "DefaultAddonAudioEncoder.png" },
   {"kodi.audiodecoder",                 ADDON_AUDIODECODER,         201,  "DefaultAddonAudioDecoder.png" },
   {"xbmc.service",                      ADDON_SERVICE,             24018, "DefaultAddonService.png" },
   {"kodi.resource.images",              ADDON_RESOURCE_IMAGES,     24035, "DefaultAddonImages.png" },
   {"kodi.resource.language",            ADDON_RESOURCE_LANGUAGE,   24026, "DefaultAddonLanguage.png" },
   {"kodi.resource.uisounds",            ADDON_RESOURCE_UISOUNDS,   24006, "DefaultAddonUISounds.png" },
   {"kodi.resource.games",               ADDON_RESOURCE_GAMES,      35209, "DefaultAddonGame.png" },
   {"kodi.adsp",                         ADDON_ADSPDLL,             24135, "DefaultAddonAudioDSP.png" },
   {"kodi.inputstream",                  ADDON_INPUTSTREAM,         24048, "DefaultAddonInputstream.png" },
  };

std::string TranslateType(ADDON::TYPE type, bool pretty/*=false*/)
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

std::string GetIcon(ADDON::TYPE type)
{
  for (unsigned int index=0; index < ARRAY_SIZE(types); ++index)
  {
    const TypeMapping &map = types[index];
    if (type == map.type)
      return map.icon;
  }
  return "";
}

CAddon::CAddon(AddonProps props)
  : m_props(std::move(props))
{
  m_profilePath = StringUtils::Format("special://profile/addon_data/%s/", ID().c_str());
  m_userSettingsPath = URIUtils::AddFileToFolder(m_profilePath, "settings.xml");
  m_hasSettings = true;
  m_settingsLoaded = false;
  m_userSettingsLoaded = false;
}

bool CAddon::MeetsVersion(const AddonVersion &version) const
{
  return m_props.minversion <= version && version <= m_props.version;
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
  std::string addonFileName = URIUtils::AddFileToFolder(m_props.path, "resources", "settings.xml");

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

bool CAddon::HasSettingsToSave() const
{
  return !m_settings.empty();
}

void CAddon::SaveSettings(void)
{
  if (!HasSettingsToSave())
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
  
  CAddonMgr::GetInstance().ReloadSettings(ID());//push the settings changes to the running addon instance
#ifdef HAS_PYTHON
  g_pythonParser.OnSettingsChanged(ID());
#endif
}

std::string CAddon::GetSetting(const std::string& key)
{
  if (!LoadSettings())
    return ""; // no settings available

  std::map<std::string, std::string>::const_iterator i = m_settings.find(key);
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
  for (std::map<std::string, std::string>::const_iterator i = m_settings.begin(); i != m_settings.end(); ++i)
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

std::string CAddon::LibPath() const
{
  if (m_props.libname.empty())
    return "";

  std::string strLibPath = URIUtils::AddFileToFolder(m_props.path, m_props.libname);

  // Check if add-on library has been installed to the binaries path instead
  std::string strSharePath = CSpecialProtocol::TranslatePath("special://xbmc/");
  const bool bIsInSharePath = StringUtils::StartsWith(strLibPath, strSharePath);
  if (bIsInSharePath && !CFile::Exists(strLibPath))
  {
    std::string strBinPath = CSpecialProtocol::TranslatePath("special://xbmcbin/");
    strLibPath.replace(0, strSharePath.length(), strBinPath);
  }

  return strLibPath;
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
  if (CAddonMgr::GetInstance().GetAddon(id, addon, ADDON_PVRDLL) ||
      CAddonMgr::GetInstance().GetAddon(id, addon, ADDON_ADSPDLL) ||
      CAddonMgr::GetInstance().GetAddon(id, addon, ADDON_PERIPHERALDLL))
    return addon->OnEnabled();

  if (CAddonMgr::GetInstance().ServicesHasStarted())
  {
    if (CAddonMgr::GetInstance().GetAddon(id, addon, ADDON_SERVICE))
      std::static_pointer_cast<CService>(addon)->Start();
  }

  if (CAddonMgr::GetInstance().GetAddon(id, addon, ADDON_REPOSITORY))
    CRepositoryUpdater::GetInstance().ScheduleUpdate(); //notify updater there is a new addon
}

void OnDisabled(const std::string& id)
{

  AddonPtr addon;
  if (CAddonMgr::GetInstance().GetAddon(id, addon, ADDON_PVRDLL, false) ||
      CAddonMgr::GetInstance().GetAddon(id, addon, ADDON_ADSPDLL, false) ||
      CAddonMgr::GetInstance().GetAddon(id, addon, ADDON_PERIPHERALDLL, false))
    return addon->OnDisabled();

  if (CAddonMgr::GetInstance().ServicesHasStarted())
  {
    if (CAddonMgr::GetInstance().GetAddon(id, addon, ADDON_SERVICE, false))
      std::static_pointer_cast<CService>(addon)->Stop();
  }

  if (CAddonMgr::GetInstance().GetAddon(id, addon, ADDON_CONTEXT_ITEM, false))
    CContextMenuManager::GetInstance().Unload(*std::static_pointer_cast<CContextMenuAddon>(addon));
}

void OnPreInstall(const AddonPtr& addon)
{
  //Before installing we need to stop/unregister any local addon
  //that have this id, regardless of what the 'new' addon is.
  AddonPtr localAddon;

  if (CAddonMgr::GetInstance().ServicesHasStarted())
  {
    if (CAddonMgr::GetInstance().GetAddon(addon->ID(), localAddon, ADDON_SERVICE))
      std::static_pointer_cast<CService>(localAddon)->Stop();
  }

  if (CAddonMgr::GetInstance().GetAddon(addon->ID(), localAddon, ADDON_CONTEXT_ITEM))
    CContextMenuManager::GetInstance().Unload(*std::static_pointer_cast<CContextMenuAddon>(localAddon));

  //Fallback to the pre-install callback in the addon.
  //! @bug If primary extension point have changed we're calling the wrong method.
  addon->OnPreInstall();
}

void OnPostInstall(const AddonPtr& addon, bool update, bool modal)
{
  AddonPtr localAddon;
  if (CAddonMgr::GetInstance().ServicesHasStarted())
  {
    if (CAddonMgr::GetInstance().GetAddon(addon->ID(), localAddon, ADDON_SERVICE))
      std::static_pointer_cast<CService>(localAddon)->Start();
  }

  if (CAddonMgr::GetInstance().GetAddon(addon->ID(), localAddon, ADDON_REPOSITORY))
    CRepositoryUpdater::GetInstance().ScheduleUpdate(); //notify updater there is a new addon or version

  addon->OnPostInstall(update, modal);
}

void OnPreUnInstall(const AddonPtr& addon)
{
  AddonPtr localAddon;

  if (CAddonMgr::GetInstance().ServicesHasStarted())
  {
    if (CAddonMgr::GetInstance().GetAddon(addon->ID(), localAddon, ADDON_SERVICE))
      std::static_pointer_cast<CService>(localAddon)->Stop();
  }

  if (CAddonMgr::GetInstance().GetAddon(addon->ID(), localAddon, ADDON_CONTEXT_ITEM))
    CContextMenuManager::GetInstance().Unload(*std::static_pointer_cast<CContextMenuAddon>(localAddon));

  addon->OnPreUnInstall();
}

void OnPostUnInstall(const AddonPtr& addon)
{
  addon->OnPostUnInstall();
}

} /* namespace ADDON */

