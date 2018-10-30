/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RPRendererGBM.h"

#include "cores/RetroPlayer/buffers/RenderBufferGBM.h"
#include "cores/RetroPlayer/buffers/RenderBufferPoolGBM.h"
#include "cores/RetroPlayer/rendering/RenderContext.h"
#include "cores/RetroPlayer/rendering/RenderVideoSettings.h"
#include "utils/GLUtils.h"

#include <cassert>
#include <cstddef>

using namespace KODI;
using namespace RETRO;

// --- CRendererFactoryGBM ------------------------------------------------

std::string CRendererFactoryGBM::RenderSystemName() const
{
  return "GBM";
}

CRPBaseRenderer *CRendererFactoryGBM::CreateRenderer(const CRenderSettings &settings, CRenderContext &context, std::shared_ptr<IRenderBufferPool> bufferPool)
{
  return new CRPRendererGBM(settings, context, std::move(bufferPool));
}

RenderBufferPoolVector CRendererFactoryGBM::CreateBufferPools(CRenderContext &context)
{
  return {
#if defined(HAS_GBM_BO_MAP)
    std::make_shared<CRenderBufferPoolGBM>(context)
#endif
  };
}

// --- CRPRendererGBM -----------------------------------------------------

CRPRendererGBM::CRPRendererGBM(const CRenderSettings &renderSettings, CRenderContext &context, std::shared_ptr<IRenderBufferPool> bufferPool) :
  CRPRendererOpenGLES(renderSettings, context, std::move(bufferPool))
{
  m_textureTarget = GL_TEXTURE_EXTERNAL_OES;
}

void CRPRendererGBM::Render(uint8_t alpha)
{
  CRenderBufferGBM *renderBuffer = static_cast<CRenderBufferGBM*>(m_renderBuffer);
  assert(renderBuffer != nullptr);

  CRect rect = m_sourceRect;

  rect.x1 /= renderBuffer->GetWidth();
  rect.x2 /= renderBuffer->GetWidth();
  rect.y1 /= renderBuffer->GetHeight();
  rect.y2 /= renderBuffer->GetHeight();

  const uint32_t color = (alpha << 24) | 0xFFFFFF;

  glBindTexture(m_textureTarget, renderBuffer->TextureID());

  GLint filter = GL_NEAREST;
  if (GetRenderSettings().VideoSettings().GetScalingMethod() == SCALINGMETHOD::LINEAR)
    filter = GL_LINEAR;
  glTexParameteri(m_textureTarget, GL_TEXTURE_MAG_FILTER, filter);
  glTexParameteri(m_textureTarget, GL_TEXTURE_MIN_FILTER, filter);
  glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  m_context.EnableGUIShader(GL_SHADER_METHOD::TEXTURE_RGBA_OES);

  GLubyte colour[4];
  GLubyte idx[4] = {0, 1, 3, 2}; // Determines order of triangle strip
  struct PackedVertex
  {
    float x, y, z;
    float u1, v1;
  } vertex[4];

  GLint vertLoc = m_context.GUIShaderGetPos();
  GLint loc = m_context.GUIShaderGetCoord0();
  GLint uniColLoc = m_context.GUIShaderGetUniCol();

  // Setup color values
  colour[0] = static_cast<GLubyte>(GET_R(color));
  colour[1] = static_cast<GLubyte>(GET_G(color));
  colour[2] = static_cast<GLubyte>(GET_B(color));
  colour[3] = static_cast<GLubyte>(GET_A(color));

  for (unsigned int i = 0; i < 4; i++)
  {
    // Setup vertex position values
    vertex[i].x = m_rotatedDestCoords[i].x;
    vertex[i].y = m_rotatedDestCoords[i].y;
    vertex[i].z = 0.0f;
  }

  // Setup texture coordinates
  vertex[0].u1 = vertex[3].u1 = rect.x1;
  vertex[0].v1 = vertex[1].v1 = rect.y1;
  vertex[1].u1 = vertex[2].u1 = rect.x2;
  vertex[2].v1 = vertex[3].v1 = rect.y2;

  glBindBuffer(GL_ARRAY_BUFFER, m_mainVertexVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(PackedVertex)*4, &vertex[0], GL_STATIC_DRAW);

  glVertexAttribPointer(vertLoc, 3, GL_FLOAT, 0, sizeof(PackedVertex), reinterpret_cast<const GLuint*>(offsetof(PackedVertex, x)));
  glVertexAttribPointer(loc, 2, GL_FLOAT, 0, sizeof(PackedVertex), reinterpret_cast<const GLuint*>(offsetof(PackedVertex, u1)));

  glEnableVertexAttribArray(vertLoc);
  glEnableVertexAttribArray(loc);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_mainIndexVBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLubyte)*4, idx, GL_STATIC_DRAW);

  glUniform4f(uniColLoc,(colour[0] / 255.0f), (colour[1] / 255.0f), (colour[2] / 255.0f), (colour[3] / 255.0f));
  glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_BYTE, 0);

  glDisableVertexAttribArray(vertLoc);
  glDisableVertexAttribArray(loc);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

  m_context.DisableGUIShader();
}
