#pragma once
/*
 *      Copyright (C) 2016 Team KODI
 *      http://kodi.tv
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
 *  along with KODI; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <stdint.h>

namespace V2
{
namespace KodiAPI
{

struct CB_AddOnLib;

namespace AudioEngine
{
extern "C"
{

  class CAddOnAEStream
  {
  public:
    static void Init(struct CB_AddOnLib *interfaces);

    /**
    * Returns the amount of space available in the stream
    * @return The number of bytes AddData will consume
    */
    static unsigned int AEStream_GetSpace(void *AddonData, void *StreamHandle);

    /**
    * Add planar or interleaved PCM data to the stream
    * @param data array of pointers to the planes
    * @param offset to frame in frames
    * @param frames number of frames
    * @param pts timestamp
    * @return The number of frames consumed
    */
    static unsigned int AEStream_AddData(void *AddonData, void *StreamHandle, uint8_t* const *Data, unsigned int Offset, unsigned int Frames);

    /**
    * Returns the time in seconds that it will take
    * for the next added packet to be heard from the speakers.
    * @return seconds
    */
    static double AEStream_GetDelay(void *AddonData, void *StreamHandle);

    /**
    * Returns if the stream is buffering
    * @return True if the stream is buffering
    */
    static bool AEStream_IsBuffering(void *AddonData, void *StreamHandle);

    /**
    * Returns the time in seconds that it will take
    * to underrun the cache if no sample is added.
    * @return seconds
    */
    static double AEStream_GetCacheTime(void *AddonData, void *StreamHandle);

    /**
    * Returns the total time in seconds of the cache
    * @return seconds
    */
    static double AEStream_GetCacheTotal(void *AddonData, void *StreamHandle);

    /**
    * Pauses the stream playback
    */
    static void AEStream_Pause(void *AddonData, void *StreamHandle);

    /**
    * Resumes the stream after pausing
    */
    static void AEStream_Resume(void *AddonData, void *StreamHandle);

    /**
    * Start draining the stream
    * @note Once called AddData will not consume more data.
    */
    static void AEStream_Drain(void *AddonData, void *StreamHandle, bool Wait);

    /**
    * Returns true if the is stream draining
    */
    static bool AEStream_IsDraining(void *AddonData, void *StreamHandle);

    /**
    * Returns true if the is stream has finished draining
    */
    static bool AEStream_IsDrained(void *AddonData, void *StreamHandle);

    /**
    * Flush all buffers dropping the audio data
    */
    static void AEStream_Flush(void *AddonData, void *StreamHandle);

    /**
    * Return the stream's current volume level
    * @return The volume level between 0.0 and 1.0
    */
    static float AEStream_GetVolume(void *AddonData, void *StreamHandle);

    /**
    * Set the stream's volume level
    * @param volume The new volume level between 0.0 and 1.0
    */
    static void  AEStream_SetVolume(void *AddonData, void *StreamHandle, float Volume);

    /**
    * Gets the stream's volume amplification in linear units.
    * @return The volume amplification factor between 1.0 and 1000.0
    */
    static float AEStream_GetAmplification(void *AddonData, void *StreamHandle);

    /**
    * Sets the stream's volume amplification in linear units.
    * @param The volume amplification factor between 1.0 and 1000.0
    */
    static void AEStream_SetAmplification(void *AddonData, void *StreamHandle, float Amplify);

    /**
    * Returns the size of one audio frame in bytes (channelCount * resolution)
    * @return The size in bytes of one frame
    */
    static const unsigned int AEStream_GetFrameSize(void *AddonData, void *StreamHandle);

    /**
    * Returns the number of channels the stream is configured to accept
    * @return The channel count
    */
    static const unsigned int AEStream_GetChannelCount(void *AddonData, void *StreamHandle);

    /**
    * Returns the stream's sample rate, if the stream is using a dynamic sample rate, this value will NOT reflect any changes made by calls to SetResampleRatio()
    * @return The stream's sample rate (eg, 48000)
    */
    static const unsigned int AEStream_GetSampleRate(void *AddonData, void *StreamHandle);

    /**
    * Return the data format the stream has been configured with
    * @return The stream's data format (eg, AE_FMT_S16LE)
    */
    static const int AEStream_GetDataFormat(void *AddonData, void *StreamHandle);

    /**
    * Return the resample ratio
    * @note This will return an undefined value if the stream is not resampling
    * @return the current resample ratio or undefined if the stream is not resampling
    */
    static double AEStream_GetResampleRatio(void *AddonData, void *StreamHandle);

    /**
    * Sets the resample ratio
    * @note This function may return false if the stream is not resampling, if you wish to use this be sure to set the AESTREAM_FORCE_RESAMPLE option
    * @param ratio the new sample rate ratio, calculated by ((double)desiredRate / (double)GetSampleRate())
    */
    static void AEStream_SetResampleRatio(void *AddonData, void *StreamHandle, double Ratio);
  };

} /* extern "C" */
} /* namespace AudioEngine */

} /* namespace KodiAPI */
} /* namespace V2 */
