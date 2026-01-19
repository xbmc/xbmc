/*
 *  Copyright (C) 2005-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <cstdint>

class CBitstream
{
public:
  CBitstream(const uint8_t* data, int bits)
  {
    m_data = data;
    m_offset = 0;
    m_len = bits;
    m_error = false;
  }
  unsigned int readBits(int num)
  {
    int r = 0;
    while (num > 0)
    {
      if (m_offset >= m_len)
      {
        m_error = true;
        return 0;
      }
      num--;
      if (m_data[m_offset / 8] & (1 << (7 - (m_offset & 7))))
        r |= 1 << num;
      m_offset++;
    }
    return r;
  }
  unsigned int readGolombUE(int maxbits = 32)
  {
    int lzb = -1;
    int bits = 0;
    for (int b = 0; !b; lzb++, bits++)
    {
      if (bits > maxbits)
      {
        m_error = true;
        return 0;
      }
      b = readBits(1);
    }
    // Prevent undefined behavior from shifting >= 32 bits
    // Valid Golomb codes should have lzb < 32
    if (lzb > 31)
    {
      m_error = true;
      return 0;
    }
    return (1U << lzb) - 1 + readBits(lzb);
  }

  bool hasError() const { return m_error; }

private:
  const uint8_t* m_data;
  int m_offset;
  int m_len;
  bool m_error;
};
