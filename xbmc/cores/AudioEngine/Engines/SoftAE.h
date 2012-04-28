#pragma once
/*
 *      Copyright (C) 2010-2012 Team XBMC
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
#include <vector>
#include <map>

#include "system.h"
#include "threads/Thread.h"
#include "threads/CriticalSection.h"
#include "threads/SharedSection.h"

#include "Interfaces/ThreadedAE.h"
#include "Interfaces/AESink.h"
#include "Interfaces/AEEncoder.h"
#include "Utils/AEConvert.h"
#include "Utils/AERemap.h"
#include "Utils/AEBuffer.h"
#include "AEAudioFormat.h"
#include "AESinkFactory.h"

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
protected:
  friend class CAEFactory;
  CSoftAE();
  virtual ~CSoftAE();

public:
  virtual void  Shutdown();
  virtual bool  Initialize();
  virtual void  OnSettingsChange(std::string setting);

  virtual void   Run();
  virtual void   Stop();
  virtual double GetDelay();

  virtual float GetVolume();
  virtual void  SetVolume(const float volume);
  virtual void  SetMute(const bool enabled) { m_muted = enabled; }
  virtual bool  IsMuted() { return m_muted; }

  /* returns a new stream for data in the specified format */
  virtual IAEStream *MakeStream(enum AEDataFormat dataFormat, unsigned int sampleRate, unsigned int encodedSampleRate, CAEChannelInfo channelLayout, unsigned int options = 0);
  virtual IAEStream *FreeStream(IAEStream *stream);

  /* returns a new sound object */
  virtual IAESound *MakeSound(const std::string& file);
  virtual void      FreeSound(IAESound *sound);
  void PlaySound(IAESound *sound);
  void StopSound(IAESound *sound);

  /* free's sounds that have expired */
  virtual void GarbageCollect();

  /* these are for the streams so they can provide compatible data */
  unsigned int          GetSampleRate   ();
  unsigned int          GetChannelCount () {return m_chLayout.Count()      ;}
  CAEChannelInfo&       GetChannelLayout() {return m_chLayout              ;}
  enum AEStdChLayout    GetStdChLayout  () {return m_stdChLayout           ;}
  unsigned int          GetFrames       () {return m_sinkFormat.m_frames   ;}
  unsigned int          GetFrameSize    () {return m_frameSize             ;}

  /* these are for streams that are in RAW mode */
  const AEAudioFormat*  GetSinkAudioFormat() {return &m_sinkFormat               ;}
  enum AEDataFormat     GetSinkDataFormat () {return m_sinkFormat.m_dataFormat   ;}
  CAEChannelInfo&       GetSinkChLayout   () {return m_sinkFormat.m_channelLayout;}
  unsigned int          GetSinkFrameSize  () {return m_sinkFormat.m_frameSize    ;}

  /* for streams so they can calc cachetimes correct */
  double GetCacheTime();
  double GetCacheTotal();

  virtual void EnumerateOutputDevices(AEDeviceList &devices, bool passthrough);
  virtual std::string GetDefaultDevice(bool passthrough);
  virtual bool SupportsRaw();

  /* internal stream methods */
  void PauseStream (CSoftAEStream *stream);
  void ResumeStream(CSoftAEStream *stream);

private:
  CThread *m_thread;

  CSoftAEStream *GetMasterStream();

  void LoadSettings();
  void VerifySoundDevice(std::string &device, bool passthrough);
  void OpenSink();

  void InternalOpenSink();
  void ResetEncoder();
  bool SetupEncoder(AEAudioFormat &format);
  void Deinitialize();

  IAESink *GetSink(AEAudioFormat &desiredFormat, bool passthrough, std::string &device);
  void StopAllSounds();

  enum AEStdChLayout m_stdChLayout;
  std::string m_device;
  std::string m_passthroughDevice;
  bool m_audiophile;
  bool m_stereoUpmix;

  /* internal vars */
  bool             m_running, m_reOpen, m_reOpened;
  CEvent           m_reOpenEvent;

  CCriticalSection m_runningLock;     /* released when the thread exits */
  CCriticalSection m_streamLock;      /* m_streams lock */
  CCriticalSection m_soundLock;       /* m_sounds lock */
  CCriticalSection m_soundSampleLock; /* m_playing_sounds lock */
  CSharedSection   m_sinkLock;        /* lock for m_sink on re-open */

  /* the current configuration */
  float               m_volume;
  bool                m_muted;
  CAEChannelInfo      m_chLayout;
  unsigned int        m_frameSize;

  /* the sink, its format information, and conversion function */
  AESinkInfoList            m_sinkInfoList;
  IAESink                  *m_sink;
  AEAudioFormat             m_sinkFormat;
  float                     m_sinkFormatSampleRateMul;
  float                     m_sinkFormatFrameSizeMul;
  unsigned int              m_sinkBlockSize;
  AEAudioFormat             m_encoderFormat;
  float                     m_encoderFrameSizeMul;
  unsigned int              m_bytesPerSample;
  CAEConvert::AEConvertFrFn m_convertFn;

  /* currently playing sounds */
  typedef struct {
    CSoftAESound *owner;
    float        *samples;
    unsigned int  sampleCount;
  } SoundState;

  typedef std::vector<CSoftAEStream*> StreamList;
  typedef std::list  <CSoftAESound* > SoundList;
  typedef std::list  <SoundState    > SoundStateList;  
    
  /* the streams, sounds, output buffer and output buffer fill size */
  bool           m_transcode;
  bool           m_rawPassthrough;
  StreamList     m_newStreams, m_streams, m_playingStreams;
  SoundList      m_sounds;
  SoundStateList m_playing_sounds;

  /* this will contain either float, or uint8_t depending on if we are in raw mode or not */
  CAEBuffer      m_buffer;

  /* the encoder */
  IAEEncoder    *m_encoder;
  CAEBuffer      m_encodedBuffer;

  /* the channel remapper  */
  CAERemap        m_remap;
  float          *m_remapped;
  size_t          m_remappedSize;
  uint8_t        *m_converted;
  size_t          m_convertedSize;

  /* thread run stages */
  void         MixSounds        (float *buffer, unsigned int samples);
  void         FinalizeSamples  (float *buffer, unsigned int samples);

  CSoftAEStream *m_masterStream;

  void         (CSoftAE::*m_outputStageFn)();
  void         RunOutputStage   ();
  void         RunRawOutputStage();
  void         RunTranscodeStage();

  unsigned int (CSoftAE::*m_streamStageFn)(unsigned int channelCount, void *out, bool &restart);
  unsigned int RunRawStreamStage (unsigned int channelCount, void *out, bool &restart);
  unsigned int RunStreamStage    (unsigned int channelCount, void *out, bool &restart);

  void         ResumeSlaveStreams(const StreamList &streams);
  void         RunNormalizeStage (unsigned int channelCount, void *out, unsigned int mixed);

  void         RemoveStream(StreamList &streams, CSoftAEStream *stream);
};

