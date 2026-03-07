/*
 *  Copyright (C) 2017-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RPRendererDMAOpenGL.h"

#include "cores/RetroPlayer/buffers/RenderBufferDMA.h"
#include "cores/RetroPlayer/buffers/RenderBufferPoolDMA.h"
#include "cores/RetroPlayer/rendering/RenderContext.h"
#include "cores/RetroPlayer/shaders/gl/ShaderPresetGL.h"
#include "cores/RetroPlayer/shaders/gl/ShaderTextureGL.h"
#include "cores/RetroPlayer/shaders/gl/ShaderTextureGLRef.h"
#include "utils/BufferObjectFactory.h"
#include "utils/GLUtils.h"

#include <cstddef>

using namespace KODI;
using namespace RETRO;

std::string CRendererFactoryDMAOpenGL::RenderSystemName() const
{
  return "DMAOpenGL";
}

CRPBaseRenderer* CRendererFactoryDMAOpenGL::CreateRenderer(
    const CRenderSettings& settings,
    CRenderContext& context,
    std::shared_ptr<IRenderBufferPool> bufferPool)
{
  return new CRPRendererDMAOpenGL(settings, context, std::move(bufferPool));
}

RenderBufferPoolVector CRendererFactoryDMAOpenGL::CreateBufferPools(CRenderContext& context)
{
  if (!CBufferObjectFactory::CreateBufferObject(false))
    return {};

  return {std::make_shared<CRenderBufferPoolDMA>(context)};
}

CRPRendererDMAOpenGL::CRPRendererDMAOpenGL(const CRenderSettings& renderSettings,
                                           CRenderContext& context,
                                           std::shared_ptr<IRenderBufferPool> bufferPool)
  : CRPRendererOpenGL(renderSettings, context, std::move(bufferPool))
{
}

void CRPRendererDMAOpenGL::Render(uint8_t alpha)
{
  auto renderBuffer = static_cast<CRenderBufferDMA*>(m_renderBuffer);
  if (renderBuffer == nullptr)
    return;

  // Drop cached textures if target size is changed
  if (m_fullDestWidth != m_lastTargetWidth || m_fullDestHeight != m_lastTargetHeight)
  {
    m_RBTexturesMap.clear();
    m_lastTargetWidth = m_fullDestWidth;
    m_lastTargetHeight = m_fullDestHeight;
  }

  RenderBufferTextures* rbTextures;

  const auto it = m_RBTexturesMap.find(renderBuffer);
  if (it != m_RBTexturesMap.end())
  {
    rbTextures = it->second.get();
  }
  else
  {
    rbTextures = new RenderBufferTextures{
        // Source texture
        std::make_shared<SHADER::CShaderTextureGLRef>(
            static_cast<unsigned int>(renderBuffer->GetWidth()),
            static_cast<unsigned int>(renderBuffer->GetHeight()), renderBuffer->TextureID()),
        // Target texture
        std::make_shared<SHADER::CShaderTextureGL>(static_cast<unsigned int>(m_fullDestWidth),
                                                   static_cast<unsigned int>(m_fullDestHeight),
                                                   GL_UNSIGNED_BYTE, GL_RGBA8, GL_BGRA, false)};
    rbTextures->target->CreateTexture(); // Create new internal texture
    m_RBTexturesMap.emplace(renderBuffer, rbTextures);
  }

  std::shared_ptr<SHADER::CShaderTextureGLRef> source = rbTextures->source;
  std::shared_ptr<SHADER::CShaderTextureGL> target = rbTextures->target;

  Updateshaders();

  // Use video shader preset
  if (m_bUseShaderPreset)
  {
    GLint filter = GL_NEAREST;
    if (m_shaderPreset->GetPasses().front().filterType == SHADER::FilterType::LINEAR)
      filter = GL_LINEAR;

    glBindTexture(m_textureTarget, source->GetTextureID());
    glTexParameteri(m_textureTarget, GL_TEXTURE_MAG_FILTER, filter);
    glTexParameteri(m_textureTarget, GL_TEXTURE_MIN_FILTER, filter);
    glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    if (!m_shaderPreset->RenderUpdate(*source, *target))
    {
      m_bShadersNeedUpdate = false;
      m_bUseShaderPreset = false;
    }

    glActiveTexture(GL_TEXTURE0); // GUI shader samples from texture unit 0
    glBindTexture(m_textureTarget, target->GetTextureID());
  }
  else
  {
    GLint filter = GL_NEAREST;
    if (GetRenderSettings().VideoSettings().GetScalingMethod() == SCALINGMETHOD::LINEAR)
      filter = GL_LINEAR;

    glActiveTexture(GL_TEXTURE0); // GUI shader samples from texture unit 0
    glBindTexture(m_textureTarget, source->GetTextureID());
    glTexParameteri(m_textureTarget, GL_TEXTURE_MAG_FILTER, filter);
    glTexParameteri(m_textureTarget, GL_TEXTURE_MIN_FILTER, filter);
    glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  }

  if (alpha < 255)
  {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  }
  else
  {
    glDisable(GL_BLEND);
  }

  // Use GUI shader
  m_context.EnableGUIShader(GL_SHADER_METHOD::TEXTURE);

  GLint uniColLoc = m_context.GUIShaderGetUniCol();
  GLint depthLoc = m_context.GUIShaderGetDepth();

  // Setup color values
  GLubyte col[4];
  const uint32_t color = (alpha << 24) | 0xFFFFFF;
  col[0] = UTILS::GL::GetChannelFromARGB(UTILS::GL::ColorChannel::R, color);
  col[1] = UTILS::GL::GetChannelFromARGB(UTILS::GL::ColorChannel::G, color);
  col[2] = UTILS::GL::GetChannelFromARGB(UTILS::GL::ColorChannel::B, color);
  col[3] = UTILS::GL::GetChannelFromARGB(UTILS::GL::ColorChannel::A, color);

  glUniform4f(uniColLoc, (col[0] / 255.0f), (col[1] / 255.0f), (col[2] / 255.0f),
              (col[3] / 255.0f));
  glUniform1f(depthLoc, -1.0f);

  // Setup destination rectangle
  CRect rect = m_sourceRect;
  rect.x1 /= renderBuffer->GetWidth();
  rect.x2 /= renderBuffer->GetWidth();
  rect.y1 /= renderBuffer->GetHeight();
  rect.y2 /= renderBuffer->GetHeight();

  PackedVertex vertex[4];

  // Setup vertex position values
  for (unsigned int i = 0; i < 4; i++)
  {
    vertex[i].x = m_rotatedDestCoords[i].x;
    vertex[i].y = m_rotatedDestCoords[i].y;
    vertex[i].z = 0.0f;
  }

  // Setup texture coordinates
  vertex[0].u1 = vertex[3].u1 = rect.x1;
  vertex[0].v1 = vertex[1].v1 = rect.y1;
  vertex[1].u1 = vertex[2].u1 = rect.x2;
  vertex[2].v1 = vertex[3].v1 = rect.y2;

  glBindVertexArray(m_mainVAO);

  glBindBuffer(GL_ARRAY_BUFFER, m_mainVertexVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(PackedVertex) * 4, &vertex[0], GL_DYNAMIC_DRAW);

  glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_BYTE, nullptr);

  glBindVertexArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  m_context.DisableGUIShader();
}
