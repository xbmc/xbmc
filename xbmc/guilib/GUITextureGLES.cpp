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

bool CGUITextureGLES::DrawQuads(const std::vector<Quad>& quads,
                                unsigned int version,
                                const CRect& rect)
{
  if (m_quadBufferVersion != version)
  {
    std::vector<QuadVertex> vertices;
    vertices.reserve(quads.size() * 4);
    for (const Quad& quad : quads)
    {
      const CRect& a = quad.anchor;
      const CRect& o = quad.offset;
      const std::array<std::array<float, 4>, 4> corners{{{a.x1, a.y1, o.x1, o.y1},
                                                         {a.x2, a.y1, o.x2, o.y1},
                                                         {a.x2, a.y2, o.x2, o.y2},
                                                         {a.x1, a.y2, o.x1, o.y2}}};
      const std::array<CPoint, 4> texCoords = TexCoordCorners(quad.texture, GetOrientation() & 4);
      const std::array<CPoint, 4> diffuseCoords =
          TexCoordCorners(quad.diffuse, m_info.orientation & 4);
      for (std::size_t i = 0; i < 4; i++)
      {
        const std::size_t opposite = (i + 2) % 4;
        vertices.push_back({corners[i], corners[opposite],
                            {texCoords[i].x, texCoords[i].y, texCoords[opposite].x,
                             texCoords[opposite].y},
                            {diffuseCoords[i].x, diffuseCoords[i].y, diffuseCoords[opposite].x,
                             diffuseCoords[opposite].y}});
      }
    }

    if (!vertices.empty())
      m_quadBuffer.SetData(vertices.data(), vertices.size(), GL_STATIC_DRAW);

    m_quadBufferVersion = version;
    m_quadCount = quads.size();
  }

  if (m_quadCount == 0)
    return true;

  CGraphicContext& context = CServiceBroker::GetWinSystem()->GetGfxContext();
  constexpr float unbounded = std::numeric_limits<float>::max();
  const CRect clip = context.HasClipRegion()
                         ? context.GetClipRegion()
                         : CRect(-unbounded, -unbounded, unbounded, unbounded);

  GLint posLoc = m_renderSystem->GUIShaderGetPos();
  GLint snapLoc = m_renderSystem->GUIShaderGetAttrSnap();
  GLint tex0Loc = m_renderSystem->GUIShaderGetCoord0();
  GLint tex1Loc = m_renderSystem->GUIShaderGetCoord1();
  float swap0 = (GetOrientation() & 4) ? 1.0f : 0.0f;
  float swap1 = (m_info.orientation & 4) ? 1.0f : 0.0f;
  if (m_diffuse.size() && m_texture.m_textures[m_currentFrame]->GetSwizzle() == KD_TEX_SWIZ_111R)
  {
    std::swap(tex0Loc, tex1Loc);
    std::swap(swap0, swap1);
  }

  glUniform4f(m_renderSystem->GUIShaderGetQuadRect(), rect.x1, rect.y1, rect.Width(),
              rect.Height());
  glUniform4f(m_renderSystem->GUIShaderGetQuadClip(), clip.x1, clip.y1, clip.x2, clip.y2);
  glUniform2f(m_renderSystem->GUIShaderGetTexSwap(), swap0, swap1);
  glUniformMatrix4fv(m_renderSystem->GUIShaderGetGUIMatrix(), 1, GL_FALSE,
                     CMatrixGL(context.GetGUIMatrix()));
  glUniform1f(m_renderSystem->GUIShaderGetSnap(), 1.0f);

  m_quadBuffer.Bind();
  const auto attribute = [](GLint location, std::size_t member)
  {
    glVertexAttribPointer(location, 4, GL_FLOAT, GL_FALSE, sizeof(QuadVertex),
                          reinterpret_cast<GLvoid*>(member));
    glEnableVertexAttribArray(location);
  };
  attribute(posLoc, offsetof(QuadVertex, corner));
  attribute(snapLoc, offsetof(QuadVertex, opposite));
  attribute(tex0Loc, offsetof(QuadVertex, texture));
  if (m_diffuse.size())
    attribute(tex1Loc, offsetof(QuadVertex, diffuse));

  m_renderSystem->BindGUIQuadIndices(m_quadCount);
  glDrawElements(GL_TRIANGLES, m_quadCount * 6, GL_UNSIGNED_SHORT, 0);
  CRenderSystemBase::m_GUIElementCount++;

  glDisableVertexAttribArray(posLoc);
  glDisableVertexAttribArray(snapLoc);
  glDisableVertexAttribArray(tex0Loc);
  if (m_diffuse.size())
    glDisableVertexAttribArray(tex1Loc);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

  return true;
}
