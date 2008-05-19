// BranchX86_2.c

#include "BranchX86_2.h"

#include "Alloc.h"

#ifdef _LZMA_PROB32
#define CProb UInt32
#else
#define CProb UInt16
#endif

#define IsJcc(b0, b1) ((b0) == 0x0F && ((b1) & 0xF0) == 0x80)
#define IsJ(b0, b1) ((b1 & 0xFE) == 0xE8 || IsJcc(b0, b1))

#define kNumTopBits 24
#define kTopValue ((UInt32)1 << kNumTopBits)

#define kNumBitModelTotalBits 11
#define kBitModelTotal (1 << kNumBitModelTotalBits)
#define kNumMoveBits 5

#define RC_READ_BYTE (*Buffer++)

#define RC_INIT2 Code = 0; Range = 0xFFFFFFFF; \
  { int i; for(i = 0; i < 5; i++) { RC_TEST; Code = (Code << 8) | RC_READ_BYTE; }}

#define RC_TEST { if (Buffer == BufferLim) return BCJ2_RESULT_DATA_ERROR; }

#define RC_INIT(buffer, bufferSize) Buffer = buffer; BufferLim = buffer + bufferSize; RC_INIT2
 
#define RC_NORMALIZE if (Range < kTopValue) { RC_TEST; Range <<= 8; Code = (Code << 8) | RC_READ_BYTE; }

#define IfBit0(p) RC_NORMALIZE; bound = (Range >> kNumBitModelTotalBits) * *(p); if (Code < bound)
#define UpdateBit0(p) Range = bound; *(p) += (kBitModelTotal - *(p)) >> kNumMoveBits;
#define UpdateBit1(p) Range -= bound; Code -= bound; *(p) -= (*(p)) >> kNumMoveBits;
// #define UpdateBit0(p) Range = bound; *(p) = (CProb)(*(p) + ((kBitModelTotal - *(p)) >> kNumMoveBits));
// #define UpdateBit1(p) Range -= bound; Code -= bound; *(p) = (CProb)(*(p) - (*(p) >> kNumMoveBits));

int x86_2_Decode(
    const Byte *buf0, SizeT size0, 
    const Byte *buf1, SizeT size1, 
    const Byte *buf2, SizeT size2, 
    const Byte *buf3, SizeT size3, 
    Byte *outBuf, SizeT outSize)
{
  CProb p[256 + 2];
  SizeT inPos = 0, outPos = 0;

  const Byte *Buffer, *BufferLim;
  UInt32 Range, Code;
  Byte prevByte = 0;

  unsigned int i;
  for (i = 0; i < sizeof(p) / sizeof(p[0]); i++)
    p[i] = kBitModelTotal >> 1; 
  RC_INIT(buf3, size3);

  if (outSize == 0)
    return BCJ2_RESULT_OK;

  for (;;)
  {
    Byte b;
    CProb *prob;
    UInt32 bound;

    SizeT limit = size0 - inPos;
    if (outSize - outPos < limit)
      limit = outSize - outPos;
    while (limit != 0)
    {
      Byte b = buf0[inPos];
      outBuf[outPos++] = b;
      if (IsJ(prevByte, b))
        break;
      inPos++;
      prevByte = b;
      limit--;
    }

    if (limit == 0 || outPos == outSize)
      break;

    b = buf0[inPos++];

    if (b == 0xE8)
      prob = p + prevByte;
    else if (b == 0xE9)
      prob = p + 256;
    else
      prob = p + 257;

    IfBit0(prob)
    {
      UpdateBit0(prob)
      prevByte = b;
    }
    else
    {
      UInt32 dest;
      const Byte *v;
      UpdateBit1(prob)
      if (b == 0xE8)
      {
        v = buf1;
        if (size1 < 4)
          return BCJ2_RESULT_DATA_ERROR;
        buf1 += 4;
        size1 -= 4;
      }
      else
      {
        v = buf2;
        if (size2 < 4)
          return BCJ2_RESULT_DATA_ERROR;
        buf2 += 4;
        size2 -= 4;
      }
      dest = (((UInt32)v[0] << 24) | ((UInt32)v[1] << 16) | 
          ((UInt32)v[2] << 8) | ((UInt32)v[3])) - ((UInt32)outPos + 4);
      outBuf[outPos++] = (Byte)dest;
      if (outPos == outSize)
        break;
      outBuf[outPos++] = (Byte)(dest >> 8);
      if (outPos == outSize)
        break;
      outBuf[outPos++] = (Byte)(dest >> 16);
      if (outPos == outSize)
        break;
      outBuf[outPos++] = prevByte = (Byte)(dest >> 24);
    }
  }
  return (outPos == outSize) ? BCJ2_RESULT_OK : BCJ2_RESULT_DATA_ERROR;
}
