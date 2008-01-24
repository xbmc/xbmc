// stdafx.cpp : source file that includes just the standard includes
// guiTest.pch will be the pre-compiled header
// stdafx.obj will contain the pre-compiled type information

#include "stdafx.h"

#undef QueryPerformanceFrequency

__int64 lFrequency = 0LL;
WINBASEAPI BOOL WINAPI QueryPerformanceFrequencyXbox(LARGE_INTEGER *lpFrequency)
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
    int stepping = dwStandard & 0xF;

    if( model == 11 )
    {
      //This is likely the DreamX 1480      
      //so only support fullspeed mode
      lFrequency = 1481200000;
    }
    else if( model == 8 && stepping == 6 )
    {
      //Upgraded 1ghz cpu (Intel Pentium III Coppermine)
      lFrequency = 999985000;
    }
    else
    {      
      QueryPerformanceFrequency((LARGE_INTEGER*)&lFrequency);
    }
  }

  (*lpFrequency).QuadPart = lFrequency;
  return TRUE;
}