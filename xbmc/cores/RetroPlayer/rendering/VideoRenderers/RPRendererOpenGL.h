/*
 *      Copyright (C) 2017 Team Kodi
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
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
