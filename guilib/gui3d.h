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

#include "system.h" // for WIN32 types

#define GAMMA_RAMP_FLAG  D3DSGR_CALIBRATE

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

#if defined(HAS_GL) || defined(HAS_GLES)

namespace XBMC
{
  typedef void*  DevicePtr;
  typedef GLuint SurfacePtr;
  typedef GLuint TexturePtr;
  typedef void* PalettePtr; // elis change it
  typedef GLint PixelFormat; // elis change it
}

#if defined(_LINUX) && !defined(GL_GLEXT_PROTOTYPES)
#define GL_GLEXT_PROTOTYPES
#endif

#endif // HAS_GL

#ifdef HAS_DX

namespace XBMC
{
  typedef LPDIRECT3DDEVICE9  DevicePtr;
  typedef LPDIRECT3DTEXTURE9 TexturePtr;
  typedef LPDIRECT3DSURFACE9 SurfacePtr;
  typedef LPDIRECT3DPALETTE8 PalettePtr;
};

#define DELETE_TEXTURE(texture) texture->Release()

#endif // HAS_DX

#ifdef HAS_GLES

#define GLchar char

#endif
#endif // GUILIB_GUI3D_H
