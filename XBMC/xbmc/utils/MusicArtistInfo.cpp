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
  if (!bChained)
    m_artist.Reset();

  XMLUtils::GetString(artist,"title",m_artist.strArtist);
  const TiXmlNode* node = artist->FirstChild("genre");
  while (node)
  {
    if (node->FirstChild())
    {
      CStdString strTemp = node->FirstChild()->Value();
      if (m_artist.strGenre.IsEmpty())
        m_artist.strGenre = strTemp;
      else
        m_artist.strGenre += g_advancedSettings.m_musicItemSeparator+strTemp;
    }
    node = node->NextSibling("genre");
  }
  node = artist->FirstChild("style");
  while (node)
  {
    if (node->FirstChild())
    {
      CStdString strTemp = node->FirstChild()->Value();
      if (m_artist.strStyles.IsEmpty())
        m_artist.strStyles = strTemp;
      else
        m_artist.strStyles += g_advancedSettings.m_musicItemSeparator+strTemp;
    }
    node = node->NextSibling("style");
  }
  node = artist->FirstChild("mood");
  while (node)
  {
    if (node->FirstChild())
    {
      CStdString strTemp = node->FirstChild()->Value();
      if (m_artist.strMoods.IsEmpty())
        m_artist.strMoods = strTemp;
      else
        m_artist.strMoods += g_advancedSettings.m_musicItemSeparator+strTemp;
    }
    node = node->NextSibling("mood");
  }

  XMLUtils::GetString(artist,"born",m_artist.strBorn);
  XMLUtils::GetString(artist,"instruments",m_artist.strInstruments);
  XMLUtils::GetString(artist,"biography",m_artist.strBiography);

  const TiXmlElement* thumbElement = artist->FirstChildElement("thumbs");
  if (!thumbElement || !m_artist.thumbURL.ParseElement(thumbElement) || m_artist.thumbURL.m_url.size() == 0)
  {
    if (artist->FirstChildElement("thumb") && !artist->FirstChildElement("thumb")->FirstChildElement())
    {
      if (artist->FirstChildElement("thumb")->FirstChild() && strncmp(artist->FirstChildElement("thumb")->FirstChild()->Value(),"<thumb>",7) == 0)
      {
        CStdString strValue = artist->FirstChildElement("thumb")->FirstChild()->Value();
        TiXmlDocument doc;
        doc.Parse(strValue.c_str());
        if (doc.FirstChildElement("thumbs"))
          m_artist.thumbURL.ParseElement(doc.FirstChildElement("thumbs"));
        else
          m_artist.thumbURL.ParseElement(doc.FirstChildElement("thumb"));
      }
      else
        m_artist.thumbURL.ParseElement(artist->FirstChildElement("thumb"));
    }
  }
  node = artist->FirstChild("album");
  while (node)
  {
    const TiXmlNode* title = node->FirstChild("title");
    if (title && title->FirstChild())
    {
      CStdString strTitle = title->FirstChild()->Value();
      CStdString strYear;
      const TiXmlNode* year = node->FirstChild("year");
      if (year && year->FirstChild())
        strYear = year->FirstChild()->Value();
      m_artist.discography.push_back(make_pair(strTitle,strYear));
    }
    node = node->NextSibling("album");
  }

  SetLoaded(true);

  return true;
}

bool CMusicArtistInfo::Load(XFILE::CFileCurl& http, const SScraperInfo& info, const CStdString& strFunction, const CScraperUrl* url)
{
  // load our scraper xml
  CScraperParser parser;
  if (!parser.Load("special://xbmc/system/scrapers/music/" + info.strPath))
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
    parser.m_param[i] = strHTML[i];

  CStdString strXML = parser.Parse(strFunction);
  if (strXML.IsEmpty())
  {
    CLog::Log(LOGERROR, "%s: Unable to parse web site",__FUNCTION__);
    return false;
  }

  // abit ugly, but should work. would have been better if parser
  // set the charset of the xml, and we made use of that
  if (strXML.Find("encoding=\"utf-8\"") < 0)
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

void CMusicArtistInfo::SetLoaded(bool bOnOff)
{
  m_bLoaded = bOnOff;
}

bool CMusicArtistInfo::Loaded() const
{
  return m_bLoaded;
}
