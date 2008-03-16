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

#endif


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
