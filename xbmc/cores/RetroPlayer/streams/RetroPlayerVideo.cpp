/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RetroPlayerVideo.h"

#include "cores/RetroPlayer/process/RPProcessInfo.h"
#include "cores/RetroPlayer/rendering/RPRenderManager.h"
#include "cores/RetroPlayer/rendering/RenderTranslator.h"
#include "utils/log.h"

using namespace KODI;
using namespace RETRO;

CRetroPlayerVideo::CRetroPlayerVideo(CRPRenderManager& renderManager, CRPProcessInfo& processInfo)
  : m_renderManager(renderManager), m_processInfo(processInfo)
{
  CLog::Log(LOGDEBUG, "RetroPlayer[VIDEO]: Initializing video");

  m_renderManager.Initialize();
}

CRetroPlayerVideo::~CRetroPlayerVideo()
{
  CLog::Log(LOGDEBUG, "RetroPlayer[VIDEO]: Deinitializing video");

  CloseStream();
  m_renderManager.Deinitialize();
}

bool CRetroPlayerVideo::OpenStream(const StreamProperties& properties)
{
  const VideoStreamProperties& videoProperties =
      static_cast<const VideoStreamProperties&>(properties);

  if (m_bOpen)
  {
    CloseStream();
    m_bOpen = false;
  }

  const AVPixelFormat pixfmt = videoProperties.pixfmt;
  const unsigned int nominalWidth = videoProperties.nominalWidth;
  const unsigned int nominalHeight = videoProperties.nominalHeight;
  const unsigned int maxWidth = videoProperties.maxWidth;
  const unsigned int maxHeight = videoProperties.maxHeight;
  const float pixelAspectRatio = videoProperties.pixelAspectRatio;

  CLog::Log(LOGDEBUG,
            "RetroPlayer[VIDEO]: Creating video stream - format {}, nominal {}x{}, max {}x{}",
            CRenderTranslator::TranslatePixelFormat(pixfmt), nominalWidth, nominalHeight, maxWidth,
            maxHeight);

  m_processInfo.SetVideoPixelFormat(pixfmt);
  m_processInfo.SetVideoDimensions(nominalWidth, nominalHeight); // Report nominal height for now

  if (m_renderManager.Configure(pixfmt, nominalWidth, nominalHeight, maxWidth, maxHeight,
                                pixelAspectRatio))
    m_bOpen = true;

  return m_bOpen;
}

bool CRetroPlayerVideo::GetStreamBuffer(unsigned int width,
                                        unsigned int height,
                                        StreamBuffer& buffer)
{
  VideoStreamBuffer& videoBuffer = static_cast<VideoStreamBuffer&>(buffer);

  if (m_bOpen)
    return m_renderManager.GetVideoBuffer(width, height, videoBuffer);

  return false;
}

void CRetroPlayerVideo::AddStreamData(const StreamPacket& packet)
{
  const VideoStreamPacket& videoPacket = static_cast<const VideoStreamPacket&>(packet);

  if (m_bOpen)
  {
    unsigned int orientationDegCCW = 0;
    switch (videoPacket.rotation)
    {
      case VideoRotation::ROTATION_90_CCW:
        orientationDegCCW = 90;
        break;
      case VideoRotation::ROTATION_180_CCW:
        orientationDegCCW = 180;
        break;
      case VideoRotation::ROTATION_270_CCW:
        orientationDegCCW = 270;
        break;
      default:
        break;
    }

    m_renderManager.AddFrame(videoPacket.data, videoPacket.size, videoPacket.width,
                             videoPacket.height, orientationDegCCW);
  }
}

void CRetroPlayerVideo::CloseStream()
{
  if (m_bOpen)
  {
    CLog::Log(LOGDEBUG, "RetroPlayer[VIDEO]: Closing video stream");

    m_renderManager.Flush();
    m_bOpen = false;
  }
}
