/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

enum
{
  DISPLAYCONFIG_DEVICE_INFO_SET_RESERVED1 = 14,
  DISPLAYCONFIG_DEVICE_INFO_GET_ADVANCED_COLOR_INFO_2 = 15,
  DISPLAYCONFIG_DEVICE_INFO_SET_HDR_STATE = 16,
  DISPLAYCONFIG_DEVICE_INFO_SET_WCG_STATE = 17,
};

typedef enum _DISPLAYCONFIG_ADVANCED_COLOR_MODE
{
  DISPLAYCONFIG_ADVANCED_COLOR_MODE_SDR,
  DISPLAYCONFIG_ADVANCED_COLOR_MODE_WCG,
  DISPLAYCONFIG_ADVANCED_COLOR_MODE_HDR
} DISPLAYCONFIG_ADVANCED_COLOR_MODE;

typedef struct _DISPLAYCONFIG_GET_ADVANCED_COLOR_INFO_2
{
  DISPLAYCONFIG_DEVICE_INFO_HEADER header;
  union
  {
    struct
    {
      UINT32 advancedColorSupported : 1;
      UINT32 advancedColorActive : 1;
      UINT32 reserved1 : 1;
      UINT32 advancedColorLimitedByPolicy : 1;
      UINT32 highDynamicRangeSupported : 1;
      UINT32 highDynamicRangeUserEnabled : 1;
      UINT32 wideColorSupported : 1;
      UINT32 wideColorUserEnabled : 1;
      UINT32 reserved : 24;
    };
    UINT32 value;
  };
  DISPLAYCONFIG_COLOR_ENCODING colorEncoding;
  UINT32 bitsPerColorChannel;
  DISPLAYCONFIG_ADVANCED_COLOR_MODE activeColorMode;
} DISPLAYCONFIG_GET_ADVANCED_COLOR_INFO_2;

typedef struct _DISPLAYCONFIG_SET_HDR_STATE
{
  DISPLAYCONFIG_DEVICE_INFO_HEADER header;
  union
  {
    struct
    {
      UINT32 enableHdr : 1;
      UINT32 reserved : 31;
    };
    UINT32 value;
  };
} DISPLAYCONFIG_SET_HDR_STATE;
