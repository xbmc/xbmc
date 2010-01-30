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
#include "FileSystem/File.h"
#include "FileSystem/Directory.h"
#include "Util.h"
#include "FileItem.h"
#include "Album.h"
#include "Artist.h"
#include "GUISettings.h"
#include "utils/log.h"

#include <vector>

using namespace DIRECTORY;
using namespace std;
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

CNfoFile::NFOResult CNfoFile::Create(const CStdString& strPath, const CStdString& strContent, int episode)
{
  SScraperInfo info;
  info.strContent = strContent;
  return Create(strPath, info, episode);
}

CNfoFile::NFOResult CNfoFile::Create(const CStdString& strPath, SScraperInfo& info, int episode)
{
  m_info = info; // assume we can use these settings
  m_strContent = info.strContent;
  if (FAILED(Load(strPath)))
    return NO_NFO;

  CFileItemList items;
  CStdString strScraperBasePath, strDefault, strSelected;
  bool bNfo=false;
  if (m_strContent.Equals("albums"))
  {
    CAlbum album;
    bNfo = GetDetails(album);
    CDirectory::GetDirectory("special://xbmc/system/scrapers/music/",items,".xml",false);
    strScraperBasePath = "special://xbmc/system/scrapers/music/";
    CUtil::AddFileToFolder(strScraperBasePath, g_guiSettings.GetString("musiclibrary.scraper"), strDefault);
  }
  else if (m_strContent.Equals("artists"))
  {
    CArtist artist;
    bNfo = GetDetails(artist);
    CDirectory::GetDirectory("special://xbmc/system/scrapers/music/",items,".xml",false);
    strScraperBasePath = "special://xbmc/system/scrapers/music/";
    CUtil::AddFileToFolder(strScraperBasePath, g_guiSettings.GetString("musiclibrary.scraper"), strDefault);
  }
  else if (m_strContent.Equals("tvshows") || m_strContent.Equals("movies") || m_strContent.Equals("musicvideos"))
  {
    // first check if it's an XML file with the info we need
    CVideoInfoTag details;
    bNfo = GetDetails(details);
    if (episode > -1 && bNfo && m_strContent.Equals("tvshows"))
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
    CStdString strURL = details.m_strEpisodeGuide;
    strScraperBasePath = "special://xbmc/system/scrapers/video/";
    CDirectory::GetDirectory("special://xbmc/system/scrapers/video/",items,".xml",false);

    if (m_strContent.Equals("movies"))
      CUtil::AddFileToFolder(strScraperBasePath, g_guiSettings.GetString("scrapers.moviedefault"), strDefault);
    else if (m_strContent.Equals("tvshows"))
      CUtil::AddFileToFolder(strScraperBasePath, g_guiSettings.GetString("scrapers.tvshowdefault"), strDefault);
    else if (m_strContent.Equals("musicvideos"))
      CUtil::AddFileToFolder(strScraperBasePath, g_guiSettings.GetString("scrapers.musicvideodefault"), strDefault);
  }

  // Get Selected Scraper
  CVideoDatabase database;
  database.Open();
  database.GetScraperForPath(strPath,info);
  database.Close();
  CUtil::AddFileToFolder(strScraperBasePath, info.strPath, strSelected);

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

        SScraperInfo info2;
        CScraperParser parser2;
        parser2.Load(items[i]->m_strPath);
        info2.strContent = parser2.GetContent();
        info2.strLanguage = parser2.GetLanguage();

        // skip if scraper requires settings and there's nothing set yet
        if (parser2.RequiresSettings() && info2.settings.GetSettings().IsEmpty())
          continue;

        // skip wrong content type
        if (info.strContent != info2.strContent && (info.strContent.Equals("movies") || info.strContent.Equals("tvshows") || info.strContent.Equals("musicvideos")))
          continue;

        // add same language, multi-language and music scrapers
        if (info.strLanguage == info2.strLanguage || info2.strLanguage == "multi" || info.strContent.Equals("albums") || info.strContent.Equals("artists"))
          vecScrapers.push_back(items[i]->m_strPath);
      }
    }
  }

  // add default scraper
  if (find(vecScrapers.begin(),vecScrapers.end(),strDefault) == vecScrapers.end())
    vecScrapers.push_back(strDefault);

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

  m_strImDbUrl = parser.Parse(strFunction, m_strScraper.CompareNoCase(m_info.strPath) == 0 ? &m_info.settings : 0);
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

int CNfoFile::Scrape(const CStdString& strScraperPath, const CStdString& strURL /* = "" */)
{
  CScraperParser m_parser;
  if (!m_parser.Load(strScraperPath))
    return 0;
  if (m_parser.GetContent() != m_strContent &&
      !(m_strContent.Equals("artists") && m_parser.GetContent().Equals("albums")))
      // artists are scraped by album content scrapers
  {
    return 1;
  }

  m_strScraper = CUtil::GetFileName(strScraperPath);

  if (strURL.IsEmpty())
  {
    m_parser.m_param[0] = m_doc;
    m_strImDbUrl = m_parser.Parse("NfoScrape", m_strScraper.CompareNoCase(m_info.strPath) == 0 ? &m_info.settings : 0);
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

    if (!DoScrape(m_parser))
      return 2;

    if (m_strImDbUrl.size() > 0)
      return 0;
    else
      return 1;
  }
  else // we check to identify the episodeguide url
  {
    m_parser.m_param[0] = strURL;
    CStdString strEpGuide = m_parser.Parse("EpisodeGuideUrl", m_strScraper.CompareNoCase(m_info.strPath) == 0 ? &m_info.settings : 0); // allow corrections?
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
