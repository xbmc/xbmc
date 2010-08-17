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

#ifndef AE_H
#define AE_H

#include <list>
#include <map>

#include "utils/Thread.h"
#include "utils/CriticalSection.h"

#include "AEAudioFormat.h"
#include "AEStream.h"
#include "AESound.h"
#include "AEConvert.h"
#include "AERemap.h"

#include "cores/IAudioCallback.h"
#include "addons/Visualisation.h"
#include "cores/AudioRenderers/IAudioRenderer.h"

enum AEState
{
  AE_STATE_INVALID, /* the AE has not been initialized */
  AE_STATE_READY,   /* the AE is initialized and ready to run */
  AE_STATE_RUN,     /* the AE is running */
  AE_STATE_STOP,    /* the AE is stopping, next state will be READY */
  AE_STATE_SHUTDOWN /* the AE is shutting down, next state will be INVALID */
};

class CAEStream;
class CAESound;
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

  enum AEState GetState();
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

  void RegisterAudioCallback(IAudioCallback* pCallback);
  void UnRegisterAudioCallback();
private:
  /* these are private as the class is a singleton */
  CAE();
  virtual ~CAE();
  bool Initialize();
  bool OpenSink();
  void DeInitialize();

  /* this is called by streams on dtor, you should never need to call this directly */
  friend class CAEStream;
  void RemoveStream(CAEStream *stream);

  enum AEState              m_state;
  float                     m_volume;

  /* the desired channelCount, layout and frameSize */
  unsigned int              m_channelCount;
  AEChLayout                m_chLayout;
  unsigned int              m_frameSize;

  /* the sink, its format information, and conversion function */
  IAudioRenderer           *m_sink;
  AEAudioFormat		    m_format;
  CAEConvert::AEConvertFrFn m_convertFn;

  /* currently playing sounds */
  typedef struct {
    CAESound     *owner;
    unsigned int  frame;
  } SoundState;
  std::list<SoundState>     m_playing_sounds;

  /* the streams, sounds, output buffer and output buffer fill size */
  bool                                  m_passthrough;
  std::list<CAEStream*>                 m_streams;
  std::map<const CStdString, CAESound*> m_sounds;
  uint8_t                              *m_buffer;
  unsigned int                          m_bufferSize;
  float                                 m_visBuffer[AUDIO_BUFFER_SIZE * 2];
  unsigned int                          m_visBufferSize;

  /* the channel remapper and audioCallback */
  CAERemap                  m_remap;
  IAudioCallback           *m_audioCallback;

  /* lock for threadsafe */
  CCriticalSection          m_critSection, m_critSectionSink, m_critSectionAC;

#ifdef __SSE__
  static inline void SSENormalizeSamples(float *samples, uint32_t count, const uint32_t div);
  static inline void SSEMixSamples      (float *dest, float *src, uint32_t count, const float volume);
  static inline void SSEDeAmpSamples    (float *samples, uint32_t count, const float volume);
#endif
};

/* global instance */
static CAE &AE = CAE::GetInstance();

#endif
