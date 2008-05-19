// BranchX86_2.h

#ifndef __BRANCHX86_2_H
#define __BRANCHX86_2_H

#include "BranchTypes.h"

#define BCJ2_RESULT_OK 0
#define BCJ2_RESULT_DATA_ERROR 1

/*
Conditions:
  outSize <= FullOutputSize, 
  where FullOutputSize is full size of output stream of x86_2 filter.

If buf0 overlaps outBuf, there are two required conditions:
  1) (buf0 >= outBuf)
  2) (buf0 + size0 >= outBuf + FullOutputSize).
*/

int x86_2_Decode(
    const Byte *buf0, SizeT size0, 
    const Byte *buf1, SizeT size1, 
    const Byte *buf2, SizeT size2, 
    const Byte *buf3, SizeT size3, 
    Byte *outBuf, SizeT outSize);

#endif
