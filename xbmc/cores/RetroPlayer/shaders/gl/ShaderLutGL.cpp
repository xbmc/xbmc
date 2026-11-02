/*
 *  Copyright (C) 2019-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ShaderLutGL.h"

#include "ShaderUtilsGL.h"
#include "cores/RetroPlayer/rendering/RenderContext.h"
#include "cores/RetroPlayer/shaders/IShaderPreset.h"
#include "guilib/Texture.h"
#include "guilib/TextureGL.h"
#include "rendering/gl/RenderSystemGL.h"
#include "utils/log.h"

#include <utility>

using namespace KODI::SHADER;

CShaderLutGL::CShaderLutGL(std::string id, std::string path)
  : IShaderLut(std::move(id), std::move(path))
{
}

bool CShaderLutGL::Create(const ShaderLut& lut)
{
  std::unique_ptr<CTexture> lutTexture(CreateLUTTexture(lut));
  if (!lutTexture)
  {
    CLog::Log(LOGWARNING, "CShaderLutGL::Create: Couldn't create texture for LUT: {}", lut.strId);
    return false;
  }

  m_texture = std::move(lutTexture);
  m_lut = lut;
  return true;
}

CTexture* CShaderLutGL::GetTexture()
{
  if (m_released)
  {
    // Loading binds the LUT on the active unit, where the caller may have bound its source
    GLint boundTexture = 0;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &boundTexture);

    m_texture = CreateLUTTexture(m_lut);
    m_released = false;

    glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(boundTexture));
  }

  return m_texture.get();
}

void CShaderLutGL::ReleaseTexture()
{
  m_texture.reset();
  m_released = true;
}

std::unique_ptr<CTexture> CShaderLutGL::CreateLUTTexture(const ShaderLut& lut)
{
  std::unique_ptr<CTexture> texture = CTexture::LoadFromFile(lut.path);
  auto* textureGL = static_cast<CGLTexture*>(texture.get());

  if (textureGL == nullptr)
  {
    CLog::Log(LOGERROR, "CShaderLutGL::CreateLUTTexture: Couldn't open LUT: {}", lut.path);
    return std::unique_ptr<CTexture>();
  }

  if (lut.mipmap)
    textureGL->SetMipmapping();

  textureGL->SetScalingMethod(lut.filterType == FilterType::LINEAR ? TEXTURE_SCALING::LINEAR
                                                                   : TEXTURE_SCALING::NEAREST);
  textureGL->LoadToGPU();

  const GLint wrapType = CShaderUtilsGL::TranslateWrapType(lut.wrapType);
  const GLfloat blackBorder[4] = {0.0f, 0.0f, 0.0f, 0.0f};

  glBindTexture(GL_TEXTURE_2D, textureGL->GetTextureID());
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapType);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapType);
  glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, blackBorder);

  return texture;
}
