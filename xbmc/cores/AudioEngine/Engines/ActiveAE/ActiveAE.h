#pragma once
/*
 *      Copyright (C) 2010-2013 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "system.h"
#include "threads/Thread.h"

#include "ActiveAESink.h"
#include "ActiveAEResample.h"
#include "Interfaces/AEStream.h"
#include "Interfaces/AESound.h"
#include "AEFactory.h"
#include "guilib/DispResource.h"

// ffmpeg
#include "DllAvFormat.h"
#include "DllAvCodec.h"
#include "DllAvUtil.h"

class IAESink;
class IAEEncoder;

namespace ActiveAE
{

class CActiveAESound;
class CActiveAEStream;

struct AudioSettings
{
  std::string device;
  std::string driver;
  std::string passthoughdevice;
  int channels;
  bool ac3passthrough;
  bool eac3passthrough;
  bool dtspassthrough;
  bool truehdpassthrough;
  bool dtshdpassthrough;
  bool stereoupmix;
  bool passthrough;
  int config;
  unsigned int samplerate;
  AEQuality resampleQuality;
};

class CActiveAEControlProtocol : public Protocol
{
public:
  CActiveAEControlProtocol(std::string name, CEvent* inEvent, CEvent *outEvent) : Protocol(name, inEvent, outEvent) {};
  enum OutSignal
  {
    INIT = 0,
    RECONFIGURE,
    SUSPEND,
    MUTE,
    VOLUME,
    PAUSESTREAM,
    RESUMESTREAM,
    FLUSHSTREAM,
    STREAMRGAIN,
    STREAMVOLUME,
    STREAMAMP,
    STREAMRESAMPLERATIO,
    STREAMFADE,
    STOPSOUND,
    SOUNDMODE,
    GETSTATE,
    DISPLAYLOST,
    DISPLAYRESET,
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
  CActiveAEDataProtocol(std::string name, CEvent* inEvent, CEvent *outEvent) : Protocol(name, inEvent, outEvent) {};
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
  } parameter;
};

struct MsgStreamFade
{
  CActiveAEStream *stream;
  float from;
  float target;
  unsigned int millis;
};

class CEngineStats
{
public:
  void Reset(unsigned int sampleRate);
  void UpdateSinkDelay(double delay, int samples);
  void AddSamples(int samples, std::list<CActiveAEStream*> &streams);
  float GetDelay();
  float GetDelay(CActiveAEStream *stream);
  float GetCacheTime(CActiveAEStream *stream);
  float GetCacheTotal(CActiveAEStream *stream);
  float GetWaterLevel();
  void SetSuspended(bool state);
  void SetSinkCacheTotal(float time) { m_sinkCacheTotal = time; }
  bool IsSuspended();
  CCriticalSection *GetLock() { return &m_lock; }
protected:
  float m_sinkDelay;
  float m_sinkCacheTotal;
  int m_bufferedSamples;
  unsigned int m_sinkSampleRate;
  unsigned int m_sinkUpdate;
  bool m_suspended;
  CCriticalSection m_lock;
};

#if defined(HAS_GLX) || defined(TARGET_DARWIN_OSX)
class CActiveAE : public IAE, public IDispResource, private CThread
#else
class CActiveAE : public IAE, private CThread
#endif
{
protected:
  friend class ::CAEFactory;
  friend class CActiveAESound;
  friend class CActiveAEStream;
  friend class CSoundPacket;
  friend class CActiveAEBufferPoolResample;
  CActiveAE();
  virtual ~CActiveAE();
  virtual bool  Initialize();

public:
  virtual void   Shutdown();
  virtual bool   Suspend();
  virtual bool   Resume();
  virtual bool   IsSuspended();
  virtual void   OnSettingsChange(const std::string& setting);

  virtual float GetVolume();
  virtual void  SetVolume(const float volume);
  virtual void  SetMute(const bool enabled);
  virtual bool  IsMuted();
  virtual void  SetSoundMode(const int mode);

  /* returns a new stream for data in the specified format */
  virtual IAEStream *MakeStream(enum AEDataFormat dataFormat, unsigned int sampleRate, unsigned int encodedSampleRate, CAEChannelInfo channelLayout, unsigned int options = 0);
  virtual IAEStream *FreeStream(IAEStream *stream);

  /* returns a new sound object */
  virtual IAESound *MakeSound(const std::string& file);
  virtual void      FreeSound(IAESound *sound);

  virtual void GarbageCollect() {};

  virtual void EnumerateOutputDevices(AEDeviceList &devices, bool passthrough);
  virtual std::string GetDefaultDevice(bool passthrough);
  virtual bool SupportsRaw(AEDataFormat format);
  virtual bool SupportsDrain();
  virtual bool SupportsQualityLevel(enum AEQuality level);
  virtual bool IsSettingVisible(const std::string &settingId);

  virtual void RegisterAudioCallback(IAudioCallback* pCallback);
  virtual void UnregisterAudioCallback();

  virtual void OnLostDevice();
  virtual void OnResetDevice();

