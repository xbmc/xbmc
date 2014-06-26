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

#include "system.h"
#include "GUIFont.h"
#include "GUIFontTTFGL.h"
#include "GUIFontManager.h"
#include "Texture.h"
#include "TextureManager.h"
#include "GraphicContext.h"
#include "gui3d.h"
#include "utils/log.h"
#include "utils/GLUtils.h"
#include "windowing/WindowingFactory.h"

// stuff for freetype
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_OUTLINE_H

using namespace std;

#if defined(HAS_GL) || defined(HAS_GLES)


CGUIFontTTFGL::CGUIFontTTFGL(const std::string& strFileName)
: CGUIFontTTFBase(strFileName)
{
  m_updateY1 = 0;
  m_updateY2 = 0;
  m_textureStatus = TEXTURE_VOID;
}

CGUIFontTTFGL::~CGUIFontTTFGL(void)
{
}

void CGUIFontTTFGL::Begin()
{
  if (m_nestedBeginCount == 0 && m_texture != NULL)
  {
    if (m_textureStatus == TEXTURE_REALLOCATED)
    {
      if (glIsTexture(m_nTexture))
        g_TextureManager.ReleaseHwTexture(m_nTexture);
      m_textureStatus = TEXTURE_VOID;
    }
    
    if (m_textureStatus == TEXTURE_VOID)
    {
      // Have OpenGL generate a texture object handle for us
      glGenTextures(1, (GLuint*) &m_nTexture);

      // Bind the texture object
      glBindTexture(GL_TEXTURE_2D, m_nTexture);
#ifdef HAS_GL
      glEnable(GL_TEXTURE_2D);
#endif
      // Set the texture's stretching properties
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

      // Set the texture image -- THIS WORKS, so the pixels must be wrong.
      glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, m_texture->GetWidth(), m_texture->GetHeight(), 0,
                   GL_ALPHA, GL_UNSIGNED_BYTE, 0);
      
      VerifyGLState();
      m_textureStatus = TEXTURE_UPDATED;
    }

    if (m_textureStatus == TEXTURE_UPDATED)
    {
      glBindTexture(GL_TEXTURE_2D, m_nTexture);
      glTexSubImage2D(GL_TEXTURE_2D, 0, 0, m_updateY1, m_texture->GetWidth(), m_updateY2 - m_updateY1, GL_ALPHA, GL_UNSIGNED_BYTE,
                      m_texture->GetPixels() + m_updateY1 * m_texture->GetPitch());
      glDisable(GL_TEXTURE_2D);
        
      m_updateY1 = m_updateY2 = 0;
      m_textureStatus = TEXTURE_READY;
    }

    // Turn Blending On
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE_MINUS_DST_ALPHA, GL_ONE);
    glEnable(GL_BLEND);
#ifdef HAS_GL
    glEnable(GL_TEXTURE_2D);
#endif
    glBindTexture(GL_TEXTURE_2D, m_nTexture);

#ifdef HAS_GL
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

    if(g_Windowing.UseLimitedColor())
    {
      glActiveTexture(GL_TEXTURE1);
      glBindTexture(GL_TEXTURE_2D, m_nTexture); // dummy bind
      glEnable(GL_TEXTURE_2D);

      const GLfloat rgba[4] = {16.0f / 255.0f, 16.0f / 255.0f, 16.0f / 255.0f, 0.0f};
      glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE , GL_COMBINE);
      glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, rgba);
      glTexEnvi (GL_TEXTURE_ENV, GL_COMBINE_RGB      , GL_ADD);
      glTexEnvi (GL_TEXTURE_ENV, GL_SOURCE0_RGB      , GL_PREVIOUS);
      glTexEnvi (GL_TEXTURE_ENV, GL_SOURCE1_RGB      , GL_CONSTANT);
      glTexEnvi (GL_TEXTURE_ENV, GL_OPERAND0_RGB     , GL_SRC_COLOR);
      glTexEnvi (GL_TEXTURE_ENV, GL_OPERAND1_RGB     , GL_SRC_COLOR);
      glTexEnvi (GL_TEXTURE_ENV, GL_COMBINE_ALPHA    , GL_REPLACE);
      glTexEnvi (GL_TEXTURE_ENV, GL_SOURCE0_ALPHA    , GL_PREVIOUS);
      VerifyGLState();
    }

#else
    g_Windowing.EnableGUIShader(SM_FONTS);
#endif

    m_vertex_count = 0;
  }
  // Keep track of the nested begin/end calls.
  m_nestedBeginCount++;
}

void CGUIFontTTFGL::End()
{
  if (m_nestedBeginCount == 0)
    return;

  if (--m_nestedBeginCount > 0)
    return;

#ifdef HAS_GL
  glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);

  glColorPointer   (4, GL_UNSIGNED_BYTE, sizeof(SVertex), (char*)m_vertex + offsetof(SVertex, r));
  glVertexPointer  (3, GL_FLOAT        , sizeof(SVertex), (char*)m_vertex + offsetof(SVertex, x));
  glTexCoordPointer(2, GL_FLOAT        , sizeof(SVertex), (char*)m_vertex + offsetof(SVertex, u));
  glEnableClientState(GL_COLOR_ARRAY);
  glEnableClientState(GL_VERTEX_ARRAY);
  glEnableClientState(GL_TEXTURE_COORD_ARRAY);
  glDrawArrays(GL_QUADS, 0, m_vertex_count);
  glPopClientAttrib();

  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, 0);
  glDisable(GL_TEXTURE_2D);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, 0);
  glDisable(GL_TEXTURE_2D);
