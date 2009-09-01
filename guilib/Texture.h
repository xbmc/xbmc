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

class CTexture;
class CGLTexture;
class CDXTexture;

#define MAX_PICTURE_WIDTH  2048
#define MAX_PICTURE_HEIGHT 2048

#pragma once

/*!
\ingroup textures
\brief Base texture class, subclasses of which depend on the render spec (DX, GL etc.)
*/
class CBaseTexture
{

public:
  CBaseTexture(unsigned int width = 0, unsigned int height = 0, unsigned int BPP = 0);
  CBaseTexture(const CBaseTexture& texture);  
  virtual ~CBaseTexture();

  virtual CBaseTexture& operator = (const CBaseTexture &rhs) = 0;
  virtual void Delete() = 0;

  virtual bool LoadFromFile(const CStdString& texturePath);
  virtual bool LoadFromMemory(unsigned int width, unsigned int height, unsigned int pitch, unsigned int BPP, unsigned char* pPixels);
  virtual void CreateTextureObject() = 0;
  virtual void DestroyTextureObject() = 0;
  virtual void LoadToGPU() = 0;

  XBMC::TexturePtr GetTextureObject() const { return m_pTexture; }
  virtual unsigned char* GetPixels() const { return m_pPixels; }
  virtual unsigned int GetPitch() const { return m_nTextureWidth * (m_nBPP / 8); }
  virtual unsigned int GetTextureWidth() const { return m_imageWidth; };
  virtual unsigned int GetTextureHeight() const { return m_imageHeight; };
  unsigned int GetWidth() const { return m_imageWidth; }
  unsigned int GetHeight() const { return m_imageHeight; }
  unsigned int GetBPP() const { return m_nBPP; }

  void Update(int w, int h, int pitch, const unsigned char *pixels, bool loadToGPU);
  void Allocate(unsigned int width, unsigned int height, unsigned int BPP);

  static DWORD PadPow2(DWORD x);

protected:
  unsigned int m_imageWidth;
  unsigned int m_imageHeight;
  unsigned int m_nTextureWidth;
  unsigned int m_nTextureHeight;
  unsigned int m_nBPP;
  XBMC::TexturePtr m_pTexture;
  unsigned char* m_pPixels;
  bool m_loadedToGPU;
};

#ifdef HAS_GL
#include "TextureGL.h"
#define CTexture CGLTexture
#elif defined(HAS_DX)
#include "TextureDX.h"
#define CTexture CDXTexture
#endif

#endif
