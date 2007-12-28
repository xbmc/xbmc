// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#define WIN32_LEAN_AND_MEAN   // Exclude rarely-used stuff from Windows headers
#define _WIN32_WINNT 0x500    // Win 2k/XP REQUIRED!
#include <windows.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <malloc.h>

#include <vector>
#include <list>

// Thses must be the DX SDK (8.1+) versions, not the XDK versions
#include "D3D8.h"
#include "D3DX8.h"

#ifdef USE_XDK
#include <XGraphics.h>
#endif

#include "xbox.h"

// Round a number to the nearest power of 2 rounding up
// runs pretty quickly - the only expensive op is the bsr
// alternive would be to dec the source, round down and double the result
// which is slightly faster but rounds 1 to 2 
#if defined(_MSC_VER)  
DWORD __forceinline __stdcall PadPow2(DWORD x)
{
	__asm {
		mov edx,x    // put the value in edx
			xor ecx,ecx  // clear ecx - if x is 0 bsr doesn't alter it
			bsr ecx,edx  // find MSB position
			mov eax,1    // shift 1 by result effectively
			shl eax,cl   // doing a round down to power of 2
			cmp eax,edx  // check if x was already a power of two
			adc ecx,0    // if it wasn't then CF is set so add to ecx
			mov eax,1    // shift 1 by result again, this does a round
			shl eax,cl   // up as a result of adding CF to ecx
	}
	// return result in eax
}
#else
DWORD inline PadPow2(DWORD x) 
{
  --x;
  x |= x >> 1;
  x |= x >> 2;
  x |= x >> 4;
  x |= x >> 8;
  x |= x >> 16;
  return ++x;
}
#endif

// Debug macros
#if defined(_DEBUG) && defined(_MSC_VER) 
#include <crtdbg.h>
#define TRACE0(f)                 _RPT0(_CRT_WARN, f)
#define TRACE1(f, a)              _RPT1(_CRT_WARN, f, a)
#define TRACE2(f, a, b)           _RPT2(_CRT_WARN, f, a, b)
#define TRACE3(f, a, b, c)        _RPT3(_CRT_WARN, f, a, b, c)
#define TRACE4(f, a, b, c, d)     _RPT4(_CRT_WARN, f, a, b, c, d)
#define TRACE5(f, a, b, c, d, e)  _RPT_BASE((_CRT_WARN, NULL, 0, NULL, f, a, b, c, d, e))

#else

#define TRACE0(f)
#define TRACE1(f, a)
#define TRACE2(f, a, b)
#define TRACE3(f, a, b, c)
#define TRACE4(f, a, b, c, d)
#define TRACE5(f, a, b, c, d, e)

#endif


