/*
 *  Copyright (C) 2010-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/AudioEngine/Engines/ActiveAE/ActiveAE.h"
#include "cores/AudioEngine/Engines/ActiveAE/ActiveAEBuffer.h"
#include "cores/AudioEngine/Interfaces/AEStream.h"
#include "cores/AudioEngine/Utils/AEAudioFormat.h"
#include "cores/AudioEngine/Utils/AELimiter.h"
#include "threads/Event.h"

#include <atomic>
#include <deque>

namespace ActiveAE
{
class CActiveAE;

class CSyncError
{
public:
  CSyncError()
  {
    Flush();
  }
  void Add(double error)
  {
    m_buffer += error;
    m_count++;
  }

  void Flush(std::chrono::milliseconds interval = std::chrono::milliseconds(100))
  {
    m_buffer = 0.0;
    m_lastError = 0.0;
    m_count  = 0;
    m_timer.Set(interval);
  }

  void SetErrorInterval(std::chrono::milliseconds interval = std::chrono::milliseconds(100))
  {
    m_buffer = 0.0;
    m_count = 0;
    m_timer.Set(interval);
  }

  bool Get(double& error, std::chrono::milliseconds interval = std::chrono::milliseconds(100))
  {
    if(m_timer.IsTimePast())
    {
      error = Get();
      Flush(interval);
      m_lastError = error;
      return true;
    }
    else
    {
      error = m_lastError;
      return false;
    }
  }

  double GetLastError(unsigned int &time)
  {
    time = m_timer.GetStartTime().time_since_epoch().count();
    return m_lastError;
  }

  void Correction(double correction)
  {
    m_lastError += correction;
  }

protected:
  double Get() const
  {
    if(m_count)
      return m_buffer / m_count;
    else
      return 0.0;
  }
  double m_buffer;
  double m_lastError;
  int m_count;
  XbmcThreads::EndTime<> m_timer;
};

class CActiveAEStreamBuffers
{
public:
  CActiveAEStreamBuffers(const AEAudioFormat& inputFormat, const AEAudioFormat& outputFormat, AEQuality quality);
  virtual ~CActiveAEStreamBuffers();
  bool Create(unsigned int totaltime, bool remap, bool upmix, bool normalize = true);
  void SetExtraData(int profile, enum AVMatrixEncoding matrix_encoding, enum AVAudioServiceType audio_service_type);
  bool ProcessBuffers();
  void ConfigureResampler(bool normalizelevels, bool stereoupmix, AEQuality quality);
  bool HasInputLevel(int level);
  float GetDelay();
  void Flush();
  void SetDrain(bool drain);
  bool IsDrained();
  void SetRR(double rr, double atempoThreshold);
  double GetRR();
  void FillBuffer();
  bool DoesNormalize();
  void ForceResampler(bool force);
  bool HasWork();
  std::unique_ptr<CActiveAEBufferPool> GetResampleBuffers();
  std::unique_ptr<CActiveAEBufferPool> GetAtempoBuffers();

  AEAudioFormat m_inputFormat;
  std::deque<CSampleBuffer*> m_outputSamples;
  std::deque<CSampleBuffer*> m_inputSamples;

protected:
  std::unique_ptr<CActiveAEBufferPoolResample> m_resampleBuffers;
  std::unique_ptr<CActiveAEBufferPoolAtempo> m_atempoBuffers;

private:
  CActiveAEStreamBuffers(const CActiveAEStreamBuffers&) = delete;
  CActiveAEStreamBuffers& operator=(const CActiveAEStreamBuffers&) = delete;
};

class CActiveAEStream : public IAEStream
{
protected:
  friend class CActiveAE;
  friend class CEngineStats;
  CActiveAEStream(AEAudioFormat *format, unsigned int streamid, CActiveAE *ae);
  ~CActiveAEStream() override = default;
  void FadingFinished();
  void IncFreeBuffers();
  void DecFreeBuffers();
  void ResetFreeBuffers();
  void InitRemapper();
  void RemapBuffer();
  double CalcResampleRatio(double error);
  std::chrono::milliseconds GetErrorInterval();

public:
  unsigned int GetSpace() override;
  unsigned int AddData(const uint8_t* const *data, unsigned int offset, unsigned int frames, ExtData *extData) override;
  double GetDelay() override;
  CAESyncInfo GetSyncInfo() override;
  bool IsBuffering() override;
  double GetCacheTime() override;
  double GetCacheTotal() override;
  double GetMaxDelay() override;

  void Pause() override;
  void Resume() override;
  void Drain(bool wait) override;
  bool IsDraining() override;
  bool IsDrained() override;
  void Flush() override;

  float GetVolume() override;
  float GetReplayGain() override;
  float GetAmplification() override;
  void SetVolume(float volume) override;
  void SetReplayGain(float factor) override;
  void SetAmplification(float amplify) override;
  void SetFFmpegInfo(int profile, enum AVMatrixEncoding matrix_encoding, enum AVAudioServiceType audio_service_type) override;

  unsigned int GetFrameSize() const override;
  unsigned int GetChannelCount() const override;

  unsigned int GetSampleRate() const override ;
  enum AEDataFormat GetDataFormat() const override;

  double GetResampleRatio() override;
  void SetResampleRatio(double ratio) override;
  void SetResampleMode(int mode) override;
  void RegisterAudioCallback(IAudioCallback* pCallback) override;
  void UnRegisterAudioCallback() override;
  void FadeVolume(float from, float to, unsigned int time) override;
  bool IsFading() override;
  void RegisterSlave(IAEStream *stream) override;

protected:

  CActiveAE *m_activeAE;
  unsigned int m_id;
  AEAudioFormat m_format;
  float m_streamVolume;
  float m_streamRgain;
  float m_streamAmplify;
  double m_streamResampleRatio;
  int m_streamResampleMode;
  unsigned int m_streamSpace;
  bool m_streamDraining;
  bool m_streamDrained;
  bool m_streamFading;
  int m_streamFreeBuffers;
  bool m_streamIsBuffering;
  bool m_streamIsFlushed;
  IAEStream *m_streamSlave;
  CCriticalSection m_streamLock;
  CCriticalSection m_statsLock;
  int m_leftoverBytes;
  CSampleBuffer *m_currentBuffer;
  std::unique_ptr<CSoundPacket> m_remapBuffer;
  std::unique_ptr<IAEResample> m_remapper;
  double m_lastPts;
  double m_lastPtsJump;
  std::chrono::milliseconds m_errorInterval{1000};

  // only accessed by engine
  std::unique_ptr<CActiveAEBufferPool> m_inputBuffers;
  std::unique_ptr<CActiveAEStreamBuffers> m_processingBuffers;
  std::deque<CSampleBuffer*> m_processingSamples;
  std::unique_ptr<CActiveAEDataProtocol> m_streamPort;
  CEvent m_inMsgEvent;
  bool m_drain;
  bool m_paused;
  bool m_started;
  CAELimiter m_limiter;
  float m_volume;
  float m_rgain;
  float m_amplify;
  float m_bufferedTime;
  int m_fadingSamples;
  float m_fadingBase;
  float m_fadingTarget;
  int m_fadingTime;
  int m_profile;
  int m_resampleMode;
  double m_resampleIntegral;
  double m_clockSpeed;
  enum AVMatrixEncoding m_matrixEncoding;
  enum AVAudioServiceType m_audioServiceType;
  bool m_forceResampler;
  IAEClockCallback *m_pClock;
  CSyncError m_syncError;
  double m_lastSyncError;
  CAESyncInfo::AESyncState m_syncState;
};
}

