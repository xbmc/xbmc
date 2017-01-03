/*
 *      Copyright (C) 2012-2016 Team Kodi
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

#include "RetroPlayerAudio.h"
#include "RetroPlayerDefines.h"
#include "cores/AudioEngine/AEFactory.h"
#include "cores/AudioEngine/Interfaces/AEStream.h"
#include "cores/AudioEngine/Utils/AEChannelInfo.h"
#include "cores/AudioEngine/Utils/AEUtil.h"
#include "cores/VideoPlayer/DVDCodecs/Audio/DVDAudioCodec.h"
#include "cores/VideoPlayer/DVDCodecs/DVDFactoryCodec.h"
#include "cores/VideoPlayer/DVDDemuxers/DVDDemux.h"
#include "cores/VideoPlayer/DVDClock.h"
#include "cores/VideoPlayer/DVDStreamInfo.h"
#include "cores/VideoPlayer/Process/ProcessInfo.h"
#include "threads/Thread.h"
#include "utils/log.h"

using namespace GAME;

CRetroPlayerAudio::CRetroPlayerAudio(CProcessInfo& processInfo) :
  m_processInfo(processInfo),
  m_pAudioStream(nullptr),
  m_bAudioEnabled(true)
{
}

CRetroPlayerAudio::~CRetroPlayerAudio()
{
  CloseStream(); 
}

unsigned int CRetroPlayerAudio::NormalizeSamplerate(unsigned int samplerate) const
{
  //! @todo List comes from AESinkALSA.cpp many moons ago
  static unsigned int sampleRateList[] = { 5512, 8000, 11025, 16000, 22050, 32000, 44100, 48000, 0 };

  for (unsigned int *rate = sampleRateList; ; rate++)
  {
    const unsigned int thisValue = *rate;
    const unsigned int nextValue = *(rate + 1);

    if (nextValue == 0)
    {
      // Reached the end of our list
      return thisValue;
    }

    if (samplerate < (thisValue + nextValue) / 2)
    {
      // samplerate is between this rate and the next, so use this rate
      return thisValue;
    }
  }

  return samplerate; // Shouldn't happen
}

bool CRetroPlayerAudio::OpenPCMStream(AEDataFormat format, unsigned int samplerate, const CAEChannelInfo& channelLayout)
{
  if (m_pAudioStream != nullptr)
    CloseStream();

  CLog::Log(LOGINFO, "RetroPlayerAudio: Creating audio stream, sample rate = %d", samplerate);

  // Resampling is not supported
  if (NormalizeSamplerate(samplerate) != samplerate)
  {
    CLog::Log(LOGERROR, "RetroPlayerAudio: Resampling to %d not supported", NormalizeSamplerate(samplerate));
    return false;
  }

  AEAudioFormat audioFormat;
  audioFormat.m_dataFormat = format;
  audioFormat.m_sampleRate = samplerate;
  audioFormat.m_channelLayout = channelLayout;
  m_pAudioStream = CAEFactory::MakeStream(audioFormat);

  if (!m_pAudioStream)
  {
    CLog::Log(LOGERROR, "RetroPlayerAudio: Failed to create audio stream");
    return false;
  }

  return true;
}

bool CRetroPlayerAudio::OpenEncodedStream(AVCodecID codec, unsigned int samplerate, const CAEChannelInfo& channelLayout)
{
  CDemuxStreamAudio audioStream;

  // Stream
  audioStream.uniqueId = GAME_STREAM_AUDIO_ID;
  audioStream.codec = codec;
  audioStream.type = STREAM_AUDIO;
  audioStream.source = STREAM_SOURCE_DEMUX;
  audioStream.realtime = true;

  // Audio
  audioStream.iChannels = channelLayout.Count();
  audioStream.iSampleRate = samplerate;
  audioStream.iChannelLayout = CAEUtil::GetAVChannelLayout(channelLayout);

  CDVDStreamInfo hint(audioStream);
  m_pAudioCodec.reset(CDVDFactoryCodec::CreateAudioCodec(hint, m_processInfo, false));

  if (!m_pAudioCodec)
  {
    CLog::Log(LOGERROR, "RetroPlayerAudio: Failed to create audio codec (codec=%d, samplerate=%u)", codec, samplerate);
    return false;
  }

  return true;
}

void CRetroPlayerAudio::AddData(const uint8_t* data, unsigned int size)
{
  if (m_bAudioEnabled)
  {
    if (m_pAudioCodec)
    {
      int consumed = m_pAudioCodec->Decode(const_cast<uint8_t*>(data), size, DVD_NOPTS_VALUE, DVD_NOPTS_VALUE);
      if (consumed < 0)
      {
        CLog::Log(LOGERROR, "CRetroPlayerAudio::AddData - Decode Error (%d)", consumed);
        m_pAudioCodec.reset();
        return;
      }

      DVDAudioFrame audioframe;
      m_pAudioCodec->GetData(audioframe);

      if (audioframe.nb_frames != 0)
      {
        // Open audio stream if not already open
        if (!m_pAudioStream)
        {
          const AEAudioFormat& format = audioframe.format;
          if (!OpenPCMStream(format.m_dataFormat, format.m_sampleRate, format.m_channelLayout))
            m_pAudioCodec.reset();
        }

        if (m_pAudioStream)
          m_pAudioStream->AddData(audioframe.data, 0, audioframe.nb_frames);
      }
    }
    else if (m_pAudioStream)
    {
      const unsigned int frameSize = m_pAudioStream->GetChannelCount() * (CAEUtil::DataFormatToBits(m_pAudioStream->GetDataFormat()) >> 3);
      m_pAudioStream->AddData(&data, 0, size / frameSize);
    }
  }
}

void CRetroPlayerAudio::CloseStream()
{
  if (m_pAudioCodec)
  {
    m_pAudioCodec->Dispose();
    m_pAudioCodec.reset();
  }
  if (m_pAudioStream)
  {
    CAEFactory::FreeStream(m_pAudioStream);
    m_pAudioStream = nullptr;
  }
}
