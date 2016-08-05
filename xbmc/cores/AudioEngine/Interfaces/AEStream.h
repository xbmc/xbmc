#pragma once
/*
 *      Copyright (C) 2010-2013 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "cores/AudioEngine/Utils/AEAudioFormat.h"
#include "cores/AudioEngine/Utils/AEStreamData.h"
#include "cores/AudioEngine/Interfaces/IAudioCallback.h"
#include <stdint.h>

extern "C" {
#include "libavcodec/avcodec.h"
}

/**
 * Callback interafce for VideoPlayer clock needed by AE for sync
 */
class IAEClockCallback
{
public:
  virtual double GetClock() = 0;
  virtual double GetClockSpeed() { return 1.0; };
};

class CAESyncInfo
{
public:
  double delay;
  double error;
  double rr;
  unsigned int errortime;
  enum AESyncState
  {
    SYNC_OFF,
    SYNC_INSYNC,
    SYNC_START,
    SYNC_MUTE,
    SYNC_ADJUST
  };
  AESyncState state;
};

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
   * Returns the amount of space available in the stream
   * @return The number of bytes AddData will consume
   */
  virtual unsigned int GetSpace() = 0;

  /**
   * Add planar or interleaved PCM data to the stream
   * @param data array of pointers to the planes
   * @param offset to frame in frames
   * @param frames number of frames
   * @param pts timestamp
   * @return The number of frames consumed
   */
  virtual unsigned int AddData(const uint8_t* const *data, unsigned int offset, unsigned int frames, double pts = 0.0) = 0;

  /**
   * Returns the time in seconds that it will take
   * for the next added packet to be heard from the speakers.
   * @return seconds
   */
  virtual double GetDelay() = 0;

  /**
   * Returns info about audio to clock synchronization
   * @return CAESyncInfo
   */
  virtual CAESyncInfo GetSyncInfo() = 0;

  /**
   * Returns if the stream is buffering
   * @return True if the stream is buffering
   */
  virtual bool IsBuffering() = 0;

  /**
   * Returns the time in seconds of the stream's
   * cached audio samples. Engine buffers excluded.
   * @return seconds
   */
  virtual double GetCacheTime() = 0;

  /**
   * Returns the total time in seconds of the cache
   * @return seconds
   */
  virtual double GetCacheTotal() = 0;

  /**
   * Pauses the stream playback
   */
  virtual void Pause() = 0;

  /**
   * Resumes the stream after pausing
   */
  virtual void Resume() = 0;

  /**
   * Start draining the stream
   * @note Once called AddData will not consume more data.
   */
  virtual void Drain(bool wait) = 0;

  /**
   * Returns true if the is stream draining
   */
  virtual bool IsDraining() = 0;

  /**
   * Returns true if the is stream has finished draining
   */
  virtual bool IsDrained() = 0;
  
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
  virtual void SetReplayGain(float factor) = 0;

  /**
   * Gets the stream's volume amplification in linear units.
   * @return The volume amplification factor between 1.0 and 1000.0
   */
  virtual float GetAmplification() = 0;

  /**
   * Sets the stream's volume amplification in linear units.
   * @param The volume amplification factor between 1.0 and 1000.0
   */
  virtual void SetAmplification(float amplify) = 0;

  /**
   * Sets the stream ffmpeg informations if present.
   + @param profile
   * @param matrix_encoding
   * @param audio_service_type
   */
  virtual void SetFFmpegInfo(int profile, enum AVMatrixEncoding matrix_encoding, enum AVAudioServiceType audio_service_type) = 0;

  /**
   * Returns the size of one audio frame in bytes (channelCount * resolution)
   * @return The size in bytes of one frame
  */
  virtual const unsigned int GetFrameSize() const = 0;

  /**
   * Returns the number of channels the stream is configured to accept
   * @return The channel count
   */
  virtual const unsigned int GetChannelCount() const = 0;

  /**
   * Returns the stream's sample rate, if the stream is using a dynamic sample rate, this value will NOT reflect any changes made by calls to SetResampleRatio()
   * @return The stream's sample rate (eg, 48000)
   */
  virtual const unsigned int GetSampleRate() const = 0;

  /**
   * Return the data format the stream has been configured with
   * @return The stream's data format (eg, AE_FMT_S16LE)
   */
  virtual const enum AEDataFormat GetDataFormat() const = 0;

  /**
   * Return the resample ratio
   * @note This will return an undefined value if the stream is not resampling
   * @return the current resample ratio or undefined if the stream is not resampling
   */
  virtual double GetResampleRatio() = 0;

  /**
   * Sets the resample ratio
   * @note This function may return false if the stream is not resampling, if you wish to use this be sure to set the AESTREAM_FORCE_RESAMPLE option
   * @param ratio the new sample rate ratio, calculated by ((double)desiredRate / (double)GetSampleRate())
   */
  virtual void SetResampleRatio(double ratio) = 0;

  /**
   * Sets the resamplling on/ff
   */
  virtual void SetResampleMode(int mode) = 0;

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

  /**
    * Fade the volume level over the specified time
    * @param from The volume level to fade from (0.0f-1.0f) - See notes
    * @param target The volume level to fade to (0.0f-1.0f)
    * @param time The amount of time in milliseconds for the fade to occur
    * @note The from parameter does not set the streams volume, it is only used to calculate the fade time properly 
    */
  virtual void FadeVolume(float from, float target, unsigned int time) {} /* FIXME: once all the engines have these new methods */

  /**
   * Returns if a fade is still running
   * @return true if a fade is in progress, otherwise false
   */
  virtual bool IsFading() { return false; }

  /**
   * Slave a stream to resume when this stream has drained
   */
  virtual void RegisterSlave(IAEStream *stream) = 0;

  /**
   * Indicates if dsp addon system is active.
   */
  virtual bool HasDSP() = 0;
};

