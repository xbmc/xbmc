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
  const HwFramebufferProperties& hwProperties =
      static_cast<const HwFramebufferProperties&>(properties);

  const AVPixelFormat pixfmt = hwProperties.pixfmt;
  const unsigned int nominalWidth = hwProperties.nominalWidth;
  const unsigned int nominalHeight = hwProperties.nominalHeight;
  const unsigned int maxWidth = hwProperties.maxWidth;
  const unsigned int maxHeight = hwProperties.maxHeight;
  const float pixelAspectRatio = hwProperties.pixelAspectRatio;

  CLog::Log(
      LOGDEBUG,
      "RetroPlayer[RENDERING]: Creating rendering stream - format {}, nominal {}x{}, max {}x{}",
      CRenderTranslator::TranslatePixelFormat(pixfmt), nominalWidth, nominalHeight, maxWidth,
      maxHeight);

  m_processInfo.SetVideoPixelFormat(pixfmt);
  m_processInfo.SetVideoDimensions(nominalWidth, nominalHeight); // Report nominal height for now

  if (!m_renderManager.Configure(pixfmt, nominalWidth, nominalHeight, maxWidth, maxHeight,
                                 pixelAspectRatio))
    return false;

  return m_renderManager.Create(maxWidth, maxHeight);
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
