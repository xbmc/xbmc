/*
 *      Copyright (C) 2005-2009 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "Addon.h"
#include "AddonManager.h"
#include "Settings.h"
#include "GUISettings.h"
#include "StringUtils.h"
#include "FileSystem/Directory.h"
#include "FileSystem/File.h"
#ifdef __APPLE__
#include "../osx/OSXGNUReplacements.h"
#endif
#include "log.h"
#include <vector>
#include <string.h>

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
  const char*  name;
  CONTENT_TYPE type;
  int          pretty;
} ContentMapping;

static const ContentMapping content[] =
  {{"unknown",       CONTENT_NONE,          231 },
   {"albums",        CONTENT_ALBUMS,        132 },
   {"music",         CONTENT_ALBUMS,        132 },
   {"artists",       CONTENT_ARTISTS,       133 },
   {"movies",        CONTENT_MOVIES,      20342 },
   {"tvshows",       CONTENT_TVSHOWS,     20343 },
   {"episodes",      CONTENT_EPISODES,    20360 },
   {"musicvideos",   CONTENT_MUSICVIDEOS, 20389 },
   {"audio",         CONTENT_AUDIO,           0 },
   {"image",         CONTENT_IMAGE,           0 },
   {"program",       CONTENT_PROGRAM,         0 },
   {"video",         CONTENT_VIDEO,           0 }};

typedef struct
{
  const char* name;
  TYPE        type;
  int         pretty;
} TypeMapping;

static const TypeMapping types[] =
  {{"unknown",                       ADDON_UNKNOWN,            0 },
   {"xbmc.metadata.scraper",         ADDON_SCRAPER,        24007 },
   {"xbmc.metadata.scraper.library", ADDON_SCRAPER_LIBRARY,    0 },
   {"xbmc.ui.screensaver",           ADDON_SCREENSAVER,    24008 },
   {"xbmc.player.musicviz",          ADDON_VIZ,            24010 },
   {"visualization-library",         ADDON_VIZ_LIBRARY,        0 },
   {"xbmc.python.plugin",            ADDON_PLUGIN,         24005 },
   {"xbmc.python.script",            ADDON_SCRIPT,         24009 },
   {"xbmc.python.weather",           ADDON_SCRIPT_WEATHER,   24027 },
   {"xbmc.python.subtitles",         ADDON_SCRIPT_SUBTITLES, 24012 },
   {"xbmc.python.lyrics",            ADDON_SCRIPT_LYRICS,    24013 },
   {"xbmc.python.library",           ADDON_SCRIPT_LIBRARY,   24014 },
   {"xbmc.gui.skin",                 ADDON_SKIN,             166 },
   {"xbmc.addon.repository",         ADDON_REPOSITORY,     24011 },
   {"pvrclient",                     ADDON_PVRDLL,             0 }};

const CStdString TranslateContent(const CONTENT_TYPE &type, bool pretty/*=false*/)
{
  for (unsigned int index=0; index < sizeof(content)/sizeof(content[0]); ++index)
  {
    const ContentMapping &map = content[index];
    if (type == map.type)
      if (pretty && map.pretty)
        return g_localizeStrings.Get(map.pretty);
      else
        return map.name;
  }
  return "";
}

const CONTENT_TYPE TranslateContent(const CStdString &string)
{
  for (unsigned int index=0; index < sizeof(content)/sizeof(content[0]); ++index)
  {
    const ContentMapping &map = content[index];
    if (string.Equals(map.name))
      return map.type;
  }
  return CONTENT_NONE;
}

const CStdString TranslateType(const ADDON::TYPE &type, bool pretty/*=false*/)
{
  for (unsigned int index=0; index < sizeof(types)/sizeof(types[0]); ++index)
  {
    const TypeMapping &map = types[index];
    if (type == map.type)
      if (pretty && map.pretty)
        return g_localizeStrings.Get(map.pretty);
      else
        return map.name;
  }
  return "";
}

const TYPE TranslateType(const CStdString &string)
{
  for (unsigned int index=0; index < sizeof(types)/sizeof(types[0]); ++index)
  {
    const TypeMapping &map = types[index];
    if (string.Equals(map.name))
      return map.type;
  }
  return ADDON_UNKNOWN;
}

