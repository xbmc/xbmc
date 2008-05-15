/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */
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
