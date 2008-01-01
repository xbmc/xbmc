#include "stdafx.h"
#include "musicalbuminfo.h"
#include "ScraperParser.h"
#include "XMLUtils.h"

using namespace MUSIC_GRABBER;

CMusicAlbumInfo::CMusicAlbumInfo(void)
{
  m_strTitle2 = "";
  m_strDateOfRelease = "";
  m_bLoaded = false;
}

CMusicAlbumInfo::~CMusicAlbumInfo(void)
{
}

CMusicAlbumInfo::CMusicAlbumInfo(const CStdString& strAlbumInfo, const CScraperUrl& strAlbumURL)
{
  m_strTitle2 = strAlbumInfo;
  m_strDateOfRelease = "";
  m_albumURL = strAlbumURL;
  m_bLoaded = false;
}

CMusicAlbumInfo::CMusicAlbumInfo(const CStdString& strAlbum, const CStdString& strArtist, const CStdString& strAlbumInfo, const CScraperUrl& strAlbumURL)
{
  m_album.strAlbum = strAlbum;
  m_album.strArtist = strArtist;
  m_strTitle2 = strAlbumInfo;
  m_strDateOfRelease = "";
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
  m_strDateOfRelease.Format("%i", album.iYear);
  m_strTitle2 = "";
  m_bLoaded = true;
}

const VECSONGS &CMusicAlbumInfo::GetSongs() const
{
  return m_songs;
}

void CMusicAlbumInfo::SetSongs(VECSONGS &songs)
{
  m_songs = songs;
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
  return m_strDateOfRelease;
}

bool CMusicAlbumInfo::Parse(const TiXmlElement* album)
{
  XMLUtils::GetString(album,"title",m_album.strAlbum);
  
  CStdString strTemp;
  const TiXmlNode* node = album->FirstChild("artist");
  while (node)
  {
    if (node->FirstChild())
    {
      strTemp = node->FirstChild()->Value();
      if (m_album.strArtist.IsEmpty())
        m_album.strArtist = strTemp;
      else
        m_album.strArtist += g_advancedSettings.m_musicItemSeparator+strTemp;
    }
    node = node->NextSibling("artist");
  }
  node = album->FirstChild("genre");
  while (node)
  {
    if (node->FirstChild())
    {
      strTemp = node->FirstChild()->Value();
      if (m_album.strGenre.IsEmpty())
        m_album.strGenre = strTemp;
      else
        m_album.strGenre += g_advancedSettings.m_musicItemSeparator+strTemp;
    }
    node = node->NextSibling("genre");
  }
  node = album->FirstChild("style");
  while (node)
  {
    if (node->FirstChild())
    {
      strTemp = node->FirstChild()->Value();
      if (m_album.strStyles.IsEmpty())
        m_album.strStyles = strTemp;
      else
        m_album.strStyles += g_advancedSettings.m_musicItemSeparator+strTemp;
    }
    node = node->NextSibling("style");
  }
  node = album->FirstChild("mood");
  while (node)
  {
    if (node->FirstChild())
    {
      strTemp = node->FirstChild()->Value();
      if (m_album.strMoods.IsEmpty())
        m_album.strMoods = strTemp;
      else
        m_album.strMoods += g_advancedSettings.m_musicItemSeparator+strTemp;
    }
    node = node->NextSibling("mood");
  }
  node = album->FirstChild("theme");
  while (node)
  {
    if (node->FirstChild())
    {
      strTemp = node->FirstChild()->Value();
      if (m_album.strThemes.IsEmpty())
        m_album.strThemes = strTemp;
      else
        m_album.strThemes += g_advancedSettings.m_musicItemSeparator+strTemp;
    }
    node = node->NextSibling("theme");
  }

  m_songs.clear();
  node = album->FirstChild("track");
  while (node)
  {
    if (node->FirstChild())
    {
      CSong song;
      XMLUtils::GetInt(node,"position",song.iTrack);
      XMLUtils::GetString(node,"title",song.strTitle);
      CStdString strDur;
      XMLUtils::GetString(node,"duration",strDur);
      song.iDuration = StringUtils::TimeStringToSeconds(strDur);
      m_songs.push_back(song);
    }
    node = node->NextSibling("track");
  }

  XMLUtils::GetString(album,"review",m_album.strReview);
  XMLUtils::GetString(album,"releasedate",m_strDateOfRelease);
  XMLUtils::GetInt(album,"year",m_album.iYear);
  XMLUtils::GetInt(album,"rating",m_album.iRating);

  m_album.thumbURL.ParseElement(album->FirstChildElement("thumbs"));
  if (m_album.thumbURL.m_url.size() == 0)
  {
    if (album->FirstChildElement("thumb") && !album->FirstChildElement("thumb")->FirstChildElement())
    {
      if (album->FirstChildElement("thumb")->FirstChild() && strncmp(album->FirstChildElement("thumb")->FirstChild()->Value(),"<thumb>",7) == 0)
      {
        CStdString strValue = album->FirstChildElement("thumb")->FirstChild()->Value();
        TiXmlDocument doc;
        doc.Parse(strValue.c_str());
        if (doc.FirstChildElement("thumbs"))
          m_album.thumbURL.ParseElement(doc.FirstChildElement("thumbs"));
        else
          m_album.thumbURL.ParseElement(doc.FirstChildElement("thumb"));
      }
      else
        m_album.thumbURL.ParseElement(album->FirstChildElement("thumb"));
    }
    else
      m_album.thumbURL.ParseElement(album->FirstChildElement("thumb"));
  }
  
  if (m_strTitle2.IsEmpty()) 
    m_strTitle2 = m_album.strAlbum;

  SetLoaded(true);

  return true;
}


bool CMusicAlbumInfo::Load(CHTTP& http, const SScraperInfo& info, const CStdString& strFunction, const CScraperUrl* url)
{
  // load our scraper xml
  CScraperParser parser;
  if (!parser.Load("q:\\system\\scrapers\\music\\"+info.strPath))
    return false;

  if (!url)
    url = &GetAlbumURL();

  std::vector<CStdString> strHTML;
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
  if (strXML.Find("encoding=\"utf-8\"") < 0)
    g_charsetConverter.stringCharsetToUtf8(strXML);

    // ok, now parse the xml file
  TiXmlBase::SetCondenseWhiteSpace(false);
  TiXmlDocument doc;
  doc.Parse(strXML.c_str(),0,TIXML_ENCODING_UTF8);
  if (!doc.RootElement())
  {
    CLog::Log(LOGERROR, "%s: Unable to parse xml",__FUNCTION__);
    return false;
  }

  bool ret = Parse(doc.RootElement());
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
