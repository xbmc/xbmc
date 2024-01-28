/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "BitstreamReader.h"

CBitstreamReader::CBitstreamReader(const uint8_t* buf, int len)
  : buffer(buf), start(buf), length(len)
{
}

uint32_t CBitstreamReader::ReadBits(int nbits)
{
  uint32_t ret = GetBits(nbits);

  offbits += nbits;
  buffer += offbits / 8;
  offbits %= 8;

  m_posBits += nbits;

  return ret;
}

void CBitstreamReader::SkipBits(int nbits)
{
  offbits += nbits;
  buffer += offbits / 8;
  offbits %= 8;

  m_posBits += nbits;

  if (buffer > (start + length))
    oflow = 1;
}

uint32_t CBitstreamReader::GetBits(int nbits)
{
  int i, nbytes;
  uint32_t ret = 0;
  const uint8_t *buf;

  buf = buffer;
  nbytes = (offbits + nbits) / 8;

  if (((offbits + nbits) % 8) > 0)
    nbytes++;

  if ((buf + nbytes) > (start + length))
  {
    oflow = 1;
    return 0;
  }
  for (i = 0; i<nbytes; i++)
    ret += buf[i] << ((nbytes - i - 1) * 8);

  i = (4 - nbytes) * 8 + offbits;

  ret = ((ret << i) >> i) >> ((nbytes * 8) - nbits - offbits);

  return ret;
}

const uint8_t* find_start_code(const uint8_t *p, const uint8_t *end, uint32_t *state)
{
  if (p >= end)
    return end;

  for (int i = 0; i < 3; i++)
  {
    uint32_t tmp = *state << 8;
    *state = tmp + *(p++);
    if (tmp == 0x100 || p == end)
      return p;
  }

  while (p < end)
  {
    if (p[-1] > 1) p += 3;
    else if (p[-2]) p += 2;
    else if (p[-3] | (p[-1] - 1)) p++;
    else {
      p++;
      break;
    }
  }

  p = (p < end)? p - 4 : end - 4;
  *state = BS_RB32(p);

  return p + 4;
}
