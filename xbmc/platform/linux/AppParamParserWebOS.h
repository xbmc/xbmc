/*
 *  Copyright (C) 2005-2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "AppParamParserLinux.h"

class CAppParamParserWebOS : public CAppParamParserLinux
{
public:
  CAppParamParserWebOS() = default;
  ~CAppParamParserWebOS() = default;

protected:
  void ParseArg(const std::string& arg) override;

private:
  int m_nArgs{0};
};
