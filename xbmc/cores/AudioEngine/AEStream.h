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

/**
 * Bit options to pass to IAE::GetStream and IAE::AlterStream
 */
enum AEStreamOptions {
  AESTREAM_FREE_ON_DRAIN  = 0x01, /* auto free the stream when it has drained */
  AESTREAM_OWNS_POST_PROC = 0x02, /* free postproc filters on stream free */
  AESTREAM_FORCE_RESAMPLE = 0x04, /* force resample even if rates match */
  AESTREAM_PAUSED         = 0x08  /* create the stream paused */
};

class IAEPostProc;
class CAEStreamWrapper;

/**
 * IAEStream Stream Interface for streaming audio
 */
class IAEStream
{
protected:
  friend class IAE;
  IAEStream() {}
  virtual ~IAEStream() {}

public:
  /**
   * Callback prototye for Drain and Data callbacks
   * @see SetDataCallback(), SetDrainCallback()
   * @param stream The calling stream
   * @param arg The user supplied pointer
   * @param samples The number of samples (only used for the Data callback, otherwise this value is unspecified)
   */
  typedef void (AECBFunc)(IAEStream*stream, void *arg, unsigned int samples);

  /**
   * Call this to destroy the stream
   * @note Do not use delete
   */
  virtual void Destroy() = 0;

  /**
   * Disable the callbacks and block until they have finished
   * @param free Disable the free callback too
   */
  virtual void DisableCallbacks(bool free = true) = 0;  

  /**
   * Set the callback function to call when more data is required, this is called when there is at-least one full frame of audio free.
   * @param cbFunc The callback function
   * @param arg Pointer to pass to the callback function (eg, this)   
   */
  virtual void SetDataCallback (AECBFunc *cbFunc, void *arg) = 0;

  /**
   * Set the callback function to call when the stream has completed draining
   * @param cbFunc The callback function
   * @param arg Pointer to pass to the callback function (eg, this)
   */
  virtual void SetDrainCallback(AECBFunc *cbFunc, void *arg) = 0;

  /**
   * Set the callback function to call when the stream is about to be freed
   * @param cbFunc The callback function
   * @param arg Pointer to pass to the callback function (eg, this)
   */
  virtual void SetFreeCallback(AECBFunc *cbFunc, void *arg) = 0;

  /**
   * Add interleaved PCM data to the stream
   * @param data The interleaved PCM data
   * @param size The size in bytes of data
   * @return The number of bytes consumed
   */
  virtual unsigned int AddData(void *data, unsigned int size) = 0;

  /**
   * Returns how long until new data will be played
   * @return The delay in seconds
   */
  virtual float GetDelay() = 0;

  /**
   * Returns how long until playback will start
   * @return The delay in seconds
   */
  virtual float GetCacheTime() = 0;

  /**
   * Returns the total length of the cache before playback will start
   * @return The delay in seconds
   */
  virtual float GetCacheTotal() = 0;

  /**
   * Pauses the stream playback
   */
  virtual void Pause() = 0;

  /**
   * Resumes the stream after pausing
   */
  virtual void Resume  () = 0;

  /**
   * Start draining the stream
   * @note Once called AddData will not consume more data.
   */
  virtual void Drain() = 0;

  /**
   * Returns true if the is stream draining
   */
  virtual bool IsDraining() = 0;

  /**
   * Flush all buffers dropping the audio data
   */
  virtual void Flush() = 0;

  /**
   * Return the stream's current volume level
   * @return The volume level between 0.0 and 1.0
   */
  virtual float GetVolume() = 0;

  /**
   * Set the stream's volume level
   * @param volume The new volume level between 0.0 and 1.0
   */
  virtual void  SetVolume(float volume) = 0;

  /**
   * Returns the stream's current replay gain factor
   * @return The replay gain factor between 0.0 and 1.0
   */
  virtual float GetReplayGain() = 0;

  /**
   * Sets the stream's replay gain factor, this is used by formats such as MP3 that have attenuation information in their streams
   * @param factor The replay gain factor
   */
  virtual void  SetReplayGain(float factor) = 0;

  /**
   * Appends a post-processor filter to the stream
   * @param pp The post-processor to append
   */
  virtual void AppendPostProc (IAEPostProc *pp) = 0;

  /**
   * Prepends a post-processor filter to the stream
   * @param pp The post-processor to prepend
   */
  virtual void PrependPostProc(IAEPostProc *pp) = 0;

  /**
   * Removes a post-processor filter from the stream
   * @param pp The post-processor to remove
   */
  virtual void RemovePostProc (IAEPostProc *pp) = 0;

  /**
   * Returns the size of one audio frame in bytes (channelCount * resolution)
   * @return The size in bytes of one frame
  */
  virtual unsigned int GetFrameSize() = 0;

  /**
   * Returns the number of channels the stream is configured to accept
   * @return The channel count
   */
  virtual unsigned int GetChannelCount() = 0;

  /**
   * Returns the stream's sample rate, if the stream is using a dynamic sample rate, this value will NOT reflect any changes made by calls to SetResampleRatio()
   * @return The stream's sample rate (eg, 48000)
   */
  virtual unsigned int GetSampleRate() = 0;

  /**
   * Return the data format the stream has been configured with
   * @return The stream's data format (eg, AE_FMT_S16LE)
   */
  virtual enum AEDataFormat GetDataFormat() = 0;

  /**
   * Return the resample ratio
   * @note This will return an undefined value if the stream is not resampling
   * @return the current resample ratio or undefined if the stream is not resampling
   */
  virtual double GetResampleRatio() = 0;

  /**
   * Sets the resample ratio
   * @note This function will silently fail if the stream is not resampling, if you wish to use this be sure to set the AESTREAM_FORCE_RESAMPLE option
   * @param ratio the new sample rate ratio, calculated by ((double)desiredRate / (double)GetSampleRate())
   */
  virtual void   SetResampleRatio(double ratio) = 0;

  /**
   * Registers the audio callback to call with each block of data, this is used by Audio Visualizations
   * @warning Currently the callbacks require stereo float data in blocks of 512 samples, any deviation from this may crash XBMC, or cause junk to be rendered
   * @param pCallback The callback
   */
  virtual void RegisterAudioCallback(IAudioCallback* pCallback) = 0;

  /**
   * Unregisters the current audio callback
   */
  virtual void UnRegisterAudioCallback() = 0;
};

