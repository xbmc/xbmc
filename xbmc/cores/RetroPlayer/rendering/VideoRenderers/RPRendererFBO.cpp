/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RPRendererFBO.h"

#if (defined(HAS_EGL) || defined(TARGET_DARWIN_OSX)) && (defined(HAS_GL) || HAS_GLES == 3)
#include "RenderGeometryFBO.h"
#include "cores/RetroPlayer/buffers/RenderBufferFBO.h"
#include "cores/RetroPlayer/buffers/RenderBufferPoolFBO.h"
#include "cores/RetroPlayer/rendering/RenderContext.h"
#include "cores/RetroPlayer/rendering/RenderVideoSettings.h"
#if defined(HAS_GLES)
#include "cores/RetroPlayer/shaders/gles/ShaderPresetGLES.h"
#include "cores/RetroPlayer/shaders/gles/ShaderTextureGLES.h"
#include "cores/RetroPlayer/shaders/gles/ShaderTextureGLESRef.h"
#else
#include "cores/RetroPlayer/shaders/gl/ShaderPresetGL.h"
#include "cores/RetroPlayer/shaders/gl/ShaderTextureGL.h"
#include "cores/RetroPlayer/shaders/gl/ShaderTextureGLRef.h"
#endif
#include "utils/GLUtils.h"
#include "utils/ScopeGuard.h"
#include "utils/log.h"
#endif

#include <cmath>
#include <memory>
#include <stddef.h>

using namespace KODI;
using namespace RETRO;

// --- CRendererFactoryFBO ------------------------------------------------

#if (defined(HAS_EGL) || defined(TARGET_DARWIN_OSX)) && (defined(HAS_GL) || HAS_GLES == 3)
std::string CRendererFactoryFBO::RenderSystemName() const
{
  return "FBO";
}

CRPBaseRenderer* CRendererFactoryFBO::CreateRenderer(const CRenderSettings& settings,
                                                     CRenderContext& context,
                                                     std::shared_ptr<IRenderBufferPool> bufferPool)
{
  return new CRPRendererFBO(settings, context, std::move(bufferPool));
}

RenderBufferPoolVector CRendererFactoryFBO::CreateBufferPools(CRenderContext& context)
{
  return {std::make_shared<CRenderBufferPoolFBO>(context)};
}

// --- CRPRendererFBO -----------------------------------------------------

CRPRendererFBO::CRPRendererFBO(const CRenderSettings& renderSettings,
                               CRenderContext& context,
                               std::shared_ptr<IRenderBufferPool> bufferPool)
  : CRPBaseRenderer(renderSettings, context, std::move(bufferPool))
{
  m_context.CaptureStateBlock();

  // Initialize CRPBaseRenderer
#if defined(HAS_GLES)
  m_shaderPreset = std::make_unique<SHADER::CShaderPresetGLES>(m_context);
#else
  m_shaderPreset = std::make_unique<SHADER::CShaderPresetGL>(m_context);
#endif

  // Initialize CRPRendererOpenGL
  m_clearColor = m_context.UseLimitedColor() ? (16.0f / 255.0f) : 0.0f;

  m_context.EnableGUIShader(GL_SHADER_METHOD::TEXTURE);

  const GLint posLoc = m_context.GUIShaderGetPos();
  const GLint tex0Loc = m_context.GUIShaderGetCoord0();

  const GLubyte idx[4] = {0, 1, 3, 2};

  // Set up main screen VAO/VBO
  glGenVertexArrays(1, &m_mainVAO);
  glBindVertexArray(m_mainVAO);

  glGenBuffers(1, &m_mainVertexVBO);
  glBindBuffer(GL_ARRAY_BUFFER, m_mainVertexVBO);

  glVertexAttribPointer(posLoc, 3, GL_FLOAT, GL_FALSE, sizeof(PackedVertex),
                        reinterpret_cast<const GLvoid*>(offsetof(PackedVertex, x)));
  glEnableVertexAttribArray(posLoc);
  glVertexAttribPointer(tex0Loc, 2, GL_FLOAT, GL_FALSE, sizeof(PackedVertex),
                        reinterpret_cast<const GLvoid*>(offsetof(PackedVertex, u1)));
  glEnableVertexAttribArray(tex0Loc);

  glGenBuffers(1, &m_mainIndexVBO);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_mainIndexVBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idx), idx, GL_STATIC_DRAW);

  glBindVertexArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  m_context.DisableGUIShader();

  m_context.EnableGUIShader(GL_SHADER_METHOD::DEFAULT);

  const GLint blackbarsPosLoc = m_context.GUIShaderGetPos();

  // Set up black bars VAO/VBO
  glGenVertexArrays(1, &m_blackbarsVAO);
  glBindVertexArray(m_blackbarsVAO);

  glGenBuffers(1, &m_blackbarsVertexVBO);
  glBindBuffer(GL_ARRAY_BUFFER, m_blackbarsVertexVBO);

  glVertexAttribPointer(blackbarsPosLoc, 3, GL_FLOAT, GL_FALSE, sizeof(Svertex), nullptr);
  glEnableVertexAttribArray(blackbarsPosLoc);

  glBindVertexArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  m_context.DisableGUIShader();

  m_context.ApplyStateBlock();
}

