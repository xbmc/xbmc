/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RenderBufferOpenGL.h"

#include "cores/RetroPlayer/rendering/RenderContext.h"
#include "utils/log.h"

using namespace KODI;
using namespace RETRO;

CRenderBufferOpenGL::CRenderBufferOpenGL(GLuint pixelType,
                                         GLuint internalFormat,
                                         GLuint pixelFormat,
                                         GLuint bpp,
                                         bool hardware,
                                         bool depth,
                                         bool stencil)
  : m_pixelType(pixelType),
    m_internalFormat(internalFormat),
    m_pixelFormat(pixelFormat),
    m_bpp(bpp),
    m_bHardware(hardware),
    m_depth(depth),
    m_stencil(stencil)
{
}

CRenderBufferOpenGL::~CRenderBufferOpenGL()
{
  DeleteFramebuffer();
  DeleteTexture();
}

bool CRenderBufferOpenGL::Allocate(AVPixelFormat format,
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

void CRenderBufferOpenGL::CreateTexture()
{
  glGenTextures(1, &m_textureId);

  glBindTexture(m_textureTarget, m_textureId);
  glTexParameteri(m_textureTarget, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(m_textureTarget, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  // Force alpha to 1, because game client can leave it undefined
  if (m_internalFormat == GL_RGBA8)
    glTexParameteri(m_textureTarget, GL_TEXTURE_SWIZZLE_A, GL_ONE);

  glTexImage2D(m_textureTarget, 0, m_internalFormat, m_width, m_height, 0, m_pixelFormat,
               m_pixelType, nullptr);

  glBindTexture(m_textureTarget, 0);
}

bool CRenderBufferOpenGL::UploadTexture()
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

  glPixelStorei(GL_UNPACK_ROW_LENGTH, stride / m_bpp);

  //! @todo This is subject to change:
  //! We want to use PBO's instead of glTexSubImage2D!
  //! This code has been borrowed from OpenGL ES in order
  //! to remove GL dependencies on GLES.
  glTexSubImage2D(m_textureTarget, 0, 0, 0, m_width, m_height, m_pixelFormat, m_pixelType,
                  m_data.data());

  glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
  return true;
}

void CRenderBufferOpenGL::DeleteTexture()
{
  if (glIsTexture(m_textureId))
    glDeleteTextures(1, &m_textureId);

  m_textureId = 0;
}

bool CRenderBufferOpenGL::CreateFramebuffer()
{
  // Create the color attachment (the texture Kodi presents and the core renders into)
  CreateTexture();

  glGenFramebuffers(1, &m_fboId);
  glBindFramebuffer(GL_FRAMEBUFFER, m_fboId);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_textureTarget, m_textureId, 0);

  // Attach a combined depth/stencil renderbuffer when the core requests either.
  // 3D cores (e.g. Mupen64Plus, Beetle PSX HW) need a depth buffer for correct
  // occlusion. A packed depth24/stencil8 renderbuffer covers both requests.
  if (m_depth || m_stencil)
  {
    glGenRenderbuffers(1, &m_depthStencilRbo);
    glBindRenderbuffer(GL_RENDERBUFFER, m_depthStencilRbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, m_width, m_height);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER,
                              m_depthStencilRbo);
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

void CRenderBufferOpenGL::DeleteFramebuffer()
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
