/*
 *  Copyright (C) 2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GameClientStreamAudio.h"
#include "cores/RetroPlayer/streams/RetroPlayerAudio.h"
#include "games/addons/GameClientTranslator.h"
#include "utils/log.h"

using namespace KODI;
using namespace GAME;

CGameClientStreamAudio::CGameClientStreamAudio(double sampleRate) :
  m_sampleRate(sampleRate)
{
}

bool CGameClientStreamAudio::OpenStream(RETRO::IRetroPlayerStream* stream, const game_stream_properties& properties)
{
  RETRO::CRetroPlayerAudio* audioStream = dynamic_cast<RETRO::CRetroPlayerAudio*>(stream);
  if (audioStream == nullptr)
  {
    CLog::Log(LOGERROR, "GAME: RetroPlayer stream is not an audio stream");
    return false;
  }

  std::unique_ptr<RETRO::AudioStreamProperties> audioProperties(TranslateProperties(properties.audio, m_sampleRate));
  if (audioProperties)
  {
    if (audioStream->OpenStream(static_cast<const RETRO::StreamProperties&>(*audioProperties)))
      m_stream = stream;
  }

  return m_stream != nullptr;
}

void CGameClientStreamAudio::CloseStream()
{
  if (m_stream != nullptr)
  {
    m_stream->CloseStream();
    m_stream = nullptr;
  }
}

void CGameClientStreamAudio::AddData(const game_stream_packet &packet)
{
  if (packet.type != GAME_STREAM_AUDIO)
    return;

  if (m_stream != nullptr)
  {
    const game_stream_audio_packet &audio = packet.audio;

    RETRO::AudioStreamPacket audioPacket{
      audio.data,
      audio.size
    };

    m_stream->AddStreamData(static_cast<RETRO::StreamPacket&>(audioPacket));
  }
}

RETRO::AudioStreamProperties* CGameClientStreamAudio::TranslateProperties(const game_stream_audio_properties &properties, double sampleRate)
{
  const RETRO::PCMFormat pcmFormat = CGameClientTranslator::TranslatePCMFormat(properties.format);
  if (pcmFormat == RETRO::PCMFormat::FMT_UNKNOWN)
  {
    CLog::Log(LOGERROR, "GAME: Unknown PCM format: %d", static_cast<int>(properties.format));
    return nullptr;
  }

  RETRO::AudioChannelMap channelMap = { { RETRO::AudioChannel::CH_NULL } };
  unsigned int i = 0;
  if (properties.channel_map != nullptr)
  {
    for (const GAME_AUDIO_CHANNEL* channelPtr = properties.channel_map;
      *channelPtr != GAME_CH_NULL;
      channelPtr++)
    {
      RETRO::AudioChannel channel = CGameClientTranslator::TranslateAudioChannel(*channelPtr);
      if (channel == RETRO::AudioChannel::CH_NULL)
      {
        CLog::Log(LOGERROR, "GAME: Unknown channel ID: %d", static_cast<int>(*channelPtr));
        return nullptr;
      }

      channelMap[i++] = channel;
      if (i + 1 >= channelMap.size())
        break;
    }
  }
  channelMap[i] = RETRO::AudioChannel::CH_NULL;

  if (channelMap[0] == RETRO::AudioChannel::CH_NULL)
  {
    CLog::Log(LOGERROR, "GAME: Empty channel layout");
    return nullptr;
  }

  return new RETRO::AudioStreamProperties{
    pcmFormat,
    sampleRate,
    channelMap
  };
}
