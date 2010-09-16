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
#include "cores/IAudioCallback.h"
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
protected:
  friend class IAE;
  virtual ~IAEStream() {}

public:
  typedef void (AECBFunc)(IAEStream*stream, void *arg, unsigned int samples);

  /* use this to destroy & free the stream */
  virtual void Destroy() = 0;

  /* set callbacks for when more data is needed, or when drain has completed */
  virtual void SetDataCallback (AECBFunc *cbFunc, void *arg) = 0; /* called when the buffer < 50% full */
  virtual void SetDrainCallback(AECBFunc *cbFunc, void *arg) = 0; /* called when the buffer has been drained */

  /* add data to the stream */
  virtual unsigned int AddData(void *data, unsigned int size) = 0;

  /* get the delay till playback of this stream */
  virtual float GetDelay() = 0;

  /* pause the stream */
  virtual void Pause() = 0;

  /* resume the stream */
  virtual void Resume  () = 0;

  /* drain the stream */
  virtual void Drain() = 0;

  /* returns true if the is stream draining */
  virtual bool IsDraining() = 0;

  /* flush the stream */
  virtual void Flush() = 0;

  /* get the streams playback volume */
  virtual float GetVolume() = 0;

  /* set the streams playback volume */
  virtual void  SetVolume    (float volume) = 0;

  /* get the replay gain of the stream */
  virtual float GetReplayGain() = 0;

  /* set the replay gain of the stream */
  virtual void  SetReplayGain(float factor) = 0;

  /* append/prepend/remove a post proc filter to/from the stream */
  virtual void AppendPostProc (IAEPostProc *pp) = 0;
  virtual void PrependPostProc(IAEPostProc *pp) = 0;
  virtual void RemovePostProc (IAEPostProc *pp) = 0;

  /* returns the size in bytes of one frame */
  virtual unsigned int GetFrameSize() = 0;

  /* returns the number of channels */
  virtual unsigned int GetChannelCount() = 0;

  /* returns the stream's sample rate */
  /* note, this is not updated by Get/Set Resample ratio */
  virtual unsigned int GetSampleRate() = 0;

  /* returns the data format the stream expects */
  virtual enum AEDataFormat GetDataFormat() = 0;

  /* for dynamic sample rate changes (smoothvideo) */
  virtual double GetResampleRatio() = 0;
  virtual void   SetResampleRatio(double ratio) = 0;

  /* vizualization callback register/unregister function */
  virtual void RegisterAudioCallback(IAudioCallback* pCallback) = 0;
  virtual void UnRegisterAudioCallback() = 0;
};

