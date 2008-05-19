/* BranchPPC.c */

#include "BranchPPC.h"

UInt32 PPC_B_Convert(Byte *data, UInt32 size, UInt32 nowPos, int encoding)
{
  UInt32 i;
  for (i = 0; i + 4 <= size; i += 4)
  {
    /* PowerPC branch 6(48) 24(Offset) 1(Abs) 1(Link) */
    if ((data[i] >> 2) == 0x12 && 
    (
      (data[i + 3] & 3) == 1 
      /* || (data[i+3] & 3) == 3 */
      )
    )
    {
      UInt32 src = ((data[i + 0] & 3) << 24) |
        (data[i + 1] << 16) |
        (data[i + 2] << 8) |
        (data[i + 3] & (~3));
      
      UInt32 dest;
      if (encoding)
        dest = nowPos + i + src;
      else
        dest = src - (nowPos + i);
      data[i + 0] = (Byte)(0x48 | ((dest >> 24) &  0x3));
      data[i + 1] = (Byte)(dest >> 16);
      data[i + 2] = (Byte)(dest >> 8);
      data[i + 3] &= 0x3;
      data[i + 3] |= dest;
    }
  }
  return i;
}
