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

#include "MusicInfoScraper.h"
#include "URL.h"
#include "utils/CharsetConverter.h"
#include "utils/log.h"

using namespace MUSIC_GRABBER;
using namespace ADDON;
using namespace std;

CMusicInfoScraper::CMusicInfoScraper(const ADDON::ScraperPtr &scraper)
{
  m_bSucceeded=false;
  m_bCanceled=false;
  m_iAlbum=-1;
  m_iArtist=-1;
  m_scraper = scraper;
}

CMusicInfoScraper::~CMusicInfoScraper(void)
{
  StopThread();
}

int CMusicInfoScraper::GetAlbumCount() const
{
  return (int)m_vecAlbums.size();
}

int CMusicInfoScraper::GetArtistCount() const
{
  return (int)m_vecArtists.size();
}

CMusicAlbumInfo& CMusicInfoScraper::GetAlbum(int iAlbum)
{
  return m_vecAlbums[iAlbum];
}

CMusicArtistInfo& CMusicInfoScraper::GetArtist(int iArtist)
{
  return m_vecArtists[iArtist];
}

void CMusicInfoScraper::FindAlbumInfo(const CStdString& strAlbum, const CStdString& strArtist /* = "" */)
{
  m_strAlbum=strAlbum;
  m_strArtist=strArtist;
  m_bSucceeded=false;
  StopThread();
  Create();
}

void CMusicInfoScraper::FindArtistInfo(const CStdString& strArtist)
{
  m_strArtist=strArtist;
  m_bSucceeded=false;
  StopThread();
  Create();
}


void CMusicInfoScraper::FindAlbumInfo()
{
  CStdString strAlbum=m_strAlbum;
  CStdString strHTML;
  m_vecAlbums.erase(m_vecAlbums.begin(), m_vecAlbums.end());

  if (!m_scraper->Load() || !m_scraper->GetParser().HasFunction("CreateAlbumSearchUrl"))
    return;

  CLog::Log(LOGDEBUG, "%s: Searching for '%s - %s' using %s scraper (path: '%s', content: '%s', version: '%s')",
    __FUNCTION__, m_strArtist.c_str(), strAlbum.c_str(), m_scraper->Name().c_str(), m_scraper->Path().c_str(),
    ADDON::TranslateContent(m_scraper->Content()).c_str(), m_scraper->Version().str.c_str());
  
  vector<CStdString> extras;
  extras.push_back(strAlbum);
  extras.push_back(m_strArtist);
  g_charsetConverter.utf8To(m_scraper->GetParser().GetSearchStringEncoding(), strAlbum, extras[0]);
  g_charsetConverter.utf8To(m_scraper->GetParser().GetSearchStringEncoding(), m_strArtist, extras[1]);
  CURL::Encode(extras[0]);
  CURL::Encode(extras[1]);

  CScraperUrl scrURL;
  vector<CStdString> url = m_scraper->Run("CreateAlbumSearchUrl",scrURL,m_http,&extras);
  if (url.empty())
    return;
  scrURL.ParseString(url[0]);
  vector<CStdString> xml = m_scraper->Run("GetAlbumSearchResults",scrURL,m_http);

  for (vector<CStdString>::iterator it  = xml.begin();
                                    it != xml.end(); ++it)
  {
    // ok, now parse the xml file
    TiXmlDocument doc;
    doc.Parse(it->c_str(),0,TIXML_ENCODING_UTF8);
    TiXmlHandle docHandle( &doc );
    TiXmlElement* album = docHandle.FirstChild( "results" ).FirstChild( "entity" ).Element();

    while (album)
    {
      TiXmlNode* title = album->FirstChild("title");
      TiXmlElement* link = album->FirstChildElement("url");
      TiXmlNode* artist = album->FirstChild("artist");
      TiXmlNode* year = album->FirstChild("year");
      TiXmlElement* relevance = album->FirstChildElement("relevance");
      if (title && title->FirstChild())
      {
        CStdString strTitle = title->FirstChild()->Value();
        CStdString strArtist;
        CStdString strAlbumName;

        if (artist && artist->FirstChild())
        {
          strArtist = artist->FirstChild()->Value();
          strAlbumName.Format("%s - %s",strArtist.c_str(),strTitle.c_str());
        }
        else
          strAlbumName = strTitle;

        if (year && year->FirstChild())
          strAlbumName.Format("%s (%s)",strAlbumName.c_str(),year->FirstChild()->Value());

        CScraperUrl url;
        if (!link)
          url.ParseString(scrURL.m_xml);

        while (link && link->FirstChild())
        {
          url.ParseElement(link);
          link = link->NextSiblingElement("url");
        }
        CMusicAlbumInfo newAlbum(strTitle, strArtist, strAlbumName, url);
        if (relevance && relevance->FirstChild())
        {
          float scale=1;
          const char* newscale = relevance->Attribute("scale");
          if (newscale)
            scale = (float)atof(newscale);
          newAlbum.SetRelevance((float)atof(relevance->FirstChild()->Value())/scale);
        }
        m_vecAlbums.push_back(newAlbum);
      }
      album = album->NextSiblingElement();
    }
  }

  if (m_vecAlbums.size()>0)
    m_bSucceeded=true;

  return;
}

