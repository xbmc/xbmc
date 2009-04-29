/*
*      Copyright (C) 2005-2008 Team XBMC
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
#include "Settings.h"
#include "settings/AddonSettings.h"
#include "PVRManager.h"
#include "Util.h"
#include "URL.h"

namespace ADDON
{

CAddon::CAddon()
{
  m_guid = "";
  m_addonType = ADDON_UNKNOWN;
  m_strPath = "";
  m_disabled = false;
  m_stars = -1;
  m_strVersion = "";
  m_strName = "";
  m_summary = "";
  m_strDesc = "";
  m_disclaimer = "";
  m_strLibName = "";
}

bool CAddon::operator==(const CAddon &rhs) const {
  return (m_guid == rhs.m_guid);
}

void CAddon::LoadAddonStrings(const CURL &url)
{
  // Path where the addon resides
  CStdString pathToAddon;
  
  //TODO fix all Addon paths
  if (url.GetProtocol() == "plugin")
    pathToAddon = "special://home/plugins/";
  else
    pathToAddon = "special://xbmc/";

  // Build the addon's path
  CUtil::AddFileToFolder(pathToAddon, url.GetHostName(), pathToAddon);
  CUtil::AddFileToFolder(pathToAddon, url.GetFileName(), pathToAddon);

  // Path where the language strings reside
  CStdString pathToLanguageFile = pathToAddon;
  CStdString pathToFallbackLanguageFile = pathToAddon;
  CUtil::AddFileToFolder(pathToLanguageFile, "resources", pathToLanguageFile);
  CUtil::AddFileToFolder(pathToFallbackLanguageFile, "resources", pathToFallbackLanguageFile);
  CUtil::AddFileToFolder(pathToLanguageFile, "language", pathToLanguageFile);
  CUtil::AddFileToFolder(pathToFallbackLanguageFile, "language", pathToFallbackLanguageFile);
  CUtil::AddFileToFolder(pathToLanguageFile, g_guiSettings.GetString("locale.language"), pathToLanguageFile);
  CUtil::AddFileToFolder(pathToFallbackLanguageFile, "english", pathToFallbackLanguageFile);
  CUtil::AddFileToFolder(pathToLanguageFile, "strings.xml", pathToLanguageFile);
  CUtil::AddFileToFolder(pathToFallbackLanguageFile, "strings.xml", pathToFallbackLanguageFile);

  // Load language strings temporarily
  g_localizeStringsTemp.Load(pathToLanguageFile, pathToFallbackLanguageFile);
}

void CAddon::ClearAddonStrings()
{
  // Unload temporary language strings
  g_localizeStringsTemp.Clear();
}

void CAddon::TransferAddonSettings(const CURL &url)
{
  // Path where the addon resides
  CStdString pathToAddon = "addon://";

  // Build the addon's path
  CUtil::AddFileToFolder(pathToAddon, url.GetHostName(), pathToAddon);
  CUtil::AddFileToFolder(pathToAddon, url.GetFileName(), pathToAddon);

  CAddon addon;
  if (g_settings.AddonFromInfoXML(pathToAddon, addon))
  {
    addon.m_strPath = pathToAddon;
    TransferAddonSettings(&addon);
  }
  else
  {
    CLog::Log(LOGERROR, "Unknown URL %s to transfer AddOn Settings", pathToAddon.c_str());
  }
}

void CAddon::TransferAddonSettings(const CAddon* addon)
{
  CLog::Log(LOGDEBUG, "Calling TransferAddonSettings for: %s", addon->m_strName.c_str());
  
  if (addon == NULL)
    return;
        
  /* Transmit current unified user settings to the PVR Addon */
  ADDON::IAddonCallback* addonCB = GetCallbackForType(addon->m_addonType);

  CAddonSettings settings;
  settings.Load(addon->m_strPath);

  TiXmlElement *setting = settings.GetAddonRoot()->FirstChildElement("setting");
  while (setting)
  {
    const char *id = setting->Attribute("id");
    const char *type = setting->Attribute("type");
        
    if (type)
    {
      if (strcmpi(type, "text") == 0 || strcmpi(type, "ipaddress") == 0 ||
          strcmpi(type, "folder") == 0 || strcmpi(type, "action") == 0 ||
          strcmpi(type, "music") == 0 || strcmpi(type, "pictures") == 0 ||
          strcmpi(type, "folder") == 0 || strcmpi(type, "programs") == 0 ||
          strcmpi(type, "files") == 0 || strcmpi(type, "fileenum") == 0)
      {
        addonCB->SetSetting(addon, id, (const char*) settings.Get(id).c_str());
      }
      else if (strcmpi(type, "integer") == 0 || strcmpi(type, "enum") == 0 ||
               strcmpi(type, "labelenum") == 0)
      {
        int tmp = atoi(settings.Get(id));
        addonCB->SetSetting(addon, id, (int*) &tmp);
      }
      else if (strcmpi(type, "bool") == 0)
      {
        bool tmp = settings.Get(id) == "true" ? true : false;
        addonCB->SetSetting(addon, id, (bool*) &tmp);
      }
      else
      {
        CLog::Log(LOGERROR, "Unknown setting type '%s' for %s", type, addon->m_strName.c_str());
      }
    }
    setting = setting->NextSiblingElement("setting");
  }
}

IAddonCallback* CAddon::GetCallbackForType(AddonType type)
{
  switch (type)
  {
    case ADDON_MULTITYPE:
      return NULL;
    case ADDON_VIZ:
      return NULL;
    case ADDON_SKIN:
      return NULL;
    case ADDON_PVRDLL:
      return CPVRManager::GetInstance();
    case ADDON_SCRIPT:
      return NULL;
    case ADDON_SCRAPER:
      return NULL;
    case ADDON_SCREENSAVER:
      return NULL;
    case ADDON_PLUGIN_PVR:
      return NULL;
    case ADDON_PLUGIN_MUSIC:
      return NULL;
    case ADDON_PLUGIN_VIDEO:
      return NULL;
    case ADDON_PLUGIN_PROGRAM:
      return NULL;
    case ADDON_PLUGIN_PICTURES:
      return NULL;
    case ADDON_UNKNOWN:
    default:
      return NULL;
  }
}

} /* namespace ADDON */

