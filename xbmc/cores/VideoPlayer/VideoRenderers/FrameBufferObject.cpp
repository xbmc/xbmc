/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FrameBufferObject.h"

#include "ServiceBroker.h"
#include "rendering/RenderSystem.h"
#include "utils/GLUtils.h"
#include "utils/log.h"

//////////////////////////////////////////////////////////////////////
// CFrameBufferObject
//////////////////////////////////////////////////////////////////////

CFrameBufferObject::CFrameBufferObject()
{
  m_fbo = 0;
  m_valid = false;
  m_supported = false;
  m_bound = false;
  m_texid = 0;
}

bool CFrameBufferObject::IsSupported()
{
  if(CServiceBroker::GetRenderSystem()->IsExtSupported("GL_EXT_framebuffer_object"))
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

  gl::GenFramebuffers(1, &m_fbo);
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
    gl::DeleteFramebuffers(1, &m_fbo);

  if (m_texid)
    gl::DeleteTextures(1, &m_texid);

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
    gl::DeleteTextures(1, &m_texid);

  gl::GenTextures(1, &m_texid);
  gl::BindTexture(target, m_texid);
  gl::TexImage2D(target, 0, format, width, height, 0, GL_RGBA, type, NULL);
  gl::TexParameteri(target, GL_TEXTURE_WRAP_S, clampmode);
  gl::TexParameteri(target, GL_TEXTURE_WRAP_T, clampmode);
  gl::TexParameteri(target, GL_TEXTURE_MAG_FILTER, filter);
  gl::TexParameteri(target, GL_TEXTURE_MIN_FILTER, filter);
  VerifyGLState();

  m_bound = false;
  gl::BindFramebuffer(GL_FRAMEBUFFER, m_fbo);
  gl::BindTexture(target, m_texid);
  gl::FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, target, m_texid, 0);
  VerifyGLState();
  GLenum status = gl::CheckFramebufferStatus(GL_FRAMEBUFFER);
  gl::BindFramebuffer(GL_FRAMEBUFFER, 0);
  if (status != GL_FRAMEBUFFER_COMPLETE)
  {
    VerifyGLState();
    return false;
  }
  m_bound = true;
  return true;
}

void CFrameBufferObject::SetFiltering(GLenum target, GLenum mode)
{
  gl::BindTexture(target, m_texid);
  gl::TexParameteri(target, GL_TEXTURE_MAG_FILTER, mode);
  gl::TexParameteri(target, GL_TEXTURE_MIN_FILTER, mode);
}

// Begin rendering to FBO
bool CFrameBufferObject::BeginRender()
{
  if (IsValid() && IsBound())
  {
    gl::BindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    return true;
  }
  return false;
}

// Finish rendering to FBO
void CFrameBufferObject::EndRender() const
{
  if (IsValid())
    gl::BindFramebuffer(GL_FRAMEBUFFER, 0);
}
