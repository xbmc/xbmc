/*
*      Copyright (C) 2017 Team XBMC
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
*  along with XBMC; see the file COPYING.  If not, see
*  <http://www.gnu.org/licenses/>.
*
*/

#include "BitstreamReader.h"

CBitstreamReader::CBitstreamReader(const uint8_t *buf, int len)
  : buffer(buf)
  , start(buf)
  , offbits(0)
  , length(len)
  , oflow(0)
{
}

uint32_t CBitstreamReader::ReadBits(int nbits)
{
  uint32_t ret = GetBits(nbits);

  offbits += nbits;
  buffer += offbits / 8;
  offbits %= 8;

  return ret;
}

void CBitstreamReader::SkipBits(int nbits)
{
  offbits += nbits;
  buffer += offbits / 8;
  offbits %= 8;

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
