/*
 *  Copyright (C) 2010-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "ActiveAESink.h"
#include "cores/AudioEngine/Engines/ActiveAE/ActiveAEBuffer.h"
#include "cores/AudioEngine/Interfaces/AESound.h"
#include "cores/AudioEngine/Interfaces/AEStream.h"
#include "guilib/DispResource.h"
#include "threads/SystemClock.h"
#include "threads/Thread.h"

#include <list>
#include <memory>
#include <queue>
#include <string>
#include <utility>
#include <vector>

// ffmpeg
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
}

class IAESink;
class IAEEncoder;

namespace ActiveAE
{

class CActiveAESound;
class CActiveAEStream;
class CActiveAESettings;

struct AudioSettings
{
  std::string device;
  std::string driver;
  std::string passthroughdevice;
  int channels;
  bool ac3passthrough;
  bool ac3transcode;
  bool eac3passthrough;
  bool dtspassthrough;
  bool truehdpassthrough;
  bool dtshdpassthrough;
  bool usesdtscorefallback;
  bool stereoupmix;
  bool normalizelevels;
  bool passthrough;
  int config;
  int guisoundmode;
  unsigned int samplerate;
  AEQuality resampleQuality;
  double atempoThreshold;
  bool streamNoise;
  int silenceTimeoutMinutes;
};

class CActiveAEControlProtocol : public Protocol
{
public:
  CActiveAEControlProtocol(std::string name, CEvent* inEvent, CEvent* outEvent)
    : Protocol(std::move(name), inEvent, outEvent)
  {
  }
  enum OutSignal
  {
    INIT = 0,
    RECONFIGURE,
    SUSPEND,
    DEVICECHANGE,
    DEVICECOUNTCHANGE,
    MUTE,
    VOLUME,
    PAUSESTREAM,
    RESUMESTREAM,
    FLUSHSTREAM,
    STREAMRGAIN,
    STREAMVOLUME,
    STREAMAMP,
    STREAMRESAMPLERATIO,
    STREAMRESAMPLEMODE,
    STREAMFADE,
    STREAMFFMPEGINFO,
    STOPSOUND,
    GETSTATE,
    DISPLAYLOST,
    DISPLAYRESET,
    APPFOCUSED,
    KEEPCONFIG,
    TIMEOUT,
  };
  enum InSignal
  {
    ACC,
    ERR,
    STATS,
  };
};

class CActiveAEDataProtocol : public Protocol
{
public:
  CActiveAEDataProtocol(std::string name, CEvent* inEvent, CEvent* outEvent)
    : Protocol(std::move(name), inEvent, outEvent)
  {
  }
  enum OutSignal
  {
    NEWSOUND = 0,
    PLAYSOUND,
    FREESOUND,
    NEWSTREAM,
    FREESTREAM,
    STREAMSAMPLE,
    DRAINSTREAM,
  };
  enum InSignal
  {
    ACC,
    ERR,
    STREAMBUFFER,
    STREAMDRAINED,
  };
};

struct MsgStreamNew
{
  AEAudioFormat format;
  unsigned int options;
  IAEClockCallback *clock;
};

struct MsgStreamFree
{
  CActiveAEStream *stream;
  bool finish; // if true switch back to gui sound mode
};

struct MsgStreamSample
{
  CSampleBuffer *buffer;
  CActiveAEStream *stream;
};

struct MsgStreamParameter
{
  CActiveAEStream *stream;
  union
  {
    float float_par;
    double double_par;
    int int_par;
  } parameter;
};

struct MsgStreamFade
{
  CActiveAEStream *stream;
  float from;
  float target;
  unsigned int millis;
};

struct MsgStreamFFmpegInfo
{
  CActiveAEStream *stream;
  int profile;
  enum AVMatrixEncoding matrix_encoding;
  enum AVAudioServiceType audio_service_type;
};

class CEngineStats
{
public:
  void Reset(unsigned int sampleRate, bool pcm);
  void UpdateSinkDelay(const AEDelayStatus& status, int samples);
  void AddSamples(int samples, const std::list<CActiveAEStream*>& streams);
  void GetDelay(AEDelayStatus& status);
  void AddStream(unsigned int streamid);
  void RemoveStream(unsigned int streamid);
  void UpdateStream(CActiveAEStream *stream);
  void GetDelay(AEDelayStatus& status, CActiveAEStream *stream);
  void GetSyncInfo(CAESyncInfo& info, CActiveAEStream *stream);
  float GetCacheTime(CActiveAEStream *stream);
  float GetCacheTotal();
  float GetMaxDelay() const;
  float GetWaterLevel();
  void SetSuspended(bool state);
  void SetCurrentSinkFormat(const AEAudioFormat& SinkFormat);
  void SetSinkCacheTotal(float time) { m_sinkCacheTotal = time; }
  void SetSinkLatency(float time) { m_sinkLatency = time; }
  void SetSinkNeedIec(bool needIEC) { m_sinkNeedIecPack = needIEC; }
  bool IsSuspended();
  AEAudioFormat GetCurrentSinkFormat();
protected:
  float m_sinkCacheTotal;
  float m_sinkLatency;
  int m_bufferedSamples;
  unsigned int m_sinkSampleRate;
  AEDelayStatus m_sinkDelay;
  bool m_suspended;
  AEAudioFormat m_sinkFormat;
  bool m_pcmOutput;
  bool m_sinkNeedIecPack{false};
  CCriticalSection m_lock;
  struct StreamStats
  {
    unsigned int m_streamId;
    double m_bufferedTime;
    double m_resampleRatio;
    double m_syncError;
    unsigned int m_errorTime;
    CAESyncInfo::AESyncState m_syncState;
  };
  std::vector<StreamStats> m_streamStats;
};

class CActiveAE : public IAE, public IDispResource, private CThread
{
protected:
  friend class CActiveAESound;
  friend class CActiveAEStream;
  friend class CSoundPacket;
  friend class CActiveAEBufferPoolResample;

public:
  CActiveAE();
  ~CActiveAE() override;
  void Start() override;
  void Shutdown() override;
  bool Suspend() override;
  bool Resume() override;
  bool IsSuspended() override;
  void OnSettingsChange();

  float GetVolume() override;
  void SetVolume(const float volume) override;
  void SetMute(const bool enabled) override;
  bool IsMuted() override;

  /* returns a new stream for data in the specified format */
  IAE::StreamPtr MakeStream(AEAudioFormat& audioFormat,
                            unsigned int options = 0,
                            IAEClockCallback* clock = NULL) override;

  /* returns a new sound object */
  IAE::SoundPtr MakeSound(const std::string& file) override;

  void EnumerateOutputDevices(AEDeviceList &devices, bool passthrough) override;
  bool SupportsRaw(AEAudioFormat &format) override;
  bool SupportsSilenceTimeout() override;
  bool UsesDtsCoreFallback() override;
  bool HasStereoAudioChannelCount() override;
  bool HasHDAudioChannelCount() override;
  bool SupportsQualityLevel(enum AEQuality level) override;
  bool IsSettingVisible(const std::string &settingId) override;
  void KeepConfiguration(unsigned int millis) override;
  void DeviceChange() override;
  void DeviceCountChange(const std::string& driver) override;
  bool GetCurrentSinkFormat(AEAudioFormat &SinkFormat) override;

  void RegisterAudioCallback(IAudioCallback* pCallback) override;
  void UnregisterAudioCallback(IAudioCallback* pCallback) override;

  void OnLostDisplay() override;
  void OnResetDisplay() override;
  void OnAppFocusChange(bool focus) override;

