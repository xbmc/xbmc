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

#include "stdafx.h"
#include "Addon.h"

namespace ADDON
{

const CStdString TranslateContent(const CONTENT_TYPE &type)
{
  switch (type)
  {
  case CONTENT_ALBUMS:
    {
      return "albums";
    }
  case CONTENT_ARTISTS:
    {
      return "artists";
    }
  case CONTENT_MOVIES:
    {
      return "movies";
    }
  case CONTENT_TVSHOWS:
    {
      return "tvshows";
    }
  case CONTENT_MUSICVIDEOS:
    {
      return "musicvideos";
    }
  case CONTENT_EPISODES:
    {
      return "episodes";
    }
  case CONTENT_PLUGIN:
    {
      return "plugin";
    }
  case CONTENT_WEATHER:
    {
      return "weather";
    }
  default:
    {
      return "";
    }
  }
}

const CONTENT_TYPE TranslateContent(const CStdString &string)
{
  if (string == "albums")
    return CONTENT_ALBUMS;
  else if (string == "artists")
    return CONTENT_ARTISTS;
  else if (string == "movies")
    return CONTENT_MOVIES;
  else if (string == "tvshows")
    return CONTENT_TVSHOWS;
  else if (string == "episoes")
    return CONTENT_EPISODES;
  else if (string == "musicvideos")
    return CONTENT_MUSICVIDEOS;
  else if (string == "plugin")
    return CONTENT_PLUGIN;
  else if (string == "weather")
    return CONTENT_WEATHER;
  else
    return CONTENT_NONE;
}

const CStdString TranslateType(const ADDON::TYPE &type)
{
	switch (type)
	{
	case ADDON::ADDON_PVRDLL:
		{
			return "pvrclient";
		}
	case ADDON::ADDON_SCRAPER:
		{
			return "scraper";
		}
	case ADDON::ADDON_SCREENSAVER:
		{
			return "screensaver";
		}
	case ADDON::ADDON_VIZ:
		{
			return "visualisation";
		}
	case ADDON::ADDON_PLUGIN:
		{
			return "plugin";
		}
	default:
		{
			return "";
		}
	}
}

const ADDON::TYPE TranslateType(const CStdString &string)
{
	if (string == "pvrclient")
		return ADDON_PVRDLL;
	else if (string == "scraper")
		return ADDON_SCRAPER;
	else if (string == "screensaver")
		return ADDON_SCREENSAVER;
	else if (string == "visualisation")
		return ADDON_VIZ;
	else if (string == "plugin")
		return ADDON_PLUGIN;
	else if (string == "script")
		return ADDON_SCRIPT;
	else
		return ADDON_MULTITYPE;
}

CAddon::CAddon(const AddonProps &props)
: m_guid(props.uuid)
, m_guid_parent(props.parent)
, m_type(props.type)
, m_content(props.contents)
{
  m_strPath     = props.path;
  m_disabled    = true;
  m_icon        = props.icon;
  m_stars       = props.stars;
  m_strVersion  = props.version;
  m_strName     = props.name;
  m_summary     = props.summary;
  m_strDesc     = props.description;
  m_disclaimer  = props.disclaimer;
  m_strLibName  = props.libname;
  m_userSettingsPath = GetUserSettingsPath();
}

CAddon::CAddon(const CAddon &rhs)
: m_guid(CUtil::CreateUUID())
, m_guid_parent(rhs.UUID())
, m_type(rhs.Type())
{
  m_strPath     = rhs.Path();
  m_disabled    = true;
  m_stars       = rhs.Stars();
  m_strVersion  = rhs.Version();
  m_strName     = rhs.Name();
  m_summary     = rhs.Summary();
  m_strDesc     = rhs.Description();
  m_disclaimer  = rhs.Disclaimer();
  m_strLibName  = rhs.LibName();
  m_userSettingsPath = GetUserSettingsPath();
}

AddonPtr CAddon::Clone() const
{
  return AddonPtr(new CAddon(*this));
}

/*
* Language File Handling
*/
bool CAddon::LoadStrings()
{
  // Path where the language strings reside
  CStdString pathToLanguageFile = m_strPath;
  CStdString pathToFallbackLanguageFile = m_strPath;
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

CStdString CAddon::GetString(DWORD id) const
{
  return m_strings.Get(id);
}

/*
* Settings Handling
*/
bool CAddon::HasSettings()
{
  CStdString addonFileName = m_strPath;
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
  CStdString addonFileName = m_strPath;
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

  // Load the user saved settings. If it does not exist, create it
  if (!m_userXmlDoc.LoadFile(m_userSettingsPath))
  {
    TiXmlDocument doc;
    TiXmlDeclaration decl("1.0", "UTF-8", "yes");
    doc.InsertEndChild(decl);

    TiXmlElement xmlRootElement("settings");
    doc.InsertEndChild(xmlRootElement);

    m_userXmlDoc = doc;

    // Don't worry about the actual settings, they will be set when the user clicks "Ok"
    // in the settings dialog
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
  if (!GetSettingsXML()) //if scraper has no settings return false
    return;

  TiXmlElement *setting = GetSettingsXML()->FirstChildElement("setting");
  while (setting)
  {
    CStdString id;
    if (setting->Attribute("id"))
      id = setting->Attribute("id");
    CStdString value;
    if (setting->Attribute("default"))
      value = setting->Attribute("default");
    /* Set(id,value);*/
    setting = setting->NextSiblingElement("setting");
  }
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

void CAddon::UpdateSetting(const CStdString& key, const void* value)
{
  if (key == "") return;

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
    if (id && strcmpi(id, key) == 0)
    {
 /*     setting->SetAttribute("value", value);*/
      return;
    }

    setting = setting->NextSiblingElement("setting");
  }

  // Setting not found, add it
  TiXmlElement nodeSetting("setting");
  nodeSetting.SetAttribute("id", key.c_str());
  /*nodeSetting.SetAttribute("value", value);*/
  m_userXmlDoc.RootElement()->InsertEndChild(nodeSetting);
}

TiXmlElement* CAddon::GetSettingsXML()
{
  return m_addonXmlDoc.RootElement();
}

CStdString CAddon::GetUserSettingsPath()
{
  CStdString addonUserName;

  // and create the users filepath
  addonUserName.Format("special://profile/addon_data/%s", UUID().c_str());
  CUtil::AddFileToFolder(addonUserName, "settings.xml", addonUserName);
  return addonUserName;
}

} /* namespace ADDON */
