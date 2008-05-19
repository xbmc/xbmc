/* BranchARM.c */

#include "BranchARM.h"

UInt32 ARM_Convert(Byte *data, UInt32 size, UInt32 nowPos, int encoding)
{
  UInt32 i;
  for (i = 0; i + 4 <= size; i += 4)
  {
    if (data[i + 3] == 0xEB)
    {
      UInt32 dest;
      UInt32 src = (data[i + 2] << 16) | (data[i + 1] << 8) | (data[i + 0]);
      src <<= 2;
      if (encoding)
        dest = nowPos + i + 8 + src;
      else
        dest = src - (nowPos + i + 8);
      dest >>= 2;
      data[i + 2] = (Byte)(dest >> 16);
      data[i + 1] = (Byte)(dest >> 8);
      data[i + 0] = (Byte)dest;
    }
  }
  return i;
}
