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

#include "system.h"
#include "GUITextureGLES.h"
#include "Texture.h"
#include "WindowingFactory.h"
#include "utils/log.h"

#if defined(HAS_GLES)

CGUITextureGLES::CGUITextureGLES(float posX, float posY, float width, float height, const CTextureInfo &texture)
: CGUITextureBase(posX, posY, width, height, texture)
{
}

void CGUITextureGLES::Begin()
{
  CBaseTexture* texture = m_texture.m_textures[m_currentFrame];
  glActiveTexture(GL_TEXTURE0);
  texture->LoadToGPU();
  if (m_diffuse.size())
    m_diffuse.m_textures[0]->LoadToGPU();

  glBindTexture(GL_TEXTURE_2D, texture->GetTextureObject());
  glEnable(GL_TEXTURE_2D);

  glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_BLEND);          // Turn Blending On

  if (m_diffuse.size())
  {
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_diffuse.m_textures[0]->GetTextureObject());
    glEnable(GL_TEXTURE_2D);

    //TODO: Enable multitexturing shader
  }
  else
  {
    //TODO: Enable normal texturing shader
  }

  GLint posLoc; //TODO: Get vertex location from shader
  GLint colLoc; //TODO: Get colour location from shader
  GLint tex0Loc; //TODO: Get texture coordinate location from shader

  glVertexAttribPointer(posLoc, 3, GL_FLOAT, 0, 0, m_vert);
  glVertexAttribPointer(colLoc, 4, GL_UNSIGNED_BYTE, 0, 0, m_col);
  glVertexAttribPointer(tex0Loc, 2, GL_FLOAT, 0, 0, m_tex0);

  glEnableVertexAttribArray(posLoc);
  glEnableVertexAttribArray(colLoc);
  glEnableVertexAttribArray(tex0Loc);

  if (m_diffuse.size())
  {
    GLint tex1Loc; //TODO: Get second texture coordinate location from shader
    glVertexAttribPointer(tex1Loc, 2, GL_FLOAT, 0, 0, m_tex1);
    glEnableVertexAttribArray(tex1Loc);
  }
}

void CGUITextureGLES::End()
{
  if (m_diffuse.size())
  {
    glDisable(GL_TEXTURE_2D);
    glDisableVertexAttribArray(NULL/*TODO: Get location from shader*/);
    glActiveTexture(GL_TEXTURE0);
  }

  glDisable(GL_TEXTURE_2D);
  glDisableVertexAttribArray(NULL/*TODO: Get location from shader*/);
  glDisableVertexAttribArray(NULL/*TODO: Get location from shader*/);
  glDisableVertexAttribArray(NULL/*TODO: Get location from shader*/);

  //TODO: Disable shader
}

void CGUITextureGLES::Draw(float *x, float *y, float *z, const CRect &texture, const CRect &diffuse, DWORD color, int orientation)
{
  GLubyte idx[4] = {0, 1, 3, 2};        //determines order of triangle strip

  for (int i=0; i<4; i)
  {
    // Setup vertex position values
    m_vert[i][0] = x[i];
    m_vert[i][1] = y[i];
    m_vert[i][2] = z[i];

    // Setup Colours
    m_col[i][0] = (GLubyte)GET_R(color);
    m_col[i][1] = (GLubyte)GET_G(color);
    m_col[i][2] = (GLubyte)GET_B(color);
    m_col[i][3] = (GLubyte)GET_A(color);
  }

  // Setup texture coordinates
  //TopLeft
  m_tex0[0][0] = texture.x1;
  m_tex0[0][1] = texture.y1;
  //TopRight
  if (orientation & 4)
  {
    m_tex0[1][0] = texture.x1;
    m_tex0[1][1] = texture.y2;
  }
  else
  {
    m_tex0[1][0] = texture.x2;
    m_tex0[1][1] = texture.y1;
  }
  //BottomRight
  m_tex0[2][0] = texture.x2;
  m_tex0[2][1] = texture.y2;
  //BottomLeft
  if (orientation & 4)
  {
    m_tex0[3][0] = texture.x2;
    m_tex0[3][1] = texture.y1;
  }
  else
  {
    m_tex0[3][0] = texture.x1;
    m_tex0[3][1] = texture.y2;
  }

  if (m_diffuse.size())
  {
    //TopLeft
    m_tex1[0][0] = diffuse.x1;
    m_tex1[0][1] = diffuse.y1;
    //TopRight
    if (m_info.orientation & 4)
    {
      m_tex1[1][0] = diffuse.x1;
      m_tex1[1][1] = diffuse.y2;
    }
    else
    {
      m_tex1[1][0] = diffuse.x2;
      m_tex1[1][1] = diffuse.y1;
    }
    //BottomRight
    m_tex1[2][0] = diffuse.x2;
    m_tex1[2][1] = diffuse.y2;
    //BottomLeft
    if (m_info.orientation & 4)
    {
      m_tex1[3][0] = diffuse.x2;
      m_tex1[3][1] = diffuse.y1;
    }
    else
    {
      m_tex1[3][0] = diffuse.x1;
      m_tex1[3][1] = diffuse.y2;
    }
  }

  glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_BYTE, idx);
}

