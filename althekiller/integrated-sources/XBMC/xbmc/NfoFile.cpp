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

CNfoFile::CNfoFile(const CStdString& strContent)
{
  m_strContent = strContent;
  m_doc = NULL;
}

CNfoFile::~CNfoFile()
{
  Close();
}

HRESULT CNfoFile::Create(const CStdString& strPath)
{
  if (FAILED(Load(strPath)))
    return E_FAIL;

  CStdString strURL;
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
    CDirectory::GetDirectory("q:\\system\\scrapers\\video",items,".xml",false);
    if (m_strContent.Equals("tvshows") && bNfo) // need to identify which scraper
      strURL = details.m_strEpisodeGuide;

  }
  if (bNfo)
  {
    m_strScraper = "NFO";
    if (!m_strContent.Equals("tvshows") || !CUtil::GetFileName(strPath).Equals("tvshow.nfo")) // need to identify which scraper
      return S_OK;
  }

  for (int i=0;i<items.Size();++i)
  {
    if (!items[i]->m_bIsFolder && !FAILED(Scrape(items[i]->m_strPath,strURL)))
    {
      strURL.Empty();
      break;
    }
  }

  if (m_strContent.Equals("tvshows"))
    return (strURL.IsEmpty() && !m_strScraper.IsEmpty())?S_OK:E_FAIL;

  return (m_strImDbUrl.size() > 0) ? S_OK : E_FAIL;
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
    if (doc.RootElement())
    {
      CVideoInfoTag details;
      if (GetDetails(details,m_strImDbUrl.c_str()))
      {
        m_strScraper = "NFO";
        Close();
        m_size = m_strImDbUrl.size();
        m_doc = new char[m_size+1];
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

    m_strImDbNr = CUtil::GetFileName(strScraperPath); // used to pass scraper info for tvshows with nfo and episode guide urls
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
    m_doc = new char[m_size+1];
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
