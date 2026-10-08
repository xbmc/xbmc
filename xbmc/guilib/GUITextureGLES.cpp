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
#include "rendering/MatrixGL.h"
#include "rendering/gles/RenderSystemGLES.h"
#include "utils/GLUtils.h"
#include "utils/MathUtils.h"
#include "utils/log.h"
#include "windowing/GraphicContext.h"
#include "windowing/WinSystem.h"

#include <cstddef>
#include <limits>

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
  m_isGLES20 = !m_renderSystem->SupportsTextureSwizzle();
}

CGUITextureGLES::CGUITextureGLES(const CGUITextureGLES& texture)
  : CGUITexture(texture),
    m_renderSystem(texture.m_renderSystem),
    m_isGLES20(texture.m_isGLES20)
{
}

CGUITextureGLES* CGUITextureGLES::Clone() const
{
  return new CGUITextureGLES(*this);
}

namespace
{
// Texture coordinates for the corners of a quad, clockwise from the top left.
std::array<CPoint, 4> TexCoordCorners(const CRect& rect, bool swapXY)
{
  if (swapXY)
    return {{{rect.x1, rect.y1}, {rect.x1, rect.y2}, {rect.x2, rect.y2}, {rect.x2, rect.y1}}};

  return {{{rect.x1, rect.y1}, {rect.x2, rect.y1}, {rect.x2, rect.y2}, {rect.x1, rect.y2}}};
}
} // namespace

void CGUITextureGLES::Free()
{
  m_quadBuffer.Destroy();
  m_quadBufferVersion = 0;
  m_quadCount = 0;
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

  GLint uniColLoc = m_renderSystem->GUIShaderGetUniCol();
  if (uniColLoc >= 0)
  {
    glUniform4f(uniColLoc, (m_col[0] / 255.0f), (m_col[1] / 255.0f), (m_col[2] / 255.0f),
                (m_col[3] / 255.0f));
  }

  glUniform1f(m_renderSystem->GUIShaderGetDepth(), m_depth);
}

void CGUITextureGLES::End()
{
  if (m_diffuse.size())
    glActiveTexture(GL_TEXTURE0);
  glEnable(GL_BLEND);

  m_renderSystem->DisableGUIShader();
}

void CGUITextureGLES::Draw(float*, float*, float*, const CRect&, const CRect&, int)
{
  // Unused: DrawQuads() draws every quad.
}

