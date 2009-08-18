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
\file TextureManager.h
\brief 
*/

#ifndef GUILIB_TEXTUREGL_H
#define GUILIB_TEXTUREGL_H

#include "Texture.h"

#pragma once

#ifdef HAS_GL

/************************************************************************/
/*    CGLTexture                                                       */
/************************************************************************/
class CGLTexture : public CBaseTexture
{
public:
  CGLTexture(unsigned int width = 0, unsigned int height = 0, unsigned int BPP = 0);
  virtual ~CGLTexture();
  
  void Delete();
  bool LoadFromFile(const CStdString& texturePath);
  bool LoadFromMemory(unsigned int height, unsigned int pitch, unsigned int BPP, unsigned char* pPixels); 
  void LoadToGPU();
  unsigned int GetPitch() const;
  unsigned char* GetPixels() const;

protected:
  CGLTexture(CBaseTexture& texture); 
  CBaseTexture& operator = (const CBaseTexture &rhs);
  void Update(int w, int h, int pitch, const unsigned char *pixels, bool loadToGPU);
  void Allocate(unsigned int width, unsigned int height, unsigned int BPP);

  bool NeedPower2Texture();
  unsigned int m_nTextureWidth;
  unsigned int m_nTextureHeight;

};

#endif

#endif
