/*
 *      Copyright (C) 2017 Team Kodi
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "RPRendererOpenGLES.h"
#include "cores/RetroPlayer/rendering/RenderContext.h"
#include "cores/RetroPlayer/rendering/RenderVideoSettings.h"
#include "utils/GLUtils.h"
#include "utils/log.h"

#include <cstring>

using namespace KODI;
using namespace RETRO;

// --- CRendererFactoryOpenGLES ------------------------------------------------

CRPBaseRenderer *CRendererFactoryOpenGLES::CreateRenderer(const CRenderSettings &settings, CRenderContext &context, std::shared_ptr<IRenderBufferPool> bufferPool)
{
  return new CRPRendererOpenGLES(settings, context, std::move(bufferPool));
}

RenderBufferPoolVector CRendererFactoryOpenGLES::CreateBufferPools()
{
  return { std::make_shared<CRenderBufferPoolOpenGLES>() };
}

// --- CRenderBufferOpenGLES ---------------------------------------------------

CRenderBufferOpenGLES::CRenderBufferOpenGLES(AVPixelFormat format, AVPixelFormat targetFormat, unsigned int width, unsigned int height) :
  m_format(format),
  m_targetFormat(targetFormat),
  m_width(width),
  m_height(height)
{
}

CRenderBufferOpenGLES::~CRenderBufferOpenGLES()
{
  DeleteTexture();
}

void CRenderBufferOpenGLES::CreateTexture()
{
  glGenTextures(1, &m_textureId);

  glBindTexture(m_textureTarget, m_textureId);

  glTexImage2D(m_textureTarget, 0, GL_RGB, m_width, m_height, 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, NULL);

  glTexParameteri(m_textureTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(m_textureTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

bool CRenderBufferOpenGLES::UploadTexture()
{
  if (!glIsTexture(m_textureId))
    CreateTexture();

  glBindTexture(m_textureTarget, m_textureId);

  const int stride = GetFrameSize() / m_height;
  const uint8_t* src = m_data.data();

  for (unsigned int y = 0; y < m_height; ++y, src += stride)
    glTexSubImage2D(m_textureTarget, 0, 0, y, m_width, 1, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, src);

  return true;
}

void CRenderBufferOpenGLES::DeleteTexture()
{
  if (glIsTexture(m_textureId))
    glDeleteTextures(1, &m_textureId);

  m_textureId = 0;
}

// --- CRenderBufferPoolOpenGLES -----------------------------------------------

bool CRenderBufferPoolOpenGLES::IsCompatible(const CRenderVideoSettings &renderSettings) const
{
  if (!CRPRendererOpenGLES::SupportsScalingMethod(renderSettings.GetScalingMethod()))
    return false;

  return true;
}

IRenderBuffer *CRenderBufferPoolOpenGLES::CreateRenderBuffer(void *header /* = nullptr */)
{
  return new CRenderBufferOpenGLES(m_format, m_targetFormat, m_width, m_height);
}

bool CRenderBufferPoolOpenGLES::SetTargetFormat(AVPixelFormat targetFormat)
{
  if (m_targetFormat != AV_PIX_FMT_NONE)
    return false; // Already configured

  m_targetFormat = targetFormat;

  return true;
}

// --- CRPRendererOpenGLES -----------------------------------------------------

CRPRendererOpenGLES::CRPRendererOpenGLES(const CRenderSettings &renderSettings, CRenderContext &context, std::shared_ptr<IRenderBufferPool> bufferPool) :
  CRPBaseRenderer(renderSettings, context, std::move(bufferPool))
{
}

CRPRendererOpenGLES::~CRPRendererOpenGLES()
{
  Deinitialize();
}

bool CRPRendererOpenGLES::ConfigureInternal()
{
  AVPixelFormat targetFormat = AV_PIX_FMT_RGB565LE;

  static_cast<CRenderBufferPoolOpenGLES*>(m_bufferPool.get())->SetTargetFormat(targetFormat);

  return true;
}

void CRPRendererOpenGLES::RenderInternal(bool clear, uint8_t alpha)
{
  if (clear)
  {
    ClearBackBuffer();
  }

  if (alpha < 0xFF)
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
}

