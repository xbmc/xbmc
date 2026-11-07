/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RPRendererOpenGL.h"

#include "RPHdrShader.h"
#include "cores/RetroPlayer/buffers/RenderBufferOpenGL.h"
#include "cores/RetroPlayer/buffers/RenderBufferPoolOpenGL.h"
#include "cores/RetroPlayer/rendering/RenderContext.h"
#include "cores/RetroPlayer/shaders/gl/ShaderPresetGL.h"
#include "cores/RetroPlayer/shaders/gl/ShaderTextureGL.h"
#include "cores/RetroPlayer/shaders/gl/ShaderTextureGLRef.h"
#include "utils/GLUtils.h"
#include "utils/log.h"

#include <cstddef>

using namespace KODI;
using namespace RETRO;

// --- CRendererFactoryOpenGL --------------------------------------------------

std::string CRendererFactoryOpenGL::RenderSystemName() const
{
  return "OpenGL";
}

CRPBaseRenderer* CRendererFactoryOpenGL::CreateRenderer(
    const CRenderSettings& settings,
    CRenderContext& context,
    std::shared_ptr<IRenderBufferPool> bufferPool)
{
  return new CRPRendererOpenGL(settings, context, std::move(bufferPool));
}

RenderBufferPoolVector CRendererFactoryOpenGL::CreateBufferPools(CRenderContext& context)
{
  return {std::make_shared<CRenderBufferPoolOpenGL>()};
}

// --- CRPRendererOpenGL -------------------------------------------------------

CRPRendererOpenGL::CRPRendererOpenGL(const CRenderSettings& renderSettings,
                                     CRenderContext& context,
                                     std::shared_ptr<IRenderBufferPool> bufferPool)
  : CRPBaseRenderer(renderSettings, context, std::move(bufferPool))
{
  m_context.CaptureStateBlock();

  // Initialize CRPBaseRenderer
  m_shaderPreset = std::make_unique<SHADER::CShaderPresetGL>(m_context);

  m_context.EnableGUIShader(GL_SHADER_METHOD::TEXTURE);

  GLint posLoc = m_context.GUIShaderGetPos();
  GLint tex0Loc = m_context.GUIShaderGetCoord0();

  const GLubyte idx[4] = {0, 1, 3, 2}; // Determines order of triangle strip

  // Set up main screen VAO/VBO
  glGenVertexArrays(1, &m_mainVAO);
  glBindVertexArray(m_mainVAO);

  glGenBuffers(1, &m_mainVertexVBO);
  glBindBuffer(GL_ARRAY_BUFFER, m_mainVertexVBO);

  glVertexAttribPointer(posLoc, 3, GL_FLOAT, 0, sizeof(PackedVertex),
                        reinterpret_cast<const GLvoid*>(offsetof(PackedVertex, x)));
  glEnableVertexAttribArray(posLoc);
  glVertexAttribPointer(tex0Loc, 2, GL_FLOAT, 0, sizeof(PackedVertex),
                        reinterpret_cast<const GLvoid*>(offsetof(PackedVertex, u1)));
  glEnableVertexAttribArray(tex0Loc);

  glGenBuffers(1, &m_mainIndexVBO);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_mainIndexVBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLubyte) * 4, idx, GL_STATIC_DRAW);

  glBindVertexArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  m_context.DisableGUIShader();

  m_context.ApplyStateBlock();
}

CRPRendererOpenGL::~CRPRendererOpenGL()
{
  glDeleteBuffers(1, &m_hdrVBO);
  glDeleteVertexArrays(1, &m_hdrVAO);
  glDeleteBuffers(1, &m_mainIndexVBO);
  glDeleteBuffers(1, &m_mainVertexVBO);
  glDeleteVertexArrays(1, &m_mainVAO);
}

void CRPRendererOpenGL::RenderInternal(uint8_t alpha)
{
  Render(alpha);
}

void CRPRendererOpenGL::FlushInternal()
{
  m_RBTexturesMap.clear();

  if (!m_bConfigured)
    return;

  glFinish();
}

bool CRPRendererOpenGL::Supports(RENDERFEATURE feature) const
{
  return feature == RENDERFEATURE::STRETCH || feature == RENDERFEATURE::ZOOM ||
         feature == RENDERFEATURE::PIXEL_RATIO || feature == RENDERFEATURE::ROTATION;
}

bool CRPRendererOpenGL::SupportsScalingMethod(SCALINGMETHOD method)
{
  return method == SCALINGMETHOD::AUTO || method == SCALINGMETHOD::NEAREST ||
         method == SCALINGMETHOD::LINEAR;
}

void CRPRendererOpenGL::Render(uint8_t alpha)
{
  auto renderBuffer = static_cast<CRenderBufferOpenGL*>(m_renderBuffer);
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

std::shared_ptr<SHADER::CShaderTextureGL> CRPRendererOpenGL::CreateTargetTexture() const
{
  // More than 8 bits, as the HDR shader raises the brightest values steeply
  if (m_hdrPeak != 0)
    return std::make_shared<SHADER::CShaderTextureGL>(static_cast<unsigned int>(m_fullDestWidth),
                                                      static_cast<unsigned int>(m_fullDestHeight),
                                                      GL_HALF_FLOAT, GL_RGBA16F, GL_RGBA, false);

  return std::make_shared<SHADER::CShaderTextureGL>(static_cast<unsigned int>(m_fullDestWidth),
                                                    static_cast<unsigned int>(m_fullDestHeight),
                                                    GL_UNSIGNED_BYTE, GL_RGBA8, GL_BGRA, false);
}

bool CRPRendererOpenGL::LoadHdrShader()
{
  if (m_hdrShader)
    return true;

  if (m_hdrShaderFailed)
    return false;

  std::string defines;
  if (m_context.UseLimitedColor())
    defines += "#define KODI_LIMITED_RANGE 1\n";

  auto shader = std::make_unique<CRPHdrShader>(defines);
  if (!shader->CompileAndLink() || shader->GetPosLoc() < 0)
  {
    CLog::Log(LOGERROR, "RetroPlayer[RENDER]: Failed to compile the HDR shader");
    m_hdrShaderFailed = true;
    return false;
  }

  // A core profile draws from a vertex array. Triangle strip order: top left,
  // top right, bottom left, bottom right.
  static constexpr GLfloat unitQuad[4][2] = {
      {0.0f, 0.0f}, {1.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 1.0f}};

  glGenVertexArrays(1, &m_hdrVAO);
  glBindVertexArray(m_hdrVAO);

  glGenBuffers(1, &m_hdrVBO);
  glBindBuffer(GL_ARRAY_BUFFER, m_hdrVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(unitQuad), unitQuad, GL_STATIC_DRAW);

  glVertexAttribPointer(shader->GetPosLoc(), 2, GL_FLOAT, GL_FALSE, 0, nullptr);
  glEnableVertexAttribArray(shader->GetPosLoc());

  glBindVertexArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  m_hdrShader = std::move(shader);
  return true;
}

void CRPRendererOpenGL::DrawFrame(uint8_t alpha, float width, float height)
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

    glBindVertexArray(m_hdrVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);

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
