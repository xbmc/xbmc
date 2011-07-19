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

#ifndef __COREAUDIOAESTREAM_H__
#define __COREAUDIOAESTREAM_H__

#include <samplerate.h>
#include <list>

#include "AEStream.h"
#include "AEAudioFormat.h"
#include "AEConvert.h"
#include "AERemap.h"
#include "CoreAudioRingBuffer.h"
#include "threads/CriticalSection.h"

class CCoreAudioAEStream : public IAEStream
{
protected:
  friend class CCoreAudioAE;
  CCoreAudioAEStream(enum AEDataFormat format, unsigned int sampleRate, unsigned int channelCount, AEChLayout channelLayout, unsigned int options);
  virtual ~CCoreAudioAEStream();

public:
  void Initialize(AEAudioFormat &outputFormat);
  void InitializeRemap();
  virtual void Destroy();

  virtual unsigned int GetFrameSize();
  virtual unsigned int GetSpace();
  virtual unsigned int AddData(void *data, unsigned int size);
  unsigned int GetFrames(uint8_t *buffer, unsigned int size);
  virtual float GetDelay();
  virtual float GetCacheTime();
  virtual float GetCacheTotal();

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

  virtual unsigned int      GetChannelCount();
  virtual unsigned int      GetSampleRate();
  virtual enum AEDataFormat GetDataFormat();
  virtual bool              IsRaw();

  /* for dynamic sample rate changes (smoothvideo) */
  virtual double GetResampleRatio();
  virtual void   SetResampleRatio(double ratio);

  virtual void RegisterAudioCallback(IAudioCallback* pCallback);
  virtual void UnRegisterAudioCallback();

  virtual void FadeVolume(float from, float to, unsigned int time);
  virtual bool IsFading();
  
private:
  void InternalFlush();

  CCriticalSection        m_MutexStream;

  AEAudioFormat           m_OutputFormat;
  AEAudioFormat           m_StreamFormat;
  unsigned int            m_StreamBytesPerSample;
  unsigned int            m_OutputBytesPerSample;

  bool                    m_forceResample; /* true if we are to force resample even when the rates match */
  bool                    m_resample;      /* true if the audio needs to be resampled  */
  bool                    m_convert;       /* true if the bitspersample needs converting */
  bool                    m_valid;         /* true if the stream is valid */
  bool                    m_delete;        /* true if CCoreAudioAE is to free this object */
  CAERemap                m_remap;         /* the remapper */
  float                   m_volume;        /* the volume level */
  float                   m_rgain;         /* replay gain level */

  CAEConvert::AEConvertToFn m_convertFn;

  CoreAudioRingBuffer    *m_Buffer;
  float                  *m_convertBuffer;      /* buffer for converted data */
  int                     m_convertBufferSize;
  float                  *m_resampleBuffer;     /* buffer for resample data */
  int                     m_resampleBufferSize;
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
  bool               m_fadeRunning;
  bool               m_fadeDirUp;
  float              m_fadeStep;
  float              m_fadeTarget;
  unsigned int       m_fadeTime;
};

#endif
