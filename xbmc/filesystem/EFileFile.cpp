/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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

#include "EFileFile.h"
#include "URL.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/XMLUtils.h"

using namespace XFILE;

CEFileFile::CEFileFile(void)
  : COverrideFile(true)
{ }

CEFileFile::~CEFileFile(void)
{ }

bool CEFileFile::Exists(const CURL& url)
{
  return CFile::Exists(GetOriginalPath(url));
}

void CEFileFile::GetMessages(const CURL& url, std::string& title, std::string& message)
{
  GetXMLString(url, "title", title);
  GetXMLString(url, "message", message);
}

std::string CEFileFile::GetOriginalPath(const CURL& url)
{
  return URIUtils::AddFileToFolder(url.GetHostName(), url.GetFileName());
}

std::string CEFileFile::GetTranslatedPath(const CURL& url)
{
  return GetXMLString(url, "path");
}

std::string CEFileFile::GetXMLString(const CURL& url, std::string strXMLTag)
{
  std::string strValue;
  GetXMLString(url, strXMLTag, strValue);
  return strValue;
}

void CEFileFile::GetXMLString(const CURL& url, std::string strXMLTag, std::string& strValue)
{
  CXBMCTinyXML stubXML;
  std::string strFilename(GetOriginalPath(url));
  if (stubXML.LoadFile(strFilename))
  {
    TiXmlElement * pRootElement = stubXML.RootElement();
    if (pRootElement)
    {
      if (!strXMLTag.empty()) {
        XMLUtils::GetString(pRootElement, strXMLTag.c_str(), strValue);
      }
      return;
    }
  }
  CLog::Log(LOGERROR, "Error loading %s", strFilename.c_str());
}

std::string CEFileFile::TranslatePath(const CURL& url)
{
  return GetTranslatedPath(url);
}