protected:
  void PlaySound(CActiveAESound *sound);
  uint8_t **AllocSoundSample(SampleConfig &config, int &samples, int &bytes_per_sample, int &planes, int &linesize);
  void FreeSoundSample(uint8_t **data);
  float GetDelay(CActiveAEStream *stream) { return m_stats.GetDelay(stream); }
  float GetCacheTime(CActiveAEStream *stream) { return m_stats.GetCacheTime(stream); }
  float GetCacheTotal(CActiveAEStream *stream) { return m_stats.GetCacheTotal(stream); }
  void FlushStream(CActiveAEStream *stream);
  void PauseStream(CActiveAEStream *stream, bool pause);
  void StopSound(CActiveAESound *sound);
  void SetStreamAmplification(CActiveAEStream *stream, float amplify);
  void SetStreamReplaygain(CActiveAEStream *stream, float rgain);
  void SetStreamVolume(CActiveAEStream *stream, float volume);
  void SetStreamResampleRatio(CActiveAEStream *stream, double ratio);
  void SetStreamFade(CActiveAEStream *stream, float from, float target, unsigned int millis);

protected:
  void Process();
  void StateMachine(int signal, Protocol *port, Message *msg);
  bool InitSink();
  void DrainSink();
  void UnconfigureSink();
  bool IsSinkCompatible(const AEAudioFormat format, const std::string &device);
  void Start();
  void Dispose();
  void LoadSettings();
  bool NeedReconfigureBuffers();
  bool NeedReconfigureSink();
  void ApplySettingsToFormat(AEAudioFormat &format, AudioSettings &settings, int *mode = NULL);
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

  void ResampleSounds();
  bool ResampleSound(CActiveAESound *sound);
  void MixSounds(CSoundPacket &dstSample);
  void Deamplify(CSoundPacket &dstSample);

  bool CompareFormat(AEAudioFormat &lhs, AEAudioFormat &rhs);

  CEvent m_inMsgEvent;
  CEvent m_outMsgEvent;
  CActiveAEControlProtocol m_controlPort;
  CActiveAEDataProtocol m_dataPort;
  int m_state;
  bool m_bStateMachineSelfTrigger;
  int m_extTimeout;
  bool m_extError;
  bool m_extDrain;
  XbmcThreads::EndTime m_extDrainTimer;
  bool m_extDeferData;

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

  // buffers
  CActiveAEBufferPoolResample *m_sinkBuffers;
  CActiveAEBufferPoolResample *m_vizBuffers;
  CActiveAEBufferPool *m_silenceBuffers;  // needed to drive gui sounds if we have no streams
  CActiveAEBufferPool *m_encoderBuffers;

  // streams
  std::list<CActiveAEStream*> m_streams;
  std::list<CActiveAEBufferPool*> m_discardBufferPools;

  // gui sounds
  struct SoundState
  {
    CActiveAESound *sound;
    int samples_played;
  };
  std::list<SoundState> m_sounds_playing;
  std::vector<CActiveAESound*> m_sounds;
  int m_soundMode;

  float m_volume;
  bool m_muted;
  bool m_sinkHasVolume;

  // viz
  IAudioCallback *m_audioCallback;
  bool m_vizInitialized;
  CCriticalSection m_vizLock;

  // ffmpeg
  DllAvFormat m_dllAvFormat;
  DllAvCodec  m_dllAvCodec;
  DllAvUtil   m_dllAvUtil;

  // polled via the interface
  float m_aeVolume;
  bool m_aeMuted;
};
};
