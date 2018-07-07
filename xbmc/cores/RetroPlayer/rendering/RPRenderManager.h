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
   * when we detect a pause event, the frame is preemptively cached so that a
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
    bool Configure(AVPixelFormat format, unsigned int nominalWidth, unsigned int nominalHeight, unsigned int maxWidth, unsigned int maxHeight);
    bool GetVideoBuffer(unsigned int width, unsigned int height, AVPixelFormat &format, uint8_t *&data, size_t &size);
    void AddFrame(const uint8_t* data, size_t size, unsigned int width, unsigned int height, unsigned int orientationDegCW);

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
    bool SupportsRenderFeature(RENDERFEATURE feature) const override;
    bool SupportsScalingMethod(SCALINGMETHOD method) const override;

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

    /*!
     * \brief Create a render buffer and copy the cached data into it
     *
     * The cached frame is accessed by both the game and rendering threads,
     * and therefore requires synchronization.
     *
     * However, assuming the memory copy is expensive, we must avoid holding
     * the mutex during the copy.
     *
     * To allow for this, the function is permitted to invalidate its
     * cachedFrame parameter, as long as it is restored upon exit. While the
     * mutex is exited inside this function, cachedFrame is guaranteed to be
     * empty.
     *
     * \param cachedFrame The cached frame
     * \param bufferPool The buffer pool used to create the render buffer
     * \param mutex The locked mutex, to be unlocked during memory copy
     *
     * \return The render buffer if one was created from the cached frame,
     *         otherwise nullptr
     */
    IRenderBuffer *CreateFromCache(std::vector<uint8_t> &cachedFrame, IRenderBufferPool *bufferPool, CCriticalSection &mutex);

    void UpdateResolution();

    /*!
     * \brief Utility function to copy a frame and rescale pixels if necessary
     */
    void CopyFrame(IRenderBuffer *renderBuffer, AVPixelFormat format, const uint8_t *data, size_t size, unsigned int width, unsigned int height);

    CRenderVideoSettings GetEffectiveSettings(const IGUIRenderSettings *settings) const;

    void CheckFlush();

    // Construction parameters
    CRPProcessInfo &m_processInfo;
    CRenderContext &m_renderContext;

    // Subsystems
    std::shared_ptr<IGUIRenderSettings> m_renderSettings;
    std::shared_ptr<CGUIRenderTargetFactory> m_renderControlFactory;

    // Stream properties
    AVPixelFormat m_format = AV_PIX_FMT_NONE;
    unsigned int m_maxWidth = 0;
    unsigned int m_maxHeight = 0;
    unsigned int m_width = 0; //! @todo Remove me when dimension changing is implemented
    unsigned int m_height = 0; //! @todo Remove me when dimension changing is implemented

    // Render resources
    std::set<std::shared_ptr<CRPBaseRenderer>> m_renderers;
    std::vector<IRenderBuffer*> m_pendingBuffers; // Only access from game thread
    std::vector<IRenderBuffer*> m_renderBuffers;
    std::map<AVPixelFormat, SwsContext*> m_scalers;
    std::vector<uint8_t> m_cachedFrame;

    // State parameters
    enum class RENDER_STATE
    {
      UNCONFIGURED,
      CONFIGURING,
      RECONFIGURING,
      CONFIGURED,
    };
    RENDER_STATE m_state = RENDER_STATE::UNCONFIGURED;
    bool m_bHasCachedFrame = false; // Invariant: m_cachedFrame is empty if false
    std::set<std::string> m_failedShaderPresets;
    bool m_bTriggerUpdateResolution = false;
    std::atomic<bool> m_bFlush = {false};

    // Playback parameters
    std::atomic<double> m_speed = {1.0};

    // Synchronization parameters
    CCriticalSection m_stateMutex;
    CCriticalSection m_bufferMutex;
  };
}
}
