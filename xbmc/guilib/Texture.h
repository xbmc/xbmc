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

/*!
\file Texture.h
\brief
*/

#ifndef GUILIB_TEXTURE_H
#define GUILIB_TEXTURE_H

#include "gui3d.h"
#include "utils/StdString.h"
#include "XBTF.h"

#pragma pack(1)
struct COLOR {unsigned char b,g,r,x;};	// Windows GDI expects 4bytes per color
#pragma pack()

#ifdef HAS_DX
#include "D3DResource.h"
#endif

class CTexture;
class CGLTexture;
class CDXTexture;

#pragma once

/*!
\ingroup textures
\brief Base texture class, subclasses of which depend on the render spec (DX, GL etc.)
*/
class CBaseTexture
{

public:
  CBaseTexture(unsigned int width = 0, unsigned int height = 0, unsigned int format = XB_FMT_A8R8G8B8);
  virtual ~CBaseTexture();

  bool LoadFromFile(const CStdString& texturePath, unsigned int maxHeight = 0, unsigned int maxWidth = 0,
                    bool autoRotate = false, unsigned int *originalWidth = NULL, unsigned int *originalHeight = NULL);
  bool LoadFromMemory(unsigned int width, unsigned int height, unsigned int pitch, unsigned int format, bool hasAlpha, unsigned char* pixels);
  bool LoadPaletted(unsigned int width, unsigned int height, unsigned int pitch, unsigned int format, const unsigned char *pixels, const COLOR *palette);

  bool HasAlpha() const;

  virtual void CreateTextureObject() = 0;
  virtual void DestroyTextureObject() = 0;
  virtual void LoadToGPU() = 0;

  XBMC::TexturePtr GetTextureObject() const
  {
#ifdef HAS_DX
    return m_texture.Get();
#else
    return m_texture;
#endif
  }
  unsigned char* GetPixels() const { return m_pixels; }
  unsigned int GetPitch() const { return GetPitch(m_textureWidth); }
  unsigned int GetRows() const { return GetRows(m_textureHeight); }
  unsigned int GetTextureWidth() const { return m_textureWidth; }
  unsigned int GetTextureHeight() const { return m_textureHeight; }
  unsigned int GetWidth() const { return m_imageWidth; }
  unsigned int GetHeight() const { return m_imageHeight; }
  int GetOrientation() const { return m_orientation; }

  void Update(unsigned int width, unsigned int height, unsigned int pitch, unsigned int format, const unsigned char *pixels, bool loadToGPU);
  void Allocate(unsigned int width, unsigned int height, unsigned int format);
  void ClampToEdge();

  static unsigned int PadPow2(unsigned int x);
  bool SwapBlueRed(unsigned char *pixels, unsigned int height, unsigned int pitch, unsigned int elements = 4, unsigned int offset=0);

protected:
  // helpers for computation of texture parameters for compressed textures
  unsigned int GetPitch(unsigned int width) const;
  unsigned int GetRows(unsigned int height) const;
  unsigned int GetBlockSize() const;

  unsigned int m_imageWidth;
  unsigned int m_imageHeight;
  unsigned int m_textureWidth;
  unsigned int m_textureHeight;
#ifdef HAS_DX
  CD3DTexture m_texture;
#else
  XBMC::TexturePtr m_texture;
#endif
  unsigned char* m_pixels;
  bool m_loadedToGPU;
  unsigned int m_format;
  int m_orientation;
  bool m_hasAlpha;
};

#if defined(HAS_GL) || defined(HAS_GLES)
#include "TextureGL.h"
#define CTexture CGLTexture
#elif defined(HAS_DX)
#include "TextureDX.h"
#define CTexture CDXTexture
#endif

#endif
