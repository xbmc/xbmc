/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RetroPlayerAudio.h"

#include "ServiceBroker.h"
#include "cores/AudioEngine/Interfaces/AE.h"
#include "cores/AudioEngine/Interfaces/AEStream.h"
#include "cores/AudioEngine/Utils/AEChannelInfo.h"
#include "cores/AudioEngine/Utils/AEUtil.h"
#include "cores/RetroPlayer/audio/AudioTranslator.h"
#include "cores/RetroPlayer/process/RPProcessInfo.h"
#include "utils/log.h"

#include <algorithm>
#include <chrono>
#include <thread>

#include <cmath>

using namespace KODI;
using namespace RETRO;

const double MAX_DELAY = 0.3; // seconds

// How long a single packet may wait before it is given up on. Long enough to
// ride out a sink that is merely busy, short enough that a sink which has
// stopped draining costs the game loop one frame rather than hanging it.
constexpr auto MAX_WAIT = std::chrono::milliseconds(100);

// How much audio to keep queued ahead of the speakers. This has to sit above
// AudioEngine's own buffering floor, or the delay can never reach it and every
// packet burns the full wait: at 0.1 the floor turned out to be around 0.2 and
// games ran at about half a frame per second. MAX_DELAY is Kodi's own idea of
// too much latency, and is comfortably above the floor.
const double TARGET_DELAY = MAX_DELAY; // seconds

CRetroPlayerAudio::CRetroPlayerAudio(CRPProcessInfo& processInfo) : m_processInfo(processInfo)
{
  CLog::Log(LOGDEBUG, "RetroPlayer[AUDIO]: Initializing audio");
}

CRetroPlayerAudio::~CRetroPlayerAudio()
{
  CLog::Log(LOGDEBUG, "RetroPlayer[AUDIO]: Deinitializing audio");

  CloseStream();
}

bool CRetroPlayerAudio::OpenStream(const StreamProperties& properties)
{
  const AudioStreamProperties& audioProperties =
      static_cast<const AudioStreamProperties&>(properties);

  const AEDataFormat pcmFormat = CAudioTranslator::TranslatePCMFormat(audioProperties.format);
  if (pcmFormat == AE_FMT_INVALID)
  {
    CLog::Log(LOGERROR, "RetroPlayer[AUDIO]: Unknown PCM format: {}",
              static_cast<int>(audioProperties.format));
    return false;
  }

  unsigned int iSampleRate = static_cast<unsigned int>(std::round(audioProperties.sampleRate));
  if (iSampleRate == 0)
  {
    CLog::Log(LOGERROR, "RetroPlayer[AUDIO]: Invalid samplerate: {:f}", audioProperties.sampleRate);
    return false;
  }

  CAEChannelInfo channelLayout;
  for (auto it = audioProperties.channelMap.begin(); it != audioProperties.channelMap.end(); ++it)
  {
    AEChannel channel = CAudioTranslator::TranslateAudioChannel(*it);
    if (channel == AE_CH_NULL)
      break;

    channelLayout += channel;
  }

  if (!channelLayout.IsLayoutValid())
  {
    CLog::Log(LOGERROR, "RetroPlayer[AUDIO]: Empty channel layout");
    return false;
  }

  if (m_pAudioStream != nullptr)
    CloseStream();

  IAE* audioEngine = CServiceBroker::GetActiveAE();
  if (audioEngine == nullptr)
    return false;

  CLog::Log(
      LOGINFO,
      "RetroPlayer[AUDIO]: Creating audio stream, format = {}, sample rate = {}, channels = {}",
      CAEUtil::DataFormatToStr(pcmFormat), iSampleRate, channelLayout.Count());

  AEAudioFormat audioFormat;
  audioFormat.m_dataFormat = pcmFormat;
  audioFormat.m_sampleRate = iSampleRate;
  audioFormat.m_channelLayout = channelLayout;
  m_pAudioStream = audioEngine->MakeStream(audioFormat);

  if (m_pAudioStream == nullptr)
  {
    CLog::Log(LOGERROR, "RetroPlayer[AUDIO]: Failed to create audio stream");
    return false;
  }

  m_processInfo.SetAudioChannels(audioFormat.m_channelLayout);
  m_processInfo.SetAudioSampleRate(audioFormat.m_sampleRate);
  m_processInfo.SetAudioBitsPerSample(CAEUtil::DataFormatToUsedBits(audioFormat.m_dataFormat));

  return true;
}

