/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RPRendererOpenGLES.h"

#include "cores/RetroPlayer/buffers/RenderBufferOpenGLES.h"
#include "cores/RetroPlayer/buffers/RenderBufferPoolOpenGLES.h"
#include "cores/RetroPlayer/rendering/RenderContext.h"
#include "cores/RetroPlayer/shaders/gles/ShaderPresetGLES.h"
#include "cores/RetroPlayer/shaders/gles/ShaderTextureGLES.h"
#include "cores/RetroPlayer/shaders/gles/ShaderTextureGLESRef.h"
#include "rendering/gles/RenderSystemGLES.h"
#include "utils/GLUtils.h"
#include "utils/log.h"

using namespace KODI;
using namespace RETRO;

// --- CRendererFactoryOpenGLES ------------------------------------------------

std::string CRendererFactoryOpenGLES::RenderSystemName() const
{
  return "OpenGLES";
}

CRPBaseRenderer* CRendererFactoryOpenGLES::CreateRenderer(
    const CRenderSettings& settings,
    CRenderContext& context,
    std::shared_ptr<IRenderBufferPool> bufferPool)
{
  return new CRPRendererOpenGLES(settings, context, std::move(bufferPool));
}

RenderBufferPoolVector CRendererFactoryOpenGLES::CreateBufferPools(CRenderContext& context)
{
  bool supportsTextureSwizzle = false;
#if defined(GL_ES_VERSION_3_0)
  auto* renderSystem = static_cast<CRenderSystemGLES*>(context.Rendering());
  supportsTextureSwizzle = renderSystem && renderSystem->SupportsTextureSwizzle();
#endif
  return {std::make_shared<CRenderBufferPoolOpenGLES>(supportsTextureSwizzle)};
}

// --- CRPRendererOpenGLES -----------------------------------------------------

CRPRendererOpenGLES::CRPRendererOpenGLES(const CRenderSettings& renderSettings,
                                         CRenderContext& context,
                                         std::shared_ptr<IRenderBufferPool> bufferPool)
  : CRPBaseRenderer(renderSettings, context, std::move(bufferPool))
{
  m_context.CaptureStateBlock();

  // Initialize CRPBaseRenderer
  m_shaderPreset = std::make_unique<SHADER::CShaderPresetGLES>(m_context);

  // Initialize CRPRendererOpenGLES
  m_clearColor = m_context.UseLimitedColor() ? (16.0f / 0xff) : 0.0f;

  m_context.ApplyStateBlock();
}

void CRPRendererOpenGLES::RenderInternal(bool clear, uint8_t alpha)
{
  if (clear)
  {
    if (alpha == 255)
      DrawBlackBars();
    else
      ClearBackBuffer();
  }

  Render(alpha);

  glEnable(GL_BLEND);
}

void CRPRendererOpenGLES::FlushInternal()
{
  m_RBTexturesMap.clear();

  if (!m_bConfigured)
    return;

  glFinish();
}

bool CRPRendererOpenGLES::Supports(RENDERFEATURE feature) const
{
  return feature == RENDERFEATURE::STRETCH || feature == RENDERFEATURE::ZOOM ||
         feature == RENDERFEATURE::PIXEL_RATIO || feature == RENDERFEATURE::ROTATION;
}

bool CRPRendererOpenGLES::SupportsScalingMethod(SCALINGMETHOD method)
{
  return method == SCALINGMETHOD::AUTO || method == SCALINGMETHOD::NEAREST ||
         method == SCALINGMETHOD::LINEAR;
}

void CRPRendererOpenGLES::ClearBackBuffer()
{
  glClearColor(m_clearColor, m_clearColor, m_clearColor, 0.0f);
  glClear(GL_COLOR_BUFFER_BIT);
  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
}

