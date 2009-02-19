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

#include "stdafx.h"
#include "MusicAlbumInfo.h"
#include "ScraperParser.h"
#include "ScraperSettings.h"
#include "XMLUtils.h"
#include "HTMLTable.h"
#include "HTMLUtil.h"
#include "Util.h"

using namespace MUSIC_GRABBER;
using namespace std;

CMusicAlbumInfo::CMusicAlbumInfo(void)
{
  m_strTitle2 = "";
  m_bLoaded = false;
  m_relevance = -1;
}

CMusicAlbumInfo::~CMusicAlbumInfo(void)
{
}

CMusicAlbumInfo::CMusicAlbumInfo(const CStdString& strAlbumInfo, const CScraperUrl& strAlbumURL)
{
  m_strTitle2 = strAlbumInfo;
  m_albumURL = strAlbumURL;
  m_bLoaded = false;
}

CMusicAlbumInfo::CMusicAlbumInfo(const CStdString& strAlbum, const CStdString& strArtist, const CStdString& strAlbumInfo, const CScraperUrl& strAlbumURL)
{
  m_album.strAlbum = strAlbum;
  m_album.strArtist = strArtist;
  m_strTitle2 = strAlbumInfo;
  m_albumURL = strAlbumURL;
  m_bLoaded = false;
}

const CAlbum& CMusicAlbumInfo::GetAlbum() const
{
  return m_album;
}

CAlbum& CMusicAlbumInfo::GetAlbum()
{
  return m_album;
}

void CMusicAlbumInfo::SetAlbum(CAlbum& album)
{
  m_album = album;
  m_album.m_strDateOfRelease.Format("%i", album.iYear);
  m_strTitle2 = "";
  m_bLoaded = true;
}

const VECSONGS &CMusicAlbumInfo::GetSongs() const
{
  return m_album.songs;
}

void CMusicAlbumInfo::SetSongs(VECSONGS &songs)
{
  m_album.songs = songs;
}

void CMusicAlbumInfo::SetTitle(const CStdString& strTitle)
{
  m_album.strAlbum = strTitle;
}

const CScraperUrl& CMusicAlbumInfo::GetAlbumURL() const
{
  return m_albumURL;
}

const CStdString& CMusicAlbumInfo::GetTitle2() const
{
  return m_strTitle2;
}

const CStdString& CMusicAlbumInfo::GetDateOfRelease() const
{
  return m_album.m_strDateOfRelease;
}

bool CMusicAlbumInfo::Parse(const TiXmlElement* album, bool bChained)
{
  if (!m_album.Load(album,bChained))
    return false;

  if (m_strTitle2.IsEmpty())
    m_strTitle2 = m_album.strAlbum;

  SetLoaded(true);

  return true;
}


bool CMusicAlbumInfo::Load(XFILE::CFileCurl& http, const SScraperInfo& info, const CStdString& strFunction, const CScraperUrl* url)
{
  // load our scraper xml
  CScraperParser parser;
  if (!parser.Load("special://xbmc/system/scrapers/music/" + info.strPath))
    return false;

  bool bChained=true;
  if (!url)
  {
    bChained=false;
    url = &GetAlbumURL();
    CScraperParser::ClearCache();
  }

  vector<CStdString> strHTML;
  for (unsigned int i=0;i<url->m_url.size();++i)
  {
    CStdString strCurrHTML;
    if (!CScraperUrl::Get(url->m_url[i],strCurrHTML,http) || strCurrHTML.size() == 0)
      return false;
    strHTML.push_back(strCurrHTML);
  }

  // now grab our details using the scraper
  for (unsigned int i=0;i<strHTML.size();++i)
    parser.m_param[i] = strHTML[i];

  CStdString strXML = parser.Parse(strFunction);
  if (strXML.IsEmpty())
  {
    CLog::Log(LOGERROR, "%s: Unable to parse web site",__FUNCTION__);
    return false;
  }

  // abit ugly, but should work. would have been better if parser
  // set the charset of the xml, and we made use of that
  if (!XMLUtils::HasUTF8Declaration(strXML))
    g_charsetConverter.unknownToUTF8(strXML);

    // ok, now parse the xml file
  TiXmlBase::SetCondenseWhiteSpace(false);
  TiXmlDocument doc;
  doc.Parse(strXML.c_str(),0,TIXML_ENCODING_UTF8);
  if (!doc.RootElement())
  {
    CLog::Log(LOGERROR, "%s: Unable to parse xml",__FUNCTION__);
    return false;
  }

  bool ret = Parse(doc.RootElement(),bChained);
  TiXmlElement* pRoot = doc.RootElement();
  TiXmlElement* xurl = pRoot->FirstChildElement("url");
  while (xurl && xurl->FirstChild())
  {
    const char* szFunction = xurl->Attribute("function");
    if (szFunction)
    {
      CScraperUrl scrURL(xurl);
      Load(http,info,szFunction,&scrURL);
    }
    xurl = xurl->NextSiblingElement("url");
  }
  TiXmlBase::SetCondenseWhiteSpace(true);

  return ret;
}

void CMusicAlbumInfo::SetLoaded(bool bOnOff)
{
  m_bLoaded = bOnOff;
}

bool CMusicAlbumInfo::Loaded() const
{
  return m_bLoaded;
}
