/*
 *      vdr-plugin-vnsi - XBMC server plugin for VDR
 *
 *      Copyright (C) 2010 Alwin Esch (Team XBMC)
 *
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include <stdio.h>
#include <inttypes.h>
#include "bitstream.h"

cBitstream::cBitstream(uint8_t *data, int bits)
{
  m_data   = data;
  m_offset = 0;
  m_len    = bits;
}

void cBitstream::setBitstream(uint8_t *data, int bits)
{
  m_data   = data;
  m_offset = 0;
  m_len    = bits;
}

void cBitstream::skipBits(int num)
{
  m_offset += num;
}

unsigned int cBitstream::readBits(int num)
{
  int r = 0;

  while(num > 0)
  {
    if(m_offset >= m_len)
      return 0;

    num--;

    if(m_data[m_offset / 8] & (1 << (7 - (m_offset & 7))))
      r |= 1 << num;

    m_offset++;
  }
  return r;
}

unsigned int cBitstream::showBits(int num)
{
  int r = 0;
  int offs = m_offset;

  while(num > 0)
  {
    if(offs >= m_len)
      return 0;

    num--;

    if(m_data[offs / 8] & (1 << (7 - (offs & 7))))
      r |= 1 << num;

    offs++;
  }
  return r;
}

unsigned int cBitstream::readGolombUE()
{
  int lzb = -1;

  for(int b = 0; !b; lzb++)
    b = readBits1();

  return (1 << lzb) - 1 + readBits(lzb);
}

signed int cBitstream::readGolombSE()
{
  int v, neg;
  v = readGolombUE();
  if(v == 0)
    return 0;

  neg = v & 1;
  v = (v + 1) >> 1;
  return neg ? -v : v;
}


unsigned int cBitstream::remainingBits()
{
  return m_len - m_offset;
}


void cBitstream::putBits(int val, int num)
{
  while(num > 0) {
    if(m_offset >= m_len)
      return;

    num--;

    if(val & (1 << num))
      m_data[m_offset / 8] |= 1 << (7 - (m_offset & 7));
    else
      m_data[m_offset / 8] &= ~(1 << (7 - (m_offset & 7)));

    m_offset++;
  }
}
