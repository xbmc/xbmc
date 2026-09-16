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
namespace
{
// Restore the GUI render target before presenting the filtered texture.
class CFramebufferState
{
public:
  CFramebufferState()
  {
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &m_readFbo);
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &m_drawFbo);
    glGetIntegerv(GL_VIEWPORT, m_viewport);
    glGetIntegerv(GL_SCISSOR_BOX, m_scissorBox);
#if defined(HAS_GL)
    m_sRGBEnabled = glIsEnabled(GL_FRAMEBUFFER_SRGB);
#endif
  }

  CFramebufferState(const CFramebufferState&) = delete;
  CFramebufferState& operator=(const CFramebufferState&) = delete;

  ~CFramebufferState()
  {
    glBindFramebuffer(GL_READ_FRAMEBUFFER, m_readFbo);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_drawFbo);
    glViewport(m_viewport[0], m_viewport[1], m_viewport[2], m_viewport[3]);
    glScissor(m_scissorBox[0], m_scissorBox[1], m_scissorBox[2], m_scissorBox[3]);
#if defined(HAS_GL)
    if (m_sRGBEnabled)
      glEnable(GL_FRAMEBUFFER_SRGB);
    else
      glDisable(GL_FRAMEBUFFER_SRGB);
#endif
  }

private:
  GLint m_readFbo{};
  GLint m_drawFbo{};
  GLint m_viewport[4]{};
  GLint m_scissorBox[4]{};
#if defined(HAS_GL)
  GLboolean m_sRGBEnabled{};
#endif
};
} // namespace

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

  m_clearColour = m_context.UseLimitedColor() ? (16.0f / 255.0f) : 0.0f;
#if defined(HAS_GLES)
  m_shaderPreset = std::make_unique<SHADER::CShaderPresetGLES>(m_context);
#else
  m_shaderPreset = std::make_unique<SHADER::CShaderPresetGL>(m_context);
#endif

  m_context.EnableGUIShader(GL_SHADER_METHOD::TEXTURE);
  const GLint posLoc = m_context.GUIShaderGetPos();
  const GLint tex0Loc = m_context.GUIShaderGetCoord0();
  const GLubyte idx[4] = {0, 1, 3, 2};

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

  DestroyShaderResources();
}

void CRPRendererFBO::DestroyShaderResources()
{
  m_shaderTargetTexture.reset();
  m_shaderTargetWidth = 0;
  m_shaderTargetHeight = 0;
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
  glClearColor(m_clearColour, m_clearColour, m_clearColour, 0.0f);
  glClear(GL_COLOR_BUFFER_BIT);
  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
}

