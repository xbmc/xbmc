/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "addons/kodi-dev-kit/include/kodi/AudioEngine.h"
#include "cores/AudioEngine/Utils/AEChannelData.h"

extern "C"
{
namespace ADDON
{

struct Interface_AudioEngine
{
  static void Init(AddonGlobalInterface* addonInterface);
  static void DeInit(AddonGlobalInterface* addonInterface);

  /**
   * @brief Translation functions to separate Kodi and addons
   *
   * This thought to make it more safe for cases as something changed inside
   * Kodi, addons overseen and breaks API, further to have on addons a better
   * documentation about this parts.
   */
  //@{
  static AEChannel TranslateAEChannelToKodi(AudioEngineChannel channel);
  static AudioEngineChannel TranslateAEChannelToAddon(AEChannel channel);
  static AEDataFormat TranslateAEFormatToKodi(AudioEngineDataFormat format);
  static AudioEngineDataFormat TranslateAEFormatToAddon(AEDataFormat format);
  //@}

  /**
   * Creates and returns a new handle to an IAEStream in the format specified, this function should never fail
   * @param[in] streamFormat Format to use for stream
   * @param[in] options A bit field of stream options (see: enum AEStreamOptions)
   * @return a new Handle to an IAEStream that will accept data in the requested format
   */
  static AEStreamHandle* audioengine_make_stream(void* kodiBase,
                                                 AUDIO_ENGINE_FORMAT* streamFormat,
                                                 unsigned int options);

  /**
   * This method will remove the specified stream from the engine.
   * For OSX/IOS this is essential to reconfigure the audio output.
   * @param[in] streamHandle The stream to be altered
   */
  static void audioengine_free_stream(void* kodiBase, AEStreamHandle* streamHandle);

  /**
   * Get the current sink data format
   *
   * @param[in] sinkFormat sink data format. For more details see AUDIO_ENGINE_FORMAT.
   * @return Returns true on success, else false.
   */
  static bool get_current_sink_format(void* kodiBase, AUDIO_ENGINE_FORMAT* sinkFormat);

  /**
   * Returns the amount of space available in the stream
   * @return The number of bytes AddData will consume
   */
  static unsigned int aestream_get_space(void* kodiBase, AEStreamHandle* streamHandle);

  /**
   * Add planar or interleaved PCM data to the stream
   * @param[in] data array of pointers to the planes
   * @param[in] offset to frame in frames
   * @param[in] frames number of frames
   * @return The number of frames consumed
   */
  static unsigned int aestream_add_data(void* kodiBase,
                                        AEStreamHandle* streamHandle,
                                        uint8_t* const* data,
                                        unsigned int offset,
                                        unsigned int frames,
                                        double pts,
                                        bool hasDownmix,
                                        double centerMixLevel);

  /**
   * Returns the time in seconds that it will take
   * for the next added packet to be heard from the speakers.
   * @return seconds
   */
  static double aestream_get_delay(void* kodiBase, AEStreamHandle* streamHandle);

  /**
   * Returns if the stream is buffering
   * @return True if the stream is buffering
   */
  static bool aestream_is_buffering(void* kodiBase, AEStreamHandle* streamHandle);

  /**
   * Returns the time in seconds that it will take
   * to underrun the cache if no sample is added.
   * @return seconds
   */
  static double aestream_get_cache_time(void* kodiBase, AEStreamHandle* streamHandle);

  /**
   * Returns the total time in seconds of the cache
   * @return seconds
   */
  static double aestream_get_cache_total(void* kodiBase, AEStreamHandle* streamHandle);

  /**
   * Pauses the stream playback
   */
  static void aestream_pause(void* kodiBase, AEStreamHandle* streamHandle);

  /**
   * Resumes the stream after pausing
   */
  static void aestream_resume(void* kodiBase, AEStreamHandle* streamHandle);

  /**
   * Start draining the stream
   * @note Once called AddData will not consume more data.
   */
  static void aestream_drain(void* kodiBase, AEStreamHandle* streamHandle, bool wait);

  /**
   * Returns true if the is stream draining
   */
  static bool aestream_is_draining(void* kodiBase, AEStreamHandle* streamHandle);

  /**
   * Returns true if the is stream has finished draining
   */
  static bool aestream_is_drained(void* kodiBase, AEStreamHandle* streamHandle);

  /**
   * Flush all buffers dropping the audio data
   */
  static void aestream_flush(void* kodiBase, AEStreamHandle* streamHandle);

  /**
   * Return the stream's current volume level
   * @return The volume level between 0.0 and 1.0
   */
  static float aestream_get_volume(void* kodiBase, AEStreamHandle* streamHandle);

  /**
   * Set the stream's volume level
   * @param volume The new volume level between 0.0 and 1.0
   */
  static void aestream_set_volume(void* kodiBase, AEStreamHandle* streamHandle, float volume);

  /**
   * Gets the stream's volume amplification in linear units.
   * @return The volume amplification factor between 1.0 and 1000.0
   */
  static float aestream_get_amplification(void* kodiBase, AEStreamHandle* streamHandle);

  /**
   * Sets the stream's volume amplification in linear units.
   * @param amplify The volume amplification factor between 1.0 and 1000.0
   */
  static void aestream_set_amplification(void* kodiBase,
                                         AEStreamHandle* streamHandle,
                                         float amplify);

  /**
   * Returns the size of one audio frame in bytes (channelCount * resolution)
   * @return The size in bytes of one frame
   */
  static unsigned int aestream_get_frame_size(void* kodiBase, AEStreamHandle* streamHandle);

  /**
   * Returns the number of channels the stream is configured to accept
   * @return The channel count
   */
  static unsigned int aestream_get_channel_count(void* kodiBase, AEStreamHandle* streamHandle);

  /**
   * Returns the stream's sample rate, if the stream is using a dynamic sample
   * rate, this value will NOT reflect any changes made by calls to SetResampleRatio()
   * @return The stream's sample rate (eg, 48000)
   */
  static unsigned int aestream_get_sample_rate(void* kodiBase, AEStreamHandle* streamHandle);

  /**
   * Return the data format the stream has been configured with
   * @return The stream's data format (eg, AE_FMT_S16LE)
   */
  static AudioEngineDataFormat aestream_get_data_format(void* kodiBase,
                                                        AEStreamHandle* streamHandle);

  /**
   * Return the resample ratio
   * @note This will return an undefined value if the stream is not resampling
   * @return the current resample ratio or undefined if the stream is not resampling
   */
  static double aestream_get_resample_ratio(void* kodiBase, AEStreamHandle* streamHandle);

  /**
   * Sets the resample ratio
   * @note This function may return false if the stream is not resampling, if you wish to use this be sure to set the AESTREAM_FORCE_RESAMPLE option
   * @param[in] ratio the new sample rate ratio, calculated by ((double)desiredRate / (double)GetSampleRate())
   */
  static void aestream_set_resample_ratio(void* kodiBase,
                                          AEStreamHandle* streamHandle,
                                          double ratio);
};

} /* namespace ADDON */
} /* extern "C" */
