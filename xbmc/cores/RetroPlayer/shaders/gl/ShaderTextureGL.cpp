/*
 *  Copyright (C) 2019-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ShaderTextureGL.h"

#include "guilib/TextureGL.h"
#include "utils/log.h"

#include <cassert>

using namespace KODI::SHADER;

CShaderTextureGL::CShaderTextureGL(std::shared_ptr<CGLTexture> texture, bool sRgbFramebuffer)
  : m_texture(std::move(texture)),
    m_sRgbFramebuffer(sRgbFramebuffer)
{
  assert(m_texture.get() != nullptr);
}

CShaderTextureGL::~CShaderTextureGL()
{
  if (FBO != 0)
    glDeleteFramebuffers(1, &FBO);
}

float CShaderTextureGL::GetWidth() const
{
  return static_cast<float>(m_texture->GetWidth());
}

float CShaderTextureGL::GetHeight() const
{
  return static_cast<float>(m_texture->GetHeight());
}

bool CShaderTextureGL::CreateFBO()
{
  if (FBO == 0)
    glGenFramebuffers(1, &FBO);

  return true;
}

bool CShaderTextureGL::BindFBO()
{
  const GLuint renderTargetID = m_texture->GetTextureID();
  if (renderTargetID == 0)
    return false;

  if (FBO == 0 && !CreateFBO())
    return false;

  glBindFramebuffer(GL_FRAMEBUFFER, FBO);
  glBindTexture(GL_TEXTURE_2D, renderTargetID);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, renderTargetID, 0);

  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
  {
    CLog::LogF(LOGERROR, "Framebuffer is not complete!");
    UnbindFBO();
    return false;
  }

  if (IsSRGBFramebuffer())
    glEnable(GL_FRAMEBUFFER_SRGB);

  return true;
}

void CShaderTextureGL::UnbindFBO() const
{
  if (IsSRGBFramebuffer())
    glDisable(GL_FRAMEBUFFER_SRGB);

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