void CRPRendererOpenGLES::FlushInternal()
{
  if (!m_bConfigured)
    return;

  glFinish();
}

bool CRPRendererOpenGLES::Supports(ERENDERFEATURE feature) const
{
  if (feature == RENDERFEATURE_STRETCH         ||
      feature == RENDERFEATURE_ZOOM            ||
      feature == RENDERFEATURE_VERTICAL_SHIFT  ||
      feature == RENDERFEATURE_PIXEL_RATIO     ||
      feature == RENDERFEATURE_ROTATION)
  {
    return true;
  }

  return false;
}

bool CRPRendererOpenGLES::SupportsScalingMethod(ESCALINGMETHOD method)
{
  if (method == VS_SCALINGMETHOD_NEAREST ||
      method == VS_SCALINGMETHOD_LINEAR)
  {
    return true;
  }

  return false;
}

void CRPRendererOpenGLES::ClearBackBuffer()
{
  glClearColor(m_clearColour, m_clearColour, m_clearColour, 0.0f);
  glClear(GL_COLOR_BUFFER_BIT);
  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
}

void CRPRendererOpenGLES::Render(uint8_t alpha)
{
  CRenderBufferOpenGLES *renderBuffer = static_cast<CRenderBufferOpenGLES*>(m_renderBuffer);

  if (renderBuffer == nullptr)
    return;

  CRect rect = m_sourceRect;

  rect.x1 /= m_sourceWidth;
  rect.x2 /= m_sourceWidth;
  rect.y1 /= m_sourceHeight;
  rect.y2 /= m_sourceHeight;

  float u1 = rect.x1;
  float u2 = rect.x2;
  float v1 = rect.y1;
  float v2 = rect.y2;

  const uint32_t color = (alpha << 24) | 0xFFFFFF;

  glBindTexture(m_textureTarget, renderBuffer->TextureID());

  GLint filter = GL_NEAREST;
  if (GetRenderSettings().VideoSettings().GetScalingMethod() == VS_SCALINGMETHOD_LINEAR)
    filter = GL_LINEAR;
  glTexParameteri(m_textureTarget, GL_TEXTURE_MAG_FILTER, filter);
  glTexParameteri(m_textureTarget, GL_TEXTURE_MIN_FILTER, filter);
  glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  m_context.EnableGUIShader(GL_SHADER_METHOD::TEXTURE);

  GLubyte colour[4];
  GLfloat ver[4][3];
  GLfloat tex[4][2];
  GLubyte idx[4] = {0, 1, 3, 2}; // Determines order of triangle strip

  GLint posLoc = m_context.GUIShaderGetPos();
  GLint tex0Loc = m_context.GUIShaderGetCoord0();
  GLint uniColLoc = m_context.GUIShaderGetUniCol();

  glVertexAttribPointer(posLoc, 3, GL_FLOAT, 0, 0, ver);
  glVertexAttribPointer(tex0Loc, 2, GL_FLOAT, 0, 0, tex);

  glEnableVertexAttribArray(posLoc);
  glEnableVertexAttribArray(tex0Loc);

  // Setup color values
  colour[0] = static_cast<GLubyte>(GET_R(color));
  colour[1] = static_cast<GLubyte>(GET_G(color));
  colour[2] = static_cast<GLubyte>(GET_B(color));
  colour[3] = static_cast<GLubyte>(GET_A(color));

  for (unsigned int i = 0; i < 4; i++)
  {
    // Setup vertex position values
    ver[i][0] = m_rotatedDestCoords[i].x;
    ver[i][1] = m_rotatedDestCoords[i].y;
    ver[i][2] = 0.0f;
  }

  // Setup texture coordinates
  tex[0][0] = tex[3][0] = u1;
  tex[0][1] = tex[1][1] = v1;
  tex[1][0] = tex[2][0] = u2;
  tex[2][1] = tex[3][1] = v2;

  glUniform4f(uniColLoc,(colour[0] / 255.0f), (colour[1] / 255.0f), (colour[2] / 255.0f), (colour[3] / 255.0f));
  glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_BYTE, idx);

  glDisableVertexAttribArray(posLoc);
  glDisableVertexAttribArray(tex0Loc);

  m_context.DisableGUIShader();
}
