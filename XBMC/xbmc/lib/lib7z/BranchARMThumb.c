/* BranchARMThumb.c */

#include "BranchARMThumb.h"

UInt32 ARMThumb_Convert(Byte *data, UInt32 size, UInt32 nowPos, int encoding)
{
  UInt32 i;
  for (i = 0; i + 4 <= size; i += 2)
  {
    if ((data[i + 1] & 0xF8) == 0xF0 && 
        (data[i + 3] & 0xF8) == 0xF8)
    {
      UInt32 dest;
      UInt32 src = 
        ((data[i + 1] & 0x7) << 19) |
        (data[i + 0] << 11) |
        ((data[i + 3] & 0x7) << 8) |
        (data[i + 2]);
      
      src <<= 1;
      if (encoding)
        dest = nowPos + i + 4 + src;
      else
        dest = src - (nowPos + i + 4);
      dest >>= 1;
      
      data[i + 1] = (Byte)(0xF0 | ((dest >> 19) & 0x7));
      data[i + 0] = (Byte)(dest >> 11);
      data[i + 3] = (Byte)(0xF8 | ((dest >> 8) & 0x7));
      data[i + 2] = (Byte)dest;
      i += 2;
    }
  }
  return i;
}
