/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "RPBaseRenderer.h"
#include "cores/RetroPlayer/buffers/video/RenderBufferSysMem.h"
#include "cores/RetroPlayer/buffers/BaseRenderBufferPool.h"
#include "cores/RetroPlayer/process/RPProcessInfo.h"
#include "cores/GameSettings.h"

#include "system_gl.h"

#include <atomic>
#include <stdint.h>
#include <vector>

namespace KODI
{
namespace RETRO
{
  class CRendererFactoryOpenGLES : public IRendererFactory
  {
  public:
    virtual ~CRendererFactoryOpenGLES() = default;

    // implementation of IRendererFactory
    std::string RenderSystemName() const override;
    CRPBaseRenderer *CreateRenderer(const CRenderSettings &settings, CRenderContext &context, std::shared_ptr<IRenderBufferPool> bufferPool) override;
    RenderBufferPoolVector CreateBufferPools(CRenderContext &context) override;
  };

  class CRenderBufferOpenGLES : public CRenderBufferSysMem
  {
  public:
    CRenderBufferOpenGLES(CRenderContext &context,
                          GLuint pixeltype,
                          GLuint internalformat,
                          GLuint pixelformat,
                          GLuint bpp);
    ~CRenderBufferOpenGLES() override;

    // implementation of IRenderBuffer via CRenderBufferSysMem
    bool UploadTexture() override;

    GLuint TextureID() const { return m_textureId; }

  protected:
    // Construction parameters
    CRenderContext &m_context;
    const GLuint m_pixeltype;
    const GLuint m_internalformat;
    const GLuint m_pixelformat;
    const GLuint m_bpp;

    const GLenum m_textureTarget = GL_TEXTURE_2D; //! @todo
    GLuint m_textureId = 0;

    void CreateTexture();

  private:
    void DeleteTexture();
  };

  class CRenderBufferPoolOpenGLES : public CBaseRenderBufferPool
  {
  public:
    CRenderBufferPoolOpenGLES(CRenderContext &context);
    ~CRenderBufferPoolOpenGLES() override = default;

    // implementation of IRenderBufferPool via CBaseRenderBufferPool
    bool IsCompatible(const CRenderVideoSettings &renderSettings) const override;

  protected:
    // implementation of CBaseRenderBufferPool
    IRenderBuffer *CreateRenderBuffer(void *header = nullptr) override;
    bool ConfigureInternal() override;

    // Construction parameters
    CRenderContext &m_context;

    // Configuration parameters
    GLuint m_pixeltype = 0;
    GLuint m_internalformat = 0;
    GLuint m_pixelformat = 0;
    GLuint m_bpp = 0;
  };

  class CRPRendererOpenGLES : public CRPBaseRenderer
  {
  public:
    CRPRendererOpenGLES(const CRenderSettings &renderSettings, CRenderContext &context, std::shared_ptr<IRenderBufferPool> bufferPool);
    ~CRPRendererOpenGLES() override;

    // implementation of CRPBaseRenderer
    bool Supports(RENDERFEATURE feature) const override;
    SCALINGMETHOD GetDefaultScalingMethod() const override { return SCALINGMETHOD::NEAREST; }

    static bool SupportsScalingMethod(SCALINGMETHOD method);

  protected:
    // implementation of CRPBaseRenderer
    void RenderInternal(bool clear, uint8_t alpha) override;
    void FlushInternal() override;

    /*!
     * \brief Set the entire backbuffer to black
     */
    void ClearBackBuffer();

    /*!
     * \brief Draw black bars around the video quad
     *
     * This is more efficient than glClear() since it only sets pixels to
     * black that aren't going to be overwritten by the game.
     */
    void DrawBlackBars();

    virtual void Render(uint8_t alpha);

    GLenum m_textureTarget = GL_TEXTURE_2D;
    float m_clearColour = 0.0f;
  };
}
}