/**
 * AddonVersion
 *
 */

bool AddonVersion::operator==(const AddonVersion &rhs) const
{
  return str.Equals(rhs.str);
}

bool AddonVersion::operator!=(const AddonVersion &rhs) const
{
  return !(*this == rhs);
}

bool AddonVersion::operator>(const AddonVersion &rhs) const
{
  return (strverscmp(str.c_str(), rhs.str.c_str()) > 0);
}

bool AddonVersion::operator>=(const AddonVersion &rhs) const
{
  return (*this == rhs) || (*this > rhs);
}

bool AddonVersion::operator<(const AddonVersion &rhs) const
{
  return (strverscmp(str.c_str(), rhs.str.c_str()) < 0);
}

bool AddonVersion::operator<=(const AddonVersion &rhs) const
{
  return (*this == rhs) || !(*this > rhs);
}

CStdString AddonVersion::Print() const
{
  CStdString out;
  out.Format("%s %s", g_localizeStrings.Get(24051), str); // "Version <str>"
  return CStdString(out);
}

AddonProps::AddonProps(cp_plugin_info_t *props)
  : id(props->identifier)
  , version(props->version)
  , name(props->name)
  , path(props->plugin_path)
  , author(props->provider_name)
  , stars(0)
{
  //FIXME only considers the first registered extension for each addon
  if (props->extensions->ext_point_id)
    type = TranslateType(props->extensions->ext_point_id);
  // Grab more detail from the props...
  const cp_extension_t *metadata = CAddonMgr::Get().GetExtension(props, "xbmc.addon.metadata");
  if (metadata)
  {
    CStdString platforms = CAddonMgr::Get().GetExtValue(metadata->configuration, "platform");
    summary = CAddonMgr::Get().GetTranslatedString(metadata->configuration, "summary");
    description = CAddonMgr::Get().GetTranslatedString(metadata->configuration, "description");
    disclaimer = CAddonMgr::Get().GetTranslatedString(metadata->configuration, "disclaimer");
    license = CAddonMgr::Get().GetExtValue(metadata->configuration, "license");
    vector<CStdString> content = CAddonMgr::Get().GetExtValues(metadata->configuration,"supportedcontent");
    for (unsigned int i=0;i<content.size();++i)
      contents.insert(TranslateContent(content[i]));
    //FIXME other stuff goes here
    //CStdString version = CAddonMgr::Get().GetExtValue(metadata->configuration, "minversion/xbmc");
  }
  icon = "icon.png";
  fanart = CUtil::AddFileToFolder(path, "fanart.jpg");
  changelog = CUtil::AddFileToFolder(path, "changelog.txt");
}

/**
 * CAddon
 *
 */

CAddon::CAddon(cp_plugin_info_t *props)
  : m_props(props)
  , m_parent(AddonPtr())
{
  BuildLibName(props);
  BuildProfilePath();
  CUtil::AddFileToFolder(Profile(), "settings.xml", m_userSettingsPath);
  m_enabled = true;
  m_hasStrings = false;
  m_checkedStrings = false;
}

CAddon::CAddon(const AddonProps &props)
  : m_props(props)
  , m_parent(AddonPtr())
{
  if (props.libname.IsEmpty()) BuildLibName();
  else m_strLibName = props.libname;
  BuildProfilePath();
  CUtil::AddFileToFolder(Profile(), "settings.xml", m_userSettingsPath);
  m_enabled = true;
  m_hasStrings = false;
  m_checkedStrings = false;
}

CAddon::CAddon(const CAddon &rhs, const AddonPtr &parent)
  : m_props(rhs.Props())
  , m_parent(parent)
{
  m_userXmlDoc  = rhs.m_userXmlDoc;
  BuildProfilePath();
  CUtil::AddFileToFolder(Profile(), "settings.xml", m_userSettingsPath);
  m_strLibName  = rhs.LibName();
  m_enabled = rhs.Enabled();
  m_hasStrings  = false;
  m_checkedStrings  = false;
}

AddonPtr CAddon::Clone(const AddonPtr &self) const
{
  return AddonPtr(new CAddon(*this, self));
}

