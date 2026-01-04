/*
 *  Copyright (C) 2019-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ShaderTextureGLES.h"

#include "utils/log.h"

#include <cassert>

using namespace KODI::SHADER;

CShaderTextureGLES::CShaderTextureGLES(uint32_t textureWidth, uint32_t textureHeight)
  : m_textureWidth(textureWidth),
    m_textureHeight(textureHeight)
{
}

CShaderTextureGLES::~CShaderTextureGLES()
{
  DestroyFBO();
  DestroyTextureObject();
}

float CShaderTextureGLES::GetWidth() const
{
  return static_cast<float>(m_textureWidth);
}

float CShaderTextureGLES::GetHeight() const
{
  return static_cast<float>(m_textureHeight);
}

void CShaderTextureGLES::CreateTextureObject()
{
  glGenTextures(1, &m_texture);
}

void CShaderTextureGLES::DestroyTextureObject()
{
  if (m_texture != 0)
    glDeleteTextures(1, &m_texture);

  m_texture = 0;
}

void CShaderTextureGLES::BindToUnit(unsigned int unit)
{
  glActiveTexture(GL_TEXTURE0 + unit);
  glBindTexture(GL_TEXTURE_2D, m_texture);
}

void CShaderTextureGLES::CreateFBO()
{
  if (m_FBO == 0)
    glGenFramebuffers(1, &m_FBO);
}

void CShaderTextureGLES::DestroyFBO()
{
  if (m_FBO != 0)
    glDeleteFramebuffers(1, &m_FBO);

  m_FBO = 0;
}

bool CShaderTextureGLES::BindFBO()
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

  return true;
}

void CShaderTextureGLES::UnbindFBO() const
{
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
