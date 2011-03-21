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
#include "filesystem/File.h"
#include "filesystem/Directory.h"
#include "filesystem/FileCurl.h"
#include "AddonManager.h"
#include "utils/ScraperParser.h"
#include "utils/ScraperUrl.h"
#include "utils/CharsetConverter.h"
#include "utils/log.h"
#include "settings/AdvancedSettings.h"
#include "FileItem.h"
#include "utils/URIUtils.h"
#include "utils/XMLUtils.h"
#include "music/MusicDatabase.h"
#include "video/VideoDatabase.h"
#include <sstream>

using namespace std;
using namespace XFILE;

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

CONTENT_TYPE TranslateContent(const CStdString &string)
{
  for (unsigned int index=0; index < sizeof(content)/sizeof(content[0]); ++index)
  {
    const ContentMapping &map = content[index];
    if (string.Equals(map.name))
      return map.type;
  }
  return CONTENT_NONE;
}

TYPE ScraperTypeFromContent(const CONTENT_TYPE &content)
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
    m_language = CAddonMgr::Get().GetExtValue(ext->configuration, "@language");
    m_requiressettings = CAddonMgr::Get().GetExtValue(ext->configuration,"@requiressettings").Equals("true");
    CStdString persistence = CAddonMgr::Get().GetExtValue(ext->configuration, "@cachepersistence");
    if (!persistence.IsEmpty())
      m_persistence.SetFromTimeString(persistence);
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
  m_persistence = rhs.m_persistence;
  m_requiressettings = rhs.m_requiressettings;
  m_language = rhs.m_language;
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

  TiXmlDocument doc;
  doc.Parse(xml.c_str());
  m_userSettingsLoaded = SettingsFromXML(doc);

  return m_userSettingsLoaded;
}

CStdString CScraper::GetPathSettings()
{
  if (!LoadSettings())
    return "";

  stringstream stream;
  TiXmlDocument doc;
  SettingsToXML(doc);
  if (doc.RootElement())
    stream << *doc.RootElement();

  return stream.str();
}

void CScraper::ClearCache()
{
  CStdString strCachePath = URIUtils::AddFileToFolder(g_advancedSettings.m_cachePath, "scrapers");

  // create scraper cache dir if needed
  if (!CDirectory::Exists(strCachePath))
    CDirectory::Create(strCachePath);

  strCachePath = URIUtils::AddFileToFolder(strCachePath, ID());
  URIUtils::AddSlashAtEnd(strCachePath);

  if (CDirectory::Exists(strCachePath))
  {
    CFileItemList items;
    CDirectory::GetDirectory(strCachePath,items);
    for (int i=0;i<items.Size();++i)
    {
      // wipe cache
      if (items[i]->m_dateTime + m_persistence <= CDateTime::GetCurrentDateTime())
        CFile::Delete(items[i]->m_strPath);
    }
  }
  else
    CDirectory::Create(strCachePath);
}

// returns a vector of strings: the first is the XML output by the function; the rest
// is XML output by chained functions, possibly recursively
// the CFileCurl object is passed in so that URL fetches can be canceled from other threads
vector<CStdString> CScraper::Run(const CStdString& function,
                                 const CScraperUrl& scrURL,
                                 CFileCurl& http,
                                 const vector<CStdString>* extras)
{
  vector<CStdString> result;
  CStdString strXML = InternalRun(function,scrURL,http,extras);
  if (strXML.IsEmpty())
  {
    if (function != "NfoUrl")
      CLog::Log(LOGERROR, "%s: Unable to parse web site",__FUNCTION__);
    return result;
  }

  CLog::Log(LOGDEBUG,"scraper: %s returned %s",function.c_str(),strXML.c_str());

  if (!XMLUtils::HasUTF8Declaration(strXML))
    g_charsetConverter.unknownToUTF8(strXML);

  TiXmlDocument doc;
  doc.Parse(strXML.c_str(),0,TIXML_ENCODING_UTF8);
  if (!doc.RootElement())
  {
    CLog::Log(LOGERROR, "%s: Unable to parse XML",__FUNCTION__);
    return result; 
  }

  result.push_back(strXML);
  TiXmlElement* xchain = doc.RootElement()->FirstChildElement();
  // skip children of the root element until <url> or <chain>
  while (xchain && strcmp(xchain->Value(),"url") && strcmp(xchain->Value(),"chain"))
      xchain = xchain->NextSiblingElement();
  while (xchain && xchain->FirstChild())
  {
    // <chain|url function="...">param</>
    const char* szFunction = xchain->Attribute("function");
    if (szFunction)
    {
      CScraperUrl scrURL2;
      vector<CStdString> extras;
      // for <chain>, pass the contained text as a parameter; for <url>, as URL content
      if (strcmp(xchain->Value(),"chain")==0)
        extras.push_back(xchain->FirstChild()->Value());
      else
        scrURL2.ParseElement(xchain);
      vector<CStdString> result2 = Run(szFunction,scrURL2,http,&extras);
      result.insert(result.end(),result2.begin(),result2.end());
    }
    xchain = xchain->NextSiblingElement();
    // continue to skip past non-<url> or <chain> elements
    while (xchain && strcmp(xchain->Value(),"url") && strcmp(xchain->Value(),"chain"))
      xchain = xchain->NextSiblingElement();
  }
  
  return result;
}

CStdString CScraper::InternalRun(const CStdString& function,
                                 const CScraperUrl& scrURL,
                                 CFileCurl& http,
                                 const vector<CStdString>* extras)
{
  // walk the list of input URLs and fetch each into parser parameters
  unsigned int i;
  for (i=0;i<scrURL.m_url.size();++i)
  {
    CStdString strCurrHTML;
    if (!CScraperUrl::Get(scrURL.m_url[i],m_parser.m_param[i],http,ID()) || m_parser.m_param[i].size() == 0)
      return "";
  }
  // put the 'extra' parameterts into the parser parameter list too
  if (extras)
  {
    for (unsigned int j=0;j<extras->size();++j)
      m_parser.m_param[j+i] = (*extras)[j];
  }

  return m_parser.Parse(function,this);
}

bool CScraper::Load()
{
  bool result=m_parser.Load(LibPath());
  if (result)
  {
    // TODO: this routine assumes that deps are a single level, and assumes the dep is installed.
    //       1. Does it make sense to have recursive dependencies?
    //       2. Should we be checking the dep versions or do we assume it is ok?
    ADDONDEPS deps = GetDeps();
    ADDONDEPS::iterator itr = deps.begin();
    while (itr != deps.end())
    {
      if (itr->first.Equals("xbmc.metadata"))
      {
        ++itr;
        continue;
      }  
      AddonPtr dep;
      if (!CAddonMgr::Get().GetAddon((*itr).first, dep))
        return false;
      TiXmlDocument doc;
      if (dep->Type() == ADDON_SCRAPER_LIBRARY && doc.LoadFile(dep->LibPath()))
        m_parser.AddDocument(&doc);
      itr++;
    }
  }

  return result;
}

bool CScraper::IsInUse() const
{
  if (Supports(CONTENT_ALBUMS) || Supports(CONTENT_ARTISTS))
  { // music scraper
    CMusicDatabase db;
    if (db.Open() && db.ScraperInUse(ID()))
      return true;
  }
  else
  { // video scraper
    CVideoDatabase db;
    if (db.Open() && db.ScraperInUse(ID()))
      return true;
  }
  return false;
}

}; /* namespace ADDON */

