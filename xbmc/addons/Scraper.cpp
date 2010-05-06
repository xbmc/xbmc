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
#include "Scraper.h"
#include "XMLUtils.h"
#include "FileSystem/File.h"
#include "FileSystem/Directory.h"
#include "FileSystem/FileCurl.h"
#include "AddonManager.h"
#include "utils/ScraperParser.h"
#include "utils/ScraperUrl.h"
#include "utils/CharsetConverter.h"
#include "utils/log.h"

#include <sstream>

using std::vector;
using std::stringstream;

namespace ADDON
{

class CAddon;

AddonPtr CScraper::Clone(const AddonPtr &self) const
{
  return AddonPtr(new CScraper(*this, self));
}

CScraper::CScraper(const CScraper &rhs, const AddonPtr &self)
  : CAddon(rhs, self)
{
}

bool CScraper::LoadSettings()
{
  //TODO if cloned settings don't exist, load master settings and copy
  if (!Parent())
    return CAddon::LoadUserSettings();
  else
    return LoadSettingsXML();
}

bool CScraper::HasSettings()
{
  if (!m_userXmlDoc.RootElement())
    return LoadSettingsXML();

  return true;
}

bool CScraper::Load(const CStdString& strSettings, const CStdString& strSaved)
{
  if (!LoadSettingsXML(strSettings))
    return false;
  if (!LoadUserXML(strSaved))
    return false;

  return true;
}

bool CScraper::LoadUserXML(const CStdString& strSaved)
{
  m_userXmlDoc.Clear();
  m_userXmlDoc.Parse(strSaved.c_str());

  return m_userXmlDoc.RootElement()?true:false;
}

bool CScraper::LoadSettingsXML(const CStdString& strFunction, const CScraperUrl* url)
{
  AddonPtr addon;
  if (!Parent() && !CAddonMgr::Get().GetAddon(ID(), addon, ADDON_SCRAPER))
    return false;
  else if (Parent())
    addon = Parent();

  CScraperParser parser;
  if (!parser.Load(addon))
    return false;

  if (!parser.HasFunction(strFunction))
    return CAddon::LoadSettings();

  if (!url && strFunction.Equals("GetSettings")) // entry point
    m_addonXmlDoc.Clear();

  vector<CStdString> strHTML;
  if (url)
  {
    XFILE::CFileCurl http;
    for (unsigned int i=0;i<url->m_url.size();++i)
    {
      CStdString strCurrHTML;
      if (!CScraperUrl::Get(url->m_url[i],strCurrHTML,http,parser.GetFilename()) || strCurrHTML.size() == 0)
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

  // check our document
  if (!m_addonXmlDoc.RootElement())
  {
    TiXmlElement xmlRootElement("settings");
    m_addonXmlDoc.InsertEndChild(xmlRootElement);
  }

  // loop over all tags and append any setting tags
  TiXmlElement* pElement = doc.RootElement()->FirstChildElement("setting");
  if (pElement)
    m_hasSettings = true;
  else
    m_hasSettings = false;

  while (pElement)
  {
    m_addonXmlDoc.RootElement()->InsertEndChild(*pElement);
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
      if (!LoadSettingsXML(szFunction,&scrURL))
        return false;
    }
    xurl = xurl->NextSiblingElement("url");
  }

  return m_addonXmlDoc.RootElement()?true:false;
}

CStdString CScraper::GetSettings() const
{
  stringstream stream;
  if (m_userXmlDoc.RootElement())
    stream << *m_userXmlDoc.RootElement();

  return stream.str();
}

}; /* namespace ADDON */

