/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RPRenderManager.h"

#include "RenderContext.h"
#include "RenderSettings.h"
#include "RenderTranslator.h"
#include "cores/RetroPlayer/buffers/IRenderBuffer.h"
#include "cores/RetroPlayer/buffers/IRenderBufferPool.h"
#include "cores/RetroPlayer/buffers/RenderBufferManager.h"
#include "cores/RetroPlayer/guibridge/GUIGameSettings.h"
#include "cores/RetroPlayer/guibridge/GUIRenderTargetFactory.h"
#include "cores/RetroPlayer/guibridge/IGUIRenderSettings.h"
#include "cores/RetroPlayer/process/RPProcessInfo.h"
#include "cores/RetroPlayer/rendering/VideoRenderers/RPBaseRenderer.h"
#include "threads/SingleLock.h"
#include "utils/Color.h"
#include "utils/TransformMatrix.h"
#include "utils/log.h"

extern "C"
{
#include <libswscale/swscale.h>
}

#include <algorithm>
#include <cstring>

using namespace KODI;
using namespace RETRO;

CRPRenderManager::CRPRenderManager(CRPProcessInfo& processInfo)
  : m_processInfo(processInfo),
    m_renderContext(processInfo.GetRenderContext()),
    m_renderSettings(new CGUIGameSettings(processInfo)),
    m_renderControlFactory(new CGUIRenderTargetFactory(this))
{
}

void CRPRenderManager::Initialize()
{
  CLog::Log(LOGDEBUG, "RetroPlayer[RENDER]: Initializing render manager");
}

void CRPRenderManager::Deinitialize()
{
  CLog::Log(LOGDEBUG, "RetroPlayer[RENDER]: Deinitializing render manager");

  for (auto& pixelScaler : m_scalers)
  {
    if (pixelScaler.second != nullptr)
      sws_freeContext(pixelScaler.second);
  }
  m_scalers.clear();

  for (auto renderBuffer : m_renderBuffers)
    renderBuffer->Release();
  m_renderBuffers.clear();

  for (auto buffer : m_pendingBuffers)
    buffer->Release();
  m_pendingBuffers.clear();

  m_renderers.clear();

  m_state = RENDER_STATE::UNCONFIGURED;
}

bool CRPRenderManager::Configure(AVPixelFormat format,
                                 unsigned int nominalWidth,
                                 unsigned int nominalHeight,
                                 unsigned int maxWidth,
                                 unsigned int maxHeight)
{
  CLog::Log(LOGINFO, "RetroPlayer[RENDER]: Configuring format {}, nominal {}x{}, max {}x{}",
            CRenderTranslator::TranslatePixelFormat(format), nominalWidth, nominalHeight, maxWidth,
            maxHeight);

  // Immutable parameters
  m_format = format;
  m_maxWidth = maxWidth;
  m_maxHeight = maxHeight;

  CSingleLock lock(m_stateMutex);

  m_state = RENDER_STATE::CONFIGURING;

  return true;
}

bool CRPRenderManager::GetVideoBuffer(
    unsigned int width, unsigned int height, AVPixelFormat& format, uint8_t*& data, size_t& size)
{
  for (IRenderBuffer* buffer : m_pendingBuffers)
    buffer->Release();
  m_pendingBuffers.clear();

  if (m_bFlush || m_state != RENDER_STATE::CONFIGURED)
    return false;

  // Get buffers from visible renderers
  for (IRenderBufferPool* bufferPool : m_processInfo.GetBufferManager().GetBufferPools())
  {
    if (!bufferPool->HasVisibleRenderer())
      continue;

    IRenderBuffer* renderBuffer = bufferPool->GetBuffer(width, height);
    if (renderBuffer != nullptr)
      m_pendingBuffers.emplace_back(renderBuffer);
    else
      CLog::Log(LOGDEBUG, "RetroPlayer[RENDER]: Unable to get video buffer for frame");
  }

  if (m_pendingBuffers.empty())
    return false;

  //! @todo Handle multiple buffers
  IRenderBuffer* renderBuffer = m_pendingBuffers.at(0);

  format = renderBuffer->GetFormat();
  data = renderBuffer->GetMemory();
  size = renderBuffer->GetFrameSize();

  return true;
}

