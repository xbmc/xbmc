/*
 *      Copyright (C) 2017-present Team Kodi
 *      http://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
 *
 */
#pragma once

#include "IRenderManager.h"
#include "RenderVideoSettings.h"
#include "cores/RetroPlayer/guibridge/IRenderCallback.h"
#include "threads/CriticalSection.h"

extern "C" {
#include "libavutil/pixfmt.h"
}

#include <atomic>
#include <map>
#include <memory>
#include <set>
#include <vector>

struct SwsContext;

namespace KODI
{
namespace RETRO
{
  class CGUIRenderTargetFactory;
  class CRenderContext;
  class CRenderSettings;
  class CRPBaseRenderer;
  class CRPProcessInfo;
  class IGUIRenderSettings;
  class IRenderBuffer;
  class IRenderBufferPool;

  /*!
   * \brief Renders video frames provided by the game loop
   *
   * Generally, buffer pools are registered by the windowing subsystem. A buffer
   * pool provides a software or hardware buffer to store the added frame. When
   * RenderManager is created, it instantiates all registered buffer pools.
   *
   * When a frame arrives, it is copied into a buffer from each buffer pool with
   * a visible renderer. For example, if a GLES and MMAL renderer are both
   * visible in the GUI, then the frame will be copied into two buffers.
   *
   * When it is time to render the frame, the GUI control or window calls into
   * this class through the IRenderManager interface. RenderManager selects an
   * appropriate renderer to use to render the frame. The renderer is then
   * given the buffer that came from its buffer pool.
   *
   * Special behavior is needed when the game is paused. As no new frames are
   * delivered, a newly created renderer will stay black. For this scenario,
   * when we detect a pause event, the frame is premptively cached so that a
   * newly created renderer will have something to display.
   */
  class CRPRenderManager : public IRenderManager,
                           public IRenderCallback
  {
  public:
    CRPRenderManager(CRPProcessInfo &processInfo);
    ~CRPRenderManager() override = default;

    void Initialize();
    void Deinitialize();

    /*!
     * \brief Access the factory for creating GUI render targets
     */
    CGUIRenderTargetFactory *GetGUIRenderTargetFactory() { return m_renderControlFactory.get(); }

    // Functions called from game loop
    bool Configure(AVPixelFormat format, unsigned int width, unsigned int height, unsigned int orientation);
    void AddFrame(const uint8_t* data, unsigned int size);

    // Functions called from the player
    void SetSpeed(double speed);

    // Functions called from render thread
    void FrameMove();
    void Flush();
    void TriggerUpdateResolution();

    // Implementation of IRenderManager
    void RenderWindow(bool bClear, const RESOLUTION_INFO &coordsRes) override;
    void RenderControl(bool bClear, bool bUseAlpha, const CRect &renderRegion, const IGUIRenderSettings *renderSettings) override;
    void ClearBackground() override;

    // Implementation of IRenderCallback
    bool SupportsRenderFeature(ERENDERFEATURE feature) const override;
    bool SupportsScalingMethod(ESCALINGMETHOD method) const override;

  private:
    /*!
     * \brief Get or create a renderer compatible with the given render settings
     */
    std::shared_ptr<CRPBaseRenderer> GetRenderer(const IGUIRenderSettings *renderSettings);

    /*!
     * \brief Get or create a renderer for the given buffer pool and render settings
     */
    std::shared_ptr<CRPBaseRenderer> GetRenderer(IRenderBufferPool *bufferPool, const CRenderSettings &renderSettings);

    /*!
     * \brief Render a frame using the given renderer
     */
    void RenderInternal(const std::shared_ptr<CRPBaseRenderer> &renderer, bool bClear, uint32_t alpha);

    /*!
     * \brief Return true if we have a render buffer belonging to the specified pool
     */
    bool HasRenderBuffer(IRenderBufferPool *bufferPool);

    /*!
     * \brief Get a render buffer belonging to the specified pool
     */
    IRenderBuffer *GetRenderBuffer(IRenderBufferPool *bufferPool);

    /*!
     * \brief Create a render buffer for the specified pool from a cached frame
     */
    void CreateRenderBuffer(IRenderBufferPool *bufferPool);

    void UpdateResolution();

    /*!
     * \brief Utility function to copy a frame and rescale pixels if necessary
     */
    void CopyFrame(IRenderBuffer *renderBuffer, const uint8_t *data, size_t size, AVPixelFormat format);

    CRenderVideoSettings GetEffectiveSettings(const IGUIRenderSettings *settings) const;

    // Construction parameters
    CRPProcessInfo &m_processInfo;
    CRenderContext &m_renderContext;

    // Stream properties
    AVPixelFormat m_format = AV_PIX_FMT_NONE;
    unsigned int m_width = 0;
    unsigned int m_height = 0;
    unsigned int m_orientation = 0; // Degrees counter-clockwise

    // Render properties
    enum class RENDER_STATE
    {
      UNCONFIGURED,
      CONFIGURING,
      CONFIGURED,
    };
    RENDER_STATE m_state = RENDER_STATE::UNCONFIGURED;
    std::atomic<double> m_speed;
    std::shared_ptr<IGUIRenderSettings> m_renderSettings;
    std::set<std::shared_ptr<CRPBaseRenderer>> m_renderers;
    std::shared_ptr<CGUIRenderTargetFactory> m_renderControlFactory;
    std::vector<IRenderBuffer*> m_renderBuffers;
    std::vector<uint8_t> m_cachedFrame;
    std::map<AVPixelFormat, SwsContext*> m_scalers;
    bool m_bHasCachedFrame = false;
    bool m_bTriggerUpdateResolution = false;
    CCriticalSection m_stateMutex;
    CCriticalSection m_bufferMutex;
  };
}
}
