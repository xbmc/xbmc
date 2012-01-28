/*
 *      Copyright (C) 2010 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */
#ifndef _DARWIN_UTILS_H_
#define _DARWIN_UTILS_H_

#include <string>

#ifdef __cplusplus
extern "C"
{
#endif
  bool        DarwinIsAppleTV2(void);
  const char *GetDarwinVersionString(void);
  float       GetIOSVersion(void);
  int         GetDarwinFrameworkPath(bool forPython, char* path, uint32_t *pathsize);
  int         GetDarwinExecutablePath(char* path, uint32_t *pathsize);
  bool        DarwinHasVideoToolboxDecoder(void);
  int         DarwinBatteryLevel(void);
  void        DarwinSetScheduling(int message);
#ifdef __cplusplus
}
#endif

#endif