void CRPRenderManager::AddFrame(const uint8_t* data,
                                size_t size,
                                unsigned int width,
                                unsigned int height,
                                unsigned int orientationDegCCW)
{
  if (m_bFlush || m_state != RENDER_STATE::CONFIGURED)
    return;

  // Validate parameters
  if (data == nullptr || size == 0 || width == 0 || height == 0)
    return;

  // Get render buffers to copy the frame into
  std::vector<IRenderBuffer*> renderBuffers;

  // Check pending buffers
  for (IRenderBuffer* buffer : m_pendingBuffers)
  {
    if (buffer->GetMemory() == data)
    {
      buffer->Acquire();
      renderBuffers.emplace_back(buffer);
    }
  }

  // If we aren't submitting a zero-copy frame, copy into render buffer now
  if (renderBuffers.empty())
  {
    // Copy frame to buffers with visible renderers
    for (IRenderBufferPool* bufferPool : m_processInfo.GetBufferManager().GetBufferPools())
    {
      if (!bufferPool->HasVisibleRenderer())
        continue;

      IRenderBuffer* renderBuffer = bufferPool->GetBuffer(width, height);
      if (renderBuffer != nullptr)
      {
        CopyFrame(renderBuffer, m_format, data, size, width, height);
        renderBuffers.emplace_back(renderBuffer);
      }
      else
        CLog::Log(LOGDEBUG, "RetroPlayer[RENDER]: Unable to get render buffer for frame");
    }
  }

  {
    CSingleLock lock(m_bufferMutex);

    // Set render buffers
    for (auto renderBuffer : m_renderBuffers)
      renderBuffer->Release();
    m_renderBuffers = std::move(renderBuffers);

    // Apply rotation to render buffers
    for (auto renderBuffer : m_renderBuffers)
      renderBuffer->SetRotation(orientationDegCCW);

    // Cache frame if it arrived after being paused
    if (m_speed == 0.0)
    {
      std::vector<uint8_t> cachedFrame = std::move(m_cachedFrame);

      if (!m_bHasCachedFrame)
      {
        // In this case, cachedFrame is definitely empty (see invariant for
        // m_bHasCachedFrame). Otherwise, cachedFrame may be empty if the frame
        // is being copied in the rendering thread. In that case, we would want
        // to leave cached frame empty to avoid caching another frame.

        cachedFrame.resize(size);
        m_bHasCachedFrame = true;
      }

      if (!cachedFrame.empty())
      {
        {
          CSingleExit exit(m_bufferMutex);
          std::memcpy(cachedFrame.data(), data, size);
        }
        m_cachedFrame = std::move(cachedFrame);
        m_cachedWidth = width;
        m_cachedHeight = height;
      }
    }
  }
}

void CRPRenderManager::SetSpeed(double speed)
{
  m_speed = speed;
}

void CRPRenderManager::FrameMove()
{
  CheckFlush();

  bool bIsConfigured = false;

  {
    CSingleLock lock(m_stateMutex);

    if (m_state == RENDER_STATE::CONFIGURING)
    {
      m_state = RENDER_STATE::CONFIGURED;

      CLog::Log(LOGINFO, "RetroPlayer[RENDER]: Renderer configured on first frame");
    }

    if (m_state == RENDER_STATE::CONFIGURED)
      bIsConfigured = true;
  }

  if (bIsConfigured)
  {
    for (auto& renderer : m_renderers)
      renderer->FrameMove();
  }
}

void CRPRenderManager::CheckFlush()
{
  if (m_bFlush)
  {
    {
      CSingleLock lock(m_bufferMutex);
      for (auto renderBuffer : m_renderBuffers)
        renderBuffer->Release();
      m_renderBuffers.clear();

      m_cachedFrame.clear();
      m_cachedWidth = 0;
      m_cachedHeight = 0;

      m_bHasCachedFrame = false;
    }

    for (const auto& renderer : m_renderers)
      renderer->Flush();

    m_processInfo.GetBufferManager().FlushPools();


    m_bFlush = false;
  }
}

void CRPRenderManager::Flush()
{
  m_bFlush = true;
}

