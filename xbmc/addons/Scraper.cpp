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
#include "Scraper.h"
#include "XMLUtils.h"
#include "FileSystem/File.h"
#include "FileSystem/Directory.h"
#include "FileSystem/FileCurl.h"
#include "AddonManager.h"
#include "utils/ScraperParser.h"
#include "utils/ScraperUrl.h"
#include "utils/CharsetConverter.h"
#include "utils/log.h"

#include <sstream>

using std::vector;
using std::stringstream;

namespace ADDON
{

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
   {"musicvideos",   CONTENT_MUSICVIDEOS, 20389 }};

const CStdString TranslateContent(const CONTENT_TYPE &type, bool pretty/*=false*/)
{
  for (unsigned int index=0; index < sizeof(content)/sizeof(content[0]); ++index)
  {
    const ContentMapping &map = content[index];
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

const TYPE ScraperTypeFromContent(const CONTENT_TYPE &content)
{
  switch (content)
  {
  case CONTENT_ALBUMS:
    return ADDON_SCRAPER_ALBUMS;
  case CONTENT_ARTISTS:
    return ADDON_SCRAPER_ARTISTS;
  case CONTENT_MOVIES:
    return ADDON_SCRAPER_MOVIES;
  case CONTENT_MUSICVIDEOS:
    return ADDON_SCRAPER_MUSICVIDEOS;
  case CONTENT_TVSHOWS:
    return ADDON_SCRAPER_TVSHOWS;
  default:
    return ADDON_UNKNOWN;
  }
}

class CAddon;

CScraper::CScraper(const cp_extension_t *ext) :
  CAddon(ext)
{
  if (ext)
  {
    m_language = CAddonMgr::Get().GetExtValue(ext->configuration, "language");
    m_requiressettings = CAddonMgr::Get().GetExtValue(ext->configuration,"requiressettings").Equals("true");
  }
  switch (Type())
  {
    case ADDON_SCRAPER_ALBUMS:
      m_pathContent = CONTENT_ALBUMS;
      break;
    case ADDON_SCRAPER_ARTISTS:
      m_pathContent = CONTENT_ARTISTS;
      break;
    case ADDON_SCRAPER_MOVIES:
      m_pathContent = CONTENT_MOVIES;
      break;
    case ADDON_SCRAPER_MUSICVIDEOS:
      m_pathContent = CONTENT_MUSICVIDEOS;
      break;
    case ADDON_SCRAPER_TVSHOWS:
      m_pathContent = CONTENT_TVSHOWS;
      break;
    default:
      m_pathContent = CONTENT_NONE;
      break;
  }
}

AddonPtr CScraper::Clone(const AddonPtr &self) const
{
  return AddonPtr(new CScraper(*this, self));
}

CScraper::CScraper(const CScraper &rhs, const AddonPtr &self)
  : CAddon(rhs, self)
{
  m_pathContent = rhs.m_pathContent;
}

bool CScraper::Supports(const CONTENT_TYPE &content) const
{
  return Type() == ScraperTypeFromContent(content);
}

bool CScraper::SetPathSettings(CONTENT_TYPE content, const CStdString& xml)
{
  m_pathContent = content;
  if (!LoadSettings())
    return false;

  if (xml.IsEmpty())
    return true;

  m_userXmlDoc.Clear();
  m_userXmlDoc.Parse(xml.c_str());

  return m_userXmlDoc.RootElement()?true:false;
}

CStdString CScraper::GetPathSettings()
{
  if (!LoadSettings())
    return "";

  // construct our full settings structure by ensuring we have all of them updated
  const TiXmlElement *setting = m_addonXmlDoc.RootElement()->FirstChildElement("setting");
  while (setting)
  {
    CStdString id;
    if (setting->Attribute("id"))
      id = setting->Attribute("id");
    UpdateSetting(id, GetSetting(id));
    setting = setting->NextSiblingElement("setting");
  }

  stringstream stream;
  if (m_userXmlDoc.RootElement())
    stream << *m_userXmlDoc.RootElement();

  return stream.str();
}

}; /* namespace ADDON */

