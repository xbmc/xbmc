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
                            const AVCodec* codec =
                                avcodec_find_decoder_by_name(stream.codec.data());
                            if (!codec)
                              return false;

                            return CMediaPipelineWebOS::Supports(codec->id);
                          });

  if (canStarfish && CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
                         CSettings::SETTING_VIDEOPLAYER_USESTARFISHDECODER))
  {
    bool hasAudio = m_SelectionStreams.CountType(StreamType::AUDIO);

    delete m_VideoPlayerVideo;
    delete m_VideoPlayerAudio;
    m_mediaPipelineWebOS = std::make_unique<CMediaPipelineWebOS>(
        *m_processInfo, m_renderManager, m_clock, m_messenger, m_overlayContainer, hasAudio);
    m_VideoPlayerVideo = new CVideoPlayerVideoWebOS(*m_mediaPipelineWebOS, *m_processInfo);
    m_VideoPlayerAudio = new CVideoPlayerAudioWebOS(*m_mediaPipelineWebOS, *m_processInfo);

    const CVideoSettings settings = m_processInfo->GetVideoSettings();
    m_VideoPlayerVideo->EnableSubtitle(settings.m_SubtitleOn);
    m_VideoPlayerVideo->SetSubtitleDelay(
        static_cast<int>(-settings.m_SubtitleDelay * DVD_TIME_BASE));
  }
  else if (m_mediaPipelineWebOS || (!m_VideoPlayerVideo && !m_VideoPlayerAudio))
  {
    delete m_VideoPlayerVideo;
    delete m_VideoPlayerAudio;
    m_mediaPipelineWebOS = nullptr;
    m_VideoPlayerVideo =
        new CVideoPlayerVideo(&m_clock, &m_overlayContainer, m_messenger, m_renderManager,
                              *m_processInfo, m_messageQueueTimeSize);
    m_VideoPlayerAudio =
        new CVideoPlayerAudio(&m_clock, m_messenger, *m_processInfo, m_messageQueueTimeSize);
  }

  if (m_players_created)
    return;

  m_VideoPlayerSubtitle = new CVideoPlayerSubtitle(&m_overlayContainer, *m_processInfo);
  m_VideoPlayerTeletext = new CDVDTeletextData(*m_processInfo);
  m_VideoPlayerRadioRDS = new CDVDRadioRDSData(*m_processInfo);
  m_VideoPlayerAudioID3 = std::make_unique<CVideoPlayerAudioID3>(*m_processInfo);
  m_players_created = true;
}
