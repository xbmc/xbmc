/*
 *  Copyright (C) 2010-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DVDAudioCodecPassthrough.h"

#include "DVDCodecs/DVDCodecs.h"
#include "DVDStreamInfo.h"
#include "ServiceBroker.h"
#include "cores/DataCacheCore.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "settings/lib/Setting.h"
#include "cores/AudioEngine/Utils/PackerMAT.h"
#include "cores/VideoPlayer/Interface/TimingConstants.h"
#include "utils/log.h"

#include <algorithm>
#include <cmath>

extern "C"
{
#include <libavcodec/avcodec.h>
}

namespace
{
constexpr unsigned int TRUEHD_BUF_SIZE = 61440;

// Internal sentinel for "no valid PTS" - we use -1.0 instead of DVD_NOPTS_VALUE
// because DVD_NOPTS_VALUE (0xFFF0000000000000) when cast to double becomes ~1.844e19
// which is the exact garbage value we see from the demuxer during seamless branching
constexpr double LOCAL_NOPTS = -1.0;

// Helper to check if a PTS value is valid
// Valid PTS must be >= 0 and <= 24 hours (way beyond any real content)
constexpr double MAX_REASONABLE_PTS = 86400000000.0; // 24 hours in DVD_TIME_BASE units

inline bool IsValidPts(double pts)
{
  return (pts >= 0.0) && (pts <= MAX_REASONABLE_PTS);
}
}

CDVDAudioCodecPassthrough::CDVDAudioCodecPassthrough(CProcessInfo &processInfo, CAEStreamInfo::DataType streamType) :
  CDVDAudioCodec(processInfo)
{
  m_format.m_streamInfo.m_type = streamType;
  m_deviceIsRAW = processInfo.WantsRawPassthrough();

  if (const auto settingsComponent = CServiceBroker::GetSettingsComponent())
  {
    if (const auto settings = settingsComponent->GetSettings())
    {
      settings->RegisterCallback(this, {CSettings::SETTING_COREELEC_AUDIO_AC3_DIALNORM,
                                        CSettings::SETTING_COREELEC_AUDIO_EAC3_ATMOS_DIALNORM,
                                        CSettings::SETTING_COREELEC_AUDIO_TRUEHD_ATMOS_DIALNORM});
    }
  }

  UpdateDialNormSettings();

  if (m_format.m_streamInfo.m_type == CAEStreamInfo::STREAM_TYPE_TRUEHD)
  {
    m_trueHDBuffer.resize(TRUEHD_BUF_SIZE);

    if (!m_deviceIsRAW)
      m_packerMAT = std::make_unique<CPackerMAT>();
  }
}

CDVDAudioCodecPassthrough::~CDVDAudioCodecPassthrough(void)
{
  if (const auto settingsComponent = CServiceBroker::GetSettingsComponent())
  {
    if (const auto settings = settingsComponent->GetSettings())
      settings->UnregisterCallback(this);
  }

  Dispose();
}

void CDVDAudioCodecPassthrough::UpdateDialNormSettings()
{
  const auto settingsComponent = CServiceBroker::GetSettingsComponent();
  const auto settings = settingsComponent ? settingsComponent->GetSettings() : nullptr;
  if (!settings) return;

  m_defeatAC3DialNorm.store(settings->GetBool(CSettings::SETTING_COREELEC_AUDIO_AC3_DIALNORM));
  m_defeatEAC3AtmosDialNorm.store(settings->GetBool(CSettings::SETTING_COREELEC_AUDIO_EAC3_ATMOS_DIALNORM));
  m_defeatTrueHDDialNorm.store(settings->GetBool(CSettings::SETTING_COREELEC_AUDIO_TRUEHD_ATMOS_DIALNORM));
}

void CDVDAudioCodecPassthrough::OnSettingChanged(const std::shared_ptr<const CSetting>& setting)
{
  if (!setting) return;

  const std::string& settingId = setting->GetId();
  if (settingId == CSettings::SETTING_COREELEC_AUDIO_AC3_DIALNORM ||
      settingId == CSettings::SETTING_COREELEC_AUDIO_EAC3_ATMOS_DIALNORM ||
      settingId == CSettings::SETTING_COREELEC_AUDIO_TRUEHD_ATMOS_DIALNORM)
  {
    UpdateDialNormSettings();
  }
}

bool CDVDAudioCodecPassthrough::Open(CDVDStreamInfo &hints, CDVDCodecOptions &options)
{
  UpdateDialNormSettings();

  m_parser.SetCoreOnly(false);
  switch (m_format.m_streamInfo.m_type)
  {
    case CAEStreamInfo::STREAM_TYPE_AC3:
      m_codecName = "pt-ac3";
      if (m_lavStyleSyncEnabled)
        m_jitterThreshold = JITTER_THRESHOLD_DEFAULT;
      m_parser.SetDefeatAC3DialNorm(m_defeatAC3DialNorm.load());
      break;

    case CAEStreamInfo::STREAM_TYPE_EAC3:
      m_codecName = "pt-eac3";
      if (m_lavStyleSyncEnabled)
        m_jitterThreshold = JITTER_THRESHOLD_DEFAULT;
      m_isEAC3JOC = (hints.profile == AV_PROFILE_EAC3_DDP_ATMOS);
      if (!m_isEAC3JOC || m_defeatEAC3AtmosDialNorm.load())
        m_parser.SetDefeatAC3DialNorm(m_defeatAC3DialNorm.load());
      break;

    case CAEStreamInfo::STREAM_TYPE_DTSHD_MA:
      m_codecName = "pt-dtshd_ma";
      // LAV Filters: TrueHD/DTS use 10x threshold (1 second) for bitstreaming tolerance
      if (m_lavStyleSyncEnabled)
        m_jitterThreshold = JITTER_THRESHOLD_TRUEHD_DTS;
      break;

    case CAEStreamInfo::STREAM_TYPE_DTSHD:
      m_codecName = "pt-dtshd_hra";
      if (m_lavStyleSyncEnabled)
        m_jitterThreshold = JITTER_THRESHOLD_TRUEHD_DTS;
      break;

    case CAEStreamInfo::STREAM_TYPE_DTSHD_CORE:
      m_codecName = "pt-dts";
      m_parser.SetCoreOnly(true);
      if (m_lavStyleSyncEnabled)
        m_jitterThreshold = JITTER_THRESHOLD_TRUEHD_DTS;
      break;

    case CAEStreamInfo::STREAM_TYPE_TRUEHD:
      m_codecName = "pt-truehd";
      // LAV Filters: TrueHD/DTS use 10x threshold (1 second) for bitstreaming tolerance
      if (m_lavStyleSyncEnabled)
        m_jitterThreshold = JITTER_THRESHOLD_TRUEHD_DTS;
      m_parser.SetDefeatTrueHDDialNorm(m_defeatTrueHDDialNorm.load());

      CLog::Log(LOGDEBUG, "CDVDAudioCodecPassthrough::{} - passthrough output device is {}",
                __func__, m_deviceIsRAW ? "RAW" : "IEC");
      break;

    default:
      return false;
  }

  if (m_lavStyleSyncEnabled)
  {
    CLog::Log(LOGDEBUG, "CDVDAudioCodecPassthrough::{} - LAV Full sync ENABLED, jitter threshold {:.0f}ms for {}",
              __func__, m_jitterThreshold / 1000.0, m_codecName);
  }
  else if (m_lavSeamlessBranchEnabled)
  {
    CLog::Log(LOGDEBUG, "CDVDAudioCodecPassthrough::{} - LAV Seamless Branch ENABLED for {}",
              __func__, m_codecName);
  }
  else
  {
    CLog::Log(LOGDEBUG, "CDVDAudioCodecPassthrough::{} - LAV all sync DISABLED, using standard Kodi PTS handling for {}",
              __func__, m_codecName);
  }

  m_dataSize = 0;
  m_bufferSize = 0;
  m_backlogSize = 0;
  
  if (m_lavStyleSyncEnabled)
  {
    // LAV Full: Use LOCAL_NOPTS sentinel for PTS validation
    m_currentPts = LOCAL_NOPTS;
    m_nextPts = LOCAL_NOPTS;
    m_lastOutputPts = LOCAL_NOPTS;
    m_jitterTracker.Reset();
  }
  else
  {
    // Standard Kodi: Use DVD_NOPTS_VALUE
    m_currentPts = DVD_NOPTS_VALUE;
    m_nextPts = DVD_NOPTS_VALUE;
  }
  return true;
}

void CDVDAudioCodecPassthrough::Dispose()
{
  if (m_buffer)
  {
    delete[] m_buffer;
    m_buffer = nullptr;
  }

  free(m_backlogBuffer);
  m_backlogBuffer = nullptr;
  m_backlogBufferSize = 0;

  m_bufferSize = 0;
}

bool CDVDAudioCodecPassthrough::AddData(const DemuxPacket &packet)
{
  // Apply cached values (updated by settings callbacks) without per-packet settings lookups.
  // Skip E-AC-3 dialnorm defeat for JOC/Atmos unless explicitly overridden —
  // modifying BSI dialnorm breaks JOC rendering on receivers.
  m_parser.SetDefeatAC3DialNorm(
    m_defeatAC3DialNorm.load() && (!m_isEAC3JOC || m_defeatEAC3AtmosDialNorm.load()));
  m_parser.SetDefeatTrueHDDialNorm(m_defeatTrueHDDialNorm.load());

  if (m_backlogSize)
  {
    m_dataSize = m_bufferSize;
    unsigned int consumed = m_parser.AddData(m_backlogBuffer, m_backlogSize, &m_buffer, &m_dataSize);
    m_bufferSize = std::max(m_bufferSize, m_dataSize);
    if (consumed != m_backlogSize)
    {
      memmove(m_backlogBuffer, m_backlogBuffer+consumed, m_backlogSize-consumed);
    }
    m_backlogSize -= consumed;
  }

  auto pData(const_cast<uint8_t*>(packet.pData));
  int iSize(packet.iSize);

  if (m_lavStyleSyncEnabled)
  {
    // LAV Full: Detect invalid PTS values using robust check for seamless branching
    double incomingPts = packet.pts;
    bool ptsIsValid = IsValidPts(incomingPts);

    if (pData)
    {
      // Sanitize PTS members if they contain garbage values (can happen during seamless branching)
      if (!IsValidPts(m_currentPts))
        m_currentPts = LOCAL_NOPTS;
      if (!IsValidPts(m_nextPts))
        m_nextPts = LOCAL_NOPTS;

      if (m_currentPts == LOCAL_NOPTS)
      {
        if (m_nextPts != LOCAL_NOPTS)
        {
          m_currentPts = m_nextPts;
          m_nextPts = ptsIsValid ? incomingPts : LOCAL_NOPTS;
        }
        else if (ptsIsValid)
        {
          m_currentPts = incomingPts;
        }
      }
      else if (ptsIsValid)
      {
        m_nextPts = incomingPts;
      }
    }
  }
  else
  {
    // Standard Kodi/avdvplus: Original PTS handling
    if (pData)
    {
      if (m_currentPts == DVD_NOPTS_VALUE)
      {
        if (m_nextPts != DVD_NOPTS_VALUE)
        {
          m_currentPts = m_nextPts;
          m_nextPts = packet.pts;
        }
        else if (packet.pts != DVD_NOPTS_VALUE)
        {
          m_currentPts = packet.pts;
        }
      }
      else
      {
        m_nextPts = packet.pts;
      }
    }
  }

  if (pData && !m_backlogSize)
  {
    if (iSize <= 0)
      return true;

    m_dataSize = m_bufferSize;
    int used = m_parser.AddData(pData, iSize, &m_buffer, &m_dataSize);
    m_bufferSize = std::max(m_bufferSize, m_dataSize);

    if (used != iSize)
    {
      const unsigned int remaining = static_cast<unsigned int>(iSize - used);
      if (m_backlogBufferSize < remaining)
      {
        m_backlogBufferSize = std::max(TRUEHD_BUF_SIZE, remaining);
        m_backlogBuffer = static_cast<uint8_t*>(realloc(m_backlogBuffer, m_backlogBufferSize));
      }
      m_backlogSize = remaining;
      memcpy(m_backlogBuffer, pData + used, m_backlogSize);
    }
  }
  else if (pData)
  {
    const unsigned int newSize = m_backlogSize + static_cast<unsigned int>(iSize);
    if (m_backlogBufferSize < newSize)
    {
      m_backlogBufferSize = std::max(TRUEHD_BUF_SIZE, newSize);
      m_backlogBuffer = static_cast<uint8_t*>(realloc(m_backlogBuffer, m_backlogBufferSize));
    }
    memcpy(m_backlogBuffer + m_backlogSize, pData, iSize);
    m_backlogSize += static_cast<unsigned int>(iSize);
  }

  if (!m_dataSize)
    return true;

  m_format.m_dataFormat = AE_FMT_RAW;
  m_format.m_streamInfo = m_parser.GetStreamInfo();
  m_format.m_sampleRate = m_parser.GetSampleRate();
  m_format.m_frameSize = 1;
  CAEChannelInfo layout;
  for (unsigned int i = 0; i < m_parser.GetChannels(); i++)
  {
    layout += AE_CH_RAW;
  }
  m_format.m_channelLayout = layout;

  if (m_format.m_streamInfo.m_type == CAEStreamInfo::STREAM_TYPE_TRUEHD)
  {
    if (m_deviceIsRAW) // RAW
    {
      m_dataSize = PackTrueHD();
    }
    else // IEC
    {
      if (m_lavStyleSyncEnabled)
      {
        // LAV Full: timestamp caching for TrueHD MAT assembly
        // Since a MAT frame contains 24 TrueHD frames, we want the timestamp of the first one
        if (!m_truehd_ptsCacheValid && IsValidPts(m_currentPts))
        {
          m_truehd_ptsCache = m_currentPts;
          m_truehd_ptsCacheValid = true;
        }
      }

      if (m_packerMAT->PackTrueHD(m_buffer, m_dataSize))
      {
        m_trueHDBuffer = m_packerMAT->GetOutputFrame();
        m_dataSize = TRUEHD_BUF_SIZE;

        if (m_lavStyleSyncEnabled)
        {
          // Consume discontinuity flag from MAT packer (seamless branch detection)
          // We don't need to react to it - LAV packer already handled padding
          // and our internal clock continues smoothly regardless
          (void)m_packerMAT->HadDiscontinuity();

          // Use cached timestamp for this MAT frame, then reset cache for next MAT
          if (m_truehd_ptsCacheValid)
          {
            m_currentPts = m_truehd_ptsCache;
            m_truehd_ptsCacheValid = false;
            m_truehd_ptsCache = LOCAL_NOPTS;
          }
        }
      }
      else
      {
        m_dataSize = 0;
      }
    }
  }

  return true;
}

unsigned int CDVDAudioCodecPassthrough::PackTrueHD()
{
  unsigned int dataSize{0};

  if (m_trueHDoffset == 0)
    m_trueHDframes = 0;

  memcpy(m_trueHDBuffer.data() + m_trueHDoffset, m_buffer, m_dataSize);

  m_trueHDoffset += m_dataSize;
  m_trueHDframes++;

  if (m_trueHDframes == 24)
  {
    dataSize = m_trueHDoffset;
    m_trueHDoffset = 0;
    m_trueHDframes = 0;
    return dataSize;
  }

  return 0;
}

void CDVDAudioCodecPassthrough::GetData(DVDAudioFrame &frame)
{
  frame.nb_frames = GetData(frame.data);
  frame.framesOut = 0;
  frame.hasDiscontinuity = false;
  frame.discontinuityCorrection = 0.0;

  if (frame.nb_frames == 0)
    return;

  frame.passthrough = true;
  frame.format = m_format;
  frame.planes = 1;
  frame.bits_per_sample = 8;
  frame.duration = DVD_MSEC_TO_TIME(frame.format.m_streamInfo.GetDuration());

  if (m_lavStyleSyncEnabled)
  {
    //============================================================================
    // LAV Internal Clock A/V Sync
    //============================================================================
    // Based on LAV Filters by Hendrik Leppkes (Nevcairiel)
    // 
    // We maintain our OWN internal clock (m_internalClock) that:
    // - Syncs to RESYNC PTS from VideoPlayer (coordinated A/V clock)
    // - Outputs PTS from our clock, not demuxer
    // - Tracks drift against demuxer to detect discontinuities
    //============================================================================

    const CAEStreamInfo::DataType streamType = m_format.m_streamInfo.m_type;
    const bool isTrueHD = (streamType == CAEStreamInfo::STREAM_TYPE_TRUEHD);

    // TrueHD-specific: Get samples offset for drift calculation (LAV)
    double samplesOffsetTime = 0.0;
    if (isTrueHD && m_packerMAT)
    {
      int samplesOffset = m_packerMAT->GetSamplesOffset();
      if (samplesOffset != 0)
      {
        samplesOffsetTime = static_cast<double>(samplesOffset) / m_format.m_sampleRate * DVD_TIME_BASE;
      }
    }

    // Demuxer PTS for this frame (may be invalid during branching)
    const double demuxerPts = m_currentPts;
    const bool haveDemuxerPts = IsValidPts(demuxerPts);

    //============================================================================
    // STEP 1: Resync internal clock if needed
    //============================================================================
    // Sync to demuxer PTS when we need resync and have valid PTS.
    // This happens on codec creation and after seeks.
    // 
    // If RESYNC arrives later (from VideoPlayer::Sync), SyncToResyncPts() will
    // override this with the correct coordinated A/V clock value.
    // This approach handles display reset codec recreation gracefully - the new
    // codec syncs to demuxer PTS, which should be close to correct since the
    // stream is already playing.
    if (m_needsResync && haveDemuxerPts)
    {
      m_internalClock = demuxerPts;
      m_needsResync = false;
      m_jitterTracker.Reset();  // Clear jitter history on resync
      
      CLog::Log(LOGDEBUG, "CDVDAudioCodecPassthrough: Internal clock synced to demuxer PTS {:.3f}s",
                demuxerPts / DVD_TIME_BASE);
    }

    //============================================================================
    // STEP 2: Track jitter and correct internal clock when threshold exceeded
    //============================================================================
    // LAV Filters approach: track drift between our internal clock and demuxer PTS.
    // When drift exceeds threshold, CORRECT the internal clock to realign.
    // This handles both:
    // - Seamless branch points (large sudden jumps in demuxer PTS)
    // - Long-term drift accumulation
    //============================================================================
    
    if (IsValidPts(m_internalClock) && haveDemuxerPts)
    {
      // Jitter = our_clock - demuxer_pts (+ samplesOffset for TrueHD MAT compensation)
      // Positive jitter = we're ahead of demuxer, negative = we're behind
      double jitter = m_internalClock - demuxerPts + samplesOffsetTime;
      m_jitterTracker.Sample(jitter);

      // Use AbsMinimum for correction (most stable value in the window)
      double absMinJitter = m_jitterTracker.AbsMinimum();

      if (std::abs(absMinJitter) > m_jitterThreshold)
      {
        // Correct internal clock by the jitter amount (like LAV Filters)
        m_internalClock -= absMinJitter;
        m_jitterTracker.OffsetValues(-absMinJitter);
        
        // Signal discontinuity to downstream
        frame.hasDiscontinuity = true;
        frame.discontinuityCorrection = absMinJitter;
        
        CLog::Log(LOGDEBUG,
                  "CDVDAudioCodecPassthrough: Jitter correction {:.2f}ms (threshold {:.0f}ms)",
                  absMinJitter / 1000.0,
                  m_jitterThreshold / 1000.0);
      }
    }

    //============================================================================
    // STEP 3: Output PTS from internal clock (synced to RESYNC)
    //============================================================================
    // The whole point of LAV sync is to output PTS from our internal clock
    // which has been synced to the RESYNC pts (coordinated A/V clock).
    // 
    // The internal clock:
    // - Is synced to RESYNC pts (from VideoPlayer::Sync, the authoritative A/V clock)
    // - Advances by frame duration each frame
    // - Tracks drift against demuxer PTS for discontinuity detection
    //
    // We ALWAYS use internal clock for output after it's been synced.
    // The demuxer PTS is only used for:
    // - Initial sync before RESYNC arrives
    // - Drift tracking (to detect discontinuities)
    //============================================================================
    
    if (IsValidPts(m_internalClock))
    {
      // Output from internal clock (synced to RESYNC)
      frame.pts = m_internalClock;
      m_internalClock += frame.duration;
      m_lastOutputPts = frame.pts;
      m_dataCacheCore.SetAudioPts(frame.pts);
    }
    else if (haveDemuxerPts)
    {
      // Fallback: internal clock not yet set, use demuxer PTS
      // This happens before RESYNC arrives
      frame.pts = demuxerPts;
      m_internalClock = demuxerPts + frame.duration;
      m_lastOutputPts = frame.pts;
      m_dataCacheCore.SetAudioPts(frame.pts);
    }
    else
    {
      // No valid PTS available anywhere
      frame.pts = DVD_NOPTS_VALUE;
    }

    // Clear current PTS after use
    m_currentPts = LOCAL_NOPTS;
  }
  else
  {
    //============================================================================
    // Standard Kodi PTS handling (no LAV sync)
    //============================================================================
    // Original avdvplus code
    //============================================================================

    frame.pts = m_currentPts;

    if (m_currentPts != DVD_NOPTS_VALUE)
      m_dataCacheCore.SetAudioPts(m_currentPts);

    m_currentPts = DVD_NOPTS_VALUE;
  }
}

int CDVDAudioCodecPassthrough::GetData(uint8_t** dst)
{
  if (!m_dataSize)
    AddData(DemuxPacket());

  if (m_format.m_streamInfo.m_type == CAEStreamInfo::STREAM_TYPE_TRUEHD)
    *dst = m_trueHDBuffer.data();
  else
    *dst = m_buffer;

  int bytes = m_dataSize;
  m_dataSize = 0;
  return bytes;
}

void CDVDAudioCodecPassthrough::Reset()
{
  m_trueHDoffset = 0;
  m_dataSize = 0;
  m_bufferSize = 0;
  m_backlogSize = 0;

  if (m_lavStyleSyncEnabled)
  {
    // LAV Full reset: use LOCAL_NOPTS sentinel
    m_currentPts = LOCAL_NOPTS;
    m_nextPts = LOCAL_NOPTS;
    m_lastOutputPts = LOCAL_NOPTS;

    // Reset TrueHD-specific state
    m_truehd_ptsCache = LOCAL_NOPTS;
    m_truehd_ptsCacheValid = false;

    // Reset LAV internal clock - will resync on next valid PTS or RESYNC
    m_internalClock = LOCAL_NOPTS;
    m_needsResync = true;
    m_jitterTracker.Reset();

    CLog::Log(LOGDEBUG, "CDVDAudioCodecPassthrough::Reset - Internal clock reset, will resync");

    // Reset PackerMAT state for TrueHD
    if (m_packerMAT)
      m_packerMAT->Reset();
    
    m_parser.Reset();
  }
  else
  {
    // Standard Kodi/avdvplus reset - EXACT order from original
    m_currentPts = DVD_NOPTS_VALUE;
    m_nextPts = DVD_NOPTS_VALUE;
    m_parser.Reset();
  }
}

void CDVDAudioCodecPassthrough::SetLavStyleSyncEnabled(bool enabled)
{
  m_lavStyleSyncEnabled = enabled;
  
  // LAV Full also enables seamless branch fix
  if (enabled)
    m_lavSeamlessBranchEnabled = true;
  
  // Propagate to PackerMAT for TrueHD discontinuity detection
  // PackerMAT should be enabled if EITHER full LAV sync OR seamless branch is enabled
  if (m_packerMAT)
    m_packerMAT->SetLavStyleEnabled(m_lavStyleSyncEnabled || m_lavSeamlessBranchEnabled);
}

void CDVDAudioCodecPassthrough::SetLavSeamlessBranchEnabled(bool enabled)
{
  m_lavSeamlessBranchEnabled = enabled;
  
  // Propagate to PackerMAT for TrueHD discontinuity detection
  // PackerMAT should be enabled if EITHER full LAV sync OR seamless branch is enabled
  if (m_packerMAT)
    m_packerMAT->SetLavStyleEnabled(m_lavStyleSyncEnabled || m_lavSeamlessBranchEnabled);
}

void CDVDAudioCodecPassthrough::ResetLavSyncState()
{
  if (!m_lavStyleSyncEnabled)
    return;

  // Reset PTS tracking to force resync on next valid timestamp
  m_lastOutputPts = LOCAL_NOPTS;

  // Reset TrueHD timestamp cache
  m_truehd_ptsCache = LOCAL_NOPTS;
  m_truehd_ptsCacheValid = false;

  // Reset internal clock - will resync to demuxer on next valid PTS
  m_internalClock = LOCAL_NOPTS;
  m_needsResync = true;
  m_jitterTracker.Reset();

  CLog::Log(LOGDEBUG, "CDVDAudioCodecPassthrough::ResetLavSyncState - Internal clock reset, will resync");
}

void CDVDAudioCodecPassthrough::SyncToResyncPts(double pts)
{
  if (!m_lavStyleSyncEnabled)
    return;

  // VideoPlayer::Sync() sends RESYNC with a coordinated A/V clock value.
  // We trust this value and use it directly for our internal clock.
  // VideoPlayer.cpp has been modified to only send RESYNC when both
  // audio AND video have valid PTS values.
  
  if (pts != DVD_NOPTS_VALUE && pts >= 0.0 && pts <= MAX_REASONABLE_PTS)
  {
    m_internalClock = pts;
    m_needsResync = false;
    m_jitterTracker.Reset();
    
    CLog::Log(LOGDEBUG, "CDVDAudioCodecPassthrough::SyncToResyncPts - Internal clock set to RESYNC pts {:.3f}s",
              pts / DVD_TIME_BASE);
  }
  else
  {
    CLog::Log(LOGDEBUG, "CDVDAudioCodecPassthrough::SyncToResyncPts - Invalid pts, ignoring");
  }
}

int CDVDAudioCodecPassthrough::GetBufferSize()
{
  return (int)m_parser.GetBufferSize();
}

