/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FrameBufferObject.h"

#include "ServiceBroker.h"
#include "rendering/GLExtensions.h"
#include "rendering/RenderSystem.h"
#include "utils/GLUtils.h"
#include "utils/log.h"

//////////////////////////////////////////////////////////////////////
// CFrameBufferObject
//////////////////////////////////////////////////////////////////////

CFrameBufferObject::CFrameBufferObject()
{
  m_valid = false;
  m_supported = false;
  m_bound = false;
}

bool CFrameBufferObject::IsSupported()
{
  if (CGLExtensions::IsExtensionSupported(CGLExtensions::EXT_framebuffer_object))
    m_supported = true;
  else
    m_supported = false;
  return m_supported;
}

bool CFrameBufferObject::Initialize()
{
  if (!IsSupported())
    return false;

  Cleanup();

  glGenFramebuffers(1, &m_fbo);
  VerifyGLState();

  if (!m_fbo)
    return false;

  m_valid = true;
  return true;
}

void CFrameBufferObject::Cleanup()
{
  if (!IsValid())
    return;

  if (m_fbo)
    glDeleteFramebuffers(1, &m_fbo);

  if (m_texid)
    glDeleteTextures(1, &m_texid);

  if (m_depthBuffer)
    glDeleteRenderbuffers(1, &m_depthBuffer);

  if (m_stencilBuffer)
    glDeleteRenderbuffers(1, &m_stencilBuffer);

  m_depthBuffer = 0;
  m_stencilBuffer = 0;
  m_texid = 0;
  m_fbo = 0;
  m_valid = false;
  m_bound = false;
}

bool CFrameBufferObject::CreateAndBindToTexture(GLenum target, int width, int height, GLenum format, GLenum type,
                                                GLenum filter, GLenum clampmode)
{
  if (!IsValid())
    return false;

  if (m_texid)
    glDeleteTextures(1, &m_texid);

  glGenTextures(1, &m_texid);
  glBindTexture(target, m_texid);
  glTexImage2D(target, 0, format,  width, height, 0, GL_RGBA, type, NULL);
  glTexParameteri(target, GL_TEXTURE_WRAP_S, clampmode);
  glTexParameteri(target, GL_TEXTURE_WRAP_T, clampmode);
  glTexParameteri(target, GL_TEXTURE_MAG_FILTER, filter);
  glTexParameteri(target, GL_TEXTURE_MIN_FILTER, filter);
  VerifyGLState();

  m_bound = false;
  glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
  glBindTexture(target, m_texid);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, target, m_texid, 0);
  VerifyGLState();
  GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  if (status != GL_FRAMEBUFFER_COMPLETE)
  {
    VerifyGLState();
    return false;
  }
  m_bound = true;
  return true;
}

bool CFrameBufferObject::AttachDepthBuffer(int width, int height)
{
  if (!IsValid() || !IsBound())
    return false;

  if (m_depthBuffer)
    glDeleteRenderbuffers(1, &m_depthBuffer);

  if (m_stencilBuffer)
    glDeleteRenderbuffers(1, &m_stencilBuffer);
  m_stencilBuffer = 0;

  glGenRenderbuffers(1, &m_depthBuffer);
  glBindRenderbuffer(GL_RENDERBUFFER, m_depthBuffer);
#if defined(HAS_GL)
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
#else
  auto* renderSystem = CServiceBroker::GetRenderSystem();
  unsigned int major, minor;
  renderSystem->GetRenderVersion(major, minor);
  const bool packedDepthStencil =
      major >= 3 || renderSystem->IsExtSupported("GL_OES_packed_depth_stencil");
  if (packedDepthStencil)
  {
#if HAS_GLES >= 3
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
#else
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8_OES, width, height);
#endif
  }
  else
  {
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, width, height);
    glGenRenderbuffers(1, &m_stencilBuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, m_stencilBuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_STENCIL_INDEX8, width, height);
  }
#endif
  glBindRenderbuffer(GL_RENDERBUFFER, 0);

  glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_depthBuffer);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER,
                            m_stencilBuffer ? m_stencilBuffer : m_depthBuffer);
  GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

  if (status != GL_FRAMEBUFFER_COMPLETE)
  {
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, 0);
    glDeleteRenderbuffers(1, &m_depthBuffer);
    m_depthBuffer = 0;
    if (m_stencilBuffer)
      glDeleteRenderbuffers(1, &m_stencilBuffer);
    m_stencilBuffer = 0;
#if defined(HAS_GLES)
    CLog::Log(
        LOGWARNING,
        "CFrameBufferObject: incomplete depth/stencil framebuffer ({:#x}), retrying depth-only",
        status);
    while (glGetError() != GL_NO_ERROR)
    {
    }
    glGenRenderbuffers(1, &m_depthBuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, m_depthBuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, width, height);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_depthBuffer);
    status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE)
    {
      glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, 0);
      glDeleteRenderbuffers(1, &m_depthBuffer);
      m_depthBuffer = 0;
      while (glGetError() != GL_NO_ERROR)
      {
      }
    }
#endif
    if (status != GL_FRAMEBUFFER_COMPLETE)
      CLog::Log(LOGERROR, "CFrameBufferObject: incomplete depth/stencil framebuffer ({:#x})",
                status);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return status == GL_FRAMEBUFFER_COMPLETE;
  }
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  return true;
}

void CFrameBufferObject::SetFiltering(GLenum target, GLenum mode)
{
  glBindTexture(target, m_texid);
  glTexParameteri(target, GL_TEXTURE_MAG_FILTER, mode);
  glTexParameteri(target, GL_TEXTURE_MIN_FILTER, mode);
}

// Begin rendering to FBO
bool CFrameBufferObject::BeginRender()
{
  if (IsValid() && IsBound())
  {
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    return true;
  }
  return false;
}

// Finish rendering to FBO
void CFrameBufferObject::EndRender() const
{
  if (IsValid())
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