void CRetroPlayerAudio::AddStreamData(const StreamPacket& packet)
{
  const AudioStreamPacket& audioPacket = static_cast<const AudioStreamPacket&>(packet);

  if (m_bAudioEnabled)
  {
    if (m_pAudioStream)
    {
      const double delaySecs = m_pAudioStream->GetDelay();

      const size_t frameSize = m_pAudioStream->GetChannelCount() *
                               (CAEUtil::DataFormatToBits(m_pAudioStream->GetDataFormat()) >> 3);

      const unsigned int frameCount = static_cast<unsigned int>(audioPacket.size / frameSize);

      // Only when the delay is far past what waiting below should ever allow,
      // which means something other than a full sink is wrong -- a device that
      // stopped draining, or a client that raced ahead while the stream was
      // unable to take anything.
      if (delaySecs > MAX_DELAY * 2.0)
      {
        m_pAudioStream->Flush();
        CLog::Log(LOGDEBUG, "RetroPlayer[AUDIO]: Audio delay ({:0.2f} ms) is too high - flushing",
                  delaySecs * 1000);
      }

      const std::chrono::steady_clock::time_point giveUpAt =
          std::chrono::steady_clock::now() + MAX_WAIT;

      // Feed the sink until it has taken everything, waiting if it is full.
      unsigned int framesWritten = 0;
      while (framesWritten < frameCount)
      {
        framesWritten += m_pAudioStream->AddData(&audioPacket.data, framesWritten,
                                                 frameCount - framesWritten, nullptr);

        if (framesWritten >= frameCount)
          break;

        if (std::chrono::steady_clock::now() >= giveUpAt)
        {
          CLog::Log(LOGDEBUG,
                    "RetroPlayer[AUDIO]: Sink took {} of {} frames before the wait ran out",
                    framesWritten, frameCount);
          break;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
      }

      // Then hold the client to the speed its own sound plays at.
      //
      // This is the throttle, and it has to watch the delay rather than the
      // sink. AudioEngine buffers generously and takes a whole packet the
      // moment it is offered, so a client rendering faster than real time is
      // never blocked by a full sink -- it just runs flat out while the backlog
      // grows until it is flushed. That is exactly what Dreamcast games did:
      // roughly twice speed, with the delay climbing past 600 ms and being
      // flushed over a hundred times a session.
      //
      // Libretro clients expect the frontend to pace them through audio;
      // several have no other throttle. Waiting here for the queue to drain to
      // TARGET_DELAY is what supplies that.
      //
      // Bounded by the same deadline, so a stream that has stopped draining
      // costs the game loop a frame rather than hanging it.
      // Never wait longer than the audio just handed over would take to play.
      // That bound is what makes this safe: however wrong the target turns out
      // to be for a given sink, the client can only ever be held to real time,
      // never slower. Waiting a fixed 100 ms instead, with a target below the
      // sink's floor, cost every packet the full wait and ran games at about
      // half a frame per second.
      const unsigned int sampleRate = m_pAudioStream->GetSampleRate();
      if (sampleRate > 0)
      {
        const std::chrono::steady_clock::time_point throttleUntil =
            std::min(giveUpAt, std::chrono::steady_clock::now() +
                                   std::chrono::microseconds(static_cast<long long>(
                                       1000000.0 * frameCount / sampleRate)));

        while (m_pAudioStream->GetDelay() > TARGET_DELAY &&
               std::chrono::steady_clock::now() < throttleUntil)
        {
          std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
      }
    }
  }
}

void CRetroPlayerAudio::CloseStream()
{
  if (m_pAudioStream)
  {
    CLog::Log(LOGDEBUG, "RetroPlayer[AUDIO]: Closing audio stream");

    m_pAudioStream.reset();
  }
}
