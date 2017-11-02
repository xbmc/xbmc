/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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

#include <string>
#include <vector>

#include "GUIFontTTF.h"
#include "system.h"
#include "system_gl.h"

class CGUIFontTTFGL : public CGUIFontTTFBase
{
public:
  explicit CGUIFontTTFGL(const std::string& strFileName);
  ~CGUIFontTTFGL(void) override;

  bool FirstBegin() override;
  void LastEnd() override;

  CVertexBuffer CreateVertexBuffer(const std::vector<SVertex> &vertices) const override;
  void DestroyVertexBuffer(CVertexBuffer &bufferHandle) const override;
  static void CreateStaticVertexBuffers(void);
  static void DestroyStaticVertexBuffers(void);

protected:
  CBaseTexture* ReallocTexture(unsigned int& newHeight) override;
  bool CopyCharToTexture(FT_BitmapGlyph bitGlyph, unsigned int x1, unsigned int y1, unsigned int x2, unsigned int y2) override;
  void DeleteHardwareTexture() override;

  static GLuint m_elementArrayHandle;

private:
  unsigned int m_updateY1;
  unsigned int m_updateY2;
  
  enum TextureStatus
  {
    TEXTURE_VOID = 0,
    TEXTURE_READY,
    TEXTURE_REALLOCATED,
    TEXTURE_UPDATED,
  };
  
  TextureStatus m_textureStatus;

  static bool m_staticVertexBufferCreated;
};

