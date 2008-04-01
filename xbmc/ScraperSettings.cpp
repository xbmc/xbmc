#include "stdafx.h"
#include "ScraperSettings.h"
#include "FileSystem/File.h"
#include "FileSystem/Directory.h"
#include "utils/HTTP.h"
#include  "utils/ScraperParser.h"

#include <sstream>

CScraperSettings::CScraperSettings()
{
}

CScraperSettings::~CScraperSettings()
{
}

bool CScraperSettings::Load(const CStdString& strSettings, const CStdString& strSaved)
{
  if (!LoadSettingsXML(strSettings))
    return false;
  if (!LoadUserXML(strSaved))
    return false;

  return true;
}

bool CScraperSettings::LoadUserXML(const CStdString& strSaved)
{
  m_userXmlDoc.Clear();
  m_userXmlDoc.Parse(strSaved.c_str());

  return m_userXmlDoc.RootElement()?true:false;
}

bool CScraperSettings::LoadSettingsXML(const CStdString& strScraper, const CStdString& strFunction, const CScraperUrl* url)
{
  CScraperParser parser;
  // load our scraper xml
  if (!parser.Load(strScraper))
    return false;

  if (!url && strFunction.Equals("GetSettings")) // entry point
    m_pluginXmlDoc.Clear();
    
  std::vector<CStdString> strHTML;
  if (url)
  {
    CHTTP http;
    for (unsigned int i=0;i<url->m_url.size();++i)
    {
      CStdString strCurrHTML;
      if (!CScraperUrl::Get(url->m_url[i],strCurrHTML,http) || strCurrHTML.size() == 0)
        return false;
      strHTML.push_back(strCurrHTML);
    }
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

  // check our document
  if (!m_pluginXmlDoc.RootElement())
  {
    TiXmlElement xmlRootElement("settings");
    m_pluginXmlDoc.InsertEndChild(xmlRootElement);
  }

  // loop over all tags and append any setting tags
  TiXmlElement* pElement = doc.RootElement()->FirstChildElement("setting");
  while (pElement)
  {
    m_pluginXmlDoc.RootElement()->InsertEndChild(*pElement);
    pElement = pElement->NextSiblingElement("setting");
  }

  // and call any chains
  TiXmlElement* pRoot = doc.RootElement();
  TiXmlElement* xurl = pRoot->FirstChildElement("url");
  while (xurl && xurl->FirstChild())
  {
    const char* szFunction = xurl->Attribute("function");
    if (szFunction)
    {      
      CScraperUrl scrURL(xurl);
      LoadSettingsXML(strScraper,szFunction,&scrURL);
    }
    xurl = xurl->NextSiblingElement("url");
  }

  return m_pluginXmlDoc.RootElement()?true:false;
}

CStdString CScraperSettings::GetSettings() const
{
  std::stringstream stream;
  if (m_userXmlDoc.RootElement())
    stream << *m_userXmlDoc.RootElement();

  return stream.str();
}

