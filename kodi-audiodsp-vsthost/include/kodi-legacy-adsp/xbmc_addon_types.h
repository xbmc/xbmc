#pragma once

/*
 *      Copyright (C) 2005-2015 Team Kodi
 *      http://kodi.tv
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
 *  along with Kodi; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#ifdef TARGET_WINDOWS
#include <windows.h>
#else
#ifndef __cdecl
#define __cdecl
#endif
#ifndef __declspec
#define __declspec(X)
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Guard against redefinition when this header is included from a Kodi
 * translation unit that already pulled in kodi-addon-dev-kit/include/kodi/AddonBase.h.
 * That header defines ADDON_STATUS and ADDON_HANDLE_STRUCT and sets the
 * sentinel macro KODI_ADDON_DEV_KIT_TYPES_DEFINED.  When the sentinel is
 * absent (e.g. building audiodsp.vsthost itself) we define the types normally. */
#ifndef KODI_ADDON_DEV_KIT_TYPES_DEFINED

enum ADDON_STATUS
{
  ADDON_STATUS_OK,
  ADDON_STATUS_LOST_CONNECTION,
  ADDON_STATUS_NEED_RESTART,
  ADDON_STATUS_NEED_SETTINGS,
  ADDON_STATUS_UNKNOWN,
  ADDON_STATUS_NEED_SAVEDSETTINGS,
  ADDON_STATUS_PERMANENT_FAILURE   /**< permanent failure, like failing to resolve methods */
};

#endif /* !KODI_ADDON_DEV_KIT_TYPES_DEFINED */

typedef struct
{
  int           type;
  char*         id;
  char*         label;
  int           current;
  char**        entry;
  unsigned int  entry_elements;
} ADDON_StructSetting;

/*!
 * @brief Handle used to return data from the PVR add-on to CPVRClient
 */
#ifndef KODI_ADDON_DEV_KIT_TYPES_DEFINED

struct ADDON_HANDLE_STRUCT
{
  void *callerAddress;  /*!< address of the caller */
  void *dataAddress;    /*!< address to store data in */
  int   dataIdentifier; /*!< parameter to pass back when calling the callback */
};
typedef ADDON_HANDLE_STRUCT *ADDON_HANDLE;

#endif /* !KODI_ADDON_DEV_KIT_TYPES_DEFINED */

#ifdef __cplusplus
};
#endif

