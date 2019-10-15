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
  : m_renderManager(renderManager), m_processInfo(processInfo)
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
  [[maybe_unused]] const HwFramebufferProperties& hwProperties =
      static_cast<const HwFramebufferProperties&>(properties);

  //! @todo
  const AVPixelFormat pixelFormat = AV_PIX_FMT_NONE;
  const unsigned int width = 640;
  const unsigned int height = 480;
  const float pixelAspectRatio = 1.0f;

  CLog::Log(LOGDEBUG, "RetroPlayer[RENDERING]: Creating rendering stream - width {}, height {}",
            width, height);

  m_processInfo.SetVideoPixelFormat(pixelFormat);
  m_processInfo.SetVideoDimensions(width, height);

  if (!m_renderManager.Configure(pixelFormat, width, height, width, height, pixelAspectRatio))
    return false;

  CLog::Log(LOGDEBUG, "RetroPlayer[RENDERING]: Render manager configured");

  //! @todo: This must be called from the rendering thread
  //return m_renderManager.Create(width, height);

  return false;
}

void CRetroPlayerRendering::CloseStream()
{
  CLog::Log(LOGDEBUG, "RetroPlayer[RENDERING]: Closing rendering stream");

  //! @todo
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
