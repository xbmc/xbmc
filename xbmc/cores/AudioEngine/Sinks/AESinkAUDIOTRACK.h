/*
 *  Copyright (C) 2010-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/AudioEngine/Interfaces/AESink.h"
#include "cores/AudioEngine/Utils/AEDeviceInfo.h"
#include "cores/AudioEngine/Utils/AEUtil.h"
#include "threads/CriticalSection.h"
#include "threads/SystemClock.h"
#include "threads/Thread.h"

#include <deque>
#include <set>

#include <androidjni/AudioTrack.h>

class CAESinkAUDIOTRACK : public IAESink
{
public:
  const char* GetName() override { return "AUDIOTRACK"; }

  CAESinkAUDIOTRACK();
  ~CAESinkAUDIOTRACK() override;

  bool Initialize(AEAudioFormat& format, std::string& device) override;
  void Deinitialize() override;
  bool IsInitialized();

  void GetDelay(AEDelayStatus& status) override;
  double GetLatency() override;
  double GetCacheTotal() override;
  unsigned int AddPackets(uint8_t** data, unsigned int frames, unsigned int offset) override;
  void AddPause(unsigned int millis) override;
  void Drain() override;
  static void          EnumerateDevicesEx(AEDeviceInfoList &list, bool force = false);
  static void Register();
  static std::unique_ptr<IAESink> Create(std::string& device, AEAudioFormat& desiredFormat);

protected:
  static jni::CJNIAudioTrack *CreateAudioTrack(int stream, int sampleRate, int channelMask, int encoding, int bufferSize);
  static bool IsSupported(int sampleRateInHz, int channelConfig, int audioFormat);
  static bool VerifySinkConfiguration(int sampleRate,
                                      int channelMask,
                                      int encoding,
                                      bool isRaw = false);
  static void UpdateAvailablePCMCapabilities();
  static void UpdateAvailablePassthroughCapabilities(bool isRaw = false);

  int AudioTrackWrite(char* audioData, int offsetInBytes, int sizeInBytes);
  int AudioTrackWrite(char* audioData, int sizeInBytes, int64_t timestamp);

private:
  jni::CJNIAudioTrack  *m_at_jni;
  int     m_jniAudioFormat;

  double                m_duration_written;
  unsigned int          m_min_buffer_size;
  uint64_t              m_headPos;
  uint64_t m_headPosOld;
  uint32_t m_stuckCounter;
  uint64_t m_timestampPos = 0;
  // Moving Average computes the weighted average delay over
  // a fixed size of delay values - current size: 20 values
  double                GetMovingAverageDelay(double newestdelay);

  // We maintain our linear weighted average delay counter in here
  // The n-th value (timely oldest value) is weighted with 1/n
  // the newest value gets a weight of 1
  std::deque<double>   m_linearmovingaverage;

  static CAEDeviceInfo m_info;
  static CAEDeviceInfo m_info_raw;
  static CAEDeviceInfo m_info_iec;
  static bool m_hasIEC;
  static std::set<unsigned int>       m_sink_sampleRates;
  static bool m_sinkSupportsFloat;
  static bool m_sinkSupportsMultiChannelFloat;
  static bool m_passthrough_use_eac3;

  AEAudioFormat      m_format;
  int16_t           *m_alignedS16;
  unsigned int       m_sink_frameSize;
  unsigned int       m_sink_sampleRate;
  bool               m_passthrough;
  double             m_audiotrackbuffer_sec;
  double m_audiotrackbuffer_sec_orig;
  int                m_encoding;
  double m_pause_ms = 0.0;
  double m_delay = 0.0;
  double m_hw_delay = 0.0;
  CJNIAudioTimestamp m_timestamp;
  XbmcThreads::EndTime<> m_stampTimer;
  bool m_superviseAudioDelay = false;

  std::vector<float> m_floatbuf;
  std::vector<int16_t> m_shortbuf;
  std::vector<char> m_charbuf;
};
