/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RenderBufferOpenGLES.h"

#include "cores/RetroPlayer/rendering/RenderContext.h"
#include "utils/log.h"

using namespace KODI;
using namespace RETRO;

CRenderBufferOpenGLES::CRenderBufferOpenGLES(CRenderContext& context,
                                             GLuint pixelType,
                                             GLuint internalFormat,
                                             GLuint pixelFormat,
                                             GLuint bpp,
                                             bool hardware,
                                             bool depth,
                                             bool stencil)
  : m_context(context),
    m_pixelType(pixelType),
    m_internalFormat(internalFormat),
    m_pixelFormat(pixelFormat),
    m_bpp(bpp),
    m_bHardware(hardware),
    m_depth(depth),
    m_stencil(stencil)
{
}

CRenderBufferOpenGLES::~CRenderBufferOpenGLES()
{
  DeleteFramebuffer();
  DeleteTexture();
}

bool CRenderBufferOpenGLES::Allocate(AVPixelFormat format,
                                     unsigned int width,
                                     unsigned int height)
{
  // Hardware rendering: the game core renders directly into a Kodi-owned FBO,
  // signalled by AV_PIX_FMT_NONE. No system memory is allocated in this case.
  if (m_bHardware || format == AV_PIX_FMT_NONE)
  {
    m_format = AV_PIX_FMT_NONE;
    m_width = width;
    m_height = height;

    return CreateFramebuffer();
  }

  // Software rendering: fall back to the system-memory upload path.
  return CRenderBufferSysMem::Allocate(format, width, height);
}

void CRenderBufferOpenGLES::CreateTexture()
{
  glGenTextures(1, &m_textureId);

  glBindTexture(m_textureTarget, m_textureId);
  glTexParameteri(m_textureTarget, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(m_textureTarget, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  // Force alpha to 1, because game client can leave it undefined
#if defined(GL_ES_VERSION_3_0)
  if (m_internalFormat == GL_RGBA || m_internalFormat == GL_BGRA_EXT)
    glTexParameteri(m_textureTarget, GL_TEXTURE_SWIZZLE_A, GL_ONE);
#endif

  glTexImage2D(m_textureTarget, 0, m_internalFormat, m_width, m_height, 0, m_pixelFormat,
               m_pixelType, nullptr);

  glBindTexture(m_textureTarget, 0);
}

bool CRenderBufferOpenGLES::UploadTexture()
{
  // Hardware-rendered buffers already contain the frame in the FBO's color
  // texture; there is nothing to upload from system memory.
  if (m_bHardware)
    return true;

  if (!glIsTexture(m_textureId))
    CreateTexture();

  glBindTexture(m_textureTarget, m_textureId);

  const int stride = GetFrameSize() / m_height;

  glPixelStorei(GL_UNPACK_ALIGNMENT, m_bpp);

  if (m_bpp == 4 && m_pixelFormat == GL_RGBA)
  {
    // XOR Swap RGBA -> BGRA
    // GLES 2.0 doesn't support strided textures (unless GL_UNPACK_ROW_LENGTH_EXT is supported)
    uint8_t* pixels = const_cast<uint8_t*>(m_data.data());
    for (unsigned int y = 0; y < m_height; ++y, pixels += stride)
    {
      for (int x = 0; x < stride; x += 4)
        std::swap(pixels[x], pixels[x + 2]);
      glTexSubImage2D(m_textureTarget, 0, 0, y, m_width, 1, m_pixelFormat, m_pixelType, pixels);
    }
  }
  else if (m_context.IsExtSupported("GL_EXT_unpack_subimage"))
  {
#ifdef GL_UNPACK_ROW_LENGTH_EXT
    glPixelStorei(GL_UNPACK_ROW_LENGTH_EXT, stride / m_bpp);
    glTexSubImage2D(m_textureTarget, 0, 0, 0, m_width, m_height, m_pixelFormat, m_pixelType,
                    m_data.data());
    glPixelStorei(GL_UNPACK_ROW_LENGTH_EXT, 0);
#endif
  }
  else
  {
    uint8_t* pixels = const_cast<uint8_t*>(m_data.data());
    for (unsigned int y = 0; y < m_height; ++y, pixels += stride)
      glTexSubImage2D(m_textureTarget, 0, 0, y, m_width, 1, m_pixelFormat, m_pixelType, pixels);
  }

  glBindTexture(m_textureTarget, 0);

  return true;
}

void CRenderBufferOpenGLES::DeleteTexture()
{
  if (glIsTexture(m_textureId))
    glDeleteTextures(1, &m_textureId);

  m_textureId = 0;
}

bool CRenderBufferOpenGLES::CreateFramebuffer()
{
  // Create the color attachment (the texture Kodi presents and the core renders into)
  CreateTexture();

  glGenFramebuffers(1, &m_fboId);
  glBindFramebuffer(GL_FRAMEBUFFER, m_fboId);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_textureTarget, m_textureId, 0);

  // Attach a depth/stencil renderbuffer when the core requests either. 3D cores
  // (e.g. Mupen64Plus, Beetle PSX HW) need a depth buffer for correct occlusion.
  if (m_depth || m_stencil)
  {
    glGenRenderbuffers(1, &m_depthStencilRbo);
    glBindRenderbuffer(GL_RENDERBUFFER, m_depthStencilRbo);
#if defined(GL_ES_VERSION_3_0)
    // GLES 3.0+: packed depth24/stencil8 covers both requests
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, m_width, m_height);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER,
                              m_depthStencilRbo);
#else
    // GLES 2.0 fallback: a 16-bit depth renderbuffer (stencil omitted)
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, m_width, m_height);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER,
                              m_depthStencilRbo);
#endif
  }

  const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

  // Restore the default framebuffer so we don't leave the FBO bound
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  if (status != GL_FRAMEBUFFER_COMPLETE)
  {
    CLog::Log(LOGERROR,
              "RetroPlayer[RENDER]: Hardware framebuffer {}x{} incomplete (status 0x{:x})", m_width,
              m_height, static_cast<unsigned int>(status));
    return false;
  }

  CLog::Log(LOGDEBUG, "RetroPlayer[RENDER]: Created hardware framebuffer {} ({}x{}, depth={}, stencil={})",
            m_fboId, m_width, m_height, m_depth, m_stencil);
  return true;
}

void CRenderBufferOpenGLES::DeleteFramebuffer()
{
  if (m_depthStencilRbo != 0)
  {
    glDeleteRenderbuffers(1, &m_depthStencilRbo);
    m_depthStencilRbo = 0;
  }

  if (m_fboId != 0)
  {
    glDeleteFramebuffers(1, &m_fboId);
    m_fboId = 0;
  }
}
