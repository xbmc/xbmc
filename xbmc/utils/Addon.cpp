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
#include "log.h"

namespace ADDON
{

const CStdString TranslateContent(const CONTENT_TYPE &type, bool pretty/*=false*/)
{
  switch (type)
  {
  case CONTENT_ALBUMS:
    {
      if (pretty)
        return g_localizeStrings.Get(132);
      return "albums";
    }
  case CONTENT_ARTISTS:
    {
      if (pretty)
        return g_localizeStrings.Get(133);
      return "artists";
    }
  case CONTENT_MOVIES:
    {
      if (pretty)
        return g_localizeStrings.Get(20342);
      return "movies";
    }
  case CONTENT_TVSHOWS:
    {
      if (pretty)
        return g_localizeStrings.Get(20343);
      return "tvshows";
    }
  case CONTENT_MUSICVIDEOS:
    {
      if (pretty)
        return g_localizeStrings.Get(20389);
      return "musicvideos";
    }
  case CONTENT_EPISODES:
    {
      if (pretty)
        return g_localizeStrings.Get(20360);
      return "episodes";
    }
  case CONTENT_NONE:
    {
      if (pretty)
        return g_localizeStrings.Get(231);
      return "";
    }
  default:
    {
      return "";
    }
  }
}

const CONTENT_TYPE TranslateContent(const CStdString &string)
{
  if (string.Equals("albums")) return CONTENT_ALBUMS;
  else if (string.Equals("artists")) return CONTENT_ARTISTS;
  else if (string.Equals("movies")) return CONTENT_MOVIES;
  else if (string.Equals("tvshows")) return CONTENT_TVSHOWS;
  else if (string.Equals("episodes")) return CONTENT_EPISODES;
  else if (string.Equals("musicvideos")) return CONTENT_MUSICVIDEOS;
  else if (string.Equals("plugin")) return CONTENT_PLUGIN;
  else if (string.Equals("weather")) return CONTENT_WEATHER;
  else return CONTENT_NONE;
}

const CStdString TranslateType(const ADDON::TYPE &type, bool pretty/*=false*/)
{
  switch (type)
  {
    case ADDON::ADDON_PVRDLL:
    {
      if (pretty)
        return g_localizeStrings.Get(23015);
      return "pvrclient";
    }
    case ADDON::ADDON_SCRAPER:
    {
      if (pretty)
        return g_localizeStrings.Get(21416);
      return "scraper";
    }
    case ADDON::ADDON_SCRAPER_LIBRARY:
    {
      return "scraper-library";
    }
    case ADDON::ADDON_SCREENSAVER:
    {
      if (pretty)
        return g_localizeStrings.Get(23021);
      return "screensaver";
    }
    case ADDON::ADDON_VIZ:
    {
      if (pretty)
        return g_localizeStrings.Get(23013);
      return "visualization";
    }
    case ADDON::ADDON_PLUGIN:
    {
      if (pretty)
        return g_localizeStrings.Get(23029);
      return "plugin";
    }
    case ADDON::ADDON_SCRIPT:
    {
      if (pretty)
        return g_localizeStrings.Get(23016);
      return "script";
    }
    default:
    {
      return "";
    }
  }
}

const ADDON::TYPE TranslateType(const CStdString &string)
{
  if (string.Equals("pvrclient")) return ADDON_PVRDLL;
  else if (string.Equals("scraper")) return ADDON_SCRAPER;
  else if (string.Equals("scraper-library")) return ADDON_SCRAPER_LIBRARY;
  else if (string.Equals("screensaver")) return ADDON_SCREENSAVER;
  else if (string.Equals("visualization")) return ADDON_VIZ;
  else if (string.Equals("plugin")) return ADDON_PLUGIN;
  else if (string.Equals("script")) return ADDON_SCRIPT;
  else return ADDON_UNKNOWN;
}

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
  // easy compare two integer revisions
  if (!str.Find('.') && !rhs.str.Find('.'))
    return (atoi(str) > atoi(rhs.str));

  return false;
}

bool AddonVersion::operator>=(const AddonVersion &rhs) const
{
  return (*this == rhs) || (*this > rhs);
}

bool AddonVersion::operator<(const AddonVersion &rhs) const
{
  return !(*this == rhs) && !(*this > rhs);
}

bool AddonVersion::operator<=(const AddonVersion &rhs) const
{
  return (*this == rhs) || !(*this > rhs);
}

std::ostream& AddonVersion::operator<<(std::ostream& out) const
{
  return out << str;
}

