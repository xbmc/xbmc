/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
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
  unsigned int Position() { return m_posBits; }
  unsigned int AvailableBits() { return length * 8 - m_posBits; }

private:
  const uint8_t *buffer, *start;
  int offbits = 0, length, oflow = 0;
  int m_posBits{0};
};

const uint8_t* find_start_code(const uint8_t *p, const uint8_t *end, uint32_t *state);

////////////////////////////////////////////////////////////////////////////////////////////
//! @todo refactor this so as not to need these ffmpeg routines.
//! These are not exposed in ffmpeg's API so we dupe them here.

/*
 *  AVC helper functions for muxers
 *  Copyright (c) 2006 Baptiste Coudurier <baptiste.coudurier@smartjog.com>
 *  This is part of FFmpeg
 *  
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *  See LICENSES/README.md for more information.
 */
constexpr uint32_t BS_RB24(const uint8_t* x)
{
  return (x[0] << 16) | (x[1] << 8) | x[2];
}

constexpr uint32_t BS_RB32(const uint8_t* x)
{
  return (x[1] << 24) | (x[1] << 16) | (x[2] << 8) | x[3];
}

