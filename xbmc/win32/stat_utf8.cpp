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

#include "stat_utf8.h"
#include "utils/CharsetConverter.h"

int stat64_utf8(const char* __file, struct stat64* __buf)
{
  std::wstring fileW;
  if (!CCharsetConverter::Utf8ToWSystemSafe(__file, fileW))
  {
    errno = EINVAL;
    return -1;
  }
  return _wstat64(fileW.c_str(), __buf);
}

