/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "VideoPlayerWebOS.h"

#include "MediaPipelineWebOS.h"
#include "VideoPlayerAudioWebOS.h"
#include "VideoPlayerVideoWebOS.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"

#include <algorithm>

CVideoPlayerWebOS::CVideoPlayerWebOS(IPlayerCallback& callback) : CVideoPlayer(callback)
{
}

CVideoPlayerWebOS::~CVideoPlayerWebOS() = default;

void CVideoPlayerWebOS::CreatePlayers()
{
  const auto canStarfish =
      std::ranges::any_of(m_SelectionStreams.Get(StreamType::VIDEO),
                          [](const auto& stream)
                          {
                            if (stream.codecId != AV_CODEC_ID_NONE)
                              return CMediaPipelineWebOS::Supports(stream.codecId, stream.profile);
                            const AVCodec* codec =
                                avcodec_find_decoder_by_name(stream.codec.data());
                            if (!codec)
                              return false;

                            return CMediaPipelineWebOS::Supports(codec->id, stream.profile);
                          });

  if (canStarfish && CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
                         CSettings::SETTING_VIDEOPLAYER_USESTARFISHDECODER))
  {
    const bool hasAudio = m_SelectionStreams.CountType(StreamType::AUDIO);
    const bool subtitlesEnabled = m_VideoPlayerVideo->IsSubtitleEnabled();
    const double subtitleDelay = m_VideoPlayerVideo->GetSubtitleDelay();

    if (!m_mediaPipelineWebOS)
    {
      m_mediaPipelineWebOS = std::make_unique<CMediaPipelineWebOS>(
          *m_processInfo, m_renderManager, m_clock, m_messenger, m_overlayContainer, hasAudio);
      m_VideoPlayerVideo =
          std::make_unique<CVideoPlayerVideoWebOS>(*m_mediaPipelineWebOS, *m_processInfo);
      m_VideoPlayerAudio =
          std::make_unique<CVideoPlayerAudioWebOS>(*m_mediaPipelineWebOS, *m_processInfo);
      m_VideoPlayerVideo->EnableSubtitle(subtitlesEnabled);
      m_VideoPlayerVideo->SetSubtitleDelay(subtitleDelay);
    }
  }
  else if (m_mediaPipelineWebOS || (!m_VideoPlayerVideo && !m_VideoPlayerAudio))
  {
    m_mediaPipelineWebOS = nullptr;
    m_VideoPlayerVideo = std::make_unique<CVideoPlayerVideo>(
        &m_clock, &m_overlayContainer, m_messenger, m_renderManager, *m_processInfo,
        m_messageQueueTimeSize);
    m_VideoPlayerAudio = std::make_unique<CVideoPlayerAudio>(&m_clock, m_messenger, *m_processInfo,
                                                             m_messageQueueTimeSize);
  }

  if (m_players_created)
    return;

  m_VideoPlayerSubtitle =
      std::make_unique<CVideoPlayerSubtitle>(&m_overlayContainer, *m_processInfo);
  m_VideoPlayerTeletext = std::make_unique<CDVDTeletextData>(*m_processInfo);
  m_VideoPlayerRadioRDS = std::make_unique<CDVDRadioRDSData>(*m_processInfo);
  m_VideoPlayerAudioID3 = std::make_unique<CVideoPlayerAudioID3>(*m_processInfo);
  m_players_created = true;
}

void CVideoPlayerWebOS::GetVideoResolution(unsigned int& width, unsigned int& height)
{
  if (m_mediaPipelineWebOS)
  {
    m_mediaPipelineWebOS->GetVideoResolution(width, height);
  }
  else
    CVideoPlayer::GetVideoResolution(width, height);
}

void CVideoPlayerWebOS::UpdateContent()
{
  CVideoPlayer::UpdateContent();
  CreatePlayers();
}
