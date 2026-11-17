/*
 *  Copyright (C) 2017-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RPRendererDMAOpenGL.h"

#include "cores/RetroPlayer/buffers/RenderBufferDMA.h"
#include "cores/RetroPlayer/buffers/RenderBufferPoolDMAOpenGL.h"
#include "cores/RetroPlayer/rendering/RenderContext.h"
#include "cores/RetroPlayer/shaders/gl/ShaderPresetGL.h"
#include "cores/RetroPlayer/shaders/gl/ShaderTextureGL.h"
#include "cores/RetroPlayer/shaders/gl/ShaderTextureGLRef.h"
#include "utils/BufferObjectFactory.h"
#include "utils/GLUtils.h"

#include <cstddef>

using namespace KODI;
using namespace RETRO;

// --- CRendererFactoryDMAOpenGL --------------------------------------------------

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

  return {std::make_shared<CRenderBufferPoolDMAOpenGL>()};
}

CRPRendererDMAOpenGL::CRPRendererDMAOpenGL(const CRenderSettings& renderSettings,
                                           CRenderContext& context,
                                           std::shared_ptr<IRenderBufferPool> bufferPool)
  : CRPRendererOpenGL(renderSettings, context, std::move(bufferPool))
{
}

void CRPRendererDMAOpenGL::FlushInternal()
{
  m_RBTexturesMap.clear();
  CRPRendererOpenGL::FlushInternal();
}

void CRPRendererDMAOpenGL::Render(uint8_t alpha)
{
  auto renderBuffer = static_cast<CRenderBufferDMA*>(m_renderBuffer);
  if (renderBuffer == nullptr)
    return;

  UpdateShaders();

  // Use video shader preset
  if (m_bUseShaderPreset)
  {
    RenderBufferTextures* rbTextures;

    // Drop cached textures if target size or HDR state is changed
    const bool hdr = m_hdrPeak != 0;
    if (m_fullDestWidth != m_lastTargetWidth || m_fullDestHeight != m_lastTargetHeight ||
        hdr != m_lastTargetHdr)
    {
      m_RBTexturesMap.clear();
      m_lastTargetWidth = m_fullDestWidth;
      m_lastTargetHeight = m_fullDestHeight;
      m_lastTargetHdr = hdr;
    }

    const auto it = m_RBTexturesMap.find(renderBuffer);
    if (it != m_RBTexturesMap.end() &&
        it->second->sourceTexture->GetTextureID() == renderBuffer->TextureID() &&
        it->second->sourceTexture->GetWidth() == renderBuffer->GetWidth() &&
        it->second->sourceTexture->GetHeight() == renderBuffer->GetHeight())
    {
      rbTextures = it->second.get();
    }
    else
    {
      if (it != m_RBTexturesMap.end())
        m_RBTexturesMap.erase(it);

      rbTextures = new RenderBufferTextures{
          // Source texture
          std::make_shared<SHADER::CShaderTextureGLRef>(
              static_cast<unsigned int>(renderBuffer->GetWidth()),
              static_cast<unsigned int>(renderBuffer->GetHeight()), renderBuffer->TextureID()),
          // Target texture
          CreateTargetTexture()};
      rbTextures->targetTexture->CreateTexture(); // Create new internal texture
      m_RBTexturesMap.emplace(renderBuffer, rbTextures);
    }

    std::shared_ptr<SHADER::CShaderTextureGLRef> sourceTexture = rbTextures->sourceTexture;
    std::shared_ptr<SHADER::CShaderTextureGL> targetTexture = rbTextures->targetTexture;

    GLint filter = GL_NEAREST;
    if (m_shaderPreset->GetPasses().front().filterType == SHADER::FilterType::LINEAR)
      filter = GL_LINEAR;

    glBindTexture(m_textureTarget, sourceTexture->GetTextureID());
    glTexParameteri(m_textureTarget, GL_TEXTURE_MAG_FILTER, filter);
    glTexParameteri(m_textureTarget, GL_TEXTURE_MIN_FILTER, filter);
    glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    if (m_shaderPreset->RenderUpdate(*sourceTexture, *targetTexture))
    {
      glActiveTexture(GL_TEXTURE0); // GUI shader samples from texture unit 0
      glBindTexture(m_textureTarget, targetTexture->GetTextureID());
    }
    else
    {
      m_bShadersNeedUpdate = false;
      m_bUseShaderPreset = false;
    }
  }

  if (!m_bUseShaderPreset)
  {
    GLint filter = GL_NEAREST;
    if (GetRenderSettings().VideoSettings().GetScalingMethod() == SCALINGMETHOD::LINEAR)
      filter = GL_LINEAR;

    glActiveTexture(GL_TEXTURE0); // GUI shader samples from texture unit 0
    glBindTexture(m_textureTarget, renderBuffer->TextureID());
    glTexParameteri(m_textureTarget, GL_TEXTURE_MAG_FILTER, filter);
    glTexParameteri(m_textureTarget, GL_TEXTURE_MIN_FILTER, filter);
    glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  }

  DrawFrame(alpha, renderBuffer->GetWidth(), renderBuffer->GetHeight());
}
