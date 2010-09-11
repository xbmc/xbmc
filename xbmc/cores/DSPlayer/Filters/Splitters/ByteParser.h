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

#pragma once

/**
* Byte Parser Utility Class
*/
class CByteParser
{
public:
  /** Construct a Byte Parser to parse the given BYTE array with the given length */
  CByteParser(const BYTE *pData, uint32_t length);
  virtual ~CByteParser();

  /** Read 1 to 32 Bits from the Byte Array. If peek is set, the data will just be returned, and the buffer not advanced. */
  uint32_t BitRead(uint8_t numBits, bool peek = false);

  /** Read a unsigned number in Exponential Golomb encoding (with k = 0) */
  uint64_t UExpGolombRead();
  /** Read a signed number in Exponential Golomb encoding (with k = 0) */
  int64_t SExpGolombRead();

  /** Seek to the position (in bytes) in the byte array */
  void Seek(DWORD pos);

  /** Pointer to the start of the byte array */
  const BYTE *Start() const
  {
    return m_pData;
  }
  /** Pointer to the end of the byte array */
  const BYTE *End() const
  {
    return m_pEnd;
  }

  /** Overall length (in bytes) of the byte array */
  uint32_t Length() const
  {
    return m_dwLen;
  }
  /** Current byte position in the array. Any incomplete bytes ( buffer < 8 bits ) will not be counted */
  uint32_t Pos() const
  {
    return (m_pCurrent - m_pData) - (m_bitLen >> 3);
  }
  /** Number of bytes remaining in the array */
  uint32_t Remaining() const
  {
    return Length() - Pos();
  }

  /** Flush any bits currently in the buffer */
  void BitFlush()
  {
    m_bitLen = 0;
    m_bitBuffer = 0;
  }
  /** Skip bits until the next byte border */
  void BitByteAlign()
  {
    BitRead(m_bitLen & 7);
  }

private:
  // Pointer to the start of the data buffer
  const BYTE * const m_pData;

  // Pointer to the current position in the data buffer
  const BYTE *m_pCurrent;
  // Pointer to the end in the data buffer
  const BYTE * const m_pEnd;

  const uint32_t m_dwLen;

  uint64_t m_bitBuffer;
  uint8_t m_bitLen;
};