private:
  bool FreeStream(IAEStream* stream, bool finish) override;
  void FreeSound(IAESound* sound) override;

protected:
  void PlaySound(CActiveAESound *sound);
  static uint8_t **AllocSoundSample(SampleConfig &config, int &samples, int &bytes_per_sample, int &planes, int &linesize);
  static void FreeSoundSample(uint8_t **data);
  void GetDelay(AEDelayStatus& status, CActiveAEStream *stream) { m_stats.GetDelay(status, stream); }
  void GetSyncInfo(CAESyncInfo& info, CActiveAEStream *stream) { m_stats.GetSyncInfo(info, stream); }
  float GetCacheTime(CActiveAEStream *stream) { return m_stats.GetCacheTime(stream); }
  float GetCacheTotal() { return m_stats.GetCacheTotal(); }
  float GetMaxDelay() { return m_stats.GetMaxDelay(); }
  void FlushStream(CActiveAEStream *stream);
  void PauseStream(CActiveAEStream *stream, bool pause);
  void StopSound(CActiveAESound *sound);
  void SetStreamAmplification(CActiveAEStream *stream, float amplify);
  void SetStreamReplaygain(CActiveAEStream *stream, float rgain);
  void SetStreamVolume(CActiveAEStream *stream, float volume);
  void SetStreamResampleRatio(CActiveAEStream *stream, double ratio);
  void SetStreamResampleMode(CActiveAEStream *stream, int mode);
  void SetStreamFFmpegInfo(CActiveAEStream *stream, int profile, enum AVMatrixEncoding matrix_encoding, enum AVAudioServiceType audio_service_type);
  void SetStreamFade(CActiveAEStream *stream, float from, float target, unsigned int millis);

