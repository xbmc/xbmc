/*
 *      Copyright (C) 2012 Team XBMC
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
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
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

