/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUITextureGLES.h"

#include "ServiceBroker.h"
#include "Texture.h"
#include "rendering/gles/RenderSystemGLES.h"
#include "utils/GLUtils.h"
#include "utils/MathUtils.h"
#include "utils/log.h"
#include "windowing/GraphicContext.h"
#include "windowing/WinSystem.h"

#include <cstddef>

void CGUITextureGLES::Register()
{
  CGUITexture::Register(CGUITextureGLES::CreateTexture, CGUITextureGLES::DrawQuad);
}

CGUITexture* CGUITextureGLES::CreateTexture(
    float posX, float posY, float width, float height, const CTextureInfo& texture)
{
  return new CGUITextureGLES(posX, posY, width, height, texture);
}

CGUITextureGLES::CGUITextureGLES(
    float posX, float posY, float width, float height, const CTextureInfo& texture)
  : CGUITexture(posX, posY, width, height, texture)
{
  m_renderSystem = dynamic_cast<CRenderSystemGLES*>(CServiceBroker::GetRenderSystem());
}

CGUITextureGLES* CGUITextureGLES::Clone() const
{
  return new CGUITextureGLES(*this);
}

void CGUITextureGLES::Begin(UTILS::COLOR::Color color)
{
  CTexture* texture = m_texture.m_textures[m_currentFrame].get();
  texture->LoadToGPU();
  if (m_diffuse.size())
    m_diffuse.m_textures[0]->LoadToGPU();

  texture->BindToUnit(0);

  // Setup Colors
  m_col[0] = KODI::UTILS::GL::GetChannelFromARGB(KODI::UTILS::GL::ColorChannel::R, color);
  m_col[1] = KODI::UTILS::GL::GetChannelFromARGB(KODI::UTILS::GL::ColorChannel::G, color);
  m_col[2] = KODI::UTILS::GL::GetChannelFromARGB(KODI::UTILS::GL::ColorChannel::B, color);
  m_col[3] = KODI::UTILS::GL::GetChannelFromARGB(KODI::UTILS::GL::ColorChannel::A, color);

  if (CServiceBroker::GetWinSystem()->UseLimitedColor())
  {
    m_col[0] = (235 - 16) * m_col[0] / 255 + 16;
    m_col[1] = (235 - 16) * m_col[1] / 255 + 16;
    m_col[2] = (235 - 16) * m_col[2] / 255 + 16;
  }

  bool hasAlpha = m_texture.m_textures[m_currentFrame]->HasAlpha() || m_col[3] < 255;

  if (m_diffuse.size())
  {
    if (m_col[0] == 255 && m_col[1] == 255 && m_col[2] == 255 && m_col[3] == 255 )
    {
      m_renderSystem->EnableGUIShader(ShaderMethodGLES::SM_MULTI);
    }
    else
    {
      m_renderSystem->EnableGUIShader(ShaderMethodGLES::SM_MULTI_BLENDCOLOR);
    }

    hasAlpha |= m_diffuse.m_textures[0]->HasAlpha();

    m_diffuse.m_textures[0]->BindToUnit(1);

  }
  else
  {
    if (m_col[0] == 255 && m_col[1] == 255 && m_col[2] == 255 && m_col[3] == 255)
    {
      m_renderSystem->EnableGUIShader(ShaderMethodGLES::SM_TEXTURE_NOBLEND);
    }
    else
    {
      m_renderSystem->EnableGUIShader(ShaderMethodGLES::SM_TEXTURE);
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
  if (m_packedVertices.size())
  {
    GLint posLoc  = m_renderSystem->GUIShaderGetPos();
    GLint tex0Loc = m_renderSystem->GUIShaderGetCoord0();
    GLint tex1Loc = m_renderSystem->GUIShaderGetCoord1();
    GLint uniColLoc = m_renderSystem->GUIShaderGetUniCol();
    GLint depthLoc = m_renderSystem->GUIShaderGetDepth();

    if(uniColLoc >= 0)
    {
      glUniform4f(uniColLoc,(m_col[0] / 255.0f), (m_col[1] / 255.0f), (m_col[2] / 255.0f), (m_col[3] / 255.0f));
    }

    glUniform1f(depthLoc, m_depth);

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
      glDisableVertexAttribArray(tex1Loc);

    glDisableVertexAttribArray(posLoc);
    glDisableVertexAttribArray(tex0Loc);
  }

  if (m_diffuse.size())
    glActiveTexture(GL_TEXTURE0);
  glEnable(GL_BLEND);
  m_renderSystem->DisableGUIShader();
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

void CGUITextureGLES::DrawQuad(const CRect& rect,
                               UTILS::COLOR::Color color,
                               CTexture* texture,
                               const CRect* texCoords,
                               const float depth,
                               const bool blending)
{
  CRenderSystemGLES *renderSystem = dynamic_cast<CRenderSystemGLES*>(CServiceBroker::GetRenderSystem());
  if (texture)
  {
    texture->LoadToGPU();
    texture->BindToUnit(0);
  }

  if (blending)
  {
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
  }
  else
  {
    glDisable(GL_BLEND);
  }

  VerifyGLState();

  GLubyte col[4];
  GLfloat ver[4][3];
  GLfloat tex[4][2];
  GLubyte idx[4] = {0, 1, 3, 2};        //determines order of triangle strip

  if (texture)
    renderSystem->EnableGUIShader(ShaderMethodGLES::SM_TEXTURE);
  else
    renderSystem->EnableGUIShader(ShaderMethodGLES::SM_DEFAULT);

  GLint posLoc   = renderSystem->GUIShaderGetPos();
  GLint tex0Loc  = renderSystem->GUIShaderGetCoord0();
  GLint uniColLoc= renderSystem->GUIShaderGetUniCol();
  GLint depthLoc = renderSystem->GUIShaderGetDepth();

  glVertexAttribPointer(posLoc,  3, GL_FLOAT, 0, 0, ver);
  if (texture)
    glVertexAttribPointer(tex0Loc, 2, GL_FLOAT, 0, 0, tex);

  glEnableVertexAttribArray(posLoc);
  if (texture)
    glEnableVertexAttribArray(tex0Loc);

  // Setup Colors
  col[0] = KODI::UTILS::GL::GetChannelFromARGB(KODI::UTILS::GL::ColorChannel::R, color);
  col[1] = KODI::UTILS::GL::GetChannelFromARGB(KODI::UTILS::GL::ColorChannel::G, color);
  col[2] = KODI::UTILS::GL::GetChannelFromARGB(KODI::UTILS::GL::ColorChannel::B, color);
  col[3] = KODI::UTILS::GL::GetChannelFromARGB(KODI::UTILS::GL::ColorChannel::A, color);

  glUniform4f(uniColLoc, col[0] / 255.0f, col[1] / 255.0f, col[2] / 255.0f, col[3] / 255.0f);
  glUniform1f(depthLoc, depth);

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

  renderSystem->DisableGUIShader();
}

