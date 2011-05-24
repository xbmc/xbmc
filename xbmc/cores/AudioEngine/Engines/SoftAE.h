#pragma once
/*
 *      Copyright (C) 2005-2010 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include <list>
#include <map>

#include "system.h"
#include "threads/Thread.h"
#include "threads/CriticalSection.h"
#include "threads/SharedSection.h"

#include "ThreadedAE.h"
#include "AEConvert.h"
#include "AERemap.h"
#include "AESink.h"
#include "AEAudioFormat.h"
#include "AEEncoder.h"

#include "SoftAEStream.h"
#include "SoftAESound.h"

#include "cores/IAudioCallback.h"

/* forward declarations */
class IThreadedAE;
class CSoftAEStream;
class CSoftAESound;
class IAESink;

class CSoftAE : public IThreadedAE
{
public:
  /* this should NEVER be called directly, use CAEFactory */
  CSoftAE();
  virtual ~CSoftAE();

  virtual bool  Initialize      ();
  virtual void  OnSettingsChange(CStdString setting);

  virtual void  Run();
  virtual void  Stop();
  virtual float GetDelay();

  virtual float GetVolume();
  virtual void  SetVolume(float volume);

  /* returns a new stream for data in the specified format */
  virtual IAEStream *GetStream  (enum AEDataFormat dataFormat, unsigned int sampleRate, unsigned int channelCount, AEChLayout channelLayout, unsigned int options = 0);
  virtual IAEStream *AlterStream(IAEStream *stream, enum AEDataFormat dataFormat, unsigned int sampleRate, unsigned int channelCount, AEChLayout channelLayout, unsigned int options = 0);
  virtual IAEStream *FreeStream(IAEStream *stream);

  /* returns a new sound object */
  virtual IAESound *GetSound(CStdString file);
  virtual void FreeSound(IAESound *sound);
  virtual void PlaySound(IAESound *sound);
  virtual void StopSound(IAESound *sound);

  /* free's sounds that have expired */
  virtual void GarbageCollect();

  /* these are for the streams so they can provide compatible data */
  virtual unsigned int   GetSampleRate   ();
  virtual unsigned int   GetChannelCount () {return m_channelCount          ;}
  virtual AEChLayout     GetChannelLayout() {return m_chLayout              ;}
  virtual unsigned int   GetFrames       () {return m_sinkFormat.m_frames   ;}
  virtual unsigned int   GetFrameSize    () {return m_frameSize             ;}

  /* these are for streams that are in RAW mode */
  const AEAudioFormat* GetSinkAudioFormat() {return &m_sinkFormat               ;}
  enum AEDataFormat    GetSinkDataFormat () {return m_sinkFormat.m_dataFormat   ;}
  AEChLayout           GetSinkChLayout   () {return m_sinkFormat.m_channelLayout;}
  unsigned int         GetSinkChCount    () {return m_sinkFormat.m_channelCount ;}
  unsigned int         GetSinkFrameSize  () {return m_sinkFormat.m_frameSize    ;}

  virtual void EnumerateOutputDevices(AEDeviceList &devices, bool passthrough);
  virtual bool SupportsRaw();

#ifdef __SSE__
  inline static void SSEMulAddArray(float *data, float *add, const float mul, uint32_t count);
  inline static void SSEMulArray   (float *data, const float mul, uint32_t count);
#endif

private:
  CThread *m_thread;

  void LoadSettings();
  bool OpenSink(unsigned int sampleRate = 44100, unsigned int channels = 2, bool forceRaw = false, enum AEDataFormat rawFormat = AE_FMT_AC3);
  void ResetEncoder();
  bool SetupEncoder(AEAudioFormat &format);
  void Deinitialize();

  IAESink *GetSink(AEAudioFormat &desiredFormat, bool passthrough, CStdString &device);

  unsigned int m_delayFrames;
  void DelayFrames();

  /* this is called by streams on dtor, you should never need to call this directly */
  friend class CSoftAEStream;
  void RemoveStream(IAEStream *stream);

  enum AEStdChLayout m_stdChLayout;
  CStdString m_device, m_driver;
  CStdString m_passthroughDevice, m_passthroughDriver;

  /* internal vars */
  bool m_running, m_reOpened;
  CCriticalSection m_runningLock;     /* released when the thread exits */
  CSharedSection   m_sinkLock;        /* sink & configuration lock */
  CCriticalSection m_streamLock;      /* m_streams lock */
  CCriticalSection m_soundLock;       /* m_sounds lock */
  CCriticalSection m_soundSampleLock; /* m_playing_sounds lock */

  /* the current configuration */
  float               m_volume;
  unsigned int        m_channelCount;
  AEChLayout          m_chLayout;
  unsigned int        m_frameSize;

  /* the sink, its format information, and conversion function */
  IAESink                  *m_sink;
  AEAudioFormat             m_sinkFormat;
  AEAudioFormat             m_encoderFormat;
  unsigned int              m_bytesPerSample;
  CAEConvert::AEConvertFrFn m_convertFn;

  /* currently playing sounds */
  typedef struct {
    CSoftAESound *owner;
    float        *samples;
    unsigned int  sampleCount;
  } SoundState;
  std::list<SoundState> m_playing_sounds;

  /* the streams, sounds, output buffer and output buffer fill size */
  bool                                      m_transcode;
  bool                                      m_rawPassthrough;
  std::list<CSoftAEStream*>                 m_streams;
  std::list<CSoftAESound*>                  m_sounds;

  /* this will contain either float, or uint8_t depending on if we are in raw mode or not */
  unsigned int                              m_bufferSize;
  void                                     *m_buffer;
  unsigned int                              m_bufferSamples;

  /* the encoder */
  IAEEncoder    *m_encoder;
  uint8_t       *m_encodedBuffer;
  unsigned int   m_encodedBufferSize;
  unsigned int   m_encodedBufferPos;
  unsigned int   m_encodedBufferFrames;
  bool           m_encodedPending;

  /* the channel remapper  */
  CAERemap        m_remap;
  float          *m_remapped;
  size_t          m_remappedSize;
  uint8_t        *m_converted;
  size_t          m_convertedSize;

  /* thread run stages */
  void         MixSounds        (float *buffer, unsigned int samples);
  void         FinalizeSamples  (float *buffer, unsigned int samples);

  void         RunOutputStage   ();

  void         RunTranscodeStage();
  unsigned int RunStreamStage   (unsigned int channelCount, void *out, bool &restart);
  void         RunNormalizeStage(unsigned int channelCount, void *out, unsigned int mixed);
  void         RunBufferStage   (void *out);
};

