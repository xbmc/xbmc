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

extern "C" {
#include "libswscale/swscale.h"
}

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
  size_t targetSize = m_width * m_height * glFormatElementByteCount(GL_RGBA); //! @todo: Get GLenum format from renderer
  m_textureBuffer.resize(targetSize);
}

CRenderBufferOpenGLES::~CRenderBufferOpenGLES()
{
  DeleteTexture();

  if (m_swsContext != nullptr)
    sws_freeContext(m_swsContext);
}

void CRenderBufferOpenGLES::CreateTexture()
{
  glEnable(m_textureTarget);

  glGenTextures(1, &m_textureId);

  glBindTexture(m_textureTarget, m_textureId);

  glTexImage2D(m_textureTarget, 0, GL_LUMINANCE, m_width, m_height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, NULL);

  glTexParameteri(m_textureTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(m_textureTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  glDisable(m_textureTarget);
}

bool CRenderBufferOpenGLES::UploadTexture()
{
  if (m_textureBuffer.empty())
  {
    CLog::Log(LOGERROR, "Renderer: Unknown target texture size");
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

  glBindTexture(m_textureTarget, m_textureId);
  glTexSubImage2D(m_textureTarget, 0, 0, 0, m_width, m_height, GL_LUMINANCE, datatype, m_textureBuffer.data());

  glBindTexture(m_textureTarget, 0);

  glDisable(m_textureTarget);

  return true;
}

void CRenderBufferOpenGLES::DeleteTexture()
{
  if (glIsTexture(m_textureId))
    glDeleteTextures(1, &m_textureId);

  m_textureId = 0;
}

bool CRenderBufferOpenGLES::CreateScalingContext()
{
  m_swsContext = sws_getContext(m_width, m_height, m_format, m_width, m_height, m_targetFormat,
    SWS_FAST_BILINEAR, NULL, NULL, NULL);

  if (m_swsContext == nullptr)
  {
    CLog::Log(LOGERROR, "Renderer: Failed to create swscale context");
    return false;
  }

  return true;
}

void CRenderBufferOpenGLES::ScalePixels(uint8_t *source, size_t sourceSize, uint8_t *target, size_t targetSize)
{
  const int sourceStride = sourceSize / m_height;
  const int targetStride = targetSize / m_height;

  uint8_t* src[] =       { source,        nullptr,   nullptr,   nullptr };
  int      srcStride[] = { sourceStride,  0,         0,         0       };
  uint8_t *dst[] =       { target,        nullptr,   nullptr,   nullptr };
  int      dstStride[] = { targetStride,  0,         0,         0       };

  sws_scale(m_swsContext, src, srcStride, 0, m_height, dst, dstStride);
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
  if (m_swsContext)
    sws_freeContext(m_swsContext);
}

bool CRPRendererOpenGLES::ConfigureInternal()
{
  AVPixelFormat targetFormat = AV_PIX_FMT_BGRA; //! @todo

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
  if (m_renderBuffer == nullptr)
    return;

  CRenderBufferOpenGLES *renderBuffer = static_cast<CRenderBufferOpenGLES*>(m_renderBuffer);

  glDisable(GL_DEPTH_TEST);

  // Render texture
  glActiveTexture(GL_TEXTURE0);
  glEnable(m_textureTarget);
  glBindTexture(m_textureTarget, renderBuffer->TextureID());

  // Try some clamping or wrapping
  glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  GLint filter = (m_renderSettings.VideoSettings().GetScalingMethod() == VS_SCALINGMETHOD_NEAREST ? GL_NEAREST : GL_LINEAR);
  glTexParameteri(m_textureTarget, GL_TEXTURE_MAG_FILTER, filter);
  glTexParameteri(m_textureTarget, GL_TEXTURE_MIN_FILTER, filter);

  //! @todo Translate OpenGL code to GLES
  /*
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
  */

  glBindTexture (m_textureTarget, 0);
  glDisable(m_textureTarget);
}