void CGUITextureGLES::DrawQuad(const CRect &rect, color_t color, CBaseTexture *texture, const CRect *texCoords)
{
  if (texture)
  {
    glActiveTexture(GL_TEXTURE0);
    texture->LoadToGPU();
    glBindTexture(GL_TEXTURE_2D, texture->GetTextureObject());
    glEnable(GL_TEXTURE_2D);
  }
  else
    glDisable(GL_TEXTURE_2D);

  glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_BLEND);          // Turn Blending On

  VerifyGLState();

  GLfloat col[4][4];
  GLfloat ver[4][3];
  GLfloat tex[4][2];
  GLubyte idx[4] = {0, 1, 3, 2};        //determines order of triangle strip

  //TODO: Enable texturing shader

  GLint posLoc; //TODO: Get location from shader
  GLint colLoc; //TODO: Get location from shader
  GLint tex0Loc; //TODO: Get location from shader

  glVertexAttribPointer(posLoc,  3, GL_FLOAT, 0, 0, ver);
  glVertexAttribPointer(colLoc,  4, GL_UNSIGNED_BYTE, 0, 0, col);
  glVertexAttribPointer(tex0Loc, 2, GL_FLOAT, 0, 0, tex);

  glEnableVertexAttribArray(posLoc);
  glEnableVertexAttribArray(tex0Loc);
  glEnableVertexAttribArray(colLoc);

  for (int i=0; i<4; i)
  {
    // Setup Colour Values
    col[i][0] = (GLubyte)GET_R(color);
    col[i][1] = (GLubyte)GET_G(color);
    col[i][2] = (GLubyte)GET_B(color);
    col[i][3] = (GLubyte)GET_A(color);
  }

  // Setup vertex position values
  ver[0][3] = ver[1][3] = ver[2][3] = ver[3][3] = 0.0f;
  ver[0][0] = ver[3][0] = rect.x1;
  ver[0][1] = ver[1][1] = rect.y1;
  ver[1][0] = ver[2][0] = rect.x2;
  ver[2][1] = ver[3][1] = rect.y2;

  // Setup texture coordinates
  CRect coords = texCoords ? *texCoords : CRect(0.0f, 0.0f, 1.0f, 1.0f);
  tex[0][0] = tex[3][0] = coords.x1;
  tex[0][1] = tex[1][1] = coords.y1;
  tex[1][0] = tex[2][0] = coords.x2;
  tex[2][1] = tex[3][1] = coords.y2;

  glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_BYTE, idx);

  glDisableVertexAttribArray(posLoc);
  glDisableVertexAttribArray(colLoc);
  glDisableVertexAttribArray(tex0Loc);

  //TODO: Disable texturing shader

  if (texture)
    glDisable(GL_TEXTURE_2D);
}

#endif
