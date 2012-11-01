/*
*      Copyright (C) 2005-2012 Team XBMC
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
*  along with XBMC; see the file COPYING.  If not, see
*  <http://www.gnu.org/licenses/>.
*
*/

#pragma once

#include "Texture.h"

#if defined(HAS_GL) || defined(HAS_GLES)

#include "system_gl.h"

/************************************************************************/
/*    CGLTexture                                                       */
/************************************************************************/
class CGLTexture : public CBaseTexture
{
public:
  CGLTexture(unsigned int width = 0, unsigned int height = 0, unsigned int format = XB_FMT_A8R8G8B8);
  virtual ~CGLTexture();

  void CreateTextureObject();
  virtual void DestroyTextureObject();
  void LoadToGPU();
  void BindToUnit(unsigned int unit);

private:
  GLuint m_texture;
};

#endif
