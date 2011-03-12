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
// NfoFile.cpp: implementation of the CNfoFile class.
//
//////////////////////////////////////////////////////////////////////

#include "NfoFile.h"
#include "video/VideoInfoDownloader.h"
#include "addons/AddonManager.h"
#include "filesystem/File.h"
#include "settings/GUISettings.h"
#include "Util.h"
#include "FileItem.h"
#include "music/Album.h"
#include "music/Artist.h"
#include "settings/GUISettings.h"
#include "LangInfo.h"
#include "utils/log.h"

#include <vector>

using namespace XFILE;
using namespace std;
using namespace ADDON;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CNfoFile::CNfoFile()
{
  m_doc = NULL;
  m_headofdoc = NULL;
}

CNfoFile::~CNfoFile()
{
  Close();
}

CNfoFile::NFOResult CNfoFile::Create(const CStdString& strPath, const ScraperPtr& info, int episode, const CStdString& strPath2)
{
  m_info = info; // assume we can use these settings
  m_type = ScraperTypeFromContent(info->Content());
  if (FAILED(Load(strPath)))
    return NO_NFO;

  CFileItemList items;
  bool bNfo=false;

  AddonPtr addon;
  ScraperPtr defaultScraper;
  if (CAddonMgr::Get().GetDefault(m_type, addon))
    defaultScraper = boost::dynamic_pointer_cast<CScraper>(addon);

  if (m_type == ADDON_SCRAPER_ALBUMS)
  {
    CAlbum album;
    bNfo = GetDetails(album);
  }
  else if (m_type == ADDON_SCRAPER_ARTISTS)
  {
    CArtist artist;
    bNfo = GetDetails(artist);
  }
  else if (m_type == ADDON_SCRAPER_TVSHOWS || m_type == ADDON_SCRAPER_MOVIES || m_type == ADDON_SCRAPER_MUSICVIDEOS)
  {
    // first check if it's an XML file with the info we need
    CVideoInfoTag details;
    bNfo = GetDetails(details);
    if (episode > -1 && bNfo && m_type == ADDON_SCRAPER_TVSHOWS)
    {
      int infos=0;
      while (m_headofdoc && details.m_iEpisode != episode)
      {
        m_headofdoc = strstr(m_headofdoc+1,"<episodedetails>");
        bNfo  = GetDetails(details);
        infos++;
      }
      if (details.m_iEpisode != episode)
      {
        bNfo = false;
        details.Reset();
        m_headofdoc = m_doc;
        if (infos == 1) // still allow differing nfo/file numbers for single ep nfo's
          bNfo = GetDetails(details);
      }
    }
  }

  vector<ScraperPtr> vecScrapers;

  // add selected scraper
  if (m_info)
    vecScrapers.push_back(m_info);

  VECADDONS addons;
  CAddonMgr::Get().GetAddons(m_type,addons);
  // first pass - add language based scrapers
  if (m_info && g_guiSettings.GetBool("scrapers.langfallback"))
    AddScrapers(addons,vecScrapers);

  // add default scraper
  if (defaultScraper && m_info && m_info->ID() != defaultScraper->ID())
    vecScrapers.push_back(defaultScraper);

  // search ..
  int res = -1;
  for (unsigned int i=0;i<vecScrapers.size();++i)
    if ((res = Scrape(vecScrapers[i])) == 0 || res == 2)
      break;

  if (res == 2)
    return ERROR_NFO;
  if (bNfo)
    return (m_strImDbUrl.size() > 0) ? COMBINED_NFO:FULL_NFO;

  return   (m_strImDbUrl.size() > 0) ? URL_NFO : NO_NFO;
}

bool CNfoFile::DoScrape(ScraperPtr& scraper)
{
  vector<CStdString> extras;
  extras.push_back(m_doc);
  
  CScraperUrl url;
  CFileCurl http;
  vector<CStdString> xml;
  if (scraper->GetParser().HasFunction("NfoUrl"))
    xml = scraper->Run("NfoUrl",url,http,&extras);

  for (vector<CStdString>::iterator it  = xml.begin();
                                    it != xml.end(); ++it)
  {
    TiXmlDocument doc;
    doc.Parse(it->c_str());

    if (doc.RootElement())
    {
      if (stricmp(doc.RootElement()->Value(),"error")==0)
      {
        CVideoInfoDownloader::ShowErrorDialog(doc.RootElement());
        return false;
      }

      TiXmlElement* pId = doc.FirstChildElement("id");
      if (pId && pId->FirstChild())
        m_strImDbNr = pId->FirstChild()->Value();

      TiXmlElement* url = doc.FirstChildElement("url");
      if (url)
      {
        stringstream str;
        str << *url;
        m_strImDbUrl = str.str();
        SetScraperInfo(scraper);
      }
      else if (strcmp(doc.RootElement()->Value(),"url")==0)
      {
        SetScraperInfo(scraper);
        m_strImDbUrl = *it;
      }
    }
  }
  return true;
}

int CNfoFile::Scrape(ScraperPtr& scraper, const CStdString& strURL /* = "" */)
{
  if (scraper->Type() != m_type)
  {
    return 1;
  }
  if (!scraper->Load())
    return 0;

  // init and clear cache
  scraper->ClearCache();

  vector<CStdString> extras;
  CScraperUrl url;
  CFileCurl http;
  if (strURL.IsEmpty())
  {
    if (!DoScrape(scraper))
      return 2;
    if (m_strImDbUrl.size() > 0)
      return 0;
    else
      return 1;
  }
  else // we check to identify the episodeguide url
  {
    extras.push_back(strURL);
    vector<CStdString> result = scraper->Run("EpisodeGuideUrl",url,http,&extras);
    if (result.empty() || result[0].IsEmpty())
      return 1;
    return 0;
  }
}

int CNfoFile::Load(const CStdString& strFile)
{
  Close();
  XFILE::CFile file;
  if (file.Open(strFile))
  {
    m_size = (int)file.GetLength();
    try
    {
      m_doc = new char[m_size+1];
      m_headofdoc = m_doc;
    }
    catch (...)
    {
      CLog::Log(LOGERROR, "%s: Exception while creating file buffer",__FUNCTION__);
      return 1;
    }
    if (!m_doc)
    {
      file.Close();
      return 1;
    }
    file.Read(m_doc, m_size);
    m_doc[m_size] = 0;
    file.Close();
    return 0;
  }
  return 1;
}

void CNfoFile::Close()
{
  if (m_doc != NULL)
  {
    delete m_doc;
    m_doc = 0;
  }

  m_strImDbUrl = "";
  m_strImDbNr = "";
  m_size = 0;
}

void CNfoFile::AddScrapers(VECADDONS& addons,
                           vector<ScraperPtr>& vecScrapers)
{
  for (unsigned i=0;i<addons.size();++i)
  {
    ScraperPtr scraper = boost::dynamic_pointer_cast<CScraper>(addons[i]);

    // skip if scraper requires settings and there's nothing set yet
    if (scraper->RequiresSettings() && !scraper->HasUserSettings())
      continue;

    // add same language and multi-language
    if (scraper->Language() == m_info->Language() || scraper->Language().Equals("multi"))
      vecScrapers.push_back(scraper);
  }
}
