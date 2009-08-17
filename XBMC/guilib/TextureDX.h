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
\file TextureDX.h
\brief 
*/

#ifndef GUILIB_TEXTUREDX_H
#define GUILIB_TEXTUREDX_H

#include "RenderSystem.h"
#include "Texture.h"

#pragma once

#ifdef HAS_DX

/************************************************************************/
/*    CDXTexture                                                       */
/************************************************************************/
class CDXTexture : public CBaseTexture
{
public:
  CDXTexture();
  virtual ~CDXTexture();

  void Allocate(unsigned int width, unsigned int height, unsigned int BPP);
 
  void Delete();
  bool LoadFromFile(const CStdString& texturePath);
  bool LoadFromMemory(unsigned int height, unsigned int pitch, unsigned int BPP, unsigned char* pPixels); 
  virtual void LoadToGPU() { }


protected:
  CDXTexture(CBaseTexture& texture);
   CBaseTexture& operator = (const CBaseTexture &rhs);
};

#endif

#endif