void CRPRenderManager::RenderWindow(bool bClear, const RESOLUTION_INFO& coordsRes)
{
  // Get a renderer for the fullscreen window
  std::shared_ptr<CRPBaseRenderer> renderer = GetRenderer(nullptr);
  if (!renderer)
    return;

  // Get a render buffer for the renderer
  IRenderBuffer* renderBuffer = GetRenderBuffer(renderer->GetBufferPool());

  m_renderContext.SetRenderingResolution(m_renderContext.GetVideoResolution(), false);

  if (!m_bDisplayScaleSet && m_renderContext.DisplayHardwareScalingEnabled())
  {
    // If the renderer has a render buffer, get the dimensions
    const unsigned int sourceWidth = (renderBuffer != nullptr ? renderBuffer->GetWidth() : 0);
    const unsigned int sourceHeight = (renderBuffer != nullptr ? renderBuffer->GetHeight() : 0);

    // Get render video settings for the fullscreen window
    CRenderVideoSettings renderVideoSettings = GetEffectiveSettings(nullptr);

    // Get the scaling mode of the render video settings
    const SCALINGMETHOD scaleMode = renderVideoSettings.GetScalingMethod();
    const STRETCHMODE stretchMode = renderVideoSettings.GetRenderStretchMode();

    // Update display with video dimensions for integer scaling
    if (scaleMode == SCALINGMETHOD::NEAREST && stretchMode == STRETCHMODE::Original &&
        sourceWidth > 0 && sourceHeight > 0)
    {
      RESOLUTION_INFO gameRes = m_renderContext.GetResInfo();
      gameRes.Overscan.left = 0;
      gameRes.Overscan.top = 0;
      gameRes.Overscan.right = sourceWidth;
      gameRes.Overscan.bottom = sourceHeight;
      gameRes.iWidth = sourceWidth;
      gameRes.iHeight = sourceHeight;
      gameRes.iScreenWidth = sourceWidth;
      gameRes.iScreenHeight = sourceHeight;

      m_renderContext.UpdateDisplayHardwareScaling(gameRes);
      m_bDisplayScaleSet = true;
    }
  }

  RenderInternal(renderer, renderBuffer, bClear, 255);

  m_renderContext.SetRenderingResolution(coordsRes, false);
}

void CRPRenderManager::RenderControl(bool bClear,
                                     bool bUseAlpha,
                                     const CRect& renderRegion,
                                     const IGUIRenderSettings* renderSettings)
{
  // Get a renderer for the control
  std::shared_ptr<CRPBaseRenderer> renderer = GetRenderer(renderSettings);
  if (!renderer)
    return;

  // Get a render buffer for the renderer
  IRenderBuffer* renderBuffer = GetRenderBuffer(renderer->GetBufferPool());

  // Set fullscreen
  const bool bWasFullscreen = m_renderContext.IsFullScreenVideo();
  if (bWasFullscreen)
    m_renderContext.SetFullScreenVideo(false);

  // Set coordinates
  CRect coords = renderSettings->GetDimensions();
  m_renderContext.SetViewWindow(coords.x1, coords.y1, coords.x2, coords.y2);
  TransformMatrix mat;
  m_renderContext.SetTransform(mat, 1.0, 1.0);

  // Clear render area
  if (bClear)
  {
    CRect old = m_renderContext.GetScissors();
    CRect region = renderRegion;
    region.Intersect(old);
    m_renderContext.SetScissors(region);
    m_renderContext.Clear(0);
    m_renderContext.SetScissors(old);
  }

  // Calculate alpha
  UTILS::Color alpha = 255;
  if (bUseAlpha)
    alpha = m_renderContext.MergeAlpha(0xFF000000) >> 24;

  RenderInternal(renderer, renderBuffer, false, alpha);

  // Restore coordinates
  m_renderContext.RemoveTransform();

  // Restore fullscreen
  if (bWasFullscreen)
    m_renderContext.SetFullScreenVideo(true);
}

void CRPRenderManager::ClearBackground()
{
  m_renderContext.Clear(0);
}

bool CRPRenderManager::SupportsRenderFeature(RENDERFEATURE feature) const
{
  //! @todo Move to ProcessInfo
  for (const auto& renderer : m_renderers)
  {
    if (renderer->Supports(feature))
      return true;
  }

  return false;
}

bool CRPRenderManager::SupportsScalingMethod(SCALINGMETHOD method) const
{
  //! @todo Move to ProcessInfo
  for (IRenderBufferPool* bufferPool : m_processInfo.GetBufferManager().GetBufferPools())
  {
    CRenderVideoSettings renderSettings;
    renderSettings.SetScalingMethod(method);
    if (bufferPool->IsCompatible(renderSettings))
      return true;
  }

  return false;
}

void CRPRenderManager::RenderInternal(const std::shared_ptr<CRPBaseRenderer>& renderer,
                                      IRenderBuffer* renderBuffer,
                                      bool bClear,
                                      uint32_t alpha)
{
  renderer->PreRender(bClear);

  CSingleExit exitLock(m_renderContext.GraphicsMutex());

  if (renderBuffer != nullptr)
  {
    bool bUploaded = true;

    if (!renderBuffer->IsLoaded())
    {
      bUploaded = renderBuffer->UploadTexture();
      renderBuffer->SetLoaded(true);
    }

    if (bUploaded)
      renderer->SetBuffer(renderBuffer);

    renderBuffer->Release();
  }

  renderer->RenderFrame(bClear, alpha);
}

