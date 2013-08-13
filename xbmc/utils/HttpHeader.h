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

#include <map>
#include <vector>
#include <string>

#define HTTPHEADER_CONTENT_TYPE "Content-Type"

typedef std::map<std::string, std::string> HeaderParams;
typedef HeaderParams::iterator HeaderParamsIter;

class CHttpHeader
{
public:
  CHttpHeader();
  ~CHttpHeader();

  void Parse(const std::string& strData);
  void AddParamWithValue(const std::string& param, const std::string& value);

  std::string GetValue(std::string strParam) const;

  std::string& GetHeader(std::string& strHeader) const;
  std::string GetHeader(void) const;

  std::string GetMimeType() { return GetValue(HTTPHEADER_CONTENT_TYPE); }
  std::string GetProtoLine() { return m_protoLine; }

  void Clear();

protected:
  HeaderParams m_params;
  std::string   m_protoLine;
};

