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
#include "MusicArtistInfo.h"
#include "ScraperParser.h"
#include "ScraperSettings.h"
#include "XMLUtils.h"
#include "Util.h"
#include "Settings.h"

using namespace MUSIC_GRABBER;
using namespace std;

CMusicArtistInfo::CMusicArtistInfo(void)
{
  m_bLoaded = false;
}

CMusicArtistInfo::~CMusicArtistInfo(void)
{
}

CMusicArtistInfo::CMusicArtistInfo(const CStdString& strArtist, const CScraperUrl& strArtistURL)
{
  m_artist.strArtist = strArtist;
  m_artistURL = strArtistURL;
  m_bLoaded = false;
}

const CArtist& CMusicArtistInfo::GetArtist() const
{
  return m_artist;
}

CArtist& CMusicArtistInfo::GetArtist()
{
  return m_artist;
}

void CMusicArtistInfo::SetArtist(const CArtist& artist)
{
  m_artist = artist;
  m_bLoaded = true;
}

const CScraperUrl& CMusicArtistInfo::GetArtistURL() const
{
  return m_artistURL;
}

bool CMusicArtistInfo::Parse(const TiXmlElement* artist, bool bChained)
{
  if (!m_artist.Load(artist,bChained))
    return false;

  SetLoaded(true);

  return true;
}

bool CMusicArtistInfo::Load(XFILE::CFileCurl& http, const SScraperInfo& info, const CStdString& strFunction, const CScraperUrl* url)
{
  // load our scraper xml
  if (!m_parser.Load("special://xbmc/system/scrapers/music/" + info.strPath))
    return false;

  bool bChained=true;
  if (!url)
  {
    bChained=false;
    url = &GetArtistURL();
  }

  vector<CStdString> strHTML;
  for (unsigned int i=0;i<url->m_url.size();++i)
  {
    CStdString strCurrHTML;
    if (!CScraperUrl::Get(url->m_url[i],strCurrHTML, http) || strCurrHTML.size() == 0)
      return false;
    strHTML.push_back(strCurrHTML);
  }

  // now grab our details using the scraper
  for (unsigned int i=0;i<strHTML.size();++i)
    m_parser.m_param[i] = strHTML[i];

  m_parser.m_param[strHTML.size()] = m_strSearch;

  CStdString strXML = m_parser.Parse(strFunction);
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

  return ret;
}

void CMusicArtistInfo::SetLoaded(bool bOnOff)
{
  m_bLoaded = bOnOff;
}

bool CMusicArtistInfo::Loaded() const
{
  return m_bLoaded;
}
