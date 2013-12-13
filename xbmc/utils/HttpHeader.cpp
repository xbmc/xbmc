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

// header white space characters according to RFC 2616
const char* const CHttpHeader::m_whitespaceChars = " \t";


CHttpHeader::CHttpHeader()
{
  m_headerdone = false;
}

CHttpHeader::~CHttpHeader()
{
}

void CHttpHeader::Parse(const std::string& strData)
{
  size_t pos = 0;
  const size_t len = strData.length();
  const char* const strDataC = strData.c_str();

  // According to RFC 2616 any header line can have continuation on next line, if next line is started from whitespace char
  // This code at first checks for whitespace char at the begging of the line, and if found, then current line is appended to m_lastHeaderLine
  // If current line is NOT started from whitespace char, then previously stored (and completed) m_lastHeaderLine is parsed and current line is assigned to m_lastHeaderLine (to be parsed later)
  while (pos < len)
  {
    const size_t lineEnd = strData.find("\x0d\x0a", pos); // use "\x0d\x0a" instead of "\r\n" to be platform independent

    if (lineEnd == std::string::npos)
      return; // error: expected only complete lines

    if (m_headerdone)
      Clear(); // clear previous header and process new one

    if (strDataC[pos] == ' ' || strDataC[pos] == '\t') // same chars as in CHttpHeader::m_whitespaceChars
    { // line is started from whitespace char: this is continuation of previous line
      pos = strData.find_first_not_of(m_whitespaceChars);

      m_lastHeaderLine.push_back(' '); // replace all whitespace chars at start of the line with single space
      m_lastHeaderLine.append(strData, pos, lineEnd - pos); // append current line
    }
    else
    { // this line is NOT continuation, this line is new header line
      if (!m_lastHeaderLine.empty())
        ParseLine(m_lastHeaderLine); // process previously stored completed line (if any)

      m_lastHeaderLine.assign(strData, pos, lineEnd - pos); // store current line to (possibly) complete later. Will be parsed on next turns.

      if (pos == lineEnd)
        m_headerdone = true; // current line is bare "\r\n", means end of header; no need to process current m_lastHeaderLine
    }

    pos = lineEnd + 2; // '+2' for "\r\n": go to next line (if any)
  }
}

bool CHttpHeader::ParseLine(const std::string& headerLine)
{
  const size_t valueStart = headerLine.find(':');

  if (valueStart != std::string::npos)
  {
    std::string strParam(headerLine, 0, valueStart);
    std::string strValue(headerLine, valueStart + 1);

    StringUtils::Trim(strParam, m_whitespaceChars);
    StringUtils::ToLower(strParam);

    StringUtils::Trim(strValue, m_whitespaceChars);

    if (!strParam.empty() && !strValue.empty())
      m_params.push_back(HeaderParams::value_type(strParam, strValue));
    else
      return false;
  }
  else if (m_protoLine.empty())
    m_protoLine = headerLine;

  return true;
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
  std::string strHeader(m_protoLine + '\n');

  for (HeaderParams::const_iterator iter = m_params.begin(); iter != m_params.end(); ++iter)
    strHeader += ((*iter).first + ": " + (*iter).second + "\n");

  strHeader += "\n";
  return strHeader;
}

std::string CHttpHeader::GetMimeType(void) const
{
  std::string strValue(GetValueRaw("content-type"));

  std::string mimeType(strValue, 0, strValue.find(';'));
  StringUtils::TrimRight(mimeType, m_whitespaceChars);

  return mimeType;
}

std::string CHttpHeader::GetCharset(void) const
{
  std::string strValue(GetValueRaw("content-type"));
  if (strValue.empty())
    return strValue;

  StringUtils::ToUpper(strValue);
  const size_t len = strValue.length();

  // extract charset value from 'contenttype/contentsubtype;pram1=param1Val ; charset=XXXX\t;param2=param2Val'
  // most common form: 'text/html; charset=XXXX'
  // charset value can be in double quotes: 'text/xml; charset="XXX XX"'

  size_t pos = strValue.find(';');
  while (pos < len)
  {
    // move to the next non-whitespace character
    pos = strValue.find_first_not_of(m_whitespaceChars, pos + 1);

    if (pos != std::string::npos)
    {
      if (strValue.compare(pos, 8, "CHARSET=", 8) == 0)
      {
        pos += 8; // move position to char after 'CHARSET='
        std::string charset(strValue, pos, strValue.find(';', pos));  // intentionally ignoring possible ';' inside quoted string
                                                                      // as we don't support any charset with ';' in name
        StringUtils::Trim(charset, m_whitespaceChars);
        if (!charset.empty())
        {
          if (charset[0] != '"')
            return charset;
          else
          { // charset contains quoted string (allowed according to RFC 2616)
            StringUtils::Replace(charset, "\\", ""); // unescape chars, ignoring possible '\"' and '\\'
            const size_t closingQ = charset.find('"', 1);
            if (closingQ == std::string::npos)
              return ""; // no closing quote

            return charset.substr(1, closingQ - 1);
          }
        }
      }
      pos = strValue.find(';', pos); // find next parameter
    }
  }

  return ""; // no charset is detected
}

void CHttpHeader::Clear()
{
  m_params.clear();
  m_protoLine.clear();
  m_headerdone = false;
  m_lastHeaderLine.clear();
}
