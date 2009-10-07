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
#include "utils/IAddon.h"
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

  CFileItemList items;
  CStdString strScraperBasePath, strDefault, strSelected;
  bool bNfo=false;
  if (m_content == CONTENT_ALBUMS)
  {
    CAlbum album;
    bNfo                = GetDetails(album);
    strDefault          = g_guiSettings.GetString("musiclibrary.defaultscraper");
    VECADDONS addons;
    CAddonMgr::Get()->GetAddons(ADDON_SCRAPER, addons, CONTENT_MUSIC);
    if (addons.empty())
      return NO_NFO;

    for (IVECADDONS it = addons.begin(); it != addons.end(); it++)
    {
      CStdString pathFile = (*it)->Path() + (*it)->LibName();
      CFileItemPtr newItem(new CFileItem(pathFile ,false));
      items.Add(newItem);
    }
  }
  else if (m_content == CONTENT_ARTISTS)
  {
    CArtist artist;
    bNfo                = GetDetails(artist);
    strDefault          = g_guiSettings.GetString("musiclibrary.defaultscraper");
    VECADDONS addons;
    CAddonMgr::Get()->GetAddons(ADDON_SCRAPER, addons, CONTENT_MUSIC);
    if (addons.empty())
      return NO_NFO;

    for (IVECADDONS it = addons.begin(); it != addons.end(); it++)
    {
      CStdString pathFile = (*it)->Path() + (*it)->LibName();
      CFileItemPtr newItem(new CFileItem(pathFile ,false));
      items.Add(newItem);
    }
  }
  else if (m_content == CONTENT_TVSHOWS || m_content == CONTENT_MOVIES || m_content == CONTENT_MUSICVIDEOS)
  {
    // first check if it's an XML file with the info we need
    CVideoInfoTag details;
    bNfo = GetDetails(details);
    if (m_content == CONTENT_MOVIES)
      strDefault = g_guiSettings.GetString("scrapers.moviedefault");
    else if (m_content == CONTENT_TVSHOWS)
      strDefault = g_guiSettings.GetString("scrapers.tvshowdefault");
    else if (m_content == CONTENT_MUSICVIDEOS)
      strDefault = g_guiSettings.GetString("scrapers.musicvideodefault");
    //TODO 
    VECADDONS addons;
    CAddonMgr::Get()->GetAddons(ADDON_SCRAPER, addons, CONTENT_MOVIES);
    if (addons.empty())
      return NO_NFO;

    for (IVECADDONS it = addons.begin(); it != addons.end(); it++)
    {
      CStdString pathFile = (*it)->Path() + (*it)->LibName();
      CFileItemPtr newItem(new CFileItem(pathFile ,false));
      items.Add(newItem);
    }

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
  /*ADDON::CScraperPtr info;*/
  database.Open();
  database.GetScraperForPath(strPath,info);
  database.Close();
  CUtil::AddFileToFolder(strScraperBasePath, info->Path(), strSelected);

  vector<CStdString> vecScrapers;

  // add selected scraper
  vecScrapers.push_back(strSelected);

  if (g_guiSettings.GetBool("scrapers.langfallback"))
  {
    for (int i=0;i<items.Size();++i)
    {
      if (!items[i]->m_bIsFolder)
      {
        // skip selected and default scraper
        if (items[i]->m_strPath.Equals(strSelected) || items[i]->m_strPath.Equals(strDefault))
          continue;

        ADDON::CScraperPtr info2;
        ADDON::CScraperParser parser2;
        parser2.Load(items[i]->m_strPath);
        //info2->Content() = parser2.GetContent(); //TODO modifying scrapers
        //info2.m_strLanguage = parser2.GetLanguage();

        // skip wrong content type //TODO refactor
        if (info->Content() != info2->Content() && (info->Content() == CONTENT_MOVIES 
                                                  || info->Content() == CONTENT_TVSHOWS 
                                                  || info->Content() == CONTENT_MUSICVIDEOS))
          continue;

        // add same language, multi-language and music scrapers
       /* if (info.m_strLanguage == info2.m_strLanguage || info2.m_strLanguage == "multi" 
            || info->Content() == CONTENT_ALBUMS || info->Content() == CONTENT_ARTISTS)
          vecScrapers.push_back(items[i]->m_strPath);*/
      }
    }
  }

  // add default scraper
  if (find(vecScrapers.begin(),vecScrapers.end(),strDefault) == vecScrapers.end())
    vecScrapers.push_back(strDefault);

  // search ..
  for (unsigned int i=0;i<vecScrapers.size();++i)
    if (!Scrape(vecScrapers[i]))
      break;

  if (bNfo)
    return (m_strImDbUrl.size() > 0) ? COMBINED_NFO:FULL_NFO;

  return   (m_strImDbUrl.size() > 0) ? URL_NFO : NO_NFO;
}

void CNfoFile::DoScrape(ADDON::CScraperParser& parser, const CScraperUrl* pURL, const CStdString& strFunction)
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

HRESULT CNfoFile::Scrape(const CStdString& strScraperPath, const CStdString& strURL /* = "" */)
{
  CScraperParser m_parser;
  if (!m_parser.Load(strScraperPath))
    return E_FAIL;
  if (m_parser.GetContent() != m_content &&
      !(m_content == CONTENT_ARTISTS && m_parser.GetContent() == CONTENT_ALBUMS))
      // artists are scraped by album content scrapers
  {
    return E_FAIL;
  }

  m_strScraper = CUtil::GetFileName(strScraperPath);

  if (strURL.IsEmpty())
  {
    m_parser.m_param[0] = m_doc;
    m_strImDbUrl = m_parser.Parse("NfoScrape");
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

    DoScrape(m_parser);

    if (m_strImDbUrl.size() > 0)
      return S_OK;
    else
      return E_FAIL;
  }
  else // we check to identify the episodeguide url
  {
    m_parser.m_param[0] = strURL;
    CStdString strEpGuide = m_parser.Parse("EpisodeGuideUrl"); // allow corrections?
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
