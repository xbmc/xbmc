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
#include "cores/RetroPlayer/rendering/RenderVideoSettings.h"
#include "utils/GLUtils.h"
#include "utils/log.h"

#include <cstring>
#include <stddef.h>

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
  return {std::make_shared<CRenderBufferPoolOpenGLES>(context)};
}

// --- CRPRendererOpenGLES -----------------------------------------------------

CRPRendererOpenGLES::CRPRendererOpenGLES(const CRenderSettings& renderSettings,
                                         CRenderContext& context,
                                         std::shared_ptr<IRenderBufferPool> bufferPool)
  : CRPBaseRenderer(renderSettings, context, std::move(bufferPool))
{
  glGenBuffers(1, &m_mainIndexVBO);
  glGenBuffers(1, &m_mainVertexVBO);
  glGenBuffers(1, &m_blackbarsVertexVBO);
}

CRPRendererOpenGLES::~CRPRendererOpenGLES()
{
  glDeleteBuffers(1, &m_mainIndexVBO);
  glDeleteBuffers(1, &m_mainVertexVBO);
  glDeleteBuffers(1, &m_blackbarsVertexVBO);
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

  if (alpha < 255)
  {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  }
  else
  {
    glDisable(GL_BLEND);
  }

  Render(alpha);

  glEnable(GL_BLEND);
  glFlush();
}

void CRPRendererOpenGLES::FlushInternal()
{
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
  return method == SCALINGMETHOD::NEAREST || method == SCALINGMETHOD::LINEAR;
}

void CRPRendererOpenGLES::ClearBackBuffer()
{
  glClearColor(m_clearColour, m_clearColour, m_clearColour, 0.0f);
  glClear(GL_COLOR_BUFFER_BIT);
  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
}

