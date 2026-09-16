/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "RPBaseRenderer.h"
#include "cores/GameSettings.h"
#include "cores/RetroPlayer/process/RPProcessInfo.h"

#include <memory>
#include <stdint.h>
#include <string>

#include "system_gl.h"

namespace KODI
{
namespace SHADER
{
class IShaderTexture;
}

namespace RETRO
{
class CRenderBufferFBO;

/*!
 * \brief Renderer factory for game clients that render on the GPU
 *
 * Register this last. Buffer pools are tried in registration order and the
 * search stops at the first match, so software streams settle on DMA or sysmem
 * without ever consulting this hardware-only pool.
 */
class CRendererFactoryFBO : public IRendererFactory
{
public:
  ~CRendererFactoryFBO() override = default;

  // implementation of IRendererFactory
  std::string RenderSystemName() const override;
  CRPBaseRenderer* CreateRenderer(const CRenderSettings& settings,
                                  CRenderContext& context,
                                  std::shared_ptr<IRenderBufferPool> bufferPool) override;
  RenderBufferPoolVector CreateBufferPools(CRenderContext& context) override;
};

#if (defined(HAS_EGL) || defined(TARGET_DARWIN_OSX)) && (defined(HAS_GL) || HAS_GLES == 3)
class CRPRendererFBO : public CRPBaseRenderer
{
public:
  CRPRendererFBO(const CRenderSettings& renderSettings,
                 CRenderContext& context,
                 std::shared_ptr<IRenderBufferPool> bufferPool);
  ~CRPRendererFBO() override;

  // implementation of CRPBaseRenderer
  bool Supports(RENDERFEATURE feature) const override;
  SCALINGMETHOD GetDefaultScalingMethod() const override { return SCALINGMETHOD::NEAREST; }

  static bool SupportsScalingMethod(SCALINGMETHOD method);

protected:
  struct PackedVertex
  {
    float x, y, z;
    float u1, v1;
  };

  struct Svertex
  {
    float x, y, z;
  };

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

  void DestroyShaderResources();

  GLenum m_textureTarget = GL_TEXTURE_2D;
  float m_clearColour = 0.0f;

  struct FrameGeometry
  {
    unsigned int frameWidth{0};
    unsigned int frameHeight{0};
    unsigned int textureWidth{0};
    unsigned int textureHeight{0};
    CRect sourceRect;
    CRect samplingRect;
    bool bottomLeftOrigin{false};

    bool operator==(const FrameGeometry& rhs) const
    {
      return frameWidth == rhs.frameWidth && frameHeight == rhs.frameHeight &&
             textureWidth == rhs.textureWidth && textureHeight == rhs.textureHeight &&
             sourceRect == rhs.sourceRect && samplingRect == rhs.samplingRect &&
             bottomLeftOrigin == rhs.bottomLeftOrigin;
    }
    bool operator!=(const FrameGeometry& rhs) const { return !(*this == rhs); }
  };

  FrameGeometry m_loggedGeometry;
  bool m_loggedHardwarePresentation{false};

  GLuint m_mainVAO{0};
  GLuint m_mainVertexVBO{0};
  GLuint m_mainIndexVBO{0};
  GLuint m_blackbarsVAO{0};
  GLuint m_blackbarsVertexVBO{0};

  std::shared_ptr<SHADER::IShaderTexture> m_shaderTargetTexture;

  unsigned int m_shaderTargetWidth{0};
  unsigned int m_shaderTargetHeight{0};
};
#endif
} // namespace RETRO
} // namespace KODI
