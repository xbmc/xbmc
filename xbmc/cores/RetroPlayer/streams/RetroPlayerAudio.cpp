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
#include "cores/AudioEngine/Utils/AEStreamData.h"
#include "cores/AudioEngine/Utils/AEUtil.h"
#include "cores/RetroPlayer/audio/AudioTranslator.h"
#include "cores/RetroPlayer/process/RPProcessInfo.h"
#include "utils/log.h"

#include <algorithm>
#include <chrono>
#include <cmath>

using namespace KODI;
using namespace RETRO;

const double MAX_DELAY = 0.3; // seconds

// Smaller changes in the game's speed than this aren't passed to the sound
const double MIN_RATE_CHANGE = 0.00001;

// How long to stay quiet between log lines.
const std::chrono::seconds DROP_LOG_INTERVAL{10};

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
  // Resampling follows the game when it runs at the screen's rate instead of
  // its own
  m_pAudioStream = audioEngine->MakeStream(
      audioFormat, m_processInfo.GetDisplayPacing().Enabled() ? AESTREAM_FORCE_RESAMPLE : 0);
  m_playbackRate = 1.0;
  m_playingDelay = 0.0;
  m_framesToSkip = 0;

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
      const double playbackRate = m_processInfo.GetDisplayPacing().PlaybackRate();
      if (std::abs(playbackRate - m_playbackRate) > MIN_RATE_CHANGE)
      {
        m_pAudioStream->SetResampleRatio(1.0 / playbackRate);
        m_playbackRate = playbackRate;
      }

      const double delaySecs = m_pAudioStream->GetDelay();

      const size_t frameSize = m_pAudioStream->GetChannelCount() *
                               (CAEUtil::DataFormatToBits(m_pAudioStream->GetDataFormat()) >> 3);

      const unsigned int frameCount = static_cast<unsigned int>(audioPacket.size / frameSize);

      // While the game is paused, muted, rewound or fast-forwarded, the sink
      // fills the device with silence, and the game's sound would queue behind
      // it for good. Skip the start of the next packets until the delay is back
      // to what it was, keeping at least half of each so that the stream never
      // runs dry and gets padded again.
      const double sampleRate = m_pAudioStream->GetSampleRate();
      if (m_restoreDelay.exchange(false) && m_playingDelay > 0.0)
        m_framesToSkip = static_cast<unsigned int>(MAX_DELAY * sampleRate);

      unsigned int skipFrames = 0;
      if (m_framesToSkip > 0)
      {
        const double excessSecs = delaySecs - m_playingDelay;
        if (excessSecs > 0.0)
          skipFrames = std::min(
              {m_framesToSkip, frameCount / 2, static_cast<unsigned int>(excessSecs * sampleRate)});
        m_framesToSkip = skipFrames > 0 ? m_framesToSkip - skipFrames : 0;
      }

      if (m_framesToSkip == 0)
        m_playingDelay = delaySecs;

      if (delaySecs > MAX_DELAY)
      {
        m_pAudioStream->Flush();
        skipFrames = 0;
        m_framesToSkip = 0;
        CLog::Log(LOGDEBUG, "RetroPlayer[AUDIO]: Audio delay ({:0.2f} ms) is too high - flushing",
                  delaySecs * 1000);
      }

      const unsigned int accepted =
          m_pAudioStream->AddData(&audioPacket.data, skipFrames, frameCount - skipFrames, nullptr);

      // Dropping what the sink won't take is deliberate; being silent about it
      // is not.
      if (accepted < frameCount - skipFrames)
      {
        m_droppedFrames += frameCount - skipFrames - accepted;
        ++m_dropEvents;

        const auto now = std::chrono::steady_clock::now();

        if (!m_lastDropLog || now - *m_lastDropLog >= DROP_LOG_INTERVAL)
        {
          CLog::Log(LOGDEBUG,
                    "RetroPlayer[AUDIO]: Sink accepted {} of {} frames, {} refusals since the last "
                    "message, {} frames dropped so far",
                    accepted, frameCount, m_dropEvents, m_droppedFrames);

          m_lastDropLog = now;
          m_dropEvents = 0;
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
