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

#include <samplerate.h>
#include <list>

#include "threads/SharedSection.h"

#include "AEAudioFormat.h"
#include "Interfaces/AEStream.h"
#include "Utils/AEConvert.h"
#include "Utils/AERemap.h"
#include "Utils/AEBuffer.h"
#include "Utils/AELimiter.h"

class IAEPostProc;
class CSoftAEStream : public IAEStream
{
protected:
  friend class CSoftAE;
  CSoftAEStream(enum AEDataFormat format, unsigned int sampleRate, unsigned int encodedSamplerate, CAEChannelInfo channelLayout, unsigned int options, CCriticalSection& lock);
  virtual ~CSoftAEStream();

  void Initialize();
  void InitializeRemap();
  void Destroy();
  uint8_t* GetFrame();

  bool IsPaused   () { return m_paused; }
  bool IsDestroyed() { return m_delete; }
  bool IsValid    () { return m_valid;  }
  const bool IsRaw() const { return AE_IS_RAW(m_initDataFormat); }  

public:
  virtual unsigned int      GetSpace        ();
  virtual unsigned int      AddData         (void *data, unsigned int size);
  virtual double            GetDelay        ();
  virtual bool              IsBuffering     () { return m_refillBuffer > 0; }
  virtual double            GetCacheTime    ();
  virtual double            GetCacheTotal   ();

  virtual void              Pause           ();
  virtual void              Resume          ();
  virtual void              Drain           (bool wait);
  virtual bool              IsDraining      () { return m_draining;    }
  virtual bool              IsDrained       ();
  virtual void              Flush           ();

  virtual float             GetVolume       ()             { return m_volume; }
  virtual float             GetReplayGain   ()             { return m_rgain ; }
  virtual float             GetAmplification()             { return m_limiter.GetAmplification(); }
  virtual void              SetVolume       (float volume) { m_volume = std::max( 0.0f, std::min(1.0f, volume)); }
  virtual void              SetReplayGain   (float factor) { m_rgain  = std::max( 0.0f, factor); }
  virtual void              SetAmplification(float amplify){ m_limiter.SetAmplification(amplify); }

  virtual float             RunLimiter(float* frame, int channels) { return m_limiter.Run(frame, channels); }

  virtual const unsigned int      GetFrameSize   () const  { return m_format.m_frameSize; }
  virtual const unsigned int      GetChannelCount() const  { return m_initChannelLayout.Count(); }
  
  virtual const unsigned int      GetSampleRate  () const  { return m_initSampleRate; }
  virtual const unsigned int GetEncodedSampleRate() const  { return m_initEncodedSampleRate; }
  virtual const enum AEDataFormat GetDataFormat  () const  { return m_initDataFormat; }
  
  virtual double            GetResampleRatio();
  virtual bool              SetResampleRatio(double ratio);
  virtual void              RegisterAudioCallback(IAudioCallback* pCallback);
  virtual void              UnRegisterAudioCallback();
  virtual void              FadeVolume(float from, float to, unsigned int time);
  virtual bool              IsFading();
  virtual void              RegisterSlave(IAEStream *stream);
private:
  void InternalFlush();
  void CheckResampleBuffers();

  CCriticalSection& m_lock;
  enum AEDataFormat m_initDataFormat;
  unsigned int      m_initSampleRate;
  unsigned int      m_initEncodedSampleRate;
  CAEChannelInfo    m_initChannelLayout;
  unsigned int      m_chLayoutCount;
  
  typedef struct
  {
    CAEBuffer data;
    CAEBuffer vizData;
  } PPacket;

  AEAudioFormat m_format;

  bool                    m_forceResample; /* true if we are to force resample even when the rates match */
  bool                    m_resample;      /* true if the audio needs to be resampled  */
  double                  m_resampleRatio; /* user specified resample ratio */
  double                  m_internalRatio; /* internal resample ratio */ 
  bool                    m_convert;       /* true if the bitspersample needs converting */
  float                  *m_convertBuffer; /* buffer for converted data */
  bool                    m_valid;         /* true if the stream is valid */
  bool                    m_delete;        /* true if CSoftAE is to free this object */
  CAERemap                m_remap;         /* the remapper */
  float                   m_volume;        /* the volume level */
  float                   m_rgain;         /* replay gain level */
  unsigned int            m_waterLevel;    /* the fill level to fall below before calling the data callback */
  unsigned int            m_refillBuffer;  /* how many frames that need to be buffered before we return any frames */

  CAEConvert::AEConvertToFn m_convertFn;

  CAEBuffer           m_inputBuffer;
  unsigned int        m_bytesPerSample;
  unsigned int        m_bytesPerFrame;
  unsigned int        m_samplesPerFrame;
  CAEChannelInfo      m_aeChannelLayout;
  unsigned int        m_aeBytesPerFrame;
  SRC_STATE          *m_ssrc;
  SRC_DATA            m_ssrcData;
  unsigned int        m_framesBuffered;
  std::list<PPacket*> m_outBuffer;
  unsigned int        ProcessFrameBuffer();
  PPacket            *m_newPacket;
  PPacket            *m_packet;
  uint8_t            *m_packetPos;
  float              *m_vizPacketPos;
  bool                m_paused;
  bool                m_autoStart;
  bool                m_draining;
  CAELimiter          m_limiter;

  /* vizualization internals */
  CAERemap           m_vizRemap;
  float              m_vizBuffer[512];
  unsigned int       m_vizBufferSamples;
  IAudioCallback    *m_audioCallback;

  /* fade values */
  bool               m_fadeRunning;
  bool               m_fadeDirUp;
  float              m_fadeStep;
  float              m_fadeTarget;
  unsigned int       m_fadeTime;

  /* slave stream */
  CSoftAEStream     *m_slave;
};

