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

#pragma once

#include <stdint.h>

class CBitstreamReader
{
public:
  CBitstreamReader(const uint8_t *buf, int len);
  uint32_t   ReadBits(int nbits);
  void       SkipBits(int nbits);
  uint32_t   GetBits(int nbits);

private:
  const uint8_t *buffer, *start;
  int      offbits, length, oflow;
};

const uint8_t* find_start_code(const uint8_t *p, const uint8_t *end, uint32_t *state);

////////////////////////////////////////////////////////////////////////////////////////////
//! @todo refactor this so as not to need these ffmpeg routines.
//! These are not exposed in ffmpeg's API so we dupe them here.
// AVC helper functions for muxers,
//  * Copyright (c) 2006 Baptiste Coudurier <baptiste.coudurier@smartjog.com>
// This is part of FFmpeg
//  * License as published by the Free Software Foundation; either
//  * version 2.1 of the License, or (at your option) any later version.
constexpr uint32_t BS_RB24(const uint8_t* x)
{
  return (x[0] << 16) | (x[1] << 8) | x[2];
}

constexpr uint32_t BS_RB32(const uint8_t* x)
{
  return (x[1] << 24) | (x[1] << 16) | (x[2] << 8) | x[3];
}
