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
#include "ProgramInfo.h"
#include "ScraperParser.h"
#include "ScraperSettings.h"
#include "XMLUtils.h"
#include "HTMLTable.h"
#include "HTMLUtil.h"
#include "Util.h"

using namespace PROGRAM_GRABBER;
using namespace std;

CProgramInfo::CProgramInfo(void)
{
  m_bLoaded = false;
}

CProgramInfo::~CProgramInfo(void)
{
}

CProgramInfo::CProgramInfo(const CStdString& strProgramInfo, const CScraperUrl& strAlbumURL)
{
  m_program.m_strTitle = strProgramInfo;
  m_programURL = strAlbumURL;
  m_bLoaded = false;
}

CProgramInfo::CProgramInfo(const CStdString& strTitle, const CStdString strPlatform, const CStdString strYear, const CScraperUrl& strProgramURL)
{
  m_program.m_strTitle = strTitle;
  m_program.m_strPlatform = strPlatform;
  m_program.m_strYear = strYear;
  m_programURL = strProgramURL;
  m_bLoaded = false;
}

const CProgramInfoTag& CProgramInfo::GetProgram() const
{
  return m_program;
}

CProgramInfoTag& CProgramInfo::GetProgram()
{
  return m_program;
}

void CProgramInfo::SetProgram(CProgramInfoTag& program)
{
  m_program = program;
  m_bLoaded = true;
}

const CScraperUrl& CProgramInfo::GetProgramURL() const
{
  return m_programURL;
}

bool CProgramInfo::Parse(const TiXmlElement* program, bool bChained)
{
  if (!m_program.Load(program,bChained))
    return false;

  SetLoaded(true);
  return true;
}

bool CProgramInfo::Load(XFILE::CFileCurl& http, const SScraperInfo& info, const CStdString& strFunction, const CScraperUrl* url)
{
  CScraperParser parser;

  // load our scraper xml
  if (!parser.Load("special://xbmc/system/scrapers/programs/" + info.strPath))
    return false;

  bool bChained=true;
  if (!url)
  {
    bChained=false;
    url = &GetProgramURL();
    CScraperParser::ClearCache();
  }

  vector<CStdString> strHTML;
  for (unsigned int i=0;i<url->m_url.size();++i)
  {
    CStdString strCurrHTML;
    // XXX - this hack is needed so parameters with spaces will pass correctly to the web site 
    CScraperUrl::SUrlEntry sUrl = url->m_url[i];
    sUrl.m_url.Replace(" ","%20");
    if (!CScraperUrl::Get(sUrl,strCurrHTML,http) || strCurrHTML.size() == 0)
      return false;
    strHTML.push_back(strCurrHTML);
  }


  // now grab our details using the scraper
  for (unsigned int i=0;i<strHTML.size();++i)
  {
    CLog::Log(LOGINFO, "%s",strHTML[i].c_str());
    parser.m_param[i] = strHTML[i];
  }

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

void CProgramInfo::SetLoaded(bool bOnOff)
{
  m_bLoaded = bOnOff;
}

bool CProgramInfo::Loaded() const
{
  return m_bLoaded;
}


