/*
*      Copyright (C) 2010 Team XBMC
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

#include "ByteParser.h"

CByteParser::CByteParser(const BYTE *pData, uint32_t length)
    : m_pData(pData), m_pCurrent(pData), m_pEnd(pData + length), m_dwLen(length), m_bitBuffer(0), m_bitLen(0)
{
}

CByteParser::~CByteParser()
{
}

uint32_t CByteParser::BitRead(uint8_t numBits, bool peek)
{
  ASSERT(numBits <= 32);
  ASSERT(numBits <= (m_bitLen + (8 * (m_pEnd - m_pCurrent))));

  if (numBits == 0)
  {
    return 0;
  }

  bool atEnd = false;
  // Read more data in the buffer
  while (m_bitLen < numBits)
  {
    m_bitBuffer <<= 8;
    m_bitBuffer += !atEnd ? *m_pCurrent : 0;
    // Just a safety check so we don't cross the array boundary
    if (m_pCurrent < m_pEnd)
    {
      m_pCurrent++;
    }
    else
    {
      atEnd = true;
    }
    m_bitLen += 8;
  }

  // Number of superfluous bits in the buffer
  int bitlen = m_bitLen - numBits;

  // Compose the return value
  // Shift the value so the superfluous bits fall off, and then crop the result with an AND
  uint32_t ret = (m_bitBuffer >> bitlen) & ((1ui64 << numBits) - 1);

  // If we're not peeking, then update the buffer and remove the data we just read
  if (!peek)
  {
    m_bitBuffer &= ((1ui64 << bitlen) - 1);
    m_bitLen = bitlen;
  }

  return ret;
}

// Exponential Golomb Coding (with k = 0)
// As used in H.264/MPEG-4 AVC
// http://en.wikipedia.org/wiki/Exponential-Golomb_coding

uint64_t CByteParser::UExpGolombRead()
{
  int n = -1;
  for (BYTE b = 0; !b; n++)
  {
    b = BitRead(1);
  }
  return (1ui64 << n) - 1 + BitRead(n);
}

int64_t CByteParser::SExpGolombRead()
{
  uint64_t k = UExpGolombRead();
  // Negative numbers are interleaved in the series
  // k:      0, 1,  2, 3,  4, 5,  6, ...
  // Actual: 0, 1, -1, 2, -2, 3, -3, ....
  // So all even numbers are negative (last bit = 0)
  return ((k&1) ? 1 : -1) * ((k + 1) >> 1);
}

void CByteParser::Seek(DWORD pos)
{
  m_pCurrent = m_pData + std::min((DWORD)m_dwLen, pos);
  BitFlush();
}
