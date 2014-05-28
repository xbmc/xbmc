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

#ifndef COMMONS_COMMONS_ILOG_H_INCLUDED
#define COMMONS_COMMONS_ILOG_H_INCLUDED
#include "commons/ilog.h"
#endif

#ifndef COMMONS_UTILS_STDSTRING_H_INCLUDED
#define COMMONS_UTILS_STDSTRING_H_INCLUDED
#include "utils/StdString.h"
#endif

#ifndef COMMONS_UTILS_STRINGUTILS_H_INCLUDED
#define COMMONS_UTILS_STRINGUTILS_H_INCLUDED
#include "utils/StringUtils.h"
#endif


namespace XbmcCommons
{
  void ILogger::Log(int loglevel, const char *format, ... )
  {
    CStdString strData;

    strData.reserve(16384);
    va_list va;
    va_start(va, format);
    strData = StringUtils::FormatV(format,va);
    va_end(va);

    log(loglevel, strData);
  }
}
