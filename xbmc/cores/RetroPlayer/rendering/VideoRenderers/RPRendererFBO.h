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
#include "cores/RetroPlayer/buffers/BaseRenderBufferPool.h"
#include "cores/RetroPlayer/buffers/video/RenderBufferSysMem.h"
#include "cores/RetroPlayer/process/RPProcessInfo.h"

#include <atomic>
#include <chrono>
#include <memory>
#include <stdint.h>
#include <string>
#include <vector>

//! The renderer needs glBlitFramebuffer and fence syncs, so it is built for
//! desktop GL and GLES 3.0 upwards. The factory is declared either way and
//! declines where the renderer is absent, so a caller registering it needs no
//! build check of its own.
#if defined(HAS_EGL) && (defined(HAS_GL) || HAS_GLES == 3)
#define HAS_RP_RENDERER_FBO
#endif

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

#ifdef HAS_RP_RENDERER_FBO
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

  /*!
   * \brief Copy the captured frame into a texture with a top-left origin
   *
   * Shader presets sample the entire texture and must process the frame in
   * display order, including filters with directional effects.
   *
   * \param renderBuffer The buffer holding the captured frame
   *
   * \return True if the frame is in m_shaderSourceTexture and can be shaded
   */
  bool CopyFrameForShaders(CRenderBufferFBO* renderBuffer);

  //! \brief Release the textures and framebuffer used to feed the shader chain
  void DestroyShaderResources();

  GLenum m_textureTarget = GL_TEXTURE_2D;
  float m_clearColour = 0.0f;

  /*!
   * \brief The geometry a frame was drawn with, as far as logging cares
   *
   * Compared exactly rather than with a tolerance: every field is copied or
   * derived the same way each frame, so any difference at all is a real change
   * and worth seeing.
   */
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

  //! \brief The geometry reported by the last line written to the log
  FrameGeometry m_loggedGeometry;

  //! \brief Set once anything has been logged, so the first frame always is
  bool m_bLoggedGeometry = false;

  //! \brief Changes seen since the last line was written
  unsigned int m_geometryChanges = 0;

  //! \brief When the last line was written, to keep a churning geometry quiet
  std::chrono::steady_clock::time_point m_lastGeometryLog;

  //! \brief Framebuffer used to blit the client's frame into a tight texture
  GLuint m_shaderCopyFbo{0};

  //! Reads the client's frame into the copy above. Its own object because a
  //! framebuffer is not shared between contexts, only the texture it holds is
  GLuint m_shaderReadFbo{0};

  //! \brief The client's frame at its own size, as the shader chain wants it
  GLuint m_shaderSourceTexture{0};

  //! \brief The size m_shaderSourceTexture was created at
  unsigned int m_shaderSourceWidth{0};
  unsigned int m_shaderSourceHeight{0};

  //! \brief Where the shader chain writes, and what is finally drawn
  //! \brief Last reported filter state, so the log carries changes not frames
  std::string m_lastLoggedPreset{"\0"};
  bool m_bLastLoggedUsePreset{false};

  //! \brief Isolates vertex attributes and index bindings from the GUI
  GLuint m_vao{0};

  std::shared_ptr<SHADER::IShaderTexture> m_shaderTargetTexture;

  //! \brief The size m_shaderTargetTexture was created at
  unsigned int m_shaderTargetWidth{0};
  unsigned int m_shaderTargetHeight{0};
};
#endif
} // namespace RETRO
} // namespace KODI