void CMusicInfoScraper::FindArtistInfo()
{
  CStdString strArtist=m_strArtist;
  CStdString strHTML;
  m_vecArtists.erase(m_vecArtists.begin(), m_vecArtists.end());

  if (!m_scraper->Load())
    return;

  vector<CStdString> extras;
  extras.push_back(m_strArtist);
  g_charsetConverter.utf8To(m_scraper->GetParser().GetSearchStringEncoding(), m_strArtist, extras[0]);
  CURL::Encode(extras[0]);
  
  CLog::Log(LOGDEBUG, "%s: Searching for '%s' using %s scraper (file: '%s', content: '%s', version: '%s')",
    __FUNCTION__, m_strArtist.c_str(), m_scraper->Name().c_str(), m_scraper->Path().c_str(),
    ADDON::TranslateContent(m_scraper->Content()).c_str(), m_scraper->Version().str.c_str());

  CScraperUrl scrURL;
  vector<CStdString> url = m_scraper->Run("CreateArtistSearchUrl",scrURL,m_http,&extras);
  if (url.empty())
    return;
  scrURL.ParseString(url[0]);

  vector<CStdString> xml = m_scraper->Run("GetArtistSearchResults",scrURL,m_http,&extras);

  for (vector<CStdString>::iterator it  = xml.begin();
                                    it != xml.end(); ++it)
  {
    // ok, now parse the xml file
    TiXmlDocument doc;
    doc.Parse(it->c_str(),0,TIXML_ENCODING_UTF8);
    if (!doc.RootElement())
    {
      CLog::Log(LOGERROR, "%s: Unable to parse xml",__FUNCTION__);
      continue;
    }

    TiXmlHandle docHandle( &doc );
    TiXmlElement* artist = docHandle.FirstChild( "results" ).FirstChild( "entity" ).Element();

    while (artist)
    {
      TiXmlNode* title = artist->FirstChild("title");
      TiXmlNode* year = artist->FirstChild("year");
      TiXmlNode* genre = artist->FirstChild("genre");
      TiXmlElement* link = artist->FirstChildElement("url");
      if (title && title->FirstChild())
      {
        CStdString strTitle = title->FirstChild()->Value();
        CScraperUrl url;
        if (!link)
          url.ParseString(scrURL.m_xml);
        while (link && link->FirstChild())
        {
          url.ParseElement(link);
          link = link->NextSiblingElement("url");
        }
        CMusicArtistInfo newArtist(strTitle, url);
        if (genre && genre->FirstChild())
          newArtist.GetArtist().strGenre = genre->FirstChild()->Value();
        if (year && year->FirstChild())
          newArtist.GetArtist().strBorn = year->FirstChild()->Value();
        m_vecArtists.push_back(newArtist);
      }
      artist = artist->NextSiblingElement();
    }
  }

  if (m_vecArtists.size()>0)
    m_bSucceeded=true;

  return;
}

void CMusicInfoScraper::LoadAlbumInfo(int iAlbum)
{
  m_iAlbum=iAlbum;
  m_iArtist=-1;
  StopThread();
  Create();
}

void CMusicInfoScraper::LoadArtistInfo(int iArtist)
{
  m_iAlbum=-1;
  m_iArtist=iArtist;
  StopThread();
  Create();
}

void CMusicInfoScraper::LoadAlbumInfo()
{
  if (m_iAlbum<0 || m_iAlbum>=(int)m_vecAlbums.size())
    return;

  CMusicAlbumInfo& album=m_vecAlbums[m_iAlbum];
  album.GetAlbum().strArtist.Empty();
  if (album.Load(m_http,m_scraper))
    m_bSucceeded=true;
}

void CMusicInfoScraper::LoadArtistInfo()
{
  if (m_iArtist<0 || m_iArtist>=(int)m_vecArtists.size())
    return;

  CMusicArtistInfo& artist=m_vecArtists[m_iArtist];
  artist.GetArtist().strArtist.Empty();
  if (artist.Load(m_http,m_scraper))
    m_bSucceeded=true;
}

bool CMusicInfoScraper::Completed()
{
  return WaitForThreadExit(10);
}

bool CMusicInfoScraper::Succeeded()
{
  return !m_bCanceled && m_bSucceeded;
}

void CMusicInfoScraper::Cancel()
{
  m_http.Cancel();
  m_bCanceled=true;
  m_http.Reset();
}

bool CMusicInfoScraper::IsCanceled()
{
  return m_bCanceled;
}

void CMusicInfoScraper::OnStartup()
{
  m_bSucceeded=false;
  m_bCanceled=false;
}

void CMusicInfoScraper::Process()
{
  try
  {
    if (m_strAlbum.size())
    {
      FindAlbumInfo();
      m_strAlbum.Empty();
      m_strArtist.Empty();
    }
    else if (m_strArtist.size())
    {
      FindArtistInfo();
      m_strArtist.Empty();
    }
    if (m_iAlbum>-1)
    {
      LoadAlbumInfo();
      m_iAlbum=-1;
    }
    if (m_iArtist>-1)
    {
      LoadArtistInfo();
      m_iArtist=-1;
    }
  }
  catch(...)
  {
    CLog::Log(LOGERROR, "Exception in CMusicInfoScraper::Process()");
  }
}

bool CMusicInfoScraper::CheckValidOrFallback(const CStdString &fallbackScraper)
{
  return true;
/*
 * TODO handle fallback mechanism
  if (m_scraper->Path() != fallbackScraper &&
      parser.Load("special://xbmc/system/scrapers/music/" + fallbackScraper))
  {
    CLog::Log(LOGWARNING, "%s - scraper %s fails to load, falling back to %s", __FUNCTION__, m_info.strPath.c_str(), fallbackScraper.c_str());
    m_info.strPath = fallbackScraper;
    m_info.strContent = "albums";
    m_info.strTitle = parser.GetName();
    m_info.strDate = parser.GetDate();
    m_info.strFramework = parser.GetFramework();
    m_info.strLanguage = parser.GetLanguage();
    m_info.settings.LoadSettingsXML("special://xbmc/system/scrapers/music/" + m_info.strPath);
    return true;
  }
  return false; */
}
