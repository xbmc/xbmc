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

#include "system.h"
#if defined(HAS_GL)
#include "GUITextureGL.h"
#endif
#include "Texture.h"
#include "utils/log.h"
#include "utils/GLUtils.h"

#if defined(HAS_GL)

CGUITextureGL::CGUITextureGL(float posX, float posY, float width, float height, const CTextureInfo &texture)
: CGUITextureBase(posX, posY, width, height, texture)
{
  memset(m_col, 0, sizeof(m_col));
}

void CGUITextureGL::Begin(color_t color)
{
  m_col[0] = (GLubyte)GET_R(color);
  m_col[1] = (GLubyte)GET_G(color);
  m_col[2] = (GLubyte)GET_B(color);
  m_col[3] = (GLubyte)GET_A(color);

  CBaseTexture* texture = m_texture.m_textures[m_currentFrame];
  texture->LoadToGPU();
  if (m_diffuse.size())
    m_diffuse.m_textures[0]->LoadToGPU();

  texture->BindToUnit(0);

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

  if (m_diffuse.size())
  {
    m_diffuse.m_textures[0]->BindToUnit(1);
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

void CGUITextureGL::End()
{
  glEnd();
  if (m_diffuse.size())
  {
    glDisable(GL_TEXTURE_2D);
    glActiveTextureARB(GL_TEXTURE0_ARB);
  }
  glDisable(GL_TEXTURE_2D);
}

void CGUITextureGL::Draw(float *x, float *y, float *z, const CRect &texture, const CRect &diffuse, int orientation)
{
  // Top-left vertex (corner)
  glColor4ub(m_col[0], m_col[1], m_col[2], m_col[3]);
  glMultiTexCoord2fARB(GL_TEXTURE0_ARB, texture.x1, texture.y1);
  if (m_diffuse.size())
    glMultiTexCoord2fARB(GL_TEXTURE1_ARB, diffuse.x1, diffuse.y1);
  glVertex3f(x[0], y[0], z[0]);

  // Top-right vertex (corner)
  glColor4ub(m_col[0], m_col[1], m_col[2], m_col[3]);
  if (orientation & 4)
    glMultiTexCoord2fARB(GL_TEXTURE0_ARB, texture.x1, texture.y2);
  else
    glMultiTexCoord2fARB(GL_TEXTURE0_ARB, texture.x2, texture.y1);
  if (m_diffuse.size())
  {
    if (m_info.orientation & 4)
      glMultiTexCoord2fARB(GL_TEXTURE1_ARB, diffuse.x1, diffuse.y2);
    else
      glMultiTexCoord2fARB(GL_TEXTURE1_ARB, diffuse.x2, diffuse.y1);
  }
  glVertex3f(x[1], y[1], z[1]);

  // Bottom-right vertex (corner)
  glColor4ub(m_col[0], m_col[1], m_col[2], m_col[3]);
  glMultiTexCoord2fARB(GL_TEXTURE0_ARB, texture.x2, texture.y2);
  if (m_diffuse.size())
    glMultiTexCoord2fARB(GL_TEXTURE1_ARB, diffuse.x2, diffuse.y2);
  glVertex3f(x[2], y[2], z[2]);

  // Bottom-left vertex (corner)
  glColor4ub(m_col[0], m_col[1], m_col[2], m_col[3]);
  if (orientation & 4)
    glMultiTexCoord2fARB(GL_TEXTURE0_ARB, texture.x2, texture.y1);
  else
    glMultiTexCoord2fARB(GL_TEXTURE0_ARB, texture.x1, texture.y2);
  if (m_diffuse.size())
  {
    if (m_info.orientation & 4)
      glMultiTexCoord2fARB(GL_TEXTURE1_ARB, diffuse.x2, diffuse.y1);
    else
      glMultiTexCoord2fARB(GL_TEXTURE1_ARB, diffuse.x1, diffuse.y2);
  }
  glVertex3f(x[3], y[3], z[3]);
}

void CGUITextureGL::DrawQuad(const CRect &rect, color_t color, CBaseTexture *texture, const CRect *texCoords)
{
  if (texture)
  {
    texture->LoadToGPU();
    texture->BindToUnit(0);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
    glTexEnvf(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);
    glTexEnvf(GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_TEXTURE1);
    glTexEnvf(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);
    glTexEnvf(GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_PREVIOUS);
    glTexEnvf(GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR);
  }
  else
  glDisable(GL_TEXTURE_2D);

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

  glBegin(GL_QUADS);

  glColor4ub((GLubyte)GET_R(color), (GLubyte)GET_G(color), (GLubyte)GET_B(color), (GLubyte)GET_A(color));

  CRect coords = texCoords ? *texCoords : CRect(0.0f, 0.0f, 1.0f, 1.0f);
  glTexCoord2f(coords.x1, coords.y1);
  glVertex3f(rect.x1, rect.y1, 0);
  glTexCoord2f(coords.x2, coords.y1);
  glVertex3f(rect.x2, rect.y1, 0);
  glTexCoord2f(coords.x2, coords.y2);
  glVertex3f(rect.x2, rect.y2, 0);
  glTexCoord2f(coords.x1, coords.y2);
  glVertex3f(rect.x1, rect.y2, 0);

  glEnd();
  if (texture)
    glDisable(GL_TEXTURE_2D);
}

#endif
