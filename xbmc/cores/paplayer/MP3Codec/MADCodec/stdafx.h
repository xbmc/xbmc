// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once


#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#ifndef _LINUX
#include <windows.h>
#else
#include <unistd.h>
#endif


// TODO: reference additional headers your program requires here
