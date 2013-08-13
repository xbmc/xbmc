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

void CHttpHeader::Parse(std::string strData)
{
  size_t pos = 0;
  size_t iValueStart = 0;
  size_t iValueEnd = 0;

  std::string strParam;
  std::string strValue;

  while (pos < strData.length())
  {
    iValueStart = strData.find(':', pos);
    iValueEnd = strData.find("\r\n", pos);

    if (iValueEnd == std::string::npos)
      break;

    if (iValueStart != std::string::npos && iValueStart < iValueEnd)
    {
      strParam.assign(strData, pos, iValueStart - pos);
      strValue.assign(strData, iValueStart + 1, iValueEnd - iValueStart - 1);

      /*
      CUtil::Lower(strParam.c_str()
      // trim left and right
      {
        string::size_type pos = strValue.find_last_not_of(' ');
        if(pos != string::npos)
        {
          strValue.erase(pos + 1);
          pos = strValue.find_first_not_of(' ');
          if(pos != string::npos) strValue.erase(0, pos);
        }
        else strValue.erase(strValue.begin(), strValue.end());
      }*/
      StringUtils::Trim(strParam);
      StringUtils::ToLower(strParam);

      StringUtils::Trim(strValue);

      m_params.insert(HeaderParams::value_type(strParam, strValue));
    }
    else if (m_protoLine.empty())
      m_protoLine = strData;


    pos = iValueEnd + 2;
  }
}

std::string CHttpHeader::GetValue(std::string strParam) const
{
  StringUtils::ToLower(strParam);

  HeaderParams::const_iterator pIter = m_params.find(strParam);
  if (pIter != m_params.end()) return pIter->second;

  return "";
}

void CHttpHeader::GetHeader(std::string& strHeader) const
{
  strHeader.clear();

  HeaderParams::const_iterator iter = m_params.begin();
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
