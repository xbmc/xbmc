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

class CTexture;

class CSlideShowPicGLES : public CSlideShowPic
{
public:
  CSlideShowPicGLES() = default;
  ~CSlideShowPicGLES() override = default;

protected:
  void Render(float* x, float* y, CTexture* pTexture, KODI::UTILS::COLOR::Color color) override;

private:
  KODI::UTILS::GL::CGLBufferObject m_posVBO{GL_ARRAY_BUFFER};
  KODI::UTILS::GL::CGLBufferObject m_texVBO{GL_ARRAY_BUFFER};
  KODI::UTILS::GL::CGLBufferObject m_IBO{GL_ELEMENT_ARRAY_BUFFER};
};
