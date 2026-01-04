/*
 *  Copyright (C) 2019-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ShaderTextureGL.h"

#include "utils/log.h"

#include <cassert>

using namespace KODI::SHADER;

CShaderTextureGL::CShaderTextureGL(uint32_t textureWidth, uint32_t textureHeight)
  : m_textureWidth(textureWidth),
    m_textureHeight(textureHeight)
{
}

CShaderTextureGL::~CShaderTextureGL()
{
  DestroyFBO();
  DestroyTextureObject();
}

float CShaderTextureGL::GetWidth() const
{
  return static_cast<float>(m_textureWidth);
}

float CShaderTextureGL::GetHeight() const
{
  return static_cast<float>(m_textureHeight);
}

void CShaderTextureGL::CreateTextureObject()
{
  glGenTextures(1, &m_texture);
}

void CShaderTextureGL::DestroyTextureObject()
{
  if (m_texture != 0)
    glDeleteTextures(1, &m_texture);

  m_texture = 0;
}

void CShaderTextureGL::BindToUnit(unsigned int unit)
{
  glActiveTexture(GL_TEXTURE0 + unit);
  glBindTexture(GL_TEXTURE_2D, m_texture);
}

void CShaderTextureGL::CreateFBO()
{
  if (m_FBO == 0)
    glGenFramebuffers(1, &m_FBO);
}

void CShaderTextureGL::DestroyFBO()
{
  if (m_FBO != 0)
    glDeleteFramebuffers(1, &m_FBO);

  m_FBO = 0;
}

bool CShaderTextureGL::BindFBO()
{
  if (m_texture == 0)
    return false;

  if (m_FBO == 0)
    CreateFBO();

  glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);
  glBindTexture(GL_TEXTURE_2D, m_texture);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_texture, 0);

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
