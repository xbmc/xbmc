/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "TextureGLBase.h"

class CTextureGLES : public CTextureGLBase
{
public:
  CTextureGLES(unsigned int width = 0,
               unsigned int height = 0,
               unsigned int format = XB_FMT_A8R8G8B8);

  void LoadToGPU() override;
};
