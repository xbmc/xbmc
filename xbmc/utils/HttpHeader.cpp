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

#include "HttpHeader.h"
#include "utils/StringUtils.h"

CHttpHeader::CHttpHeader()
{
  m_params.clear();
}

CHttpHeader::~CHttpHeader()
{
}

void CHttpHeader::Parse(CStdString strData)
{
  unsigned int iIter = 0;
  size_t iValueStart = 0;
  size_t iValueEnd = 0;

  CStdString strParam;
  CStdString strValue;

  while (iIter < strData.size())
  {
    iValueStart = strData.find(":", iIter);
    iValueEnd = strData.find("\r\n", iIter);

    if (iValueEnd == std::string::npos) break;

    if (iValueStart > 0)
    {
      strParam = strData.substr(iIter, iValueStart - iIter);
      strValue = strData.substr(iValueStart + 1, iValueEnd - iValueStart - 1);

      StringUtils::Trim(strParam);
      StringUtils::ToLower(strParam);

      StringUtils::Trim(strValue);

      m_params[strParam] = strValue;
    }
    else if (m_protoLine.empty())
      m_protoLine = strData;


    iIter = iValueEnd + 2;
  }
}

CStdString CHttpHeader::GetValue(CStdString strParam) const
{
  StringUtils::ToLower(strParam);

  std::map<CStdString,CStdString>::const_iterator pIter = m_params.find(strParam);
  if (pIter != m_params.end()) return pIter->second;

  return "";
}

void CHttpHeader::GetHeader(CStdString& strHeader) const
{
  strHeader.clear();

  std::map<CStdString,CStdString>::const_iterator iter = m_params.begin();
  while (iter != m_params.end())
  {
    strHeader += ((*iter).first + ": " + (*iter).second + "\n");
    iter++;
  }

  strHeader += "\n";
}

void CHttpHeader::Clear()
{
  m_params.clear();
}
