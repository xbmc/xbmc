/*
 *      Copyright (C) 2013 Team XBMC
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

#include "StubUtil.h"
#include "FileItem.h"
#include "utils/XMLUtils.h"
#include "utils/log.h"
#include "utils/Variant.h"
#include "filesystem/File.h"

CStubUtil g_stubutil;

CStubUtil::CStubUtil(void)
{
}
CStubUtil::~CStubUtil(void)
{
}

bool CStubUtil::IsEfileStub(const std::string strPath)
{
  CFileItem item = CFileItem(strPath, false);
  return item.IsEfileStub();
}

bool CStubUtil::CheckRootElement(const std::string strFilename, std::string strRootElement)
{
  CXBMCTinyXML stubXML;
  if (stubXML.LoadFile(strFilename))
  {
    TiXmlElement * pRootElement = stubXML.RootElement();
    if (pRootElement && strcmpi(pRootElement->Value(), strRootElement.c_str()) == 0)
      return true;
  }
  return false;
}

void CStubUtil::GetXMLString(const std::string strFilename, std::string strRootElement, std::string strXMLTag, std::string& strValue)
{
  CXBMCTinyXML stubXML;
  if (stubXML.LoadFile(strFilename))
  {
    TiXmlElement * pRootElement = stubXML.RootElement();
    if (!pRootElement || strcmpi(pRootElement->Value(), strRootElement.c_str()) != 0)

		CLog::Log(LOGERROR, "Error loading %s, no <%s> node", strFilename.c_str(), strRootElement.c_str());
    else
      XMLUtils::GetString(pRootElement, strXMLTag.c_str(), strValue);
  }
}

std::string CStubUtil::GetXMLString(const std::string strFilename, std::string strRootElement, std::string strXMLTag)
{
  std::string strValue;
  GetXMLString(strFilename, strRootElement, strXMLTag, strValue);
  return strValue;
}
