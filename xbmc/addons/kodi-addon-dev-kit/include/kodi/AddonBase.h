#pragma once
/*
 *      Copyright (C) 2005-2016 Team Kodi
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

#include <stdarg.h>     /* va_list, va_start, va_arg, va_end */
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

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

#include "versions.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Standard undefined pointer handle */
typedef void* KODI_HANDLE;

///
typedef enum ADDON_STATUS
{
  ///
  ADDON_STATUS_OK,

  ///
  ADDON_STATUS_LOST_CONNECTION,

  ///
  ADDON_STATUS_NEED_RESTART,

  ///
  ADDON_STATUS_NEED_SETTINGS,

  ///
  ADDON_STATUS_UNKNOWN,

  ///
  ADDON_STATUS_NEED_SAVEDSETTINGS,

  /// permanent failure, like failing to resolve methods
  ADDON_STATUS_PERMANENT_FAILURE,

  /* internal used return error if function becomes not used from child on
   * addon */
  ADDON_STATUS_NOT_IMPLEMENTED
} ADDON_STATUS;

/*
 *
 */
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
struct ADDON_HANDLE_STRUCT
{
  void *callerAddress;  /*!< address of the caller */
  void *dataAddress;    /*!< address to store data in */
  int   dataIdentifier; /*!< parameter to pass back when calling the callback */
};
typedef ADDON_HANDLE_STRUCT *ADDON_HANDLE;

/* To have a on add-on and kodi itself handled string always on known size! */
#define ADDON_STANDARD_STRING_LENGTH 1024
#define ADDON_STANDARD_STRING_LENGTH_SMALL 256

#ifdef __cplusplus
}
#endif /* __cplusplus */
