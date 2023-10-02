/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>
#include <vector>

class CFileItem;

class CExecString
{
public:
  CExecString() = default;
  explicit CExecString(const std::string& execString);
  CExecString(const std::string& function, const std::vector<std::string>& params);
  CExecString(const std::string& function, const CFileItem& target, const std::string& param);
  CExecString(const CFileItem& item, const std::string& contextWindow);

  virtual ~CExecString() = default;

  std::string GetExecString() const { return m_execString; }

  bool IsValid() const { return m_valid; }

  std::string GetFunction() const { return m_function; }
  std::vector<std::string> GetParams() const { return m_params; }

private:
  bool Parse(const std::string& execString);
  bool Parse(const CFileItem& item, const std::string& contextWindow);

  void Build(const std::string& function, const std::vector<std::string>& params);
  void BuildPlayMedia(const CFileItem& item, const std::string& target);

  void SetExecString();

  bool m_valid{false};
  std::string m_function;
  std::vector<std::string> m_params;
  std::string m_execString;
};
