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

  virtual bool LoadFromFile(const CStdString& texturePath) = 0;
  virtual bool LoadFromMemory(unsigned int width, unsigned int height, unsigned int pitch, unsigned int BPP, unsigned char* pPixels) = 0; 
  virtual void LoadToGPU() = 0;

  XBMC::TexturePtr GetTextureObject() const { return m_pTexture; }
  virtual unsigned char* GetPixels() const { return NULL; }
  virtual unsigned int GetPitch() const { return 0; }
  virtual unsigned int GetTextureWidth() const { return m_imageWidth; };
  virtual unsigned int GetTextureHeight() const { return m_imageHeight; };
  unsigned int GetWidth() const { return m_imageWidth; }
  unsigned int GetHeight() const { return m_imageHeight; }
  unsigned int GetBPP() const { return m_nBPP; }

protected:
  virtual void Allocate(unsigned int width, unsigned int height, unsigned int BPP) = 0;

  unsigned int m_imageWidth;
  unsigned int m_imageHeight;
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
