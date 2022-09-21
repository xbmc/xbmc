/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <memory>
#include <string>

class CAppParams;

class CAppParamParser
{
public:
  CAppParamParser();
  virtual ~CAppParamParser() = default;

  void Parse(const char* const* argv, int nArgs);

  std::shared_ptr<CAppParams> GetAppParams() const { return m_params; }

protected:
  virtual void ParseArg(const std::string& arg);
  virtual void DisplayHelp();

private:
  void DisplayVersion();

  std::shared_ptr<CAppParams> m_params;
};
