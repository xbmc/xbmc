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

#include <samplerate.h>
#include <list>

#include "threads/CriticalSection.h"

#include "AEAudioFormat.h"
#include "Interfaces/AEStream.h"
#include "Utils/AEConvert.h"
#include "Utils/AERemap.h"

class IAEPostProc;
class CSoftAEStream : public IAEStream
{
protected:
  friend class CSoftAE;
  CSoftAEStream(enum AEDataFormat format, unsigned int sampleRate, CAEChannelInfo channelLayout, unsigned int options);
  virtual ~CSoftAEStream();

public:
  void Initialize();
  void InitializeRemap();
  virtual void Destroy();

  virtual unsigned int GetFrameSize() {return m_format.m_frameSize;}
  virtual unsigned int GetSpace();
  virtual unsigned int AddData(void *data, unsigned int size);
  uint8_t* GetFrame();
  virtual float GetDelay();
  virtual float GetCacheTime();
  virtual float GetCacheTotal();

  bool IsPaused     () { return m_paused;      }
  virtual bool IsDraining   () { return m_draining;    }
  virtual bool IsDrained    ();
  bool IsDestroyed  () { return m_delete;      }
  bool IsValid() { return m_valid; }

  virtual void Pause   () {m_paused = true; }
  virtual void Resume  () {m_paused = false;}
  virtual void Drain   ();
  virtual void Flush   ();

  virtual float GetVolume    ()             { return m_volume;   }
  virtual float GetReplayGain()             { return m_rgain ;   }
  virtual void  SetVolume    (float volume) { m_volume = std::max( 0.0f, std::min(1.0f, volume)); }
  virtual void  SetReplayGain(float factor) { m_rgain  = std::max(-1.0f, std::max(1.0f, factor)); }

  unsigned int              GetFrameSamples() { return m_format.m_frameSamples;     }
  virtual unsigned int      GetChannelCount() { return m_initChannelLayout.Count(); }
  virtual unsigned int      GetSampleRate()   { return m_initSampleRate;            }
  virtual enum AEDataFormat GetDataFormat()   { return m_initDataFormat;            }
  virtual bool              IsRaw()           { return AE_IS_RAW(m_initDataFormat); }

  /* for dynamic sample rate changes (smoothvideo) */
  virtual double GetResampleRatio();
  virtual void   SetResampleRatio(double ratio);

  virtual void RegisterAudioCallback(IAudioCallback* pCallback);
  virtual void UnRegisterAudioCallback();

  virtual void FadeVolume(float from, float to, unsigned int time);
  virtual bool IsFading();
  
private:
  void InternalFlush();
  void CheckResampleBuffers();

  CCriticalSection  m_critSection;
  enum AEDataFormat m_initDataFormat;
  unsigned int      m_initSampleRate;
  CAEChannelInfo    m_initChannelLayout;
  
  typedef struct
  {
    unsigned int  samples;
    uint8_t      *data;
    float        *vizData;
  } PPacket;

  AEAudioFormat m_format;

  bool                    m_forceResample; /* true if we are to force resample even when the rates match */
  bool                    m_resample;      /* true if the audio needs to be resampled  */
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

  uint8_t           *m_frameBuffer;
  unsigned int       m_frameBufferSize;
  unsigned int       m_bytesPerSample;
  unsigned int       m_bytesPerFrame;
  CAEChannelInfo     m_aeChannelLayout;
  unsigned int       m_aeChannelCount;
  unsigned int       m_aePacketSamples;
  SRC_STATE         *m_ssrc;
  SRC_DATA           m_ssrcData;
  unsigned int       m_framesBuffered;
  std::list<PPacket> m_outBuffer;
  unsigned int       ProcessFrameBuffer();
  PPacket            m_newPacket;
  PPacket            m_packet;
  uint8_t           *m_packetPos;
  float             *m_vizPacketPos;
  bool               m_paused;
  bool               m_draining;

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
};

