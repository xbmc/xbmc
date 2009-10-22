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
#include "utils/AddonManager.h"
#include "FileSystem/File.h"
#include "FileSystem/Directory.h"
#include "GUISettings.h"
#include "Util.h"
#include "FileItem.h"
#include "Album.h"
#include "Artist.h"
#include "GUISettings.h"

#include <vector>

using namespace DIRECTORY;
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

CNfoFile::NFOResult CNfoFile::Create(const CStdString& strPath, CScraperPtr& info, int episode)
{
  m_info = info; // assume we can use these settings
  m_content = info->Content();
  if (FAILED(Load(strPath)))
    return NO_NFO;

  CScraperPtr defaultScraper;
  bool bNfo=false;

  VECADDONS addons; 
  CAddonMgr::Get()->GetAddons(ADDON_SCRAPER, addons, m_content);
  if (addons.empty())
    return NO_NFO;

  if (!CAddonMgr::Get()->GetDefaultScraper(defaultScraper, m_content))
    return NO_NFO; //TODO check this is correct response

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
  ADDON::CScraperPtr selected;
  database.Open();
  database.GetScraperForPath(strPath,selected);
  database.Close();

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

      // skip wrong content type
      if (info->Content() != content && (info->Content() == CONTENT_MOVIES 
                                                || info->Content() == CONTENT_TVSHOWS 
                                                || info->Content() == CONTENT_MUSICVIDEOS))
        continue;

      // add same language, multi-language and music scrapers
      // TODO addons language handling
    }
  }

  // add default scraper
  if (find(vecScrapers.begin(),vecScrapers.end(),strDefault) == vecScrapers.end())
    vecScrapers.push_back(strDefault);

  // search ..
  //TODO
  for (unsigned int i=0;i<vecScrapers.size();++i)
    if (!Scrape(vecScrapers[i]))
      break;*/

  if (bNfo)
    return (m_strImDbUrl.size() > 0) ? COMBINED_NFO:FULL_NFO;

  return   (m_strImDbUrl.size() > 0) ? URL_NFO : NO_NFO;
}

void CNfoFile::DoScrape(CScraperParser& parser, const CScraperUrl* pURL, const CStdString& strFunction)
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
      if (!CScraperUrl::Get(pURL->m_url[i],strCurrHTML,http) || strCurrHTML.size() == 0)
        return;
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
}

HRESULT CNfoFile::Scrape(const AddonPtr& addon, const CStdString& strURL /* = "" */)
{
  CScraperParser parser;
  CScraperPtr scraper = boost::dynamic_pointer_cast<CScraper>(addon);
  if (!parser.Load(scraper))
    return E_FAIL;
  if (scraper->Content() != m_content &&
      !(m_content == CONTENT_ARTISTS && scraper->Content() == CONTENT_ALBUMS))
      // artists are scraped by album content scrapers
  {
    return E_FAIL;
  }

  m_strScraper = addon->Name(); 

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
        return S_OK;
      }
    }

    DoScrape(parser);

    if (m_strImDbUrl.size() > 0)
      return S_OK;
    else
      return E_FAIL;
  }
  else // we check to identify the episodeguide url
  {
    parser.m_param[0] = strURL;
    CStdString strEpGuide = parser.Parse("EpisodeGuideUrl"); // allow corrections?
    if (strEpGuide.IsEmpty())
      return E_FAIL;
    return S_OK;
  }
}

HRESULT CNfoFile::Load(const CStdString& strFile)
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
      return E_FAIL;
    }
    if (!m_doc)
    {
      file.Close();
      return E_FAIL;
    }
    file.Read(m_doc, m_size);
    m_doc[m_size] = 0;
    file.Close();
    return S_OK;
  }
  return E_FAIL;
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
