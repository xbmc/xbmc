/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RenderBufferPoolOpenGL.h"

#include "RenderBufferOpenGL.h"
#include "cores/RetroPlayer/rendering/RenderContext.h"
#include "cores/RetroPlayer/rendering/RenderVideoSettings.h"
#include "cores/RetroPlayer/rendering/VideoRenderers/RPRendererOpenGL.h"
#include "utils/GLUtils.h"

using namespace KODI;
using namespace RETRO;

bool CRenderBufferPoolOpenGL::IsCompatible(const CRenderVideoSettings& renderSettings) const
{
  return CRPRendererOpenGL::SupportsScalingMethod(renderSettings.GetScalingMethod());
}

IRenderBuffer* CRenderBufferPoolOpenGL::CreateRenderBuffer(void* header /* = nullptr */)
{
  return new CRenderBufferOpenGL(m_pixelType, m_internalFormat, m_pixelFormat, m_bpp);
}

bool CRenderBufferPoolOpenGL::ConfigureInternal()
{
  // Configure CRenderBufferPoolOpenGL
  switch (m_format)
  {
    case AV_PIX_FMT_0RGB32:
    {
      m_pixelType = GL_UNSIGNED_BYTE;
      m_internalFormat = GL_RGBA8;
      m_pixelFormat = GL_BGRA;
      m_bpp = sizeof(uint32_t);
      return true;
    }
    case AV_PIX_FMT_RGB555:
    {
      m_pixelType = GL_UNSIGNED_SHORT_5_5_5_1;
      m_internalFormat = GL_RGB;
      m_pixelFormat = GL_RGB;
      m_bpp = sizeof(uint16_t);
      return true;
    }
    case AV_PIX_FMT_RGB565:
    {
      m_pixelType = GL_UNSIGNED_SHORT_5_6_5;
      m_internalFormat = GL_RGB565;
      m_pixelFormat = GL_RGB;
      m_bpp = sizeof(uint16_t);
      return true;
    }
    default:
      break;
  }

  return false;
}
