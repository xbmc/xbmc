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

#ifndef WIN32_STAT_UTF8_H_INCLUDED
#define WIN32_STAT_UTF8_H_INCLUDED
#include "stat_utf8.h"
#endif

#ifndef WIN32_UTILS_CHARSETCONVERTER_H_INCLUDED
#define WIN32_UTILS_CHARSETCONVERTER_H_INCLUDED
#include "utils/CharsetConverter.h"
#endif


int stat64_utf8(const char* __file, struct stat64* __buf)
{
  std::wstring fileW;
  g_charsetConverter.utf8ToW(__file, fileW, false);
  return _wstat64(fileW.c_str(), __buf);
}

