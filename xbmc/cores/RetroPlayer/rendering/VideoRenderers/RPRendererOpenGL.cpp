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
