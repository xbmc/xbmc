#ifndef __XBMC_ADDON_TYPES_H__
#define __XBMC_ADDON_TYPES_H__

/*
 *      Copyright (C) 2005-2010 Team XBMC
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

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32                   // windows
#define ADDON_HELPER_PLATFORM   "win32"
#define ADDON_HELPER_EXT        ".dll"
#else                           // linux+osx
#define ADDON_HELPER_EXT        ".so"
#if defined(__APPLE__)          // osx
#define ADDON_HELPER_PLATFORM   "osx"
#if defined(__POWERPC__)
#define ADDON_HELPER_ARCH       "powerpc"
#elif defined(__arm__)
#define ADDON_HELPER_ARCH       "arm"
#else
#define ADDON_HELPER_ARCH       "x86"
#endif
#else                           // linux
#define ADDON_HELPER_PLATFORM   "linux"
#if defined(__x86_64__)
#define ADDON_HELPER_ARCH       "x86_64"
#elif defined(_POWERPC)
#define ADDON_HELPER_ARCH       "powerpc"
#elif defined(_POWERPC64)
#define ADDON_HELPER_ARCH       "powerpc64"
#elif defined(_ARMEL)
#define ADDON_HELPER_ARCH       "arm"
#elif defined(_MIPSEL)
#define ADDON_HELPER_ARCH       "mipsel"
#else
#define ADDON_HELPER_ARCH       "i486"
#endif
#endif
#endif

enum ADDON_STATUS
{
  ADDON_STATUS_OK,
  ADDON_STATUS_LOST_CONNECTION,
  ADDON_STATUS_NEED_RESTART,
  ADDON_STATUS_NEED_SETTINGS,
  ADDON_STATUS_UNKNOWN,
  ADDON_STATUS_NEED_SAVEDSETTINGS
};

typedef struct
{
  int           type;
  char*         id;
  char*         label;
  int           current;
  char**        entry;
  unsigned int  entry_elements;
} ADDON_StructSetting;

#ifdef __cplusplus
};
#endif

#endif
