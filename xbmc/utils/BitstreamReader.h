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

const uint8_t* find_start_code(const uint8_t* p, const uint8_t* end, uint32_t* state);
