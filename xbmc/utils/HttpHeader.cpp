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

#include <boost/bind.hpp>
#include <boost/algorithm/string.hpp>
#include "HttpHeader.h"
#include "utils/StringUtils.h"

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

namespace
{
/* We don't have boost::regex as it is a boost library
 * so this function will have to do instead */
bool AcceptableCharacter(char c)
{
  const char lowercaseStart = 'a';
  const char lowercaseEnd = 'z';
  const char uppercaseStart = 'A';
  const char uppercaseEnd = 'Z';
  const char numericStart = '0';
  const char numericEnd = '9';
  const char hyphen = '-';

  if (c == hyphen)
    return true;

  if (c >= lowercaseStart &&
      c <= lowercaseEnd)
    return true;

  if (c >= uppercaseStart &&
      c <= uppercaseEnd)
    return true;

  if (c >= numericStart &&
      c <= numericEnd)
    return true;

  return false;
}

boost::iterator_range<std::string::iterator>
CharsetFinder(std::string::iterator begin,
              std::string::iterator end)
{
  while(!AcceptableCharacter(*begin))
    ++begin;

  /* End is the element after the end, so we need
   * to restore it to the last element */
  --end;

  while (!AcceptableCharacter(*end))
    --end;

  return boost::make_iterator_range(begin, ++end);
}
}

std::string CHttpHeader::GetCharset(void) const
{
  std::string strValue(GetValueRaw("content-type"));
  if (strValue.empty())
    return strValue;

  const size_t semicolonPos = strValue.find(';');
  if (semicolonPos == std::string::npos)
    return "";

  StringUtils::ToUpper(strValue);
  size_t posCharset;
  if ((posCharset = strValue.find("CHARSET=", semicolonPos)) != std::string::npos)
    posCharset += 8;
  else
    return "";

  std::string::iterator begin(strValue.begin() + posCharset);
  std::string::iterator end(strValue.end());

  boost::iterator_range<std::string::iterator> charsetDataToEnd(boost::make_iterator_range(begin ,end));

  boost::iterator_range<std::string::iterator> charsetDataRange =
    boost::algorithm::find(charsetDataToEnd,
                           CharsetFinder);

  return std::string(charsetDataRange.begin(),
                     charsetDataRange.end());
}


void CHttpHeader::Clear()
{
  m_params.clear();
  m_protoLine.clear();
  m_headerdone = false;
}
