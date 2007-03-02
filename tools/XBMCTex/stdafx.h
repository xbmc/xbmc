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

// Debug macros
#include <crtdbg.h>

#ifdef _DEBUG

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

#include "xbox.h"