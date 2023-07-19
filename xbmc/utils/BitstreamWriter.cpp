/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "BitstreamWriter.h"

CBitstreamWriter::CBitstreamWriter(uint8_t* buffer, unsigned int buffer_size, int writer_le)
  : writer_le(writer_le), buf(buffer), buf_ptr(buf)
{
}

void CBitstreamWriter::WriteBits(int n, unsigned int value)
{
  // Write up to 32 bits into a bitstream.
  unsigned int bit_buf;
  int bit_left;

  if (n == 32)
  {
    // Write exactly 32 bits into a bitstream.
    // danger, recursion in play.
    int lo = value & 0xffff;
    int hi = value >> 16;
    if (writer_le)
    {
      WriteBits(16, lo);
      WriteBits(16, hi);
    }
    else
    {
      WriteBits(16, hi);
      WriteBits(16, lo);
    }
    return;
  }

  bit_buf = this->bit_buf;
  bit_left = this->bit_left;

  if (writer_le)
  {
    bit_buf |= value << (32 - bit_left);
    if (n >= bit_left) {
      BS_WL32(buf_ptr, bit_buf);
      buf_ptr += 4;
      bit_buf = (bit_left == 32) ? 0 : value >> bit_left;
      bit_left += 32;
    }
    bit_left -= n;
  }
  else
  {
    if (n < bit_left) {
      bit_buf = (bit_buf << n) | value;
      bit_left -= n;
    }
    else {
      bit_buf <<= bit_left;
      bit_buf |= value >> (n - bit_left);
      BS_WB32(buf_ptr, bit_buf);
      buf_ptr += 4;
      bit_left += 32 - n;
      bit_buf = value;
    }
  }

  this->bit_buf = bit_buf;
  this->bit_left = bit_left;
}

void CBitstreamWriter::SkipBits(int n)
{
  // Skip the given number of bits.
  // Must only be used if the actual values in the bitstream do not matter.
  // If n is 0 the behavior is undefined.
  bit_left -= n;
  buf_ptr -= 4 * (bit_left >> 5);
  bit_left &= 31;
}

void CBitstreamWriter::FlushBits()
{
  if (!writer_le)
  {
    if (bit_left < 32)
      bit_buf <<= bit_left;
  }
  while (bit_left < 32)
  {

    if (writer_le)
    {
      *buf_ptr++ = bit_buf;
      bit_buf >>= 8;
    }
    else
    {
      *buf_ptr++ = bit_buf >> 24;
      bit_buf <<= 8;
    }
    bit_left += 8;
  }
  bit_left = 32;
  bit_buf = 0;
}
