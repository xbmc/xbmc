/*
 *      Copyright (C) 2005-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "XMLUtils.h"
#include "ScraperUrl.h"
#include "settings/AdvancedSettings.h"
#include "HTMLUtil.h"
#include "CharsetConverter.h"
#include "URL.h"
#include "filesystem/CurlFile.h"
#include "filesystem/ZipFile.h"
#include "URIUtils.h"

#include <cstring>
#include <sstream>

using namespace std;

CScraperUrl::CScraperUrl(const CStdString& strUrl)
{
  relevance = 0;
  ParseString(strUrl);
}

CScraperUrl::CScraperUrl(const TiXmlElement* element)
{
  relevance = 0;
  ParseElement(element);
}

CScraperUrl::CScraperUrl()
{
  relevance = 0;
}

CScraperUrl::~CScraperUrl()
{
}

void CScraperUrl::Clear()
{
  m_url.clear();
  m_spoof.clear();
  m_xml.clear();
  relevance = 0;
}

bool CScraperUrl::Parse()
{
  CStdString strToParse = m_xml;
  m_xml.Empty();
  return ParseString(strToParse);
}

bool CScraperUrl::ParseElement(const TiXmlElement* element)
{
  if (!element || !element->FirstChild() ||
      !element->FirstChild()->Value()) return false;

  stringstream stream;
  stream << *element;
  m_xml += stream.str();

  SUrlEntry url;
  url.m_url = element->FirstChild()->Value();
  const char* pSpoof = element->Attribute("spoof");
  if (pSpoof)
    url.m_spoof = pSpoof;
  const char* szPost=element->Attribute("post");
  if (szPost && stricmp(szPost,"yes") == 0)
    url.m_post = true;
  else
    url.m_post = false;
  const char* szIsGz=element->Attribute("gzip");
  if (szIsGz && stricmp(szIsGz,"yes") == 0)
    url.m_isgz = true;
  else
    url.m_isgz = false;
  const char* pCache = element->Attribute("cache");
  if (pCache)
    url.m_cache = pCache;

  const char* szType = element->Attribute("type");
  url.m_type = URL_TYPE_GENERAL;
  url.m_season = -1;
  if (szType && stricmp(szType,"season") == 0)
  {
    url.m_type = URL_TYPE_SEASON;
    const char* szSeason = element->Attribute("season");
    if (szSeason)
      url.m_season = atoi(szSeason);
  }
  const char *aspect = element->Attribute("aspect");
  if (aspect)
    url.m_aspect = aspect;

  m_url.push_back(url);

  return true;
}

bool CScraperUrl::ParseString(CStdString strUrl)
{
  if (strUrl.IsEmpty())
    return false;

  // ok, now parse the xml file
  if (!XMLUtils::HasUTF8Declaration(strUrl))
    g_charsetConverter.unknownToUTF8(strUrl);

  CXBMCTinyXML doc;
  doc.Parse(strUrl.c_str(),0,TIXML_ENCODING_UTF8);

  TiXmlElement* pElement = doc.RootElement();
  if (!pElement)
  {
    SUrlEntry url;
    url.m_url = strUrl;
    url.m_type = URL_TYPE_GENERAL;
    url.m_season = -1;
    url.m_post = false;
    url.m_isgz = false;
    m_url.push_back(url);
    m_xml = strUrl;
  }
  else
  {
    while (pElement)
    {
      ParseElement(pElement);
      pElement = pElement->NextSiblingElement(pElement->Value());
    }
  }

  return true;
}

const CScraperUrl::SUrlEntry CScraperUrl::GetFirstThumb(const std::string &type) const
{
  for (vector<SUrlEntry>::const_iterator iter=m_url.begin();iter != m_url.end();++iter)
  {
    if (iter->m_type == URL_TYPE_GENERAL && (type.empty() || type == "thumb" || iter->m_aspect == type))
      return *iter;
  }

  SUrlEntry result;
  result.m_type = URL_TYPE_GENERAL;
  result.m_post = false;
  result.m_isgz = false;
  result.m_season = -1;
  return result;
}

const CScraperUrl::SUrlEntry CScraperUrl::GetSeasonThumb(int season, const std::string &type) const
{
  for (vector<SUrlEntry>::const_iterator iter=m_url.begin();iter != m_url.end();++iter)
  {
    if (iter->m_type == URL_TYPE_SEASON && iter->m_season == season &&
       (type.empty() || type == "thumb" || iter->m_aspect == type))
      return *iter;
  }

  SUrlEntry result;
  result.m_type = URL_TYPE_GENERAL;
  result.m_post = false;
  result.m_isgz = false;
  result.m_season = -1;
  return result;
}

unsigned int CScraperUrl::GetMaxSeasonThumb() const
{
  unsigned int maxSeason = 0;
  for (vector<SUrlEntry>::const_iterator iter=m_url.begin();iter != m_url.end();++iter)
  {
    if (iter->m_type == URL_TYPE_SEASON && iter->m_season > 0 && (unsigned int)iter->m_season > maxSeason)
      maxSeason = iter->m_season;
  }
  return maxSeason;
}

bool CScraperUrl::Get(const SUrlEntry& scrURL, std::string& strHTML, XFILE::CCurlFile& http, const CStdString& cacheContext)
{
  CURL url(scrURL.m_url);
  http.SetReferer(scrURL.m_spoof);
  CStdString strCachePath;

  if (scrURL.m_isgz)
    http.SetContentEncoding("gzip");

  if (!scrURL.m_cache.IsEmpty())
  {
    URIUtils::AddFileToFolder(g_advancedSettings.m_cachePath,
                              "scrapers/"+cacheContext+"/"+scrURL.m_cache,
                              strCachePath);
    if (XFILE::CFile::Exists(strCachePath))
    {
      XFILE::CFile file;
      if (file.Open(strCachePath))
      {
        char* temp = new char[(int)file.GetLength()];
        file.Read(temp,file.GetLength());
        strHTML.clear();
        strHTML.append(temp,temp+file.GetLength());
        file.Close();
        delete[] temp;
        return true;
      }
    }
  }

  CStdString strHTML1(strHTML);

  if (scrURL.m_post)
  {
    CStdString strOptions = url.GetOptions();
    strOptions = strOptions.substr(1);
    url.SetOptions("");

    if (!http.Post(url.Get(), strOptions, strHTML1))
      return false;
  }
  else
    if (!http.Get(url.Get(), strHTML1))
      return false;

  strHTML = strHTML1;

  if (scrURL.m_url.Find(".zip") > -1 )
  {
    XFILE::CZipFile file;
    CStdString strBuffer;
    int iSize = file.UnpackFromMemory(strBuffer,strHTML,scrURL.m_isgz);
    if (iSize)
    {
      strHTML.clear();
      strHTML.append(strBuffer.c_str(),strBuffer.data()+iSize);
    }
  }

  if (!scrURL.m_cache.IsEmpty())
  {
    CStdString strCachePath;
    URIUtils::AddFileToFolder(g_advancedSettings.m_cachePath,
                              "scrapers/"+cacheContext+"/"+scrURL.m_cache,
                              strCachePath);
    XFILE::CFile file;
    if (file.OpenForWrite(strCachePath,true))
      file.Write(strHTML.data(),strHTML.size());
    file.Close();
  }
  return true;
}

// XML format is of strUrls is:
// <TAG><url>...</url>...</TAG> (parsed by ParseElement) or <url>...</url> (ditto)
bool CScraperUrl::ParseEpisodeGuide(CStdString strUrls)
{
  if (strUrls.IsEmpty())
    return false;

  // ok, now parse the xml file
  if (!XMLUtils::HasUTF8Declaration(strUrls))
    g_charsetConverter.unknownToUTF8(strUrls);

  CXBMCTinyXML doc;
  doc.Parse(strUrls.c_str(),0,TIXML_ENCODING_UTF8);
  if (doc.RootElement())
  {
    TiXmlHandle docHandle( &doc );
    TiXmlElement *link = docHandle.FirstChild("episodeguide").Element();
    if (link->FirstChildElement("url"))
    {
      for (link = link->FirstChildElement("url"); link; link = link->NextSiblingElement("url"))
        ParseElement(link);
    }
    else if (link->FirstChild() && link->FirstChild()->Value())
      ParseElement(link);
  }
  else
    return false;

  return true;
}

CStdString CScraperUrl::GetThumbURL(const CScraperUrl::SUrlEntry &entry)
{
  if (entry.m_spoof.IsEmpty())
    return entry.m_url;
  CStdString spoof = entry.m_spoof;
  CURL::Encode(spoof);
  return entry.m_url + "|Referer=" + spoof;
}

void CScraperUrl::GetThumbURLs(std::vector<CStdString> &thumbs, const std::string &type, int season) const
{
  for (vector<SUrlEntry>::const_iterator iter = m_url.begin(); iter != m_url.end(); ++iter)
  {
    if (iter->m_aspect == type || type.empty() || type == "thumb")
    {
      if ((iter->m_type == CScraperUrl::URL_TYPE_GENERAL && season == -1)
       || (iter->m_type == CScraperUrl::URL_TYPE_SEASON && iter->m_season == season))
      {
        thumbs.push_back(GetThumbURL(*iter));
      }
    }
  }
}
