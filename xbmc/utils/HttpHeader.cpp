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
  m_headerdone = false;
  m_charsetIsCached = false;
}

CHttpHeader::~CHttpHeader()
{
}

void CHttpHeader::Parse(const std::string& strData)
{
  if (m_headerdone)
    Clear();

  size_t pos = 0;
  const size_t len = strData.length();
  while (pos < len)
  {
    const size_t valueStart = strData.find(':', pos);
    const size_t lineEnd = strData.find("\r\n", pos);

    if (lineEnd == std::string::npos)
      break;

    if (valueStart == pos)
    { 
      /* skip (erroneously) empty parameter */
    }
    else if (lineEnd == pos)
    {
      m_headerdone = true;
      break;
    }
    else if (valueStart != std::string::npos && valueStart < lineEnd)
    {
      std::string strParam(strData, pos, valueStart - pos);
      std::string strValue(strData, valueStart + 1, lineEnd - valueStart - 1);

      StringUtils::Trim(strParam);
      StringUtils::ToLower(strParam);

      StringUtils::Trim(strValue);

      m_params.insert(HeaderParams::value_type(strParam, strValue));
    }
    else if (m_protoLine.empty())
      m_protoLine.assign(strData, pos, lineEnd - pos);

    pos = lineEnd + 2;
  }
}

void CHttpHeader::AddParamWithValue(const std::string& param, const std::string& value)
{
  ClearCached();
  m_params.insert(HeaderParams::value_type(param, value));
}

std::string CHttpHeader::GetValue(std::string strParam) const
{
  StringUtils::ToLower(strParam);

  HeaderParams::const_iterator pIter = m_params.find(strParam);
  if (pIter != m_params.end())
    return pIter->second;

  return "";
}

std::string& CHttpHeader::GetHeader(std::string& strHeader) const
{
  strHeader = m_protoLine + '\n';

  HeaderParams::const_iterator iter;
  for(iter = m_params.begin(); iter != m_params.end(); ++iter)
    strHeader += ((*iter).first + ": " + (*iter).second + '\n');

  strHeader += '\n';
  
  return strHeader;
}

std::string CHttpHeader::GetHeader(void) const
{
  std::string strHeader;
  return GetHeader(strHeader);
}

std::string CHttpHeader::GetMimeType(void) const
{
  const HeaderParams::const_iterator it = m_params.find("content-type");
  if (it == m_params.end())
    return "";

  const std::string& strValue = it->second;
  return strValue.substr(0, strValue.find(';'));
}

std::string CHttpHeader::GetCharset(void)
{
  if (m_charsetIsCached && m_headerdone)
    return m_detectedCharset;

  m_charsetIsCached = m_headerdone;

  const HeaderParams::const_iterator it = m_params.find("content-type");
  if (it == m_params.end())
  {
    m_detectedCharset.clear();
    return m_detectedCharset;
  }

  std::string strValue = it->second;
  StringUtils::ToLower(strValue);
  size_t charsetParamPos = strValue.find("; charset=");
  size_t charsetNamePos;
  if (charsetParamPos != std::string::npos)
    charsetNamePos = charsetParamPos + 10;
  else
  {
    charsetParamPos = strValue.find(";charset=");
    charsetNamePos = charsetParamPos + 9;
  }

  if (charsetParamPos == std::string::npos || charsetNamePos >= strValue.length())
  {
    m_detectedCharset.clear();
    return m_detectedCharset;
  }

  m_detectedCharset.assign(strValue, charsetNamePos, strValue.find(';', charsetNamePos));
  StringUtils::ToUpper(m_detectedCharset);

  return m_detectedCharset;
}

void CHttpHeader::Clear()
{
  ClearCached();
  m_params.clear();
  m_protoLine.clear();
  m_headerdone = false;
}

void CHttpHeader::ClearCached(void)
{
  m_detectedCharset.clear();
  m_charsetIsCached = false;
}

