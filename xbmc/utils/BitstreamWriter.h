#pragma once
/*
*      Copyright (C) 2017 Team Kodi
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

#include <stdint.h>

class CBitstreamWriter 
{
public:
  CBitstreamWriter(uint8_t *buffer, unsigned int buffer_size, int writer_le);
  void WriteBits(int n, unsigned int value);
  void SkipBits(int n);
  void FlushBits();

private:
  int       writer_le;
  uint32_t  bit_buf;
  int       bit_left;
  uint8_t   *buf, *buf_ptr, *buf_end;
  int       size_in_bits;
};

////////////////////////////////////////////////////////////////////////////////////////////
//! @todo refactor this so as not to need these ffmpeg routines.
//! These are not exposed in ffmpeg's API so we dupe them here.
// AVC helper functions for muxers,
//  * Copyright (c) 2006 Baptiste Coudurier <baptiste.coudurier@smartjog.com>
// This is part of FFmpeg
//  * License as published by the Free Software Foundation; either
//  * version 2.1 of the License, or (at your option) any later version.

#define BS_WB32(p, d) { \
  ((uint8_t*)(p))[3] = (d); \
  ((uint8_t*)(p))[2] = (d) >> 8; \
  ((uint8_t*)(p))[1] = (d) >> 16; \
  ((uint8_t*)(p))[0] = (d) >> 24; }

#define BS_WL32(p, d) { \
  ((uint8_t*)(p))[0] = (d); \
  ((uint8_t*)(p))[1] = (d) >> 8; \
  ((uint8_t*)(p))[2] = (d) >> 16; \
  ((uint8_t*)(p))[3] = (d) >> 24; }
