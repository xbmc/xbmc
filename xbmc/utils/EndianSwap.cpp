/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "EndianSwap.h"

/* based on libavformat/spdif.c */
void Endian_Swap16_buf(uint16_t *dst, uint16_t *src, int w)
{
  int i;

  for (i = 0; i + 8 <= w; i += 8) {
    dst[i + 0] = Endian_Swap16(src[i + 0]);
    dst[i + 1] = Endian_Swap16(src[i + 1]);
    dst[i + 2] = Endian_Swap16(src[i + 2]);
    dst[i + 3] = Endian_Swap16(src[i + 3]);
    dst[i + 4] = Endian_Swap16(src[i + 4]);
    dst[i + 5] = Endian_Swap16(src[i + 5]);
    dst[i + 6] = Endian_Swap16(src[i + 6]);
    dst[i + 7] = Endian_Swap16(src[i + 7]);
  }

  for (; i < w; i++)
    dst[i + 0] = Endian_Swap16(src[i + 0]);
}

