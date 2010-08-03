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

#include "utils/Thread.h"
#include "utils/CriticalSection.h"

#include "AEAudioFormat.h"
#include "AEStream.h"
#include "AESound.h"
#include "AEConvert.h"
#include "AERemap.h"
#include "AudioRenderers/IAudioRenderer.h"

using namespace std;

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

  /* returns a new stream for data in the specified format */
  CAEStream *GetStream(enum AEDataFormat dataFormat, unsigned int sampleRate, unsigned int channelCount, AEChLayout channelLayout);
  void PlaySound(CAESound *sound);
  void StopSound(CAESound *sound);

  /* these are for the streams so they can provide compatible data */
  unsigned int        GetSampleRate   () {return m_format.m_sampleRate   ;}
  unsigned int        GetChannelCount () {return m_channelCount          ;}
  AEChLayout          GetChannelLayout() {return m_chLayout              ;}
  unsigned int        GetFrames       () {return m_format.m_frames       ;}
  unsigned int        GetFrameSize    () {return m_frameSize             ;}
  
  /* this is called by streams on dtor, you should never need to call this directly */
  void RemoveStream(CAEStream *stream);
private:
  /* these are private as the class is a singleton */
  CAE();
  virtual ~CAE();
  bool Initialize();
  void DeInitialize();

  enum AEState              m_state;

  /* the desired channelCount, layout and frameSize */
  unsigned int              m_channelCount;
  AEChLayout                m_chLayout;
  unsigned int              m_frameSize;

  /* the renderer, its format information, and conversion function */
  IAudioRenderer           *m_renderer;
  AEAudioFormat		    m_format;
  CAEConvert::AEConvertFrFn m_convertFn;

  /* currently playing sounds */
  typedef struct {
    CAESound     *owner;
    unsigned int  frame;
  } SoundState;
  list<SoundState>          m_sounds;

  /* the streams, output buffer and output buffer fill size */
  list<CAEStream*>          m_streams;
  uint8_t                  *m_buffer;
  unsigned int              m_bufferSize;

  /* the channel remapper */
  CAERemap                  m_remap;

  /* lock for threadsafe */
  CCriticalSection          m_critSection;
};

/* global instance */
static CAE &AE = CAE::GetInstance();

#endif