void CRPRendererOpenGLES::DrawBlackBars()
{
  glDisable(GL_BLEND);

  m_context.EnableGUIShader(GL_SHADER_METHOD::DEFAULT);

  GLint uniColLoc = m_context.GUIShaderGetUniCol();
  GLint depthLoc = m_context.GUIShaderGetDepth();

  glUniform4f(uniColLoc, m_clearColor / 255.0f, m_clearColor / 255.0f, m_clearColor / 255.0f, 1.0f);
  glUniform1f(depthLoc, -1.0f);

  auto& renderSystem = dynamic_cast<CRenderSystemGLES&>(*m_context.Rendering());
  const float width = m_context.GetScreenWidth();
  const float height = m_context.GetScreenHeight();
  const auto& dest = m_rotatedDestCoords;

  // top quad
  if (dest[0].y > 0.0f)
    renderSystem.DrawGUIQuad({0.0f, 0.0f}, {width, 0.0f}, {0.0f, dest[0].y});

  // bottom quad
  if (dest[2].y < height)
    renderSystem.DrawGUIQuad({0.0f, dest[2].y}, {width, dest[2].y}, {0.0f, height});

  // left quad
  if (dest[0].x > 0.0f)
    renderSystem.DrawGUIQuad({0.0f, dest[0].y}, dest[0], {0.0f, dest[3].y});

  // right quad
  if (dest[2].x < width)
    renderSystem.DrawGUIQuad(dest[1], {width, dest[1].y}, {dest[1].x, dest[2].y});

  m_context.DisableGUIShader();
}

void CRPRendererOpenGLES::Render(uint8_t alpha)
{
  auto renderBuffer = static_cast<CRenderBufferOpenGLES*>(m_renderBuffer);
  if (renderBuffer == nullptr)
    return;

  UpdateShaders();

  // Use video shader preset
  if (m_bUseShaderPreset)
  {
    RenderBufferTextures* rbTextures;

    // Drop cached textures if target size is changed
    if (m_fullDestWidth != m_lastTargetWidth || m_fullDestHeight != m_lastTargetHeight)
    {
      m_RBTexturesMap.clear();
      m_lastTargetWidth = m_fullDestWidth;
      m_lastTargetHeight = m_fullDestHeight;
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
          std::make_shared<SHADER::CShaderTextureGLESRef>(
              static_cast<unsigned int>(renderBuffer->GetWidth()),
              static_cast<unsigned int>(renderBuffer->GetHeight()), renderBuffer->TextureID()),
          // Target texture
          std::make_shared<SHADER::CShaderTextureGLES>(static_cast<unsigned int>(m_fullDestWidth),
                                                       static_cast<unsigned int>(m_fullDestHeight),
                                                       GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA, false)};
      rbTextures->targetTexture->CreateTexture(); // Create new internal texture
      m_RBTexturesMap.emplace(renderBuffer, rbTextures);
    }

    std::shared_ptr<SHADER::CShaderTextureGLESRef> sourceTexture = rbTextures->sourceTexture;
    std::shared_ptr<SHADER::CShaderTextureGLES> targetTexture = rbTextures->targetTexture;

    GLint filter = GL_NEAREST;
    if (m_shaderPreset->GetPasses().front().filterType == SHADER::FilterType::LINEAR)
      filter = GL_LINEAR;

    glBindTexture(m_textureTarget, sourceTexture->GetTextureID());
    glTexParameteri(m_textureTarget, GL_TEXTURE_MAG_FILTER, filter);
    glTexParameteri(m_textureTarget, GL_TEXTURE_MIN_FILTER, filter);
    glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    if (!m_shaderPreset->RenderUpdate(*sourceTexture, *targetTexture))
    {
      m_bShadersNeedUpdate = false;
      m_bUseShaderPreset = false;
    }

    glActiveTexture(GL_TEXTURE0); // GUI shader samples from texture unit 0
    glBindTexture(m_textureTarget, targetTexture->GetTextureID());
  }
  else
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

  // Setup texture coordinates
  CRect rect = m_sourceRect;
  rect.x1 /= renderBuffer->GetWidth();
  rect.x2 /= renderBuffer->GetWidth();
  rect.y1 /= renderBuffer->GetHeight();
  rect.y2 /= renderBuffer->GetHeight();

  // The destination corners are a rotated rectangle: top left, top right, bottom right, bottom left
  dynamic_cast<CRenderSystemGLES&>(*m_context.Rendering())
      .DrawGUIQuad(m_rotatedDestCoords[0], m_rotatedDestCoords[1], m_rotatedDestCoords[3], &rect);

  m_context.DisableGUIShader();
}
