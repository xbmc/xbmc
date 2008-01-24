/*!
\file gui3d.h
\brief 
*/

#ifndef GUILIB_GUI3D_H
#define GUILIB_GUI3D_H
#pragma once

#define ALLOW_TEXTURE_COMPRESSION

#ifdef _XBOX
#define HAS_XBOX_D3D
#ifdef ALLOW_TEXTURE_COMPRESSION
 #define GUI_D3D_FMT D3DFMT_A8R8G8B8
#else
 #define GUI_D3D_FMT D3DFMT_LIN_A8R8G8B8
#endif
#define GAMMA_RAMP_FLAG  D3DSGR_IMMEDIATE

#include <xgraphics.h>
#include <d3d8.h>
#include <d3dx8.h>

#define LPD3DXBUFFER XGBuffer*
#define D3DXAssembleShader(str, len, flags, constants, shader, errors) XGAssembleShader("UNKNOWN", str, len, flags, constants, shader, errors, NULL, NULL, NULL, NULL)

// sadly D3DXCreateTexture won't consider linear formats with non power of 2 textures as valid, thus we use standard instead
#define D3DXCreateTexture(device, width, height, levels, usage, format, pool, texture) (device)->CreateTexture(width, height, levels, usage, format, pool, texture)

#else

#define GAMMA_RAMP_FLAG  D3DSGR_CALIBRATE

#undef HAS_XBOX_D3D

 #include "D3D8.h"
 #include "D3DX8.h"

#define D3DPRESENTFLAG_INTERLACED 0
#define D3DPRESENTFLAG_WIDESCREEN 0
#define D3DPRESENTFLAG_PROGRESSIVE 0

#define D3DFMT_LIN_A8R8G8B8 D3DFMT_A8R8G8B8
#define D3DFMT_LIN_X8R8G8B8 D3DFMT_X8R8G8B8
#define D3DFMT_LIN_L8       D3DFMT_L8
#define D3DFMT_LIN_D16      D3DFMT_D16
#define D3DFMT_LIN_A8       D3DFMT_A8

#define D3DPIXELSHADERDEF DWORD

struct D3DTexture 
{
  DWORD Common;
  DWORD Data;
  DWORD Lock;

  DWORD Format;   // Format information about the texture.
  DWORD Size;     // Size of a non power-of-2 texture, must be zero otherwise
};

#define D3DCOMMON_TYPE_MASK 0x0070000
#define D3DCOMMON_TYPE_TEXTURE 0x0040000

struct D3DPalette
{
  DWORD Common;
  DWORD Data;
  DWORD Lock;
};

typedef D3DPalette* LPDIRECT3DPALETTE8;
#define GUI_D3D_FMT D3DFMT_X8R8G8B8

#endif

#endif
