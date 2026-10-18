/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RPRendererOpenGLES.h"

#include "RPHdrShader.h"
#include "ServiceBroker.h"
#include "cores/RetroPlayer/buffers/RenderBufferOpenGLES.h"
#include "cores/RetroPlayer/buffers/RenderBufferPoolOpenGLES.h"
#include "cores/RetroPlayer/rendering/RenderContext.h"
#include "cores/RetroPlayer/shaders/gles/ShaderPresetGLES.h"
#include "cores/RetroPlayer/shaders/gles/ShaderTextureGLES.h"
#include "cores/RetroPlayer/shaders/gles/ShaderTextureGLESRef.h"
#include "rendering/GLExtensions.h"
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

  m_context.ApplyStateBlock();
}

CRPRendererOpenGLES::~CRPRendererOpenGLES() = default;

void CRPRendererOpenGLES::RenderInternal(uint8_t alpha)
{
  Render(alpha);
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
          std::make_shared<SHADER::CShaderTextureGLESRef>(
              static_cast<unsigned int>(renderBuffer->GetWidth()),
              static_cast<unsigned int>(renderBuffer->GetHeight()), renderBuffer->TextureID()),
          // Target texture
          CreateTargetTexture()};
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

std::shared_ptr<SHADER::CShaderTextureGLES> CRPRendererOpenGLES::CreateTargetTexture() const
{
  GLuint pixelType = GL_UNSIGNED_BYTE;
  GLuint internalFormat = GL_RGBA;
#if HAS_GLES >= 3
  // More than 8 bits, as the HDR shader raises the brightest values steeply.
  // A GLES 3 context can always render to 10 bits, but to half floats only
  // with an extension. GBM can fall back to a GLES 2 context.
  unsigned int major{0};
  unsigned int minor{0};
  CServiceBroker::GetRenderSystem()->GetRenderVersion(major, minor);
  if (m_hdrPeak != 0 && major >= 3)
  {
    if (CGLExtensions::IsExtensionSupported(CGLExtensions::EXT_color_buffer_float))
    {
      pixelType = GL_HALF_FLOAT;
      internalFormat = GL_RGBA16F;
    }
    else
    {
      pixelType = GL_UNSIGNED_INT_2_10_10_10_REV;
      internalFormat = GL_RGB10_A2;
    }
  }
#endif
  return std::make_shared<SHADER::CShaderTextureGLES>(static_cast<unsigned int>(m_fullDestWidth),
                                                      static_cast<unsigned int>(m_fullDestHeight),
                                                      pixelType, internalFormat, GL_RGBA, false);
}

bool CRPRendererOpenGLES::LoadHdrShader()
{
  if (m_hdrShader)
    return true;

  if (m_hdrShaderFailed)
    return false;

  std::string defines;
  if (m_context.UseLimitedColor())
    defines += "#define KODI_LIMITED_RANGE 1\n";

  auto shader = std::make_unique<CRPHdrShader>(defines);
  if (!shader->CompileAndLink())
  {
    CLog::Log(LOGERROR, "RetroPlayer[RENDER]: Failed to compile the HDR shader");
    m_hdrShaderFailed = true;
    return false;
  }

  m_hdrShader = std::move(shader);
  return true;
}

void CRPRendererOpenGLES::DrawFrame(uint8_t alpha, float width, float height)
{
  if (alpha < 255)
  {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  }
  else
  {
    glDisable(GL_BLEND);
  }

  // Setup texture coordinates
  CRect rect = m_sourceRect;
  rect.x1 /= width;
  rect.x2 /= width;
  rect.y1 /= height;
  rect.y2 /= height;

  if (m_hdrPeak != 0 && LoadHdrShader())
  {
    m_hdrShader->SetLuminance(m_hdrPaperWhite, m_hdrPeak);
    m_hdrShader->SetAlpha(alpha / 255.0f);
    m_hdrShader->SetQuad(m_rotatedDestCoords[0], m_rotatedDestCoords[1], m_rotatedDestCoords[3],
                         rect);
    m_hdrShader->Enable();

    dynamic_cast<CRenderSystemGLES&>(*m_context.Rendering()).BindGUIUnitQuad();
    const GLint posLoc = m_hdrShader->GetPosLoc();
    glVertexAttribPointer(posLoc, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(posLoc);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glDisableVertexAttribArray(posLoc);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    m_hdrShader->Disable();
    return;
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

  // The destination corners are a rotated rectangle: top left, top right, bottom right, bottom left
  dynamic_cast<CRenderSystemGLES&>(*m_context.Rendering())
      .DrawGUIQuad(m_rotatedDestCoords[0], m_rotatedDestCoords[1], m_rotatedDestCoords[3], &rect);

  m_context.DisableGUIShader();
}
