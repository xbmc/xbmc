#ifndef MY_NTDDCDRM_H
#define MY_NTDDCDRM_H

/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
 *
 */

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

#endif