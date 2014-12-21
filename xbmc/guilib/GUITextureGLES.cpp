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
#if defined(HAS_GLES)
#include "GUITextureGLES.h"
#endif
#include "Texture.h"
#include "utils/log.h"
#include "utils/GLUtils.h"
#include "utils/MathUtils.h"
#include "windowing/WindowingFactory.h"
#include "guilib/GraphicContext.h"

#include <cstddef>

#if defined(HAS_GLES)


CGUITextureGLES::CGUITextureGLES(float posX, float posY, float width, float height, const CTextureInfo &texture)
: CGUITextureBase(posX, posY, width, height, texture)
{
}

void CGUITextureGLES::Begin(color_t color)
{
  CBaseTexture* texture = m_texture.m_textures[m_currentFrame];
  texture->LoadToGPU();
  if (m_diffuse.size())
    m_diffuse.m_textures[0]->LoadToGPU();

  texture->BindToUnit(0);

  // Setup Colors
  m_col[0] = (GLubyte)GET_R(color);
  m_col[1] = (GLubyte)GET_G(color);
  m_col[2] = (GLubyte)GET_B(color);
  m_col[3] = (GLubyte)GET_A(color);

  bool hasAlpha = m_texture.m_textures[m_currentFrame]->HasAlpha() || m_col[3] < 255;

  if (m_diffuse.size())
  {
    if (m_col[0] == 255 && m_col[1] == 255 && m_col[2] == 255 && m_col[3] == 255 )
    {
      g_Windowing.EnableGUIShader(SM_MULTI);
    }
    else
    {
      g_Windowing.EnableGUIShader(SM_MULTI_BLENDCOLOR);
    }

    hasAlpha |= m_diffuse.m_textures[0]->HasAlpha();

    m_diffuse.m_textures[0]->BindToUnit(1);

  }
  else
  {
    if (m_col[0] == 255 && m_col[1] == 255 && m_col[2] == 255 && m_col[3] == 255)
    {
      g_Windowing.EnableGUIShader(SM_TEXTURE_NOBLEND);
    }
    else
    {
      g_Windowing.EnableGUIShader(SM_TEXTURE);
    }
  }

  if ( hasAlpha )
  {
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE_MINUS_DST_ALPHA, GL_ONE);
    glEnable( GL_BLEND );
  }
  else
  {
    glDisable(GL_BLEND);
  }
  m_packedVertices.clear();
}

void CGUITextureGLES::End()
{
  GLint posLoc  = g_Windowing.GUIShaderGetPos();
  GLint tex0Loc = g_Windowing.GUIShaderGetCoord0();
  GLint tex1Loc = g_Windowing.GUIShaderGetCoord1();
  GLint uniColLoc = g_Windowing.GUIShaderGetUniCol();

  if(uniColLoc >= 0)
  {
    glUniform4f(uniColLoc,(m_col[0] / 255.0f), (m_col[1] / 255.0f), (m_col[2] / 255.0f), (m_col[3] / 255.0f));
  }

  if(m_diffuse.size())
  {
    glVertexAttribPointer(tex1Loc, 2, GL_FLOAT, 0, sizeof(PackedVertex), (char*)&m_packedVertices[0] + offsetof(PackedVertex, u2));
    glEnableVertexAttribArray(tex1Loc);
  }
  glVertexAttribPointer(posLoc, 3, GL_FLOAT, 0, sizeof(PackedVertex), (char*)&m_packedVertices[0] + offsetof(PackedVertex, x));
  glEnableVertexAttribArray(posLoc);
  glVertexAttribPointer(tex0Loc, 2, GL_FLOAT, 0, sizeof(PackedVertex), (char*)&m_packedVertices[0] + offsetof(PackedVertex, u1));
  glEnableVertexAttribArray(tex0Loc);

  glDrawElements(GL_TRIANGLES, m_packedVertices.size()*6 / 4, GL_UNSIGNED_SHORT, m_idx.data());

  if (m_diffuse.size())
  {
    glDisableVertexAttribArray(tex1Loc);
    glActiveTexture(GL_TEXTURE0);
  }

  glDisableVertexAttribArray(posLoc);
  glDisableVertexAttribArray(tex0Loc);

  glEnable(GL_BLEND);
  g_Windowing.DisableGUIShader();
}

