/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/GameSettings.h"
#include "cores/RetroPlayer/RetroPlayerTypes.h"
#include "threads/CriticalSection.h"

#include <functional>
#include <memory>
#include <string>
#include <vector>

extern "C"
{
#include <libavutil/pixfmt.h>
}

class CDataCacheCore;

namespace KODI
{
namespace RETRO
{
class CRenderBufferManager;
class CRenderContext;
class CRenderSettings;
class CRPBaseRenderer;
class CRPProcessInfo;
class IRenderBufferPool;

/*!
 * \brief Process info factory
 */
using CreateRPProcessControl = std::function<std::unique_ptr<CRPProcessInfo>()>;

/*!
 * \brief Rendering factory
 */
class IRendererFactory
{
public:
  virtual ~IRendererFactory() = default;

  /*!
   * \brief Get a description name of the rendering system
   */
  virtual std::string RenderSystemName() const = 0;

  /*!
   * \brief Create a renderer
   *
   * \param settings The renderer's initial settings
   * \param context The rendering context
   * \param bufferPool The buffer pool to which buffers are returned
   */
  virtual CRPBaseRenderer* CreateRenderer(const CRenderSettings& settings,
                                          CRenderContext& context,
                                          std::shared_ptr<IRenderBufferPool> bufferPool) = 0;

  /*!
   * \brief Create buffer pools to manager buffers
   *
   * \param context The rendering context shared with the buffer pools
   *
   * \return The buffer pools supported by the rendering system
   */
  virtual RenderBufferPoolVector CreateBufferPools(CRenderContext& context) = 0;
};

/*!
 * \brief Player process info
 */
class CRPProcessInfo
{
public:
  static std::unique_ptr<CRPProcessInfo> CreateInstance();
  static void RegisterProcessControl(const CreateRPProcessControl& createFunc);
  static void RegisterRendererFactory(IRendererFactory* factory);

  virtual ~CRPProcessInfo();

  /*!
   * \brief Get the descriptive name of the platform
   *
   * \return The name of the platform as set by windowing
   */
  const std::string& GetPlatformName() const { return m_platformName; }

  /*!
   * \brief Get the descriptive name of the rendering system
   *
   * \param renderBufferPool A pool belonging to the rendering system
   *
   * \return The name of the rendering system as set by windowing
   */
  std::string GetRenderSystemName(IRenderBufferPool* renderBufferPool) const;

  /*!
   * \brief Create a renderer
   *
   * \param renderBufferPool The buffer pool used to return render buffers
   * \param renderSettings The settings for this renderer
   *
   * \return The renderer, or nullptr on failure
   */
  CRPBaseRenderer* CreateRenderer(IRenderBufferPool* renderBufferPool,
                                  const CRenderSettings& renderSettings);

  /*!
   * \brief Set data cache
   */
  void SetDataCache(CDataCacheCore* cache);

  /*!
   * \brief Reset data cache info
   */
  void ResetInfo();

  /// @name Rendering functions
  ///{

  /*!
   * \brief Get the context shared by the rendering system
   */
  CRenderContext& GetRenderContext() { return *m_renderContext; }

  /*!
   * \brief Get the buffer manager that owns the buffer pools
   */
  CRenderBufferManager& GetBufferManager() { return *m_renderBufferManager; }

  /*!
   * \brief Check if a buffer pool supports the given scaling method
   */
  bool HasScalingMethod(SCALINGMETHOD scalingMethod) const;

  /*!
   * \brief Get the default scaling method for this rendering system
   */
  SCALINGMETHOD GetDefaultScalingMethod() const { return m_defaultScalingMethod; }

  ///}

  /// @name Player video info
  ///{
  void SetVideoPixelFormat(AVPixelFormat pixFormat);
  void SetVideoDimensions(int width, int height);
  void SetVideoFps(float fps);
  ///}

  /// @name Player audio info
  ///{
  void SetAudioChannels(const std::string& channels);
  void SetAudioSampleRate(int sampleRate);
  void SetAudioBitsPerSample(int bitsPerSample);
  ///}

  /// @name Player states
  ///{
  void SetSpeed(float speed);
  void SetPlayTimes(time_t start, int64_t current, int64_t min, int64_t max);
  ///}

protected:
  /*!
   * \brief Constructor
   *
   * \param platformName A descriptive name of the platform
   */
  CRPProcessInfo(std::string platformName);

  /*!
   * \brief Get all scaling methods available to the rendering system
   */
  static std::vector<SCALINGMETHOD> GetScalingMethods();

  // Static factories
  static CreateRPProcessControl m_processControl;
  static std::vector<std::unique_ptr<IRendererFactory>> m_rendererFactories;
  static CCriticalSection m_createSection;

  // Construction parameters
  const std::string m_platformName;

  // Info parameters
  CDataCacheCore* m_dataCache = nullptr;

  // Rendering parameters
  std::unique_ptr<CRenderBufferManager> m_renderBufferManager;

private:
  // Rendering parameters
  std::unique_ptr<CRenderContext> m_renderContext;
  SCALINGMETHOD m_defaultScalingMethod = SCALINGMETHOD::AUTO;
};

} // namespace RETRO
} // namespace KODI
