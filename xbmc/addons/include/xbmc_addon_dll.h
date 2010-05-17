#ifndef __XBMC_ADDON_H__
#define __XBMC_ADDON_H__

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

#ifdef _WIN32
#include <windows.h>
#else
#ifndef __cdecl
#define __cdecl
#endif
#ifndef __declspec
#define __declspec(X)
#endif
#endif

extern "C"
{ 
  enum ADDON_STATUS
  {
    STATUS_OK,
    STATUS_LOST_CONNECTION,
    STATUS_NEED_RESTART,
    STATUS_NEED_SETTINGS,
    STATUS_UNKNOWN
  };

  typedef struct
  {
    int           type;
    char*         id;
    char*         label;
    int           current;
    char**        entry;
    unsigned int  entry_elements;
  } StructSetting;

  ADDON_STATUS __declspec(dllexport) Create(void *callbacks, void* props);
  void __declspec(dllexport) Stop();
  void __declspec(dllexport) Destroy();
  ADDON_STATUS __declspec(dllexport) GetStatus();
  bool __declspec(dllexport) HasSettings();
  unsigned int __declspec(dllexport) GetSettings(StructSetting ***sSet);
  ADDON_STATUS __declspec(dllexport) SetSetting(const char *settingName, const void *settingValue);
  void __declspec(dllexport) FreeSettings();
};

#endif
