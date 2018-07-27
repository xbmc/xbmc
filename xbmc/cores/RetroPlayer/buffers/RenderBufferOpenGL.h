/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "RenderBufferOpenGLES.h"
#include "system_gl.h"

namespace KODI
{
namespace RETRO
{
  class CRenderContext;

  class CRenderBufferOpenGL : public CRenderBufferOpenGLES
  {
  public:
    CRenderBufferOpenGL(CRenderContext &context,
                        GLuint pixeltype,
                        GLuint internalformat,
                        GLuint pixelformat,
                        GLuint bpp);
    ~CRenderBufferOpenGL() override = default;

    // implementation of IRenderBuffer via CRenderBufferOpenGLES
    bool UploadTexture() override;
  };
}
}