void CRPRendererOpenGLES::DrawBlackBars()
{
  glDisable(GL_BLEND);

  struct Svertex
  {
    float x;
    float y;
    float z;
  };
  Svertex vertices[24];
  GLubyte count = 0;

  m_context.EnableGUIShader(GL_SHADER_METHOD::DEFAULT);
  GLint posLoc = m_context.GUIShaderGetPos();
  GLint uniCol = m_context.GUIShaderGetUniCol();
  GLint depthLoc = m_context.GUIShaderGetDepth();

  glUniform4f(uniCol, m_clearColour / 255.0f, m_clearColour / 255.0f, m_clearColour / 255.0f, 1.0f);
  glUniform1f(depthLoc, -1.0f);

  // top quad
  if (m_rotatedDestCoords[0].y > 0.0f)
  {
    GLubyte quad = count;
    vertices[quad].x = 0.0;
    vertices[quad].y = 0.0;
    vertices[quad].z = 0;
    vertices[quad + 1].x = m_context.GetScreenWidth();
    vertices[quad + 1].y = 0;
    vertices[quad + 1].z = 0;
    vertices[quad + 2].x = m_context.GetScreenWidth();
    vertices[quad + 2].y = m_rotatedDestCoords[0].y;
    vertices[quad + 2].z = 0;
    vertices[quad + 3] = vertices[quad + 2];
    vertices[quad + 4].x = 0;
    vertices[quad + 4].y = m_rotatedDestCoords[0].y;
    vertices[quad + 4].z = 0;
    vertices[quad + 5] = vertices[quad];
    count += 6;
  }

  // bottom quad
  if (m_rotatedDestCoords[2].y < m_context.GetScreenHeight())
  {
    GLubyte quad = count;
    vertices[quad].x = 0.0;
    vertices[quad].y = m_rotatedDestCoords[2].y;
    vertices[quad].z = 0;
    vertices[quad + 1].x = m_context.GetScreenWidth();
    vertices[quad + 1].y = m_rotatedDestCoords[2].y;
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

  // left quad
  if (m_rotatedDestCoords[0].x > 0.0f)
  {
    GLubyte quad = count;
    vertices[quad].x = 0.0;
    vertices[quad].y = m_rotatedDestCoords[0].y;
    vertices[quad].z = 0;
    vertices[quad + 1].x = m_rotatedDestCoords[0].x;
    vertices[quad + 1].y = m_rotatedDestCoords[0].y;
    vertices[quad + 1].z = 0;
    vertices[quad + 2].x = m_rotatedDestCoords[3].x;
    vertices[quad + 2].y = m_rotatedDestCoords[3].y;
    vertices[quad + 2].z = 0;
    vertices[quad + 3] = vertices[quad + 2];
    vertices[quad + 4].x = 0;
    vertices[quad + 4].y = m_rotatedDestCoords[3].y;
    vertices[quad + 4].z = 0;
    vertices[quad + 5] = vertices[quad];
    count += 6;
  }

  // right quad
  if (m_rotatedDestCoords[2].x < m_context.GetScreenWidth())
  {
    GLubyte quad = count;
    vertices[quad].x = m_rotatedDestCoords[1].x;
    vertices[quad].y = m_rotatedDestCoords[1].y;
    vertices[quad].z = 0;
    vertices[quad + 1].x = m_context.GetScreenWidth();
    vertices[quad + 1].y = m_rotatedDestCoords[1].y;
    vertices[quad + 1].z = 0;
    vertices[quad + 2].x = m_context.GetScreenWidth();
    vertices[quad + 2].y = m_rotatedDestCoords[2].y;
    vertices[quad + 2].z = 0;
    vertices[quad + 3] = vertices[quad + 2];
    vertices[quad + 4].x = m_rotatedDestCoords[1].x;
    vertices[quad + 4].y = m_rotatedDestCoords[2].y;
    vertices[quad + 4].z = 0;
    vertices[quad + 5] = vertices[quad];
    count += 6;
  }

  glBindBuffer(GL_ARRAY_BUFFER, m_blackbarsVertexVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(Svertex) * count, &vertices[0], GL_STATIC_DRAW);

  glVertexAttribPointer(posLoc, 3, GL_FLOAT, GL_FALSE, sizeof(Svertex), 0);
  glEnableVertexAttribArray(posLoc);

  glDrawArrays(GL_TRIANGLES, 0, count);

  glDisableVertexAttribArray(posLoc);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  m_context.DisableGUIShader();
}

void CRPRendererOpenGLES::Render(uint8_t alpha)
{
  CRenderBufferOpenGLES* renderBuffer = static_cast<CRenderBufferOpenGLES*>(m_renderBuffer);

  if (renderBuffer == nullptr)
    return;

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

  m_context.EnableGUIShader(GL_SHADER_METHOD::TEXTURE_NOALPHA);

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
  GLint depthLoc = m_context.GUIShaderGetDepth();

  // Setup color values
  colour[0] = UTILS::GL::GetChannelFromARGB(UTILS::GL::ColorChannel::R, color);
  colour[1] = UTILS::GL::GetChannelFromARGB(UTILS::GL::ColorChannel::G, color);
  colour[2] = UTILS::GL::GetChannelFromARGB(UTILS::GL::ColorChannel::B, color);
  colour[3] = UTILS::GL::GetChannelFromARGB(UTILS::GL::ColorChannel::A, color);

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
  glBufferData(GL_ARRAY_BUFFER, sizeof(PackedVertex) * 4, &vertex[0], GL_STATIC_DRAW);

  glVertexAttribPointer(vertLoc, 3, GL_FLOAT, 0, sizeof(PackedVertex),
                        reinterpret_cast<const GLvoid*>(offsetof(PackedVertex, x)));
  glVertexAttribPointer(loc, 2, GL_FLOAT, 0, sizeof(PackedVertex),
                        reinterpret_cast<const GLvoid*>(offsetof(PackedVertex, u1)));

  glEnableVertexAttribArray(vertLoc);
  glEnableVertexAttribArray(loc);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_mainIndexVBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLubyte) * 4, idx, GL_STATIC_DRAW);

  glUniform4f(uniColLoc, (colour[0] / 255.0f), (colour[1] / 255.0f), (colour[2] / 255.0f),
              (colour[3] / 255.0f));
  glUniform1f(depthLoc, -1.0f);
  glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_BYTE, 0);

  glDisableVertexAttribArray(vertLoc);
  glDisableVertexAttribArray(loc);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

  m_context.DisableGUIShader();
}
