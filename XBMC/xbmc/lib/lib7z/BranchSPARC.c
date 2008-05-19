/* BranchSPARC.c */

#include "BranchSPARC.h"

UInt32 SPARC_Convert(Byte *data, UInt32 size, UInt32 nowPos, int encoding)
{
  UInt32 i;
  for (i = 0; i + 4 <= size; i += 4)
  {
    if (data[i] == 0x40 && (data[i + 1] & 0xC0) == 0x00 || 
        data[i] == 0x7F && (data[i + 1] & 0xC0) == 0xC0)
    {
      UInt32 src = 
        ((UInt32)data[i + 0] << 24) |
        ((UInt32)data[i + 1] << 16) |
        ((UInt32)data[i + 2] << 8) |
        ((UInt32)data[i + 3]);
      UInt32 dest;
      
      src <<= 2;
      if (encoding)
        dest = nowPos + i + src;
      else
        dest = src - (nowPos + i);
      dest >>= 2;
      
      dest = (((0 - ((dest >> 22) & 1)) << 22) & 0x3FFFFFFF) | (dest & 0x3FFFFF) | 0x40000000;

      data[i + 0] = (Byte)(dest >> 24);
      data[i + 1] = (Byte)(dest >> 16);
      data[i + 2] = (Byte)(dest >> 8);
      data[i + 3] = (Byte)dest;
    }
  }
  return i;
}
