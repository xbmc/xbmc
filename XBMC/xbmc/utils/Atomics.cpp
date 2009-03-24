/*
 *  Atomics.cpp
 *  XBMC
 *
 *  Created by Chris Lance on 3/24/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include "Atomics.h"

int cas(void* pAddr,long expectedVal, long swapVal)
{
  int ret_val = 0;
  
  __asm
  {
    // Load parameters
    mov eax, expectedVal ;
    mov ebx, pAddr ;
    mov ecx, swapVal ;
    
    // Do Swap
    lock cmpxchg [ebx], ecx ;
    
    // Get Result
    jz success ;
    mov ret_val,0 ;
    jmp end ;
  success:
    mov ret_val,1 ;
  end:
  }
  
  return ret_val;
}