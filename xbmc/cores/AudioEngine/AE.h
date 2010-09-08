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
#include "utils/Thread.h"
#include "utils/CriticalSection.h"

#include "AEAudioFormat.h"
#include "AEStream.h"
#include "AESound.h"
#include "AEConvert.h"
#include "AERemap.h"
#include "AEPacketizer.h"
#include "AESink.h"

#include "cores/IAudioCallback.h"

/* forward declarations */
class CAEStream;
class CAESound;
class IAESink;

class CAE : public IRunnable
{
public:
  /* returns a singleton instance of the AE */
  static CAE& GetInstance()
  {
    static CAE* instance = NULL;
    if (!instance) instance = new CAE();
    return *instance;
  }

  virtual void Run();
  void         Stop();
  float        GetDelay();

  float        GetVolume();
  void         SetVolume(float volume);

  /* returns a new stream for data in the specified format */
  CAEStream *GetStream(enum AEDataFormat dataFormat, unsigned int sampleRate, unsigned int channelCount, AEChLayout channelLayout, bool freeOnDrain = false, bool ownsPostProc = false);

  /* returns a new sound object */
  CAESound *GetSound(CStdString file);
  void FreeSound(CAESound *sound);
  void PlaySound(CAESound *sound);
  void StopSound(CAESound *sound);
  bool IsPlaying(CAESound *sound);

  /* free's sounds that have expired */
  void GarbageCollect();

  /* these are for the streams so they can provide compatible data */
  unsigned int        GetSampleRate   () {return m_format.m_sampleRate   ;}
  unsigned int        GetChannelCount () {return m_channelCount          ;}
  AEChLayout          GetChannelLayout() {return m_chLayout              ;}
  unsigned int        GetFrames       () {return m_format.m_frames       ;}
  unsigned int        GetFrameSize    () {return m_frameSize             ;}
  IAEPacketizer      *GetPacketizer   () {return m_packetizer            ;}

  /* these are for streams that are in RAW mode */
  enum AEDataFormat   GetSinkDataFormat() {return m_format.m_dataFormat   ;}
  AEChLayout          GetSinkChLayout  () {return m_format.m_channelLayout;}
  unsigned int        GetSinkChCount   () {return m_format.m_channelCount ;}
  unsigned int        GetSinkFrameSize () {return m_format.m_frameSize    ;}

  void RegisterAudioCallback(IAudioCallback* pCallback);
  void UnRegisterAudioCallback();

#ifdef __SSE__
  inline static void SSEMulAddArray(float *data, float *add, const float mul, uint32_t count);
  inline static void SSEMulArray   (float *data, const float mul, uint32_t count);
#endif

private:
  /* these are private as the class is a singleton */
  CAE();
  virtual ~CAE();

  /* these are only callable by the application */
  friend class CApplication;
  bool Initialize();
  bool OpenSink(unsigned int sampleRate = 44100, bool forceRaw = false);
  void Deinitialize();

  unsigned int m_delayFrames;
  void DelayFrames();

  /* this is called by streams on dtor, you should never need to call this directly */
  friend class CAEStream;
  void RemoveStream(CAEStream *stream);

  /* internal vars */
  bool m_running, m_reOpened;
  CCriticalSection m_runLock;         /* released when the thread exits */
  CCriticalSection m_critSection;     /* generic lock */
  CCriticalSection m_critSectionSink; /* sink & configuration lock */

  /* the current configuration */
  float                     m_volume;
  enum AEStdChLayout        m_stdChLayout;
  unsigned int              m_channelCount;
  AEChLayout                m_chLayout;
  unsigned int              m_frameSize;

  /* the sink, its format information, and conversion function */
  IAEPacketizer            *m_packetizer; 
  uint8_t                  *m_packetPos;
  unsigned int              m_packetFrames;
  bool                      m_dropPacket;

  IAESink                  *m_sink;
  AEAudioFormat		    m_format;
  unsigned int              m_bytesPerSample;
  CAEConvert::AEConvertFrFn m_convertFn;

  /* currently playing sounds */
  typedef struct {
    CAESound     *owner;
    float        *samples;
    unsigned int  sampleCount;
  } SoundState;
  std::list<SoundState>     m_playing_sounds;

  /* the streams, sounds, output buffer and output buffer fill size */
  bool                                  m_rawPassthrough;
  bool                                  m_passthrough;
  std::list<CAEStream*>                 m_streams;
  std::map<const CStdString, CAESound*> m_sounds;
  /* this will contain either float, or uint8_t depending on if we are in raw mode or not */
  void                                 *m_buffer;
  unsigned int                          m_bufferSamples;
  float                                 m_vizBuffer[512];
  unsigned int                          m_vizBufferSamples;

  /* the channel remapper and audioCallback */
  CAERemap                  m_remap;
  IAudioCallback           *m_audioCallback;

  /* thread run stages */
  void         MixSounds        (unsigned int samples);
  void         RunOutputStage   ();
  unsigned int RunStreamStage   (unsigned int channelCount, void *out, bool &restart);
  void         RunNormalizeStage(unsigned int channelCount, void *out, unsigned int mixed);
  void         RunBufferStage   (void *out);
};

/* global instance */
static CAE &AE = CAE::GetInstance();

