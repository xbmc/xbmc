/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

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
  uint32_t bit_buf = 0;
  int bit_left = 32;
  uint8_t   *buf, *buf_ptr;
};

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
