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

#include "AEAudioFormat.h"
#include <stdint.h>

/* options to pass to GetStream */
enum {
  AESTREAM_FREE_ON_DRAIN  = 0x01, /* auto free the stream when it has drained */
  AESTREAM_OWNS_POST_PROC = 0x02, /* free postproc filters on stream free */
  AESTREAM_FORCE_RESAMPLE = 0x04  /* force resample even if rates match */
};

class IAEPostProc;
class IAEStream
{
public:
  typedef void (AECBFunc)(IAEStream*stream, void *arg, unsigned int samples);

  IAEStream(enum AEDataFormat format, unsigned int sampleRate, unsigned int channelCount, AEChLayout channelLayout, unsigned int options) {}
  virtual ~IAEStream() {}

  virtual void Initialize() = 0;
  virtual void Destroy() = 0;
  virtual void SetDataCallback (AECBFunc *cbFunc, void *arg) = 0; /* called when the buffer < 50% full */
  virtual void SetDrainCallback(AECBFunc *cbFunc, void *arg) = 0; /* called when the buffer has been drained */

  virtual unsigned int GetFrameSize() = 0;
  virtual unsigned int AddData(void *data, unsigned int size) = 0;
  virtual uint8_t* GetFrame() = 0;
  virtual float GetDelay() = 0;

  virtual bool IsPaused     () = 0;
  virtual bool IsDraining   () = 0;
  virtual bool IsFreeOnDrain() = 0;
  virtual bool IsDestroyed  () = 0;

  virtual void Pause   () = 0;
  virtual void Resume  () = 0;
  virtual void Drain   () = 0;
  virtual void Flush   () = 0;

  virtual float GetVolume    () = 0;
  virtual float GetReplayGain() = 0;
  virtual void  SetVolume    (float volume) = 0;
  virtual void  SetReplayGain(float factor) = 0;

  virtual void AppendPostProc (IAEPostProc *pp) = 0;
  virtual void PrependPostProc(IAEPostProc *pp) = 0;
  virtual void RemovePostProc (IAEPostProc *pp) = 0;

  virtual unsigned int      GetFrameSamples() = 0;
  virtual unsigned int      GetChannelCount() = 0;
  virtual unsigned int      GetSampleRate  () = 0;
  virtual enum AEDataFormat GetDataFormat  () = 0;
  virtual bool              IsRaw          () = 0;

  /* for dynamic sample rate changes (smoothvideo) */
  virtual double GetResampleRatio() = 0;
  virtual void   SetResampleRatio(double ratio) = 0;
};

