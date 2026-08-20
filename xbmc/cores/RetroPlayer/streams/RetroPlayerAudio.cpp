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
#include <limits>
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

// How much audio to keep queued ahead of what the sink reports when it has
// nothing of ours left to play.
//
// Measured against that floor rather than against zero. GetDelay() includes the
// sink's own latency, which is not a small or fixed number: AESinkPULSE asks for
// a 400 ms buffer on the paths it considers high-latency. A fixed target below
// that floor can never be reached, so every packet burns its whole wait and the
// game ends up slower than real time -- seen at about half a frame per second
// with a 0.1 s target against a floor near 0.2 s.
//
// Subtracting the observed floor makes the target mean the same thing on every
// sink: how much of our audio is queued, not how much latency the device has.
const double TARGET_AHEAD = 0.1; // seconds

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

  // Learned per stream: a new one may be on a device with a different latency
  m_floorDelaySecs = std::numeric_limits<double>::max();

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

      // The smallest delay seen on this stream approximates the sink's own
      // latency, which is what remains once everything we queued has played.
      m_floorDelaySecs = std::min(m_floorDelaySecs, delaySecs);
      const double targetDelay = m_floorDelaySecs + TARGET_AHEAD;

      const size_t frameSize = m_pAudioStream->GetChannelCount() *
                               (CAEUtil::DataFormatToBits(m_pAudioStream->GetDataFormat()) >> 3);

      const unsigned int frameCount = static_cast<unsigned int>(audioPacket.size / frameSize);

      // Only when the delay is far past what waiting below should ever allow,
      // which means something other than a full sink is wrong -- a device that
      // stopped draining, or a client that raced ahead while the stream was
      // unable to take anything.
      if (delaySecs > targetDelay + MAX_DELAY)
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
      // Never wait longer than the audio just handed over would take to play,
      // so a packet cannot cost more than the time it represents. That caps how
      // wrong this can go, but it does not make a wrong target free: the wait
      // runs after the frame has been emulated, so a target the sink can never
      // reach still adds to every frame. Hence measuring the floor above.
      const unsigned int sampleRate = m_pAudioStream->GetSampleRate();
      if (sampleRate > 0)
      {
        const std::chrono::steady_clock::time_point throttleUntil =
            std::min(giveUpAt, std::chrono::steady_clock::now() +
                                   std::chrono::microseconds(static_cast<long long>(
                                       1000000.0 * frameCount / sampleRate)));

        while (m_pAudioStream->GetDelay() > targetDelay &&
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
