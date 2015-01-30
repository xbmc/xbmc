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

/*!
\file GUIFont.h
\brief
*/

#ifndef CGUILIB_GUIFONTTTF_GL_H
#define CGUILIB_GUIFONTTTF_GL_H
#pragma once


#include "GUIFontTTF.h"
#include "system.h"
#include "system_gl.h"


/*!
 \ingroup textures
 \brief
 */
class CGUIFontTTFGL : public CGUIFontTTFBase
{
public:
  CGUIFontTTFGL(const std::string& strFileName);
  virtual ~CGUIFontTTFGL(void);

  virtual bool FirstBegin();
  virtual void LastEnd();
#if HAS_GLES
  virtual CVertexBuffer CreateVertexBuffer(const std::vector<SVertex> &vertices) const;
  virtual void DestroyVertexBuffer(CVertexBuffer &bufferHandle) const;
  static void CreateStaticVertexBuffers(void);
  static void DestroyStaticVertexBuffers(void);
#endif

protected:
  virtual CBaseTexture* ReallocTexture(unsigned int& newHeight);
  virtual bool CopyCharToTexture(FT_BitmapGlyph bitGlyph, unsigned int x1, unsigned int y1, unsigned int x2, unsigned int y2);
  virtual void DeleteHardwareTexture();

#if HAS_GLES
#define ELEMENT_ARRAY_MAX_CHAR_INDEX (1000)

  static GLuint m_elementArrayHandle;
#endif

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

#if HAS_GLES
  static bool m_staticVertexBufferCreated;
#endif
};

#endif
