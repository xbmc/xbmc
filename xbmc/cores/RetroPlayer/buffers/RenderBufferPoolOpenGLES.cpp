/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RenderBufferPoolOpenGLES.h"

#include "RenderBufferOpenGLES.h"
#include "cores/RetroPlayer/rendering/RenderContext.h"
#include "cores/RetroPlayer/rendering/RenderVideoSettings.h"
#include "cores/RetroPlayer/rendering/VideoRenderers/RPRendererOpenGLES.h"
#include "utils/GLUtils.h"

using namespace KODI;
using namespace RETRO;

CRenderBufferPoolOpenGLES::CRenderBufferPoolOpenGLES(CRenderContext &context)
  : m_context(context)
{
}

bool CRenderBufferPoolOpenGLES::IsCompatible(const CRenderVideoSettings &renderSettings) const
{
  return CRPRendererOpenGLES::SupportsScalingMethod(renderSettings.GetScalingMethod());
}

IRenderBuffer *CRenderBufferPoolOpenGLES::CreateRenderBuffer(void *header /* = nullptr */)
{
  return new CRenderBufferOpenGLES(m_context,
                                   m_pixeltype,
                                   m_internalformat,
                                   m_pixelformat,
                                   m_bpp);
}

bool CRenderBufferPoolOpenGLES::ConfigureInternal()
{
  switch (m_format)
  {
  case AV_PIX_FMT_0RGB32:
  {
    m_pixeltype = GL_UNSIGNED_BYTE;
    if (m_context.IsExtSupported("GL_EXT_texture_format_BGRA8888") ||
        m_context.IsExtSupported("GL_IMG_texture_format_BGRA8888"))
    {
      m_internalformat = GL_BGRA_EXT;
      m_pixelformat = GL_BGRA_EXT;
    }
    else if (m_context.IsExtSupported("GL_APPLE_texture_format_BGRA8888"))
    {
      // Apple's implementation does not conform to spec. Instead, they require
      // differing format/internalformat, more like GL.
      m_internalformat = GL_RGBA;
      m_pixelformat = GL_BGRA_EXT;
    }
    else
    {
      m_internalformat = GL_RGBA;
      m_pixelformat = GL_RGBA;
    }
    m_bpp = sizeof(uint32_t);
    return true;
  }
  case AV_PIX_FMT_RGB555:
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
