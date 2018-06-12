/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://kodi.tv
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

#pragma once

#include <utility>
#include <vector>
#include <string>

class CHttpHeader
{
public:
  typedef std::pair<std::string, std::string> HeaderParamValue;
  typedef std::vector<HeaderParamValue> HeaderParams;
  typedef HeaderParams::iterator HeaderParamsIter;

  CHttpHeader();
  ~CHttpHeader();

  void Parse(const std::string& strData);
  void AddParam(const std::string& param, const std::string& value, const bool overwrite = false);

  std::string GetValue(const std::string& strParam) const;
  std::vector<std::string> GetValues(std::string strParam) const;

  std::string GetHeader(void) const;

  std::string GetMimeType(void) const;
  std::string GetCharset(void) const;
  inline std::string GetProtoLine() const
  { return m_protoLine; }

  inline bool IsHeaderDone(void) const
  { return m_headerdone; }

  void Clear();

protected:
  std::string GetValueRaw(const std::string& strParam) const;
  bool ParseLine(const std::string& headerLine);

  HeaderParams m_params;
  std::string   m_protoLine;
  bool m_headerdone;
  std::string m_lastHeaderLine;
  static const char* const m_whitespaceChars;
};