void CRPRendererFBO::DrawBlackBars()
{
  glDisable(GL_BLEND);

  Svertex vertices[24];
  GLubyte count = 0;
  const CRect destRect = CRenderGeometryFBO::GetDestinationRect(m_rotatedDestCoords);

  m_context.EnableGUIShader(GL_SHADER_METHOD::DEFAULT);
  GLint uniCol = m_context.GUIShaderGetUniCol();

  glUniform4f(uniCol, 0.0f, 0.0f, 0.0f, 1.0f);
  glUniform1f(m_context.GUIShaderGetDepth(), -1.0f);

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
  CRenderBufferFBO* renderBuffer = static_cast<CRenderBufferFBO*>(m_renderBuffer);

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

  const uint32_t color = (alpha << 24) | 0xFFFFFF;

  renderBuffer->WaitForCapture();
  const UTILS::CScopeGuard<CRenderBufferFBO*, nullptr, void(CRenderBufferFBO*)> finishRender(
      [](CRenderBufferFBO* buffer) { buffer->FinishRender(); }, renderBuffer);

  GLuint drawTexture = renderBuffer->TextureID();
  bool bShaded = false;

  // GLES shader passes configure vertex attributes on the default VAO.
  glBindVertexArray(0);
  UpdateShaders();

  if (m_bUseShaderPreset)
  {
    const CFramebufferState framebufferState;
    const CSize destSize = CRenderGeometryFBO::GetShaderOutputSize(
        m_sourceRect, renderBuffer->GetWidth(), renderBuffer->GetHeight(), m_rotatedDestCoords);
    GLint maxTextureSize;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
    const bool validSize = std::isfinite(destSize.Width()) && std::isfinite(destSize.Height()) &&
                           destSize.Width() > 0.0f && destSize.Height() > 0.0f &&
                           destSize.Width() <= maxTextureSize &&
                           destSize.Height() <= maxTextureSize;
    const unsigned int destWidth =
        validSize ? static_cast<unsigned int>(std::ceil(destSize.Width())) : 0;
    const unsigned int destHeight =
        validSize ? static_cast<unsigned int>(std::ceil(destSize.Height())) : 0;

    if (m_shaderTargetTexture &&
        (m_shaderTargetWidth != destWidth || m_shaderTargetHeight != destHeight))
    {
      m_shaderTargetTexture.reset();
    }

    if (!m_shaderTargetTexture && destWidth > 0 && destHeight > 0)
    {
      glActiveTexture(GL_TEXTURE0);
#if defined(HAS_GLES)
      auto targetTexture = std::make_shared<SHADER::CShaderTextureGLES>(
          destWidth, destHeight, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA, false);
#else
      auto targetTexture = std::make_shared<SHADER::CShaderTextureGL>(
          destWidth, destHeight, GL_UNSIGNED_BYTE, GL_RGBA8, GL_BGRA, false);
#endif
      targetTexture->CreateTexture();
      if (targetTexture->BindFBO())
      {
        GLint targetFbo;
        glGetIntegerv(GL_FRAMEBUFFER_BINDING, &targetFbo);
        targetTexture->UnbindFBO();
        if (targetFbo != 0)
        {
          m_shaderTargetTexture = std::move(targetTexture);
          m_shaderTargetWidth = destWidth;
          m_shaderTargetHeight = destHeight;
        }
      }
    }

    if (m_shaderTargetTexture)
    {
#if defined(HAS_GLES)
      SHADER::CShaderTextureGLESRef sourceTexture(
          renderBuffer->GetWidth(), renderBuffer->GetHeight(), renderBuffer->TextureID());
      auto* target = static_cast<SHADER::CShaderTextureGLES*>(m_shaderTargetTexture.get());
#else
      SHADER::CShaderTextureGLRef sourceTexture(renderBuffer->GetWidth(), renderBuffer->GetHeight(),
                                                renderBuffer->TextureID());
      auto* target = static_cast<SHADER::CShaderTextureGL*>(m_shaderTargetTexture.get());
#endif
      const GLint filter =
          m_shaderPreset->GetPasses().front().filterType == SHADER::FilterType::LINEAR ? GL_LINEAR
                                                                                       : GL_NEAREST;
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(m_textureTarget, renderBuffer->TextureID());
      glTexParameteri(m_textureTarget, GL_TEXTURE_MAG_FILTER, filter);
      glTexParameteri(m_textureTarget, GL_TEXTURE_MIN_FILTER, filter);

      const bool bRendered = m_shaderPreset->RenderUpdate(sourceTexture, *target);

      if (bRendered)
      {
        drawTexture = target->GetTextureID();
        bShaded = true;
      }
      else
      {
        CLog::Log(LOGERROR,
                  "RetroPlayer[RENDER]: Video filter failed, drawing the frame unfiltered");
        m_bShadersNeedUpdate = false;
        m_bUseShaderPreset = false;
        DestroyShaderResources();
      }
    }
  }

  if (bShaded)
    rect = CRenderGeometryFBO::GetTextureCoordinates(m_sourceRect, renderBuffer->GetHeight(),
                                                     renderBuffer->GetWidth(),
                                                     renderBuffer->GetHeight(), false);

  if (alpha < 255)
  {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  }
  else
  {
    glDisable(GL_BLEND);
  }

  // Unit 0, because that is where the GUI shader samples from. Binding without
  // selecting it leaves the texture on whichever unit something else last made
  // active, and the shader then reads a unit this renderer never wrote to.
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(m_textureTarget, drawTexture);

  // The vertices below are in screen coordinates, taken from m_rotatedDestCoords,
  // and the GUI shader transforms them with the render context's own matrices.
  // Those matrices are what place and size the picture -- they carry the view
  // mode, zoom, pixel ratio and, for a game rendered into a GUI control, the
  // control's rectangle. Replacing them here with an identity modelview and an
  // Ortho2D spanning the whole viewport would discard all of it, taking scaling
  // and the placement of video filter previews with it.

  GLint filter = GL_NEAREST;
  if (GetRenderSettings().VideoSettings().GetScalingMethod() == SCALINGMETHOD::LINEAR)
    filter = GL_LINEAR;
  glTexParameteri(m_textureTarget, GL_TEXTURE_MAG_FILTER, filter);
  glTexParameteri(m_textureTarget, GL_TEXTURE_MIN_FILTER, filter);
  glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  m_context.EnableGUIShader(GL_SHADER_METHOD::TEXTURE);

  GLubyte colour[4];
  PackedVertex vertex[4];

  GLint uniColLoc = m_context.GUIShaderGetUniCol();
  GLint depthLoc = m_context.GUIShaderGetDepth();

  colour[0] = UTILS::GL::GetChannelFromARGB(UTILS::GL::ColorChannel::R, color);
  colour[1] = UTILS::GL::GetChannelFromARGB(UTILS::GL::ColorChannel::G, color);
  colour[2] = UTILS::GL::GetChannelFromARGB(UTILS::GL::ColorChannel::B, color);
  colour[3] = UTILS::GL::GetChannelFromARGB(UTILS::GL::ColorChannel::A, color);

  for (unsigned int i = 0; i < 4; i++)
  {
    vertex[i].x = m_rotatedDestCoords[i].x;
    vertex[i].y = m_rotatedDestCoords[i].y;
    vertex[i].z = 0.0f;
  }

  vertex[0].u1 = vertex[3].u1 = rect.x1;
  vertex[0].v1 = vertex[1].v1 = rect.y1;
  vertex[1].u1 = vertex[2].u1 = rect.x2;
  vertex[2].v1 = vertex[3].v1 = rect.y2;

  glBindVertexArray(m_mainVAO);
  glBindBuffer(GL_ARRAY_BUFFER, m_mainVertexVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertex), vertex, GL_DYNAMIC_DRAW);

  // The GUI shader positions the quad in depth from this. Leaving it unset
  // draws at whatever the uniform happened to hold.
  glUniform1f(depthLoc, -1.0f);

  glUniform4f(uniColLoc, (colour[0] / 255.0f), (colour[1] / 255.0f), (colour[2] / 255.0f),
              (colour[3] / 255.0f));

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
