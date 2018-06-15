/*
 *  Copyright (C) 2017 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "RPRendererOpenGLES.h"
#include "cores/RetroPlayer/process/RPProcessInfo.h"

#include "system_gl.h"

namespace KODI
{
namespace RETRO
{
  class CRenderContext;

  class CRendererFactoryOpenGL : public IRendererFactory
  {
  public:
    virtual ~CRendererFactoryOpenGL() = default;

    // implementation of IRendererFactory
    std::string RenderSystemName() const override;
    CRPBaseRenderer *CreateRenderer(const CRenderSettings &settings, CRenderContext &context, std::shared_ptr<IRenderBufferPool> bufferPool) override;
    RenderBufferPoolVector CreateBufferPools(CRenderContext &context) override;
  };

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

  class CRPRendererOpenGL : public CRPRendererOpenGLES
  {
  public:
    CRPRendererOpenGL(const CRenderSettings &renderSettings, CRenderContext &context, std::shared_ptr<IRenderBufferPool> bufferPool);
    ~CRPRendererOpenGL() override = default;
  };
}
}
