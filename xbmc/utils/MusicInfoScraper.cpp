#include "stdafx.h"

#include "./MusicInfoScraper.h"
#include "./HTMLUtil.h"
#include "./HTMLTable.h"
#include "../Util.h"
#include "ScraperParser.h"
#include "HTTP.h"

using namespace MUSIC_GRABBER;
using namespace HTML;
CMusicInfoScraper::CMusicInfoScraper(const SScraperInfo& info)
{
  m_bSuccessfull=false;
  m_bCanceled=false;
  m_iAlbum=-1;
  m_iArtist=-1;
  m_info = info;
}

CMusicInfoScraper::~CMusicInfoScraper(void)
{
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

void CMusicInfoScraper::FindAlbuminfo(const CStdString& strAlbum, const CStdString& strArtist /* = "" */)
{
  m_strAlbum=strAlbum;
  m_strArtist=strArtist;
  StopThread();
  Create();
}

void CMusicInfoScraper::FindArtistinfo(const CStdString& strArtist)
{
  m_strArtist=strArtist;
  StopThread();
  Create();
}


void CMusicInfoScraper::FindAlbuminfo()
{
  CStdString strAlbum=m_strAlbum;
  CStdString strHTML;
  m_vecAlbums.erase(m_vecAlbums.begin(), m_vecAlbums.end());

  CScraperParser parser;
  if (!parser.Load("q:\\system\\scrapers\\music\\"+m_info.strPath))
    return;

  parser.m_param[0] = strAlbum;
  parser.m_param[1] = m_strArtist;
  CUtil::URLEncode(parser.m_param[0]);
  CUtil::URLEncode(parser.m_param[1]);

  CScraperUrl scrURL;
  scrURL.ParseString(parser.Parse("CreateAlbumSearchUrl"));
  if (!CScraperUrl::Get(scrURL.m_url[0], strHTML, m_http) || strHTML.size() == 0)
  {
    CLog::Log(LOGERROR, "%s: Unable to retrieve web site",__FUNCTION__);
    return;
  }

  parser.m_param[0] = strHTML;
  CStdString strXML = parser.Parse("GetAlbumSearchResults");
  if (strXML.IsEmpty())
  {
    CLog::Log(LOGERROR, "%s: Unable to parse web site",__FUNCTION__);
    return;
  }

  if (strXML.Find("encoding=\"utf-8\"") < 0)
    g_charsetConverter.stringCharsetToUtf8(strXML);

  // ok, now parse the xml file
  TiXmlDocument doc;
  doc.Parse(strXML.c_str(),0,TIXML_ENCODING_UTF8);
  if (!doc.RootElement())
  {
    CLog::Log(LOGERROR, "%s: Unable to parse xml",__FUNCTION__);
    return;
  }
  TiXmlHandle docHandle( &doc );
  TiXmlElement* album = docHandle.FirstChild( "results" ).FirstChild( "entity" ).Element();
  if (!album)
    return;

  while (album)
  {
    TiXmlNode* title = album->FirstChild("title");
    TiXmlElement* link = album->FirstChildElement("url");
    TiXmlNode* artist = album->FirstChild("artist");
    TiXmlNode* year = album->FirstChild("year");
    TiXmlNode* id = album->FirstChild("id");
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
      m_vecAlbums.push_back(newAlbum);
    }
    album = album->NextSiblingElement();
  }
  
  if (m_vecAlbums.size()>0)
    m_bSuccessfull=true;

  return;
}

void CMusicInfoScraper::FindArtistinfo()
{
  CStdString strArtist=m_strArtist;
  CStdString strHTML;
  m_vecArtists.erase(m_vecArtists.begin(), m_vecArtists.end());

  CScraperParser parser;
  if (!parser.Load("q:\\system\\scrapers\\music\\"+m_info.strPath))
    return;

  parser.m_param[0] = m_strArtist;
  CUtil::URLEncode(parser.m_param[0]);

  CScraperUrl scrURL;
  scrURL.ParseString(parser.Parse("CreateArtistSearchUrl"));
  if (!CScraperUrl::Get(scrURL.m_url[0], strHTML, m_http) || strHTML.size() == 0)
  {
    CLog::Log(LOGERROR, "%s: Unable to retrieve web site",__FUNCTION__);
    return;
  }

  parser.m_param[0] = strHTML;
  CStdString strXML = parser.Parse("GetArtistSearchResults");
  if (strXML.IsEmpty())
  {
    CLog::Log(LOGERROR, "%s: Unable to parse web site",__FUNCTION__);
    return;
  }

  if (strXML.Find("encoding=\"utf-8\"") < 0)
    g_charsetConverter.stringCharsetToUtf8(strXML);

  // ok, now parse the xml file
  TiXmlDocument doc;
  doc.Parse(strXML.c_str(),0,TIXML_ENCODING_UTF8);
  if (!doc.RootElement())
  {
    CLog::Log(LOGERROR, "%s: Unable to parse xml",__FUNCTION__);
    return;
  }

  TiXmlHandle docHandle( &doc );
  TiXmlElement* artist = docHandle.FirstChild( "results" ).FirstChild( "entity" ).Element();
  if (!artist)
    return;

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
  
  if (m_vecArtists.size()>0)
    m_bSuccessfull=true;

  return;
}

void CMusicInfoScraper::LoadAlbuminfo(int iAlbum)
{
  m_iAlbum=iAlbum;
  m_iArtist=-1;
  StopThread();
  Create();
}

void CMusicInfoScraper::LoadArtistinfo(int iArtist)
{
  m_iAlbum=-1;
  m_iArtist=iArtist;
  StopThread();
  Create();
}

void CMusicInfoScraper::LoadAlbuminfo()
{
  if (m_iAlbum<0 || m_iAlbum>(int)m_vecAlbums.size())
    return;

  CMusicAlbumInfo& album=m_vecAlbums[m_iAlbum];
  album.GetAlbum().strArtist.Empty();
  if (album.Load(m_http,m_info))
    m_bSuccessfull=true;
}

void CMusicInfoScraper::LoadArtistinfo()
{
  if (m_iArtist<0 || m_iArtist>(int)m_vecArtists.size())
    return;

  CMusicArtistInfo& artist=m_vecArtists[m_iArtist];
  artist.GetArtist().strArtist.Empty();
  if (artist.Load(m_http,m_info))
    m_bSuccessfull=true;
}

bool CMusicInfoScraper::Completed()
{
  return WaitForThreadExit(10);
}

bool CMusicInfoScraper::Successfull()
{
  return !m_bCanceled && m_bSuccessfull;
}

void CMusicInfoScraper::Cancel()
{
  m_http.Cancel();
  m_bCanceled=true;
}

bool CMusicInfoScraper::IsCanceled()
{
  return m_bCanceled;
}

void CMusicInfoScraper::OnStartup()
{
  m_bSuccessfull=false;
  m_bCanceled=false;
}

void CMusicInfoScraper::Process()
{
  try
  {
    if (m_strAlbum.size())
    {
      FindAlbuminfo();
      m_strAlbum.Empty();
      m_strArtist.Empty();
    }
    else if (m_strArtist.size())
    {
      FindArtistinfo();
      m_strArtist.Empty();
    }
    if (m_iAlbum>-1)
    {
      LoadAlbuminfo();
      m_iAlbum=-1;
    }
    if (m_iArtist>-1)
    {
      LoadArtistinfo();
      m_iArtist=-1;
    }
  }
  catch(...)
  {
    CLog::Log(LOGERROR, "Exception in CMusicInfoScraper::Process()");
  }
}
