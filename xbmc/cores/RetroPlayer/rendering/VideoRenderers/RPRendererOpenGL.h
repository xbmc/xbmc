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
    CRPBaseRenderer *CreateRenderer(const CRenderSettings &settings, CRenderContext &context, std::shared_ptr<IRenderBufferPool> bufferPool) override;
    RenderBufferPoolVector CreateBufferPools() override;
  };

  class CRenderBufferOpenGL : public CRenderBufferOpenGLES
  {
  public:
    CRenderBufferOpenGL(AVPixelFormat format, AVPixelFormat targetFormat, unsigned int width, unsigned int height);
    ~CRenderBufferOpenGL() override = default;

    // implementation of IRenderBuffer via CRenderBufferOpenGLES
    bool UploadTexture() override;

  private:
    void CreateTexture();
  };

  class CRenderBufferPoolOpenGL : public CRenderBufferPoolOpenGLES
  {
  public:
    CRenderBufferPoolOpenGL() = default;
    ~CRenderBufferPoolOpenGL() override = default;

    // implementation of CBaseRenderBufferPool via CRenderBufferPoolOpenGLES
    IRenderBuffer *CreateRenderBuffer(void *header = nullptr) override;
  };

  class CRPRendererOpenGL : public CRPRendererOpenGLES
  {
  public:
    CRPRendererOpenGL(const CRenderSettings &renderSettings, CRenderContext &context, std::shared_ptr<IRenderBufferPool> bufferPool);
    ~CRPRendererOpenGL() override = default;

  protected:
    // implementation of CRPBaseRenderer via CRPRendererOpenGLES
    void RenderInternal(bool clear, uint8_t alpha) override;

  private:
    void Render();

    void DrawBlackBars();
  };
}
}
