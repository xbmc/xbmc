/*
 *  Copyright (C) 2010-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 *
 *  LAV A/V sync improvements based on LAV Filters by Hendrik Leppkes (Nevcairiel)
 *  https://github.com/Nevcairiel/LAVFilters
 *  (enabled via m_lavStyleSyncEnabled flag)
 */

#pragma once

#include "DVDAudioCodec.h"
#include "FloatingAverage.h"
#include "cores/AudioEngine/Utils/AEAudioFormat.h"
#include "cores/AudioEngine/Utils/AEBitstreamPacker.h"
#include "cores/AudioEngine/Utils/AEStreamInfo.h"
#include "settings/lib/ISettingCallback.h"

#include <atomic>
#include <list>
#include <memory>
#include <vector>

class CProcessInfo;
class CPackerMAT;

class CSetting;

class CDVDAudioCodecPassthrough : public CDVDAudioCodec, public ISettingCallback
{
public:
  CDVDAudioCodecPassthrough(CProcessInfo &processInfo, CAEStreamInfo::DataType streamType);
  ~CDVDAudioCodecPassthrough() override;

  bool Open(CDVDStreamInfo &hints, CDVDCodecOptions &options) override;
  void Dispose() override;
  bool AddData(const DemuxPacket &packet) override;
  void GetData(DVDAudioFrame &frame) override;
  void Reset() override;
  AEAudioFormat GetFormat() override { return m_format; }
  bool NeedPassthrough() override { return true; }
  std::string GetName() override { return m_codecName; }
  int GetBufferSize() override;

  // Enable/disable LAV A/V sync features
  // LAV Full: Internal clock + jitter tracking + seamless branch
  // LAV SB: Seamless branch fix ONLY (MAT packer discontinuity detection)
  void SetLavStyleSyncEnabled(bool enabled);       // Full LAV sync
  void SetLavSeamlessBranchEnabled(bool enabled);  // Seamless branch only
  bool IsLavStyleSyncEnabled() const { return m_lavStyleSyncEnabled; }
  bool IsLavSeamlessBranchEnabled() const { return m_lavSeamlessBranchEnabled; }

  // Reset LAV sync state (for GENERAL_RESYNC without full codec reset)
  void ResetLavSyncState();

  // Sync internal clock to VideoPlayer's coordinated RESYNC timestamp
  // This is the AUTHORITATIVE clock value that accounts for both audio and video
  // Call this from GENERAL_RESYNC handler AFTER ResetLavSyncState()
  void SyncToResyncPts(double pts);

  void OnSettingChanged(const std::shared_ptr<const CSetting>& setting) override;

private:
  void UpdateDialNormSettings();

  int GetData(uint8_t** dst);
  unsigned int PackTrueHD();
  CAEStreamParser m_parser;
  uint8_t* m_buffer = nullptr;
  unsigned int m_bufferSize = 0;
  unsigned int m_dataSize = 0;
  AEAudioFormat m_format;
  uint8_t *m_backlogBuffer = nullptr;
  unsigned int m_backlogBufferSize = 0;
  unsigned int m_backlogSize = 0;
  double m_currentPts = DVD_NOPTS_VALUE;
  double m_nextPts = DVD_NOPTS_VALUE;
  std::string m_codecName;

  // TrueHD specifics
  std::unique_ptr<CPackerMAT> m_packerMAT;
  std::vector<uint8_t> m_trueHDBuffer;
  unsigned int m_trueHDoffset = 0;
  unsigned int m_trueHDframes = 0;
  bool m_deviceIsRAW{false};

  //============================================================================
  // LAV A/V Sync - Enable/Disable Switches
  //============================================================================
  // m_lavStyleSyncEnabled: Full LAV sync - internal clock + jitter tracking + seamless branch
  // m_lavSeamlessBranchEnabled: Seamless branch fix ONLY - MAT packer discontinuity detection
  bool m_lavStyleSyncEnabled{false};        // Full LAV sync
  bool m_lavSeamlessBranchEnabled{false};   // Seamless branch only

  //============================================================================
  // LAV A/V Sync Members (used when m_lavStyleSyncEnabled == true)
  //============================================================================
  // Based on LAV Filters by Hendrik Leppkes (Nevcairiel)
  // https://github.com/Nevcairiel/LAVFilters

  // Internal sentinel for "no valid PTS" (-1.0 instead of DVD_NOPTS_VALUE)
  static constexpr double LOCAL_NOPTS = -1.0;
  static constexpr double MAX_REASONABLE_PTS = 86400000000.0; // 24 hours

  // Track last output PTS for seamless branch recovery and jitter calculation
  double m_lastOutputPts{LOCAL_NOPTS};

  // TrueHD timestamp caching (LAV) - cache PTS of first frame in MAT assembly
  double m_truehd_ptsCache{LOCAL_NOPTS};
  bool m_truehd_ptsCacheValid{false};

  // Jitter tracking using LAV FloatingAverage
  static constexpr size_t JITTER_WINDOW_SIZE = 256;
  AudioSync::CFloatingAverage<double, JITTER_WINDOW_SIZE> m_jitterTracker;

  // Jitter correction thresholds (in DVD_TIME_BASE units = microseconds)
  // LAV Filters: TrueHD/DTS use 10x threshold for bitstreaming tolerance
  static constexpr double JITTER_THRESHOLD_TRUEHD_DTS = 100000.0;  // 100ms
  static constexpr double JITTER_THRESHOLD_DEFAULT = 10000.0;      // 10ms
  double m_jitterThreshold{JITTER_THRESHOLD_DEFAULT};

  // Cached settings (updated via callback, read in hot path)
  std::atomic<bool> m_defeatAC3DialNorm{false};
  std::atomic<bool> m_defeatEAC3AtmosDialNorm{false};
  std::atomic<bool> m_defeatTrueHDDialNorm{false};

  // E-AC-3 JOC/Atmos: dialnorm defeat must be skipped because modifying BSI
  // dialnorm breaks JOC rendering (receiver cross-checks against OAMD metadata)
  bool m_isEAC3JOC{false};

  //============================================================================
  // Internal Clock A/V Sync
  //============================================================================
  // We maintain our own internal clock (m_internalClock) that:
  // - Syncs to RESYNC PTS from VideoPlayer (coordinated A/V clock)
  // - Outputs PTS from our clock, not demuxer
  // - Tracks drift against demuxer PTS for discontinuity detection
  // This isolates us from demuxer PTS chaos during seamless branching
  //============================================================================
  double m_internalClock{LOCAL_NOPTS};  // Running output timestamp (like LAV's m_rtStart)
  bool m_needsResync{true};             // When true, sync to next valid demuxer PTS
};
