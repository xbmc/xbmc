/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "commons/ilog.h"
#include "utils/StringUtils.h"

namespace XbmcCommons
{
  void ILogger::Log(int loglevel, const char *format, ... )
  {
    std::string strData;

    strData.reserve(16384);
    va_list va;
    va_start(va, format);
    strData = StringUtils::FormatV(format,va);
    va_end(va);

    log(loglevel, strData.c_str());
  }
}