const AddonVersion CAddon::Version()
{
  return m_props.version;
}

//TODO platform/path crap should be negotiated between the addon and
// the handler for it's type
void CAddon::BuildLibName(cp_plugin_info_t *props)
{
  if (!props)
  {
    m_strLibName = "default";
    CStdString ext;
    switch (m_props.type)
    {
    case ADDON_SCRAPER:
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
    case ADDON_SCRIPT:
    case ADDON_SCRIPT_LIBRARY:
    case ADDON_SCRIPT_LYRICS:
    case ADDON_SCRIPT_WEATHER:
    case ADDON_SCRIPT_SUBTITLES:
    case ADDON_PLUGIN:
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
      case ADDON_SCRIPT_SUBTITLES:
      case ADDON_SCRAPER:
      case ADDON_SCRAPER_LIBRARY:
      case ADDON_PLUGIN:
        {
          CStdString temp = CAddonMgr::Get().GetExtValue(props->extensions->configuration, "@library");
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
  CStdString chosenPath;
  chosenPath.Format("resources/language/%s/strings.xml", g_guiSettings.GetString("locale.language").c_str());
  CStdString chosen = CUtil::AddFileToFolder(m_props.path, chosenPath);
  CStdString fallback = CUtil::AddFileToFolder(m_props.path, "resources/language/English/strings.xml");

  m_hasStrings = m_strings.Load(chosen, fallback);
  return m_checkedStrings = true;
}

void CAddon::ClearStrings()
{
  // Unload temporary language strings
  m_strings.Clear();
  m_hasStrings = false;
}

CStdString CAddon::GetString(uint32_t id)
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
  CStdString addonFileName = CUtil::AddFileToFolder(m_props.path, "resources/settings.xml");

  // Load the settings file to verify it's valid
  TiXmlDocument xmlDoc;
  if (!xmlDoc.LoadFile(addonFileName))
    return false;

  // Make sure that the addon XML has the settings element
  TiXmlElement *setting = xmlDoc.RootElement();
  if (!setting || strcmpi(setting->Value(), "settings") != 0)
    return false;

  return true;
}

bool CAddon::LoadSettings()
{
  CStdString addonFileName = CUtil::AddFileToFolder(m_props.path, "resources/settings.xml");

  if (!m_addonXmlDoc.LoadFile(addonFileName))
  {
    CLog::Log(LOGERROR, "Unable to load: %s, Line %d\n%s", addonFileName.c_str(), m_addonXmlDoc.ErrorRow(), m_addonXmlDoc.ErrorDesc());
    return false;
  }

  // Make sure that the addon XML has the settings element
  TiXmlElement *setting = m_addonXmlDoc.RootElement();
  if (!setting || strcmpi(setting->Value(), "settings") != 0)
  {
    CLog::Log(LOGERROR, "Error loading Settings %s: cannot find root element 'settings'", addonFileName.c_str());
    return false;
  }
  return LoadUserSettings();
}

bool CAddon::LoadUserSettings(bool create)
{
  // Load the user saved settings. If it does not exist, create it
  if (!m_userXmlDoc.LoadFile(m_userSettingsPath))
  {
    if (!create)
      return false;

    TiXmlDocument doc;
    TiXmlDeclaration decl("1.0", "UTF-8", "yes");
    doc.InsertEndChild(decl);

    TiXmlElement xmlRootElement("settings");
    doc.InsertEndChild(xmlRootElement);

    m_userXmlDoc = doc;
  }

  return true;
}

void CAddon::SaveSettings(void)
{
  // break down the path into directories
  CStdString strRoot, strAddon;
  CUtil::GetDirectory(m_userSettingsPath, strAddon);
  CUtil::RemoveSlashAtEnd(strAddon);
  CUtil::GetDirectory(strAddon, strRoot);
  CUtil::RemoveSlashAtEnd(strRoot);

  // create the individual folders
  if (!CDirectory::Exists(strRoot))
    CDirectory::Create(strRoot);
  if (!CDirectory::Exists(strAddon))
    CDirectory::Create(strAddon);

  m_userXmlDoc.SaveFile(m_userSettingsPath);
}

void CAddon::SaveFromDefault()
{
  if (!GetSettingsXML())
  { // no settings found
    return;
  }

  const TiXmlElement *setting = GetSettingsXML()->FirstChildElement("setting");
  while (setting)
  {
    CStdString id;
    if (setting->Attribute("id"))
      id = setting->Attribute("id");
    CStdString type;
    if (setting->Attribute("type"))
      type = setting->Attribute("type");
    CStdString value;
    if (setting->Attribute("default"))
      value = setting->Attribute("default");
    UpdateSetting(id, type, value);
    setting = setting->NextSiblingElement("setting");
  }

  // now save to file
  SaveSettings();
}

CStdString CAddon::GetSetting(const CStdString& key) const
{
  if (m_userXmlDoc.RootElement())
  {
    // Try to find the setting and return its value
    const TiXmlElement *setting = m_userXmlDoc.RootElement()->FirstChildElement("setting");
    while (setting)
    {
      const char *id = setting->Attribute("id");
      if (id && strcmpi(id, key) == 0)
        return setting->Attribute("value");

      setting = setting->NextSiblingElement("setting");
    }
  }

  if (m_addonXmlDoc.RootElement())
  {
    // Try to find the setting in the addon and return its default value
    const TiXmlElement* setting = m_addonXmlDoc.RootElement()->FirstChildElement("setting");
    while (setting)
    {
      const char *id = setting->Attribute("id");
      if (id && strcmpi(id, key) == 0 && setting->Attribute("default"))
        return setting->Attribute("default");

      setting = setting->NextSiblingElement("setting");
    }
  }

  // Otherwise return empty string
  return "";
}

void CAddon::UpdateSetting(const CStdString& key, const CStdString& value, const CStdString& type/* = "" */)
{
  if (key.empty()) return;

  // Try to find the setting and change its value
  if (!m_userXmlDoc.RootElement())
  {
    TiXmlElement node("settings");
    m_userXmlDoc.InsertEndChild(node);
  }
  TiXmlElement *setting = m_userXmlDoc.RootElement()->FirstChildElement("setting");
  while (setting)
  {
    const char *id = setting->Attribute("id");
    const char *storedtype = setting->Attribute("type");
    if (id && strcmpi(id, key) == 0)
    {
      if (!type.empty() && storedtype && strcmpi(storedtype, type) != 0)
        setting->SetAttribute("type", type.c_str());

      setting->SetAttribute("value", value.c_str());
      return;
    }
    setting = setting->NextSiblingElement("setting");
  }

  // Setting not found, add it
  TiXmlElement nodeSetting("setting");
  nodeSetting.SetAttribute("id", key.c_str()); //FIXME otherwise attribute value isn't updated
  if (!type.empty())
    nodeSetting.SetAttribute("type", type.c_str());
  else
    nodeSetting.SetAttribute("type", "text");
  nodeSetting.SetAttribute("value", value.c_str());
  m_userXmlDoc.RootElement()->InsertEndChild(nodeSetting);
}

TiXmlElement* CAddon::GetSettingsXML()
{
  return m_addonXmlDoc.RootElement();
}

void CAddon::BuildProfilePath()
{
  m_profile.Format("special://profile/addon_data/%s/", ID().c_str());
}

const CStdString CAddon::Icon() const
{
  if (CURL::IsFullPath(m_props.icon))
    return m_props.icon;
  return CUtil::AddFileToFolder(m_props.path, m_props.icon);
}

ADDONDEPS CAddon::GetDeps()
{
  return CAddonMgr::Get().GetDeps(ID());
}

/**
 * CAddonLibrary
 *
 */

CAddonLibrary::CAddonLibrary(cp_plugin_info_t *props)
  : CAddon(props)
  , m_addonType(SetAddonType())
{
}

CAddonLibrary::CAddonLibrary(const AddonProps& props)
  : CAddon(props)
  , m_addonType(SetAddonType())
{
}

TYPE CAddonLibrary::SetAddonType()
{
  if (Type() == ADDON_SCRAPER_LIBRARY)
    return ADDON_SCRAPER;
  else if (Type() == ADDON_VIZ_LIBRARY)
    return ADDON_VIZ;
  else
    return ADDON_UNKNOWN;
}

} /* namespace ADDON */

