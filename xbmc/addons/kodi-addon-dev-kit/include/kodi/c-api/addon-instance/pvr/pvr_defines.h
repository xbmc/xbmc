/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

  #define PVR_ADDON_NAME_STRING_LENGTH 1024
  #define PVR_ADDON_URL_STRING_LENGTH 1024
  #define PVR_ADDON_DESC_STRING_LENGTH 1024
  #define PVR_ADDON_INPUT_FORMAT_STRING_LENGTH 32
  #define PVR_ADDON_EDL_LENGTH 32
  #define PVR_ADDON_TIMERTYPE_ARRAY_SIZE 32
  #define PVR_ADDON_TIMERTYPE_VALUES_ARRAY_SIZE 512
  #define PVR_ADDON_TIMERTYPE_VALUES_ARRAY_SIZE_SMALL 128
  #define PVR_ADDON_TIMERTYPE_STRING_LENGTH 128
  #define PVR_ADDON_ATTRIBUTE_DESC_LENGTH 128
  #define PVR_ADDON_ATTRIBUTE_VALUES_ARRAY_SIZE 512
  #define PVR_ADDON_DESCRAMBLE_INFO_STRING_LENGTH 64
  #define PVR_ADDON_DATE_STRING_LENGTH 32

  typedef struct PVR_ATTRIBUTE_INT_VALUE
  {
    int iValue;
    char strDescription[PVR_ADDON_ATTRIBUTE_DESC_LENGTH];
  } PVR_ATTRIBUTE_INT_VALUE;

  typedef struct PVR_NAMED_VALUE
  {
    char strName[PVR_ADDON_NAME_STRING_LENGTH];
    char strValue[PVR_ADDON_NAME_STRING_LENGTH];
  } PVR_NAMED_VALUE;

#ifdef __cplusplus
}
#endif /* __cplusplus */
