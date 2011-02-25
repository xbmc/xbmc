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
#if defined(HAS_GLES)
#include "GUITextureGLES.h"
#endif
#include "Texture.h"
#include "utils/log.h"
#include "utils/GLUtils.h"
#include "windowing/WindowingFactory.h"

#if defined(HAS_GLES)

CGUITextureGLES::CGUITextureGLES(float posX, float posY, float width, float height, const CTextureInfo &texture)
: CGUITextureBase(posX, posY, width, height, texture)
{
}

void CGUITextureGLES::Begin(color_t color)
{
  CBaseTexture* texture = m_texture.m_textures[m_currentFrame];
  glActiveTexture(GL_TEXTURE0);
  texture->LoadToGPU();
  if (m_diffuse.size())
    m_diffuse.m_textures[0]->LoadToGPU();

  glBindTexture(GL_TEXTURE_2D, texture->GetTextureObject());
  glEnable(GL_TEXTURE_2D);

  // Setup Colors
  for (int i = 0; i < 4; i++)
  {
    m_col[i][0] = (GLubyte)GET_R(color);
    m_col[i][1] = (GLubyte)GET_G(color);
    m_col[i][2] = (GLubyte)GET_B(color);
    m_col[i][3] = (GLubyte)GET_A(color);
  }

  GLint posLoc  = g_Windowing.GUIShaderGetPos();
  GLint colLoc  = g_Windowing.GUIShaderGetCol();
  GLint tex0Loc = g_Windowing.GUIShaderGetCoord0();

  glVertexAttribPointer(posLoc, 3, GL_FLOAT, 0, 0, m_vert);
  glVertexAttribPointer(colLoc, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, m_col);
  glVertexAttribPointer(tex0Loc, 2, GL_FLOAT, 0, 0, m_tex0);

  glEnableVertexAttribArray(posLoc);
  glEnableVertexAttribArray(colLoc);
  glEnableVertexAttribArray(tex0Loc);

  bool hasAlpha = m_texture.m_textures[m_currentFrame]->HasAlpha() || m_col[0][3] < 255;

  if (m_diffuse.size())
  {
    if (m_col[0][0] == 255 && m_col[0][1] == 255 && m_col[0][2] == 255 && m_col[0][3] == 255 )
    {
      g_Windowing.EnableGUIShader(SM_MULTI);
    }
    else
    {
      g_Windowing.EnableGUIShader(SM_MULTI_BLENDCOLOR);
    }

    hasAlpha |= m_diffuse.m_textures[0]->HasAlpha();

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_diffuse.m_textures[0]->GetTextureObject());
    glEnable(GL_TEXTURE_2D);

    GLint tex1Loc = g_Windowing.GUIShaderGetCoord1();
    glVertexAttribPointer(tex1Loc, 2, GL_FLOAT, 0, 0, m_tex1);
    glEnableVertexAttribArray(tex1Loc);

    hasAlpha = true;
  }
  else
  {
    if ( hasAlpha )
    {
      g_Windowing.EnableGUIShader(SM_TEXTURE);
    }
    else
    {
      g_Windowing.EnableGUIShader(SM_TEXTURE_NOBLEND);
    }
  }


  if ( hasAlpha )
  {
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glEnable( GL_BLEND );
  }
  else
  {
    glDisable(GL_BLEND);
  }

}

void CGUITextureGLES::End()
{
  if (m_diffuse.size())
  {
    glDisable(GL_TEXTURE_2D);
    glDisableVertexAttribArray(g_Windowing.GUIShaderGetCoord1());
    glActiveTexture(GL_TEXTURE0);
  }

  glDisable(GL_TEXTURE_2D);
  glDisableVertexAttribArray(g_Windowing.GUIShaderGetPos());
  glDisableVertexAttribArray(g_Windowing.GUIShaderGetCol());
  glDisableVertexAttribArray(g_Windowing.GUIShaderGetCoord0());

  glEnable(GL_BLEND);
  g_Windowing.DisableGUIShader();
}

void CGUITextureGLES::Draw(float *x, float *y, float *z, const CRect &texture, const CRect &diffuse, int orientation)
{
  GLubyte idx[4] = {0, 1, 3, 2};        //determines order of triangle strip

  // Setup vertex position values
  for (int i=0; i<4; i++)
  {
    m_vert[i][0] = x[i];
    m_vert[i][1] = y[i];
    m_vert[i][2] = z[i];
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

  g_Windowing.EnableGUIShader(SM_TEXTURE);

  GLint posLoc   = g_Windowing.GUIShaderGetPos();
  GLint colLoc   = g_Windowing.GUIShaderGetCol();
  GLint tex0Loc  = g_Windowing.GUIShaderGetCoord0();

  glVertexAttribPointer(posLoc,  3, GL_FLOAT, 0, 0, ver);
  glVertexAttribPointer(colLoc,  4, GL_UNSIGNED_BYTE, GL_TRUE, 0, col);
  glVertexAttribPointer(tex0Loc, 2, GL_FLOAT, 0, 0, tex);

  glEnableVertexAttribArray(posLoc);
  glEnableVertexAttribArray(tex0Loc);
  glEnableVertexAttribArray(colLoc);

  for (int i=0; i<4; i++)
  {
    // Setup Colour Values
    col[i][0] = (GLubyte)GET_R(color);
    col[i][1] = (GLubyte)GET_G(color);
    col[i][2] = (GLubyte)GET_B(color);
    col[i][3] = (GLubyte)GET_A(color);
  }

  // Setup vertex position values
  // ver[0][3] = ver[1][3] = ver[2][3] = ver[3][3] = 0.0f; // FIXME, ver has only 3 elements - this is not correct
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

  g_Windowing.DisableGUIShader();

  if (texture)
    glDisable(GL_TEXTURE_2D);
}

#endif
