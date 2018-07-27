/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "RenderBufferPoolOpenGLES.h"
#include "system_gl.h"

namespace KODI
{
namespace RETRO
{
  class CRenderBufferPoolOpenGL : public CRenderBufferPoolOpenGLES
  {
  public:
    CRenderBufferPoolOpenGL(CRenderContext &context);
    ~CRenderBufferPoolOpenGL() override = default;

  protected:
    // implementation of CBaseRenderBufferPool via CRenderBufferPoolOpenGLES
    IRenderBuffer *CreateRenderBuffer(void *header = nullptr) override;
    bool ConfigureInternal() override;
  };
}
}
