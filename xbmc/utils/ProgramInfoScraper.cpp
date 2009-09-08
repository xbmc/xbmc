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
#include "XMLUtils.h"
#include "ProgramInfoScraper.h"
#include "HTMLUtil.h"
#include "HTMLTable.h"
#include "Util.h"
#include "ScraperParser.h"
#include "utils/log.h"
#include "utils/CharsetConverter.h"

using namespace PROGRAM_GRABBER;
using namespace HTML;

CProgramInfoScraper::CProgramInfoScraper(const SScraperInfo& info)
{
  m_bSuccessfull=false;
  m_bCanceled=false;
  m_info = info;
  m_iProgram=-1;
}

CProgramInfoScraper::~CProgramInfoScraper(void)
{
}

void CProgramInfoScraper::OnStartup()
{
  m_bSuccessfull=false;
  m_bCanceled=false;
}

bool CProgramInfoScraper::Completed()
{
  return WaitForThreadExit(10);
}

bool CProgramInfoScraper::Successfull()
{
  return !m_bCanceled && m_bSuccessfull;
}

void CProgramInfoScraper::Cancel()
{
  m_http.Cancel();
  StopThread();
  m_bCanceled=true;
  m_http.Reset();
}

bool CProgramInfoScraper::IsCanceled()
{
  return m_bCanceled;
}


void CProgramInfoScraper::FindProgramInfo(const CStdString& strProgram)
{
  m_strProgram=strProgram;
  m_bSuccessfull=false;
  StopThread();
  Create();
}

void CProgramInfoScraper::FindProgramInfo()
{
  CStdString strProgram=m_strProgram;
  CStdString strHTML;
  m_vecPrograms.erase(m_vecPrograms.begin(), m_vecPrograms.end());

  CScraperParser parser;
  if (!parser.Load("special://xbmc/system/scrapers/programs/" + m_info.strPath))
    return;

  if (!m_info.settings.GetPluginRoot() || m_info.settings.GetSettings().IsEmpty())
  {
    m_info.settings.LoadSettingsXML("special://xbmc/system/scrapers/programs/" + m_info.strPath);
    m_info.settings.SaveFromDefault();
  }

  parser.m_param[0] = strProgram;
  CUtil::URLEncode(parser.m_param[0]);

  CScraperUrl scrURL;
  scrURL.ParseString(parser.Parse("CreateProgramSearchUrl"));
  if (!CScraperUrl::Get(scrURL.m_url[0], strHTML, m_http) || strHTML.size() == 0)
  {
    CLog::Log(LOGERROR, "%s: Unable to retrieve web site",__FUNCTION__);
    return;
  }

  parser.m_param[0] = strHTML;
  CStdString strXML = parser.Parse("GetProgramSearchResults",&m_info.settings);



    if (strXML.IsEmpty())
  {
    CLog::Log(LOGERROR, "%s: Unable to parse web site",__FUNCTION__);
    return;
  }

  if (!XMLUtils::HasUTF8Declaration(strXML))
    g_charsetConverter.unknownToUTF8(strXML);

  // ok, now parse the xml file
  TiXmlDocument doc;
  doc.Parse(strXML.c_str(),0,TIXML_ENCODING_UTF8);
  if (!doc.RootElement())
  {
    CLog::Log(LOGERROR, "%s: Unable to parse xml",__FUNCTION__);
    return;
  }
  TiXmlHandle docHandle( &doc );
  TiXmlElement* program = docHandle.FirstChild( "results" ).FirstChild( "entity" ).Element();
  if (!program)
    return;

  while (program)
  {
    TiXmlNode* title = program->FirstChild("title");
    TiXmlNode* platform = program->FirstChild("platform");
    TiXmlNode* year = program->FirstChild("year");
    TiXmlElement* link = program->FirstChildElement("url");
    
    if (title && title->FirstChild())
    {
      CStdString strTitle = title->FirstChild()->Value();
      CStdString strPlatform = "";
      CStdString strYear = "";
      

      if (platform && platform->FirstChild())
      {
        strPlatform = platform->FirstChild()->Value();
      }
      if (year && year->FirstChild())
      {
        strYear = year->FirstChild()->Value();
      }

      CScraperUrl url;
      if (!link)
        url.ParseString(scrURL.m_xml);

      while (link && link->FirstChild())
      {
        url.ParseElement(link);
        link = link->NextSiblingElement("url");
      }

      CProgramInfo newProgram(strTitle, strPlatform, strYear, url);
      m_vecPrograms.push_back(newProgram);
    }
    program = program->NextSiblingElement();
  }

  if (m_vecPrograms.size()>0)
    m_bSuccessfull=true;

  return;

}

std::vector<CProgramInfo> CProgramInfoScraper::GetPrograms()
{
   return m_vecPrograms;
}

void CProgramInfoScraper::LoadProgramInfo(int iProgram)
{
  m_iProgram=iProgram;
  StopThread();
  Create();
}

void CProgramInfoScraper::LoadProgramInfo()
{
  if (m_iProgram<0 || m_iProgram>=(int)m_vecPrograms.size())
    return;

  CProgramInfo& program=m_vecPrograms[m_iProgram];
  if (program.Load(m_http,m_info))
    m_bSuccessfull=true;
}

void CProgramInfoScraper::Process()
{
  try
  {
    if (m_strProgram.size())
    {
      FindProgramInfo();
      m_strProgram.Empty();
    }
    if (m_iProgram>-1)
    {
      LoadProgramInfo();
      m_iProgram=-1;
    }
  }
  catch(...)
  {
    CLog::Log(LOGERROR, "Exception in CProgramInfoScraper::Process()");
  }
}

