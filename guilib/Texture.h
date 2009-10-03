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
#include "StdString.h"
#include "XBTF.h"

#pragma pack(1)
struct COLOR {unsigned char b,g,r,x;};	// Windows GDI expects 4bytes per color
#pragma pack()

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
  CBaseTexture(unsigned int width = 0, unsigned int height = 0, unsigned int BPP = 0, unsigned int format = XB_FMT_B8G8R8A8);
  virtual ~CBaseTexture();

  // TODO: Clean up this interface once things have settled down (none of these need to be virtual)
  virtual bool LoadFromFile(const CStdString& texturePath);
  virtual bool LoadFromMemory(unsigned int width, unsigned int height, unsigned int pitch, unsigned int BPP, unsigned int format, unsigned char* pPixels);
  bool LoadPaletted(unsigned int width, unsigned int height, unsigned int pitch, unsigned int format, const unsigned char *pixels, const COLOR *palette);

  virtual void CreateTextureObject() = 0;
  virtual void DestroyTextureObject() = 0;
  virtual void LoadToGPU() = 0;

  XBMC::TexturePtr GetTextureObject() const { return m_pTexture; }
  virtual unsigned char* GetPixels() const { return m_pPixels; }
  virtual unsigned int GetPitch() const { return m_nTextureWidth * (m_nBPP / 8); }
  virtual unsigned int GetTextureWidth() const { return m_nTextureWidth; };
  virtual unsigned int GetTextureHeight() const { return m_nTextureHeight; };
  unsigned int GetWidth() const { return m_imageWidth; }
  unsigned int GetHeight() const { return m_imageHeight; }
  unsigned int GetBPP() const { return m_nBPP; }

  void Update(int w, int h, int pitch, unsigned int format, const unsigned char *pixels, bool loadToGPU);
  void Allocate(unsigned int width, unsigned int height, unsigned int BPP, unsigned int format);

  static unsigned int PadPow2(unsigned int x);

protected:
  unsigned int m_imageWidth;
  unsigned int m_imageHeight;
  unsigned int m_nTextureWidth;
  unsigned int m_nTextureHeight;
  unsigned int m_nBPP;
  XBMC::TexturePtr m_pTexture;
  unsigned char* m_pPixels;
  unsigned int m_bufferSize;
  bool m_loadedToGPU;
  unsigned int m_format;
};

#if defined(HAS_GL) || defined(HAS_GLES)
#include "TextureGL.h"
#define CTexture CGLTexture
#elif defined(HAS_DX)
#include "TextureDX.h"
#define CTexture CDXTexture
#endif

#endif