CRPRendererFBO::~CRPRendererFBO()
{
  glDeleteBuffers(1, &m_mainIndexVBO);
  glDeleteBuffers(1, &m_mainVertexVBO);
  glDeleteVertexArrays(1, &m_mainVAO);

  glDeleteBuffers(1, &m_blackbarsVertexVBO);
  glDeleteVertexArrays(1, &m_blackbarsVAO);
}

void CRPRendererFBO::RenderInternal(bool clear, uint8_t alpha)
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

void CRPRendererFBO::FlushInternal()
{
  m_RBTexturesMap.clear();

  if (!m_bConfigured)
    return;

  glFinish();
}

bool CRPRendererFBO::Supports(RENDERFEATURE feature) const
{
  return feature == RENDERFEATURE::STRETCH || feature == RENDERFEATURE::ZOOM ||
         feature == RENDERFEATURE::PIXEL_RATIO || feature == RENDERFEATURE::ROTATION;
}

bool CRPRendererFBO::SupportsScalingMethod(SCALINGMETHOD method)
{
  return method == SCALINGMETHOD::AUTO || method == SCALINGMETHOD::NEAREST ||
         method == SCALINGMETHOD::LINEAR;
}

void CRPRendererFBO::ClearBackBuffer()
{
  glClearColor(m_clearColor, m_clearColor, m_clearColor, 0.0f);
  glClear(GL_COLOR_BUFFER_BIT);
  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
}

