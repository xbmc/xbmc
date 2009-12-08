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

#include "Application.h"
#include "Addon.h"
#include "Settings.h"
#include "settings/AddonSettings.h"
#include "utils/log.h"
#include "LocalizeStrings.h"
#include "GUISettings.h"
#include "Util.h"

using namespace std;
using namespace XFILE;

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
      return "pvrclient";
    case ADDON::ADDON_SCRAPER:
      return "scraper";
    case ADDON::ADDON_SCREENSAVER:
      return "screensaver";
    case ADDON::ADDON_VIZ:
      return "visualisation";
    case ADDON::ADDON_PLUGIN:
      return "plugin";
    default:
      return "";
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

/**********************************************************
 * CAddon - AddOn Info and Helper Class
 *
 */

CAddon::CAddon()
{
  Reset();
}

void CAddon::Reset()
{
  m_guid        = "";
  m_guid_parent = "";
  m_type        = ADDON_MULTITYPE;
  m_strPath     = "";
  m_disabled    = false;
  m_stars       = -1;
  m_strVersion  = "";
  m_strName     = "";
  m_summary     = "";
  m_strDesc     = "";
  m_disclaimer  = "";
  m_strLibName  = "";
  m_childs      = 0;
}

void CAddon::Set(const AddonProps &props)
{
  m_type        = props.type;
  m_content     = props.contents;
  m_guid        = props.uuid;
  m_guid_parent = props.parent;
  m_strPath     = props.path;
//  m_strProfile  = GetProfilePath();
  m_disabled    = false;
  m_icon        = props.icon;
  m_stars       = props.stars;
  m_strVersion  = props.version;
  m_strName     = props.name;
  m_summary     = props.summary;
  m_strDesc     = props.description;
  m_disclaimer  = props.disclaimer;
  m_strLibName  = props.libname;
//  m_userSettingsPath = GetUserSettingsPath();
}

void CAddon::LoadAddonStrings(const CURL &url)
{
  // Path where the addon resides
  CStdString pathToAddon;
  if (url.GetProtocol() == "plugin")
  {
    pathToAddon = "special://home/plugins/";
    CUtil::AddFileToFolder(pathToAddon, url.GetHostName(), pathToAddon);
    CUtil::AddFileToFolder(pathToAddon, url.GetFileName(), pathToAddon);
  }
  else
    pathToAddon = url.Get();

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

bool CAddon::CreateChildAddon(const CAddon &parent, CAddon &child)
{
  if (parent.m_type != ADDON_PVRDLL)
  {
    CLog::Log(LOGERROR, "Can't create a child add-on for '%s' and type '%i', is not allowed for this type!", parent.m_strName.c_str(), parent.m_type);
    return false;
  }

  child = parent;
  child.m_guid_parent = parent.m_guid;
  child.m_guid = StringUtils::CreateUUID();

  VECADDONS *addons = g_addonmanager.GetAddonsFromType(parent.m_type);
  if (!addons) return false;

  for (IVECADDONS it = addons->begin(); it != addons->end(); it++)
  {
    if ((*it).m_guid == parent.m_guid)
    {
      (*it).m_childs++;
      child.m_strName.Format("%s #%i", child.m_strName.c_str(), (*it).m_childs);
      break;
    }
  }

  return true;
}

} /* namespace ADDON */

