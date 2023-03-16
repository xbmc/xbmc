/*
 *  Copyright (C) 2005-2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AppParamParserWebOS.h"

void CAppParamParserWebOS::ParseArg(const std::string& arg)
{
  // On webOS the app launcher adds a JSON as the first argument by default. These arguments do not contain
  // useful information. So to avoid kodi trying to parse these args and misinterpret them as a video file.
  if (m_nArgs++ == 0 && arg.size() > 1 && arg.front() == '{' && arg.back() == '}')
    return;

  CAppParamParserLinux::ParseArg(arg);
}
