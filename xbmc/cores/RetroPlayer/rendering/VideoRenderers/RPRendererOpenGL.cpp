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

std::string CRendererFactoryOpenGL::RenderSystemName() const
{
  return "OpenGL";
}

CRPBaseRenderer *CRendererFactoryOpenGL::CreateRenderer(const CRenderSettings &settings, CRenderContext &context, std::shared_ptr<IRenderBufferPool> bufferPool)
{
  return new CRPRendererOpenGL(settings, context, std::move(bufferPool));
}

RenderBufferPoolVector CRendererFactoryOpenGL::CreateBufferPools(CRenderContext &context)
{
  return { std::make_shared<CRenderBufferPoolOpenGL>(context) };
}

// --- CRenderBufferOpenGL -----------------------------------------------------

CRenderBufferOpenGL::CRenderBufferOpenGL(CRenderContext &context,
                                         GLuint pixeltype,
                                         GLuint internalformat,
                                         GLuint pixelformat,
                                         GLuint bpp) :
  CRenderBufferOpenGLES(context,
                        pixeltype,
                        internalformat,
                        pixelformat,
                        bpp)
{
}

bool CRenderBufferOpenGL::UploadTexture()
{
  if (!glIsTexture(m_textureId))
    CreateTexture();

  glBindTexture(m_textureTarget, m_textureId);

  const int stride = GetFrameSize() / m_height;

  glPixelStorei(GL_UNPACK_ALIGNMENT, m_bpp);

  glPixelStorei(GL_UNPACK_ROW_LENGTH, stride / m_bpp);
  glTexSubImage2D(m_textureTarget, 0, 0, 0, m_width, m_height, m_pixelformat, m_pixeltype, m_data.data());
  glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

  return true;
}

// --- CRenderBufferPoolOpenGL -------------------------------------------------

CRenderBufferPoolOpenGL::CRenderBufferPoolOpenGL(CRenderContext &context)
  : CRenderBufferPoolOpenGLES(context)
{
}

IRenderBuffer *CRenderBufferPoolOpenGL::CreateRenderBuffer(void *header /* = nullptr */)
{
  return new CRenderBufferOpenGL(m_context,
                                 m_pixeltype,
                                 m_internalformat,
                                 m_pixelformat,
                                 m_bpp);
}

bool CRenderBufferPoolOpenGL::ConfigureInternal()
{
  // Configure CRenderBufferPoolOpenGLES
  switch (m_format)
  {
  case AV_PIX_FMT_0RGB32:
  {
    m_pixeltype = GL_UNSIGNED_BYTE;
    m_internalformat = GL_RGBA;
    m_pixelformat = GL_BGRA;
    m_bpp = sizeof(uint32_t);
    return true;
  }
  case AV_PIX_FMT_RGB555:
  {
    m_pixeltype = GL_UNSIGNED_SHORT_5_5_5_1;
    m_internalformat = GL_RGB;
    m_pixelformat = GL_RGB;
    m_bpp = sizeof(uint16_t);
    return true;
  }
  case AV_PIX_FMT_RGB565:
  {
    m_pixeltype = GL_UNSIGNED_SHORT_5_6_5;
    m_internalformat = GL_RGB;
    m_pixelformat = GL_RGB;
    m_bpp = sizeof(uint16_t);
    return true;
  }
  default:
    break;
  }

  return false;
}

// --- CRPRendererOpenGL -------------------------------------------------------

CRPRendererOpenGL::CRPRendererOpenGL(const CRenderSettings &renderSettings, CRenderContext &context, std::shared_ptr<IRenderBufferPool> bufferPool) :
  CRPRendererOpenGLES(renderSettings, context, std::move(bufferPool))
{
  // Initialize CRPRendererOpenGLES
  m_clearColour = m_context.UseLimitedColor() ? (16.0f / 0xff) : 0.0f;
}
