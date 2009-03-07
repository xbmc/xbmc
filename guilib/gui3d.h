/*!
\file gui3d.h
\brief
*/

#ifndef GUILIB_GUI3D_H
#define GUILIB_GUI3D_H
#pragma once

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

#define GAMMA_RAMP_FLAG  D3DSGR_CALIBRATE

#ifndef HAS_SDL
 #include "D3D8.h"
 #include "D3DX8.h"
#elif !defined(_LINUX)  // TODO - this stuff is in PlatformDefs.h otherwise, but should be merged to some local file
  // Basic D3D stuff
  typedef DWORD D3DCOLOR;

  typedef enum _D3DFORMAT
  {
    D3DFMT_A8R8G8B8 = 0x00000006,
    D3DFMT_DXT1     = 0x0000000C,
    D3DFMT_DXT2     = 0x0000000E,
    D3DFMT_DXT4     = 0x0000000F,
    D3DFMT_UNKNOWN  = 0xFFFFFFFF
  } D3DFORMAT;

  typedef enum D3DRESOURCETYPE
  {
      D3DRTYPE_SURFACE = 1,
      D3DRTYPE_VOLUME = 2,
      D3DRTYPE_TEXTURE = 3,
      D3DRTYPE_VOLUMETEXTURE = 4,
      D3DRTYPE_CubeTexture = 5,
      D3DRTYPE_VERTEXBUFFER = 6,
      D3DRTYPE_INDEXBUFFER = 7,
      D3DRTYPE_FORCE_DWORD = 0x7fffffff,
  } D3DRESOURCETYPE, *LPD3DRESOURCETYPE;

  typedef enum D3DXIMAGE_FILEFORMAT
  {
      D3DXIFF_BMP = 0,
      D3DXIFF_JPG = 1,
      D3DXIFF_TGA = 2,
      D3DXIFF_PNG = 3,
      D3DXIFF_DDS = 4,
      D3DXIFF_PPM = 5,
      D3DXIFF_DIB = 6,
      D3DXIFF_HDR = 7,
      D3DXIFF_PFM = 8,
      D3DXIFF_FORCE_DWORD = 0x7fffffff,
  } D3DXIMAGE_FILEFORMAT, *LPD3DXIMAGE_FILEFORMAT;

  typedef struct D3DXIMAGE_INFO {
      UINT Width;
      UINT Height;
      UINT Depth;
      UINT MipLevels;
      D3DFORMAT Format;
      D3DRESOURCETYPE ResourceType;
      D3DXIMAGE_FILEFORMAT ImageFileFormat;
  } D3DXIMAGE_INFO, *LPD3DXIMAGE_INFO;

  typedef struct _D3DPRESENT_PARAMETERS_
  {
      UINT                BackBufferWidth;
      UINT                BackBufferHeight;
      D3DFORMAT           BackBufferFormat;
      UINT                BackBufferCount;
      //D3DMULTISAMPLE_TYPE MultiSampleType;
      //D3DSWAPEFFECT       SwapEffect;
      //HWND                hDeviceWindow;
      BOOL                Windowed;
      BOOL                EnableAutoDepthStencil;
      D3DFORMAT           AutoDepthStencilFormat;
      DWORD               Flags;
      UINT                FullScreen_RefreshRateInHz;
      UINT                FullScreen_PresentationInterval;
      //D3DSurface         *BufferSurfaces[3];
      //D3DSurface         *DepthStencilSurface;
  } D3DPRESENT_PARAMETERS;

  typedef enum D3DPRIMITIVETYPE
  {
      D3DPT_POINTLIST = 1,
      D3DPT_LINELIST = 2,
      D3DPT_LINESTRIP = 3,
      D3DPT_TRIANGLELIST = 4,
      D3DPT_TRIANGLESTRIP = 5,
      D3DPT_TRIANGLEFAN = 6,
      D3DPT_FORCE_DWORD = 0x7fffffff,
  } D3DPRIMITIVETYPE, *LPD3DPRIMITIVETYPE;

  typedef struct _D3DMATRIX {
      union {
          struct {
              float        _11, _12, _13, _14;
              float        _21, _22, _23, _24;
              float        _31, _32, _33, _34;
              float        _41, _42, _43, _44;
          };
          float m[4][4];
      };
  } D3DMATRIX;

  typedef void DIRECT3DTEXTURE8;
  typedef void* LPDIRECT3DTEXTURE8;
#endif

#define D3DPRESENTFLAG_INTERLACED 1
#define D3DPRESENTFLAG_WIDESCREEN 2
#define D3DPRESENTFLAG_PROGRESSIVE 4

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

#ifdef HAS_SDL
#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#ifdef HAS_SDL_OPENGL
#if defined(_LINUX) && !defined(GL_GLEXT_PROTOTYPES)
#define GL_GLEXT_PROTOTYPES
#endif
#include <GL/glew.h>
#endif
#endif

#endif