void CRPRendererFBO::DrawBlackBars()
{
  glDisable(GL_BLEND);

  m_context.EnableGUIShader(GL_SHADER_METHOD::DEFAULT);

  GLint uniColLoc = m_context.GUIShaderGetUniCol();

  glUniform4f(uniColLoc, m_clearColor / 255.0f, m_clearColor / 255.0f, m_clearColor / 255.0f, 1.0f);
  glUniform1f(m_context.GUIShaderGetDepth(), -1.0f);

  Svertex vertices[24];
  GLubyte count = 0;

  const CRect destRect = CRenderGeometryFBO::GetDestinationRect(m_rotatedDestCoords);

  if (destRect.y1 > 0.0f)
  {
    GLubyte quad = count;
    vertices[quad].x = 0.0;
    vertices[quad].y = 0.0;
    vertices[quad].z = 0;
    vertices[quad + 1].x = m_context.GetScreenWidth();
    vertices[quad + 1].y = 0;
    vertices[quad + 1].z = 0;
    vertices[quad + 2].x = m_context.GetScreenWidth();
    vertices[quad + 2].y = destRect.y1;
    vertices[quad + 2].z = 0;
    vertices[quad + 3] = vertices[quad + 2];
    vertices[quad + 4].x = 0;
    vertices[quad + 4].y = destRect.y1;
    vertices[quad + 4].z = 0;
    vertices[quad + 5] = vertices[quad];
    count += 6;
  }

  if (destRect.y2 < m_context.GetScreenHeight())
  {
    GLubyte quad = count;
    vertices[quad].x = 0.0;
    vertices[quad].y = destRect.y2;
    vertices[quad].z = 0;
    vertices[quad + 1].x = m_context.GetScreenWidth();
    vertices[quad + 1].y = destRect.y2;
    vertices[quad + 1].z = 0;
    vertices[quad + 2].x = m_context.GetScreenWidth();
    vertices[quad + 2].y = m_context.GetScreenHeight();
    vertices[quad + 2].z = 0;
    vertices[quad + 3] = vertices[quad + 2];
    vertices[quad + 4].x = 0;
    vertices[quad + 4].y = m_context.GetScreenHeight();
    vertices[quad + 4].z = 0;
    vertices[quad + 5] = vertices[quad];
    count += 6;
  }

  if (destRect.x1 > 0.0f)
  {
    GLubyte quad = count;
    vertices[quad].x = 0.0;
    vertices[quad].y = destRect.y1;
    vertices[quad].z = 0;
    vertices[quad + 1].x = destRect.x1;
    vertices[quad + 1].y = destRect.y1;
    vertices[quad + 1].z = 0;
    vertices[quad + 2].x = destRect.x1;
    vertices[quad + 2].y = destRect.y2;
    vertices[quad + 2].z = 0;
    vertices[quad + 3] = vertices[quad + 2];
    vertices[quad + 4].x = 0;
    vertices[quad + 4].y = destRect.y2;
    vertices[quad + 4].z = 0;
    vertices[quad + 5] = vertices[quad];
    count += 6;
  }

  if (destRect.x2 < m_context.GetScreenWidth())
  {
    GLubyte quad = count;
    vertices[quad].x = destRect.x2;
    vertices[quad].y = destRect.y1;
    vertices[quad].z = 0;
    vertices[quad + 1].x = m_context.GetScreenWidth();
    vertices[quad + 1].y = destRect.y1;
    vertices[quad + 1].z = 0;
    vertices[quad + 2].x = m_context.GetScreenWidth();
    vertices[quad + 2].y = destRect.y2;
    vertices[quad + 2].z = 0;
    vertices[quad + 3] = vertices[quad + 2];
    vertices[quad + 4].x = destRect.x2;
    vertices[quad + 4].y = destRect.y2;
    vertices[quad + 4].z = 0;
    vertices[quad + 5] = vertices[quad];
    count += 6;
  }

  glBindVertexArray(m_blackbarsVAO);

  glBindBuffer(GL_ARRAY_BUFFER, m_blackbarsVertexVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(Svertex) * count, &vertices[0], GL_DYNAMIC_DRAW);

  glDrawArrays(GL_TRIANGLES, 0, count);

  glBindVertexArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  m_context.DisableGUIShader();
}

