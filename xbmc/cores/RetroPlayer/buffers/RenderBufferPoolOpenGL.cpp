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

void CRenderBufferPoolOpenGL::ConfigureHardware(bool depth, bool stencil)
{
  m_hwDepth = depth;
  m_hwStencil = stencil;
}

IRenderBuffer* CRenderBufferPoolOpenGL::CreateRenderBuffer(void* header /* = nullptr */)
{
  // For hardware rendering (AV_PIX_FMT_NONE) the buffer owns an FBO the game
  // core renders into, with the requested depth/stencil attachments.
  const bool hardware = (m_format == AV_PIX_FMT_NONE);

  return new CRenderBufferOpenGL(m_pixelType, m_internalFormat, m_pixelFormat, m_bpp, hardware,
                                 m_hwDepth, m_hwStencil);
}

bool CRenderBufferPoolOpenGL::ConfigureInternal()
{
  // Configure CRenderBufferPoolOpenGL
  switch (m_format)
  {
    case AV_PIX_FMT_NONE:
    {
      // Hardware rendering: the FBO color attachment is an RGBA8 texture
      m_pixelType = GL_UNSIGNED_BYTE;
      m_internalFormat = GL_RGBA8;
      m_pixelFormat = GL_RGBA;
      m_bpp = sizeof(uint32_t);
      return true;
    }
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
