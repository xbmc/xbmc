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

#include "RPRendererOpenGL.h"
#include "cores/RetroPlayer/rendering/RenderContext.h"
#include "utils/log.h"

using namespace KODI;
using namespace RETRO;

// --- CRendererFactoryOpenGL --------------------------------------------------

CRPBaseRenderer *CRendererFactoryOpenGL::CreateRenderer(const CRenderSettings &settings, CRenderContext &context, std::shared_ptr<IRenderBufferPool> bufferPool)
{
  return new CRPRendererOpenGL(settings, context, std::move(bufferPool));
}

RenderBufferPoolVector CRendererFactoryOpenGL::CreateBufferPools()
{
  return { std::make_shared<CRenderBufferPoolOpenGL>() };
}

// --- CRenderBufferOpenGL -----------------------------------------------------

CRenderBufferOpenGL::CRenderBufferOpenGL(AVPixelFormat format, AVPixelFormat targetFormat, unsigned int width, unsigned int height) :
  CRenderBufferOpenGLES(format, targetFormat, width, height)
{
}

void CRenderBufferOpenGL::CreateTexture()
{
  glEnable(m_textureTarget);

  glGenTextures(1, &m_textureId);

  glBindTexture(m_textureTarget, m_textureId);

  glTexImage2D(m_textureTarget, 0, GL_RGBA, m_width, m_height, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);

  glTexParameteri(m_textureTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(m_textureTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  glDisable(m_textureTarget);
}

bool CRenderBufferOpenGL::UploadTexture()
{
  if (m_textureBuffer.empty())
  {
    CLog::Log(LOGERROR, "Renderer: Unknown target texture size");
    return false;
  }

  if (m_targetFormat == AV_PIX_FMT_NONE)
  {
    CLog::Log(LOGERROR, "Renderer: Invalid target pixel format");
    return false;
  }

  if (!CreateScalingContext())
    return false;

  if (!glIsTexture(m_textureId))
    CreateTexture();

  ScalePixels(m_data.data(), m_data.size(), m_textureBuffer.data(), m_textureBuffer.size());

  glEnable(m_textureTarget);

  const unsigned int bpp = 1;
  glPixelStorei(GL_UNPACK_ALIGNMENT, bpp);

  const unsigned datatype = GL_UNSIGNED_BYTE;

  glPixelStorei(GL_UNPACK_ROW_LENGTH, m_width);

  glBindTexture(m_textureTarget, m_textureId);
  glTexSubImage2D(m_textureTarget, 0, 0, 0, m_width, m_height, GL_BGRA, datatype, m_textureBuffer.data());

  glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

  glBindTexture(m_textureTarget, 0);

  glDisable(m_textureTarget);

  return true;
}

// --- CRenderBufferPoolOpenGL -------------------------------------------------

IRenderBuffer *CRenderBufferPoolOpenGL::CreateRenderBuffer(void *header /* = nullptr */)
{
  return new CRenderBufferOpenGL(m_format, m_targetFormat, m_width, m_height);
}

// --- CRPRendererOpenGL -------------------------------------------------------

CRPRendererOpenGL::CRPRendererOpenGL(const CRenderSettings &renderSettings, CRenderContext &context, std::shared_ptr<IRenderBufferPool> bufferPool) :
  CRPRendererOpenGLES(renderSettings, context, std::move(bufferPool))
{
  // Initialize CRPRendererOpenGLES
  m_clearColour = m_context.UseLimitedColor() ? (16.0f / 0xff) : 0.0f;
}

void CRPRendererOpenGL::RenderInternal(bool clear, uint8_t alpha)
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
    glColor4f(1.0f, 1.0f, 1.0f, alpha / 250.f);
  }
  else
  {
    glDisable(GL_BLEND);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
  }

  Render();

  glEnable(GL_BLEND);
  glFlush();
}