protected:
  void Process() override;
  void StateMachine(int signal, Protocol *port, Message *msg);
  bool InitSink();
  void DrainSink();
  void UnconfigureSink();
  void Dispose();
  void LoadSettings();
  void ValidateOutputDevices(bool saveChanges);
  bool NeedReconfigureBuffers();
  bool NeedReconfigureSink();
  void ApplySettingsToFormat(AEAudioFormat& format,
                             const AudioSettings& settings,
                             int* mode = NULL);
  void Configure(AEAudioFormat *desiredFmt = NULL);
  AEAudioFormat GetInputFormat(AEAudioFormat *desiredFmt = NULL);
  CActiveAEStream* CreateStream(MsgStreamNew *streamMsg);
  void DiscardStream(CActiveAEStream *stream);
  void SFlushStream(CActiveAEStream *stream);
  void FlushEngine();
  void ClearDiscardedBuffers();
  void SStopSound(CActiveAESound *sound);
  void DiscardSound(CActiveAESound *sound);
  void ChangeResamplers();

  bool RunStages();
  bool HasWork();
  CSampleBuffer* SyncStream(CActiveAEStream *stream);

  void ResampleSounds();
  bool ResampleSound(CActiveAESound *sound);
  void MixSounds(CSoundPacket &dstSample);
  void Deamplify(CSoundPacket &dstSample);

  bool CompareFormat(const AEAudioFormat& lhs, const AEAudioFormat& rhs);

  CEvent m_inMsgEvent;
  CEvent m_outMsgEvent;
  CActiveAEControlProtocol m_controlPort;
  CActiveAEDataProtocol m_dataPort;
  int m_state;
  bool m_bStateMachineSelfTrigger;
  std::chrono::milliseconds m_extTimeout;
  bool m_extError;
  bool m_extDrain;
  XbmcThreads::EndTime<> m_extDrainTimer;
  std::chrono::milliseconds m_extKeepConfig;
  bool m_extDeferData;
  std::queue<time_t> m_extLastDeviceChange;
  bool m_extSuspended = false;
  bool m_isWinSysReg = false;

  enum
  {
    MODE_RAW,
    MODE_TRANSCODE,
    MODE_PCM
  }m_mode;

  CActiveAESink m_sink;
  AEAudioFormat m_sinkFormat;
  AEAudioFormat m_sinkRequestFormat;
  AEAudioFormat m_encoderFormat;
  AEAudioFormat m_internalFormat;
  AEAudioFormat m_inputFormat;
  AudioSettings m_settings;
  CEngineStats m_stats;
  IAEEncoder *m_encoder;
  std::string m_currDevice;
  std::unique_ptr<CActiveAESettings> m_settingsHandler;

  // buffers
  std::unique_ptr<CActiveAEBufferPoolResample> m_sinkBuffers;
  std::unique_ptr<CActiveAEBufferPoolResample> m_vizBuffers;
  std::unique_ptr<CActiveAEBufferPool> m_vizBuffersInput;
  std::unique_ptr<CActiveAEBufferPool>
      m_silenceBuffers; // needed to drive gui sounds if we have no streams
  std::unique_ptr<CActiveAEBufferPool> m_encoderBuffers;

  // streams
  std::list<CActiveAEStream*> m_streams;
  std::list<std::unique_ptr<CActiveAEBufferPool>> m_discardBufferPools;
  unsigned int m_streamIdGen;

  // gui sounds
  struct SoundState
  {
    CActiveAESound *sound;
    int samples_played;
  };
  std::list<SoundState> m_sounds_playing;
  std::vector<CActiveAESound*> m_sounds;

  float m_volume; // volume on a 0..1 scale corresponding to a proportion along the dB scale
  float m_volumeScaled; // multiplier to scale samples in order to achieve the volume specified in m_volume
  bool m_muted;
  bool m_sinkHasVolume;

  // viz
  std::vector<IAudioCallback*> m_audioCallback;
  bool m_vizInitialized;
  CCriticalSection m_vizLock;

  // polled via the interface
  float m_aeVolume;
  bool m_aeMuted;
  bool m_aeGUISoundForce;
};
};
