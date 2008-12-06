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

#include "stdafx.h"
#include "NfoFile.h"
#include "utils/ScraperParser.h"
#include "utils/IMDB.h"
#include "FileSystem/File.h"
#include "FileSystem/Directory.h"
#include "Util.h"
#include "FileItem.h"
#include "Album.h"
#include "Artist.h"

using namespace DIRECTORY;
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
  m_strContent = strContent;
  if (FAILED(Load(strPath)))
    return NO_NFO;

  CStdString strURL, strURL2;
  CFileItemList items;
  bool bNfo=false;
  if (m_strContent.Equals("albums"))
  {
    CAlbum album;
    bNfo = GetDetails(album);
    CDirectory::GetDirectory("q:\\system\\scrapers\\music",items,".xml",false);
  }
  else if (m_strContent.Equals("artists"))
  {
    CArtist artist;
    bNfo = GetDetails(artist);
    CDirectory::GetDirectory("q:\\system\\scrapers\\music",items,".xml",false);
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
    CDirectory::GetDirectory("q:\\system\\scrapers\\video",items,".xml",false);
    strURL2 = details.m_strEpisodeGuide;
  }

  for (int i=0;i<items.Size();++i)
  {
    if (!items[i]->m_bIsFolder && !FAILED(Scrape(items[i]->m_strPath,strURL)))
    {
      strURL.Empty();
      break;
    }
  }

  if (m_strContent.Equals("tvshows") && bNfo) // need to identify which scraper
  {
    strURL = strURL2;
    for (int i=0;i<items.Size();++i)
    {
      if (!items[i]->m_bIsFolder && !FAILED(Scrape(items[i]->m_strPath,strURL)))
      {
        strURL.Empty();
        break;
      }
    }
  }

  if (bNfo)
    return (m_strImDbUrl.size() > 0) ? COMBINED_NFO:FULL_NFO;
  
  return   (m_strImDbUrl.size() > 0) ? URL_NFO : NO_NFO;
}

HRESULT CNfoFile::Scrape(const CStdString& strScraperPath, const CStdString& strURL /* = "" */)
{
  CScraperParser m_parser;
  if (!m_parser.Load(strScraperPath))
    return E_FAIL;
  if (m_parser.GetContent() != m_strContent && 
      !(m_strContent.Equals("artists") && m_parser.GetContent().Equals("albums")))
      // artists are scraped by album content scrapers
  {
    return E_FAIL;
  }

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
    m_parser.m_param[0] = m_doc;
    m_strImDbUrl = m_parser.Parse("NfoUrl");
    doc.Parse(m_strImDbUrl.c_str());
    TiXmlElement* pId = doc.FirstChildElement("id");
    if (pId && pId->FirstChild())
      m_strImDbNr = pId->FirstChild()->Value();

    if (m_strImDbUrl.size() > 0)
    {
      m_strScraper = CUtil::GetFileName(strScraperPath);
      return S_OK;
    }
    else
      return E_FAIL;
  }
  else // we check to identify the episodeguide url
  {
    m_parser.m_param[0] = strURL;
    CStdString strEpGuide = m_parser.Parse("EpisodeGuideUrl"); // allow corrections?
    if (strEpGuide.IsEmpty())
      return E_FAIL;

    m_strScraper = CUtil::GetFileName(strScraperPath);
    return S_OK;
  }
}

HRESULT CNfoFile::Load(const CStdString& strFile)
{
  Close();
  XFILE::CFile file;
  if (file.Open(strFile, true))
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
