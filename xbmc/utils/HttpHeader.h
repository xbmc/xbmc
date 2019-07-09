/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>
#include <utility>
#include <vector>

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

