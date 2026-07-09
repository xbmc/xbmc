/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUITextureGLES.h"

#include "GUIQuadDrawerGLES.h"
#include "ServiceBroker.h"
#include "Texture.h"
#include "guilib/TextureFormats.h"
#include "rendering/gles/RenderSystemGLES.h"
#include "utils/GLUtils.h"
#include "utils/MathUtils.h"
#include "utils/log.h"
#include "windowing/GraphicContext.h"
#include "windowing/WinSystem.h"

#include <cstddef>

void CGUITextureGLES::Register(CGUIQuadDrawerGLES& quadDrawer)
{
  CGUITexture::Register(
      CGUITextureGLES::CreateTexture,
      [&quadDrawer](const CRect& coords, KODI::UTILS::COLOR::Color color, CTexture* texture,
                    const CRect* texCoords, const float depth, const bool blending)
      { quadDrawer.DrawQuad(coords, color, texture, texCoords, depth, blending); });
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
  unsigned int major, minor;
  m_renderSystem->GetRenderVersion(major, minor);
  m_isGLES20 = major == 2;
}

CGUITextureGLES::CGUITextureGLES(const CGUITextureGLES& texture)
  : CGUITexture(texture),
    m_col(texture.m_col),
    m_packedVertices(texture.m_packedVertices),
    m_idx(texture.m_idx),
    m_renderSystem(texture.m_renderSystem),
    m_isGLES20(texture.m_isGLES20)
{
}

CGUITextureGLES* CGUITextureGLES::Clone() const
{
  return new CGUITextureGLES(*this);
}

void CGUITextureGLES::Free()
{
  m_VBO.Destroy();
  m_IBO.Destroy();
}

