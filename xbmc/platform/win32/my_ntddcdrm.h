/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <winioctl.h>

#define IOCTL_CDROM_BASE             FILE_DEVICE_CD_ROM
#define IOCTL_CDROM_RAW_READ         CTL_CODE(IOCTL_CDROM_BASE, 0x000F, METHOD_OUT_DIRECT,  FILE_READ_ACCESS)

typedef enum _TRACK_MODE_TYPE {
  YellowMode2,
  XAForm2,
  CDDA
} TRACK_MODE_TYPE, *PTRACK_MODE_TYPE;

typedef struct __RAW_READ_INFO
{
  LARGE_INTEGER DiskOffset;
  ULONG SectorCount;
  TRACK_MODE_TYPE TrackMode;
}
RAW_READ_INFO, *PRAW_READ_INFO;

