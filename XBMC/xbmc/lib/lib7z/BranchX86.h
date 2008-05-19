/* BranchX86.h */

#ifndef __BRANCHX86_H
#define __BRANCHX86_H

#include "BranchTypes.h"

#define x86_Convert_Init(state) { state = 0; }

SizeT x86_Convert(Byte *buffer, SizeT endPos, UInt32 nowPos, UInt32 *state, int encoding);

#endif
