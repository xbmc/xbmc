#include "stdafx.h"
#include "MusicArtistInfo.h"
#include "ScraperParser.h"
#include "XMLUtils.h"

using namespace MUSIC_GRABBER;

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

  m_artist.Load(artist);

  SetLoaded(true);

  return true;
}

bool CMusicArtistInfo::Load(CHTTP& http, const SScraperInfo& info, const CStdString& strFunction, const CScraperUrl* url)
{
  // load our scraper xml
  CScraperParser parser;
  if (!parser.Load("q:\\system\\scrapers\\music\\"+info.strPath))
    return false;

  bool bChained=true;
  if (!url)
  {
    bChained=false;
    url = &GetArtistURL();
    CScraperParser::ClearCache();
  }

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
