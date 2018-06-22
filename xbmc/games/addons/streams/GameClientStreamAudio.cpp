/*
 *      Copyright (C) 2018 Team Kodi
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
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