bool CGUITextureGLES::DrawQuads(const std::vector<Quad>& quads, unsigned int version)
{
  if (m_quadBufferVersion != version)
  {
    const std::size_t count = quads.size();
    if (count > 0)
    {
      const int orientation = GetOrientation();
      std::vector<QuadVertex> vertices;
      vertices.reserve(count * 4);
      for (const Quad& quad : quads)
      {
        const CRect& rect = quad.vertex;
        const std::array<CPoint, 4> corners{
            {{rect.x1, rect.y1}, {rect.x2, rect.y1}, {rect.x2, rect.y2}, {rect.x1, rect.y2}}};
        const std::array<CPoint, 4> texCoords = TexCoordCorners(quad.texture, orientation & 4);
        const std::array<CPoint, 4> diffuseCoords =
            TexCoordCorners(quad.diffuse, m_info.orientation & 4);
        const CPoint texDx = (texCoords[1] - texCoords[0]) / rect.Width();
        const CPoint texDy = (texCoords[3] - texCoords[0]) / rect.Height();
        const CPoint diffuseDx = (diffuseCoords[1] - diffuseCoords[0]) / rect.Width();
        const CPoint diffuseDy = (diffuseCoords[3] - diffuseCoords[0]) / rect.Height();
        for (std::size_t i = 0; i < 4; i++)
        {
          const CPoint& opposite = corners[(i + 2) % 4];
          // CGUITexture::Render() pushes the bottom right and bottom left corners.
          const float push = i >= 2 ? 1.0f : 0.0f;
          vertices.push_back({corners[i].x, corners[i].y, opposite.x, opposite.y, push,
                              texCoords[i].x, texCoords[i].y, diffuseCoords[i].x,
                              diffuseCoords[i].y, texDx.x, texDx.y, texDy.x, texDy.y,
                              diffuseDx.x, diffuseDx.y, diffuseDy.x, diffuseDy.y});
        }
      }
      m_quadBuffer.SetData(vertices.data(), vertices.size(), GL_STATIC_DRAW);
    }
    m_quadBufferVersion = version;
    m_quadCount = count;
  }
  else if (m_quadCount > 0)
  {
    m_quadBuffer.Bind();
  }

  if (m_quadCount == 0)
    return true;

  CGraphicContext& context = CServiceBroker::GetWinSystem()->GetGfxContext();
  constexpr float unbounded = std::numeric_limits<float>::max();
  const CRect clip = context.HasClipRegion()
                         ? context.GetClipRegion()
                         : CRect(-unbounded, -unbounded, unbounded, unbounded);
  glUniform4f(m_renderSystem->GUIShaderGetQuadClip(), clip.x1, clip.y1, clip.x2, clip.y2);
  glUniformMatrix4fv(m_renderSystem->GUIShaderGetGUIMatrix(), 1, GL_FALSE,
                     CMatrixGL(context.GetGUIMatrix()));
  glUniform1f(m_renderSystem->GUIShaderGetSnap(), 1.0f);

  GLint posLoc = m_renderSystem->GUIShaderGetPos();
  GLint snapLoc = m_renderSystem->GUIShaderGetAttrSnap();
  GLint tex0Loc = m_renderSystem->GUIShaderGetCoord0();
  GLint tex1Loc = m_renderSystem->GUIShaderGetCoord1();
  GLint grad0Loc = m_renderSystem->GUIShaderGetAttrGrad0();
  GLint grad1Loc = m_renderSystem->GUIShaderGetAttrGrad1();

  m_renderSystem->BindGUIQuadIndices(m_quadCount);

  if (m_diffuse.size())
  {
    if (m_texture.m_textures[m_currentFrame]->GetSwizzle() == KD_TEX_SWIZ_111R)
    {
      std::swap(tex0Loc, tex1Loc);
      std::swap(grad0Loc, grad1Loc);
    }
    glVertexAttribPointer(tex1Loc, 2, GL_FLOAT, 0, sizeof(QuadVertex),
                          reinterpret_cast<GLvoid*>(offsetof(QuadVertex, u2)));
    glEnableVertexAttribArray(tex1Loc);
    glVertexAttribPointer(grad1Loc, 4, GL_FLOAT, 0, sizeof(QuadVertex),
                          reinterpret_cast<GLvoid*>(offsetof(QuadVertex, du2dx)));
    glEnableVertexAttribArray(grad1Loc);
  }
  glVertexAttribPointer(posLoc, 2, GL_FLOAT, 0, sizeof(QuadVertex),
                        reinterpret_cast<GLvoid*>(offsetof(QuadVertex, x)));
  glEnableVertexAttribArray(posLoc);
  glVertexAttribPointer(snapLoc, 3, GL_FLOAT, 0, sizeof(QuadVertex),
                        reinterpret_cast<GLvoid*>(offsetof(QuadVertex, oppositeX)));
  glEnableVertexAttribArray(snapLoc);
  glVertexAttribPointer(tex0Loc, 2, GL_FLOAT, 0, sizeof(QuadVertex),
                        reinterpret_cast<GLvoid*>(offsetof(QuadVertex, u1)));
  glEnableVertexAttribArray(tex0Loc);
  glVertexAttribPointer(grad0Loc, 4, GL_FLOAT, 0, sizeof(QuadVertex),
                        reinterpret_cast<GLvoid*>(offsetof(QuadVertex, du1dx)));
  glEnableVertexAttribArray(grad0Loc);

  glDrawElements(GL_TRIANGLES, m_quadCount * 6, GL_UNSIGNED_SHORT, 0);
  CRenderSystemBase::m_GUIElementCount++;

  if (m_diffuse.size())
  {
    glDisableVertexAttribArray(tex1Loc);
    glDisableVertexAttribArray(grad1Loc);
  }
  glDisableVertexAttribArray(posLoc);
  glDisableVertexAttribArray(snapLoc);
  glDisableVertexAttribArray(grad0Loc);
  glDisableVertexAttribArray(tex0Loc);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

  return true;
}
