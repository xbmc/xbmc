/*
 *  Copyright (C) 2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GameClientStreamVideo.h"

#include "cores/RetroPlayer/streams/RetroPlayerVideo.h"
#include "games/addons/GameClientTranslator.h"
#include "utils/log.h"

using namespace KODI;
using namespace GAME;

bool CGameClientStreamVideo::OpenStream(RETRO::IRetroPlayerStream* stream,
                                        const game_stream_properties& properties)
{
  RETRO::CRetroPlayerVideo* videoStream = dynamic_cast<RETRO::CRetroPlayerVideo*>(stream);
  if (videoStream == nullptr)
  {
    CLog::Log(LOGERROR, "GAME: RetroPlayer stream is not a video stream");
    return false;
  }

  std::unique_ptr<RETRO::VideoStreamProperties> videoProperties(
      TranslateProperties(properties.video));
  if (videoProperties)
  {
    if (videoStream->OpenStream(static_cast<const RETRO::StreamProperties&>(*videoProperties)))
      m_stream = stream;
  }

  return m_stream != nullptr;
}

void CGameClientStreamVideo::CloseStream()
{
  if (m_stream != nullptr)
  {
    m_stream->CloseStream();
    m_stream = nullptr;
  }
}

void CGameClientStreamVideo::AddData(const game_stream_packet& packet)
{
  if (packet.type != GAME_STREAM_VIDEO && packet.type != GAME_STREAM_SW_FRAMEBUFFER)
    return;

  if (m_stream != nullptr)
  {
    const game_stream_video_packet& video = packet.video;

    RETRO::VideoRotation rotation = CGameClientTranslator::TranslateRotation(video.rotation);

    RETRO::VideoStreamPacket videoPacket{
        video.width, video.height, rotation, video.data, video.size,
    };

    m_stream->AddStreamData(static_cast<const RETRO::StreamPacket&>(videoPacket));
  }
}

RETRO::VideoStreamProperties* CGameClientStreamVideo::TranslateProperties(
    const game_stream_video_properties& properties)
{
  const AVPixelFormat pixelFormat = CGameClientTranslator::TranslatePixelFormat(properties.format);
  if (pixelFormat == AV_PIX_FMT_NONE)
  {
    CLog::Log(LOGERROR, "GAME: Unknown pixel format: {}", properties.format);
    return nullptr;
  }

  const unsigned int nominalWidth = properties.nominal_width;
  const unsigned int nominalHeight = properties.nominal_height;
  if (nominalWidth == 0 || nominalHeight == 0)
  {
    CLog::Log(LOGERROR, "GAME: Invalid nominal dimensions: {}x{}", nominalWidth, nominalHeight);
    return nullptr;
  }

  const unsigned int maxWidth = properties.max_width;
  const unsigned int maxHeight = properties.max_height;
  if (maxWidth == 0 || maxHeight == 0)
  {
    CLog::Log(LOGERROR, "GAME: Invalid max dimensions: {}x{}", maxWidth, maxHeight);
    return nullptr;
  }

  if (nominalWidth > maxWidth || nominalHeight > maxHeight)
    CLog::Log(LOGERROR, "GAME: Nominal dimensions ({}x{}) bigger than max dimensions ({}x{})",
              nominalWidth, nominalHeight, maxWidth, maxHeight);

  float pixelAspectRatio;

  // Game API: If aspect_ratio is <= 0.0, an aspect ratio of
  // (nominal_width / nominal_height) is assumed
  if (properties.aspect_ratio <= 0.0f)
    pixelAspectRatio = 1.0f;
  else
    pixelAspectRatio = properties.aspect_ratio * nominalHeight / nominalWidth;

  return new RETRO::VideoStreamProperties{pixelFormat, nominalWidth, nominalHeight,
                                          maxWidth,    maxHeight,    pixelAspectRatio};
}
