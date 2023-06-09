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

/************************************************************************/
/*    CGLTexture                                                       */
/************************************************************************/
class CGLTexture : public CTexture
{
public:
  CGLTexture(unsigned int width = 0, unsigned int height = 0, XB_FMT format = XB_FMT_A8R8G8B8);
  ~CGLTexture() override;

  void CreateTextureObject() override;
  void DestroyTextureObject() override;
  void LoadToGPU() override;
  void BindToUnit(unsigned int unit) override;

protected:
  GLuint m_texture = 0;
  bool m_isOglVersion3orNewer = false;
};