std::shared_ptr<CRPBaseRenderer> CRPRenderManager::GetRenderer(
    const IGUIRenderSettings* renderSettings)
{
  std::shared_ptr<CRPBaseRenderer> renderer;

  {
    CSingleLock lock(m_stateMutex);
    if (m_state == RENDER_STATE::UNCONFIGURED)
      return renderer;
  }

  CRenderSettings effectiveRenderSettings;
  effectiveRenderSettings.VideoSettings() = GetEffectiveSettings(renderSettings);

  // Check renderers in order of buffer pools
  for (IRenderBufferPool* bufferPool : m_processInfo.GetBufferManager().GetBufferPools())
  {
    renderer = GetRenderer(bufferPool, effectiveRenderSettings);
    if (renderer)
      break;
  }

  if (renderer)
  {
    renderer->SetScalingMethod(effectiveRenderSettings.VideoSettings().GetScalingMethod());
    renderer->SetStretchMode(effectiveRenderSettings.VideoSettings().GetRenderStretchMode());
    renderer->SetRenderRotation(effectiveRenderSettings.VideoSettings().GetRenderRotation());
  }

  return renderer;
}

std::shared_ptr<CRPBaseRenderer> CRPRenderManager::GetRenderer(
    IRenderBufferPool* bufferPool, const CRenderSettings& renderSettings)
{
  std::shared_ptr<CRPBaseRenderer> renderer;

  if (!bufferPool->IsCompatible(renderSettings.VideoSettings()))
  {
    CLog::Log(LOGERROR, "RetroPlayer[RENDER]: buffer pool is not compatible with renderer");
    return renderer;
  }

  // Get compatible renderer for this buffer pool
  for (const auto& it : m_renderers)
  {
    if (it->GetBufferPool() != bufferPool)
      continue;

    if (!it->IsCompatible(renderSettings.VideoSettings()))
      continue;

    renderer = it;
    break;
  }

  // If buffer pool has no compatible renderers, create one now
  if (!renderer)
  {
    CLog::Log(LOGERROR, "RetroPlayer[RENDER]: Creating renderer for {}",
              m_processInfo.GetRenderSystemName(bufferPool));

    renderer.reset(m_processInfo.CreateRenderer(bufferPool, renderSettings));
    if (renderer && renderer->Configure(m_format))
    {
      // Ensure we have a render buffer for this renderer
      CreateRenderBuffer(renderer->GetBufferPool());

      m_renderers.insert(renderer);
    }
    else
      renderer.reset();
  }

  return renderer;
}

bool CRPRenderManager::HasRenderBuffer(IRenderBufferPool* bufferPool)
{
  bool bHasRenderBuffer = false;

  CSingleLock lock(m_bufferMutex);

  auto it = std::find_if(
      m_renderBuffers.begin(), m_renderBuffers.end(),
      [bufferPool](IRenderBuffer* renderBuffer) { return renderBuffer->GetPool() == bufferPool; });

  if (it != m_renderBuffers.end())
    bHasRenderBuffer = true;

  return bHasRenderBuffer;
}

IRenderBuffer* CRPRenderManager::GetRenderBuffer(IRenderBufferPool* bufferPool)
{
  if (m_bFlush || m_state != RENDER_STATE::CONFIGURED)
    return nullptr;

  IRenderBuffer* renderBuffer = nullptr;

  CSingleLock lock(m_bufferMutex);

  auto getRenderBuffer = [bufferPool](IRenderBuffer* renderBuffer) {
    return renderBuffer->GetPool() == bufferPool;
  };

  auto it = std::find_if(m_renderBuffers.begin(), m_renderBuffers.end(), getRenderBuffer);

  // If our renderer has no buffer, try to create one from paused frame now
  if (it == m_renderBuffers.end())
  {
    CreateRenderBuffer(bufferPool);
    it = std::find_if(m_renderBuffers.begin(), m_renderBuffers.end(), getRenderBuffer);
  }

  if (it != m_renderBuffers.end())
  {
    renderBuffer = *it;
    renderBuffer->Acquire();
  }

  return renderBuffer;
}

