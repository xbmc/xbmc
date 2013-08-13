#pragma once

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

#include <utility>
#include <vector>
#include <string>

#define HTTPHEADER_CONTENT_TYPE "Content-Type"

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

  std::string GetValue(std::string strParam) const;
  std::vector<std::string> GetValues(std::string strParam) const;

  std::string GetHeader(void) const;

  std::string GetMimeType() { return GetValue(HTTPHEADER_CONTENT_TYPE); }
  std::string GetProtoLine() { return m_protoLine; }

  inline bool IsHeaderDone(void) const
  { return m_headerdone; }

  void Clear();

protected:
  HeaderParams m_params;
  std::string   m_protoLine;
  bool m_headerdone;
};

