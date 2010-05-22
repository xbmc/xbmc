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
#include "VideoDatabase.h"
#include "utils/IMDB.h"
#include "addons/AddonManager.h"
#include "FileSystem/File.h"
#include "FileSystem/Directory.h"
#include "GUISettings.h"
#include "Util.h"
#include "FileItem.h"
#include "Album.h"
#include "Artist.h"
#include "GUISettings.h"
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

CNfoFile::NFOResult CNfoFile::Create(const CStdString& strPath, const ScraperPtr& info, int episode)
{
  m_info = info; // assume we can use these settings
  m_content = info->Content();
  if (FAILED(Load(strPath)))
    return NO_NFO;

  CFileItemList items;
  bool bNfo=false;

  AddonPtr addon;
  ScraperPtr defaultScraper;
  if (!CAddonMgr::Get().GetDefault(ADDON_SCRAPER, addon, m_content))
    return NO_NFO;
  else
    defaultScraper = boost::dynamic_pointer_cast<CScraper>(addon);

  if (m_content == CONTENT_ALBUMS)
  {
    CAlbum album;
    bNfo = GetDetails(album);
  }
  else if (m_content == CONTENT_ARTISTS)
  {
    CArtist artist;
    bNfo = GetDetails(artist);
  }
  else if (m_content == CONTENT_TVSHOWS || m_content == CONTENT_MOVIES || m_content == CONTENT_MUSICVIDEOS)
  {
    // first check if it's an XML file with the info we need
    CVideoInfoTag details;
    bNfo = GetDetails(details);
    if (episode > -1 && bNfo && m_content == CONTENT_TVSHOWS)
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

  // Get Selected Scraper
  CVideoDatabase database;
  database.Open();
  ADDON::ScraperPtr selected = database.GetScraperForPath(strPath);
  database.Close();

  vector<ScraperPtr> vecScrapers;

  // add selected scraper
  if (selected)
    vecScrapers.push_back(selected);

  /*if (g_guiSettings.GetBool("scrapers.langfallback"))
  {
    for (unsigned i=0;i<addons.size();++i)
    {
      // skip selected and default scraper
      if (addons[i]->UUID().Equals(selected->Parent()) || addons[i]->UUID().Equals(defaultScraper->UUID()))
        continue;

      (CScraperParser parser2;
      parser2.Load(addons[i]);
      CONTENT_TYPE content = parser2.GetContent();

        // skip if scraper requires settings and there's nothing set yet
        if (parser2.RequiresSettings() && info2.settings.GetSettings().IsEmpty())
          continue;

        // skip if scraper requires settings and there's nothing set yet
        if (parser2.RequiresSettings() && info2.settings.GetSettings().IsEmpty())
          continue;

        // skip wrong content type
        if (info.strContent != info2.strContent && (info.strContent.Equals("movies") || info.strContent.Equals("tvshows") || info.strContent.Equals("musicvideos")))
          continue;

      // add same language, multi-language and music scrapers
      // TODO addons language handling
    }
  }*/

  // add default scraper
  if ((selected && selected->Parent() != defaultScraper) || !selected) 
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

bool CNfoFile::DoScrape(CScraperParser& parser, const CScraperUrl* pURL, const CStdString& strFunction)
{
  if (!pURL)
    parser.m_param[0] = m_doc;
  else
  {
    vector<CStdString> strHTML;
    for (unsigned int i=0;i<pURL->m_url.size();++i)
    {
      CStdString strCurrHTML;
      XFILE::CFileCurl http;
      if (!CScraperUrl::Get(pURL->m_url[i],strCurrHTML,http,parser.GetFilename()) || strCurrHTML.size() == 0)
        return false;
      strHTML.push_back(strCurrHTML);
    }
    for (unsigned int i=0;i<strHTML.size();++i)
      parser.m_param[i] = strHTML[i];
  }

  m_strImDbUrl = parser.Parse(strFunction);
  TiXmlDocument doc;
  doc.Parse(m_strImDbUrl.c_str());

  if (doc.RootElement())
  {
    if (stricmp(doc.RootElement()->Value(),"error")==0)
    {
      CIMDB::ShowErrorDialog(doc.RootElement());
      return false;
    }

    TiXmlElement* xurl = doc.FirstChildElement("url");
    while (xurl && xurl->FirstChild())
    {
      const char* szFunction = xurl->Attribute("function");
      if (szFunction)
      {
        CScraperUrl scrURL(xurl);
        DoScrape(parser,&scrURL,szFunction);
      }
      xurl = xurl->NextSiblingElement("url");
    }
    TiXmlElement* pId = doc.FirstChildElement("id");
    if (pId && pId->FirstChild())
      m_strImDbNr = pId->FirstChild()->Value();
  }
  return true;
}

int CNfoFile::Scrape(const ScraperPtr& scraper, const CStdString& strURL /* = "" */)
{
  CScraperParser parser;
  if (!parser.Load(scraper))
    return 0;
  if (!scraper->Supports(m_content) &&
      !(m_content == CONTENT_ARTISTS && scraper->Content() == CONTENT_ALBUMS))
      // artists are scraped by album content scrapers
  {
    return 1;
  }

  // init and clear cache
  parser.ClearCache();

  if (strURL.IsEmpty())
  {
    parser.m_param[0] = m_doc;
    m_strImDbUrl = parser.Parse("NfoScrape");
    TiXmlDocument doc;
    doc.Parse(m_strImDbUrl.c_str());
    if (doc.RootElement() && doc.RootElement()->FirstChildElement())
    {
      CVideoInfoTag details;
      if (GetDetails(details,m_strImDbUrl.c_str()))
      {
        Close();
        m_size = m_strImDbUrl.size();
        m_doc = new char[m_size+1];
        m_headofdoc = m_doc;
        strcpy(m_doc,m_strImDbUrl.c_str());
        return 0;
      }
    }

    if (!DoScrape(parser))
      return 2;

    if (m_strImDbUrl.size() > 0)
      return 0;
    else
      return 1;
  }
  else // we check to identify the episodeguide url
  {
    parser.m_param[0] = strURL;
    CStdString strEpGuide = parser.Parse("EpisodeGuideUrl"); // allow corrections?
    if (strEpGuide.IsEmpty())
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

  m_size = 0;
}
