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
#include "GUITextureGL.h"
#include "GraphicContext.h"
#ifdef HAS_SDL_OPENGL
#include <GL/glew.h>

CGUITexture::CGUITexture(float posX, float posY, float width, float height, const CTextureInfo &texture)
: CGUITextureBase(posX, posY, width, height, texture)
{
}

void CGUITexture::Begin()
{
  CGLTexture* texture = m_textures[m_currentFrame].m_texture;
  glActiveTextureARB(GL_TEXTURE0_ARB);
  texture->LoadToGPU();
  if (m_diffuse.m_texture)
    m_diffuse.m_texture->LoadToGPU();

  glBindTexture(GL_TEXTURE_2D, texture->id);
  glEnable(GL_TEXTURE_2D);
  
  glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_BLEND);          // Turn Blending On
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
     
  // diffuse coloring
  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
  glTexEnvf(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);
  glTexEnvf(GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_TEXTURE0);
  glTexEnvf(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);
  glTexEnvf(GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_PRIMARY_COLOR);
  glTexEnvf(GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR);
  VerifyGLState();

  if (m_diffuse.m_texture)
  {
    glActiveTextureARB(GL_TEXTURE1_ARB);
    glBindTexture(GL_TEXTURE_2D, m_diffuse.m_texture->id);
    glEnable(GL_TEXTURE_2D);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
    glTexEnvf(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);
    glTexEnvf(GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_TEXTURE1);
    glTexEnvf(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);
    glTexEnvf(GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_PREVIOUS);
    glTexEnvf(GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR);
    VerifyGLState();
  }
  //glDisable(GL_TEXTURE_2D); // uncomment these 2 lines to switch to wireframe rendering
  //glBegin(GL_LINE_LOOP);
  glBegin(GL_QUADS);
}

void CGUITexture::End()
{
  glEnd();
  if (m_diffuse.m_texture)
  {
    glDisable(GL_TEXTURE_2D);
    glActiveTextureARB(GL_TEXTURE0_ARB);
  }
  glDisable(GL_TEXTURE_2D);
}

void CGUITexture::Draw(float *x, float *y, float *z, const CRect &texture, const CRect &diffuse, DWORD color, int orientation)
{
  GLubyte a = (GLubyte)(color >> 24);
  GLubyte r = (GLubyte)((color >> 16) & 0xff);
  GLubyte g = (GLubyte)((color >> 8) & 0xff);
  GLubyte b = (GLubyte)(color & 0xff);

  // Top-left vertex (corner)
  glColor4ub(r, g, b, a);
  glMultiTexCoord2fARB(GL_TEXTURE0_ARB, texture.x1, texture.y1);
  if (m_diffuse.m_texture)
    glMultiTexCoord2fARB(GL_TEXTURE1_ARB, diffuse.x1, diffuse.y1);
  glVertex3f(x[0], y[0], z[0]);
  
  // Top-right vertex (corner)
  glColor4ub(r, g, b, a);
  if (orientation & 4)
    glMultiTexCoord2fARB(GL_TEXTURE0_ARB, texture.x1, texture.y2);
  else
    glMultiTexCoord2fARB(GL_TEXTURE0_ARB, texture.x2, texture.y1);
  if (m_diffuse.m_texture)
  {
    if (m_info.orientation & 4)
      glMultiTexCoord2fARB(GL_TEXTURE1_ARB, diffuse.x1, diffuse.y2);
    else
      glMultiTexCoord2fARB(GL_TEXTURE1_ARB, diffuse.x2, diffuse.y1);
  }
  glVertex3f(x[1], y[1], z[1]);

  // Bottom-right vertex (corner)
  glColor4ub(r, g, b, a);
  glMultiTexCoord2fARB(GL_TEXTURE0_ARB, texture.x2, texture.y2);
  if (m_diffuse.m_texture)
    glMultiTexCoord2fARB(GL_TEXTURE1_ARB, diffuse.x2, diffuse.y2);
  glVertex3f(x[2], y[2], z[2]);
  
  // Bottom-left vertex (corner)
  glColor4ub(r, g, b, a);
  if (orientation & 4)
    glMultiTexCoord2fARB(GL_TEXTURE0_ARB, texture.x2, texture.y1);
  else
    glMultiTexCoord2fARB(GL_TEXTURE0_ARB, texture.x1, texture.y2);
  if (m_diffuse.m_texture)
  {
    if (m_info.orientation & 4)
      glMultiTexCoord2fARB(GL_TEXTURE1_ARB, diffuse.x2, diffuse.y1);
    else
      glMultiTexCoord2fARB(GL_TEXTURE1_ARB, diffuse.x1, diffuse.y2);
  }
  glVertex3f(x[3], y[3], z[3]);
}

#endif
