/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IRenderManager.h"
#include "RenderVideoSettings.h"
#include "cores/RetroPlayer/guibridge/IRenderCallback.h"
#include "threads/CriticalSection.h"

extern "C"
{
#include <libavutil/pixfmt.h>
}

#include <atomic>
#include <future>
#include <map>
#include <memory>
#include <set>
#include <string>
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
class ISavestate;
struct VideoStreamBuffer;

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
class CRPRenderManager : public IRenderManager, public IRenderCallback
{
public:
  CRPRenderManager(CRPProcessInfo& processInfo);
  ~CRPRenderManager() override = default;

  void Initialize();
  void Deinitialize();

  /*!
   * \brief Access the factory for creating GUI render targets
   */
  CGUIRenderTargetFactory* GetGUIRenderTargetFactory() { return m_renderControlFactory.get(); }

  // Stream properties, set upon configuration
  AVPixelFormat GetPixelFormat() const { return m_format; }
  unsigned int GetNominalWidth() const { return m_nominalWidth; }
  unsigned int GetNominalHeight() const { return m_nominalHeight; }
  unsigned int GetMaxWidth() const { return m_maxWidth; }
  unsigned int GetMaxHeight() const { return m_maxHeight; }
  float GetPixelAspectRatio() const { return m_pixelAspectRatio; }

  // Functions called from game loop
  bool Configure(AVPixelFormat format,
                 unsigned int nominalWidth,
                 unsigned int nominalHeight,
                 unsigned int maxWidth,
                 unsigned int maxHeight,
                 float pixelAspectRatio);
  bool GetVideoBuffer(unsigned int width, unsigned int height, VideoStreamBuffer& buffer);
  void AddFrame(const uint8_t* data,
                size_t size,
                unsigned int width,
                unsigned int height,
                unsigned int orientationDegCW);
  void Flush();

  // Functions called from the player
  void SetSpeed(double speed);

  // Functions called from render thread
  void FrameMove();

  // Implementation of IRenderManager
  void RenderWindow(bool bClear, const RESOLUTION_INFO& coordsRes) override;
  void RenderControl(bool bClear,
                     bool bUseAlpha,
                     const CRect& renderRegion,
                     const IGUIRenderSettings* renderSettings) override;
  void ClearBackground() override;

  // Implementation of IRenderCallback
  bool SupportsRenderFeature(RENDERFEATURE feature) const override;
  bool SupportsScalingMethod(SCALINGMETHOD method) const override;

  // Savestate functions
  void SaveThumbnail(const std::string& thumbnailPath);

  // Savestate functions
  void CacheVideoFrame(const std::string& savestatePath);
  void SaveVideoFrame(const std::string& savestatePath, ISavestate& savestate);
  void ClearVideoFrame(const std::string& savestatePath);

private:
  /*!
   * \brief Get or create a renderer compatible with the given render settings
   */
  std::shared_ptr<CRPBaseRenderer> GetRendererForSettings(const IGUIRenderSettings* renderSettings);

  /*!
   * \brief Get or create a renderer for the given buffer pool and render settings
   */
  std::shared_ptr<CRPBaseRenderer> GetRendererForPool(IRenderBufferPool* bufferPool,
                                                      const CRenderSettings& renderSettings);

  /*!
   * \brief Render a frame using the given renderer
   */
  void RenderInternal(const std::shared_ptr<CRPBaseRenderer>& renderer,
                      IRenderBuffer* renderBuffer,
                      bool bClear,
                      uint32_t alpha);

  /*!
   * \brief Return true if we have a render buffer belonging to the specified pool
   */
  bool HasRenderBuffer(IRenderBufferPool* bufferPool);

  /*!
   * \brief Get a render buffer belonging to the specified pool
   */
  IRenderBuffer* GetRenderBuffer(IRenderBufferPool* bufferPool);

  /*!
   * \brief Get a render buffer containing pixels from the specified savestate
   */
  IRenderBuffer* GetRenderBufferForSavestate(const std::string& savestatePath,
                                             const IRenderBufferPool* bufferPool);

  /*!
   * \brief Create a render buffer for the specified pool from a cached frame
   */
  void CreateRenderBuffer(IRenderBufferPool* bufferPool);

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
   * \param width The width of the cached frame
   * \param height The height of the cached frame
   * \param bufferPool The buffer pool used to create the render buffer
   * \param mutex The locked mutex, to be unlocked during memory copy
   *
   * \return The render buffer if one was created from the cached frame,
   *         otherwise nullptr
   */
  IRenderBuffer* CreateFromCache(std::vector<uint8_t>& cachedFrame,
                                 unsigned int width,
                                 unsigned int height,
                                 IRenderBufferPool* bufferPool,
                                 CCriticalSection& mutex);

  /*!
   * \brief Utility function to copy a frame and rescale pixels if necessary
   */
  void CopyFrame(IRenderBuffer* renderBuffer,
                 AVPixelFormat format,
                 const uint8_t* data,
                 size_t size,
                 unsigned int width,
                 unsigned int height);

  CRenderVideoSettings GetEffectiveSettings(const IGUIRenderSettings* settings) const;

  void CheckFlush();

  void GetVideoFrame(IRenderBuffer*& readableBuffer, std::vector<uint8_t>& cachedFrame);
  void FreeVideoFrame(IRenderBuffer* readableBuffer, std::vector<uint8_t> cachedFrame);
  void LoadVideoFrameAsync(const std::string& savestatePath);
  void LoadVideoFrameSync(const std::string& savestatePath);

  // Construction parameters
  CRPProcessInfo& m_processInfo;
  CRenderContext& m_renderContext;

  // Subsystems
  std::shared_ptr<IGUIRenderSettings> m_renderSettings;
  std::shared_ptr<CGUIRenderTargetFactory> m_renderControlFactory;

  // Stream properties
  AVPixelFormat m_format = AV_PIX_FMT_NONE;
  unsigned int m_nominalWidth{0};
  unsigned int m_nominalHeight{0};
  unsigned int m_maxWidth = 0;
  unsigned int m_maxHeight = 0;
  float m_pixelAspectRatio{1.0f};

  // Render resources
  std::set<std::shared_ptr<CRPBaseRenderer>> m_renderers;
  std::vector<IRenderBuffer*> m_pendingBuffers; // Only access from game thread
  std::vector<IRenderBuffer*> m_renderBuffers;
  std::map<AVPixelFormat, std::map<AVPixelFormat, SwsContext*>> m_scalers; // From -> to -> context
  std::vector<uint8_t> m_cachedFrame;
  unsigned int m_cachedWidth = 0;
  unsigned int m_cachedHeight = 0;
  unsigned int m_cachedRotationCCW{0};
  std::map<std::string, std::vector<IRenderBuffer*>>
      m_savestateBuffers; // Render buffers for savestates
  std::vector<std::future<void>> m_savestateThreads;

  // State parameters
  enum class RENDER_STATE
  {
    UNCONFIGURED,
    CONFIGURING,
    CONFIGURED,
  };
  RENDER_STATE m_state = RENDER_STATE::UNCONFIGURED;
  bool m_bHasCachedFrame = false; // Invariant: m_cachedFrame is empty if false
  std::set<std::string> m_failedShaderPresets;
  std::atomic<bool> m_bFlush = {false};

  // Windowing state
  bool m_bDisplayScaleSet = false;

  // Playback parameters
  std::atomic<double> m_speed = {1.0};

  // Synchronization parameters
  CCriticalSection m_stateMutex;
  CCriticalSection m_bufferMutex;
};
} // namespace RETRO
} // namespace KODI