void CRPRendererFBO::Render(uint8_t alpha)
{
  auto renderBuffer = static_cast<CRenderBufferFBO*>(m_renderBuffer);
  if (renderBuffer == nullptr)
    return;

  auto bufferLock = renderBuffer->Lock();
  if (renderBuffer->TextureID() == 0 || renderBuffer->TextureWidth() == 0 ||
      renderBuffer->TextureHeight() == 0 || m_sourceRect.Width() <= 0.0f ||
      m_sourceRect.Height() <= 0.0f)
    return;

  CRect rect = CRenderGeometryFBO::GetTextureCoordinates(
      m_sourceRect, renderBuffer->GetHeight(), renderBuffer->TextureWidth(),
      renderBuffer->TextureHeight(), renderBuffer->BottomLeftOrigin());

  const FrameGeometry geometry{renderBuffer->GetWidth(),
                               renderBuffer->GetHeight(),
                               renderBuffer->TextureWidth(),
                               renderBuffer->TextureHeight(),
                               m_sourceRect,
                               rect,
                               renderBuffer->BottomLeftOrigin()};

  if (geometry != m_loggedGeometry)
  {
    CLog::Log(LOGDEBUG,
              "RetroPlayer[RENDER]: FBO geometry: frame {}x{}, texture {}x{}, source rect "
              "({:.1f},{:.1f})-({:.1f},{:.1f}), sampling ({:.3f},{:.3f})-({:.3f},{:.3f}), "
              "bottom-left origin {}",
              geometry.frameWidth, geometry.frameHeight, geometry.textureWidth,
              geometry.textureHeight, geometry.sourceRect.x1, geometry.sourceRect.y1,
              geometry.sourceRect.x2, geometry.sourceRect.y2, geometry.samplingRect.x1,
              geometry.samplingRect.y1, geometry.samplingRect.x2, geometry.samplingRect.y2,
              geometry.bottomLeftOrigin ? "yes" : "no");

    m_loggedGeometry = geometry;
  }

  renderBuffer->WaitForCapture();
  const UTILS::CScopeGuard<CRenderBufferFBO*, nullptr, void(CRenderBufferFBO*)> finishRender(
      [](CRenderBufferFBO* buffer) { buffer->FinishRender(); }, renderBuffer);

  GLuint drawTexture = renderBuffer->TextureID();

  UpdateShaders();

  if (m_bUseShaderPreset)
  {
    // Preserve the GUI target and clipping across shader target allocation and passes.
    GLint readFramebuffer, drawFramebuffer, scissorBox[4];
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &readFramebuffer);
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &drawFramebuffer);
    glGetIntegerv(GL_SCISSOR_BOX, scissorBox);
#if defined(HAS_GL)
    const GLboolean sRGBEnabled = glIsEnabled(GL_FRAMEBUFFER_SRGB);
#endif

    RenderBufferTextures* rbTextures = nullptr;

    if (m_fullDestWidth != m_lastTargetWidth || m_fullDestHeight != m_lastTargetHeight)
    {
      m_RBTexturesMap.clear();
      m_lastTargetWidth = m_fullDestWidth;
      m_lastTargetHeight = m_fullDestHeight;
    }

    auto it = m_RBTexturesMap.find(renderBuffer);
    if (it != m_RBTexturesMap.end())
    {
      const auto& sourceTexture = it->second->sourceTexture;
      // Capture allocations can change while the render buffer is reused.
      if (sourceTexture->GetTextureID() != renderBuffer->TextureID() ||
          sourceTexture->GetWidth() != renderBuffer->TextureWidth() ||
          sourceTexture->GetHeight() != renderBuffer->TextureHeight())
      {
        m_RBTexturesMap.erase(it);
        it = m_RBTexturesMap.end();
      }
    }

    if (it != m_RBTexturesMap.end())
    {
      rbTextures = it->second.get();
    }
    else if (m_fullDestWidth > 0 && m_fullDestHeight > 0)
    {
      auto textures = std::make_unique<RenderBufferTextures>(RenderBufferTextures{
#if defined(HAS_GL)
          std::make_shared<SHADER::CShaderTextureGLRef>(renderBuffer->TextureWidth(),
                                                        renderBuffer->TextureHeight(),
                                                        renderBuffer->TextureID()),
          std::make_shared<SHADER::CShaderTextureGL>(static_cast<unsigned int>(m_fullDestWidth),
                                                     static_cast<unsigned int>(m_fullDestHeight),
                                                     GL_UNSIGNED_BYTE, GL_RGBA8, GL_BGRA, false)
#elif defined(HAS_GLES)
          std::make_shared<SHADER::CShaderTextureGLESRef>(renderBuffer->TextureWidth(),
                                                          renderBuffer->TextureHeight(),
                                                          renderBuffer->TextureID()),
          std::make_shared<SHADER::CShaderTextureGLES>(static_cast<unsigned int>(m_fullDestWidth),
                                                       static_cast<unsigned int>(m_fullDestHeight),
                                                       GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA, false)
#endif
      });
      textures->targetTexture->CreateTexture();
      if (textures->targetTexture->BindFBO())
      {
        GLint targetFbo;
        glGetIntegerv(GL_FRAMEBUFFER_BINDING, &targetFbo);
        textures->targetTexture->UnbindFBO();
        if (targetFbo != 0)
        {
          rbTextures = textures.get();
          m_RBTexturesMap.emplace(renderBuffer, std::move(textures));
        }
      }
    }

    if (rbTextures)
    {
      const auto& sourceTexture = rbTextures->sourceTexture;
      const auto& targetTexture = rbTextures->targetTexture;

      GLint filter = GL_NEAREST;
      if (m_shaderPreset->GetPasses().front().filterType == SHADER::FilterType::LINEAR)
        filter = GL_LINEAR;

      glActiveTexture(GL_TEXTURE0);
      glBindTexture(m_textureTarget, sourceTexture->GetTextureID());
      glTexParameteri(m_textureTarget, GL_TEXTURE_MAG_FILTER, filter);
      glTexParameteri(m_textureTarget, GL_TEXTURE_MIN_FILTER, filter);
      glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

      if (m_shaderPreset->RenderUpdate(*sourceTexture, *targetTexture))
      {
        drawTexture = targetTexture->GetTextureID();
        rect = CRenderGeometryFBO::GetTextureCoordinates(m_sourceRect, renderBuffer->GetHeight(),
                                                         renderBuffer->GetWidth(),
                                                         renderBuffer->GetHeight(), false);
      }
      else
      {
        CLog::Log(LOGERROR,
                  "RetroPlayer[RENDER]: Video filter failed, drawing the frame unfiltered");
        m_bShadersNeedUpdate = false;
        m_bUseShaderPreset = false;
      }
    }

    glBindFramebuffer(GL_READ_FRAMEBUFFER, readFramebuffer);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, drawFramebuffer);
    glScissor(scissorBox[0], scissorBox[1], scissorBox[2], scissorBox[3]);