void CRPRenderManager::CreateRenderBuffer(IRenderBufferPool* bufferPool)
{
  if (m_bFlush || m_state != RENDER_STATE::CONFIGURED)
    return;

  CSingleLock lock(m_bufferMutex);

  if (!HasRenderBuffer(bufferPool) && m_bHasCachedFrame)
  {
    IRenderBuffer* renderBuffer =
        CreateFromCache(m_cachedFrame, m_cachedWidth, m_cachedHeight, bufferPool, m_bufferMutex);
    if (renderBuffer != nullptr)
      m_renderBuffers.emplace_back(renderBuffer);
  }
}

IRenderBuffer* CRPRenderManager::CreateFromCache(std::vector<uint8_t>& cachedFrame,
                                                 unsigned int width,
                                                 unsigned int height,
                                                 IRenderBufferPool* bufferPool,
                                                 CCriticalSection& mutex)
{
  // Take ownership of cached frame
  std::vector<uint8_t> ownedFrame = std::move(cachedFrame);

  if (!ownedFrame.empty())
  {
    CLog::Log(LOGERROR, "RetroPlayer[RENDER]: Creating render buffer for renderer");

    IRenderBuffer* renderBuffer = bufferPool->GetBuffer(width, height);
    if (renderBuffer != nullptr)
    {
      CSingleExit exit(mutex);
      CopyFrame(renderBuffer, m_format, ownedFrame.data(), ownedFrame.size(), width, height);
    }

    // Return ownership of cached frame
    cachedFrame = std::move(ownedFrame);

    return renderBuffer;
  }
  else
  {
    CLog::Log(LOGERROR, "RetroPlayer[RENDER]: Failed to create render buffer, no cached frame");
  }

  return nullptr;
}

void CRPRenderManager::CopyFrame(IRenderBuffer* renderBuffer,
                                 AVPixelFormat format,
                                 const uint8_t* data,
                                 size_t size,
                                 unsigned int width,
                                 unsigned int height)
{
  const uint8_t* source = data;
  uint8_t* target = renderBuffer->GetMemory();

  if (target != nullptr)
  {
    const unsigned int sourceStride = static_cast<unsigned int>(size / height);
    const unsigned int targetStride =
        static_cast<unsigned int>(renderBuffer->GetFrameSize() / renderBuffer->GetHeight());

    if (m_format == renderBuffer->GetFormat())
    {
      if (sourceStride == targetStride)
        std::memcpy(target, source, size);
      else
      {
        const unsigned int widthBytes = CRenderTranslator::TranslateWidthToBytes(width, m_format);
        if (widthBytes > 0)
        {
          for (unsigned int i = 0; i < height; i++)
            std::memcpy(target + targetStride * i, source + sourceStride * i, widthBytes);
        }
      }
    }
    else
    {
      SwsContext*& scalerContext = m_scalers[renderBuffer->GetFormat()];
      scalerContext = sws_getCachedContext(
          scalerContext, width, height, format, renderBuffer->GetWidth(), renderBuffer->GetHeight(),
          renderBuffer->GetFormat(), SWS_FAST_BILINEAR, nullptr, nullptr, nullptr);

      if (scalerContext != nullptr)
      {
        uint8_t* src[] = {const_cast<uint8_t*>(source), nullptr, nullptr, nullptr};
        int srcStride[] = {static_cast<int>(sourceStride), 0, 0, 0};
        uint8_t* dst[] = {target, nullptr, nullptr, nullptr};
        int dstStride[] = {static_cast<int>(targetStride), 0, 0, 0};

        sws_scale(scalerContext, src, srcStride, 0, height, dst, dstStride);
      }
    }
  }

  renderBuffer->ReleaseMemory();
}

CRenderVideoSettings CRPRenderManager::GetEffectiveSettings(
    const IGUIRenderSettings* settings) const
{
  CRenderVideoSettings effectiveSettings = m_renderSettings->GetSettings().VideoSettings();

  if (settings != nullptr)
  {
    if (settings->HasVideoFilter())
      effectiveSettings.SetVideoFilter(settings->GetSettings().VideoSettings().GetVideoFilter());
    if (settings->HasStretchMode())
      effectiveSettings.SetRenderStretchMode(
          settings->GetSettings().VideoSettings().GetRenderStretchMode());
    if (settings->HasRotation())
      effectiveSettings.SetRenderRotation(
          settings->GetSettings().VideoSettings().GetRenderRotation());
  }

  // Sanitize settings
  if (!m_processInfo.HasScalingMethod(effectiveSettings.GetScalingMethod()))
  {
    effectiveSettings.SetScalingMethod(m_processInfo.GetDefaultScalingMethod());
  }

  return effectiveSettings;
}