void CGUITextureGLES::Draw(float *x, float *y, float *z, const CRect &texture, const CRect &diffuse, int orientation)
{
  PackedVertex vertices[4];

  // Setup texture coordinates
  //TopLeft
  vertices[0].u1 = texture.x1;
  vertices[0].v1 = texture.y1;
  //TopRight
  if (orientation & 4)
  {
    vertices[1].u1 = texture.x1;
    vertices[1].v1 = texture.y2;
  }
  else
  {
    vertices[1].u1 = texture.x2;
    vertices[1].v1 = texture.y1;
  }
  //BottomRight
  vertices[2].u1 = texture.x2;
  vertices[2].v1 = texture.y2;
  //BottomLeft
  if (orientation & 4)
  {
    vertices[3].u1 = texture.x2;
    vertices[3].v1 = texture.y1;
  }
  else
  {
    vertices[3].u1 = texture.x1;
    vertices[3].v1 = texture.y2;
  }

  if (m_diffuse.size())
  {
    //TopLeft
    vertices[0].u2 = diffuse.x1;
    vertices[0].v2 = diffuse.y1;
    //TopRight
    if (m_info.orientation & 4)
    {
      vertices[1].u2 = diffuse.x1;
      vertices[1].v2 = diffuse.y2;
    }
    else
    {
      vertices[1].u2 = diffuse.x2;
      vertices[1].v2 = diffuse.y1;
    }
    //BottomRight
    vertices[2].u2 = diffuse.x2;
    vertices[2].v2 = diffuse.y2;
    //BottomLeft
    if (m_info.orientation & 4)
    {
      vertices[3].u2 = diffuse.x2;
      vertices[3].v2 = diffuse.y1;
    }
    else
    {
      vertices[3].u2 = diffuse.x1;
      vertices[3].v2 = diffuse.y2;
    }
  }

  for (int i=0; i<4; i++)
  {
    vertices[i].x = x[i];
    vertices[i].y = y[i];
    vertices[i].z = z[i];
    m_packedVertices.push_back(vertices[i]);
  }

  if ((m_packedVertices.size() / 4) > (m_idx.size() / 6))
  {
    size_t i = m_packedVertices.size() - 4;
    m_idx.push_back(i+0);
    m_idx.push_back(i+1);
    m_idx.push_back(i+2);
    m_idx.push_back(i+2);
    m_idx.push_back(i+3);
    m_idx.push_back(i+0);
  }
}

void CGUITextureGLES::DrawQuad(const CRect &rect, color_t color, CBaseTexture *texture, const CRect *texCoords)
{
  if (texture)
  {
    texture->LoadToGPU();
    texture->BindToUnit(0);
  }

  glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_BLEND);          // Turn Blending On

  VerifyGLState();

  GLubyte col[4];
  GLfloat ver[4][3];
  GLfloat tex[4][2];
  GLubyte idx[4] = {0, 1, 3, 2};        //determines order of triangle strip

  if (texture)
    g_Windowing.EnableGUIShader(SM_TEXTURE);
  else
    g_Windowing.EnableGUIShader(SM_DEFAULT);

  GLint posLoc   = g_Windowing.GUIShaderGetPos();
  GLint tex0Loc  = g_Windowing.GUIShaderGetCoord0();
  GLint uniColLoc= g_Windowing.GUIShaderGetUniCol();

  glVertexAttribPointer(posLoc,  3, GL_FLOAT, 0, 0, ver);
  if (texture)
    glVertexAttribPointer(tex0Loc, 2, GL_FLOAT, 0, 0, tex);

  glEnableVertexAttribArray(posLoc);
  if (texture)
    glEnableVertexAttribArray(tex0Loc);

  // Setup Colors
  col[0] = (GLubyte)GET_R(color);
  col[1] = (GLubyte)GET_G(color);
  col[2] = (GLubyte)GET_B(color);
  col[3] = (GLubyte)GET_A(color);

  glUniform4f(uniColLoc, col[0] / 255.0f, col[1] / 255.0f, col[2] / 255.0f, col[3] / 255.0f);

  ver[0][0] = ver[3][0] = rect.x1;
  ver[0][1] = ver[1][1] = rect.y1;
  ver[1][0] = ver[2][0] = rect.x2;
  ver[2][1] = ver[3][1] = rect.y2;
  ver[0][2] = ver[1][2] = ver[2][2] = ver[3][2]= 0;

  if (texture)
  {
    // Setup texture coordinates
    CRect coords = texCoords ? *texCoords : CRect(0.0f, 0.0f, 1.0f, 1.0f);
    tex[0][0] = tex[3][0] = coords.x1;
    tex[0][1] = tex[1][1] = coords.y1;
    tex[1][0] = tex[2][0] = coords.x2;
    tex[2][1] = tex[3][1] = coords.y2;
  }
  glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_BYTE, idx);

  glDisableVertexAttribArray(posLoc);
  if (texture)
    glDisableVertexAttribArray(tex0Loc);

  g_Windowing.DisableGUIShader();
}

#endif
