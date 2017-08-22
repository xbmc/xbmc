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

#include "RPBaseRenderer.h"
#include "cores/RetroPlayer/process/BaseRenderBufferPool.h"
#include "cores/RetroPlayer/process/RenderBufferSysMem.h"
#include "cores/RetroPlayer/process/RPProcessInfo.h"

#include "system_gl.h"

#include <atomic>
#include <stdint.h>
#include <vector>

struct SwsContext;

namespace KODI
{
namespace RETRO
{
  class CRendererFactoryOpenGLES : public IRendererFactory
  {
  public:
    virtual ~CRendererFactoryOpenGLES() = default;

    // implementation of IRendererFactory
    CRPBaseRenderer *CreateRenderer(const CRenderSettings &settings, CRenderContext &context, std::shared_ptr<IRenderBufferPool> bufferPool) override;
    RenderBufferPoolVector CreateBufferPools() override;
  };

  class CRenderBufferOpenGLES : public CRenderBufferSysMem
  {
  public:
    CRenderBufferOpenGLES(AVPixelFormat format, AVPixelFormat targetFormat, unsigned int width, unsigned int height);
    ~CRenderBufferOpenGLES() override;

    // implementation of IRenderBuffer via CRenderBufferSysMem
    bool UploadTexture() override;

    GLuint TextureID() const { return m_textureId; }

  protected:
    bool CreateScalingContext();
    void ScalePixels(uint8_t *source, size_t sourceSize, uint8_t *target, size_t targetSize);

    // Construction parameters
    const AVPixelFormat m_format;
    const AVPixelFormat m_targetFormat;
    const unsigned int m_width;
    const unsigned int m_height;

    const GLenum m_textureTarget = GL_TEXTURE_2D; //! @todo
    GLuint m_textureId = 0;
    std::vector<uint8_t> m_textureBuffer;

  private:
    void CreateTexture();
    void DeleteTexture();

    SwsContext *m_swsContext = nullptr;
  };

  class CRenderBufferPoolOpenGLES : public CBaseRenderBufferPool
  {
  public:
    CRenderBufferPoolOpenGLES() = default;
    ~CRenderBufferPoolOpenGLES() override = default;

    // implementation of IRenderBufferPool via CRenderBufferPoolSysMem
    bool IsCompatible(const CRenderVideoSettings &renderSettings) const override;

    // implementation of CBaseRenderBufferPool via CRenderBufferPoolSysMem
    IRenderBuffer *CreateRenderBuffer(void *header = nullptr) override;

    // GLES interface
    bool SetTargetFormat(AVPixelFormat targetFormat);

  protected:
    AVPixelFormat m_targetFormat = AV_PIX_FMT_NONE; //! @todo Change type to GLenum
  };

  class CRPRendererOpenGLES : public CRPBaseRenderer
  {
  public:
    CRPRendererOpenGLES(const CRenderSettings &renderSettings, CRenderContext &context, std::shared_ptr<IRenderBufferPool> bufferPool);
    ~CRPRendererOpenGLES() override;

    // implementation of CRPBaseRenderer
    bool Supports(ERENDERFEATURE feature) const override;
    ESCALINGMETHOD GetDefaultScalingMethod() const override { return VS_SCALINGMETHOD_NEAREST; }

    static bool SupportsScalingMethod(ESCALINGMETHOD method);

  protected:
    // implementation of CRPBaseRenderer
    bool ConfigureInternal() override;
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

    void Render(uint8_t alpha);

    AVPixelFormat m_targetFormat = AV_PIX_FMT_NONE;
    GLenum m_textureTarget = GL_TEXTURE_2D;
    float m_clearColour = 0.0f;
    SwsContext *m_swsContext = nullptr;
  };
}
}