CAddon::CAddon(const AddonProps &props)
  : m_props(props)
  , m_parent(AddonPtr()) // null parent AddonPtr as default
{
  if (props.libname.empty()) BuildLibName();
  else m_strLibName = props.libname;
  m_userSettingsPath = GetUserSettingsPath();
}

CAddon::CAddon(const CAddon &rhs)
  : m_props(rhs.Props())
  , m_parent(const_cast<CAddon*>(&rhs))
{
  //m_uuid(StringUtils::CreateUUID())
  m_userXmlDoc  = rhs.m_userXmlDoc;
  m_strProfile  = GetProfilePath();
  m_disabled    = false;
  m_strLibName  = rhs.LibName();
  m_userSettingsPath = GetUserSettingsPath();
}

AddonPtr CAddon::Clone() const
{
  return AddonPtr(new CAddon(*this));
}

const AddonVersion CAddon::Version()
{
  return m_props.version;
}

//TODO platform/path crap should be negotiated between the addon and
// the handler for it's type
void CAddon::BuildLibName()
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
  case ADDON_VIZ:
    ext = ADDON_VIS_EXT;
    break;
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

/*
* Language File Handling
*/
bool CAddon::LoadStrings()
{
  if (!HasSettings())
    return false;

  // Path where the language strings reside
  CStdString pathToLanguageFile = m_props.path;
  CStdString pathToFallbackLanguageFile = m_props.path;
  CUtil::AddFileToFolder(pathToLanguageFile, "resources", pathToLanguageFile);
  CUtil::AddFileToFolder(pathToFallbackLanguageFile, "resources", pathToFallbackLanguageFile);
  CUtil::AddFileToFolder(pathToLanguageFile, "language", pathToLanguageFile);
  CUtil::AddFileToFolder(pathToFallbackLanguageFile, "language", pathToFallbackLanguageFile);
  CUtil::AddFileToFolder(pathToLanguageFile, g_guiSettings.GetString("locale.language"), pathToLanguageFile);
  CUtil::AddFileToFolder(pathToFallbackLanguageFile, "english", pathToFallbackLanguageFile);
  CUtil::AddFileToFolder(pathToLanguageFile, "strings.xml", pathToLanguageFile);
  CUtil::AddFileToFolder(pathToFallbackLanguageFile, "strings.xml", pathToFallbackLanguageFile);

  // Load language strings temporarily
  return m_strings.Load(pathToLanguageFile, pathToFallbackLanguageFile);
}

void CAddon::ClearStrings()
{
  // Unload temporary language strings
  m_strings.Clear();
}

CStdString CAddon::GetString(uint32_t id) const
{
  return m_strings.Get(id);
}

/*
* Settings Handling
*/
bool CAddon::HasSettings()
{
  CStdString addonFileName = m_props.path;
  CUtil::AddFileToFolder(addonFileName, "resources", addonFileName);
  CUtil::AddFileToFolder(addonFileName, "settings.xml", addonFileName);

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
  CStdString addonFileName = m_props.path;
  CUtil::AddFileToFolder(addonFileName, "resources", addonFileName);
  CUtil::AddFileToFolder(addonFileName, "settings.xml", addonFileName);

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

bool CAddon::LoadUserSettings()
{
  // Load the user saved settings. If it does not exist, create it
  if (!m_userXmlDoc.LoadFile(m_userSettingsPath))
  {
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
  if (!DIRECTORY::CDirectory::Exists(strRoot))
    DIRECTORY::CDirectory::Create(strRoot);
  if (!DIRECTORY::CDirectory::Exists(strAddon))
    DIRECTORY::CDirectory::Create(strAddon);

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
  nodeSetting.SetAttribute("id", std::string(key.c_str())); //FIXME otherwise attribute value isn't updated
  if (!type.empty())
    nodeSetting.SetAttribute("type", std::string(type.c_str()));
  else
    nodeSetting.SetAttribute("type", "text");
  nodeSetting.SetAttribute("value", std::string(value.c_str()));
  m_userXmlDoc.RootElement()->InsertEndChild(nodeSetting);
}

TiXmlElement* CAddon::GetSettingsXML()
{
  return m_addonXmlDoc.RootElement();
}

CStdString CAddon::GetProfilePath()
{
  CStdString profile;
  profile.Format("special://profile/addon_data/%s", UUID().c_str());
  return profile;
}

CStdString CAddon::GetUserSettingsPath()
{
  CStdString path;
  CUtil::AddFileToFolder(Profile(), "settings.xml", path);
  return path;
}

} /* namespace ADDON */

