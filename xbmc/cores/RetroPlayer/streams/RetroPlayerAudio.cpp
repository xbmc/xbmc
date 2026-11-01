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

// How far the delay may rise above the sink's own latency before the sound is
// flushed
const double MAX_DELAY = 0.3; // seconds

// Smaller changes in the resample ratio than this aren't passed to the sound
const double MIN_RATIO_CHANGE = 0.00001;

// The game's clock and the sound card's never quite agree, and the difference
// builds up in the delay until the sound is flushed or the sink runs dry. Once
// the delay has settled after the sound starts, speeding the sound up or down
// by a fraction of a percent holds it there.
const double SETTLE_TIME = 5.0; // seconds of sound
const double DRIFT_RANGE = 0.02; // seconds off the held delay for the full correction
const double MAX_DRIFT_CORRECTION = 0.005;
const double DELAY_SMOOTHING = 1.0; // seconds

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
  // Resampling follows the game's speed and corrects drift
  m_pAudioStream = audioEngine->MakeStream(audioFormat, AESTREAM_FORCE_RESAMPLE);
  m_resampleRatio = 1.0;
  m_settledSecs = 0.0;
  m_maxDelay = 0.0;
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
      const double delaySecs = m_pAudioStream->GetDelay();

      // What the sink and the engine hold, which the delay never falls below.
      // Sinks differ by hundreds of milliseconds, so the flush is measured
      // from here.
      const double floorSecs = std::max(0.0, delaySecs - m_pAudioStream->GetCacheTime());

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

      if (delaySecs > floorSecs + MAX_DELAY)
      {
        m_pAudioStream->Flush();
        // The flush puts the stream back to its own rate
        m_resampleRatio = 1.0;
        m_settledSecs = 0.0;
        skipFrames = 0;
        m_framesToSkip = 0;
        CLog::Log(LOGDEBUG, "RetroPlayer[AUDIO]: Audio delay ({:0.2f} ms) is too high - flushing",
                  delaySecs * 1000);
      }

      UpdateResampleRatio(frameCount / sampleRate, delaySecs);
      m_processInfo.GetDisplayPacing().SetSoundDelay(m_smoothedDelay, m_heldDelay);

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

void CRetroPlayerAudio::UpdateResampleRatio(double packetSecs, double delaySecs)
{
  double correction = 0.0;

  // Skipping the sound that queued during a pause is already bringing the
  // delay back down
  if (m_framesToSkip == 0)
  {
    // A new sink has a delay of its own
    const double maxDelay = m_pAudioStream->GetMaxDelay();
    if (maxDelay != m_maxDelay)
    {
      m_maxDelay = maxDelay;
      m_settledSecs = 0.0;
    }

    // The whole delay is held, not just the stream's share: the engine passes
    // sound to the sink a period at a time, so drift can build up on either
    // side of the stream
    if (m_settledSecs == 0.0)
      m_smoothedDelay = delaySecs;
    m_smoothedDelay += std::min(1.0, packetSecs / DELAY_SMOOTHING) * (delaySecs - m_smoothedDelay);

    if (m_settledSecs < SETTLE_TIME)
    {
      m_settledSecs += packetSecs;
      m_heldDelay = m_smoothedDelay;
    }
    else
    {
      correction = std::clamp((m_heldDelay - m_smoothedDelay) / DRIFT_RANGE * MAX_DRIFT_CORRECTION,
                              -MAX_DRIFT_CORRECTION, MAX_DRIFT_CORRECTION);
    }
  }

  // The correction is the only adjustment, made around the ratio that follows
  // the game: its own rate, or the screen's when it runs in step with it
  CDisplayPacing& displayPacing = m_processInfo.GetDisplayPacing();
  displayPacing.SetDriftCorrection(correction);
  const double ratio = (1.0 + correction) / displayPacing.PlaybackRate();
  if (std::abs(ratio - m_resampleRatio) > MIN_RATIO_CHANGE)
  {
    m_pAudioStream->SetResampleRatio(ratio);
    m_resampleRatio = ratio;
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