#if defined(HAS_GL)
    if (sRGBEnabled)
      glEnable(GL_FRAMEBUFFER_SRGB);
    else
      glDisable(GL_FRAMEBUFFER_SRGB);
#endif
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

  glActiveTexture(GL_TEXTURE0); // GUI shader samples from texture unit 0
  glBindTexture(m_textureTarget, drawTexture);

  GLint filter = GL_NEAREST;
  if (GetRenderSettings().VideoSettings().GetScalingMethod() == SCALINGMETHOD::LINEAR)
    filter = GL_LINEAR;
  glTexParameteri(m_textureTarget, GL_TEXTURE_MAG_FILTER, filter);
  glTexParameteri(m_textureTarget, GL_TEXTURE_MIN_FILTER, filter);
  glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  m_context.EnableGUIShader(GL_SHADER_METHOD::TEXTURE);

  GLint uniColLoc = m_context.GUIShaderGetUniCol();
  GLint depthLoc = m_context.GUIShaderGetDepth();

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
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertex), vertex, GL_DYNAMIC_DRAW);

  glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_BYTE, 0);

  if (!m_loggedHardwarePresentation)
  {
    CLog::Log(LOGDEBUG,
              "RetroPlayer[RENDER]: First hardware frame presented from shared texture {}",
              drawTexture);
    m_loggedHardwarePresentation = true;
  }

  glBindVertexArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  m_context.DisableGUIShader();
}
#else
std::string CRendererFactoryFBO::RenderSystemName() const
{
  return "FBO";
}

CRPBaseRenderer* CRendererFactoryFBO::CreateRenderer(
    const CRenderSettings& /*settings*/,
    CRenderContext& /*context*/,
    std::shared_ptr<IRenderBufferPool> /*bufferPool*/)
{
  return nullptr;
}

RenderBufferPoolVector CRendererFactoryFBO::CreateBufferPools(CRenderContext& /*context*/)
{
  return {};
}
#endif
