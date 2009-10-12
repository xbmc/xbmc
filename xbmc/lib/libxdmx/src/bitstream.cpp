
#include "common.h"
#include "bitstream.h"

CSimpleBitstreamReader::CSimpleBitstreamReader(unsigned char* pData, unsigned int dataLen) : 
  m_pData(pData), 
  m_BytesLeft(dataLen), 
  m_BitOffset(0)
{

}

bool CSimpleBitstreamReader::ReadBit()
{
  char out = 0;
  ReadBits(1, (unsigned char*)&out);
  return (out != 0);
}

char CSimpleBitstreamReader::ReadChar(int bits)
{
  if (bits > 8)
    return 0;
  char out = 0;
  ReadBits(bits, (unsigned char*)&out);
  return out;
}

short CSimpleBitstreamReader::ReadShort(int bits)
{
  if (bits > 16)
    return 0;
  short out = 0;
  ReadBits(bits, (unsigned char*)&out);
  return out;
}

int32_t CSimpleBitstreamReader::ReadInt32(int bits)
{
  if (bits > 32)
    return 0;
  int32_t out = 0;
  ReadBits(bits, (unsigned char*)&out);
  return out;
}

int64_t CSimpleBitstreamReader::ReadInt64(int bits)
{
  if (bits > 32)
    return 0;
  int64_t out = 0;
  ReadBits(bits, (unsigned char*)&out);
  return out;
}  

unsigned char* CSimpleBitstreamReader::GetCurrentPointer()
{
  if (m_BytesLeft)
    return m_pData;

  return NULL;
}

unsigned int CSimpleBitstreamReader::GetBitOffset()
{
  return m_BitOffset;
}

unsigned int CSimpleBitstreamReader::GetBytesLeft()
{
  return m_BytesLeft;
}

unsigned int CSimpleBitstreamReader::UpdatePosition(int bits)
{
  if (bits > 8)
  {
    if (m_BytesLeft)
    {
      // Remainder bits
      m_pData++;
      m_BytesLeft--;
      m_BitOffset = 0;
      bits -= (8 - m_BitOffset);

      // Complete bytes
      unsigned int bytes = bits >> 3;
      if (m_BytesLeft < bytes)
        m_BytesLeft = 0;
      else
      {
        m_pData += bytes;
        m_BytesLeft -= bytes;
        bits -= (bytes << 3);
        m_BitOffset = bits; // New remainder
      }
    }
  }
  else
  {
    m_BitOffset += bits;
    if (m_BitOffset > 7)
    {
      m_BitOffset = 0;
      if (m_BytesLeft)
      {
        m_pData++;
        m_BytesLeft--;
      }
    }
  }
  return m_BytesLeft;
}

char CSimpleBitstreamReader::GetBits(char byte, int offset, int bits)
{
  unsigned char bitmask[] = {0x00,0x01,0x03,0x07,0x0f,0x1f,0x3f,0x7f,0xff};
  int shift = (7 - offset) - (bits - 1);
  if (shift == 0)
    return byte & bitmask[bits];
  else if (shift > 0)
    return (byte >> shift) & bitmask[bits];
  else
    return 0;
}

bool CSimpleBitstreamReader::ReadBits(unsigned int bits, unsigned char* pOut)
{
  unsigned int bitsLeft = 8 - m_BitOffset;
  if (((m_BytesLeft << 3) + bitsLeft) < bits)
    return false; // Not enough data

  if (bits <= bitsLeft) // Simple read
  {
    *pOut = GetBits(*m_pData, m_BitOffset, bits);
    UpdatePosition(bits);
    return true;
  }

  unsigned int danglingBits = bits % 8;
  unsigned int bytes = (bits >> 3);

  unsigned char bitmask[] = {0x00,0x01,0x03,0x07,0x0f,0x1f,0x3f,0x7f,0xff};
  if (!m_BitOffset) // Aligned read
  {
    if (danglingBits)
    {
      char highBits = 0;
      for (unsigned int b = 0; b < bytes+1; b++)
      {
        pOut[bytes - b] = (highBits << danglingBits) | (m_pData[b] >> (8 - danglingBits));
        highBits = m_pData[b] & bitmask[8 - danglingBits];
      }
      // TODO: This returns an incorrect value
//      pOut[0] = m_pData[bytes] & (bitmask[danglingBits] << (8 - danglingBits)); // LSB
      m_BitOffset = danglingBits;
    }
    else
    {
      for (unsigned int b = 0; b < bytes; b++)
      {
        pOut[bytes - b - 1] = m_pData[b];
      }
    }
    m_pData += bytes;
    m_BytesLeft -= bytes;
    return true;
  }

  unsigned int outOffset = bits % 8; // This is how many leftover bits will be written to the highest-order output byte
  if (outOffset)
  {
    // TODO: The two offset cases likely do not work
    if (outOffset > bitsLeft)
    {
      // Get what we can
      pOut[bytes] = GetBits(*m_pData, m_BitOffset, bitsLeft); // Finish off this input byte
      m_pData++;
      m_BytesLeft--;
      // Sync vars to changes
      bitsLeft = outOffset - bitsLeft;
      m_BitOffset = 8 - bitsLeft;
      // Shift into position
      pOut[bytes] <<= bitsLeft;
      // Finish up highest-order byte
      pOut[--bytes] = GetBits(*m_pData, m_BitOffset, bitsLeft);
    }
    else if (outOffset < bitsLeft)
    {
      // Get the bits that will fit
      pOut[bytes] = GetBits(*m_pData, m_BitOffset, outOffset);
      // Sync vars to changes
      m_BitOffset += outOffset;
      bitsLeft = 8 - m_BitOffset;
    }
    else // They are equal
    {
      pOut[bytes] = GetBits(*m_pData, m_BitOffset, bitsLeft);
      m_pData++;
      m_BytesLeft--;
      m_BitOffset = 0;
      bitsLeft = 8;
    }
    while (bytes)
    {
      pOut[bytes-1] = GetBits(*m_pData, m_BitOffset, bitsLeft) << m_BitOffset;
      m_pData++;
      m_BytesLeft--;
      if (m_BitOffset)
        pOut[bytes-1] |= GetBits(*m_pData, bitsLeft, m_BitOffset);
      bytes--;
   }
  }
  return true;
}
