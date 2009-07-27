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

#include "include.h"
#include "GUIFont.h"
#include "GUIFontTTFGL.h"
#include "GUIFontManager.h"
#include "GraphicContext.h"
#include "FileSystem/SpecialProtocol.h"
#include "Util.h"
#include "gui3d.h"
#include <math.h>

// stuff for freetype
#ifndef _LINUX
#include "ft2build.h"
#else
#include <ft2build.h>
#endif
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_OUTLINE_H

using namespace std;

CGUIFontTTFGL::CGUIFontTTFGL(const CStdString& strFileName)
: CGUIFontTTFBase(strFileName)
{
 
}

CGUIFontTTFGL::~CGUIFontTTFGL(void)
{
  
}

void CGUIFontTTFGL::Begin()
{
  if (m_dwNestedBeginCount == 0)
  {
    if (!m_bTextureLoaded)
    {
      // Have OpenGL generate a texture object handle for us
      glGenTextures(1, &m_nTexture);

      // Bind the texture object
      glBindTexture(GL_TEXTURE_2D, m_nTexture);
      glEnable(GL_TEXTURE_2D);

      // Set the texture's stretching properties
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

      // Set the texture image -- THIS WORKS, so the pixels must be wrong.
      glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, m_texture->w, m_texture->h, 0,
                   GL_ALPHA, GL_UNSIGNED_BYTE, m_texture->pixels);

      VerifyGLState();
      m_bTextureLoaded = true;
    }

    // Turn Blending On
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, m_nTexture);
    glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_COMBINE);
    glTexEnvi(GL_TEXTURE_ENV,GL_COMBINE_RGB,GL_REPLACE);
    glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_PRIMARY_COLOR);
    glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);
    glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_MODULATE);
    glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA, GL_TEXTURE0);
    glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA);
    glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_ALPHA, GL_PRIMARY_COLOR);
    glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_ALPHA, GL_SRC_ALPHA);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    VerifyGLState();

    m_vertex_count = 0;
  }
  // Keep track of the nested begin/end calls.
  m_dwNestedBeginCount++;
}

void CGUIFontTTFGL::End()
{
  if (m_dwNestedBeginCount == 0)
    return;

  if (--m_dwNestedBeginCount > 0)
    return;

  glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);

  glColorPointer   (4, GL_UNSIGNED_BYTE, sizeof(SVertex), (char*)m_vertex + offsetof(SVertex, r));
  glVertexPointer  (3, GL_FLOAT        , sizeof(SVertex), (char*)m_vertex + offsetof(SVertex, x));
  glTexCoordPointer(2, GL_FLOAT        , sizeof(SVertex), (char*)m_vertex + offsetof(SVertex, u));
  glEnableClientState(GL_COLOR_ARRAY);
  glEnableClientState(GL_VERTEX_ARRAY);
  glEnableClientState(GL_TEXTURE_COORD_ARRAY);
  glDrawArrays(GL_QUADS, 0, m_vertex_count);
  glPopClientAttrib();
}

void CGUIFontTTFGL::ReleaseCharactersTexture()
{
  if (m_bTextureLoaded)
  {
    if (glIsTexture(m_nTexture))
      glDeleteTextures(1, &m_nTexture);
    m_bTextureLoaded = false;
  }
}
