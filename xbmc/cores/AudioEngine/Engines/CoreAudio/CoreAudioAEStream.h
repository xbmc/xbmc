#pragma once
/*
 *      Copyright (C) 2011-2012 Team XBMC
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

#include <samplerate.h>
#include <list>

#include "AEAudioFormat.h"
#include "CoreAudioRingBuffer.h"
#include "ICoreAudioSource.h"
#include "Interfaces/AEStream.h"
#include "cores/AudioEngine/Utils/AEConvert.h"
#include "cores/AudioEngine/Utils/AERemap.h"

#if defined(TARGET_DARWIN_IOS)
# include "CoreAudioAEHALIOS.h"
#else
# include "CoreAudioAEHALOSX.h"
#endif

class CoreAudioRingBuffer;

class CCoreAudioAEStream : public IAEStream, public ICoreAudioSource
{
protected:
  friend class CCoreAudioAE;
  CCoreAudioAEStream(enum AEDataFormat format, unsigned int sampleRate, unsigned int encodedSamplerate, CAEChannelInfo channelLayout, unsigned int options);
  virtual ~CCoreAudioAEStream();

  CAUOutputDevice    *m_outputUnit;

public:
  void ReinitConverter();
  void CloseConverter();
  void OpenConverter();

  void Initialize();
  void InitializeRemap();
  virtual void Destroy();

  virtual const unsigned int GetFrameSize() const;
  virtual unsigned int GetSpace();
  virtual unsigned int AddData(void *data, unsigned int size);
  unsigned int GetFrames(uint8_t *buffer, unsigned int size);
  virtual double GetDelay();
  virtual bool   IsBuffering();
  virtual double GetCacheTime();
  virtual double GetCacheTotal();

  bool IsPaused();
  virtual bool IsDraining();
  virtual bool IsDrained();
  bool IsDestroyed();
  bool IsValid();

  virtual void Pause();
  virtual void Resume();
  virtual void Drain();
  virtual void Flush();

  virtual float GetVolume();
  virtual float GetReplayGain();
  virtual void  SetVolume(float volume);
  virtual void  SetReplayGain(float factor);

  virtual const unsigned int      GetChannelCount() const;
  virtual const unsigned int      GetSampleRate() const;
  virtual const unsigned int      GetEncodedSampleRate() const;
  virtual const enum AEDataFormat GetDataFormat() const;
  virtual const bool              IsRaw() const;

  /* for dynamic sample rate changes (smoothvideo) */
  virtual double GetResampleRatio();
  virtual bool   SetResampleRatio(double ratio);

  virtual void RegisterAudioCallback(IAudioCallback* pCallback);
  virtual void UnRegisterAudioCallback();

  virtual void FadeVolume(float from, float to, unsigned int time);
  virtual bool IsFading();
  virtual void RegisterSlave(IAEStream *stream);

  OSStatus Render(AudioUnitRenderActionFlags* actionFlags, 
    const AudioTimeStamp* pTimeStamp, 
    UInt32 busNumber, 
    UInt32 frameCount, 
    AudioBufferList* pBufList);

private:
  void InternalFlush();

  OSStatus OnRender(AudioUnitRenderActionFlags *ioActionFlags, 
    const AudioTimeStamp *inTimeStamp, 
    UInt32 inBusNumber, 
    UInt32 inNumberFrames, 
    AudioBufferList *ioData);

  AEDataFormat            m_rawDataFormat;

  AEAudioFormat           m_OutputFormat;
  unsigned int            m_chLayoutCountOutput;
  AEAudioFormat           m_StreamFormat;
  unsigned int            m_chLayoutCountStream;
  unsigned int            m_StreamBytesPerSample;
  unsigned int            m_OutputBytesPerSample;

  //bool                    m_forceResample; /* true if we are to force resample even when the rates match */
  //bool                    m_resample;      /* true if the audio needs to be resampled  */
  bool                    m_convert;       /* true if the bitspersample needs converting */
  bool                    m_valid;         /* true if the stream is valid */
  bool                    m_delete;        /* true if CCoreAudioAE is to free this object */
  CAERemap                m_remap;         /* the remapper */
  float                   m_volume;        /* the volume level */
  float                   m_rgain;         /* replay gain level */
  IAEStream               *m_slave;        /* slave aestream */

  CAEConvert::AEConvertToFn m_convertFn;

  CoreAudioRingBuffer    *m_Buffer;
  float                  *m_convertBuffer;      /* buffer for converted data */
  int                     m_convertBufferSize;
  //float                  *m_resampleBuffer;     /* buffer for resample data */
  //int                     m_resampleBufferSize;
  uint8_t                *m_upmixBuffer;        /* buffer for remap data */
  int                     m_upmixBufferSize;
  uint8_t                *m_remapBuffer;        /* buffer for remap data */
  int                     m_remapBufferSize;
  uint8_t                *m_vizRemapBuffer;     /* buffer for remap data */
  int                     m_vizRemapBufferSize;

  SRC_STATE              *m_ssrc;
  SRC_DATA                m_ssrcData;
  bool                    m_paused;
  bool                    m_draining;
  unsigned int            m_AvgBytesPerSec;

  /* vizualization internals */
  CAERemap                m_vizRemap;
  IAudioCallback         *m_audioCallback;

  /* fade values */
  bool              m_fadeRunning;
  bool              m_fadeDirUp;
  float             m_fadeStep;
  float             m_fadeTarget;
  unsigned int      m_fadeTime;
  bool              m_isRaw;
  unsigned int      m_frameSize;
  bool              m_doRemap;
  void              Upmix(void *input, unsigned int channelsInput, void *output, unsigned int channelsOutput, unsigned int frames, AEDataFormat dataFormat);
  bool              m_firstInput;
};