#else
  // GLES 2.0 version. Cannot draw quads. Convert to triangles.
  GLint posLoc  = g_Windowing.GUIShaderGetPos();
  GLint colLoc  = g_Windowing.GUIShaderGetCol();
  GLint tex0Loc = g_Windowing.GUIShaderGetCoord0();

  // stack object until VBOs will be used
  std::vector<SVertex> vecVertices( 6 * (m_vertex_count / 4) );
  SVertex *vertices = &vecVertices[0];

  for (int i=0; i<m_vertex_count; i+=4)
  {
    *vertices++ = m_vertex[i];
    *vertices++ = m_vertex[i+1];
    *vertices++ = m_vertex[i+2];

    *vertices++ = m_vertex[i+1];
    *vertices++ = m_vertex[i+3];
    *vertices++ = m_vertex[i+2];
  }

  vertices = &vecVertices[0];

  glVertexAttribPointer(posLoc,  3, GL_FLOAT,         GL_FALSE, sizeof(SVertex), (char*)vertices + offsetof(SVertex, x));
  // Normalize color values. Does not affect Performance at all.
  glVertexAttribPointer(colLoc,  4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(SVertex), (char*)vertices + offsetof(SVertex, r));
  glVertexAttribPointer(tex0Loc, 2, GL_FLOAT,         GL_FALSE, sizeof(SVertex), (char*)vertices + offsetof(SVertex, u));

  glEnableVertexAttribArray(posLoc);
  glEnableVertexAttribArray(colLoc);
  glEnableVertexAttribArray(tex0Loc);

  glDrawArrays(GL_TRIANGLES, 0, vecVertices.size());

  glDisableVertexAttribArray(posLoc);
  glDisableVertexAttribArray(colLoc);
  glDisableVertexAttribArray(tex0Loc);

  g_Windowing.DisableGUIShader();
#endif
}

CBaseTexture* CGUIFontTTFGL::ReallocTexture(unsigned int& newHeight)
{
  newHeight = CBaseTexture::PadPow2(newHeight);

  CBaseTexture* newTexture = new CTexture(m_textureWidth, newHeight, XB_FMT_A8);

  if (!newTexture || newTexture->GetPixels() == NULL)
  {
    CLog::Log(LOGERROR, "GUIFontTTFGL::CacheCharacter: Error creating new cache texture for size %f", m_height);
    delete newTexture;
    return NULL;
  }
  m_textureHeight = newTexture->GetHeight();
  m_textureScaleY = 1.0f / m_textureHeight;
  m_textureWidth = newTexture->GetWidth();
  m_textureScaleX = 1.0f / m_textureWidth;
  if (m_textureHeight < newHeight)
    CLog::Log(LOGWARNING, "%s: allocated new texture with height of %d, requested %d", __FUNCTION__, m_textureHeight, newHeight);

  memset(newTexture->GetPixels(), 0, m_textureHeight * newTexture->GetPitch());
  if (m_texture)
  {
    m_updateY1 = 0;
    m_updateY2 = m_texture->GetHeight();

    unsigned char* src = (unsigned char*) m_texture->GetPixels();
    unsigned char* dst = (unsigned char*) newTexture->GetPixels();
    for (unsigned int y = 0; y < m_texture->GetHeight(); y++)
    {
      memcpy(dst, src, m_texture->GetPitch());
      src += m_texture->GetPitch();
      dst += newTexture->GetPitch();
    }
    delete m_texture;
  }

  m_textureStatus = TEXTURE_REALLOCATED;

  return newTexture;
}

bool CGUIFontTTFGL::CopyCharToTexture(FT_BitmapGlyph bitGlyph, unsigned int x1, unsigned int y1, unsigned int x2, unsigned int y2)
{
  FT_Bitmap bitmap = bitGlyph->bitmap;

  unsigned char* source = (unsigned char*) bitmap.buffer;
  unsigned char* target = (unsigned char*) m_texture->GetPixels() + y1 * m_texture->GetPitch() + x1;

  for (unsigned int y = y1; y < y2; y++)
  {
    memcpy(target, source, x2-x1);
    source += bitmap.width;
    target += m_texture->GetPitch();
  }
  
  switch (m_textureStatus)
  {
  case TEXTURE_UPDATED:
    {
      m_updateY1 = std::min(m_updateY1, y1);
      m_updateY2 = std::max(m_updateY2, y2);
    }
    break;
      
  case TEXTURE_READY:
    {
      m_updateY1 = y1;
      m_updateY2 = y2;
      m_textureStatus = TEXTURE_UPDATED;
    }
    break;
      
  case TEXTURE_REALLOCATED:
    {
      m_updateY2 = std::max(m_updateY2, y2);
    }
    break;

  case TEXTURE_VOID:
  default:
    break;
  }
  
  return TRUE;
}


void CGUIFontTTFGL::DeleteHardwareTexture()
{
  if (m_textureStatus != TEXTURE_VOID)
  {
    if (glIsTexture(m_nTexture))
      g_TextureManager.ReleaseHwTexture(m_nTexture);

    m_textureStatus = TEXTURE_VOID;
    m_updateY1 = m_updateY2 = 0;
  }
}

#endif
