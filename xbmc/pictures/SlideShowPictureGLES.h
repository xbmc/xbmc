/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "SlideShowPicture.h"
#include "utils/GLBufferObject.h"

#include <memory>

class CGainMapShaderGLES;
class CTexture;

class CSlideShowPicGLES : public CSlideShowPic
{
public:
  CSlideShowPicGLES();
  ~CSlideShowPicGLES() override;

  void Free() override;
  bool SupportsHdr() const override { return true; }

protected:
  void Render(float* x, float* y, CTexture* pTexture, KODI::UTILS::COLOR::Color color) override;
  bool RenderHdr(float* x,
                 float* y,
                 CTexture* pTexture,
                 CTexture* gainMap,
                 const KODI::PICTURE::GainMapParams& params,
                 KODI::UTILS::COLOR::Color color) override;

private:
  KODI::UTILS::GL::CGLBufferObject m_posVBO{GL_ARRAY_BUFFER};
  KODI::UTILS::GL::CGLBufferObject m_texVBO{GL_ARRAY_BUFFER};
  KODI::UTILS::GL::CGLBufferObject m_IBO{GL_ELEMENT_ARRAY_BUFFER};
  std::unique_ptr<CGainMapShaderGLES> m_gainMapShader;
  bool m_gainMapShaderFailed{false};
};
