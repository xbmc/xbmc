/*
 *  Copyright (C) 2017-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RetroPlayerRendering.h"

#include "cores/RetroPlayer/process/RPProcessInfo.h"
#include "cores/RetroPlayer/rendering/RPRenderManager.h"
#include "cores/RetroPlayer/rendering/RenderTranslator.h"
#include "utils/log.h"

using namespace KODI;
using namespace RETRO;

CRetroPlayerRendering::CRetroPlayerRendering(CRPRenderManager& renderManager,
                                             CRPProcessInfo& processInfo)
  : m_renderManager(renderManager),
    m_processInfo(processInfo)
{
  CLog::Log(LOGDEBUG, "RetroPlayer[RENDERING]: Initializing rendering");
}

CRetroPlayerRendering::~CRetroPlayerRendering()
{
  CLog::Log(LOGDEBUG, "RetroPlayer[RENDERING]: Deinitializing rendering");

  CloseStream();
  m_renderManager.Deinitialize();
}

bool CRetroPlayerRendering::OpenStream(const StreamProperties& properties)
{
  const HwFramebufferProperties& hwProperties =
      static_cast<const HwFramebufferProperties&>(properties);

  // For hardware rendering, pixel data is managed by the GPU directly.
  // The pixel format is AV_PIX_FMT_NONE to signal GPU-direct rendering.
  const AVPixelFormat pixelFormat = AV_PIX_FMT_NONE;

  // Use conservative default dimensions for the initial configuration.
  // The game core will provide the actual dimensions via GetStreamBuffer callbacks.
  const unsigned int width = 640;
  const unsigned int height = 480;
  const float displayAspectRatio = 0.0f; // 0.0f means square pixels

  CLog::Log(LOGDEBUG,
            "RetroPlayer[RENDERING]: Opening hardware rendering stream - context type {}, "
            "version {}.{}, depth={}, stencil={}",
            static_cast<int>(hwProperties.contextType), hwProperties.versionMajor,
            hwProperties.versionMinor, hwProperties.depth, hwProperties.stencil);

  m_contextType = hwProperties.contextType;

  // Forward the requested depth/stencil attachments so the hardware framebuffer
  // is created with them (3D cores need a depth buffer for correct occlusion).
  m_renderManager.SetHardwareContext(hwProperties.depth, hwProperties.stencil);

  m_processInfo.SetVideoPixelFormat(pixelFormat);
  m_processInfo.SetVideoDimensions(width, height);

  if (!m_renderManager.Configure(pixelFormat, width, height, displayAspectRatio, width, height))
  {
    CLog::Log(LOGERROR,
              "RetroPlayer[RENDERING]: Failed to configure render manager for hardware rendering");
    return false;
  }

  CLog::Log(LOGDEBUG, "RetroPlayer[RENDERING]: Hardware rendering stream opened successfully");

  // The GL framebuffer (Create) must be called from the rendering thread.
  // CRPRenderManager::FrameMove() detects hardware rendering via AV_PIX_FMT_NONE
  // and calls Create() at the appropriate time on the render thread.
  return true;
}

void CRetroPlayerRendering::CloseStream()
{
  CLog::Log(LOGDEBUG, "RetroPlayer[RENDERING]: Closing hardware rendering stream");

  m_renderManager.Flush();
}

bool CRetroPlayerRendering::GetStreamBuffer(unsigned int width,
                                            unsigned int height,
                                            StreamBuffer& buffer)
{
  HwFramebufferBuffer& hwBuffer = static_cast<HwFramebufferBuffer&>(buffer);

  hwBuffer.framebuffer = m_renderManager.GetCurrentFramebuffer(width, height);

  return true;
}

void CRetroPlayerRendering::AddStreamData(const StreamPacket& packet)
{
  // This is left here in case anything gets added to the api in the future
  [[maybe_unused]] const HwFramebufferPacket& hwPacket =
      static_cast<const HwFramebufferPacket&>(packet);

  m_renderManager.RenderFrame();
}
