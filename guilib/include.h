// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#define DEBUG_MOUSE
#define DEBUG_KEYBOARD

#include "system.h"
#include <vector>
#include <list>
#include <map>
#include <set>
#include <memory>
#include <queue>
#include <stdio.h>
#include "StdString.h"
#ifdef _XBOX
#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#include <FStream>
#include <stdlib.h>
#include <crtdbg.h>
#define new new( _NORMAL_BLOCK, __FILE__, __LINE__)
#endif
#endif
#include "../xbmc/utils/log.h"

// guilib internal
#include "gui3d.h"
#include "tinyXML/tinyxml.h"


#ifdef _XBOX
 #ifdef QueryPerformanceFrequency
  #undef QueryPerformanceFrequency
 #endif
 WINBASEAPI BOOL WINAPI QueryPerformanceFrequencyXbox(LARGE_INTEGER *lpFrequency);
 #define QueryPerformanceFrequency(a) QueryPerformanceFrequencyXbox(a)
#endif

#define SAFE_RELEASE(p)      { if(p) { (p)->Release(); (p)=NULL; } }

// TODO: reference additional headers your program requires here