void CGUITextureGLES::Begin(KODI::UTILS::COLOR::Color color)
{
  CTexture* texture = m_texture.m_textures[m_currentFrame].get();
  texture->LoadToGPU();
  if (m_diffuse.size())
    m_diffuse.m_textures[0]->LoadToGPU();

  // Setup Colors
  m_col[0] = KODI::UTILS::GL::GetChannelFromARGB(KODI::UTILS::GL::ColorChannel::R, color);
  m_col[1] = KODI::UTILS::GL::GetChannelFromARGB(KODI::UTILS::GL::ColorChannel::G, color);
  m_col[2] = KODI::UTILS::GL::GetChannelFromARGB(KODI::UTILS::GL::ColorChannel::B, color);
  m_col[3] = KODI::UTILS::GL::GetChannelFromARGB(KODI::UTILS::GL::ColorChannel::A, color);

  bool hasAlpha = m_texture.m_textures[m_currentFrame]->HasAlpha() || m_col[3] < 255;
  const bool hasBlendColor =
      m_col[0] != 255 || m_col[1] != 255 || m_col[2] != 255 || m_col[3] != 255;

  if (m_diffuse.size())
  {
    if (m_isGLES20 && (texture->GetSwizzle() == KD_TEX_SWIZ_111R ||
                       m_diffuse.m_textures[0]->GetSwizzle() == KD_TEX_SWIZ_111R))
    {
      if (texture->GetSwizzle() == KD_TEX_SWIZ_111R &&
          m_diffuse.m_textures[0]->GetSwizzle() == KD_TEX_SWIZ_111R)
        m_renderSystem->EnableGUIShader(ShaderMethodGLES::SM_MULTI_111R_111R_BLENDCOLOR);
      else if (hasBlendColor)
        m_renderSystem->EnableGUIShader(ShaderMethodGLES::SM_MULTI_RGBA_111R_BLENDCOLOR);
      else
        m_renderSystem->EnableGUIShader(ShaderMethodGLES::SM_MULTI_RGBA_111R);
    }
    else if (hasBlendColor)
    {
      m_renderSystem->EnableGUIShader(ShaderMethodGLES::SM_MULTI_BLENDCOLOR);
    }
    else
    {
      m_renderSystem->EnableGUIShader(ShaderMethodGLES::SM_MULTI);
    }

    hasAlpha |= m_diffuse.m_textures[0]->HasAlpha();

    // We don't need a 111R_RGBA version of the GLES 2.0 shaders, so in the
    // unlikely event of having an alpha-only texture, switch with the
    // diffuse.
    if (texture->GetSwizzle() == KD_TEX_SWIZ_111R)
    {
      texture->BindToUnit(1);
      m_diffuse.m_textures[0]->BindToUnit(0);
    }
    else
    {
      texture->BindToUnit(0);
      m_diffuse.m_textures[0]->BindToUnit(1);
    }
  }
  else
  {
    if (m_isGLES20 && texture->GetSwizzle() == KD_TEX_SWIZ_111R)
    {
      m_renderSystem->EnableGUIShader(ShaderMethodGLES::SM_TEXTURE_111R);
    }
    else if (hasBlendColor)
    {
      m_renderSystem->EnableGUIShader(ShaderMethodGLES::SM_TEXTURE);
    }
    else
    {
      m_renderSystem->EnableGUIShader(ShaderMethodGLES::SM_TEXTURE_NOBLEND);
    }

    texture->BindToUnit(0);
  }

  if (hasAlpha)
  {
    // See CGUIFontTTFGLES::FirstBegin for rationale. SDR uses accumulator
    // coverage alpha; HDR FBO composite uses a compensated squared-alpha
    // blend because the FBO is color-transformed to PQ/HLG before composite,
    // and alpha blending in non-linear space is mathematically wrong.
    if (CServiceBroker::GetWinSystem()->IsHdrComposite())
      glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA,
                          GL_ONE_MINUS_SRC_ALPHA);
    else
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
  if (!m_packedVertices.empty())
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

    m_VBO.SetData(m_packedVertices.data(), m_packedVertices.size(), GL_STREAM_DRAW);
    m_IBO.SetData(m_idx.data(), m_idx.size(), GL_STREAM_DRAW);

    if(m_diffuse.size())
    {
      if (m_texture.m_textures[m_currentFrame]->GetSwizzle() == KD_TEX_SWIZ_111R)
        std::swap(tex0Loc, tex1Loc);
      glVertexAttribPointer(tex1Loc, 2, GL_FLOAT, 0, sizeof(PackedVertex),
                            reinterpret_cast<GLvoid*>(offsetof(PackedVertex, u2)));
      glEnableVertexAttribArray(tex1Loc);
    }
    glVertexAttribPointer(posLoc, 3, GL_FLOAT, 0, sizeof(PackedVertex),
                          reinterpret_cast<GLvoid*>(offsetof(PackedVertex, x)));
    glEnableVertexAttribArray(posLoc);
    glVertexAttribPointer(tex0Loc, 2, GL_FLOAT, 0, sizeof(PackedVertex),
                          reinterpret_cast<GLvoid*>(offsetof(PackedVertex, u1)));
    glEnableVertexAttribArray(tex0Loc);

    glDrawElements(GL_TRIANGLES, m_packedVertices.size() * 6 / 4, GL_UNSIGNED_SHORT, 0);
    CRenderSystemBase::m_GUIElementCount++;

    if (m_diffuse.size())
      glDisableVertexAttribArray(tex1Loc);

    glDisableVertexAttribArray(posLoc);
    glDisableVertexAttribArray(tex0Loc);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
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
  // TopLeft
  vertices[0].u1 = texture.x1;
  vertices[0].v1 = texture.y1;

  // TopRight
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

  // BottomRight
  vertices[2].u1 = texture.x2;
  vertices[2].v1 = texture.y2;

  // BottomLeft
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
    // TopLeft
    vertices[0].u2 = diffuse.x1;
    vertices[0].v2 = diffuse.y1;

    // TopRight
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

    // BottomRight
    vertices[2].u2 = diffuse.x2;
    vertices[2].v2 = diffuse.y2;

    // BottomLeft
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
