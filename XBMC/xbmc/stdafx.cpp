// stdafx.cpp : source file that includes just the standard includes
// guiTest.pch will be the pre-compiled header
// stdafx.obj will contain the pre-compiled type information

#include "stdafx.h"
#undef QueryPerformanceFrequency

__int64 lFrequency = 0LL;
BOOL QueryPerformanceFrequencyXbox(LARGE_INTEGER *lpFrequency)
{
  if( lFrequency == 0LL )
  {
    DWORD dwStandard;
    _asm {
      // get the Standard bits
      mov eax, 1
      cpuid
      mov dwStandard, eax
    }

    int model = (dwStandard >> 4) & 0xF;

    if( model == 11 )
    {
      //This is likely the DreamX 1480
      //we don't know what mode it is in for them moment thou.
      //so only support fullspeed mode
      lFrequency = 1480000000;
    }
    else
    {      
      QueryPerformanceFrequency((LARGE_INTEGER*)&lFrequency);
    }
  }

  (*lpFrequency).QuadPart = lFrequency;
  return TRUE;
}