void CRPRendererOpenGL::Render()
{
  if (m_renderBuffer == nullptr)
    return;

  CRenderBufferOpenGL *renderBuffer = static_cast<CRenderBufferOpenGL*>(m_renderBuffer);

  glEnable(m_textureTarget);

  glActiveTextureARB(GL_TEXTURE0);

  glBindTexture(m_textureTarget, renderBuffer->TextureID());

  // Try some clamping or wrapping
  glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  GLint filter = (m_renderSettings.VideoSettings().GetScalingMethod() == VS_SCALINGMETHOD_NEAREST ? GL_NEAREST : GL_LINEAR);
  glTexParameteri(m_textureTarget, GL_TEXTURE_MAG_FILTER, filter);
  glTexParameteri(m_textureTarget, GL_TEXTURE_MIN_FILTER, filter);

  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

  glBegin(GL_QUADS);

  CRect rect = m_sourceRect;

  rect.x1 /= m_sourceWidth;
  rect.x2 /= m_sourceWidth;
  rect.y1 /= m_sourceHeight;
  rect.y2 /= m_sourceHeight;

  glTexCoord2f(rect.x1, rect.y1);  glVertex2f(m_rotatedDestCoords[0].x, m_rotatedDestCoords[0].y);
  glTexCoord2f(rect.x2, rect.y1);  glVertex2f(m_rotatedDestCoords[1].x, m_rotatedDestCoords[1].y);
  glTexCoord2f(rect.x2, rect.y2);  glVertex2f(m_rotatedDestCoords[2].x, m_rotatedDestCoords[2].y);
  glTexCoord2f(rect.x1, rect.y2);  glVertex2f(m_rotatedDestCoords[3].x, m_rotatedDestCoords[3].y);

  glEnd();

  glBindTexture(m_textureTarget, 0);
  glDisable(m_textureTarget);
}

void CRPRendererOpenGL::DrawBlackBars()
{
  glColor4f(m_clearColour, m_clearColour, m_clearColour, 1.0f);
  glDisable(GL_BLEND);
  glBegin(GL_QUADS);

  //top quad
  if (m_rotatedDestCoords[0].y > 0.0)
  {
    glVertex4f(0.0,                        0.0,                      0.0, 1.0);
    glVertex4f(m_context.GetScreenWidth(), 0.0,                      0.0, 1.0);
    glVertex4f(m_context.GetScreenWidth(), m_rotatedDestCoords[0].y, 0.0, 1.0);
    glVertex4f(0.0,                        m_rotatedDestCoords[0].y, 0.0, 1.0);
  }

  //bottom quad
  if (m_rotatedDestCoords[2].y < m_context.GetScreenHeight())
  {
    glVertex4f(0.0,                        m_rotatedDestCoords[2].y,    0.0, 1.0);
    glVertex4f(m_context.GetScreenWidth(), m_rotatedDestCoords[2].y,    0.0, 1.0);
    glVertex4f(m_context.GetScreenWidth(), m_context.GetScreenHeight(), 0.0, 1.0);
    glVertex4f(0.0,                        m_context.GetScreenHeight(), 0.0, 1.0);
  }

  //left quad
  if (m_rotatedDestCoords[0].x > 0.0)
  {
    glVertex4f(0.0,                      m_rotatedDestCoords[0].y, 0.0, 1.0);
    glVertex4f(m_rotatedDestCoords[0].x, m_rotatedDestCoords[0].y, 0.0, 1.0);
    glVertex4f(m_rotatedDestCoords[0].x, m_rotatedDestCoords[2].y, 0.0, 1.0);
    glVertex4f(0.0,                      m_rotatedDestCoords[2].y, 0.0, 1.0);
  }

  //right quad
  if (m_rotatedDestCoords[2].x < m_context.GetScreenWidth())
  {
    glVertex4f(m_rotatedDestCoords[2].x,   m_rotatedDestCoords[0].y, 0.0, 1.0);
    glVertex4f(m_context.GetScreenWidth(), m_rotatedDestCoords[0].y, 0.0, 1.0);
    glVertex4f(m_context.GetScreenWidth(), m_rotatedDestCoords[2].y, 0.0, 1.0);
    glVertex4f(m_rotatedDestCoords[2].x,   m_rotatedDestCoords[2].y, 0.0, 1.0);
  }

  glEnd();
}
