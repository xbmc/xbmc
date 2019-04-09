/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "Texture.h"

#include "system_gl.h"

class CTextureGLBase : public CTexture
{
public:
  CTextureGLBase(unsigned int width = 0,
                 unsigned int height = 0,
                 unsigned int format = XB_FMT_A8R8G8B8);
  ~CTextureGLBase() override;

  void CreateTextureObject() override;
  void DestroyTextureObject() override;
  virtual void LoadToGPU() override = 0;
  void BindToUnit(unsigned int unit) override;

protected:
  GLuint m_texture = 0;
  unsigned int m_maxSize = 0;
};
