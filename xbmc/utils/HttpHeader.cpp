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
#include "utils/StringUtils.h"
#include "HttpHeader.h"

CHttpHeader::CHttpHeader()
{
  m_headerdone = false;
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

    if (lineEnd == pos)
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

      if (!strParam.empty() && !strValue.empty())
        m_params.push_back(HeaderParams::value_type(strParam, strValue));
    }
    else if (m_protoLine.empty())
      m_protoLine.assign(strData, pos, lineEnd - pos);

    pos = lineEnd + 2;
  }
}

void CHttpHeader::AddParam(const std::string& param, const std::string& value, const bool overwrite /*= false*/)
{
  if (param.empty() || value.empty())
    return;

  std::string paramLower(param);
  if (overwrite)
  { // delete ALL parameters with the same name
    // note: 'GetValue' always returns last added parameter,
    //       so you probably don't need to overwrite 
    for (size_t i = 0; i < m_params.size();)
    {
      if (m_params[i].first == param)
        m_params.erase(m_params.begin() + i);
      else
        ++i;
    }
  }

  StringUtils::ToLower(paramLower);

  m_params.push_back(HeaderParams::value_type(paramLower, value));
}

std::string CHttpHeader::GetValue(const std::string& strParam) const
{
  std::string paramLower(strParam);
  StringUtils::ToLower(paramLower);

  return GetValueRaw(paramLower);
}

std::string CHttpHeader::GetValueRaw(const std::string& strParam) const
{
  // look in reverse to find last parameter (probably most important)
  for (HeaderParams::const_reverse_iterator iter = m_params.rbegin(); iter != m_params.rend(); ++iter)
  {
    if (iter->first == strParam)
      return iter->second;
  }

  return "";
}

std::vector<std::string> CHttpHeader::GetValues(std::string strParam) const
{
  StringUtils::ToLower(strParam);
  std::vector<std::string> values;

  for (HeaderParams::const_iterator iter = m_params.begin(); iter != m_params.end(); ++iter)
  {
    if (iter->first == strParam)
      values.push_back(iter->second);
  }

  return values;
}

std::string CHttpHeader::GetHeader(void) const
{
  /* Only pad with additional \n if there are params
   * to put into the header or the protocol line has
   * some data */
  if (m_params.empty() && m_protoLine.empty())
    return "";

  std::string strHeader(m_protoLine + '\n');

  for (HeaderParams::const_iterator iter = m_params.begin(); iter != m_params.end(); ++iter)
    strHeader += ((*iter).first + ": " + (*iter).second + "\n");

  strHeader += "\n";
  return strHeader;
}

std::string CHttpHeader::GetMimeType(void) const
{
  std::string strValue(GetValueRaw("content-type"));

  return strValue.substr(0, strValue.find(';'));
}

std::string CHttpHeader::GetCharset(void) const
{
  std::string strValue(GetValueRaw("content-type"));
  if (strValue.empty())
    return strValue;

  const size_t posSemicolon = strValue.find (";");

  /* There is an additional parameter as indicated by the semicolon */
  if (posSemicolon == std::string::npos)
    return "";

  const size_t posParameter = strValue.find_first_not_of(" \t", posSemicolon + 1);

  /* There is a parameter */
  if (posParameter == std::string::npos)
    return "";

  /* We are assuming that there is no LWS between parameter,
   * "=" and its value */
  const size_t posEquals = strValue.find_first_of("=", posParameter);

  if (posEquals == std::string::npos)
    return "";

  const size_t posParameterValue = posEquals + 1;

  /* Is there anything on the right hand side of the equals? */
  if (posEquals + 1 >= strValue.size())
    return "";

  StringUtils::ToUpper(strValue);

  /* Quickly check if the value from the first non-space value to "="
   * is CHARSET, if so then we can parse the charset on the right */
  if (strncmp(&(strValue.c_str()[posParameter]),
              "CHARSET",
              7))
    return "";

  size_t charsetParameterEnd =
    strValue.find_first_of(" \t", posParameterValue);

  if (charsetParameterEnd == std::string::npos)
    charsetParameterEnd = strValue.size();

  return strValue.substr(posParameterValue,
                         charsetParameterEnd - posParameterValue);
}


void CHttpHeader::Clear()
{
  m_params.clear();
  m_protoLine.clear();
  m_headerdone = false;
}
