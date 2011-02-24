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

#include "AEStream.h"
#include "AEAudioFormat.h"
#include "AEConvert.h"
#include "AERemap.h"
#include "AEPostProc.h"

class IAEPostProc;
class CSoftAEStream : public IAEStream
{
protected:
  friend class CSoftAE;
  CSoftAEStream(enum AEDataFormat format, unsigned int sampleRate, unsigned int channelCount, AEChLayout channelLayout, unsigned int options);
  virtual ~CSoftAEStream();

public:
  void Initialize();
  void InitializeRemap();
  virtual void Destroy();
  virtual void DisableCallbacks(); /* disable all callbacks */
  virtual void SetDataCallback (AECBFunc *cbFunc, void *arg); /* called when the buffer < 50% full */
  virtual void SetDrainCallback(AECBFunc *cbFunc, void *arg); /* called when the buffer has been drained */
  virtual void SetFreeCallback (AECBFunc *cbFunc, void *arg); /* called when the stream is deleted */

  virtual unsigned int GetFrameSize() {return m_format.m_frameSize;}
  virtual unsigned int AddData(void *data, unsigned int size);
  uint8_t* GetFrame();
  virtual float GetDelay();
  virtual float GetCacheTime();
  virtual float GetCacheTotal();

  bool IsPaused     () { return m_paused;      }
  virtual bool IsDraining   () { return m_draining;    }
  bool IsFreeOnDrain() { return m_freeOnDrain; }
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

  virtual void AppendPostProc (IAEPostProc *pp);
  virtual void PrependPostProc(IAEPostProc *pp);
  virtual void RemovePostProc (IAEPostProc *pp);

  unsigned int      GetFrameSamples() { return m_format.m_frameSamples;        }
  virtual unsigned int      GetChannelCount() { return m_initChannelCount;             }
  virtual unsigned int      GetSampleRate()   { return m_initSampleRate;               }
  virtual enum AEDataFormat GetDataFormat()   { return m_initDataFormat;               }
  virtual bool              IsRaw()           { return m_initDataFormat == AE_FMT_RAW; }

  /* for dynamic sample rate changes (smoothvideo) */
  virtual double GetResampleRatio();
  virtual void   SetResampleRatio(double ratio);

  virtual void RegisterAudioCallback(IAudioCallback* pCallback);
  virtual void UnRegisterAudioCallback();

  void SetFreeOnDrain() { m_freeOnDrain = true; }

  /* returns true if the stream is in a callback function */
  bool IsBusy();

private:
  void InternalFlush();

  CCriticalSection  m_critSection;
  enum AEDataFormat m_initDataFormat;
  unsigned int      m_initSampleRate;
  unsigned int      m_initChannelCount;
  AEChLayout        m_initChannelLayout;
  
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
  bool                    m_freeOnDrain;   /* true to free the stream when it has drained */
  std::list<IAEPostProc*> m_postProc;      /* post processing objects */
  bool                    m_ownsPostProc;  /* true if the stream should free post-proc filters */
  unsigned int            m_waterLevel;    /* the fill level to fall below before calling the data callback */
  unsigned int            m_refillBuffer;  /* how many frames that need to be buffered before we return any frames */

  CAEConvert::AEConvertToFn m_convertFn;

  uint8_t           *m_frameBuffer;
  unsigned int       m_frameBufferSize;
  unsigned int       m_bytesPerSample;
  unsigned int       m_bytesPerFrame;
  enum AEChannel    *m_aeChannelLayout;
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

  /* callback hook for more data */
  bool          m_disableCallbacks;
  AECBFunc     *m_cbDataFunc, *m_cbDrainFunc, *m_cbFreeFunc;
  void         *m_cbDataArg , *m_cbDrainArg , *m_cbFreeArg;
  bool          m_inDataFunc,  m_inDrainFunc,  m_inFreeFunc;

  /* vizualization internals */
  CAERemap           m_vizRemap;
  float              m_vizBuffer[512];
  unsigned int       m_vizBufferSamples;
  IAudioCallback    *m_audioCallback;
};

