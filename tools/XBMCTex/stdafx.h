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

// Thses must be the DX SDK (8.1+) versions, not the XDK versions
#include "/DX90SDK/Include/D3D8.h"
#include "/DX90SDK/Include/D3DX8.h"

#include <XGraphics.h>

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

// Some XBox specific stuff

struct D3DTexture 
{

	DWORD Common;
	DWORD Data;
	DWORD Lock;

	DWORD Format;   // Format information about the texture.
	DWORD Size;     // Size of a non power-of-2 texture, must be zero otherwise
};

struct D3DPalette
{
	DWORD Common;
	DWORD Data;
	DWORD Lock;

};

typedef enum _XB_D3DFORMAT
{
	XB_D3DFMT_UNKNOWN              = 0xFFFFFFFF,

	/* Swizzled formats */

	XB_D3DFMT_A8R8G8B8             = 0x00000006,
	XB_D3DFMT_X8R8G8B8             = 0x00000007,
	XB_D3DFMT_R5G6B5               = 0x00000005,
	XB_D3DFMT_R6G5B5               = 0x00000027,
	XB_D3DFMT_X1R5G5B5             = 0x00000003,
	XB_D3DFMT_A1R5G5B5             = 0x00000002,
	XB_D3DFMT_A4R4G4B4             = 0x00000004,
	XB_D3DFMT_A8                   = 0x00000019,
	XB_D3DFMT_A8B8G8R8             = 0x0000003A,
	XB_D3DFMT_B8G8R8A8             = 0x0000003B,
	XB_D3DFMT_R4G4B4A4             = 0x00000039,
	XB_D3DFMT_R5G5B5A1             = 0x00000038,
	XB_D3DFMT_R8G8B8A8             = 0x0000003C,
	XB_D3DFMT_R8B8                 = 0x00000029,
	XB_D3DFMT_G8B8                 = 0x00000028,

	XB_D3DFMT_P8                   = 0x0000000B,

	XB_D3DFMT_L8                   = 0x00000000,
	XB_D3DFMT_A8L8                 = 0x0000001A,
	XB_D3DFMT_AL8                  = 0x00000001,
	XB_D3DFMT_L16                  = 0x00000032,

	XB_D3DFMT_V8U8                 = 0x00000028,
	XB_D3DFMT_L6V5U5               = 0x00000027,
	XB_D3DFMT_X8L8V8U8             = 0x00000007,
	XB_D3DFMT_Q8W8V8U8             = 0x0000003A,
	XB_D3DFMT_V16U16               = 0x00000033,

	XB_D3DFMT_D16_LOCKABLE         = 0x0000002C,
	XB_D3DFMT_D16                  = 0x0000002C,
	XB_D3DFMT_D24S8                = 0x0000002A,
	XB_D3DFMT_F16                  = 0x0000002D,
	XB_D3DFMT_F24S8                = 0x0000002B,

	/* YUV formats */

	XB_D3DFMT_YUY2                 = 0x00000024,
	XB_D3DFMT_UYVY                 = 0x00000025,

	/* Compressed formats */

	XB_D3DFMT_DXT1                 = 0x0000000C,
	XB_D3DFMT_DXT2                 = 0x0000000E,
	XB_D3DFMT_DXT3                 = 0x0000000E,
	XB_D3DFMT_DXT4                 = 0x0000000F,
	XB_D3DFMT_DXT5                 = 0x0000000F,

	/* Linear formats */

	XB_D3DFMT_LIN_A1R5G5B5         = 0x00000010,
	XB_D3DFMT_LIN_A4R4G4B4         = 0x0000001D,
	XB_D3DFMT_LIN_A8               = 0x0000001F,
	XB_D3DFMT_LIN_A8B8G8R8         = 0x0000003F,
	XB_D3DFMT_LIN_A8R8G8B8         = 0x00000012,
	XB_D3DFMT_LIN_B8G8R8A8         = 0x00000040,
	XB_D3DFMT_LIN_G8B8             = 0x00000017,
	XB_D3DFMT_LIN_R4G4B4A4         = 0x0000003E,
	XB_D3DFMT_LIN_R5G5B5A1         = 0x0000003D,
	XB_D3DFMT_LIN_R5G6B5           = 0x00000011,
	XB_D3DFMT_LIN_R6G5B5           = 0x00000037,
	XB_D3DFMT_LIN_R8B8             = 0x00000016,
	XB_D3DFMT_LIN_R8G8B8A8         = 0x00000041,
	XB_D3DFMT_LIN_X1R5G5B5         = 0x0000001C,
	XB_D3DFMT_LIN_X8R8G8B8         = 0x0000001E,

	XB_D3DFMT_LIN_A8L8             = 0x00000020,
	XB_D3DFMT_LIN_AL8              = 0x0000001B,
	XB_D3DFMT_LIN_L16              = 0x00000035,
	XB_D3DFMT_LIN_L8               = 0x00000013,

	XB_D3DFMT_LIN_V16U16           = 0x00000036,
	XB_D3DFMT_LIN_V8U8             = 0x00000017,
	XB_D3DFMT_LIN_L6V5U5           = 0x00000037,
	XB_D3DFMT_LIN_X8L8V8U8         = 0x0000001E,
	XB_D3DFMT_LIN_Q8W8V8U8         = 0x00000012,

	XB_D3DFMT_LIN_D24S8            = 0x0000002E,
	XB_D3DFMT_LIN_F24S8            = 0x0000002F,
	XB_D3DFMT_LIN_D16              = 0x00000030,
	XB_D3DFMT_LIN_F16              = 0x00000031,

	XB_D3DFMT_VERTEXDATA           = 100,
	XB_D3DFMT_INDEX16              = 101,

	XB_D3DFMT_FORCE_DWORD          =0x7fffffff
} XB_D3DFORMAT;
